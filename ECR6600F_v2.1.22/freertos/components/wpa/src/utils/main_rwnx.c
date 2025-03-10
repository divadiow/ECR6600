/*
 * WPA Supplicant / Example program entrypoint
 * Copyright (c) 2003-2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "../../wpa_supplicant/wpa_supplicant_i.h"
#include "config.h"
#include "../../wpa_supplicant/wpas_pmk.h"
#include "system_wifi_def.h"

void wpa_supplicant_main(void *env)
{
	struct wpa_interface iface;
	int exitcode = 0, i = 0;
	struct wpa_global *global;
	struct wpa_supplicant *wpa_s = NULL;

	//os_printf(LM_APP, LL_INFO, "wpa_sup_task alive\n");

	wpas_pmk_info_load();
    
	global = wpa_supplicant_init();
	if (global == NULL) {
		goto end;
	}

    for (i = 0; i < 2; ++i) {
    	os_memset(&iface, 0, sizeof(iface));
    	iface.ifname = i == 0 ? os_strdup(ESWIN_WIFI_IF_NAME_0) : os_strdup(ESWIN_WIFI_IF_NAME_1);
    	wpa_s = wpa_supplicant_add_iface(global, &iface, NULL);
    	if (wpa_s == NULL) {
    		exitcode = -1;
            break;
    	}
    }

	if (exitcode == 0) {
		exitcode = wpa_supplicant_run(global);
	}

	wpa_supplicant_deinit(global);

  end:
	os_task_delete(0);
}


