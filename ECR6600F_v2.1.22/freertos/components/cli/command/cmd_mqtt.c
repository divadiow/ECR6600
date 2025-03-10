#include "system_wifi.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "system_queue.h"
#include "cli.h"

/****************************************************************************
* 	                                           Local Macros
****************************************************************************/
#define IS_ASCII(c) (c > 0x1F && c < 0x7F)

//#ifndef MIN
//#define MIN(x,y) (x < y ? x : y)
//#endif

#define CLIENT_ID_STRING_MAX_LEN 256//defined in Section 1.5.3
#define PRINT_STRING_MAX_LEN (CLI_PRINT_BUFFER_SIZE - 32) 


#define QOS_PROMPT "qos error! please use qos xx(0 or 1 or 2)\r\n"
#define CLEAN_SESSION_PROMPT "clean_session err! please use clean_session xx(0 or 1)\r\n"
#define URL_PROMPT "url err! please use mqtt_start url xx\r\n"
#define RETAIN_PROMPT "retain error! please use retain xx(0 or 1)\r\n"
#define CLIENT_ID_TOO_LONG_PROMPT "The length of ClientID must be between 1 and 23 bytes!\r\n"

/****************************************************************************
* 	                                           Local Types
****************************************************************************/

/****************************************************************************
* 	                                           Local Constants
****************************************************************************/

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

/****************************************************************************
* 	                                          Function Definitions
****************************************************************************/

#include "mqtt_client.h"
#include "platform_tr6600.h"
#include "os.h"

trs_mqtt_client_handle_t mqtt_client = NULL;
trs_mqtt_client_config_t mqtt_cfg = {0};
uint16_t g_mqtt_cli_port = 0;

int g_MQTT_EVENT_CONNECTED = 0;
int g_MQTT_EVENT_DISCONNECTED = 0;
int g_MQTT_EVENT_SUBSCRIBED = 0;
int g_MQTT_EVENT_UNSUBSCRIBED = 0;
int g_MQTT_EVENT_PUBLISHED = 0;
int g_MQTT_EVENT_DATA = 0;
int g_MQTT_EVENT_ERROR = 0;

int g_MQTT_PING_SEND_OK = 0;
int g_MQTT_PING_SEND_NO = 0;
int g_MQTT_PING_OK = 0;
int g_MQTT_PING_NO = 0;
int g_mqtt = 0;

struct os_time g_MQTT_disconnected_time = 
{
	.sec = 0,
	.usec = 0
};

void liuyongTime(void *env)
{
	os_printf(LM_APP, LL_INFO, "MQTT PING\r\n\tSEND_OK:%d\r\n\tSEND_NO:%d\r\n\tOK:%d\r\n\tNO:%d\r\n", g_MQTT_PING_SEND_OK, g_MQTT_PING_SEND_NO, g_MQTT_PING_OK, g_MQTT_PING_NO);
	os_printf(LM_APP, LL_INFO, "MQTT EVENT\r\n\tCON:%d\r\n\tDISCON:%d\r\n\tSUBSCRIBED:%d\r\n\tUNSUBSCRIBED:%d\r\n\tPUBLISHED:%d\r\n\tD:%d\r\n\tE:%d\r\n", 
		g_MQTT_EVENT_CONNECTED, g_MQTT_EVENT_DISCONNECTED, g_MQTT_EVENT_SUBSCRIBED, g_MQTT_EVENT_UNSUBSCRIBED, 
		g_MQTT_EVENT_PUBLISHED, g_MQTT_EVENT_DATA, g_MQTT_EVENT_ERROR);
}

os_timer_handle_t liuyong_timer_handle = NULL;

static int default_mqtt_handle(trs_mqtt_event_handle_t event)
{
    //trs_mqtt_client_handle_t client = event->client;
    //int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
			if(g_mqtt == 0)
			{
				g_MQTT_EVENT_CONNECTED += 1;
				g_mqtt = 1;
			}
            os_printf(LM_APP, LL_INFO, "MQTT_EVENT_CONNECTED:%d\n", g_MQTT_EVENT_CONNECTED);
			if(g_MQTT_disconnected_time.sec!=0 || g_MQTT_disconnected_time.usec!=0)
			{
				struct os_time now_time;
				os_get_time(&now_time);
				long s = now_time.sec - g_MQTT_disconnected_time.sec;
				long us = now_time.usec - g_MQTT_disconnected_time.usec;
				os_printf(LM_APP, LL_INFO, "* - * - * - * - * - mqtt test - * - * - * - * - * - *\n");
				os_printf(LM_APP, LL_INFO, "reconnect time= %d s %d ms %d us\n", s, (int)(us/1000), us%1000);
				g_MQTT_disconnected_time.sec=0;
				g_MQTT_disconnected_time.usec=0;
			}
            break;
			
        case MQTT_EVENT_DISCONNECTED:
			if(g_mqtt == 1)
			{
				g_MQTT_EVENT_DISCONNECTED += 1;
				g_mqtt = 0;
			}
            os_printf(LM_APP, LL_INFO, "MQTT_EVENT_DISCONNECTED:%d\n", g_MQTT_EVENT_DISCONNECTED);
			if(g_MQTT_disconnected_time.sec==0 && g_MQTT_disconnected_time.usec==0)
			{
				os_get_time(&g_MQTT_disconnected_time);
			}
            break;

        case MQTT_EVENT_SUBSCRIBED:
			g_MQTT_EVENT_SUBSCRIBED++;
            os_printf(LM_APP, LL_INFO, "MQTT_EVENT_SUBSCRIBED:%d, msg_id=%d\n", g_MQTT_EVENT_SUBSCRIBED, event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
			g_MQTT_EVENT_UNSUBSCRIBED++;
            os_printf(LM_APP, LL_INFO, "MQTT_EVENT_UNSUBSCRIBED:%d, msg_id=%d\n", g_MQTT_EVENT_UNSUBSCRIBED, event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
			g_MQTT_EVENT_PUBLISHED++;
            os_printf(LM_APP, LL_INFO, "MQTT_EVENT_PUBLISHED:%d, msg_id=%d\n", g_MQTT_EVENT_PUBLISHED, event->msg_id);
            break;
        case MQTT_EVENT_DATA:
			g_MQTT_EVENT_DATA++;
            os_printf(LM_APP, LL_INFO, "MQTT_EVENT_DATA:%d\n", g_MQTT_EVENT_DATA);
            os_printf(LM_APP, LL_INFO, "TOPIC_len=%d, DATA_len= %d\r\n", event->topic_len, event->data_len);
            if (event->topic_len < PRINT_STRING_MAX_LEN)
            {
                os_printf(LM_APP, LL_INFO, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
            }
            else
            {
                os_printf(LM_APP, LL_INFO, "The length of TOPIC exceeds the length of the print buffer, so do't printf!\r\n");
            }

            if (event->data_len < PRINT_STRING_MAX_LEN)
            {
                if (event->data_len == 0)
                {
                    os_printf(LM_APP, LL_INFO, "DATA is null!\r\n");
                }
                else
                {
                    os_printf(LM_APP, LL_INFO, "DATA=%.*s\r\n", event->data_len, event->data);
                }
            }
            else
            {
                os_printf(LM_APP, LL_INFO, "The length of DATA exceeds the length of the print buffer, so do't printf!\r\n");
            }
            
            break;
        case MQTT_EVENT_ERROR:
			g_MQTT_EVENT_ERROR++;
            os_printf(LM_APP, LL_INFO, "MQTT_EVENT_ERROR:%d\n", g_MQTT_EVENT_ERROR);
            break;
    }
    return 0;
}


extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

int mqtt_client_port(cmd_tbl_t *t, int argc, char *argv[])
{
    int i = 1;
    if ((argc == 3) && (strcmp(argv[i], "local_port") == 0))
    {
        if ((atoi(argv[i+1]) >= 49152) && (atoi(argv[i+1]) < 65535))
        {
            g_mqtt_cli_port = atoi(argv[i+1]);
            os_printf(LM_APP, LL_INFO, "mqtt client local port parameters g_mqtt_client_port is %d\r\n", g_mqtt_cli_port);
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, "mqtt client local port number is wrong! \r\n");
            return -1;
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "mqtt set client local port parameters is wrong! \r\n");
        return -1;
    }
    return 0;
}

CLI_CMD(mqtt_client_port, mqtt_client_port, "", "");

int mqtt_client_get_port(cmd_tbl_t *t, int argc, char *argv[])
{
    if(argc != 1)
	{
		os_printf(LM_CMD, LL_ERR, "mqtt get client local port parameters is wrong! \r\n");
		return -1;
	}
	else
	{
		os_printf(LM_APP, LL_INFO, "cur_mqtt_client_port: %d\r\n", g_mqtt_cli_port);
		return 0;
	}

}

CLI_CMD(mqtt_client_get_port, mqtt_client_get_port, "", "");


int mqtt_start(cmd_tbl_t *t, int argc, char *argv[])
{
    uint8_t i;
    memset(&mqtt_cfg,0,sizeof(trs_mqtt_client_config_t));
    mqtt_cfg.event_handle = default_mqtt_handle;

#if 0
    mqtt_cfg.cert_pem = (const char *)server_root_cert_pem_start;
#endif
    if ((7 <= argc ) && ( argc <= 15))
    {
        for(i=1; i<argc; i+=2)
        {
            if(strcmp(argv[i], "uri")==0)
            {
                mqtt_cfg.uri = argv[i+1];
                if(strncasecmp("mqtts://", argv[i+1], 8) == 0)
                {
                    mqtt_cfg.transport = MQTT_TRANSPORT_OVER_SSL;
                }
                else if(strncasecmp("mqtt://", argv[i+1], 7) == 0)
                {
                    mqtt_cfg.transport = MQTT_TRANSPORT_OVER_TCP;
                }
                else
                {
                    mqtt_cfg.transport = MQTT_TRANSPORT_UNKNOWN;
                    os_printf(LM_APP, LL_ERR, "ERROR MQTT_TRANSPORT_UNKNOWN\r\n");
                    return CMD_RET_FAILURE;
                }
            }
            else if(strcmp(argv[i], "port")==0)
            {
                if((atoi(argv[i+1]) >= 0) && (atoi(argv[i+1]) < 65535))
                {
                    mqtt_cfg.port = atoi(argv[i+1]);
                }
                else
                {
                    os_printf(LM_APP, LL_ERR,"mqtt_start parameters port is wrong! \r\n");
                    return CMD_RET_FAILURE;
                }

            }
            else if(strcmp(argv[i], "client_id")==0)
            {
                mqtt_cfg.client_id = (const char*)argv[i+1];
            }
            else if(strcmp(argv[i], "username")==0)
            {
                mqtt_cfg.username = (const char*)argv[i+1];
            }
            else if(strcmp(argv[i], "password")==0)
            {
                mqtt_cfg.password = (const char*)argv[i+1];
            }
            else if(strcmp(argv[i], "clean_session")==0)
            {
                mqtt_cfg.clean_session = atoi(argv[i+1]);
                if ((mqtt_cfg.clean_session < 0) || (mqtt_cfg.clean_session > 1))
                {
                    os_printf(LM_CMD, LL_INFO, CLEAN_SESSION_PROMPT);
                    return CMD_RET_FAILURE;
                }
            }
            else if(strcmp(argv[i], "keepalive")==0)
            {
                if((atoi(argv[i+1]) >= 0) && (atoi(argv[i+1]) <= 7200))
                {
                    mqtt_cfg.keepalive = atoi(argv[i+1]);
                }
                else
                {
                    os_printf(LM_APP, LL_ERR,"mqtt_start parameters keepalive is wrong! \r\n");
                    return CMD_RET_FAILURE;
                }
            }
            else
            {
                os_printf(LM_CMD, LL_INFO, "mqtt_start parameters is wrong! \r\n");
                return CMD_RET_FAILURE;
            }
        }
    }
    else
    {
        os_printf(LM_CMD, LL_INFO, "mqtt_start parameters number is wrong! \r\n");
        return CMD_RET_FAILURE;
    }

    if (mqtt_client != NULL)
    {
        os_printf(LM_CMD, LL_INFO, "mqtt has started!\r\n");
        return CMD_RET_SUCCESS;
    }

    if (mqtt_cfg.uri == NULL)  // must set url
    {
        os_printf(LM_CMD, LL_INFO, URL_PROMPT);
        return CMD_RET_FAILURE;
    }

    int client_id_len = strlen(mqtt_cfg.client_id);
    if (client_id_len > CLIENT_ID_STRING_MAX_LEN)
    {
        os_printf(LM_CMD, LL_INFO, CLIENT_ID_TOO_LONG_PROMPT);
        return CMD_RET_FAILURE;
    }

    os_printf(LM_APP, LL_INFO, "In mqtt_start(),mqtt_cfg.url = %s\r\n",mqtt_cfg.uri);
    mqtt_client = trs_mqtt_client_init(&mqtt_cfg);
    if(mqtt_client == NULL){
        os_printf(LM_APP, LL_INFO, "mqtt init failed\n");
        return CMD_RET_FAILURE;
    }
    if (0 != trs_mqtt_client_start(mqtt_client)){
        os_printf(LM_APP, LL_INFO, "mqtt start failed\r\n");
        trs_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return CMD_RET_FAILURE;
    }
	os_printf(LM_APP, LL_INFO, "mqtt start done\n");

	if(liuyong_timer_handle == NULL)
	{
		os_printf(LM_CMD, LL_INFO, "* os_timer_create mqtt timer *\n");
		extern void liuyongTime(void *env);
		liuyong_timer_handle = os_timer_create("liuyongTime", 60000, 1, liuyongTime, NULL);
		os_timer_start(liuyong_timer_handle);
	}
    return CMD_RET_SUCCESS;
}

//CLI_SUBCMD(test,mqtt_start,mqtt_start, "mqtt_test", "mqtt_test");
CLI_CMD(mqtt_start,mqtt_start, "mqtt_test", "mqtt_test");

int mqtt_stop(cmd_tbl_t *t, int argc, char *argv[])
{
    if (mqtt_client == NULL)
    {
        os_printf(LM_APP, LL_INFO, "not start mqtt\n");
        return CMD_RET_FAILURE;
    }
    MQTT_CFG_MSG_ST msg_send = {0};
    msg_send.cfg_type = CFG_MSG_TYPE_STOP;
    os_queue_send(mqtt_client->cfg_queue_handle, (char *)&msg_send, sizeof(msg_send), 0);
    mqtt_client = NULL;

    if(liuyong_timer_handle != NULL)
    {
        int ret = os_timer_delete(liuyong_timer_handle);
        if (ret == 0)
        {
            os_printf(LM_CMD, LL_INFO, "* delete mqtt timer success *\n");
        }
        else
        {
            os_printf(LM_CMD, LL_INFO, "* delete mqtt timer failed *\n");
        }
        liuyong_timer_handle = NULL;
    }

    return CMD_RET_SUCCESS;
}

CLI_CMD(mqtt_stop, mqtt_stop, "", "");

int mqtt_sub(cmd_tbl_t *t, int argc, char *argv[])
{
    int i = 0;
    char * topic = NULL;
    int qos = 0;
    if (mqtt_client == NULL)
    {
        os_printf(LM_APP, LL_INFO, "not start mqtt\n");
        return CMD_RET_FAILURE;
    }

    if(argc == 5)
    {
        for(i=1; i<argc; i+=2)
        {
            if(strcmp(argv[i], "topic")==0)
            {
                topic = argv[i+1];
            }
            else if(strcmp(argv[i], "qos")==0)
            {
                qos = atoi(argv[i+1]);
            }
            else
            {
                os_printf(LM_CMD, LL_INFO, "mqtt_sub  parameters is wrong! \r\n");
                return CMD_RET_FAILURE;
            }
        }
    }
    else
    {
        os_printf(LM_CMD, LL_INFO, "mqtt_sub parameters number is wrong! \r\n");
        return CMD_RET_FAILURE;
    }
    int topic_len = strlen(topic);
    if ((topic_len <= 0) || (topic_len >= PRINT_STRING_MAX_LEN))
    {
        os_printf(LM_APP, LL_INFO, "topic error\n");
        return CMD_RET_FAILURE;
    }

    if ((qos < MQTT_QOS0) || (qos > MQTT_QOS2))
    {
        os_printf(LM_CMD, LL_INFO, QOS_PROMPT);
        return CMD_RET_FAILURE;
    }

    MQTT_CFG_MSG_ST msg_send = {0};
    set_mqtt_msg(SET_TOPIC, &msg_send, topic, topic_len);
    msg_send.cfg_type = CFG_MSG_TYPE_SUB;
    msg_send.qos = qos;
    if (mqtt_client->cfg_queue_handle != NULL)
    {
        os_queue_send(mqtt_client->cfg_queue_handle, (char *)&msg_send, sizeof(msg_send), 0);
    }
    else
    {
        os_printf(LM_APP, LL_INFO, "mqtt_client->cfg_queue_handle is null！ \r\n");
        return CMD_RET_FAILURE;
    }

    return CMD_RET_SUCCESS;
}

CLI_CMD(mqtt_sub, mqtt_sub, "", "");

int mqtt_unsub(cmd_tbl_t *t, int argc, char *argv[])
{
    int i = 0;
    char * topic = NULL;

    if (mqtt_client == NULL)
    {
        os_printf(LM_APP, LL_INFO, "not start mqtt\n");
        return CMD_RET_FAILURE;
    }
    if(argc == 3)
    {
        for(i=1; i<argc; i+=2)
        {
            if(strcmp(argv[i], "topic")==0)
            {
                topic = argv[i+1];
            }
            else
            {
                os_printf(LM_CMD, LL_INFO, "mqtt_unsub  parameters is wrong! \r\n");
                return CMD_RET_FAILURE;
            }
        }
    }
    else
    {
        os_printf(LM_CMD, LL_INFO, "mqtt_unsub parameters number is wrong! \r\n");
        return CMD_RET_FAILURE;
    }

    int topic_len = strlen(topic);
    if ((topic_len <= 0) || (topic_len >= PRINT_STRING_MAX_LEN))
    {
        os_printf(LM_APP, LL_INFO, "topic error\n");
        return CMD_RET_FAILURE;
    }

    MQTT_CFG_MSG_ST msg_send = {0};
    set_mqtt_msg(SET_TOPIC, &msg_send, topic, topic_len);
    msg_send.cfg_type = CFG_MSG_TYPE_UNSUB;
    if (mqtt_client->cfg_queue_handle != NULL)
    {
        os_queue_send(mqtt_client->cfg_queue_handle, (char *)&msg_send, sizeof(msg_send), 0);
    }
    else
    {
        os_printf(LM_APP, LL_INFO, "mqtt_client->cfg_queue_handle is null！ \r\n");
        return CMD_RET_FAILURE;
    }

    return CMD_RET_SUCCESS;
}

CLI_CMD(mqtt_unsub, mqtt_unsub, "", "");

int mqtt_pub(cmd_tbl_t *t, int argc, char *argv[])
{
    char *topic = NULL;
    char *data = NULL;
    int qos = 0;
    int retain = 0;
    int i =0;
    if (mqtt_client == NULL)
    {
        os_printf(LM_APP, LL_INFO, "not start mqtt\n");
        return CMD_RET_FAILURE;
    }
    if(argc == 9)
    {
        for(i=1; i<argc; i+=2)
        {
            if(strcmp(argv[i], "topic")==0)
            {
                topic = argv[i+1];
            }
            else if(strcmp(argv[i], "data")==0)
            {
                data = argv[i+1];
            }
            else if(strcmp(argv[i], "qos")==0)
            {
                qos = atoi(argv[i+1]);
            }
            else if(strcmp(argv[i], "retain")==0)
            {
                retain = atoi(argv[i+1]);
            }
            else
            {
                os_printf(LM_CMD, LL_INFO, "mqtt_pub parameters is wrong! \r\n");
                return CMD_RET_FAILURE;
            }
        }
    }
    else if(argc == 8)
    {
        for(i=1; i<argc; i+=2)
        {
            if(strcmp(argv[i], "topic")==0)
            {
                topic = argv[i+1];
            }
            else if(strcmp(argv[i], "data")==0)
            {
                if(strcmp(argv[i+1], "qos")==0)
                {
                    data = "";
                    i -=1;
                }
                else
                {
                    data = argv[i+1];
                }
            }
            else if(strcmp(argv[i], "qos")==0)
            {
                qos = atoi(argv[i+1]);
            }
            else if(strcmp(argv[i], "retain")==0)
            {
                retain = atoi(argv[i+1]);
            }
            else
            {
                os_printf(LM_CMD, LL_INFO, "mqtt_pub parameters is wrong! \r\n");
                return CMD_RET_FAILURE;
            }
        }
    }

    else
    {
        os_printf(LM_CMD, LL_INFO, "mqtt_pub parameters number is wrong! \r\n");
        return CMD_RET_FAILURE;
    }

    int topic_len = strlen(topic);
    if ((topic_len <= 0) || (topic_len >= PRINT_STRING_MAX_LEN))
    {
        os_printf(LM_APP, LL_INFO, "topic long is error\r\n");
        return CMD_RET_FAILURE;
    }

    int data_len = strlen(data);
    if (data_len < 0 || data_len >= PRINT_STRING_MAX_LEN)
    {
        os_printf(LM_APP, LL_INFO, "data long is error\r\n");
        return CMD_RET_FAILURE;
    }

    if ((qos < MQTT_QOS0) || (qos > MQTT_QOS2))
    {
        os_printf(LM_CMD, LL_INFO, QOS_PROMPT);
        return CMD_RET_FAILURE;
    }

    if ((retain != 0) && (retain != 1))
    {
        os_printf(LM_CMD, LL_INFO, RETAIN_PROMPT);
        return CMD_RET_FAILURE;
    }

    MQTT_CFG_MSG_ST msg_send = {0};
    set_mqtt_msg(SET_TOPIC, &msg_send, topic, topic_len);

    if (data_len != 0)
    {
        set_mqtt_msg(SET_DATA, &msg_send, data, data_len);
    }

    msg_send.qos = qos;
    msg_send.retain = retain;
    msg_send.cfg_type = CFG_MSG_TYPE_PUB;
    if (mqtt_client->cfg_queue_handle != NULL)
    {
        os_queue_send(mqtt_client->cfg_queue_handle, (char *)&msg_send, sizeof(msg_send), 0);
    }
    else
    {
        os_printf(LM_CMD, LL_INFO, "mqtt_client->cfg_queue_handle is null！ \r\n");
        return CMD_RET_FAILURE;
    }

    return CMD_RET_SUCCESS;
}

CLI_CMD(mqtt_pub, mqtt_pub, "", "");

#if 0

typedef struct {
	unsigned int ipaddr;
	unsigned int mask;
	unsigned int gateway;
	unsigned int dns1;
	unsigned int dns2;
	unsigned char ssid[32];
	unsigned char psk[32];
	unsigned char ssid_len;
	unsigned char psk_len;
	unsigned char sec_type;
	unsigned char channel;
	unsigned char bssid[6];
	unsigned char reserved[2];
}wifi_network_t;


int wlan_sta_start(wifi_network_t *network)
{
    int ret;
    //ip_info_t ipInfoTmp;
    wifi_config_u config;

    memset(&config, 0, sizeof(config));
    memcpy(config.sta.ssid, network->ssid, MIN(network->ssid_len, sizeof(config.sta.ssid) - 1));
    
    if(network->psk_len > 0 && network->psk[0])
    {
        memcpy(config.sta.password, network->psk, MIN(sizeof(config.sta.password), network->psk_len));
    }
    
    ret = wifi_start_station(&config);
    if (SYS_ERR_WIFI_MODE == ret)
    {   
        return -1;
    }
    else if (SYS_ERR_WIFI_BUSY == ret)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}


static int test_mqtt_create_sta(cmd_tbl_t *t, int argc, char *argv[])
{
	wifi_network_t network;
	uint8_t i;

	if(argc < 3) return CMD_RET_FAILURE;

	//uint8_t channel = 0;
	memset((char*)&network, 0, sizeof(network));

	for(i=1; i<argc; i+=2)
	{
		if(strcmp(argv[i], "type")==0)
		{
			network.sec_type = atoi(argv[i+1]);
			if(network.sec_type>5)
				return CMD_RET_FAILURE;
		}
		else if(strcmp(argv[i], "ch")==0)
		{
			network.channel = atoi(argv[i+1]);
			if(network.channel < 1 || network.channel > 14)
				return CMD_RET_FAILURE;
			//channel = network.channel;
		}
		else if(strcmp(argv[i], "ssid")==0)
		{
			network.ssid_len = strlen(argv[i+1]);
			if(network.ssid_len < 33)
				memcpy(network.ssid, argv[i+1], network.ssid_len);
			else
				return CMD_RET_FAILURE;
		}
		else if(strcmp(argv[i], "psk")==0)
		{
			network.psk_len = strlen(argv[i+1]);
			if(network.psk_len < 33)
				memcpy(network.psk, argv[i+1], network.psk_len);
			else
				return CMD_RET_FAILURE;
		}
		else return CMD_RET_FAILURE;
	}

	if(network.ipaddr != 0)
	{
		if(network.gateway==0)
			network.gateway = (network.ipaddr&0x00FFFFFF)|0x01000000;
		if(network.mask==0)
			network.mask = 0x00FFFFFF; // 255.255.255.0
		if(network.dns1==0)
			network.dns1 = 0x72727272; // 114.114.114.114
		if(network.dns2==0)
			network.dns2 = 0x08080808; // 8.8.8.8
	}

	wlan_sta_start(&network);

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(test, create_sta, test_mqtt_create_sta, "", "");
#endif

