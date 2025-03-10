/**
 * @file mqtt_ota.h
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-4-15
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


#ifndef _MQTT_OTA_H
#define _MQTT_OTA_H


/*--------------------------------------------------------------------------
* 	                                        	Include files
--------------------------------------------------------------------------*/
#include "ota.h"

/*--------------------------------------------------------------------------
* 	                                        	Macros
--------------------------------------------------------------------------*/
/** Description of the macro */
#define MQTT_SERVICE_URL_DEFAULT 		"mqtt://iot-ota.eswincomputing.com"
#define MQTT_SERVICE_PORT 				"1883"
#define MQTT_SERVICE_USERNAME_DEFAULT 	"scbu_ota"
#define MQTT_SERVICE_PASSWD_DEFAULT 	"123321"
#define MQTT_SERVICE_PROTOCOL 			"1.0"
#define MQTT_BOARD 						"ECR6600"
#define MQTT_SERVICE_OEM 				MQTT_SERVICE_USERNAME
#define MQTT_HARDWARE_VERSION 			"1.0.0"

#define OTA_SN 							"OTA_SN"
#define OTA_SN_LEN						(64)
#define OTA_PROJECT_ID					"OTA_PROJECT_ID"
#define OTA_PROJECT_ID_LEN				(64)

#define DATA_MAX_LEN 					512
#define TOPIC_MAX_LEN 					256

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
int service_ota_main(void);



#endif/*_MQTT_OTA_H*/

