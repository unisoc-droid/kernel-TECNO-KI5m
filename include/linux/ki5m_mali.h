/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KI5m Mali GPU Platform Operations
 *
 * Interface between Mali kbase core and UMS9230/qogirl6 platform.
 * Implemented by vendor BSP or Xilinx r54p1 + qogirl6 port.
 */

#ifndef _LINUX_KI5M_MALI_H
#define _LINUX_KI5M_MALI_H

#include <linux/types.h>
#include <linux/device.h>

/* ========================================================================
 * Mali GPU PM Operations
 * ======================================================================== */
struct mali_gpu_pm_ops {
	int (*init)(struct device *dev);
	void (*exit)(struct device *dev);
};

/* ========================================================================
 * Clock Trace Provider
 * ======================================================================== */
struct mali_clock_trace_ops {
	int (*get)(unsigned int clk_id);
	void (*put)(unsigned int clk_id);
};

/* ========================================================================
 * PM Callbacks Provider
 * ======================================================================== */
struct mali_pm_callbacks_ops {
	int (*register)(void *cb);
	void (*unregister)(void *cb);
};

/* ========================================================================
 * Devfreq Functions Provider
 * ======================================================================== */
struct mali_devfeq_funcs_ops {
	int (*register)(void *funcs);
};

/* ========================================================================
 * Trusty SMC Shim
 * ======================================================================== */
struct mali_trusty_ops {
	long (*fast_call32)(u32 smc_nr, u32 a0, u32 a1, u32 a2, u32 a3);
};

/* ========================================================================
 * Master Mali Platform Operations
 * ======================================================================== */
struct ki5m_mali_ops {
	const struct mali_gpu_pm_ops	*gpu_pm;
	const struct mali_clock_trace_ops	*clock_trace;
	const struct mali_pm_callbacks_ops	*pm_callbacks;
	const struct mali_devfeq_funcs_ops	*devfeq_funcs;
	const struct mali_trusty_ops	*trusty;
};

/* Get the active Mali platform ops */
const struct ki5m_mali_ops *ki5m_mali_get_ops(void);

/* Register Mali platform ops (called by provider modules) */
int ki5m_mali_register_ops(const struct ki5m_mali_ops *ops);

#endif /* _LINUX_KI5M_MALI_H */