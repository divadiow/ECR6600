/*
 * Layer2 packet handling for rwnx system
 * Copyright (C) RivieraWaves 2017-2018
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "l2_packet.h"
#include "eloop.h"
#include "wifi_common.h"
#include "system_wifi_def.h"
#include "system_config.h"
#include "net_al.h"
#include "rtos_al.h"
//#include "fhost.h"
//#include "fhost_interface_map.h"

struct l2_packet_data {
	int l2_hdr; /* whether TX buffer already contain L2 (Ethernet) header */
	rtos_queue pkt_queue;
    int idx;
	void (*rx_callback)(void *ctx, const u8 *src_addr,
			    const u8 *buf, size_t len);
	void *rx_callback_ctx;
};

struct l2_packet_data *l2_list[FHOST_VIF_NUM_MAX];

static void l2_packet_receive(void *eloop_ctx, void *buf)
{
	struct l2_packet_data *l2 = eloop_ctx;
	struct mac_eth_hdr *eth;
	uint8_t *payload;
	int len;

    len = *(int *)buf;
	eth = (struct mac_eth_hdr *)(buf + 4);
	payload = buf + 4 + sizeof(struct mac_eth_hdr);
	len -= sizeof(*eth);

	l2->rx_callback(l2->rx_callback_ctx, (u8 *)(&eth->sa),
			payload, len);
    os_free(buf);
}

int trap_l2_pkt(int idx, void *pkt_tmp, int len)
{
    struct l2_packet_data *l2;
    void *pkt;
    struct mac_eth_hdr *eth;

    eth = (struct mac_eth_hdr *)pkt_tmp;
    if (eth->len != 0x8e88) {
        return 1;
    }

    l2 = l2_list[idx % FHOST_VIF_NUM_MAX];
    if (!l2) {
        return 0;
    }

    pkt = os_malloc(len + 4);
    if (!pkt) {
        return 0;
    }
    *(int *)pkt = len;

    memcpy(pkt + 4, pkt_tmp, len);
    //call wpa to process msg
    eloop_register_timeout(0, 0, l2_packet_receive, l2, (void *)pkt);

    return 0;
}

struct l2_packet_data * l2_packet_init(
	const char *ifname, const u8 *own_addr, unsigned short protocol,
	void (*rx_callback)(void *ctx, const u8 *src_addr,
			    const u8 *buf, size_t len),
	void *rx_callback_ctx, int l2_hdr)
{
	struct l2_packet_data *l2;
    //int idx = (uint8_t)atoi(&ifname[2]);
    int idx = wpa_get_drv_idx_by_name(ifname);

	l2 = os_zalloc(sizeof(struct l2_packet_data));
	if (l2 == NULL)
		return NULL;

	l2->rx_callback = rx_callback;
	l2->rx_callback_ctx = rx_callback_ctx;
	l2->l2_hdr = l2_hdr;

	l2->idx = idx;
    l2_list[l2->idx % FHOST_VIF_NUM_MAX] = l2;

    if (rtos_queue_create(sizeof(void *), 4, &l2->pkt_queue)) {
        goto err;
    }
    
	return l2;

err:
	os_free(l2);
	return NULL;
}

struct l2_packet_data * l2_packet_init_bridge(
	const char *br_ifname, const char *ifname, const u8 *own_addr,
	unsigned short protocol,
	void (*rx_callback)(void *ctx, const u8 *src_addr,
			    const u8 *buf, size_t len),
	void *rx_callback_ctx, int l2_hdr)
{
	return l2_packet_init(br_ifname, own_addr, protocol, rx_callback,
			      rx_callback_ctx, l2_hdr);
}

void l2_packet_deinit(struct l2_packet_data *l2)
{
	if (l2 == NULL)
		return;

    l2_list[l2->idx % FHOST_VIF_NUM_MAX] = NULL;
    os_queue_destory(l2->pkt_queue);
	os_free(l2);
}

int l2_packet_get_own_addr(struct l2_packet_data *l2, u8 *addr)
{
	int ret;

	if (l2 == NULL)
		return -1;

    if (0 == l2->idx)
	    ret = hal_system_get_sta_mac(addr);
	else
	    ret = hal_system_get_ap_mac(addr);

	return ret > 0 ? 0 : -1;
}

int l2_packet_send(struct l2_packet_data *l2, const u8 *dst_addr, u16 proto,
		   const u8 *buf, size_t len)
{
	if (l2 == NULL)
		return -1;

	if (l2->l2_hdr)
		dst_addr = NULL;

	net_l2_send(fhost_get_netif_by_idx(l2->idx), buf, len, proto, (void *)dst_addr);

	return 0;
}

int l2_packet_send_by_net_if(void *l2_if, const u8 *dst_addr, u16 proto,
		   const u8 *buf, size_t len)
{
	net_l2_send(l2_if, buf, len, proto, (void *)dst_addr);

	return 0;
}


int l2_packet_get_ip_addr(struct l2_packet_data *l2, char *buf, size_t len)
{
	/* TODO: get interface IP address (This function is not essential) */
	return -1;
}

void l2_packet_notify_auth_start(struct l2_packet_data *l2)
{
	/* This function can be left empty */
}

int l2_packet_set_packet_filter(struct l2_packet_data *l2,
				enum l2_packet_filter_type type)
{
	/* Only needed for advanced AP features */
	return -1;
}
