/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Interface to the Transsion OCP2131 LCD bias voltage controller.
 *
 * Exported by CONFIG_TRANSSION_BIAS_VOLTAGE. Call sites may use
 * IS_ENABLED() to remain linkable when the driver is not built.
 */
#ifndef _LINUX_TRANSSION_BIAS_VOLTAGE_H
#define _LINUX_TRANSSION_BIAS_VOLTAGE_H

int bias_voltage_set_vol(const char *name, int mode);

#endif /* _LINUX_TRANSSION_BIAS_VOLTAGE_H */
