/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KI5m Compatibility Layer - Linux 5.15 Backports
 *
 * Provides backported APIs for folio, timer, workqueue,
 * OPP, devfreq, DMA-BUF, and tracepoints.
 */

#include <linux/ki5m_compat.h>
#include <linux/mm.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/pm_opp.h>
#include <linux/devfreq.h>
#include <linux/sync_file.h>
#include <linux/dma-buf.h>
#include <linux/tracepoint.h>
#include <linux/compound.h>

/* ========================================================================
 * Folio API — backport to struct page
 * Linux 5.16+ introduced folio; 5.15 uses struct page directly.
 * For order-0 pages, folio == page. For higher order, use compound_head.
 * ======================================================================== */

static void ki5m_folio_lock_5_15(struct folio *folio)
{
	lock_page(folio_page(folio));
}

static void ki5m_folio_unlock_5_15(struct folio *folio)
{
	unlock_page(folio_page(folio));
}

static void ki5m_folio_put_5_15(struct folio *folio)
{
	put_page(folio_page(folio));
}

static void ki5m_folio_get_5_15(struct folio *folio)
{
	get_page(folio_page(folio));
}

static struct folio *ki5m_page_folio_5_15(struct page *page)
{
	/* In 5.15, folio == page for order-0; for higher order use compound_head */
	return (struct folio *)compound_head(page);
}

static struct page *ki5m_folio_page_5_15(struct folio *folio)
{
	return (struct page *)folio;
}

static struct page *ki5m_alloc_pages_noprof_5_15(gfp_t gfp, unsigned int order)
{
	return alloc_pages(gfp, order);
}

/* ========================================================================
 * Timer API — use timer_setup + manual lockdep
 * Linux 5.19+ added timer_init_key, timer_delete_sync
 * ======================================================================== */

static void ki5m_timer_init_key_5_15(struct timer_list *timer,
				     void (*callback)(struct timer_list *),
				     unsigned int flags,
				     const char *name,
				     struct lock_class_key *key)
{
	timer_setup(timer, callback, flags);
	lockdep_init_map(&timer->lockdep_map, name, key, 0);
}

static int ki5m_timer_delete_sync_5_15(struct timer_list *timer)
{
	return del_timer_sync(timer);
}

/* ========================================================================
 * Workqueue API — create dedicated percpu workqueue
 * Linux 5.17+ added system_percpu_wq; 6.1+ exports it
 * ======================================================================== */

static struct workqueue_struct *ki5m_alloc_percpu_wq_5_15(const char *name, int flags)
{
	/* 5.15: no system_percpu_wq; create our own unbound workqueue */
	return alloc_workqueue(name, flags | WQ_UNBOUND | WQ_MEM_RECLAIM, 0);
}

static void ki5m_flush_workqueue_5_15(struct workqueue_struct *wq)
{
	flush_workqueue(wq);
}

/* ========================================================================
 * OPP API — static OPP table only in 5.15
 * Linux 5.16+ added dev_pm_opp_add_dynamic / remove_dynamic
 * ======================================================================== */

static int ki5m_dev_pm_opp_add_dynamic_5_15(struct device *dev,
					    unsigned long freq,
					    unsigned long volt)
{
	return dev_pm_opp_add(dev, freq, volt);
}

static int ki5m_dev_pm_opp_remove_dynamic_5_15(struct device *dev,
					       unsigned long freq)
{
	return dev_pm_opp_remove(dev, freq);
}

/* ========================================================================
 * Devfreq API — available in 5.15
 * ======================================================================== */

static int ki5m_devfreq_add_device_5_15(struct devfreq *devfreq)
{
	return devfreq_add_device(devfreq);
}

static int ki5m_devfreq_remove_device_5_15(struct devfreq *devfreq)
{
	return devfreq_remove_device(devfreq);
}

static int ki5m_devfreq_recommended_opp_5_15(struct device *dev,
					     unsigned long *freq,
					     u32 flags)
{
	return devfreq_recommended_opp(dev, freq, flags);
}

/* ========================================================================
 * DMA-BUF / Sync File — available if CONFIG_SYNC_FILE=y
 * ======================================================================== */

static int ki5m_sync_file_create_5_15(struct dma_fence *fence, int flags)
{
	return sync_file_create(fence, flags);
}

static struct dma_fence *ki5m_sync_file_get_fence_5_15(int fd)
{
	return sync_file_get_fence(fd);
}

static int ki5m_dma_buf_fd_5_15(struct dma_buf *dmabuf, int flags)
{
	return dma_buf_fd(dmabuf, flags);
}

static struct dma_buf *ki5m_dma_buf_get_5_15(int fd)
{
	return dma_buf_get(fd);
}

/* ========================================================================
 * Trace/Perf — available if CONFIG_TRACEPOINTS=y
 * ======================================================================== */

static int ki5m_tracepoint_probe_register_5_15(struct tracepoint *tp,
					       void *probe, void *data)
{
	return tracepoint_probe_register(tp, probe, data);
}

static void ki5m_tracepoint_probe_unregister_5_15(struct tracepoint *tp,
						  void *probe, void *data)
{
	tracepoint_probe_unregister(tp, probe, data);
}

/* ========================================================================
 * Ops Structures
 * ======================================================================== */

static const struct ki5m_folio_ops ki5m_folio_ops_5_15 = {
	.folio_lock         = ki5m_folio_lock_5_15,
	.folio_unlock       = ki5m_folio_unlock_5_15,
	.folio_put          = ki5m_folio_put_5_15,
	.folio_get          = ki5m_folio_get_5_15,
	.page_folio         = ki5m_page_folio_5_15,
	.folio_page         = ki5m_folio_page_5_15,
	.alloc_pages_noprof = ki5m_alloc_pages_noprof_5_15,
};

static const struct ki5m_timer_ops ki5m_timer_ops_5_15 = {
	.timer_init_key     = ki5m_timer_init_key_5_15,
	.timer_delete_sync  = ki5m_timer_delete_sync_5_15,
};

static const struct ki5m_wq_ops ki5m_wq_ops_5_15 = {
	.alloc_percpu_wq    = ki5m_alloc_percpu_wq_5_15,
	.flush_workqueue    = ki5m_flush_workqueue_5_15,
};

static const struct ki5m_opp_ops ki5m_opp_ops_5_15 = {
	.dev_pm_opp_add_dynamic    = ki5m_dev_pm_opp_add_dynamic_5_15,
	.dev_pm_opp_remove_dynamic = ki5m_dev_pm_opp_remove_dynamic_5_15,
};

static const struct ki5m_devfreq_ops ki5m_devfreq_ops_5_15 = {
	.devfreq_add_device       = ki5m_devfreq_add_device_5_15,
	.devfreq_remove_device    = ki5m_devfreq_remove_device_5_15,
	.devfreq_recommended_opp  = ki5m_devfreq_recommended_opp_5_15,
};

static const struct ki5m_dma_ops ki5m_dma_ops_5_15 = {
	.sync_file_create  = ki5m_sync_file_create_5_15,
	.sync_file_get_fence = ki5m_sync_file_get_fence_5_15,
	.dma_buf_fd        = ki5m_dma_buf_fd_5_15,
	.dma_buf_get       = ki5m_dma_buf_get_5_15,
};

static const struct ki5m_trace_ops ki5m_trace_ops_5_15 = {
	.tracepoint_probe_register   = ki5m_tracepoint_probe_register_5_15,
	.tracepoint_probe_unregister = ki5m_tracepoint_probe_unregister_5_15,
};

const struct ki5m_compat_ops ki5m_compat_5_15_ops = {
	.folio    = &ki5m_folio_ops_5_15,
	.timer    = &ki5m_timer_ops_5_15,
	.wq       = &ki5m_wq_ops_5_15,
	.opp      = &ki5m_opp_ops_5_15,
	.devfreq  = &ki5m_devfreq_ops_5_15,
	.dma      = &ki5m_dma_ops_5_15,
	.trace    = &ki5m_trace_ops_5_15,
};