/* SPDX-License-Identifier: GPL-2.0 */
/*
 * KI5m Compatibility Layer - Runtime Initialization
 *
 * Selects the appropriate compat ops based on LINUX_VERSION_CODE.
 */

#include <linux/ki5m_compat.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>

extern const struct ki5m_compat_ops ki5m_compat_5_15_ops;
extern const struct ki5m_compat_ops ki5m_compat_6_1_ops;

static const struct ki5m_compat_ops *g_ki5m_compat;

const struct ki5m_compat_ops *ki5m_compat_get(void)
{
	return g_ki5m_compat;
}

static int __init ki5m_compat_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	g_ki5m_compat = &ki5m_compat_6_1_ops;
	pr_info("ki5m: compat layer initialized for Linux 6.1+ (native APIs)\n");
#else
	g_ki5m_compat = &ki5m_compat_5_15_ops;
	pr_info("ki5m: compat layer initialized for Linux 5.15 (backports active)\n");
#endif
	return 0;
}
arch_initcall(ki5m_compat_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KI5m kernel version compatibility layer");
MODULE_AUTHOR("KI5m Kernel Team");