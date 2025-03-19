
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "ethernetif.h"
#include "lwip/netifapi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lwip_dhcpc.h"
#include "cli.h"

#include "tkl_wired.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_system.h"
#include "tkl_spi_ethernet.h"

TKL_WIRED_STATUS_CHANGE_CB ch390_netif_link_chg_cb = NULL;
extern unsigned char ch390_link_status;
extern void tuya_ethernetif_get_ip(const TUYA_NETIF_TYPE net_if_idx, NW_IP_S *ip);
extern void cli_printf(const char *f, ...);

/**
 * @brief  get the link status of wired link
 *
 * @param[out]  is_up: the wired link status is up or not
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_status(TKL_WIRED_STAT_E *status)
{
    struct netif *ch390_netif;

    ch390_netif = tuya_ethernetif_get_netif_by_index(NETIF_ETH_IDX);
    if (NULL == ch390_netif) {
        return OPRT_COM_ERROR;
    }

    if (netif_is_link_up(ch390_netif)) {
        *status = TKL_WIRED_LINK_UP;
    } else {
        *status = TKL_WIRED_LINK_DOWN;
    }

    SPI_LAN_DBG("[LANDBG]%s: wired link status %d", __func__, *status);
    return OPRT_OK;
}

/**
 * @brief  set the status change callback
 *
 * @param[in]   cb: the callback when link status changed
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_status_cb(TKL_WIRED_STATUS_CHANGE_CB cb)
{
    ch390_netif_link_chg_cb = cb;
    SPI_LAN_DBG("[LANDBG]%s: set wired status cb %p", __func__, cb);
    if (ch390_netif_link_chg_cb && ch390_link_status) {
        SPI_LAN_DBG("[LANDBG]%s: set wired status to up", __func__);
        ch390_netif_link_chg_cb(TKL_WIRED_LINK_UP);
    }
    return OPRT_OK;
}

/**
 * @brief  get the ip address of the wired link
 * 
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_ip(NW_IP_S *ip)
{
    tuya_ethernetif_get_ip(NETIF_ETH_IDX, ip);
    //SPI_LAN_DBG("[LANDBG]%s: get wired ip %s mask %s gw %s",
    //    __func__, ip->ip, ip->mask, ip->gw);
    return OPRT_OK;
}

/**
 * @brief  get the mac address of the wired link
 * 
 * @param[in]   mac: the mac address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_get_mac(NW_MAC_S *mac)
{
    struct netif *ch390_netif;
    
    ch390_netif = tuya_ethernetif_get_netif_by_index(NETIF_ETH_IDX);
    if (NULL == ch390_netif) {
        return OPRT_COM_ERROR;
    }

    memcpy(mac->mac, ch390_netif->hwaddr, 6);
    SPI_LAN_DBG("[LANDBG]%s: get wired mac address %02x:%02x:%02x:%02x:%02x:%02x",
        __func__, mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3],
        mac->mac[4], mac->mac[5]);
    return OPRT_OK;
}

/**
 * @brief  set the mac address of the wired link
 * 
 * @param[in]   mac: the mac address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_mac(CONST NW_MAC_S *mac)
{
    return OPRT_NOT_SUPPORTED;
}