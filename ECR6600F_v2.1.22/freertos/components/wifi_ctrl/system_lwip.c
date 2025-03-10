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
#include "system_config.h"
#include <string.h>
#include "rtos_al.h"
#include "rtos_debug.h"
#ifdef CONFIG_VNET_SERVICE
#include "spi_service_mem.h"
#endif

#define    NET_SEND_PACKET_WAIT_MAX_TIMEMS                              (50)

static net_if_t lwip_net_if[MAX_IF];

static rtos_semaphore l2_semaphore;
static rtos_mutex     l2_mutex;

#define L2_PBUF_SIZE 384
static struct pbuf *l2_pbuf;
static struct pbuf_custom *l2_pbuf_cust;
static struct pbuf_custom *net_send_cust;
static rtos_semaphore net_send_semaphore;
static uint8_t net_send_waiting = 0;



extern void fhost_set_netif_by_idx(uint8_t idx, net_if_t *net_if);
extern uint8_t fhost_tx_check_is_shram(void *p);
extern void fhost_debug_dump_lwip_pbuf(void *pkt, net_if_t *net_if, uint8_t is_tx);
extern int fhost_tx_start(void *net_if, void *net_buf, uint8_t is_raw);
extern void fhost_tx_free(void *buf);
extern int trap_l2_pkt(int vif_idx, void *pkt_tmp, int len);
extern  u16_t lwip_standard_chksum(const void *dataptr, int len);

NETIF_DECLARE_EXT_CALLBACK(lwip_netif_ext_cb)

static void lwip_change_default_netif(void)
{
    struct netif *netif = NULL;

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

    vif = fhost_get_idx_by_netif(netif);
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
        evt.vif = fhost_get_idx_by_netif(netif);
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

/**
 ****************************************************************************************
 * @brief Callback used by the networking stack to push a buffer for transmission by the
 * WiFi interface.
 *
 * @param[in] net_if Pointer to the network interface on which the TX is done
 * @param[in] p_buf  Pointer to the buffer to transmit
 *
 * @return ERR_OK upon successful pushing of the buffer, ERR_BUF otherwise
 ****************************************************************************************
 */
static err_t low_level_output(struct netif *net_if, struct pbuf *p_buf)
{
    err_t status = ERR_BUF;

    //ESWIN ADD judge for PBUF address is normal
    if (!fhost_tx_check_is_shram(p_buf->payload))
    {
        SYS_LOGE("pbuf mem addr[0x%p] error.\n", p_buf->payload);
        pbuf_free(p_buf);
        return ERR_MEM;
    }

    // Increase the ref count so that the buffer is not freed by the networking stack
    // until it is actually sent over the WiFi interface
    pbuf_ref(p_buf);

	fhost_debug_dump_lwip_pbuf(p_buf, net_if, 1);

    // Push the buffer and verify the status
    if (fhost_tx_start(net_if, p_buf, 0) == 0)
    {
#ifdef CONFIG_CUSTOM_FHOSTAPD
        fhost_tx_stats_success();
#endif
        status = ERR_OK;
    }
    else
    {
        fhost_tx_free(p_buf);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Callback used by the networking stack to setup the network interface.
 * This function should be passed as a parameter to netifapi_netif_add().
 *
 * @param[in] net_if Pointer to the network interface to setup
 * @param[in] p_buf  Pointer to the buffer to transmit
 *
 * @return ERR_OK upon successful setup of the interface, other status otherwise
 ****************************************************************************************
 */
static err_t low_level_init(struct netif *net_if)
{
    err_t status = ERR_OK;
    uint8_t *addr = (uint8_t *)net_if->state;

#if LWIP_NETIF_HOSTNAME
    {
        /* Initialize interface hostname */
        net_if->hostname = "wlan";
    }
#endif /* LWIP_NETIF_HOSTNAME */

    net_if->name[ 0 ] = 'w';
    net_if->name[ 1 ] = 'l';

    net_if->output = etharp_output;
#if defined(CONFIG_IPV6)
	net_if->output_ip6 = ethip6_output;
#endif
    net_if->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
    net_if->hwaddr_len = ETHARP_HWADDR_LEN;
    net_if->mtu = 1500;
    net_if->linkoutput = low_level_output;

    os_memcpy(net_if->hwaddr, addr, ETHARP_HWADDR_LEN);
    net_if->state = NULL;

    return status;
}

static int net_l2_init(void)
{
    if (rtos_semaphore_create(&l2_semaphore, true))
    {
        SYS_LOGE("---%s:%d--semaphore_create fail\n",__func__, __LINE__);
    }

    if (rtos_mutex_create(&l2_mutex))
    {
        SYS_LOGE("---%s:%d--mutex_create fail\n",__func__, __LINE__);
    }

    l2_pbuf = pbuf_alloc(PBUF_LINK, L2_PBUF_SIZE, PBUF_RAM);
    if (l2_pbuf == NULL) {
        SYS_LOGE("---%s:%d--l2_pbuf alloc fail\n",__func__, __LINE__);
    }

    l2_pbuf_cust = (struct pbuf_custom*)mem_malloc(LWIP_MEM_ALIGN_SIZE(sizeof(*l2_pbuf_cust)));
    if (l2_pbuf_cust == NULL)
    {
        SYS_LOGE("---%s:%d--l2_pbuf_cust alloc fail\n",__func__, __LINE__);
    }

    return 0;
}

static int net_send_init(void)
{
    if (rtos_semaphore_create(&net_send_semaphore, true))
    {
        SYS_LOGE("---%s:%d--semaphore_create fail\n",__func__, __LINE__);
    }

    net_send_cust = (struct pbuf_custom*)mem_malloc(LWIP_MEM_ALIGN_SIZE(sizeof(*net_send_cust)));
    if (net_send_cust == NULL)
    {
        SYS_LOGE("---%s:%d--l2_pbuf_cust alloc fail\n",__func__, __LINE__);
    }

    return 0;
}

static void lwip_handle_interfaces(void * param)
{
    uint8_t addr[NETIF_MAX_HWADDR_LEN];

    net_l2_init();
    net_send_init();

    fhost_set_netif_by_idx(STATION_IF, &lwip_net_if[STATION_IF]);
    hal_system_get_sta_mac(addr);
    netif_add(&lwip_net_if[STATION_IF],
              NULL, NULL, NULL, addr,
              low_level_init, tcpip_input);
    set_netif_by_index(&lwip_net_if[STATION_IF], STATION_IF);

    fhost_set_netif_by_idx(SOFTAP_IF, &lwip_net_if[SOFTAP_IF]);
    hal_system_get_ap_mac(addr);
    netif_add(&lwip_net_if[SOFTAP_IF],
              NULL, NULL, NULL, addr,
              low_level_init, tcpip_input);
    set_netif_by_index(&lwip_net_if[SOFTAP_IF], SOFTAP_IF);
}

int wifi_net_input(void *net_buf, net_if_t *net_if, 
                    void *addr, uint16_t len, 
                    net_buf_free_fn free_fn)
{
    struct pbuf *p;
    net_buf_rx_t *buf = (net_buf_rx_t *)net_buf;

#ifndef CONFIG_CUSTOM_FHOSTAPD
    struct mac_eth_hdr *eth = (struct mac_eth_hdr *)addr;

    if (eth->len == 0x8e88)
    {
        if (memcmp(eth->da.array, net_if->hwaddr, 6) == 0)
        {
            os_printf(LM_WIFI, LL_INFO, "rx eapol\n");

            if (!trap_l2_pkt(fhost_get_idx_by_netif(net_if), addr, len))
            {
                free_fn(buf);
                return 0;
            }
        }
    }
#endif

    buf->custom_free_function = (pbuf_free_custom_fn)free_fn;
    p = pbuf_alloced_custom(PBUF_RAW, len, PBUF_REF, buf, addr, len);
    if (p == NULL)
    {
        free_fn(buf);
        return -1;
    }

	fhost_debug_dump_lwip_pbuf(p, net_if, 0);

#ifndef CONFIG_CUSTOM_FHOSTAPD
    if (net_if->input(p, net_if))
    {
        free_fn(buf);
        return -2;
    }
#else
    fhost_rx_stats_success();
    fhost_rx_stats_full();
    fhost_rx_stats_empty();
	if (tuya_ethernetif_recv(net_if, p))
    {
        free_fn(buf);
        return -3;
    }
#endif

    return 0;
}

/* Initialisation required by lwIP. */
void wifi_net_init(void)
{
	/* Init lwIP and start lwIP tasks. */
	tcpip_init(lwip_handle_interfaces, NULL);

    netif_add_ext_callback(&lwip_netif_ext_cb, lwip_netif_ext_callback);
}

void at_update_netif_mac(int vif, uint8_t *mac)
{
    struct netif *nif = NULL;

    if (IS_VALID_VIF(vif)) {
        nif = get_netif_by_index(vif);
        os_memcpy(nif->hwaddr, mac, NETIF_MAX_HWADDR_LEN);
    }    
}

#if defined ENABLE_LWIP_NAPT || (CONFIG_SPI_REPEATER && CONFIG_SPI_MASTER)
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

int16_t net_tx_buf_encapsulation_header_len(void)
{
    return PBUF_LINK_ENCAPSULATION_HLEN;
}

void *net_tx_buf_get_encapsulation_header(void *buf)
{
    net_buf_tx_t *net_buf = (net_buf_tx_t *)buf;

    if (pbuf_header(net_buf, PBUF_LINK_ENCAPSULATION_HLEN))
    {
        // Sanity check - we shall have enough space in the buffer
        system_assert(0);
    }

    return net_buf->payload;
}

uint16_t net_tx_buf_get_total_len(void *buf)
{
    net_buf_tx_t *net_buf = (net_buf_tx_t *)buf;

    return net_buf->tot_len;
}

void *net_tx_buf_get_payload(void *buf)
{
    net_buf_tx_t *net_buf = (net_buf_tx_t *)buf;

    return net_buf->payload;
}

uint16_t net_tx_buf_get_len(void *buf)
{
    net_buf_tx_t *net_buf = (net_buf_tx_t *)buf;

    return net_buf->len;
}

void *net_tx_buf_get_next(void *buf)
{
    net_buf_tx_t *net_buf = (net_buf_tx_t *)buf;

    return (void *)net_buf->next;
}

void *net_tx_mem_alloc(uint32_t len)
{
#ifdef CONFIG_VNET_SERVICE
    return spi_service_mem_pool_alloc(SPI_SERVICE_MEM_RX, len);
#else
    return mem_malloc(len);
#endif
}
void net_tx_mem_free(void *mem)
{
#ifdef CONFIG_VNET_SERVICE
    spi_service_mem_pool_free(mem);
#else
    mem_free(mem);
#endif
}

void net_tx_buf_free(void *buf)
{
    if (l2_pbuf != buf)
    {
        // Remove the link encapsulation header
        pbuf_header(buf, -PBUF_LINK_ENCAPSULATION_HLEN);

        // Free the buffer
        pbuf_free(buf);
    }
    else
    {
        os_printf(LM_WIFI, LL_INFO, "tx eapol cfm\n");

        pbuf_header(l2_pbuf, -PBUF_LINK);

        l2_pbuf->next = NULL;
        l2_pbuf->ref  = 1;

        os_sem_post(l2_semaphore);
    }
}

uint16_t net_rx_buf_extra_size(void)
{
    return sizeof(net_buf_rx_t);
}

uint16_t net_ip_chksum(const void *dataptr, int len)
{
    // Simply call the LwIP function
    return lwip_standard_chksum(dataptr, len);
}

const uint8_t *net_if_get_mac_addr(net_if_t *net_if)
{
    return net_if->hwaddr;
}

net_if_t *net_if_find_from_name(const char *name)
{
    return netif_find(name);
}

int net_if_get_name(net_if_t *net_if, char *buf, int *len)
{
    if (!net_if || *len < 3)
        return -1;

    buf[0] = net_if->name[0];
    buf[1] = net_if->name[1];
    buf[2] = net_if->num + '0';
    if ( *len > 3)
        buf[3] = '\0';
    *len = 3;

    return 0;
}

void net_if_up(net_if_t *net_if)
{
    #ifndef CONFIG_CUSTOM_FHOSTAPD
    netifapi_netif_set_up(net_if);
    #endif
}

void net_if_down(net_if_t *net_if)
{
    #ifndef CONFIG_CUSTOM_FHOSTAPD
    netifapi_netif_set_down(net_if);
    #endif
}

void net_if_set_ip(net_if_t *net_if, uint32_t ip, uint32_t mask, uint32_t gw)
{
    if (!net_if)
        return;

    netif_set_addr(net_if, (const ip4_addr_t *)&ip, (const ip4_addr_t *)&mask,
                   (const ip4_addr_t *)&gw);
}

int net_if_get_ip(net_if_t *net_if, uint32_t *ip, uint32_t *mask, uint32_t *gw)
{
    if (!net_if)
        return -1;

    if (ip)
        *ip = netif_ip4_addr(net_if)->addr;
    if (mask)
        *mask = netif_ip4_netmask(net_if)->addr;
    if (gw)
        *gw = netif_ip4_gw(net_if)->addr;

    return 0;
}

static void net_l2_send_cfm(struct pbuf *p)
{
    os_printf(LM_WIFI, LL_INFO, "tx eapol cfm\n");

    if (l2_pbuf != p)
        os_sem_post(l2_semaphore);
}

int net_l2_send(void *l2_if, const uint8_t *data, int data_len, uint16_t ethertype,
                const void *dst_addr_para)
{
    net_if_t *net_if = (net_if_t *)l2_if;
    struct pbuf_custom *pbuf_cust;
    struct pbuf *pbuf;
    err_t status;
    mem_size_t pbuf_data_len = data_len;
    struct mac_addr *dst_addr = (struct mac_addr *)dst_addr_para;

    os_printf(LM_WIFI, LL_INFO, "tx %d eapol...\n", data_len);

    /* In order to implement this function as blocking until the completion of the frame
     * transmission, we make use of the custom pbuf feature, which allows getting an
     * indication when the last user of the buffer is freeing it. Upon such indication
     * we can then signal the semaphore we are waiting on since we pushed the packet
     * to the lower layers */
    //W_DEBUG_I("---%s:%d-- wpa_transt buf addr:0x%x,len=0x%x \n",__func__, __LINE__, data, data_len);
    if (net_if == NULL || data == NULL || data_len >= net_if->mtu)
        return -1;

    if (!dst_addr)
    {
        /* TODO: For now don't support to send L2 packet with header already
           set as ethernet_output always write the ethernet header */
        return -1;
    }

    // Ensure no other thread will program a L2 transmission while this one is waiting
    rtos_mutex_lock(l2_mutex);
 
    if (data_len > L2_PBUF_SIZE)
    {
        // No data in this buffer as the payload is put in the custom pbuf
        pbuf = pbuf_alloc(PBUF_LINK, pbuf_data_len, PBUF_RAM);
        if (pbuf == NULL) {
            SYS_LOGE("---%s:%d-- no mem, len=0x%x \n",__func__, __LINE__, data_len);
            rtos_mutex_unlock(l2_mutex);
             return -1;
        }
    }
    else
    {
        pbuf = l2_pbuf;
        pbuf->tot_len = pbuf_data_len;
        pbuf->len = pbuf_data_len;
    }

    memcpy(pbuf->payload, data, data_len);
    
    pbuf_cust = l2_pbuf_cust;

    pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, pbuf_cust, 0, 0);
    pbuf_cust->custom_free_function = net_l2_send_cfm;

    // Concatenate the two buffers
    pbuf_cat(pbuf, &pbuf_cust->pbuf);

    status = ethernet_output(net_if, pbuf, (struct eth_addr*)net_if->hwaddr,
                             (struct eth_addr*)dst_addr, ethertype);

     // Wait for the transmission completion
    os_sem_wait(l2_semaphore, WAIT_FOREVER);

    if (l2_pbuf != pbuf)
    {
        pbuf_free(pbuf);
    }

    // Now new L2 transmissions are possible
    rtos_mutex_unlock(l2_mutex);

    os_printf(LM_WIFI, LL_INFO, "...tx eapol\n");
 
    return (status == ERR_OK ? 0 : -1);
}

static void net_packet_send_cfm(struct pbuf *p)
{
    os_printf(LM_WIFI, LL_INFO, "tx packet cfm\n");

    if (net_send_waiting && net_send_semaphore) {
        os_sem_post(net_send_semaphore);
    }
}

sys_err_t net_packet_wait_send_done(unsigned int timeoutms)
{
    int ret;

    if (!net_send_semaphore) {
        return -1;
    }

    net_send_waiting = 1;
    ret = os_sem_wait(net_send_semaphore, timeoutms);
    net_send_waiting = 0;
    if (ret == 0) {
        return ERR_OK;
    }

    return -1;
}

int net_packet_send(int idx, const uint8_t *data, int data_len, uint8_t is_raw, uint8_t is_block)
{
    struct pbuf *pbuf;
    err_t status = ERR_BUF;
    mem_size_t pbuf_data_len = data_len;
    net_if_t *net_if;
    struct pbuf_custom *pbuf_cust;

    #ifndef CONFIG_CUSTOM_FHOSTAPD
    net_if = fhost_get_netif_by_idx(idx);
    #else
    int netif_index = 0;
    if(idx == SOFTAP_IF) {
        netif_index = 1;
    }
    net_if = tuya_ethernetif_get_netif_by_index(netif_index);
    if (!net_if)
        return -1;
    #endif

    if (net_if == NULL || data == NULL || ((is_raw == 0) && (data_len > net_if->mtu + ETH_HDR_LEN)))
        return -1;

    // No data in this buffer as the payload is put in the custom pbuf
    pbuf = pbuf_alloc(PBUF_LINK, pbuf_data_len, PBUF_RAM);
    if (pbuf == NULL) {
        return -1;
    }
    memcpy(pbuf->payload, data, data_len);

    if (is_block)
    {
        pbuf_cust = net_send_cust;

        pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, pbuf_cust, 0, 0);
        pbuf_cust->custom_free_function = net_packet_send_cfm;

        // Concatenate the two buffers
        pbuf_cat(pbuf, &pbuf_cust->pbuf);
    }

    if (fhost_tx_start(net_if, pbuf, is_raw) == 0)
    {
        status = ERR_OK;
    }
    else
    {
        // Failed to push message to TX task, call pbuf_free only to decrease ref count
        fhost_tx_free(pbuf);
    }

    if ((status == ERR_OK) && is_block)
    {
        status = net_packet_wait_send_done(NET_SEND_PACKET_WAIT_MAX_TIMEMS);
    }

    return (status == ERR_OK ? 0 : -1);
}

