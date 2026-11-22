/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KUnit tests for ki5m devfreq compat ops
 */

#include <kunit/test.h>
#include <linux/ki5m_compat.h>
#include <linux/devfreq.h>
#include <linux/device.h>
#include <linux/slab.h>

struct test_device {
	struct device dev;
	struct devfreq *devfreq;
};

static struct test_device *test_dev;

static int test_devfreq_dev_init(struct kunit *test)
{
	test_dev = kzalloc(sizeof(*test_dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, test_dev);
	device_initialize(&test_dev->dev);
	return 0;
}

static void test_devfreq_dev_fini(struct kunit *test)
{
	if (test_dev) {
		if (test_dev->devfreq)
			devfreq_remove_device(test_dev->devfreq);
		put_device(&test_dev->dev);
		kfree(test_dev);
		test_dev = NULL;
	}
}

static int dummy_target(struct device *dev, unsigned long *freq, u32 flags)
{
	return 0;
}

static int dummy_get_cur_freq(struct device *dev, unsigned long *freq)
{
	*freq = 100000000;
	return 0;
}

static struct devfreq_dev_profile test_profile = {
	.target = dummy_target,
	.get_cur_freq = dummy_get_cur_freq,
	.polling_ms = 100,
};

static void test_devfreq_add_remove(struct kunit *test)
{
	int ret;

	/* Create devfreq device */
	test_dev->devfreq = devm_devfreq_add_device(&test_dev->dev,
						    &test_profile,
						    "test_devfreq",
						    NULL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, test_dev->devfreq);

	/* Test remove via compat layer */
	ret = ki5m_devfreq_remove_device(test_dev->devfreq);
	KUNIT_ASSERT_EQ(test, ret, 0);
	test_dev->devfreq = NULL; /* avoid double-free in cleanup */
}

static void test_devfreq_recommended_opp(struct kunit *test)
{
	unsigned long freq = 150000000;
	u32 flags = 0;

	/* This will fail without OPP table, but shouldn't crash */
	int ret = ki5m_devfreq_recommended_opp(&test_dev->dev, &freq, flags);
	/* Either succeeds or returns -ENOENT/-ENODEV - both OK for this test */
	KUNIT_ASSERT_TRUE(test, ret == 0 || ret == -ENOENT || ret == -ENODEV);
}

static struct kunit_case devfreq_test_cases[] = {
	KUNIT_CASE(test_devfreq_add_remove),
	KUNIT_CASE(test_devfreq_recommended_opp),
	{}
};

static struct kunit_suite devfreq_test_suite = {
	.name = "ki5m_devfreq_compat",
	.test_cases = devfreq_test_cases,
	.init = test_devfreq_dev_init,
	.exit = test_devfreq_dev_fini,
};
kunit_test_suite(devfreq_test_suite);