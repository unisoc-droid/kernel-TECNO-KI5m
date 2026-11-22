/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KUnit tests for ki5m folio compat ops
 */

#include <kunit/test.h>
#include <linux/ki5m_compat.h>
#include <linux/mm.h>
#include <linux/slab.h>

static void test_folio_lock_unlock(struct kunit *test)
{
	struct page *page = alloc_page(GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, page);

	struct folio *folio = ki5m_page_folio(page);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, folio);

	/* Lock should succeed */
	ki5m_folio_lock(folio);
	KUNIT_ASSERT_TRUE(test, PageLocked(folio_page(folio)));

	ki5m_folio_unlock(folio);
	KUNIT_ASSERT_FALSE(test, PageLocked(folio_page(folio)));

	__free_page(page);
}

static void test_folio_get_put(struct kunit *test)
{
	struct page *page = alloc_page(GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, page);

	struct folio *folio = ki5m_page_folio(page);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, folio);

	int refcount_before = page_ref_count(folio_page(folio));
	ki5m_folio_get(folio);
	KUNIT_ASSERT_EQ(test, page_ref_count(folio_page(folio)), refcount_before + 1);

	ki5m_folio_put(folio);
	KUNIT_ASSERT_EQ(test, page_ref_count(folio_page(folio)), refcount_before);

	__free_page(page);
}

static void test_alloc_pages_noprof(struct kunit *test)
{
	struct page *page = ki5m_alloc_pages_noprof(GFP_KERNEL, 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, page);

	struct folio *folio = ki5m_page_folio(page);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, folio);

	ki5m_folio_put(folio);
}

static void test_folio_page_roundtrip(struct kunit *test)
{
	struct page *page = alloc_page(GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, page);

	struct folio *folio = ki5m_page_folio(page);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, folio);

	struct page *page_back = ki5m_folio_page(folio);
	KUNIT_ASSERT_PTR_EQ(test, page, page_back);

	__free_page(page);
}

static void test_folio_compound_page(struct kunit *test)
{
	/* Test with order-1 (compound) page */
	struct page *page = alloc_pages(GFP_KERNEL, 1);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, page);

	struct folio *folio = ki5m_page_folio(page);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, folio);

	/* folio_page should return the head page */
	struct page *head = ki5m_folio_page(folio);
	KUNIT_ASSERT_PTR_EQ(test, compound_head(page), head);

	__free_pages(page, 1);
}

static struct kunit_case folio_test_cases[] = {
	KUNIT_CASE(test_folio_lock_unlock),
	KUNIT_CASE(test_folio_get_put),
	KUNIT_CASE(test_alloc_pages_noprof),
	KUNIT_CASE(test_folio_page_roundtrip),
	KUNIT_CASE(test_folio_compound_page),
	{}
};

static struct kunit_suite folio_test_suite = {
	.name = "ki5m_folio_compat",
	.test_cases = folio_test_cases,
};
kunit_test_suite(folio_test_suite);