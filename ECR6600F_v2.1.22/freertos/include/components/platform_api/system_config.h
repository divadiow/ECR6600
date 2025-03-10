/**
 * @file system_config.h
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyafeng
 * @date 2021-6-17
 * @version V0.1
 * @par Copyright by http://eswin.com/.
 * @par History 1:
 *      Date:
 *      Version:
 *      Author:
 *      Modification:
 *
 * @par History 2:
 */


#ifndef _SYSTEM_CONFIG_H
#define _SYSTEM_CONFIG_H


/*--------------------------------------------------------------------------
* 	                                        	Include files
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                        	Macros
--------------------------------------------------------------------------*/
/** Description of the macro */
typedef enum {
    MAC_SAVE_TO_AMT = 0,
	MAC_SAVE_TO_OTP = 1,
    MAC_SAVE_TO_EFUSE = 2,
    MAC_SAVE_MAX
} MAC_SAVE_LACATION;

typedef enum {
	MAC_STA = 0,
	MAC_AP = 1,
	MAC_BLE = 2,
	MAC_MAX
} MAC_TYPE;

//mac
#define STA_MAC 						    "mac"
#define AP_MAC 							"ap_mac"
#define BLE_MAC 						"ble_mac"

#define MAC_ADDR_IN_OTP					0

//amt-nv
#define AMT_NV_FLAG	                    	"amt"
#define AMT_TX_POWER 			    	    AMT_NV_FLAG"txPower"
#define AMT_TX_GAIN_FLAG 				    AMT_NV_FLAG"txGainFlag"
#define AMT_FINISH_FLAG                 AMT_NV_FLAG"FinishFlag"

	
//customer
#define CUSTOMER_NV_FLAG	        	    "cus"
#define CUSTOMER_NV_WIFI_STA_SSID       	CUSTOMER_NV_FLAG"StaSSID"
#define CUSTOMER_NV_WIFI_STA_PWD        	CUSTOMER_NV_FLAG"StaPW"
#define CUSTOMER_NV_WIFI_STA_BSSID      	CUSTOMER_NV_FLAG"StaBSSID"
#define CUSTOMER_NV_WIFI_STA_PMK        	CUSTOMER_NV_FLAG"PMK"
#define CUSTOMER_NV_WIFI_STA_CHANNEL 	    CUSTOMER_NV_FLAG"StaChannel"
#define CUSTOMER_NV_WIFI_AUTO_CONN  	    CUSTOMER_NV_FLAG"StaAutoConn"
#define CUSTOMER_NV_WIFI_OP_MODE            CUSTOMER_NV_FLAG"OpMode"

#define CUSTOMER_NV_WIFI_AP_SSID            CUSTOMER_NV_FLAG"ApSSID"
#define CUSTOMER_NV_WIFI_AP_PWD             CUSTOMER_NV_FLAG"ApPW"
#define CUSTOMER_NV_WIFI_AP_CHANNEL         CUSTOMER_NV_FLAG"ApChannel"
#define CUSTOMER_NV_WIFI_AP_AUTH            CUSTOMER_NV_FLAG"ApAuth"
#define CUSTOMER_NV_WIFI_MAX_CON            CUSTOMER_NV_FLAG"MaxCon"
#define CUSTOMER_NV_WIFI_HIDDEN_SSID        CUSTOMER_NV_FLAG"HiddenSSID"

#define CUSTOMER_NV_WIFI_COUNTRYCODE  	    CUSTOMER_NV_FLAG"CountryCode"
#define CUSTOMER_NV_WIFI_WPAS_PMKINFO 	    CUSTOMER_NV_FLAG"wpas_pmk_info"
#define CUSTOMER_NV_WIFI_WIRELESSMODE       CUSTOMER_NV_FLAG"WirelessMode"

#define CUSTOMER_NV_BLE_DEVICE_NAME    		CUSTOMER_NV_FLAG"BleDeviceName"
#define CUSTOMER_NV_BLE_ROLE                CUSTOMER_NV_FLAG"BleRole"
#define CUSTOMER_NV_BLE_ADVMODE             CUSTOMER_NV_FLAG"BleAdvMode"

//#define CUSTOMER_NV_BLE_BD_ADDR			    CUSTOMER_NV_FLAG"BleMacAddr"

	  //sntp
#define CUSTOMER_NV_SNTP_TIMEZONE           CUSTOMER_NV_FLAG"SntpTimeZone"
#define CUSTOMER_NV_SNTP_UPDATEPERIOD       CUSTOMER_NV_FLAG"SntpUpdatePeriod"
#define CUSTOMER_NV_SNTP_SERVER             CUSTOMER_NV_FLAG"SntpServer"
#define CUSTOMER_NV_SNTP_INCREASETIME       CUSTOMER_NV_FLAG"SntpIncreaseTime"

	  //PSM
#define CUSTOMER_NV_PSM_FLAG			    CUSTOMER_NV_FLAG"PsmFlag"
#define CUSTOMER_NV_PSM_TIME			    CUSTOMER_NV_FLAG"PsmDeepSleepTime"	//0-100 /ms
#define CUSTOMER_NV_PSM_POR_ENABLE          CUSTOMER_NV_FLAG"PsmPOREnable"
#define CUSTOMER_NV_PSM_CW           		CUSTOMER_NV_FLAG"PsmCw"
#define CUSTOMER_NV_PSM_WAIT_BEA_THR        CUSTOMER_NV_FLAG"PsmWatiBea"
#define CUSTOMER_NV_PSM_EN_CW_VAL           CUSTOMER_NV_FLAG"PsmEnCwVal"
#define CUSTOMER_NV_PSM_EN_CW_LOST_BEA_THR  CUSTOMER_NV_FLAG"PsmEnCwLstBea"


      //IP
#define CUSTOMER_NV_WIFI_DHCPS_EN           CUSTOMER_NV_FLAG"DHCPS_EN"
#define CUSTOMER_NV_WIFI_DHCPC_EN           CUSTOMER_NV_FLAG"DHCPC_EN"
#define CUSTOMER_NV_WIFI_DHCPS_IP           CUSTOMER_NV_FLAG"DHCPS_IP"
#define CUSTOMER_NV_WIFI_DHCPS_LEASE_TIME   CUSTOMER_NV_FLAG"DHCPS_LeaseTime"
#define CUSTOMER_NV_WIFI_AP_IP              CUSTOMER_NV_FLAG"AP_IP"
#define CUSTOMER_NV_WIFI_AP_GATEWAY         CUSTOMER_NV_FLAG"AP_gateway"
#define CUSTOMER_NV_WIFI_AP_NETMASK         CUSTOMER_NV_FLAG"AP_netmask"
#define CUSTOMER_NV_WIFI_STA_IP             CUSTOMER_NV_FLAG"STA_IP"
#define CUSTOMER_NV_WIFI_STA_GATEWAY        CUSTOMER_NV_FLAG"STA_gateway"
#define CUSTOMER_NV_WIFI_STA_NETMASK        CUSTOMER_NV_FLAG"STA_netmask"
#define CUSTOMER_NV_WIFI_STA_MAC            CUSTOMER_NV_FLAG"STA_MAC"
#define CUSTOMER_NV_TRANSLINK_EN            CUSTOMER_NV_FLAG"TransLinkEn"
#define CUSTOMER_NV_TRANSLINK_IP            CUSTOMER_NV_FLAG"TransLinkIP"
#define CUSTOMER_NV_TRANSLINK_PORT          CUSTOMER_NV_FLAG"TransLinkPort"
#define CUSTOMER_NV_TRANSLINK_TYPE          CUSTOMER_NV_FLAG"TransLinkType"
#define CUSTOMER_NV_TRANSLINK_UDPPORT       CUSTOMER_NV_FLAG"TranslinkUDPPort"
#define CUSTOMER_NV_TCPIP_DNS_SERVER0       CUSTOMER_NV_FLAG"DNSServer0"
#define CUSTOMER_NV_TCPIP_DNS_SERVER1       CUSTOMER_NV_FLAG"DNSServer1"
#define CUSTOMER_NV_DNS_AUTO_CHANGE         CUSTOMER_NV_FLAG"DNSAutoChange"
#define CUSTOMER_NV_WIFI_IP_MODE            CUSTOMER_NV_FLAG"IPMode"
	 // UART
#define CUSTOMER_NV_UART_CONFIG            CUSTOMER_NV_FLAG"UartConfig"

//develop
#define DEVELOP_NV_FLAG	                	"dev"

//otp
#define OTP_FLAG	                    	"otp"

//efuse
#define EFUSE_FLAG	                    	"efu"

/*--------------------------------------------------------------------------
* 	                                        	Types
--------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */

/*--------------------------------------------------------------------------
* 	                                        	Constants
--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------
* 	                                        	Global  Variables
--------------------------------------------------------------------------*/
/**  Description of global variable  */

/*--------------------------------------------------------------------------
* 	                                        	Function Prototypes
--------------------------------------------------------------------------*/
int hal_system_set_mac(MAC_SAVE_LACATION type, char *mac);
int hal_system_get_sta_mac(unsigned char *mac);
int hal_system_get_ap_mac(unsigned char *mac);
int hal_system_get_ble_mac(unsigned char *mac);



int hal_system_get_config(const char *key, void *buff, unsigned int buff_len);
int hal_system_set_config(const char *key, void *buff, unsigned int buff_len);
int hal_system_del_config(const char *key);




#endif/*_SYSTEM_CONFIG_H*/

