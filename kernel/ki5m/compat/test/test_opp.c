/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KUnit tests for ki5m OPP compat ops
 */

#include <kunit/test.h>
#include <linux/ki5m_compat.h>
#include <linux/pm_opp.h>
#include <linux/device.h>
#include <linux/slab.h>

/* Mock device for testing */
struct test_device {
	struct device dev;
};

static struct test_device *test_dev;

static int test_dev_init(struct kunit *test)
{
	test_dev = kzalloc(sizeof(*test_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, test_dev);
	device_initialize(&test_dev->dev);
	return 0;
}

static void test_dev_fini(struct kunit *test)
{
	if (test_dev) {
		put_device(&test_dev->dev);
		kfree(test_dev);
		test_dev = NULL;
	}
}

static void test_opp_add_remove_dynamic(struct kunit *test)
{
	unsigned long freq = 100000000; /* 100 MHz */
	unsigned long volt = 900000;    /* 900 mV */

	/* Add OPP dynamically */
	int ret = ki5m_dev_pm_opp_add_dynamic(&test_dev->dev, freq, volt);
	KUNIT_ASSERT_GE(test, ret, 0);

	/* Remove OPP */
	ret = ki5m_dev_pm_opp_remove_dynamic(&test_dev->dev, freq);
	KUNIT_ASSERT_EQ(test, ret, 0);
}

static void test_opp_multiple_frequencies(struct kunit *test)
{
	unsigned long freqs[] = { 100000000, 200000000, 300000000 };
	unsigned long volts[] = { 900000, 950000, 1000000 };
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(freqs); i++) {
		ret = ki5m_dev_pm_opp_add_dynamic(&test_dev->dev, freqs[i], volts[i]);
		KUNIT_ASSERT_GE(test, ret, 0);
	}

	for (i = 0; i < ARRAY_SIZE(freqs); i++) {
		ret = ki5m_dev_pm_opp_remove_dynamic(&test_dev->dev, freqs[i]);
		KUNIT_ASSERT_EQ(test, ret, 0);
	}
}

static struct kunit_case opp_test_cases[] = {
	KUNIT_CASE(test_opp_add_remove_dynamic),
	KUNIT_CASE(test_opp_multiple_frequencies),
	{}
};

static struct kunit_suite opp_test_suite = {
	.name = "ki5m_opp_compat",
	.test_cases = opp_test_cases,
	.init = test_dev_init,
	.exit = test_dev_fini,
};
kunit_test_suite(opp_test_suite);