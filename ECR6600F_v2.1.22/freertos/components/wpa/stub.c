#include <stdint.h>
#include "driver_wifi_tx.h"
#include "system_event.h"
#include "stub.h"
#include "drivers/driver.h"
#include "ap/hostapd.h"

uint8_t nv_bw40_enable = 0;

const static uint16_t ch_freq_table[] =
{
	2412, 2417, 2422, 2427, 2432,
	2437, 2442, 2447, 2452, 2457,
	2462, 2467, 2472, 2484, 
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

void ap_list_process_beacon(struct hostapd_iface *iface,
			    const struct ieee80211_mgmt *mgmt,
			    struct ieee802_11_elems *elems,
			    struct hostapd_frame_info *fi)
{
}

/*
char * wpa_config_get_no_key(struct wpa_ssid *ssid, const char *var)
{
	return NULL;
}
*/

uint8_t system_modem_api_mac80211_frequency_to_channel(uint32_t frequency)
{
	int i;
	uint8_t channel = 0;

	for (i=0; i<ARRAY_SIZE(ch_freq_table); i++) {
		if (ch_freq_table[i] == frequency) {
			channel = i + 1;
			break;
		}
	}

	return channel;

}

