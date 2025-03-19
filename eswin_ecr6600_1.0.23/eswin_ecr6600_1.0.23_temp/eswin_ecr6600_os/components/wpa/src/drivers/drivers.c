/*
 * Driver interface list
 * Copyright (c) 2004-2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"
#include "utils/common.h"
#include "driver.h"


const struct wpa_driver_ops *const wpa_drivers[] =
{
#ifdef CONFIG_DRIVER_ESWIN
	&wpa_driver_eswin_ops,
#endif

	NULL
};
