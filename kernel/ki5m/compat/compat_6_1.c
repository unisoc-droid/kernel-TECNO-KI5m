/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KI5m Compatibility Layer - Linux 6.1+ Native Implementation
 *
 * Direct pass-through to native kernel APIs.
 */

#include <linux/ki5m_compat.h>

/* ========================================================================
 * Folio API — native in 6.1+
 * ======================================================================== */

static void ki5m_folio_lock_6_1(struct folio *folio)
{
	folio_lock(folio);
}

static void ki5m_folio_unlock_6_1(struct folio *folio)
{
	folio_unlock(folio);
}

static void ki5m_folio_put_6_1(struct folio *folio)
{
	folio_put(folio);
}

static void ki5m_folio_get_6_1(struct folio *folio)
{
	folio_get(folio);
}

static struct folio *ki5m_page_folio_6_1(struct page *page)
{
	return page_folio(page);
}

static struct page *ki5m_folio_page_6_1(struct folio *folio)
{
	return folio_page(folio);
}

static struct page *ki5m_alloc_pages_noprof_6_1(gfp_t gfp, unsigned int order)
{
	return alloc_pages_noprof(gfp, order);
}

/* ========================================================================
 * Timer API — native in 6.1+
 * ======================================================================== */

static void ki5m_timer_init_key_6_1(struct timer_list *timer,
				    void (*callback)(struct timer_list *),
				    unsigned int flags,
				    const char *name,
				    struct lock_class_key *key)
{
	timer_init_key(timer, callback, flags, name, key);
}

static int ki5m_timer_delete_sync_6_1(struct timer_list *timer)
{
	return timer_delete_sync(timer);
}

/* ========================================================================
 * Workqueue API — system_percpu_wq available in 6.1+
 * ======================================================================== */

static struct workqueue_struct *ki5m_alloc_percpu_wq_6_1(const char *name, int flags)
{
	/* Use system percpu workqueue for common names, else create our own */
	if (strcmp(name, "events") == 0 || strcmp(name, "kcopyd") == 0)
		return system_percpu_wq; /* Not exported; fallback to alloc */
	return alloc_workqueue(name, flags | WQ_UNBOUND | WQ_MEM_RECLAIM, 0);
}

static void ki5m_flush_workqueue_6_1(struct workqueue_struct *wq)
{
	flush_workqueue(wq);
}

/* ========================================================================
 * OPP API — dynamic OPP native in 6.1+
 * ======================================================================== */

static int ki5m_dev_pm_opp_add_dynamic_6_1(struct device *dev,
					   unsigned long freq,
					   unsigned long volt)
{
	return dev_pm_opp_add_dynamic(dev, freq, volt);
}

static int ki5m_dev_pm_opp_remove_dynamic_6_1(struct device *dev,
					      unsigned long freq)
{
	return dev_pm_opp_remove_dynamic(dev, freq);
}

/* ========================================================================
 * Remaining APIs — native, direct call
 * ======================================================================== */

static const struct ki5m_folio_ops ki5m_folio_ops_6_1 = {
	.folio_lock         = folio_lock,
	.folio_unlock       = folio_unlock,
	.folio_put          = folio_put,
	.folio_get          = folio_get,
	.page_folio         = page_folio,
	.folio_page         = folio_page,
	.alloc_pages_noprof = alloc_pages_noprof,
};

static const struct ki5m_timer_ops ki5m_timer_ops_6_1 = {
	.timer_init_key     = timer_init_key,
	.timer_delete_sync  = timer_delete_sync,
};

static const struct ki5m_wq_ops ki5m_wq_ops_6_1 = {
	.alloc_percpu_wq    = ki5m_alloc_percpu_wq_6_1,
	.flush_workqueue    = flush_workqueue,
};

static const struct ki5m_opp_ops ki5m_opp_ops_6_1 = {
	.dev_pm_opp_add_dynamic    = dev_pm_opp_add_dynamic,
	.dev_pm_opp_remove_dynamic = dev_pm_opp_remove_dynamic,
};

static const struct ki5m_devfreq_ops ki5m_devfreq_ops_6_1 = {
	.devfreq_add_device       = devfreq_add_device,
	.devfreq_remove_device    = devfreq_remove_device,
	.devfreq_recommended_opp  = devfreq_recommended_opp,
};

static const struct ki5m_dma_ops ki5m_dma_ops_6_1 = {
	.sync_file_create  = sync_file_create,
	.sync_file_get_fence = sync_file_get_fence,
	.dma_buf_fd        = dma_buf_fd,
	.dma_buf_get       = dma_buf_get,
};

static const struct ki5m_trace_ops ki5m_trace_ops_6_1 = {
	.tracepoint_probe_register   = tracepoint_probe_register,
	.tracepoint_probe_unregister = tracepoint_probe_unregister,
};

const struct ki5m_compat_ops ki5m_compat_6_1_ops = {
	.folio    = &ki5m_folio_ops_6_1,
	.timer    = &ki5m_timer_ops_6_1,
	.wq       = &ki5m_wq_ops_6_1,
	.opp      = &ki5m_opp_ops_6_1,
	.devfreq  = &ki5m_devfreq_ops_6_1,
	.dma      = &ki5m_dma_ops_6_1,
	.trace    = &ki5m_trace_ops_6_1,
};