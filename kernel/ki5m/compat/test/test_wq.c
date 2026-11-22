/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KUnit tests for ki5m workqueue compat ops
 */

#include <kunit/test.h>
#include <linux/ki5m_compat.h>
#include <linux/workqueue.h>

static void wq_callback(struct work_struct *work)
{
	/* No-op callback */
}

static void test_alloc_percpu_wq(struct kunit *test)
{
	struct workqueue_struct *wq = ki5m_alloc_percpu_wq("test_wq", 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, wq);

	/* Verify it's a valid workqueue */
	DEFINE_WORK(work, wq_callback);
	KUNIT_ASSERT_TRUE(test, queue_work(wq, &work));

	flush_workqueue(wq);
	destroy_workqueue(wq);
}

static void test_alloc_percpu_wq_with_flags(struct kunit *test)
{
	struct workqueue_struct *wq = ki5m_alloc_percpu_wq("test_wq_flags", WQ_HIGHPRI);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, wq);

	destroy_workqueue(wq);
}

static void test_flush_workqueue(struct kunit *test)
{
	struct workqueue_struct *wq = ki5m_alloc_percpu_wq("test_flush", 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, wq);

	DEFINE_WORK(work, wq_callback);
	queue_work(wq, &work);

	/* flush_workqueue should not crash */
	ki5m_flush_workqueue(wq);

	destroy_workqueue(wq);
}

static struct kunit_case wq_test_cases[] = {
	KUNIT_CASE(test_alloc_percpu_wq),
	KUNIT_CASE(test_alloc_percpu_wq_with_flags),
	KUNIT_CASE(test_flush_workqueue),
	{}
};

static struct kunit_suite wq_test_suite = {
	.name = "ki5m_wq_compat",
	.test_cases = wq_test_cases,
};
kunit_test_suite(wq_test_suite);