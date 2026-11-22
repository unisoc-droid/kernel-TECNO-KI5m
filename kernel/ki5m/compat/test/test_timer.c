/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KUnit tests for ki5m timer compat ops
 */

#include <kunit/test.h>
#include <linux/ki5m_compat.h>
#include <linux/timer.h>

static void timer_callback(struct timer_list *t)
{
	/* No-op callback */
}

static void test_timer_init_delete(struct kunit *test)
{
	struct timer_list timer;
	struct lock_class_key key;

	ki5m_timer_init_key(&timer, timer_callback, 0, "test_timer", &key);
	KUNIT_ASSERT_NOT_NULL(test, timer.function);

	/* timer_delete_sync should not crash */
	int ret = ki5m_timer_delete_sync(&timer);
	KUNIT_ASSERT_EQ(test, ret, 0);
}

static void test_timer_init_key_name(struct kunit *test)
{
	struct timer_list timer;
	struct lock_class_key key;

	ki5m_timer_init_key(&timer, timer_callback, 0, "custom_name", &key);
	KUNIT_ASSERT_STREQ(test, timer.lockdep_map.name, "custom_name");

	ki5m_timer_delete_sync(&timer);
}

static void test_timer_flags(struct kunit *test)
{
	struct timer_list timer;
	struct lock_class_key key;

	ki5m_timer_init_key(&timer, timer_callback, TIMER_DEFERRABLE, "defer_timer", &key);
	KUNIT_ASSERT_TRUE(test, timer.flags & TIMER_DEFERRABLE);

	ki5m_timer_delete_sync(&timer);
}

static struct kunit_case timer_test_cases[] = {
	KUNIT_CASE(test_timer_init_delete),
	KUNIT_CASE(test_timer_init_key_name),
	KUNIT_CASE(test_timer_flags),
	{}
};

static struct kunit_suite timer_test_suite = {
	.name = "ki5m_timer_compat",
	.test_cases = timer_test_cases,
};
kunit_test_suite(timer_test_suite);