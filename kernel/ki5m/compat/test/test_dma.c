/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KUnit tests for ki5m DMA compat ops
 */

#include <kunit/test.h>
#include <linux/ki5m_compat.h>
#include <linux/dma-buf.h>
#include <linux/sync_file.h>
#include <linux/slab.h>

static void test_dma_buf_get_put(struct kunit *test)
{
	struct dma_buf *dmabuf = dma_buf_export(NULL, NULL, 4096, O_RDWR, 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dmabuf);

	/* Get fd */
	int fd = ki5m_dma_buf_fd(dmabuf, O_CLOEXEC);
	KUNIT_ASSERT_GE(test, fd, 0);

	/* Get back the buffer */
	struct dma_buf *buf = ki5m_dma_buf_get(fd);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, buf);
	KUNIT_ASSERT_PTR_EQ(test, dmabuf, buf);

	dma_buf_put(buf);
	close(fd);
}

static void test_sync_file_create_get(struct kunit *test)
{
	/* Create a dummy fence */
	struct dma_fence *fence = dma_fence_get(dma_fence_put);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, fence);

	/* Create sync file */
	int ret = ki5m_sync_file_create(fence, 0);
	KUNIT_ASSERT_GE(test, ret, 0);
	int fd = ret;

	/* Get fence back */
	struct dma_fence *fence2 = ki5m_sync_file_get_fence(fd);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, fence2);
	KUNIT_ASSERT_PTR_EQ(test, fence, fence2);

	dma_fence_put(fence2);
	close(fd);
	dma_fence_put(fence);
}

static void test_dma_buf_fd_flags(struct kunit *test)
{
	struct dma_buf *dmabuf = dma_buf_export(NULL, NULL, 4096, O_RDWR, 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dmabuf);

	/* Test with O_CLOEXEC */
	int fd = ki5m_dma_buf_fd(dmabuf, O_CLOEXEC);
	KUNIT_ASSERT_GE(test, fd, 0);

	close(fd);
	dma_buf_put(dmabuf);
}

static struct kunit_case dma_test_cases[] = {
	KUNIT_CASE(test_dma_buf_get_put),
	KUNIT_CASE(test_sync_file_create_get),
	KUNIT_CASE(test_dma_buf_fd_flags),
	{}
};

static struct kunit_suite dma_test_suite = {
	.name = "ki5m_dma_compat",
	.test_cases = dma_test_cases,
};
kunit_test_suite(dma_test_suite);