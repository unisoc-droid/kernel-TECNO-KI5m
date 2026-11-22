/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KI5m Kernel Version Compatibility Layer
 *
 * Provides version-agnostic APIs for folio, timer, workqueue,
 * OPP, devfreq, DMA-BUF, and tracepoints.
 *
 * Runtime selection based on LINUX_VERSION_CODE at arch_initcall.
 */

#ifndef _KI5M_COMPAT_H
#define _KI5M_COMPAT_H

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/pm_opp.h>
#include <linux/devfreq.h>
#include <linux/dma-buf.h>
#include <linux/sync_file.h>
#include <linux/tracepoint.h>

/* ========================================================================
 * Folio API (Linux 5.16+)
 * ======================================================================== */
struct ki5m_folio_ops {
	void (*folio_lock)(struct folio *folio);
	void (*folio_unlock)(struct folio *folio);
	void (*folio_put)(struct folio *folio);
	void (*folio_get)(struct folio *folio);
	struct folio *(*page_folio)(struct page *page);
	struct page *(*folio_page)(struct folio *folio);
	struct page *(*alloc_pages_noprof)(gfp_t gfp, unsigned int order);
};

/* ========================================================================
 * Timer API (Linux 5.19+)
 * ======================================================================== */
struct ki5m_timer_ops {
	void (*timer_init_key)(struct timer_list *timer,
			       void (*callback)(struct timer_list *),
			       unsigned int flags,
			       const char *name,
			       struct lock_class_key *key);
	int (*timer_delete_sync)(struct timer_list *timer);
};

/* ========================================================================
 * Workqueue API (Linux 5.17+/6.1+)
 * ======================================================================== */
struct ki5m_wq_ops {
	struct workqueue_struct *(*alloc_percpu_wq)(const char *name, int flags);
	void (*flush_workqueue)(struct workqueue_struct *wq);
};

/* ========================================================================
 * OPP API (Linux 5.16+ dynamic OPP)
 * ======================================================================== */
struct ki5m_opp_ops {
	int (*dev_pm_opp_add_dynamic)(struct device *dev,
				      unsigned long freq,
				      unsigned long volt);
	int (*dev_pm_opp_remove_dynamic)(struct device *dev,
					 unsigned long freq);
};

/* ========================================================================
 * Devfreq API
 * ======================================================================== */
struct ki5m_devfreq_ops {
	int (*devfreq_add_device)(struct devfreq *devfreq);
	int (*devfreq_remove_device)(struct devfreq *devfreq);
	int (*devfreq_recommended_opp)(struct device *dev,
				       unsigned long *freq,
				       u32 flags);
};

/* ========================================================================
 * DMA-BUF / Sync File
 * ======================================================================== */
struct ki5m_dma_ops {
	int (*sync_file_create)(struct dma_fence *fence, int flags);
	struct dma_fence *(*sync_file_get_fence)(int fd);
	int (*dma_buf_fd)(struct dma_buf *dmabuf, int flags);
	struct dma_buf *(*dma_buf_get)(int fd);
};

/* ========================================================================
 * Trace/Perf
 * ======================================================================== */
struct ki5m_trace_ops {
	int (*tracepoint_probe_register)(struct tracepoint *tp,
					 void *probe, void *data);
	void (*tracepoint_probe_unregister)(struct tracepoint *tp,
					    void *probe, void *data);
};

/* ========================================================================
 * Master Compat Operations
 * ======================================================================== */
struct ki5m_compat_ops {
	const struct ki5m_folio_ops	*folio;
	const struct ki5m_timer_ops	*timer;
	const struct ki5m_wq_ops	*wq;
	const struct ki5m_opp_ops	*opp;
	const struct ki5m_devfreq_ops	*devfreq;
	const struct ki5m_dma_ops	*dma;
	const struct ki5m_trace_ops	*trace;
};

/* Get the active compat ops for current kernel version */
const struct ki5m_compat_ops *ki5m_compat_get(void);

/* Convenience macros for drivers */
#define ki5m_folio_lock(folio) \
	ki5m_compat_get()->folio->folio_lock(folio)
#define ki5m_folio_unlock(folio) \
	ki5m_compat_get()->folio->folio_unlock(folio)
#define ki5m_folio_put(folio) \
	ki5m_compat_get()->folio->folio_put(folio)
#define ki5m_folio_get(folio) \
	ki5m_compat_get()->folio->folio_get(folio)
#define ki5m_page_folio(page) \
	ki5m_compat_get()->folio->page_folio(page)
#define ki5m_folio_page(folio) \
	ki5m_compat_get()->folio->folio_page(folio)
#define ki5m_alloc_pages_noprof(gfp, order) \
	ki5m_compat_get()->folio->alloc_pages_noprof(gfp, order)

#define ki5m_timer_init_key(timer, cb, flags, name, key) \
	ki5m_compat_get()->timer->timer_init_key(timer, cb, flags, name, key)
#define ki5m_timer_delete_sync(timer) \
	ki5m_compat_get()->timer->timer_delete_sync(timer)

#define ki5m_alloc_percpu_wq(name, flags) \
	ki5m_compat_get()->wq->alloc_percpu_wq(name, flags)
#define ki5m_flush_workqueue(wq) \
	ki5m_compat_get()->wq->flush_workqueue(wq)

#define ki5m_dev_pm_opp_add_dynamic(dev, freq, volt) \
	ki5m_compat_get()->opp->dev_pm_opp_add_dynamic(dev, freq, volt)
#define ki5m_dev_pm_opp_remove_dynamic(dev, freq) \
	ki5m_compat_get()->opp->dev_pm_opp_remove_dynamic(dev, freq)

#define ki5m_devfreq_add_device(devfreq) \
	ki5m_compat_get()->devfreq->devfreq_add_device(devfreq)
#define ki5m_devfreq_remove_device(devfreq) \
	ki5m_compat_get()->devfreq->devfreq_remove_device(devfreq)
#define ki5m_devfreq_recommended_opp(dev, freq, flags) \
	ki5m_compat_get()->devfreq->devfreq_recommended_opp(dev, freq, flags)

#define ki5m_sync_file_create(fence, flags) \
	ki5m_compat_get()->dma->sync_file_create(fence, flags)
#define ki5m_sync_file_get_fence(fd) \
	ki5m_compat_get()->dma->sync_file_get_fence(fd)
#define ki5m_dma_buf_fd(dmabuf, flags) \
	ki5m_compat_get()->dma->dma_buf_fd(dmabuf, flags)
#define ki5m_dma_buf_get(fd) \
	ki5m_compat_get()->dma->dma_buf_get(fd)

#define ki5m_tracepoint_probe_register(tp, probe, data) \
	ki5m_compat_get()->trace->tracepoint_probe_register(tp, probe, data)
#define ki5m_tracepoint_probe_unregister(tp, probe, data) \
	ki5m_compat_get()->trace->tracepoint_probe_unregister(tp, probe, data)

#endif /* _KI5M_COMPAT_H */