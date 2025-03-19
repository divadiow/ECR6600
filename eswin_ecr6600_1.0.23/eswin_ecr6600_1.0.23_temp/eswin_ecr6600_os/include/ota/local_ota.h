#ifndef __LOCAL_OTA_H__
#define __LOCAL_OTA_H__



#include "ota.h"


//#define CONFIG_LOCAL_OTA_SSID            "local_ota_test"
//#define CONFIG_LOCAL_OTA_PASSWD          "12345678"
//#define CONFIG_LOCAL_OTA_CHANNEL         7
#define CONFIG_LOCAL_OTA_CONNET_TIMEOUT  20


#define LOCAL_OTA_RECV_PORT         5300
#define LOCAL_OTA_SEND_PORT         5301

#define LOCAL_OTA_DEFAULT_BUFF_LEN  	256
#define LOCAL_OTA_VERSION_NUM       	6
#define LOCAL_OTA_VERSION_GREATER   	1
#define LOCAL_OTA_VERSION_LESS      	2
#define LOCAL_OTA_REPORT_CNT        	5




void local_ota_main();



#endif
