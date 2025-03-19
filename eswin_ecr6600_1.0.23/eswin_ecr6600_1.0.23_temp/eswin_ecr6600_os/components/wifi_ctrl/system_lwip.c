/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name:    
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       V1.0
 * Author:        lixiao
 * Date:          2018-12-12
 * History 1:      
 *     Date: 
 *     Version:
 *     Author: 
 *     Modification:  
 * History 2: 
  ********************************************************************************/

/****************************************************************************
* 	                                           Include files
****************************************************************************/
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "netif/conf_wl.h"
#include "wifi_common.h"
//#include "system.h"
#include "FreeRTOS.h"
//#include "standalone.h"
#include "system_wifi.h"
#include "system_event.h"
#include "system_log.h"
#include "os.h"
#include "net_al.h"
//#include "fhost.h"
//#include "fhost_interface_map.h"

/****************************************************************************
* 	                                           Local Macros
****************************************************************************/
#define netifMTU                            (1500)
#define NETIF_ADDRS &ipaddr, &netmask, &gw

/****************************************************************************
* 	                                           Local Types
****************************************************************************/

/****************************************************************************
* 	                                           Local Constants
****************************************************************************/
NETIF_DECLARE_EXT_CALLBACK(lwip_netif_ext_cb)

/****************************************************************************
* 	                                           Local Function Prototypes
****************************************************************************/

/****************************************************************************
* 	                                          Global Constants
****************************************************************************/

/****************************************************************************
* 	                                          Global Variables
****************************************************************************/

/****************************************************************************
* 	                                          Global Function Prototypes
****************************************************************************/
extern network_db_t network_db;
/****************************************************************************
* 	                                          Function Definitions
****************************************************************************/

/*******************************************************************************
 * Function: low_level_output
 * Description: Should do the actual transmission of the packet. The
 *              packet is contained in the pbuf that is passed to the function. This pbuf
 *              might be chained.
 * Parameters: 
 *   Input: netif, pbuf to send.
 *
 *   Output:
 *
 * Returns: ERR_OK when success, others failed.
 *
 *
 * Others: 
 ********************************************************************************/
#ifdef COMPILE_WARNING_OPTIMIZE_WPA
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	struct pbuf *q;
	err_t xReturn = ERR_OK;
	const int MAX_FRAME_NUM = 4;
	uint8_t *frames[MAX_FRAME_NUM];
	uint16_t frame_len[MAX_FRAME_NUM];
	int i = 0;

    if (!check_wifi_link_on(netif->num)) {
        LINK_STATS_INC(link.err);
        return ERR_OK;
    }

#if 0    
    { // for debug.
        debug_rx_tx_info_t info = {1, netif->num};
        lwip_dump_pbuf(p, &info);
    }
#endif

	for(q = p; q != NULL; q = q->next) {
		frames[i] = q->payload;
		frame_len[i] = q->len;
		i++;
	}
//	nrc_transmit_from_8023_mb(netif->num, frames, frame_len, i);
	LINK_STATS_INC(link.xmit);

	return ERR_OK;
}
#endif /* COMPILE_WARNING_OPTIMIZE_WPA */

/*******************************************************************************
 * Function: lwif_input_from_net80211
 * Description: send pkt form wifi to lwip.
 * Parameters: 
 *   Input: vif_id: wifi vif, buffer: pkt, data_len: pkt len.
 *
 *   Output:
 *
 * Returns: 
 *
 *
 * Others: 
 ********************************************************************************/
void lwif_input_from_net80211(uint8_t vif_id, void *buffer, int data_len)
{
	struct eth_hdr      *ethhdr;
	struct netif *netif = get_netif_by_index(vif_id);
	struct pbuf         *p = NULL, *q;
	int remain = data_len;
	int offset = 0;
	int len = data_len;

	p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

	if(p != NULL) {
		for(q = p; q != NULL && remain >= 0; q = q->next) {
			/* Read enough bytes to fill this pbuf in the chain. The
			   available data in the pbuf is given by the q->len variable. */
			os_memcpy(q->payload, (uint8_t*)buffer + offset, q->len);
			remain -= q->len;
			offset += q->len;
		}
		LINK_STATS_INC(link.recv);
	} else {
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		return;
	}

	/* points to packet payload, which starts with an Ethernet header */
	ethhdr = p->payload;

	switch (htons(ethhdr->type)) {
		/* IP or ARP packet? */
		case ETHTYPE_IP:
		case ETHTYPE_ARP:
#if PPPOE_SUPPORT
		/* PPPoE packet? */
		case ETHTYPE_PPPOEDISC:
		case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
		/* full packet send to tcpip_thread to process */
			if (netif->input(p, netif)!=ERR_OK) {
				LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
				pbuf_free(p);
				p = NULL;
			}
			break;

		default:
			pbuf_free(p);
			p = NULL;
			break;
	}
}

/**
 * In this function, the hardware should be initialized.
 * Called from wlif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
err_t low_level_init(struct netif *netif)
{
	//unsigned portBASE_TYPE uxPriority;

	/* set MAC hardware address length */
	netif->hwaddr_len = NETIF_MAX_HWADDR_LEN;

	/* maximum transfer unit */
	netif->mtu = netifMTU;

	/* broadcast capability */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;

	return ERR_OK;
}


/*******************************************************************************
 * Function: wlif_init
 * Description: init netif.
 * Parameters: 
 *   Input:
 *
 *   Output:
 *
 * Returns: 
 *
 *
 * Others: 
 ********************************************************************************/
err_t wlif_init(struct netif *netif)
{

//    os_printf("wlif_init\r\n");
	LWIP_ASSERT("netif != NULL", (netif != NULL));
#if LWIP_NETIF_HOSTNAME
	/* Initialize interface hostname */
	// netif->hostname = "lwip";
	if(netif->hostname == NULL){
		netif->hostname = os_calloc(5,sizeof(char));
		os_memcpy((char *)netif->hostname,"lwip",5);
	}
#endif /* LWIP_NETIF_HOSTNAME */

	/*
	* Initialize the snmp variables and counters inside the struct netif.
	* The last argument should be replaced with your link speed, in units
	* of bits per second.
	*/
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100);

	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;
	/* We directly use etharp_output() here to save a function call.
	* You can instead declare your own function an call etharp_output()
	* from it if you have to do some checks before sending (e.g. if link
	* is available...)
	*/
#if LWIP_IPV4
	netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
	netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
//	netif->linkoutput = low_level_output;

	/* set MAC hardware address length to be used by lwIP */
	netif->hwaddr_len = NETIF_MAX_HWADDR_LEN;

	/* initialize the hardware */
	low_level_init(netif);

	return ERR_OK;
}

static void lwip_change_default_netif(void)
{
    struct netif *netif = NULL;
//    fhost_interface_map_t *map;

//    map = fhost_get_if_map_by_idx(STATION_IF);
    netif = fhost_get_netif_by_idx(STATION_IF);
    if (netif) {
        netif_set_default(netif);
	#ifdef CONFIG_IPV6
		netif_create_ip6_linklocal_address(netif, 1);
		netif_ip6_addr_set_state(netif, 0, IP6_ADDR_VALID);
	#endif
        SYS_LOGD("set sta as default netif");
        if (netif_is_flag_set(netif, NETIF_FLAG_UP)) {
            return;
        }
    }

//    map = fhost_get_if_map_by_idx(SOFTAP_IF);
//    netif = &map->net_if;
    netif = fhost_get_netif_by_idx(SOFTAP_IF);
    if (netif) {
        if (netif_is_flag_set(netif, NETIF_FLAG_UP)) {
            SYS_LOGD("set ap as default netif");
            netif_set_default(netif);
#ifdef CONFIG_IPV6
			netif_create_ip6_linklocal_address(netif, 1);
			netif_ip6_addr_set_state(netif, 0, IP6_ADDR_VALID);
		#endif
        }
    }
}

static void lwip_netif_ext_callback(struct netif* netif, netif_nsc_reason_t reason,
		const netif_ext_callback_args_t* args)
{
    int vif = 0;
    wifi_status_e status;

    vif = netif->num;
    if (!IS_VALID_VIF(vif)) {
        SYS_LOGE("invalid vif, --%s--", __func__);
        return;
    }

    if (reason & LWIP_NSC_STATUS_CHANGED) {
        lwip_change_default_netif();
    }
        
    status = wifi_get_status(vif);
    if (status == AP_STATUS_STOP || status == AP_STATUS_STARTED) {
        return; // not care.
    }

	if (reason & LWIP_NSC_IPV4_SETTINGS_CHANGED) {
		system_event_t evt;
		ip4_addr_t old_ip;

		os_memset(&evt, 0, sizeof(evt));
        evt.vif = netif->num;
		evt.event_info.got_ip.ip_changed = true;
		#ifdef CONFIG_IPV6
		ip4_addr_copy(old_ip, (args->ipv4_changed.old_address->u_addr.ip4));
		#else
		ip4_addr_copy(old_ip, *args->ipv4_changed.old_address);
		#endif
		if (ip4_addr_isany_val(old_ip)
				&& !ip4_addr_isany_val(*netif_ip4_addr(netif))) {
			/* Got IP */
			evt.event_id = SYSTEM_EVENT_STA_GOT_IP;

			#ifdef CONFIG_IPV6
			ip4_addr_copy(evt.event_info.got_ip.ip_info.ip.u_addr.ip4, *netif_ip4_addr(netif)); 
			ip4_addr_copy(evt.event_info.got_ip.ip_info.netmask.u_addr.ip4, *netif_ip4_netmask(netif)); 
			ip4_addr_copy(evt.event_info.got_ip.ip_info.gw.u_addr.ip4, *netif_ip4_gw(netif));
			#else
			ip4_addr_copy(evt.event_info.got_ip.ip_info.ip, *netif_ip4_addr(netif)); 
			ip4_addr_copy(evt.event_info.got_ip.ip_info.netmask, *netif_ip4_netmask(netif)); 
			ip4_addr_copy(evt.event_info.got_ip.ip_info.gw, *netif_ip4_gw(netif));
			#endif
		} 
		else if (!ip4_addr_isany_val(old_ip) && ip4_addr_isany_val(*netif_ip4_addr(netif))) 
		{
			/* Lost IP */
			evt.event_id = SYSTEM_EVENT_STA_LOST_IP;
		} else {
            return;
        }
		sys_event_send(&evt);
	}
}

void lwip_handle_interfaces(void * param)
{
    int i =0 ;
    struct netif *nif = NULL;
//    fhost_interface_map_t *map;
    
    SYS_LOGE("--enter lwip interface...\n");
    for (i = STATION_IF; i < MAX_IF; i++)
    {
//        map = fhost_get_if_map_by_idx(i);
//        nif = &map->net_if;
        nif = fhost_get_netif_by_idx(i);
        if (nif) {
            set_netif_by_index(nif, i);
        }
    }

    //staif = get_netif_by_index(STATION_IF);
    //apif = get_netif_by_index(SOFTAP_IF);

    netif_add_ext_callback(&lwip_netif_ext_cb, lwip_netif_ext_callback);
}

#if 0
/* Initialisation required by lwIP. */
void wifi_lwip_init(void)
{
	/* Init lwIP and start lwIP tasks. */
	tcpip_init(lwip_handle_interfaces, NULL);
    netif_add_ext_callback(&lwip_netif_ext_cb, lwip_netif_ext_callback);
}
#endif

void at_update_netif_mac(int vif, uint8_t *mac)
{
    struct netif *nif = NULL;

    if (IS_VALID_VIF(vif)) {
        nif = get_netif_by_index(vif);
        os_memcpy(nif->hwaddr, mac, NETIF_MAX_HWADDR_LEN);
    }    
}

void update_netif_mac(uint8_t *mac)
{
    int idx = 0;
    struct netif *nif = NULL;

    for (idx = 0; idx < MAX_IF; ++idx) {
        nif = get_netif_by_index(idx);
        mac[5] += idx;
        os_memcpy(nif->hwaddr, mac, NETIF_MAX_HWADDR_LEN);
    }	
}

int get_netif_mac(int vif, uint8_t *mac)
{
    struct netif *nif = NULL;
    
    if (IS_VALID_VIF(vif)) {
        nif = get_netif_by_index(vif);
        os_memcpy(mac, nif->hwaddr, NETIF_MAX_HWADDR_LEN);
    }
    
    return 0;
}

#ifdef ENABLE_LWIP_NAPT
int enable_lwip_napt(int vif, int enable)
{
    struct netif *nif = NULL;
    
    if (IS_VALID_VIF(vif)) {
        nif = get_netif_by_index(vif);
        nif->napt = enable ? 1 : 0;
    }

    return 0;
}
#endif

