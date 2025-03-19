// Copyright 2018-2019 trans-semi inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "wifi_sniffer.h"

static bool promiscuous_en = false;
static wifi_promiscuous_cfg_t  promiscuous_cfg = {{WIFI_PROMIS_FILTER_MASK_ALL}, NULL};

typedef struct _GenericMacHeader {
    // Word 0 : MAC Header Word 0
    // 16 bit Frame Control
	uint16_t    version        : 2;    // default 0
	uint16_t    type           : 2;    // 0: Management , 1: Control,  2: Data,  3: reserved
	uint16_t    subtype        : 4;
	uint16_t    to_ds          : 1;    // to AP
	uint16_t    from_ds        : 1;    // from AP
	uint16_t    more_frag      : 1;
	uint16_t    retry          : 1;
	uint16_t    pwr_mgt        : 1;
	uint16_t    more_data      : 1;
	uint16_t    protect        : 1;
	uint16_t    order          : 1;
    // 16 bit Duration
	uint16_t duration_id;

    // Word 1 ~ Word 5: MAC Header Word 1 ~ MAC Header Word 5(2byte)
	uint8_t     address1[6];
	uint8_t     address2[6];
	uint8_t     address3[6];

	// Word 5 : MAC Header Word 5(2byte)
	uint16_t    fragment_number    : 4;
	uint16_t    sequence_number    : 12;

    // Qos Control Field(16bit) exist only subtype is QoS Data or Qos Null
    // Word 6 : MAC Header Word 6(2Byte)
    union {
        struct {
        	uint16_t    qos_tid                 : 4;
    	    uint16_t    qos_eosp                : 1;
        	uint16_t    qos_ack_policy          : 2;   // 00: normal ack, 01: no ack, 10: no explicit ack or scheduled ack under PSMP, 11: block ack
        	uint16_t    qos_amsdu_present       : 1;   // AMSDU Presence
        	uint16_t    qos_bit8_15             : 8;
            uint8_t     qos_payload[0];
        };
        uint8_t payload[0];
    };
} GenericMacHeader;

int wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb_t cb)
{
	promiscuous_cfg.promisc_cb = cb;
	return 0;
}

extern int fhost_start_monitor(void);
extern int fhost_set_filter(uint32_t filter);
extern int fhost_stop_monitor(void);
int wifi_set_promiscuous(bool en)
{
	if(promiscuous_en != en){
		if(en == true) {
			fhost_start_monitor();
			fhost_set_filter(promiscuous_cfg.filter.filter_mask);
		} else {
			fhost_stop_monitor();
			promiscuous_cfg.promisc_cb = NULL;
			promiscuous_cfg.filter.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
		}
		promiscuous_en = en;
	}
	return 0;
}

int wifi_get_promiscuous(bool *en)
{
	*en = promiscuous_en;
	return 0;
}

int wifi_set_promiscuous_filter(wifi_promiscuous_filter_t *filter)
{
	if(filter == NULL){
		return 1;
	}
	memcpy(&promiscuous_cfg.filter,filter,sizeof(wifi_promiscuous_filter_t));
    if (promiscuous_en)
    {
        fhost_set_filter(filter->filter_mask);
    }
	return 0;
}

int wifi_get_promiscuous_filter(wifi_promiscuous_filter_t *filter)
{
	if(filter == NULL){
		return 1;
	}
	memcpy(filter,&promiscuous_cfg.filter,sizeof(wifi_promiscuous_filter_t)); 
	return 0;
}

int process_promiscuous_frame(void* rx_head, void* mac_head, uint8_t *frame, uint16_t len)
{
	GenericMacHeader *gmh = (GenericMacHeader *)mac_head;
	wifi_promiscuous_pkt_t sniffer_pkt = {0};
	bool match = false;
	wifi_promiscuous_pkt_type_t type = WIFI_PKT_NULL;

    if(!promiscuous_en)
    {
        return -1;
    }
#if CFG_UF
    if (rx_head && !mac_head && !frame && !len)
    {
        if(promiscuous_cfg.promisc_cb != NULL){
    		memcpy(&sniffer_pkt.rx_ctrl, rx_head, 4 * 4); /* only rxvetor1 (4words) */
    		sniffer_pkt.payload = NULL;
    		promiscuous_cfg.promisc_cb(&sniffer_pkt, (sniffer_pkt.rx_ctrl.length_h << 8) | sniffer_pkt.rx_ctrl.length_l, WIFI_PKT_NULL);
    	}
        return 0;
    }
#endif

	if(promiscuous_cfg.filter.filter_mask & WIFI_PROMIS_FILTER_MASK_MGMT){
		if(gmh->type == 0) {
			type = WIFI_PKT_MGMT;
			goto process;
		}
    }

	if(promiscuous_cfg.filter.filter_mask & WIFI_PROMIS_FILTER_MASK_CTRL){
		if(gmh->type == 1) {
			type = WIFI_PKT_CTRL;
			goto process;
		}
    }

	if(promiscuous_cfg.filter.filter_mask & WIFI_PROMIS_FILTER_MASK_DATA){
		if(gmh->type == 2) {
			type = WIFI_PKT_DATA;
			goto process;
		}
    }
	if(match == false){
		goto drop;
	}

process:
	if(promiscuous_cfg.promisc_cb != NULL){
		memcpy(&sniffer_pkt.rx_ctrl, rx_head, 4 * 6); /* rxvetor 6words */
		sniffer_pkt.payload = frame;
		promiscuous_cfg.promisc_cb(&sniffer_pkt,len,type);
		return 0; 
	}

drop:
	return 0;
}

