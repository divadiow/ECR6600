#ifndef __WPAS_PMK_H__
#define __WPAS_PMK_H__

#include "config_ssid.h"
#include "wpa_supplicant_i.h"

void wpas_pmk_info_load(void);

void wpas_pmk_info_save(char *ssid, char *bssid, char *pwd, char *pmk, char *pmkid);

int wpas_pmk_info_refresh(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid);

void wpas_pmk_info_clean(void);

char * wpas_pmkid_get(void);

#endif

