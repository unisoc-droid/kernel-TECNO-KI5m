/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mali kbase Platform Integration for KI5m (UMS9230/qogirl6)
 *
 * This file provides the platform probe/remove hooks that integrate
 * the Mali kbase driver with the KI5m SoC platform providers.
 *
 * The actual Mali kbase core is expected to be provided externally
 * (Xilinx r54p1 or vendor BSP). This file wires up the platform-
 * specific initialization.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/ki5m_compat.h>
#include <linux/ki5m_mali.h>

/* Forward declarations for Mali kbase core */
extern int kbase_device_probe(struct platform_device *pdev);
extern int kbase_device_remove(struct platform_device *pdev);

static const struct ki5m_mali_ops *g_mali_ops;

const struct ki5m_mali_ops *ki5m_mali_get_ops(void)
{
	return g_mali_ops;
}
EXPORT_SYMBOL_GPL(ki5m_mali_get_ops);

/* Called by Mali kbase core during probe to register platform ops */
int ki5m_mali_register_ops(const struct ki5m_mali_ops *ops)
{
	if (!ops)
		return -EINVAL;
	if (g_mali_ops)
		return -EBUSY;
	g_mali_ops = ops;
	pr_info("ki5m: Mali platform ops registered\n");
	return 0;
}
EXPORT_SYMBOL_GPL(ki5m_mali_register_ops);

static int mali_kbase_platform_probe(struct platform_device *pdev)
{
	const struct ki5m_mali_ops *ops = ki5m_mali_get_ops();
	int ret;

	/* Ensure compat layer is initialized */
	if (!ki5m_compat_get())
		return -EPROBE_DEFER;

	/* Initialize Mali GPU PM */
	if (ops && ops->gpu_pm_init) {
		ret = ops->gpu_pm_init(&pdev->dev);
		if (ret)
			return ret;
	}

	/* Register clock trace provider (already at arch_initcall) */

	/* Register PM callbacks */
	if (ops && ops->pm_callbacks_register) {
		/* Provider registered at arch_initcall */
	}

	/* Register devfreq functions */
	if (ops && ops->devfeq_funcs_register) {
		/* Provider registered at arch_initcall */
	}

	/* Standard Mali kbase probe */
	ret = kbase_device_probe(pdev);
	if (IS_ERR_VALUE(ret))
		return ret;

	/* Register Trusty shim */
	if (ops && ops->trusty_fast_call32) {
		/* Available for kbase to use */
	}

	platform_set_drvdata(pdev, (void *)1); /* placeholder */
	return 0;
}

static int mali_kbase_platform_remove(struct platform_device *pdev)
{
	const struct ki5m_mali_ops *ops = ki5m_mali_get_ops();

	kbase_device_remove(pdev);

	if (ops && ops->gpu_pm_exit)
		ops->gpu_pm_exit(&pdev->dev);

	return 0;
}

static const struct of_device_id mali_kbase_of_match[] = {
	{ .compatible = "sprd,mali-natt" },
	{ }
};
MODULE_DEVICE_TABLE(of, mali_kbase_of_match);

static struct platform_driver mali_kbase_platform_driver = {
	.probe  = mali_kbase_platform_probe,
	.remove = mali_kbase_platform_remove,
	.driver = {
		.name = "mali_kbase",
		.of_match_table = mali_kbase_of_match,
	},
};

module_platform_driver(mali_kbase_platform_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mali kbase platform driver for UMS9230 qogirl6");
MODULE_AUTHOR("KI5m Kernel Team");