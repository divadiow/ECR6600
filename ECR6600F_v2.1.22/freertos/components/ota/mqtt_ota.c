/**
 * @file mqtt_ota.c
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


/*--------------------------------------------------------------------------
*												Include files
--------------------------------------------------------------------------*/
#include <stdio.h>

#include "mqtt_client.h"
#include "oshal.h"
#include "os.h"
#include "cli.h"
#include "system_wifi.h"
#include "platform_tr6600.h"
#include "easyflash.h"
#include "cJSON.h"
#include "hal_system.h"
#include "flash.h"
#include "sdk_version.h"
#include "hal_rtc.h"

#include "mqtt_ota.h"
#include "http_ota.h"
#include "hal_system.h"



/*--------------------------------------------------------------------------
* 	                                           	Local Macros
--------------------------------------------------------------------------*/
/** Description of the macro */
#define ESWIN_TOPIC_UPDT_PRE "ESWIN/upgrade"
#define ESWIN_TOPIC_TASK_PRE "ESWIN/event/task"

#define MQTT_SERVICE_URL_LEN        (64)
#define MQTT_SERVICE_USERNAME_LEN   (32)
#define MQTT_SERVICE_PASSWD_LEN     (32)
/*--------------------------------------------------------------------------
* 	                                           	Local Types
--------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */

/*--------------------------------------------------------------------------
* 	                                           	Local Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                           	Local Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Variables
--------------------------------------------------------------------------*/
/**  Description of global variable  */
char MQTT_SERVICE_URL[MQTT_SERVICE_URL_LEN] = {0};
char MQTT_SERVICE_USERNAME[MQTT_SERVICE_USERNAME_LEN] = {0};
char MQTT_SERVICE_PASSWD[MQTT_SERVICE_PASSWD_LEN] = {0};


os_timer_handle_t ota_timer_task_handle = NULL;
static char g_ota_task_info[8] = {0};
static char g_ota_task_time[24] = {0};

unsigned char g_ota_is_downloading = 0;
/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/
trs_mqtt_client_config_t g_mqtt_service_config;
trs_mqtt_client_handle_t mq_client;
int g_service_ota_task_handle = 0;

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/

static int mqtt_service_get_product_id(char *device_id, unsigned int len)
{
	if (amt_get_env_blob(OTA_PROJECT_ID, device_id, len, NULL) == 0) {
		os_printf(LM_APP, LL_ERR, "OTA:Can not find OTA_PROJECT_ID in dnv\n");
		return -1;
	}
	//os_printf(LM_APP, LL_INFO, "OTA:project_id %s\n", device_id);
	return 0;
}

static int mqtt_service_get_product_key(char *product_key, unsigned int len)
{
	if (amt_get_env_blob(OTA_SN, product_key, len, NULL) == 0) {
		os_printf(LM_APP, LL_ERR, "OTA:Can not find OTA_SN in dnv\n");
		return -1;
	}
	//os_printf(LM_APP, LL_INFO, "OTA:SN:%s\n", product_key);
	return 0;
}

int mqtt_service_pub_errinfo_with_code(trs_mqtt_client_handle_t mq_client, int code, const char *errstr)
{
    char project_id[OTA_PROJECT_ID_LEN] = {0};
    char sn_key[OTA_SN_LEN] = {0};
    char mac[6] = {0};
    char mac_string[32] = {0};

    if (mqtt_service_get_product_id(project_id, sizeof(project_id)) != 0) {
        return -1;
    }

    if (mqtt_service_get_product_key(sn_key, sizeof(sn_key)) != 0) {
        return -1;
    }

    if ((strlen((char *)project_id) == 0) || (strlen((char *)sn_key) == 0)) {
        os_printf(LM_APP, LL_ERR, "OTA:please set project_id and sn_key ..\n");
        return -1;
    }

    wifi_get_mac_addr(0, (unsigned char *)mac);
    snprintf(mac_string, sizeof(mac_string), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    char *device_topic = os_zalloc(DATA_MAX_LEN);
    snprintf(device_topic, DATA_MAX_LEN, "ESWIN/event/log/%s/%s/%s", MQTT_SERVICE_OEM, project_id, sn_key);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "protocol_version", MQTT_SERVICE_PROTOCOL);
	cJSON_AddStringToObject(root, "sn", sn_key);
	cJSON_AddStringToObject(root, "mac", mac_string);
	cJSON_AddStringToObject(root, "oem", MQTT_SERVICE_OEM);
	cJSON_AddStringToObject(root, "productid", project_id);
	cJSON_AddNumberToObject(root, "code", code);
	cJSON_AddStringToObject(root, "errsrting", errstr);
	char *data = cJSON_Print(root);

	os_printf(LM_APP, LL_DBG, "OTA:pub errinfo:%s\n", device_topic);
	int ret = trs_mqtt_client_publish(mq_client, device_topic, data, strlen(data), 1, 0);
	os_free(device_topic);
	os_free(data);
	cJSON_Delete(root);
    return ret;
}

int mqtt_service_pub_deviceinfo_with_code(trs_mqtt_client_handle_t mq_client)
{
    char project_id[OTA_PROJECT_ID_LEN] = {0};
    char sn_key[OTA_SN_LEN] = {0};
    char mac[6] = {0};
    char mac_string[32] = {0};

    if (mqtt_service_get_product_id(project_id, sizeof(project_id)) != 0) {
        return -1;
    }

    if (mqtt_service_get_product_key(sn_key, sizeof(sn_key)) != 0) {
        return -1;
    }

    if ((strlen((char *)project_id) == 0) || (strlen((char *)sn_key) == 0)) {
        os_printf(LM_APP, LL_ERR, "OTA:please set project_id and projectid ..\n");
        return -1;
    }

    wifi_get_mac_addr(0, (unsigned char *)mac);
    snprintf(mac_string, sizeof(mac_string), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	char *device_topic = os_zalloc(DATA_MAX_LEN);
    snprintf(device_topic, DATA_MAX_LEN, "ESWIN/event/info/%s/%s/%s", MQTT_SERVICE_OEM, project_id, sn_key);

	cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "protocol_version", MQTT_SERVICE_PROTOCOL);
    cJSON_AddStringToObject(root, "sn", sn_key);
    cJSON_AddStringToObject(root, "mac", mac_string);
    cJSON_AddStringToObject(root, "oem", MQTT_SERVICE_OEM);
    cJSON_AddStringToObject(root, "productid", project_id);
    cJSON_AddStringToObject(root, "softver", (char *)RELEASE_VERSION);
    cJSON_AddStringToObject(root, "hardver", MQTT_HARDWARE_VERSION);
    cJSON_AddStringToObject(root, "sdkver", (char *)sdk_version);
    cJSON_AddStringToObject(root, "cpu", MQTT_BOARD);
    char *data = cJSON_Print(root);

	os_printf(LM_APP, LL_DBG, "OTA:pub deviceinfo:%s\n", device_topic);
	int ret = trs_mqtt_client_publish(mq_client, device_topic, data, strlen(data), 1, 0);
	os_free(device_topic);
	os_free(data);
	cJSON_Delete(root);
    return ret;
}

int mqtt_service_sub_otaupdate(trs_mqtt_client_handle_t mq_client)
{
    char topic[256] = {0};
    char project_id[OTA_PROJECT_ID_LEN] = {0};
    char sn_key[OTA_SN_LEN] = {0};

    if (mqtt_service_get_product_id(project_id, sizeof(project_id)) != 0) {
        return -1;
    }

    if (mqtt_service_get_product_key(sn_key, sizeof(sn_key)) != 0) {
        return -1;
    }

    if ((strlen((char *)project_id) == 0) || (strlen((char *)sn_key) == 0)) {
        os_printf(LM_APP, LL_ERR, "OTA:please set project_id and sn_key ..\n");
        return -1;
    }
    snprintf(topic, sizeof(topic), "ESWIN/upgrade/%s/%s/%s", MQTT_SERVICE_OEM, project_id, sn_key);
    trs_mqtt_client_subscribe(mq_client, topic, 1);

    snprintf(topic, sizeof(topic), "ESWIN/event/task/%s/%s/%s", MQTT_SERVICE_OEM, project_id, sn_key);
    trs_mqtt_client_subscribe(mq_client, topic, 1);

	return 0;
}


int mqtt_service_update_handle(trs_mqtt_event_handle_t event)
{
	trs_mqtt_client_handle_t client = event->client;
	cJSON *root = cJSON_Parse(event->data);
    cJSON *item;
    unsigned int fm_len;
    unsigned int fm_crc;

    if (root == NULL) {
        os_printf(LM_APP, LL_ERR, "OTA:cJson data parse failed\n");
        return -1;
    }

    item = cJSON_GetObjectItem(root, "target_version");
    if (item == NULL) {
        os_printf(LM_APP, LL_ERR, "OTA:cJson get target_version failed\n");
        return -1;
    }
	if(0 == strcmp(RELEASE_VERSION, item->valuestring))
	{
		os_printf(LM_APP, LL_INFO, "OTA:Same release version: V%s\n", RELEASE_VERSION);
		return -1;
	}
	else
	{
		os_printf(LM_APP, LL_INFO, "OTA:Release version V%s to V%s\n", RELEASE_VERSION, item->valuestring);
	}


    item = cJSON_GetObjectItem(root, "filecrc");
    if (item == NULL) {
        os_printf(LM_APP, LL_ERR, "OTA:cJson get filecrc failed\n");
        return -1;
    }
    fm_crc = item->valuedouble;
    os_printf(LM_APP, LL_DBG, "OTA:fm_crc:0x%08x\n", fm_crc);

    item = cJSON_GetObjectItem(root, "filesize");
    if (item == NULL) {
        os_printf(LM_APP, LL_ERR, "OTA:cJson get filesize failed\n");
        return -1;
    }
    fm_len = item->valueint;
    os_printf(LM_APP, LL_DBG, "OTA:fm_len:0x%08x\n", fm_len);
	if (ota_firmware_check(fm_len, fm_crc) != 0)
	{
		mqtt_service_pub_errinfo_with_code(client, 2, "crc check failed");
		return -1;
	}

	item = cJSON_GetObjectItem(root, "fileurl");
	if (item == NULL)
	{
		os_printf(LM_APP, LL_ERR, "OTA:cJson get fileurl failed\n");
		return -1;
	}
	os_printf(LM_APP, LL_DBG, "fileurl:%s\n", item->valuestring);

	if(ota_timer_task_handle != NULL)
	{
		os_timer_stop(ota_timer_task_handle);
		ota_timer_task_handle = NULL;
	}

	if(0 == g_ota_is_downloading)
	{
		g_ota_is_downloading = 1;
		if (http_client_download_file(item->valuestring, strlen(item->valuestring)) != 0)
		{
			mqtt_service_pub_errinfo_with_code(client, 1, "download failed");
			os_printf(LM_APP, LL_ERR, "OTA:download failed!\n");
			ota_done(0);
			g_ota_is_downloading = 0;
			return -1;
		}
		if (ota_done(1) != 0)
		{
			g_ota_is_downloading = 0;
			os_printf(LM_APP, LL_ERR, "OTA:ota status error!\n");
		}
	}
	else
	{
		os_printf(LM_APP, LL_ERR, "OTA:ota is downloading!\n");
	}

    return 0;
}

void ota_timer_task_function(os_timer_handle_t timer)
{
    trs_mqtt_client_handle_t client = mq_client;
	static int ota_timer_timout = 0;

	char* set_data_str = (char*)g_ota_task_time;
	DATE_RTC data = {0};
	hal_rtc_utctime_to_date(hal_rtc_get_time(), &data); //sec --struct
//	os_printf(LM_APP, LL_ERR, "OTA:year %d\n", data.year);
//	os_printf(LM_APP, LL_ERR, "OTA:month %d\n", data.month);
//	os_printf(LM_APP, LL_ERR, "OTA:day %d\n", data.day);
//	os_printf(LM_APP, LL_ERR, "OTA:hour %d\n", data.hour);
//	os_printf(LM_APP, LL_ERR, "OTA:min %d\n", data.min);
//	os_printf(LM_APP, LL_ERR, "OTA:sec %d\n", data.sec);
//	os_printf(LM_APP, LL_ERR, "OTA:wday %d\n", data.wday);
//
//	os_printf(LM_APP, LL_ERR, "OTA:set_data_str %s\n", set_data_str);

	char *rtc_string = os_zalloc(strlen((char*)set_data_str)+20);
	snprintf(rtc_string, strlen((char*)set_data_str)+20, "%04d-%02d-%02d %02d:%02d:%02d",
		data.year, data.month, data.day, data.hour, data.min, data.sec);

	if(strcmp(set_data_str, rtc_string) < 0)
	{
		os_printf(LM_APP, LL_ERR, "OTA:Time is up\n");
		os_printf(LM_APP, LL_ERR, "OTA:g_ota_task_info:%s\n", g_ota_task_info);

		if(strcmp(g_ota_task_info, "info") == 0)
		{
			os_printf(LM_APP, LL_ERR, "OTA:get: info\n");
			mqtt_service_pub_deviceinfo_with_code(client);
			ota_timer_timout++;
			if(ota_timer_timout>5)
			{
				os_timer_stop(ota_timer_task_handle);
				ota_timer_task_handle = NULL;
			}
		}
		if(strcmp(g_ota_task_info, "reboot") == 0)
		{
			os_printf(LM_APP, LL_ERR, "OTA:get: reboot\n\n\n");
			hal_system_reset(RST_TYPE_OTA);
		}
	}
	else
	{
		os_printf(LM_APP, LL_ERR, "OTA:set %s\n", set_data_str);
		os_printf(LM_APP, LL_ERR, "OTA:now %s\n", rtc_string);
	}
	os_free(rtc_string);
}

int mqtt_service_task_handle(trs_mqtt_event_handle_t event)
{
    //trs_mqtt_client_handle_t client = event->client;
    cJSON *root = cJSON_Parse(event->data);
	cJSON *item;

    if (root == NULL) {
        os_printf(LM_APP, LL_ERR, "OTA:cJson data parse failed\n");
        return -1;
    }

    item = cJSON_GetObjectItem(root, "time");
    if (item == NULL) {
        os_printf(LM_APP, LL_ERR, "OTA:cJson get time failed\n");
        return -1;
    }
    os_printf(LM_APP, LL_INFO, "OTA:time:%s\n", item->valuestring);
	if(ota_timer_task_handle == NULL)
	{
		memcpy(g_ota_task_time, item->valuestring, strlen(item->valuestring));
	}

    item = cJSON_GetObjectItem(root, "task");
    if (item == NULL) {
        os_printf(LM_APP, LL_ERR, "OTA:cJson get time failed\n");
        return -1;
    }
    os_printf(LM_APP, LL_INFO, "OTA:task:%s\n", item->valuestring);

	if(ota_timer_task_handle == NULL)
	{
		os_printf(LM_APP, LL_INFO, "OTA:Create ota timer thread\n");
		memcpy(g_ota_task_info, item->valuestring, strlen(item->valuestring));
		ota_timer_task_handle = os_timer_create("ota_timer_task", 60000, 1, ota_timer_task_function, NULL);
		os_timer_start(ota_timer_task_handle);
	}
	else
	{
		os_printf(LM_APP, LL_INFO, "OTA:Have ota timer thread\n");
	}

    return 0;
}


int mqtt_service_event_handle(trs_mqtt_event_handle_t event)
{
    trs_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            os_printf(LM_APP, LL_INFO, "OTA:MQTT_EVENT_CONNECTED\n");
			ota_confirm_update();
            mqtt_service_sub_otaupdate(client);
            mqtt_service_pub_deviceinfo_with_code(client);
            break;

        case MQTT_EVENT_DISCONNECTED:
            os_printf(LM_APP, LL_INFO, "OTA:MQTT_EVENT_DISCONNECTED\n");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            os_printf(LM_APP, LL_INFO, "OTA:MQTT_EVENT_SUBSCRIBED, msg_id=%d\n", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            os_printf(LM_APP, LL_INFO, "OTA:MQTT_EVENT_UNSUBSCRIBED, msg_id=%d\n", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            os_printf(LM_APP, LL_INFO, "OTA:MQTT_EVENT_PUBLISHED, msg_id=%d\n", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            os_printf(LM_APP, LL_INFO, "OTA:MQTT_EVENT_DATA\n");
			os_printf(LM_APP, LL_INFO, "OTA:TOPIC=%.*s\n", event->topic_len, event->topic);
			if (strstr(event->topic, ESWIN_TOPIC_UPDT_PRE) != NULL)
			{
            	os_printf(LM_APP, LL_INFO, "OTA:upgrade now\n");
                mqtt_service_update_handle(event);
            }

            if (strstr(event->topic, ESWIN_TOPIC_TASK_PRE) != NULL) {
            	os_printf(LM_APP, LL_INFO, "OTA:timer task\n");
                mqtt_service_task_handle(event);
            }
            break;
        case MQTT_EVENT_ERROR:
            os_printf(LM_APP, LL_ERR, "OTA:MQTT_EVENT_ERROR\n");
            break;
    }
    return 0;
}

int mqtt_service_config_init(void)
{
    g_mqtt_service_config.client_id = mqtt_malloc(OTA_SN_LEN);
	memset((void*)g_mqtt_service_config.client_id, 0, OTA_SN_LEN);

    if(0 != mqtt_service_get_product_key((char *)g_mqtt_service_config.client_id, OTA_SN_LEN))
    {
		return -1;
    }
	//os_printf(LM_APP, LL_ERR, "OTA:client_id:%s\n", g_mqtt_service_config.client_id);

    g_mqtt_service_config.uri = MQTT_SERVICE_URL;
    g_mqtt_service_config.port = atoi(MQTT_SERVICE_PORT);

    g_mqtt_service_config.username = MQTT_SERVICE_USERNAME;
    g_mqtt_service_config.password = MQTT_SERVICE_PASSWD;

    g_mqtt_service_config.event_handle = mqtt_service_event_handle;

    return 0;
}



static void service_ota_task(void *arg)
{
	extern bool is_wifi_conn;
    while (!is_wifi_conn)
    {
        os_msleep(1000);
    }
    // Waiting for wifi connection

	os_msleep(2000);

	if ((amt_get_env_blob("mqtt_url", MQTT_SERVICE_URL, MQTT_SERVICE_URL_LEN, NULL) == 0))
	{
		strcpy(MQTT_SERVICE_URL, MQTT_SERVICE_URL_DEFAULT);
		os_printf(LM_APP, LL_ERR, "OTA:No mqtt_url in anv! default %s !\n", MQTT_SERVICE_URL);
	}
	else
	{
		os_printf(LM_APP, LL_ERR, "OTA:mqtt_url %s\n", MQTT_SERVICE_URL);
	}
	if (amt_get_env_blob("mqtt_user", MQTT_SERVICE_USERNAME, MQTT_SERVICE_USERNAME_LEN, NULL) == 0)
	{
		strcpy(MQTT_SERVICE_USERNAME, MQTT_SERVICE_USERNAME_DEFAULT);
		os_printf(LM_APP, LL_ERR, "OTA:No matt_user in anv! default %s !\n", MQTT_SERVICE_USERNAME);
	}
	else
	{
		os_printf(LM_APP, LL_ERR, "OTA:mqtt_user %s\n", MQTT_SERVICE_USERNAME);
	}
	if (amt_get_env_blob("mqtt_password", MQTT_SERVICE_PASSWD, MQTT_SERVICE_PASSWD_LEN, NULL) == 0)
	{
		strcpy(MQTT_SERVICE_PASSWD, MQTT_SERVICE_PASSWD_DEFAULT);
		os_printf(LM_APP, LL_ERR, "OTA:No matt_password in anv! default %s !\n", MQTT_SERVICE_PASSWD);
	}
	else
	{
		os_printf(LM_APP, LL_ERR, "OTA:mqtt_password %s\n", MQTT_SERVICE_PASSWD);
	}

    char sn_key[OTA_SN_LEN] = {0};
    if (mqtt_service_get_product_key(sn_key, sizeof(sn_key)) == 0) {
		mqtt_service_config_init();
	    mq_client = trs_mqtt_client_init(&g_mqtt_service_config);
	    if (mq_client == NULL) {
			os_task_delete(g_service_ota_task_handle);
	    }

	    if (trs_mqtt_client_start(mq_client) != 0) {
	        trs_mqtt_client_stop(mq_client);
	        trs_mqtt_client_destroy(mq_client);
	        mqtt_free(mq_client);
			os_task_delete(g_service_ota_task_handle);
			os_printf(LM_APP, LL_INFO, "OTA:trs_mqtt_client_start error!\n");
	    }
    }
	else
	{
	    os_printf(LM_APP, LL_INFO, "OTA:start error! No SN!\n");
	}

	os_task_delete(g_service_ota_task_handle);
}


int service_ota_main(void)
{
	g_service_ota_task_handle = os_task_create("ota_s_task", 5, (4*1024), (task_entry_t)service_ota_task, NULL);
    return 0;
}




