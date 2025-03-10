/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name:    system_wifi.h
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

#ifndef _SYSTEM_WIFI_H
#define _SYSTEM_WIFI_H

/****************************************************************************
* 	                                        Include files
****************************************************************************/
#include "system_wifi_def.h"
#include "../../components/lwip/lwip-2.1.0/src/apps/dhcpserver/dhcpserver.h"
#include "wifi_sniffer.h"



/****************************************************************************
* 	                                        Macros
****************************************************************************/
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

/****************************************************************************
* 	                                        Types
****************************************************************************/
#ifdef __cplusplus
    extern "C" {
#endif

#define CHANNEL_1_FREQ 2412
#define CHANNEL_14_FREQ 2484

#define CHANNEL_36_FREQ 5180
#define CHANNEL_165_FREQ 5825

#define CHANNEL_2G4_BANDWIDTH(channel) (channel - 1) * 5
#define CHANNEL_5G_BANDWIDTH(channel) (channel - 36) * 5

#define FREQ_TO_CHANNEL_2G4(freq) (freq - CHANNEL_1_FREQ) / 5
#define FREQ_TO_CHANNEL_5G(freq) (freq - CHANNEL_36_FREQ) / 5

typedef enum {
    WIFI_CHANNEL_1 = 1,
    WIFI_CHANNEL_14 = 14,
    WIFI_CHANNEL_36 = 36,
    WIFI_CHANNEL_165 = 165
} wifi_channel_e;

typedef enum {
    WIFI_MODE_STA,           /**< WiFi station mode */
    WIFI_MODE_AP,            /**< WiFi soft-AP mode */
    WIFI_MODE_AP_STA,        /**< WiFi station + soft-AP mode */
    WIFI_MODE_MAX
} wifi_work_mode_e;

typedef enum {
    AUTH_OPEN = 0,      /**< authenticate mode : open */
    AUTH_WEP,           /**< authenticate mode : WEP */
    AUTH_WPA_PSK,       /**< authenticate mode : WPA_PSK */
    AUTH_WPA2_PSK,      /**< authenticate mode : WPA2_PSK */
    AUTH_WPA_WPA2_PSK,  /**< authenticate mode : WPA_WPA2_PSK */
    AUTH_MAX
} wifi_auth_mode_e;

typedef enum {
	CIPHER_NONE = 0,
	CIPHER_WEP40,
	CIPHER_WEP104,
	CIPHER_TKIP,
	CIPHER_CCMP,
} wifi_cipher_mode_e;

typedef enum {
	USER_NV = 0,
	AMT_NV,
	EFUSE,
	OTP,
	DEFAULT_MAC,
} wifi_read_mac_mode_e;

typedef enum {
    REASON_NO_ERROR,
    REASON_USER_DISCONNECT ,
    REASON_BEACON_LOST,
    REASON_AP_DISCONNECT,
    REASON_SCAN_NOT_FOUND,
    REASON_PWD_ERROR,
} wifi_discon_reason_e;

typedef struct {
    uint8_t   ssid[WIFI_SSID_MAX_LEN];
	uint8_t   pwd[WIFI_PWD_MAX_LEN];
	uint8_t   ssid_len;
	uint8_t   pwd_len;
	uint8_t   auth;
	uint8_t   cipher;
	uint8_t  channel;
	uint8_t   bssid[6];
	int8_t    rssi;
} wifi_info_t;

typedef enum {
    // ap status.
    AP_STATUS_STOP,
    AP_STATUS_STARTED,
    
    // sta status.
    STA_STATUS_STOP,
    STA_STATUS_START,
    STA_STATUS_DISCON,
    STA_STATUS_CONNECTED,

    //error status
    STATUS_ERROR
} wifi_status_e;
typedef enum {
    WIFI_WLAN_RADIO_OFF,
    WIFI_WLAN_RADIO_ON
}wifi_rf_contrl;

typedef enum {
    WIFI_FAST_SCAN = 0,                   /**< Do fast scan, scan will end after find SSID match AP */
    WIFI_ALL_CHANNEL_SCAN,                /**< All channel scan, scan will end after scan all the channel */
}wifi_scan_method_e;


typedef struct {
    uint8_t          channel;      /* for record sta work channel.*/
    wifi_status_e    wifi_status;  /* 0: sta connected/ softap started, 1: sta disconnect/ap stoped*/
} wifi_conn_info_t;

typedef struct {
    wifi_conn_info_t info;
    struct netif*    net_if;
    dhcp_status_t    dhcp_stat;
    ip_info_t        ipconfig;
    struct dhcps_lease dhcps_cfg;
} netif_db_t;

typedef struct {
    wifi_work_mode_e mode;
    netif_db_t       netif_db[MAX_IF];
} network_db_t;

typedef struct {
    unsigned char             ssid[WIFI_SSID_MAX_LEN];           
    char             password[WIFI_PWD_MAX_LEN];       
    uint8_t          channel;  //in concurrent mode(softap+sta), if sta is connected, will ignore this configure here, to take sta's channel.
    wifi_auth_mode_e authmode;
    uint8_t 		 max_connect;
	uint8_t			 hidden_ssid;
} wifi_ap_config_t;

typedef struct {
    unsigned char     ssid[WIFI_SSID_MAX_LEN];      /**< SSID of target AP*/
    char     password[WIFI_PWD_MAX_LEN];   /**< password of target AP*/
    uint8_t  channel;
    uint8_t  bssid[6];
} wifi_sta_config_t;

typedef union {
    wifi_ap_config_t  ap;  /* configuration of AP */
    wifi_sta_config_t sta; /* configuration of STA */
} wifi_config_u;

typedef struct {
    uint8_t  sync;
    char     sta_ssid[WIFI_SSID_MAX_LEN];
    char     sta_pwd[WIFI_PWD_MAX_LEN];
    uint8_t  sta_bssid[6];
    uint8_t  channel;
    uint8_t  auto_conn;
} wifi_nv_info_t;

typedef struct {
    uint8_t *ssid;               /* SSID of AP */
    uint8_t *bssid;              /* MAC address of AP */
    uint8_t  channel;            /* channel, scan the specific channel */
    uint8_t  passive;            /* passive slave or not */
    uint8_t  max_item;           /* max scan item */
    int32_t scan_time;          /* scan time per channel, units: millisecond */
} wifi_scan_config_t;

/// Structure containing the information about the PHY channel that was used for this RX
struct wifi_channel_info
{
    /// PHY channel information 1
    uint32_t info1;
    /// PHY channel information 2
    uint32_t info2;
};

typedef void (*scan_done_callback_f)(void);
typedef struct {
    uint8_t              waiting;
    void*    sem;
    scan_done_callback_f cb;
} sync_scan_db_t;

struct softap_sta_info
{
    uint8_t mac_addr[6];
};

/****************************************************************************
* 	                                        Constants
****************************************************************************/

/****************************************************************************
* 	                                        Global  Variables
****************************************************************************/
wifi_nv_info_t* get_wifi_nv_info();

/****************************************************************************
* 	                                        Function Prototypes
****************************************************************************/
struct netif *get_netif_by_index(int idx);
netif_db_t *get_netdb_by_index(int idx);
void set_netif_by_index(struct netif *nif, int idx);
wifi_work_mode_e wifi_get_opmode(void);
void wifi_set_opmode(wifi_work_mode_e opmode);
void wifi_set_ready_flag(bool init_complete_flag);
bool wifi_is_ready(void);
sys_err_t wifi_system_init(void);
char wifi_remove_config_all(int vif);
char wifi_add_config(int vif);
char wifi_config_ssid(int vif, unsigned char *ssid);
char wifi_config_ap_mode(int vif);
char wifi_config_channel(int vif, int channel);
char wifi_config_commit(int vif);
char wifi_config_encrypt(int vif, char *pwd, wifi_auth_mode_e mode);
sys_err_t wifi_sniffer_start(wifi_promiscuous_cb_t cb, wifi_promiscuous_filter_t *filter);
sys_err_t wifi_sniffer_stop(void);
sys_err_t wifi_rf_set_channel(uint8_t channel);
uint8_t wifi_rf_get_channel(void);
sys_err_t wifi_set_password(int vif, char *password);
sys_err_t wifi_set_bssid(uint8_t *bssid);
sys_err_t wifi_get_bssid(unsigned char *mac, wifi_info_t *info);
sys_err_t wifi_set_scan_hidden_ssid(void);
sys_err_t wifi_connect(void);
sys_err_t wifi_disconnect(void);
sys_err_t wifi_get_state(void);
sys_err_t wifi_get_scan_result(unsigned int index, wifi_info_t *info);
sys_err_t wifi_set_mac_addr(int vif_id, uint8_t *mac);
sys_err_t wifi_get_mac_addr(wifi_interface_e vif, unsigned char *mac);
sys_err_t wifi_at_get_mac_addr(wifi_interface_e vif, unsigned char *mac);
sys_err_t wifi_get_ip_addr(wifi_interface_e vif, unsigned int *ip);
sys_err_t wifi_get_mask_addr(wifi_interface_e vif, unsigned int *mask);
sys_err_t wifi_get_gw_addr(wifi_interface_e vif, unsigned int *gw);
sys_err_t wifi_get_dns_addr(unsigned int index, unsigned int *dns);
sys_err_t wifi_get_wifi_info(wifi_info_t *info);
sys_err_t wifi_start_softap(wifi_config_u *config);
sys_err_t wifi_stop_softap(void);
sys_err_t wifi_softap_deauth_sta(const uint8_t *sta_mac, uint16_t reason);
sys_err_t wifi_softap_get_sta_list(struct softap_sta_info *info, uint32_t *num);
sys_err_t wifi_start_station(wifi_config_u *config);
sys_err_t wifi_stop_station(void);
void wifi_set_status(int vif, wifi_status_e status);
wifi_status_e wifi_get_status(int vif);
wifi_status_e wifi_get_ap_status(void);
wifi_status_e wifi_get_sta_status(void);
sys_err_t wifi_send_raw_pkt(const uint8_t *frame, const uint16_t len);
sys_err_t wifi_radio_set(unsigned char on_off);
sys_err_t wifi_chip_powerup(void);
sys_err_t wifi_chip_powerdown(void);
bool check_wifi_link_on(int vif);
unsigned char wifi_system_init_complete(void);
sys_err_t wifi_change_mac_addr(uint8_t *mac);

void wpas_task_main(void *pvParams);

extern int wifi_drv_raw_xmit(uint8_t vif_id, const uint8_t *frame, const uint16_t len);
extern int nrc_transmit_from_8023_mb(uint8_t vif_id,uint8_t **frames, const uint16_t len[], int n_frames);
extern int wpa_get_scan_num(void);
extern sys_err_t wpa_get_scan_result(unsigned int index, wifi_info_t *info);
extern sys_err_t wpa_get_wifi_info(wifi_info_t *info);
extern sys_err_t wpa_update_wifi_rssi(int8_t rssi);
extern void at_get_ap_ssid_passwd_chanel(char *ap_ssid, char *passwd, uint8_t *channel);
extern sys_err_t wifi_set_max_sta(uint16_t max_sta);
void wifi_handle_ap_connect_event(int vif, const uint8_t *addr, uint16_t aid, uint8_t type);

sys_err_t wifi_load_nv_info(wifi_nv_info_t *nv_info);
sys_err_t wifi_load_nv_cfg(void);
sys_err_t wifi_save_nv_info(wifi_nv_info_t *nv_info);
sys_err_t wifi_set_sta_by_nv(wifi_nv_info_t *nv_info);
sys_err_t wifi_clean_nv_info(void);
int wifi_update_nv_sta_start(char *ssid, char *pwd, uint8_t *pmk);
int wifi_update_nv_sta_connected(char *ssid, uint8_t channel);
sys_err_t wifi_event_scan_handler(int vif);
sys_err_t wifi_scan_start(bool block, const wifi_scan_config_t *config);
sys_err_t wifi_set_mac_addr(int vif, uint8_t *mac);
sys_err_t wifi_load_mac_addr(int vif_id, uint8_t *mac);
sys_err_t wifi_get_softap_info(wifi_ap_config_t *softap_info);
sys_err_t wifi_save_ap_nv_info(wifi_ap_config_t *softap_info);
sys_err_t wifi_sta_auto_conn_conf(uint8_t *val,uint8_t type);
sys_err_t wifi_load_ap_nv_info(wifi_ap_config_t *softap_info);
sys_err_t wifi_softap_auto_start(void);
sys_err_t wifi_sta_auto_conn_start(void);

void at_update_netif_mac(int vif, uint8_t *mac);
int get_netif_mac(int vif, uint8_t * mac);
void wpa_update_mac_addr(int vif, unsigned char * mac);
sys_err_t wifi_get_ap_rssi(char *rssi);
sys_err_t wifi_set_ap_sta_num(const unsigned char num);
sys_err_t wifi_get_conn_ap_rssi(char *rssi);
int wifi_set_country_code(const char *p_country_code);
int wifi_get_country_code(char *p_country_code);

int wifi_set_scan_method(wifi_scan_method_e method);
int wifi_get_scan_method(wifi_scan_method_e *method);
#ifdef CONFIG_VNET_SERVICE
void wpa_get_ap_info(char *ap_ssid, char *passwd, uint8_t *channel, wifi_auth_mode_e *auth);
#endif

sys_err_t wifi_request_disconnect(const int vif);

#ifdef __cplusplus
}
#endif
#endif/*_SYSTEM_WIFI_H*/

