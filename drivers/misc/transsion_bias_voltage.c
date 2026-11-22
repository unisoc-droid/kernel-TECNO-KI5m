// SPDX-License-Identifier: GPL-2.0-only
/*
 * Transsion OCP2131 LCD bias voltage controller driver.
 *
 * Native Linux 5.15 reimplementation of the stock KI5m
 * transsion_bias_voltage.ko (5.4.254-android12). Behavioral reference:
 * original binary SHA-256
 * d7612ed1bf3c6e61b0499d0a0bbddd58f77d607862842dfa4737bbfa26249864
 *
 * Verified hardware contract (from static RE + read-only live device data):
 *  - I2C slave 0x3e, child node "ocp2131", chip ID 0x51 at register 0x03
 *  - AVDD register: value of DT property "bias_avdd_reg" (stock: 0)
 *  - AVEE register: value of DT property "bias_avee_reg" (stock: 1)
 *  - Voltage conversion: ((mV - 4000) * 0x51eaa539) >> 37  (unsigned 32x32->64)
 *  - I2C write: 2-byte buffer [reg, val] via i2c_transfer_buffer_flags()
 *  - Read-back skip-check: read AVDD register; skip programming if it
 *    already equals the computed value
 *  - GPIO: "bias,avdd-gpio" (active high), "bias,avee-gpio" (active low in
 *    the enable sequence), raw output via gpiod_direction_output_raw()
 *
 * The original driver exposed bias_voltage_set_vol(const char *name, int mode)
 * to sprd_drm. This driver preserves that exported symbol so a consumer can
 * be ported without changing its call sites. The hardware has a single bias
 * controller; a single global device pointer is therefore used, guarded by a
 * mutex, matching the stock driver's global state model.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio/consumer.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/transsion_bias_voltage.h>

#define OCP2131_CHIP_ID		0x51
#define OCP2131_ID_REG		0x03
#define OCP2131_VOLTAGE_BASE	4000U
#define OCP2131_VOLTAGE_MULT	0x51eaa539ULL
#define OCP2131_VOLTAGE_SHIFT	37

/* Mode argument values used by the stock sprd_drm caller:
 *   mode 1 -> sprd_panel_prepare()   (bias on)
 *   mode 0 -> sprd_panel_unprepare() (bias off)
 */
#define BIAS_MODE_OFF		0
#define BIAS_MODE_ON		1
#define BIAS_MODE_ALT		2

struct ocp2131_bias {
	struct i2c_client *client;
	struct gpio_desc *avdd_gpio;
	struct gpio_desc *avee_gpio;
	u32 voltage_mv;
	u32 init_delay_ms;
	u32 suspend_delay_ms;
	u8 avdd_reg;
	u8 avee_reg;
	/* Stock global mode tracking (rodata "lcm"/"" name paths). */
	u32 tp_mode_state;
	u32 lcm_mode_state;
	struct mutex lock;
};

/* Single-controller hardware; protected by bias->lock (see probe). */
static struct ocp2131_bias *bias_dev;

/*
 * Reproduce the stock delay loop. Stock executes
 * __const_udelay(0x41008958) once per millisecond of the DT delay value;
 * on this platform that constant is calibrated to ~1 ms, so fsleep(1000)
 * is the portable equivalent. Preserves delay placement exactly.
 */
static void bias_delay_ms(unsigned int ms)
{
	while (ms--)
		fsleep(1000);
}

/*
 * Read one byte from an OCP2131 register using the stock 2-message
 * transaction: write the register address, then read the value.
 * Returns the byte value (0..255) or negative errno.
 */
static int bias_read_reg(struct i2c_client *client, u8 reg)
{
	struct i2c_msg msgs[2];
	u8 txbuf = reg;
	u8 rxbuf = 0;
	int ret;

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &txbuf;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = &rxbuf;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
		return ret;
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	return rxbuf;
}

/*
 * Write one register. Exactly the stock format: 2-byte [reg, value] buffer
 * sent with i2c_transfer_buffer_flags(client, buf, 2, 0).
 */
static int bias_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	u8 buf[2] = { reg, val };
	int ret;

	ret = i2c_transfer_buffer_flags(client, buf, ARRAY_SIZE(buf), 0);
	if (ret < 0)
		return ret;
	if (ret != ARRAY_SIZE(buf))
		return -EIO;

	return 0;
}

/* Exact stock voltage arithmetic: ((mV - 4000) * 0x51eaa539) >> 37. */
static u8 bias_voltage_to_reg(unsigned int voltage_mv)
{
	u64 product;

	/* Unsigned 32-bit subtraction, zero-extended to 64-bit (matches
	 * "sub w8, w1, #4000" then "umull x8, w8, w9" in the original). */
	product = (u64)(u32)(voltage_mv - OCP2131_VOLTAGE_BASE) *
		  OCP2131_VOLTAGE_MULT;

	return (u8)(product >> OCP2131_VOLTAGE_SHIFT);
}

/*
 * Reproduce the stock mode-decision logic (from bias_voltage_set_vol
 * offsets 0x168-0x348). Returns two flags:
 *   do_write1 -> program AVDD register (stock I2C write site 0x50c)
 *   do_write2 -> program AVEE register (stock I2C write site 0x538)
 * The register addresses themselves are constant (DT-provided); the flags
 * only select which writes execute, exactly as in stock.
 */
static void bias_mode_flags(struct ocp2131_bias *bias, const char *name,
			    int mode, unsigned int max_mode,
			    bool *do_write1, bool *do_write2)
{
	unsigned int current;

	if (!strcmp(name, "lcm")) {
		/* Stock name=="lcm" path: lcm_mode_state = mode. */
		current = bias->tp_mode_state;
		bias->lcm_mode_state = mode;
	} else {
		/* Stock name=="" path: tp_mode_state = mode. */
		current = bias->lcm_mode_state;
		bias->tp_mode_state = mode;
	}

	*do_write1 = false;
	*do_write2 = false;

	if (current == 0) {
		if (max_mode == 1) {
			*do_write1 = true;
			*do_write2 = true;
		} else if (max_mode == 2) {
			*do_write2 = true;
		}
	} else if (current == 1) {
		if (max_mode == 1) {
			/* both flags zero: GPIO-only transition */
		} else if (max_mode == 2) {
			*do_write2 = true;
		}
	} else if (current == 2) {
		if (max_mode == 1) {
			*do_write2 = true;
		} else if (max_mode == 2) {
			*do_write1 = true;
		} else {
			*do_write1 = true;
			*do_write2 = true;
		}
	}
	/* current >= 3: stock computes zero flags for reachable inputs. */
}

/*
 * The exported interface used by sprd_drm.
 *   name = "lcm" (panel bias) or "" (touch-panel bias state)
 *   mode = 1 enable, 0 disable (2 used by stock internal state machine)
 */
int bias_voltage_set_vol(const char *name, int mode)
{
	struct ocp2131_bias *bias = bias_dev;
	unsigned int max_mode;
	unsigned int voltage_reg;
	bool do_write1, do_write2;
	unsigned int current_mode;
	int readback;
	int ret;

	if (!bias || !bias->client)
		return -ENODEV;
	if (!name || mode < 0 || mode > BIAS_MODE_ALT)
		return -EINVAL;

	mutex_lock(&bias->lock);

	/* Stock uses max(tp_mode_state, requested mode) at 0x17c/0x1c0. */
	current_mode = strcmp(name, "lcm") ? bias->lcm_mode_state :
		       bias->tp_mode_state;
	max_mode = max_t(unsigned int, current_mode, (unsigned int)mode);

	bias_mode_flags(bias, name, mode, max_mode, &do_write1, &do_write2);

	if (max_mode == BIAS_MODE_ALT) {
		/* Stock MODE 2 branch (0x380): AVDD high, AVEE low, using
		 * suspend_delayms for both steps. */
		gpiod_direction_output_raw(bias->avdd_gpio, 1);
		if (do_write1)
			bias_delay_ms(bias->suspend_delay_ms);
		gpiod_direction_output_raw(bias->avee_gpio, 0);
		if (do_write2)
			bias_delay_ms(bias->suspend_delay_ms);
	} else if (max_mode == BIAS_MODE_ON) {
		/* Stock MODE 1 branch (0x428): AVDD high, AVEE high, using
		 * init_delayms for both steps. */
		gpiod_direction_output_raw(bias->avdd_gpio, 1);
		if (do_write1)
			bias_delay_ms(bias->init_delay_ms);
		gpiod_direction_output_raw(bias->avee_gpio, 1);
		if (do_write2)
			bias_delay_ms(bias->init_delay_ms);
	} else {
		/* Stock mode 0 path (0x210): power down, AVEE low then AVDD
		 * low, using suspend_delayms. No I2C voltage writes. */
		gpiod_direction_output_raw(bias->avee_gpio, 0);
		bias_delay_ms(bias->suspend_delay_ms);
		gpiod_direction_output_raw(bias->avdd_gpio, 0);
		bias_delay_ms(bias->suspend_delay_ms);
		goto out;
	}

	/* Voltage conversion (computed before any write, like stock 0x494). */
	voltage_reg = bias_voltage_to_reg(bias->voltage_mv);

	/* Stock read-back skip-check at 0x4c4-0x4e0: read AVDD register; if it
	 * already holds the desired value, skip all I2C programming. */
	readback = bias_read_reg(bias->client, bias->avdd_reg);
	if (readback < 0) {
		ret = readback;
		dev_err(&bias->client->dev,
			"AVDD register read failed: %d\n", ret);
		goto out;
	}
	if ((u8)readback == (u8)voltage_reg)
		goto out;	/* [drm][bias] regdata already set return ---- */

	/* Write #1 (stock 0x50c): AVDD register = voltage value. */
	if (do_write1) {
		ret = bias_write_reg(bias->client, bias->avdd_reg, voltage_reg);
		if (ret) {
			dev_err(&bias->client->dev,
				"AVDD register write failed: %d\n", ret);
			goto out;
		}
	}

	/* Write #2 (stock 0x538): AVEE register = voltage value. */
	if (do_write2) {
		ret = bias_write_reg(bias->client, bias->avee_reg, voltage_reg);
		if (ret) {
			dev_err(&bias->client->dev,
				"AVEE register write failed: %d\n", ret);
			goto out;
		}
	}

	/* Write #3 (stock 0x5ac): AVDD register = voltage value again.
	 * Stock executes this unconditionally once the skip-check passes. */
	ret = bias_write_reg(bias->client, bias->avdd_reg, voltage_reg);
	if (ret)
		dev_err(&bias->client->dev,
			"AVDD final write failed: %d\n", ret);

out:
	mutex_unlock(&bias->lock);
	return 0;
}
EXPORT_SYMBOL_GPL(bias_voltage_set_vol);

static int ocp2131_bias_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct device_node *np = client->dev.of_node;
	struct device_node *child;
	struct ocp2131_bias *bias;
	u32 tmp;
	int ret;

	if (!np)
		return -ENODEV;

	bias = devm_kzalloc(&client->dev, sizeof(*bias), GFP_KERNEL);
	if (!bias)
		return -ENOMEM;

	bias->client = client;
	mutex_init(&bias->lock);

	/* Stock uses of_get_child_by_name(np, "ocp2131"). */
	child = of_get_child_by_name(np, "ocp2131");
	if (!child) {
		dev_err(&client->dev, "ocp2131 child node not found\n");
		return -ENODEV;
	}

	/* Defaults match stock DT (6000 mV, 5 ms, 5 ms); register indices
	 * are mandatory in practice but fall back to stock 0/1 if absent. */
	bias->voltage_mv = 6000;
	bias->init_delay_ms = 5;
	bias->suspend_delay_ms = 5;
	bias->avdd_reg = 0;
	bias->avee_reg = 1;

	of_property_read_u32(child, "bias_voltage", &bias->voltage_mv);
	of_property_read_u32(child, "bias_init_delayms", &bias->init_delay_ms);
	of_property_read_u32(child, "bias_suspend_delayms",
			     &bias->suspend_delay_ms);

	ret = of_property_read_u32(child, "bias_avdd_reg", &tmp);
	if (!ret)
		bias->avdd_reg = (u8)tmp;
	ret = of_property_read_u32(child, "bias_avee_reg", &tmp);
	if (!ret)
		bias->avee_reg = (u8)tmp;

	of_node_put(child);

	/* GPIOs: use the exact stock DT property names ("bias,avdd-gpio" /
	 * "bias,avee-gpio", singular). devm_gpiod_get_from_of_node() is the
	 * native descriptor API that accepts an explicit property name, so no
	 * integer GPIO manipulation is needed. GPIOD_ASIS matches stock's
	 * of_get_named_gpio_flags() + gpiod_direction_output_raw() usage. */
	bias->avdd_gpio = devm_gpiod_get_from_of_node(&client->dev, np,
						"bias,avdd-gpio", 0,
						GPIOD_ASIS, "bias-avdd");
	if (IS_ERR(bias->avdd_gpio)) {
		ret = PTR_ERR(bias->avdd_gpio);
		dev_err(&client->dev, "failed to get AVDD GPIO: %d\n", ret);
		return ret;
	}
	bias->avee_gpio = devm_gpiod_get_from_of_node(&client->dev, np,
						"bias,avee-gpio", 0,
						GPIOD_ASIS, "bias-avee");
	if (IS_ERR(bias->avee_gpio)) {
		ret = PTR_ERR(bias->avee_gpio);
		dev_err(&client->dev, "failed to get AVEE GPIO: %d\n", ret);
		return ret;
	}

	/* Stock validates chip ID by reading 2 bytes from register 0x03 and
	 * comparing the first byte with 0x51 (bias_probe 0xa80-0xab8). */
	ret = bias_read_reg(client, OCP2131_ID_REG);
	if (ret < 0) {
		dev_err(&client->dev, "chip ID read failed: %d\n", ret);
		return ret;
	}
	if (ret != OCP2131_CHIP_ID) {
		dev_err(&client->dev,
			"unsupported OCP2131 chip ID 0x%02x (expected 0x%02x)\n",
			ret, OCP2131_CHIP_ID);
		return -ENODEV;
	}

	i2c_set_clientdata(client, bias);
	bias_dev = bias;

	dev_info(&client->dev,
		 "OCP2131 bias: voltage=%umV reg=0x%02x/0x%02x ID=0x%02x\n",
		 bias->voltage_mv, bias->avdd_reg, bias->avee_reg, ret);
	return 0;
}

static int ocp2131_bias_remove(struct i2c_client *client)
{
	struct ocp2131_bias *bias = i2c_get_clientdata(client);

	if (bias_dev == bias)
		bias_dev = NULL;
	mutex_destroy(&bias->lock);
	return 0;
}

static const struct i2c_device_id ocp2131_bias_id[] = {
	{ "transsion_bias_voltage", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ocp2131_bias_id);

static const struct of_device_id ocp2131_bias_of_match[] = {
	{ .compatible = "transsion,bias_i2c", },
	{ }
};
MODULE_DEVICE_TABLE(of, ocp2131_bias_of_match);

static struct i2c_driver ocp2131_bias_driver = {
	.driver = {
		.name		= "transsion_bias_voltage",
		.of_match_table	= ocp2131_bias_of_match,
	},
	.probe		= ocp2131_bias_probe,
	.remove		= ocp2131_bias_remove,
	.id_table	= ocp2131_bias_id,
};
module_i2c_driver(ocp2131_bias_driver);

MODULE_AUTHOR("KI5m native 5.15 port");
MODULE_DESCRIPTION("Transsion OCP2131 LCD bias voltage driver");
MODULE_LICENSE("GPL v2");
