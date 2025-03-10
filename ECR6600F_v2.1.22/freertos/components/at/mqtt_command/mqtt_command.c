
#include "system_queue.h"
#include "easyflash.h"
#include "hal_uart.h"
#include "at_common.h"
#include "dce_commands.h"
#include "at_def.h"
#include "mqtt_client.h"
#include "oshal.h"
#include "string.h"
#include "platform_tr6600.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include "psm_mode_ctrl.h"
#include "psm_user.h"


typedef enum
{
    AT_MQTT_RET_OK = 0,
    AT_MQTT_RET_ERR,
}AT_MQTT_RET_E;

typedef enum
{
    AT_MQTT_SCHEME_OVER_TCP = 1,
    AT_MQTT_SCHEME_OVER_TLS_NO_VERIFY_CERT,
    AT_MQTT_SCHEME_OVER_TLS_VERIFY_SERVER_CERT,
    AT_MQTT_SCHEME_OVER_TLS_PROVIDE_CLIENT_CERT,
    AT_MQTT_SCHEME_OVER_TLS_PROVIDE_CLIENT_CERT_AND_VERIFY_SERVER_CERT,
    AT_MQTT_SCHEME_OVER_WEB,
    AT_MQTT_SCHEME_OVER_WEB_TLS_NO_VERIFY_CERT,
    AT_MQTT_SCHEME_OVER_WEB_TLS_VERIFY_SERVER_CERT,
    AT_MQTT_SCHEME_OVER_WEB_TLS_PROVIDE_CLIENT_CERT,
    AT_MQTT_SCHEME_OVER_WEB_TLS_PROVIDE_CLIENT_CERT_AND_VERIFY_SERVER_CERT
}AT_MQTT_SCHEME_E;

typedef enum
{
    AT_MQTT_SET_HOST = 1,
    AT_MQTT_SET_URI,
    AT_MQTT_SET_CLIENT_ID,
    AT_MQTT_SET_USER_NAME,
    AT_MQTT_SET_PASSWORD,
    AT_MQTT_SET_LWT_TOPIC,
    AT_MQTT_SET_LWT_MSG,
}AT_MQTT_SET_TYPE_E;


typedef enum
{
    AT_MQTT_USERCFG_LINK_ID = 0,
    AT_MQTT_USERCFG_SCHEME,
    AT_MQTT_USERCFG_CLIENT_ID,
    AT_MQTT_USERCFG_USER_NAME,
    AT_MQTT_USERCFG_PASSWORD,
    AT_MQTT_USERCFG_CERT_ID,
    AT_MQTT_USERCFG_CA_ID,
    AT_MQTT_USERCFG_PATH,
    AT_MQTT_USERCFG_PARA_MAX_NUM
} AT_MQTT_USERCFG_PARA_NUM_E;

typedef enum
{
    AT_MQTT_CONNCFG_LINK_ID = 0,
    AT_MQTT_CONNCFG_KEEPALIVE,
    AT_MQTT_CONNCFG_DISABLE_CLEAN_SESSION,
    AT_MQTT_CONNCFG_LWT_TOPIC,
    AT_MQTT_CONNCFG_LWT_MSG,
    AT_MQTT_CONNCFG_LWT_QOS,
    AT_MQTT_CONNCFG_LWT_RETAIN,
    AT_MQTT_CONNCFG_PARA_MAX_NUM
} AT_MQTT_CONNCFG_PARA_NUM_E;

typedef enum
{
    AT_MQTT_CONN_LINK_ID = 0,
    AT_MQTT_CONN_HOST,
    AT_MQTT_CONN_PORT,
    AT_MQTT_CONN_PATH,
    AT_MQTT_CONN_RECONNECT,
    AT_MQTT_CONN_PARA_MAX_NUM
} AT_MQTT_CONN_PARA_NUM_E;

typedef enum
{
    AT_MQTT_CONN_READ_LINK_ID = 0,
    AT_MQTT_CONN_READ_STATE,
    AT_MQTT_CONN_READ_SCHEME,
    AT_MQTT_CONN_READ_HOST,
    AT_MQTT_CONN_READ_PORT,
    AT_MQTT_CONN_READ_PATH,
    AT_MQTT_CONN_READ_RECONNECT,
    AT_MQTT_CONN_READ_PARA_MAX_NUM
} AT_MQTT_CONN_READ_PARA_NUM_E;

typedef enum
{
    AT_MQTT_ENABLE_CLEAN_SESSION = 0,
    AT_MQTT_DISABLE_CLEAN_SESSION
} AT_MQTT_CONNCFG_CLEAN_SESSION_E;

typedef enum
{
    AT_MQTT_STATE_NOT_INIT = 0,
    AT_MQTT_STATE_USERCFG_SET,
    AT_MQTT_STATE_CONNCFG_SET,
    AT_MQTT_STATE_DISCONN,
    AT_MQTT_STATE_CONNECTED,
    AT_MQTT_STATE_CONN_NOT_SUB_TOPIC,
    AT_MQTT_STATE_CONN_SUB_TOPIC,
} AT_MQTT_STATE_E;

typedef enum
{
    AT_MQTT_PUB_LINK_ID = 0,
    AT_MQTT_PUB_TOPIC,
    AT_MQTT_PUB_DATA,
    AT_MQTT_PUB_QOS,
    AT_MQTT_PUB_RETAIN,
    AT_MQTT_PUB_PARA_MAX,
} AT_MQTT_PUB_PARA_NUM_E;

typedef enum
{
    AT_MQTT_PUB_RAW_LINK_ID = 0,
    AT_MQTT_PUB_RAW_TOPIC,
    AT_MQTT_PUB_RAW_LENGTH,
    AT_MQTT_PUB_RAW_QOS,
    AT_MQTT_PUB_RAW_RETAIN,
    AT_MQTT_PUB_RAW_PARA_MAX,
} AT_MQTT_PUB_RAW_PARA_NUM_E;

typedef enum
{
    AT_MQTT_SUB_LINK_ID = 0,
    AT_MQTT_SUB_TOPIC,
    AT_MQTT_SUB_QOS,
    AT_MQTT_SUB_PARA_MAX,
} AT_MQTT_SUB_PARA_NUM_E;

typedef enum
{
    AT_MQTT_SUB_READ_LINK_ID = 0,
    AT_MQTT_SUB_READ_STATE,
    AT_MQTT_SUB_READ_TOPIC,
    AT_MQTT_SUB_READ_QOS,
    AT_MQTT_SUB_READ_PARA_MAX,
} AT_MQTT_SUB_READ_PARA_NUM_E;

typedef enum
{
    AT_MQTT_SUB_RECV_LINK_ID = 0,
    AT_MQTT_SUB_RECV_TOPIC,
    AT_MQTT_SUB_RECV_DATA_LEN,
    AT_MQTT_SUB_RECV_DATA,
    AT_MQTT_SUB_RECV_PARA_MAX,
} AT_MQTT_SUB_RECV_PARA_NUM_E;


typedef enum
{
    AT_MQTT_UNSUB_LINK_ID = 0,
    AT_MQTT_UNSUB_TOPIC,
    AT_MQTT_UNSUB_PARA_MAX,
} AT_MQTT_UNSUB_PARA_NUM_E;



#define AT_MQTT_CLIENT_ID_MAX_LENGTH 256
#define AT_MQTT_USER_NAME_MAX_LENGTH 64
#define AT_MQTT_PASSWORD_MAX_LENGTH 64
#define AT_MQTT_URI_MAX_LENGTH 32
#define AT_MQTT_DOMAIN_NAME_MAX_LENGTH 128
#define AT_MQTT_PORT_MAX 65535
#define AT_MQTT_TOPIC_MAX_LENGTH 128
#define AT_MQTT_LWT_MSG_MAX_LENGTH 64
#define AT_MQTT_KEEPALIVE_MAX 7200
#define AT_MQTT_PUB_RAW_DATA_MAX_LEN 2048
#define AT_MQTT_PUB_RAW_BUFFER_SIZE (AT_MQTT_PUB_RAW_DATA_MAX_LEN + 1)
#define AT_MQTT_SUB_TOPIC_MAX_NUM (32)
#define AT_MQTT_BUFFER_SIZE (AT_MQTT_PUB_RAW_DATA_MAX_LEN + AT_MQTT_TOPIC_MAX_LENGTH + 10)

typedef struct {
    char buff[AT_MQTT_PUB_RAW_BUFFER_SIZE];
    unsigned short pos;
    unsigned short threshold;
}AT_MQTT_BUFF_T;

typedef struct {
    char topic[AT_MQTT_TOPIC_MAX_LENGTH + 1];
    int qos;
    int retain;
    AT_MQTT_BUFF_T uart_rev_buff;
    os_timer_handle_t uart_rev_timer;
}AT_MQTT_PUBRAW_T;



#define PARA_ERR_PRINT "---%s:%d---para is error!\r\n"
#define LINK_ID_ERR_PRINT "---%s:%d---link id(%d) is not 0!\r\n"

#define PUB_RAW_TIMER_NAME "AT_MQTT_REV_TIMER"
#define ALREADY_SUB "ALREADY SUBSCRIBE"
#define NO_UNSUB "NO UNSUBSCRIBE"

#define PUB_RAW_SUCCESS "+MQTTPUB:OK\r\n"
#define PUB_RAW_FAIL "+MQTTPUB:FAIL\r\n"

#define PUB_RAW_TIMER_PERIOD 5000 //1s


static trs_mqtt_client_handle_t at_mqtt_client = NULL;
static trs_mqtt_client_config_t at_mqtt_cfg = {0};
static AT_MQTT_STATE_E at_mqtt_state = AT_MQTT_STATE_NOT_INIT;
static AT_MQTT_SCHEME_E at_mqtt_scheme = AT_MQTT_SCHEME_OVER_TCP;
//static AT_MQTT_BUFF_T at_mqtt_rev_buffer_from = {0};
static AT_MQTT_PUBRAW_T at_mqtt_pub_raw_data = {0};
static dce_t *sub_recv_dce_p = NULL;


typedef struct SUB_TOPIC_ITEM {
    char *topic;
    int qos;
    STAILQ_ENTRY(SUB_TOPIC_ITEM) next;
} SUB_TOPIC_ITEM_T;


STAILQ_HEAD(SUB_TOPIC_ITEM_LIST_T, SUB_TOPIC_ITEM);


typedef struct SUB_TOPIC_ITEM_LIST_T * sub_topic_list_handle_t;
typedef struct SUB_TOPIC_ITEM * sub_topic_item_handle_t;

sub_topic_list_handle_t at_mqtt_sub_list = NULL;


static AT_MQTT_RET_E at_mqtt_send_pub_msg(const char* topic, const char* data, int qos, int retain, MQTT_CFG_MSG_TYPE msg_type);
static AT_MQTT_RET_E at_mqtt_handle_link_id_valid(arg_t argv);


static sub_topic_list_handle_t at_mqtt_sub_list_init()
{
    sub_topic_list_handle_t list = mqtt_calloc(1, sizeof(struct SUB_TOPIC_ITEM_LIST_T));
    MEM_CHECK(list, return NULL);
    STAILQ_INIT(list);
    return list;
}

static sub_topic_item_handle_t  at_mqtt_sub_topic_enqueue(sub_topic_list_handle_t head, const char *topic, int qos)
{
    if ((head == NULL) || (topic == NULL))
    {
        return NULL;
    }

    int topic_len = strlen((char *)topic);
    os_printf(LM_APP, LL_INFO, "AT MQTT:ENQUEUEsizeof(SUB_TOPIC_ITEM_T)s=%d.\r\n", sizeof(SUB_TOPIC_ITEM_T));
    sub_topic_item_handle_t item = mqtt_malloc(sizeof(SUB_TOPIC_ITEM_T));
    os_printf(LM_APP, LL_INFO, "AT MQTT:ENQUEUEsizeof(SUB_TOPIC_ITEM_T)s=%d. item:%p\r\n", sizeof(SUB_TOPIC_ITEM_T), item);
    memset(item, 0, sizeof(SUB_TOPIC_ITEM_T));
    MEM_CHECK(item, return NULL);
    item->qos = qos;
    item->topic = mqtt_malloc(topic_len + 1);
    MEM_CHECK(item->topic, {
        mqtt_free(item);
        return NULL;
    });
    memset(item->topic, 0, topic_len + 1);
    memcpy(item->topic, topic, topic_len);
    STAILQ_INSERT_TAIL(head, item, next);
    os_printf(LM_APP, LL_INFO, "AT MQTT:ENQUEUE topic =%s, qos=%d.\r\n", item->topic, item->qos);
    return item;
}

static sub_topic_item_handle_t at_mqtt_sub_topic_dequeue(sub_topic_list_handle_t head)
{
    sub_topic_item_handle_t item = NULL;
    sub_topic_item_handle_t tmp = NULL;
    STAILQ_FOREACH_SAFE(item, head, next, tmp) {
        os_printf(LM_APP, LL_INFO, "at_mqtt_sub_topic_dequeue(), item =%p\r\n", item);
        if (!STAILQ_EMPTY(head))
        {
            return item;
        }
        else
        {
            break;
        }
    }
    
    return NULL;
}

#if 0
static int at_mqtt_get_sub_item_num(sub_topic_list_handle_t head)
{
    int num = 0;
    sub_topic_item_handle_t item = NULL;
    sub_topic_item_handle_t tmp = NULL;
    STAILQ_FOREACH_SAFE(item, head, next, tmp) 
    {
        os_printf(LM_APP, LL_INFO, "at_mqtt_get_sub_item_num(), item =%p\r\n", item);
        if (item != NULL)
        {
            num++;
            if (num > AT_MQTT_SUB_TOPIC_MAX_NUM)
            {
                os_printf(LM_APP, LL_ERR, "AT MQTT:the num of sub-topic too much!\r\n");
                break;
            }
        }
        else
        {
            break;
        }
    }
    
    return num;
}
#endif

static AT_MQTT_RET_E at_mqtt_sub_topic_query(sub_topic_list_handle_t head, const char * topic, int *qos)
{
    sub_topic_item_handle_t item = NULL;
    sub_topic_item_handle_t tmp = NULL;
    STAILQ_FOREACH_SAFE(item, head, next, tmp) {
        os_printf(LM_APP, LL_INFO, "at_mqtt_sub_topic_query() =%p\r\n", item);
        if (strcmp(item->topic, topic) == 0) {
            *qos = item->qos;
            os_printf(LM_APP, LL_INFO, "AT MQTT:query topic =%s. qos = %d\r\n", item->topic, item->qos);
            return AT_MQTT_RET_OK;
        }
    }
    
    return AT_MQTT_RET_ERR;
}

static AT_MQTT_RET_E at_mqtt_sub_topic_delete(sub_topic_list_handle_t head, const char * topic, int topic_len)
{
    sub_topic_item_handle_t item = NULL;
    sub_topic_item_handle_t tmp = NULL;
    if (topic_len == 0)
    {
        topic_len = strlen(topic);
    }
    
    STAILQ_FOREACH_SAFE(item, head, next, tmp) {
        os_printf(LM_APP, LL_INFO, "at_mqtt_sub_topic_delete() =%p\r\n", item);
        if (memcmp(item->topic, topic, topic_len) == 0) {
            STAILQ_REMOVE(head, item, SUB_TOPIC_ITEM, next);
            mqtt_free(item->topic);
            mqtt_free(item);
            os_printf(LM_APP, LL_INFO, "AT MQTT:DELETED topic =%s.\r\n", topic);
            return AT_MQTT_RET_OK;
        }
    }
    
    return AT_MQTT_RET_ERR;
}

static void at_mqtt_sub_list_destroy(sub_topic_list_handle_t head)
{
    sub_topic_item_handle_t item = NULL;
    do{
        item = at_mqtt_sub_topic_dequeue(head);
        if (item == NULL) {
            break;
        }
        STAILQ_REMOVE(head, item, SUB_TOPIC_ITEM, next);
        mqtt_free(item->topic);
        mqtt_free(item);
    }while(item != NULL); 
    // don't delete head
}

static void at_mqtt_uart_rx_reset(void)
{
    dce_register_data_input_cb(NULL);
    target_dec_switch_input_state(COMMAND_STATE);
    at_mqtt_pub_raw_data.qos = 0;
    at_mqtt_pub_raw_data.retain = 0;
    at_mqtt_pub_raw_data.uart_rev_buff.pos = 0;
    at_mqtt_pub_raw_data.uart_rev_buff.threshold = 0;
    memset(at_mqtt_pub_raw_data.topic, 0 ,sizeof(at_mqtt_pub_raw_data.topic));
    memset(at_mqtt_pub_raw_data.uart_rev_buff.buff, 0 ,sizeof(at_mqtt_pub_raw_data.uart_rev_buff.buff));
}


static void at_mqtt_uart_rx_timeout(os_timer_handle_t xTimer)
{
    os_printf(LM_APP, LL_ERR, "In at_mqtt_uart_rx_timeout(), at_mqtt_pub_raw_data.uart_rev_buff.threshold = %d\r\n", at_mqtt_pub_raw_data.uart_rev_buff.threshold);
    if (at_mqtt_pub_raw_data.uart_rev_buff.threshold == 0) 
    {
        return;
    }

    if (at_mqtt_pub_raw_data.uart_rev_buff.pos != 0) 
    {
        at_mqtt_send_pub_msg(at_mqtt_pub_raw_data.topic, at_mqtt_pub_raw_data.uart_rev_buff.buff, at_mqtt_pub_raw_data.qos, at_mqtt_pub_raw_data.retain, CFG_MSG_TYPE_PUB_RAW);
    }
    
    at_mqtt_uart_rx_reset();
}


static void at_mqtt_pubraw_init(void)
{
    at_mqtt_pub_raw_data.qos = 0;
    at_mqtt_pub_raw_data.retain = 0;
    memset(at_mqtt_pub_raw_data.topic, 0 ,sizeof(at_mqtt_pub_raw_data.topic));

    at_mqtt_pub_raw_data.uart_rev_buff.pos = 0;
    at_mqtt_pub_raw_data.uart_rev_buff.threshold = 0;
    memset(at_mqtt_pub_raw_data.uart_rev_buff.buff, 0 ,sizeof(at_mqtt_pub_raw_data.uart_rev_buff.buff));

    if (at_mqtt_pub_raw_data.uart_rev_timer == NULL)
    {
        at_mqtt_pub_raw_data.uart_rev_timer = os_timer_create(PUB_RAW_TIMER_NAME, PUB_RAW_TIMER_PERIOD, 0, at_mqtt_uart_rx_timeout, NULL);  
        if (at_mqtt_pub_raw_data.uart_rev_timer == NULL)
        {
            os_printf(LM_APP, LL_ERR, "create timer[%s] err!\r\n", PUB_RAW_TIMER_NAME);
        }
    }
}

#if 0
static AT_MQTT_RET_E at_set_mqtt_scheme(AT_MQTT_SCHEME_E scheme)
{
    if (scheme == AT_MQTT_SCHEME_OVER_TCP)
    {
        at_mqtt_cfg.transport = MQTT_TRANSPORT_OVER_TCP;
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "at_set_mqtt_scheme() para is err(%d)", scheme);
        return AT_MQTT_RET_ERR;
    }
    return AT_MQTT_RET_OK;
}
#endif

static AT_MQTT_RET_E at_mqtt_get_para_inf(AT_MQTT_SET_TYPE_E set_type, const char **addr_info, int * max_len)
{
    AT_MQTT_RET_E ret = AT_MQTT_RET_OK;
//    const char **addr_info = NULL;
 //   int max_len = 0;
    switch(set_type)
    {
        case AT_MQTT_SET_HOST:
        {
            *addr_info = at_mqtt_cfg.host;
            *max_len = AT_MQTT_DOMAIN_NAME_MAX_LENGTH;
            break;
        }
        
        case AT_MQTT_SET_URI:
        {
            *addr_info = at_mqtt_cfg.uri;
            *max_len = AT_MQTT_URI_MAX_LENGTH;
            break;
        }
        
        case AT_MQTT_SET_CLIENT_ID:
        {
            *addr_info = at_mqtt_cfg.client_id;
            *max_len = AT_MQTT_CLIENT_ID_MAX_LENGTH;
//            os_printf(LM_CMD, LL_ERR, "at_mqtt_cfg.client_id is(%p) (%s)\r\n", at_mqtt_cfg.client_id, at_mqtt_cfg.client_id);
            break;
        }
        
        case AT_MQTT_SET_USER_NAME:
        {
            *addr_info = at_mqtt_cfg.username;
            *max_len = AT_MQTT_USER_NAME_MAX_LENGTH;
            break;
        }
        
        case AT_MQTT_SET_PASSWORD:
        {
            *addr_info = at_mqtt_cfg.password;
            *max_len = AT_MQTT_PASSWORD_MAX_LENGTH;
            break;
        }
        
        case AT_MQTT_SET_LWT_TOPIC:
        {
            *addr_info = at_mqtt_cfg.lwt_topic;
            *max_len = AT_MQTT_TOPIC_MAX_LENGTH;
            break;
        }
        
        case AT_MQTT_SET_LWT_MSG:
        {
            *addr_info = at_mqtt_cfg.lwt_msg;
            *max_len = AT_MQTT_LWT_MSG_MAX_LENGTH;
            break;
        }
        
        default:
        {
            os_printf(LM_CMD, LL_ERR, "at_mqtt_get_para_inf() set_type is err(%d)", set_type);
            ret = AT_MQTT_RET_ERR;
            break;
        }
    }
  
    return ret;
}

static AT_MQTT_RET_E at_mqtt_set_para_inf(AT_MQTT_SET_TYPE_E set_type, const char *string)
{
    AT_MQTT_RET_E ret = AT_MQTT_RET_OK;
    switch(set_type)
    {
        case AT_MQTT_SET_HOST:
        {
            at_mqtt_cfg.host = string;
            break;
        }
        
        case AT_MQTT_SET_URI:
        {
            at_mqtt_cfg.uri = string;
            break;
        }
        
        case AT_MQTT_SET_CLIENT_ID:
        {
            at_mqtt_cfg.client_id = string;
            os_printf(LM_CMD, LL_ERR, "at_mqtt_cfg.client_id is(%p) (%s)\r\n", at_mqtt_cfg.client_id, at_mqtt_cfg.client_id);
            break;
        }
        
        case AT_MQTT_SET_USER_NAME:
        {
            at_mqtt_cfg.username = string;
            break;
        }
        
        case AT_MQTT_SET_PASSWORD:
        {
            at_mqtt_cfg.password = string;
            break;
        }
        
        case AT_MQTT_SET_LWT_TOPIC:
        {
            at_mqtt_cfg.lwt_topic = string;
            break;
        }
        
        case AT_MQTT_SET_LWT_MSG:
        {
            at_mqtt_cfg.lwt_msg = string;
            break;
        }
        
        default:
        {
            os_printf(LM_CMD, LL_ERR, "at_mqtt_set_para_inf() set_type is err(%d)", set_type);
            ret = AT_MQTT_RET_ERR;
            break;
        }
    }

    return ret;
}

static AT_MQTT_RET_E at_mqtt_set_string_para(AT_MQTT_SET_TYPE_E set_type, const char *str)
{
    os_printf(LM_CMD, LL_DBG, "In at_mqtt_set_string_para() set_type:%d.,str is %s\r\n", set_type, str);
    AT_MQTT_RET_E ret = AT_MQTT_RET_OK;
    if (str != NULL) 
    {
        int str_len = strlen(str);
        const char *str_tmp = NULL;
        int str_max_len = 0;
        if (at_mqtt_get_para_inf(set_type, &str_tmp, &str_max_len) == AT_MQTT_RET_OK)
        {
            os_printf(LM_CMD, LL_DBG, "str_tmp(%p) (%s),str_max_len is %d\r\n", str_tmp, str_tmp, str_max_len);
            if (str_len <= str_max_len)
            {
                if (str_tmp != NULL)
                {
                    os_printf(LM_CMD, LL_DBG, "str_tmp isn't null! (%p)!\r\n", str_tmp);
                    if (strcmp(str_tmp, str) == 0)
                    {
                        os_printf(LM_CMD, LL_INFO, "str_tmp is the same as last,return!\r\n");
                        return AT_MQTT_RET_OK;
                    }
                    os_printf(LM_CMD, LL_DBG, "free first(%p)!\r\n", str_tmp);
                    os_free((void *)str_tmp);
                }

                if (str_len != 0)
                {
                    str_tmp = os_strdup(str);
                    if (str_tmp == NULL)
                    {
                        os_printf(LM_CMD, LL_ERR, "os_strdup() return err!\r\n");
                        ret = AT_MQTT_RET_ERR;
                    }
                }
                else
                {
                    str_tmp = NULL;
                }

                at_mqtt_set_para_inf(set_type, str_tmp);
            }
            else
            {
                os_printf(LM_CMD, LL_ERR, "string(set_type: %d) is too long(%d).\r\n", set_type, str_len);
                ret = AT_MQTT_RET_ERR;
            }
        }
        else
        {
            ret = AT_MQTT_RET_ERR;
        }

    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "at_mqtt_set_string_para() para is NULL\r\n");
        ret = AT_MQTT_RET_ERR;
    }
    return ret;
}

static AT_MQTT_RET_E at_mqtt_set_scheme(int scheme)
{
    if ((scheme >= AT_MQTT_SCHEME_OVER_TCP) && (scheme <= AT_MQTT_SCHEME_OVER_WEB_TLS_PROVIDE_CLIENT_CERT_AND_VERIFY_SERVER_CERT))
    {
        at_mqtt_scheme = (AT_MQTT_SCHEME_E)scheme;
        if (scheme == AT_MQTT_SCHEME_OVER_TCP)
        {
            at_mqtt_cfg.transport = MQTT_TRANSPORT_OVER_TCP;
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, "at_mqtt_set_scheme() scheme(%d) is not supported!\r\n", scheme);
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "at_mqtt_set_scheme() para(%d) is err\r\n", scheme);
        return AT_MQTT_RET_ERR;
    }
    return AT_MQTT_RET_OK;
}

static void print_input_para(size_t argc, arg_t* argv)
{
    os_printf(LM_APP, LL_DBG, "********************start print arg info ********************! \r\n");
    for (int i = 0; i < argc; i++)
    {
        if (argv[i].type == ARG_TYPE_NUMBER)
        {
            os_printf(LM_APP, LL_DBG, "argv[%d] is number,data is %d \r\n", i, argv[i].value.number);
        }
        else if (argv[i].type == ARG_TYPE_STRING)
        {
            os_printf(LM_APP, LL_DBG, "argv[%d] is string,data is %s \r\n", i, argv[i].value.string);
        }
        else
        {
             os_printf(LM_APP, LL_ERR, "argv[%d] type is err! \r\n", i, argv[i].type);
        }
    }
    os_printf(LM_APP, LL_DBG, "$$$$$$$$$$$$$$$$$$$$$$$$$$$$ end $$$$$$$$$$$$$$$$$$$$$$$$$$$$! \r\n");
}

static void print_at_mqtt_cfg_para()
{
    os_printf(LM_APP, LL_ERR, "********************start print at_mqtt_cfg ********************! \r\n");
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.host is %s \r\n", at_mqtt_cfg.host);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.uri is %s \r\n", at_mqtt_cfg.uri);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.port is %d \r\n", at_mqtt_cfg.port);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.client_id is %s \r\n", at_mqtt_cfg.client_id);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.username is %s \r\n", at_mqtt_cfg.username);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.password is %s \r\n", at_mqtt_cfg.password);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.lwt_topic is %s \r\n", at_mqtt_cfg.lwt_topic);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.lwt_msg is %s \r\n", at_mqtt_cfg.lwt_msg);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.lwt_qos is %d \r\n", at_mqtt_cfg.lwt_qos);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.lwt_retain is %d \r\n", at_mqtt_cfg.lwt_retain);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.clean_session is %d \r\n", at_mqtt_cfg.clean_session);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.keepalive is %d \r\n",  at_mqtt_cfg.keepalive);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.disable_auto_reconnect is %d \r\n", at_mqtt_cfg.disable_auto_reconnect);
    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.transport is %d \r\n", at_mqtt_cfg.transport);
//    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.host is %s \r\n", at_mqtt_cfg.host);
//    os_printf(LM_APP, LL_DBG, " at_mqtt_cfg.host is %s \r\n", at_mqtt_cfg.host);
    os_printf(LM_APP, LL_DBG, "********************print at_mqtt_cfg end********************! \r\n");
}


static dce_result_t dce_handle_mqtt_set_usercfg(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    print_input_para(argc, argv);
    if ((argc <= AT_MQTT_USERCFG_SCHEME) || (argc > AT_MQTT_USERCFG_PARA_MAX_NUM))
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        os_printf(LM_CMD, LL_ERR, "dce_handle_mqtt_set_usercfg() para num is err!\r\n");
        return DCE_FAILED;
    }
    
    dce_result_t result = DCE_OK;
    if (kind == DCE_WRITE)
    {
        do
        {
            if (at_mqtt_handle_link_id_valid(argv[AT_MQTT_USERCFG_LINK_ID]) != AT_MQTT_RET_OK)
            {
                os_printf(LM_CMD, LL_ERR, LINK_ID_ERR_PRINT, __FILE__, __LINE__, argv[AT_MQTT_USERCFG_LINK_ID].value.number);
                result = DCE_INVALID_INPUT;
                break; 
            }

            if (at_mqtt_client != NULL)
            {
                os_printf(LM_CMD, LL_INFO, "mqtt has started!\r\n");
                result = DCE_FAILED;
                break;
            }
            
            if (at_mqtt_set_scheme(argv[AT_MQTT_USERCFG_SCHEME].value.number) != AT_MQTT_RET_OK)
            {
                result = DCE_FAILED;
                break;
            }
            
            if (argc > AT_MQTT_USERCFG_CLIENT_ID)
            {
                if (at_mqtt_set_string_para(AT_MQTT_SET_CLIENT_ID, argv[AT_MQTT_USERCFG_CLIENT_ID].value.string) != AT_MQTT_RET_OK)
                {
                    result = DCE_FAILED;
                    break;
                }
            }

            if (argc > AT_MQTT_USERCFG_USER_NAME)
            {
                if (at_mqtt_set_string_para(AT_MQTT_SET_USER_NAME, argv[AT_MQTT_USERCFG_USER_NAME].value.string) != AT_MQTT_RET_OK)
                {
                    result = DCE_FAILED;
                    break;
                }
            }

            if (argc > AT_MQTT_USERCFG_PASSWORD)
            {
                if (at_mqtt_set_string_para(AT_MQTT_SET_PASSWORD, argv[AT_MQTT_USERCFG_PASSWORD].value.string) != AT_MQTT_RET_OK)
                {
                    result = DCE_FAILED;
                    break;
                }
            }

            if (argc > AT_MQTT_USERCFG_PATH)
            {
                if (at_mqtt_set_string_para(AT_MQTT_SET_URI, argv[AT_MQTT_USERCFG_PATH].value.string) != AT_MQTT_RET_OK)
                {
                    result = DCE_FAILED;
                    break;
                }
            }
        }while(0);
    }
    else
    {
        result = DCE_FAILED;
    }

    print_at_mqtt_cfg_para();
    dce_result_code_t dce_code = DCE_RC_OK;
    if (result == DCE_OK)
    {
        at_mqtt_state = AT_MQTT_STATE_USERCFG_SET;
    }
    else
    {
        dce_code = DCE_RC_ERROR;
    }
    
    dce_emit_basic_result_code(dce, dce_code);
    return result;
}

static AT_MQTT_RET_E at_mqtt_set_single_cmd(dce_t* dce, AT_MQTT_SET_TYPE_E set_type, size_t argc, arg_t* argv)
{
    os_printf(LM_CMD, LL_DBG, "enter at_mqtt_set_single_cmd(), set_type:%d!\r\n", set_type);
    print_input_para( argc, argv);
    dce_result_t result = DCE_OK;
    if (at_mqtt_client == NULL)
    {
        if (argc == 2)
        {
            if (at_mqtt_handle_link_id_valid(argv[0]) == AT_MQTT_RET_OK)
            {
                if (at_mqtt_set_string_para(set_type, argv[1].value.string) != AT_MQTT_RET_OK)
                {
                    result = DCE_FAILED;
                }
            }
            else
            {
                os_printf(LM_CMD, LL_ERR, LINK_ID_ERR_PRINT, __FILE__, __LINE__, argv[0].value.number);
                result = DCE_FAILED;
            }
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            result = DCE_FAILED;
        }
    }
    else
    {
        os_printf(LM_CMD, LL_INFO, "mqtt has started!\r\n");
        result = DCE_FAILED;
    }

    dce_result_code_t dce_code = DCE_RC_OK;
    if (result != DCE_OK)
    {
        dce_code = DCE_RC_ERROR;
    }
    print_at_mqtt_cfg_para();
    dce_emit_basic_result_code(dce, dce_code);
    return result;
}

static dce_result_t dce_handle_mqtt_set_client_id(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    return at_mqtt_set_single_cmd(dce, AT_MQTT_SET_CLIENT_ID, argc, argv);
}


static dce_result_t dce_handle_mqtt_set_user_name(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{	
    return at_mqtt_set_single_cmd(dce, AT_MQTT_SET_USER_NAME, argc, argv);
}

static dce_result_t dce_handle_mqtt_set_password(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    return at_mqtt_set_single_cmd(dce, AT_MQTT_SET_PASSWORD, argc, argv);
}

static AT_MQTT_RET_E at_mqtt_set_keepalive(arg_t argv)
{
    AT_MQTT_RET_E ret = AT_MQTT_RET_OK;
    if (argv.type == ARG_TYPE_NUMBER)
    {
        int keepalive = argv.value.number;
        if ((keepalive >= 0) && (keepalive <= AT_MQTT_KEEPALIVE_MAX))
        {
            if (keepalive == 0)
            {
                at_mqtt_cfg.keepalive = 120;
            }
            else
            {
                at_mqtt_cfg.keepalive = keepalive;
            }
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            ret = AT_MQTT_RET_ERR;
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
        ret = AT_MQTT_RET_ERR;
    }
    
    return ret;

}

static AT_MQTT_RET_E at_mqtt_set_clean_session(arg_t argv)
{
    AT_MQTT_RET_E ret = AT_MQTT_RET_OK;
    if (argv.type == ARG_TYPE_NUMBER)
    {
        int disable_clean_session = argv.value.number;
        os_printf(LM_CMD, LL_DBG, "at_mqtt_set_clean_session(), para is %d\r\n", disable_clean_session);
        if ((disable_clean_session == AT_MQTT_ENABLE_CLEAN_SESSION) || (disable_clean_session == AT_MQTT_DISABLE_CLEAN_SESSION))
        {
            if (disable_clean_session == AT_MQTT_ENABLE_CLEAN_SESSION)
            {
                at_mqtt_cfg.clean_session = 1;
            }
            else
            {
                at_mqtt_cfg.clean_session = 0;
            }
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            ret = AT_MQTT_RET_ERR;
        }
    }
    else
    {
        ret = AT_MQTT_RET_ERR;
    }
    
    return ret;
}

static AT_MQTT_RET_E at_mqtt_set_lwt_topic(arg_t argv)
{
    AT_MQTT_RET_E ret = AT_MQTT_RET_OK;
    if (argv.type == ARG_TYPE_STRING)
    {
        const char * lwt_topic = argv.value.string;
        if (lwt_topic != NULL)
        {
            ret = at_mqtt_set_string_para(AT_MQTT_SET_LWT_TOPIC, lwt_topic);
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            ret = AT_MQTT_RET_ERR;
        }
    }
    else
    {
        ret = AT_MQTT_RET_ERR;
    }
    
    return ret;
}

static AT_MQTT_RET_E at_mqtt_set_lwt_msg(arg_t argv)
{
    AT_MQTT_RET_E ret = AT_MQTT_RET_OK;
    if (argv.type == ARG_TYPE_STRING)
    {
        const char * lwt_msg = argv.value.string;
        if (lwt_msg != NULL)
        {
            ret = at_mqtt_set_string_para(AT_MQTT_SET_LWT_MSG, lwt_msg);
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        ret = AT_MQTT_RET_ERR;
    }

    return ret;
}

static AT_MQTT_RET_E at_mqtt_set_lwt_qos(arg_t argv)
{
    AT_MQTT_RET_E ret = AT_MQTT_RET_OK;
    if (argv.type == ARG_TYPE_NUMBER)
    {
        int lwt_qos = argv.value.number;
        os_printf(LM_CMD, LL_DBG, "at_mqtt_set_lwt_qos(), para is %d\r\n", lwt_qos);
        if ((lwt_qos >= MQTT_QOS0) && (lwt_qos <= MQTT_QOS2))
        {
            at_mqtt_cfg.lwt_qos = lwt_qos;
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            ret = AT_MQTT_RET_ERR;
        }
    }
    else
    {
        ret = AT_MQTT_RET_ERR;
    }
    
    return ret;
}

static AT_MQTT_RET_E at_mqtt_set_lwt_retain(arg_t argv)
{
    AT_MQTT_RET_E ret = AT_MQTT_RET_OK;
    if (argv.type == ARG_TYPE_NUMBER)
    {
        int lwt_retain = argv.value.number;
        os_printf(LM_CMD, LL_DBG, "at_mqtt_set_lwt_retain(), para is %d\r\n", lwt_retain);
        if ((lwt_retain == 0) || (lwt_retain == 1))
        {
            at_mqtt_cfg.lwt_retain = lwt_retain;
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            ret = AT_MQTT_RET_ERR;
        }
    }
    else
    {
        ret = AT_MQTT_RET_ERR;
    }
    
    return ret;
}


static dce_result_t dce_handle_mqtt_set_conn_cfg(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    print_input_para(argc, argv);
    if ((argc <= AT_MQTT_CONNCFG_KEEPALIVE) || (kind != DCE_WRITE))
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__,__LINE__);
        return DCE_FAILED;
    }

    dce_result_t result = DCE_OK;
    do
    {
        if (at_mqtt_handle_link_id_valid(argv[AT_MQTT_CONNCFG_LINK_ID]) != AT_MQTT_RET_OK)
        {
            os_printf(LM_CMD, LL_ERR, LINK_ID_ERR_PRINT, __FILE__, __LINE__, argv[AT_MQTT_CONNCFG_LINK_ID].value.number);
            result = DCE_FAILED;
            break;
        }

        if (at_mqtt_client != NULL)
        {
            os_printf(LM_CMD, LL_INFO, "mqtt has started!\r\n");
            result = DCE_FAILED;
            break;
        }

        if (at_mqtt_set_keepalive(argv[AT_MQTT_CONNCFG_KEEPALIVE]) != AT_MQTT_RET_OK)
        {
            result = DCE_FAILED;
            break;
        }

        if (argc > AT_MQTT_CONNCFG_DISABLE_CLEAN_SESSION)
        {
            if (at_mqtt_set_clean_session(argv[AT_MQTT_CONNCFG_DISABLE_CLEAN_SESSION]) != AT_MQTT_RET_OK)
            {
                result = DCE_FAILED;
                break;
            }
        }

        if (argc > AT_MQTT_CONNCFG_LWT_TOPIC)
        {
            if (at_mqtt_set_lwt_topic(argv[AT_MQTT_CONNCFG_LWT_TOPIC]) != AT_MQTT_RET_OK)
            {
                result = DCE_FAILED;
                break;
            }
        }
        if (argc > AT_MQTT_CONNCFG_LWT_MSG)
        {
            if (at_mqtt_set_lwt_msg(argv[AT_MQTT_CONNCFG_LWT_MSG]) != AT_MQTT_RET_OK)
            {
                result = DCE_FAILED;
                break;
            }
        }

        if (argc > AT_MQTT_CONNCFG_LWT_QOS)
        {
            if (at_mqtt_set_lwt_qos(argv[AT_MQTT_CONNCFG_LWT_QOS]) != AT_MQTT_RET_OK)
            {
                result = DCE_FAILED;
                break;
            }
        }

        if (argc > AT_MQTT_CONNCFG_LWT_RETAIN)
        {
            if (at_mqtt_set_lwt_retain(argv[AT_MQTT_CONNCFG_LWT_RETAIN]) != AT_MQTT_RET_OK)
            {
                result = DCE_FAILED;
                break;
            }
        }
    }while(0);
    
    dce_result_code_t dce_code = DCE_RC_OK;
    if (result != DCE_OK)
    {
        dce_code = DCE_RC_ERROR;
    }
    
    print_at_mqtt_cfg_para();
    dce_emit_basic_result_code(dce, dce_code);
    return result;
}

static AT_MQTT_RET_E at_mqtt_set_host(arg_t arg)
{
    if (arg.type == ARG_TYPE_STRING)
    {
        return at_mqtt_set_string_para(AT_MQTT_SET_HOST, arg.value.string);
    }
    else
    {
        return AT_MQTT_RET_ERR;
    }
}

static AT_MQTT_RET_E at_mqtt_set_port(arg_t arg)
{
    if (arg.type == ARG_TYPE_NUMBER)
    {
        int port = arg.value.number;
        if ((port >= 0) && (port < AT_MQTT_PORT_MAX))
        {
            at_mqtt_cfg.port = port;
        }
        else
        {
            
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        return AT_MQTT_RET_ERR;
    }
    
    return AT_MQTT_RET_OK;
}

static AT_MQTT_RET_E at_mqtt_set_path(arg_t arg)
{
    if (arg.type == ARG_TYPE_STRING)
    {
        return at_mqtt_set_string_para(AT_MQTT_SET_URI, arg.value.string);
    }
    else
    {
        return AT_MQTT_RET_ERR;
    }
}

static AT_MQTT_RET_E at_mqtt_set_reconnect_flag(arg_t arg)
{
    if (arg.type == ARG_TYPE_NUMBER)
    {
        int reconnect_flag = arg.value.number;
        if ((reconnect_flag == 0) || (reconnect_flag == 1))
        {
            at_mqtt_cfg.disable_auto_reconnect = !reconnect_flag;
        }
        else
        {
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        return AT_MQTT_RET_ERR;
    }
    
    return AT_MQTT_RET_OK;
}



static dce_result_t dce_handle_mqtt_connect_set(dce_t* dce, void* group_ctx, size_t argc, arg_t* argv)
{
    if ((argc <= AT_MQTT_CONN_LINK_ID) || (argc > AT_MQTT_CONN_PARA_MAX_NUM))
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__,__LINE__);
        return DCE_FAILED;
    }
    
    if (at_mqtt_handle_link_id_valid(argv[AT_MQTT_CONN_LINK_ID]) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, LINK_ID_ERR_PRINT, __FILE__, __LINE__, argv[AT_MQTT_CONN_LINK_ID - 1].value.number);
        return DCE_FAILED;
    }

    if (at_mqtt_client != NULL)
    {
        os_printf(LM_CMD, LL_INFO, "mqtt has started!\r\n");
        return DCE_FAILED;
    }

    dce_result_t result = DCE_OK;
    do{
        if (argc > AT_MQTT_CONN_HOST)
        {
            if (at_mqtt_set_host(argv[AT_MQTT_CONN_HOST]) != AT_MQTT_RET_OK)
            {
                os_printf(LM_CMD, LL_ERR, "HOST set err!\r\n");
                result = DCE_FAILED;
                break;
            }
        }

        if (argc > AT_MQTT_CONN_PORT)
        {
            if (at_mqtt_set_port(argv[AT_MQTT_CONN_PORT]) != AT_MQTT_RET_OK)
            {
                os_printf(LM_CMD, LL_ERR, "PORT set err!\r\n");
                result = DCE_FAILED;
                break;
            }
        }

        if (argc > AT_MQTT_CONN_PATH)
        {
            if (at_mqtt_set_path(argv[AT_MQTT_CONN_PATH]) != AT_MQTT_RET_OK)
            {
                os_printf(LM_CMD, LL_ERR, "Path set err!\r\n");
                result = DCE_FAILED;
                break;
            }
        }

        if (argc > AT_MQTT_CONN_RECONNECT)
        {
            if (at_mqtt_set_reconnect_flag(argv[AT_MQTT_CONN_RECONNECT]) != AT_MQTT_RET_OK)
            {
                os_printf(LM_CMD, LL_ERR, "reconnect set err!\r\n");
                result = DCE_FAILED;
                break;
            }
        }
        
        os_printf(LM_CMD, LL_DBG, "In mqtt_start(),mqtt_cfg.url = %s\r\n",at_mqtt_cfg.uri);
        at_mqtt_client = trs_mqtt_client_init(&at_mqtt_cfg);
        if(at_mqtt_client == NULL){
            os_printf(LM_CMD, LL_DBG, "mqtt init failed\n");
            result = DCE_FAILED;
            break;
        }
        
        if (0 != trs_mqtt_client_start(at_mqtt_client)){
            os_printf(LM_CMD, LL_ERR, "mqtt start failed\r\n");
            trs_mqtt_client_destroy(at_mqtt_client);
            at_mqtt_client = NULL;
            result = DCE_FAILED;
            break;
        }
    }while(0);

    return result;
}

static dce_result_t dce_handle_mqtt_connect_get(dce_t* dce)
{
    const char *host = NULL;
    if (at_mqtt_cfg.host == NULL)
    {
        host = "";
    }
    else
    {
        host = at_mqtt_cfg.host;
    }

    const char *uri = NULL;
    if (at_mqtt_cfg.uri == NULL)
    {
        uri = "";
    }
    else
    {
        uri = at_mqtt_cfg.uri;
    }
    
    arg_t arg[AT_MQTT_CONN_READ_PARA_MAX_NUM] = {
        {ARG_TYPE_NUMBER, .value.number = 0},
        {ARG_TYPE_NUMBER, .value.number = (int)at_mqtt_state},
        {ARG_TYPE_NUMBER, .value.number = (int)at_mqtt_scheme},
        {ARG_TYPE_STRING, .value.string = host},
        {ARG_TYPE_NUMBER, .value.number = at_mqtt_cfg.port},
        {ARG_TYPE_STRING, .value.string = uri},
        {ARG_TYPE_NUMBER, .value.number = !at_mqtt_cfg.disable_auto_reconnect}
    };

    const char handle[] = "MQTTCONN";
    dce_emit_extended_result_code_with_args(dce, handle, -1, arg, AT_MQTT_CONN_READ_PARA_MAX_NUM, true, false);
    return DCE_OK;
}


static dce_result_t dce_handle_mqtt_connect(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_result_t result = DCE_OK;
    if (kind == DCE_WRITE) 
    {
        result = dce_handle_mqtt_connect_set(dce, group_ctx, argc, argv);
        if (result == DCE_OK)
        {
            at_mqtt_state = AT_MQTT_STATE_CONNCFG_SET;
        }
    }
    else if (kind == DCE_READ)
    {
        result = dce_handle_mqtt_connect_get(dce);
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "dce_handle_mqtt_connect(), kind(%d) err!\r\n", kind);
        result = DCE_FAILED;
    }
    
    dce_result_code_t dce_code = DCE_RC_OK;
    if (result != DCE_OK)
    {
        dce_code = DCE_RC_ERROR;
    }
    else
    {
        sub_recv_dce_p = dce;
    }
    dce_emit_basic_result_code(dce, dce_code);
    return result;
}
#if 1

int TG_strtohex(const char * data,int data_len)
{
	
	memset(write_buffer,0,4110);
	//char ID1[12] = "012a3b4c5b6d";
	//uint8_t mesh_id[6];
   // uint8_t temp_id[12];
	int i;
	
	for(i = 0; i < data_len ; i++) {
        if(data[i] >= '0' && data[i] <= '9') {
            write_buffer[i] = data[i] - '0';
        } else {
            write_buffer[i] = data[i] - 'a' + 10;
        }
      // os_printf(LM_OS, LL_INFO," %d = %d  ", i, write_buffer[i]);
    }
	
    for(i = 0; i < data_len ;) {
        write_buffer[i/2] = BUILD_UINT8(write_buffer[i+1], write_buffer[i]);
        i += 2;
    }
	
	//for(i = 0; i < 6 ; i++) {
	//	os_printf(LM_OS, LL_INFO,"%02x\n", write_buffer[i]);
//	}
	
	return 0;

}

#endif
static AT_MQTT_RET_E at_mqtt_send_pub_msg(const char* topic, const char* data, int qos, int retain, MQTT_CFG_MSG_TYPE msg_type)
{
    MQTT_CFG_MSG_ST msg_send = {0};
    
    if (topic != NULL)
    {
        int topic_len = strlen(topic);
        set_mqtt_msg(SET_TOPIC, &msg_send, topic, topic_len);
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "at_mqtt_send_pub_msg(), topic is null!\r\n");
        return AT_MQTT_RET_ERR;
    }

    int data_len = 0;
    if (data != NULL)
    {
        data_len = strlen(data);
		
        os_printf(LM_APP, LL_INFO, "data_len=%d\r\n",data_len);
		
		TG_strtohex(data,data_len);
		
        set_mqtt_msg(SET_DATA, &msg_send, (char *)write_buffer, data_len/2);
    }

    msg_send.qos = qos;
    msg_send.retain = retain;
    msg_send.cfg_type = msg_type;
    if (at_mqtt_client->cfg_queue_handle != NULL)
    {
        if(os_queue_send(at_mqtt_client->cfg_queue_handle, (char *)&msg_send, sizeof(msg_send), 0) != 0 )
        {
            os_free((void *)msg_send.data);
            os_free((void *)msg_send.topic);
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "mqtt_client->cfg_queue_handle is null!\r\n");
        return AT_MQTT_RET_ERR;
    }
    return AT_MQTT_RET_OK;
}

static AT_MQTT_RET_E at_mqtt_send_sub_or_unsub_msg(const char* topic, int qos, MQTT_CFG_MSG_TYPE msg_type)
{
    MQTT_CFG_MSG_ST msg_send = {0};
    if (topic != NULL)
    {
        int topic_len = strlen(topic);
        set_mqtt_msg(SET_TOPIC, &msg_send, topic, topic_len);
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "at_mqtt_send_pub_msg(), topic is null!\r\n");
        return AT_MQTT_RET_ERR;
    }
    msg_send.qos = qos;
    msg_send.cfg_type = msg_type;
    if (at_mqtt_client->cfg_queue_handle != NULL)
    {
        if(os_queue_send(at_mqtt_client->cfg_queue_handle, (char *)&msg_send, sizeof(msg_send), 0) != 0)
        {
            os_free((void *)msg_send.topic);
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "mqtt_client->cfg_queue_handle is null!\r\n");
        return AT_MQTT_RET_ERR;
    }
    
    return AT_MQTT_RET_OK;
}


static AT_MQTT_RET_E at_mqtt_handle_link_id_valid(arg_t argv)
{
    if ((argv.type == ARG_TYPE_NUMBER) && (argv.value.number == 0))
    {
        return AT_MQTT_RET_OK;
    }
    else
    {
//        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__,__LINE__);
        return AT_MQTT_RET_ERR;
    }
}


static AT_MQTT_RET_E at_mqtt_handle_topic_valid(arg_t argv, int *topic_len)
{
    if ((argv.type == ARG_TYPE_STRING) && (argv.value.string != NULL))
    {
        *topic_len = strlen(argv.value.string);
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__,__LINE__);
        return AT_MQTT_RET_ERR;
    }
    
    if (*topic_len > AT_MQTT_TOPIC_MAX_LENGTH)
    {
        os_printf(LM_CMD, LL_ERR, "Topic is too long\r\n");
        return AT_MQTT_RET_ERR;
    }

    return AT_MQTT_RET_OK;
}

static AT_MQTT_RET_E at_mqtt_handle_data_valid(arg_t argv, int *data_len)
{
    if (argv.type == ARG_TYPE_STRING)
    {
        if (argv.value.string != NULL)
        {
            *data_len = strlen(argv.value.string);

			
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
        return AT_MQTT_RET_ERR;
    }

    return AT_MQTT_RET_OK;
}

static AT_MQTT_RET_E at_mqtt_handle_qos_valid(arg_t argv, int *qos)
{
    int qos_tmp = MQTT_QOS0;
    if (argv.type == ARG_TYPE_NUMBER)
    {
        qos_tmp = argv.value.number;
        if ((qos_tmp >= MQTT_QOS0) && (qos_tmp <= MQTT_QOS2))
        {
            *qos = qos_tmp;
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            return AT_MQTT_RET_ERR;
        }

    }
    else
    {
        return AT_MQTT_RET_ERR;
    }

    return AT_MQTT_RET_OK;
}

static AT_MQTT_RET_E at_mqtt_handle_retain_valid(arg_t argv, int *retain)
{
    int retain_tmp = 0;
    if (argv.type == ARG_TYPE_NUMBER)
    {
        retain_tmp = argv.value.number;
        if ((retain_tmp == 0) || (retain_tmp == 1))
        {
            *retain = retain_tmp;
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        return AT_MQTT_RET_ERR;
    }

    return AT_MQTT_RET_OK;
}

static AT_MQTT_RET_E at_mqtt_handle_pub_raw_length_valid(arg_t argv, int *data_length)
{
    int data_length_tmp = 0;
    if (argv.type == ARG_TYPE_NUMBER)
    {
        data_length_tmp = argv.value.number;
        if ((data_length_tmp > 0) && (data_length_tmp <= AT_MQTT_PUB_RAW_DATA_MAX_LEN))
        {
            *data_length = data_length_tmp;
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__, __LINE__);
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        return AT_MQTT_RET_ERR;
    }

    return AT_MQTT_RET_OK;
}


static AT_MQTT_RET_E at_mqtt_handle_pub(dce_t* dce, size_t argc, arg_t* argv)
{
    if (at_mqtt_client == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "%s:%d at_mqtt_client is null!\r\n", __FILE__,__LINE__);
        return AT_MQTT_RET_ERR;
    }

    if( at_mqtt_client->state != MQTT_STATE_CONNECTED)
    {
        return AT_MQTT_RET_ERR;
    }
    
    if ((argc <= AT_MQTT_PUB_TOPIC) || (argc > AT_MQTT_PUB_PARA_MAX))
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __FILE__,__LINE__);
        return AT_MQTT_RET_ERR;
    }

    if (at_mqtt_handle_link_id_valid(argv[AT_MQTT_PUB_LINK_ID]) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, LINK_ID_ERR_PRINT, __FILE__, __LINE__, argv[AT_MQTT_PUB_LINK_ID].value.number);
        return DCE_FAILED;
    }
    
    int topic_len = 0;
    if(at_mqtt_handle_topic_valid(argv[AT_MQTT_PUB_TOPIC], &topic_len) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
        return AT_MQTT_RET_ERR;
    }

    
    int data_len = 0; 
    if (argc > AT_MQTT_PUB_DATA)
    {
        if(at_mqtt_handle_data_valid(argv[AT_MQTT_PUB_DATA], &data_len) != AT_MQTT_RET_OK)
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
            return AT_MQTT_RET_ERR;
        }
    }

    int qos = MQTT_QOS0;
    if (argc > AT_MQTT_PUB_QOS)
    {
        if(at_mqtt_handle_qos_valid(argv[AT_MQTT_PUB_QOS], &qos) != AT_MQTT_RET_OK)
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
            return AT_MQTT_RET_ERR;
        }
    }
   
    int retain = 0;
    if (argc > AT_MQTT_PUB_RETAIN)
    {
        if(at_mqtt_handle_retain_valid(argv[AT_MQTT_PUB_RETAIN], &retain) != AT_MQTT_RET_OK)
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
            return AT_MQTT_RET_ERR;
        }
    }
    
    return at_mqtt_send_pub_msg(argv[AT_MQTT_PUB_TOPIC].value.string, argv[AT_MQTT_PUB_DATA].value.string, qos, retain, CFG_MSG_TYPE_PUB);
}

static dce_result_t dce_handle_mqtt_pub(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_result_t ret = DCE_OK;
    AT_MQTT_RET_E mqtt_pub_ret = at_mqtt_handle_pub(dce, argc, argv);
    dce_result_code_t result_code = DCE_RC_OK;
    if (mqtt_pub_ret != AT_MQTT_RET_OK)
    {
        result_code = DCE_RC_ERROR;
        ret = DCE_FAILED;
    }
    
    dce_emit_basic_result_code(dce, result_code);
    return ret;
}

static void at_mqtt_rx_from_uart_callback(void *param, const char *data, size_t size)
{
    AT_MQTT_PUBRAW_T *data_p = &at_mqtt_pub_raw_data;
    if (data_p->uart_rev_buff.threshold == 0) 
    {
        os_printf(LM_APP, LL_ERR, "%s :%d uart_rev_buff.threshold is 0!\r\n", __func__, __LINE__);
        at_mqtt_uart_rx_reset();
        return;
    }

    int i = 0;
    for (i = 0; ((i < size) && (data_p->uart_rev_buff.pos < data_p->uart_rev_buff.threshold)); ++i) 
    {
        data_p->uart_rev_buff.buff[data_p->uart_rev_buff.pos] = data[i];
        data_p->uart_rev_buff.pos++;
    }

    if (data_p->uart_rev_buff.pos >= data_p->uart_rev_buff.threshold) 
    {
        at_mqtt_send_pub_msg(data_p->topic, data_p->uart_rev_buff.buff, data_p->qos, data_p->retain, CFG_MSG_TYPE_PUB_RAW);
        at_mqtt_uart_rx_reset();
    }
}


static AT_MQTT_RET_E at_mqtt_prepare_rx()
{ 
    if (at_mqtt_pub_raw_data.uart_rev_timer != NULL)
    {
        if (os_timer_start(at_mqtt_pub_raw_data.uart_rev_timer) != 0)
        {
            os_printf(LM_CMD, LL_ERR, "os_timer_start() err!\r\n");
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "at_mqtt_pub_raw_data.uart_rev_timer is null!\r\n");
        return AT_MQTT_RET_ERR;
    }

    dce_register_data_input_cb(at_mqtt_rx_from_uart_callback);
    target_dec_switch_input_state(ONLINE_DATA_STATE);
    return AT_MQTT_RET_OK;
}


static dce_result_t at_mqtt_handle_pub_raw(dce_t* dce, size_t argc, arg_t* argv)
{
    if (at_mqtt_client == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "%s:%d at_mqtt_client is null!\r\n", __FILE__,__LINE__);
        return AT_MQTT_RET_ERR;
    }

    if(at_mqtt_client->state != MQTT_STATE_CONNECTED)
    {
        return AT_MQTT_RET_ERR;
    }
    
    if ((argc <= AT_MQTT_PUB_RAW_LENGTH) || (argc > AT_MQTT_PUB_RAW_PARA_MAX))
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__,__LINE__);
        return AT_MQTT_RET_ERR;
    }

    if (at_mqtt_handle_link_id_valid(argv[AT_MQTT_PUB_RAW_LINK_ID]) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, LINK_ID_ERR_PRINT, __FILE__, __LINE__, argv[AT_MQTT_PUB_RAW_LINK_ID].value.number);
        return DCE_FAILED;
    }
    
    
    int topic_len = 0;
    if(at_mqtt_handle_topic_valid(argv[AT_MQTT_PUB_RAW_TOPIC], &topic_len) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
        return AT_MQTT_RET_ERR;
    }
    
    int data_length = 0; 
    if(at_mqtt_handle_pub_raw_length_valid(argv[AT_MQTT_PUB_RAW_LENGTH], &data_length) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
        return AT_MQTT_RET_ERR;
    }

    int qos = MQTT_QOS0;
    if (argc > AT_MQTT_PUB_RAW_QOS)
    {
        if(at_mqtt_handle_qos_valid(argv[AT_MQTT_PUB_RAW_QOS], &qos) != AT_MQTT_RET_OK)
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
            return AT_MQTT_RET_ERR;
        }
    }
    int retain = 0;
    if (argc > AT_MQTT_PUB_RAW_RETAIN)
    {
        if(at_mqtt_handle_retain_valid(argv[AT_MQTT_PUB_RAW_RETAIN], &retain) != AT_MQTT_RET_OK)
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
            return AT_MQTT_RET_ERR;
        }
    }
    
    if (at_mqtt_prepare_rx() == AT_MQTT_RET_OK)
    {
        memset(at_mqtt_pub_raw_data.topic, 0, sizeof(at_mqtt_pub_raw_data.topic));
        memcpy(at_mqtt_pub_raw_data.topic, argv[AT_MQTT_PUB_RAW_TOPIC].value.string, topic_len);
        at_mqtt_pub_raw_data.qos = qos;
        at_mqtt_pub_raw_data.retain = retain;
        at_mqtt_pub_raw_data.uart_rev_buff.pos = 0;
        at_mqtt_pub_raw_data.uart_rev_buff.threshold = data_length;
    }
    else
    {
        at_mqtt_uart_rx_reset();
        return AT_MQTT_RET_ERR;
    }
    
    return AT_MQTT_RET_OK;
}

dce_result_t dce_handle_mqtt_pub_raw(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_result_t ret = DCE_OK;
    AT_MQTT_RET_E mqtt_pub_raw_ret = at_mqtt_handle_pub_raw(dce, argc, argv);
    if (mqtt_pub_raw_ret == AT_MQTT_RET_OK)
    {
        dce_emit_information_response(dce, ">", -1);
    }
    else
    {
        ret = DCE_FAILED;
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    }
    
    return ret;
}

static dce_result_t dce_handle_mqtt_sub_set(dce_t* dce, void* group_ctx, size_t argc, arg_t* argv)
{
    if (argc < AT_MQTT_SUB_QOS)
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__,__LINE__);
        return DCE_INVALID_INPUT;
    }
    
    if (at_mqtt_handle_link_id_valid(argv[AT_MQTT_SUB_LINK_ID]) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, LINK_ID_ERR_PRINT, __func__, __LINE__, argv[AT_MQTT_SUB_LINK_ID].value.number);
        return DCE_INVALID_INPUT;
    }
    
    int topic_len = 0;
    if(at_mqtt_handle_topic_valid(argv[AT_MQTT_SUB_TOPIC], &topic_len) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
        return DCE_INVALID_INPUT;
    }

    int qos = MQTT_QOS0;
    if (argc == AT_MQTT_SUB_PARA_MAX)
    {
        if(at_mqtt_handle_qos_valid(argv[AT_MQTT_SUB_QOS], &qos) != AT_MQTT_RET_OK)
        {
            os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
            return DCE_INVALID_INPUT;
        }
    }
    
    int qos_old = 0;
    if (at_mqtt_sub_list != NULL)
    {
        if (at_mqtt_sub_topic_query(at_mqtt_sub_list, argv[AT_MQTT_SUB_TOPIC].value.string, &qos_old) == AT_MQTT_RET_OK)
        {
            if (qos_old == qos)
            {
                dce_emit_information_response(dce, ALREADY_SUB, -1);//return
                return DCE_OK;
            }
            else
            {
                at_mqtt_sub_topic_delete(at_mqtt_sub_list, argv[AT_MQTT_SUB_TOPIC].value.string, topic_len);//no unsub from server
            }
        }

        if (at_mqtt_send_sub_or_unsub_msg(argv[AT_MQTT_SUB_TOPIC].value.string, qos, CFG_MSG_TYPE_SUB) == AT_MQTT_RET_OK)
        {
            at_mqtt_sub_topic_enqueue(at_mqtt_sub_list, argv[AT_MQTT_SUB_TOPIC].value.string, qos);
        }
        else
        {
            os_printf(LM_CMD, LL_ERR, "%s:%d: at_mqtt_send_sub_or_unsub_msg() return err!\r\n", __func__, __LINE__);
            return DCE_FAILED;
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "%s:%d: at_mqtt_sub_list is null\r\n", __func__, __LINE__);
    }

    return DCE_OK;
}

static dce_result_t dce_handle_mqtt_sub_get(dce_t* dce)
{
    int sub_top_num = 0;
    if (at_mqtt_sub_list == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "%s:%d: at_mqtt_sub_list is null\r\n", __func__, __LINE__);
        return DCE_FAILED;
    }

    arg_t arg[AT_MQTT_SUB_READ_PARA_MAX] = {
        {ARG_TYPE_NUMBER, .value.number = 0},
        {ARG_TYPE_NUMBER, .value.number = (int)at_mqtt_state},
        {ARG_TYPE_STRING, .value.string = NULL},
        {ARG_TYPE_NUMBER, .value.number = 0},
    };

    const char handle[] = "MQTTSUB";
    sub_topic_item_handle_t item_p = NULL;
    STAILQ_FOREACH(item_p, at_mqtt_sub_list, next)
    {
        sub_top_num++;
        if (sub_top_num > AT_MQTT_SUB_TOPIC_MAX_NUM)
        {
            os_printf(LM_APP, LL_ERR, "AT MQTT:the num of sub-topic too much!\r\n");
            break;
        }
        if (item_p != NULL)
        {
            arg[AT_MQTT_SUB_READ_TOPIC].value.string = item_p->topic;
            arg[AT_MQTT_SUB_READ_QOS].value.number = item_p->qos;
            dce_emit_extended_result_code_with_args(dce, handle, -1, arg, AT_MQTT_SUB_READ_PARA_MAX, true, false);
        }
        else
        {
            break;
        }
    }
    
    return DCE_OK;
}

static dce_result_t dce_handle_mqtt_sub(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (at_mqtt_client == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "%s:%d at_mqtt_client is null!\r\n", __FILE__,__LINE__);
        return DCE_RC_ERROR;
    }

    if(at_mqtt_client->state != MQTT_STATE_CONNECTED)
    {
        return AT_MQTT_RET_ERR;
    }
    
    dce_result_t result = DCE_OK;
    if (kind == DCE_WRITE) 
    {
        result = dce_handle_mqtt_sub_set(dce, group_ctx, argc, argv);
        at_mqtt_state = AT_MQTT_STATE_CONN_SUB_TOPIC;
    }
    else if (kind == DCE_READ)
    {
        result = dce_handle_mqtt_sub_get(dce);
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "dce_handle_mqtt_connect(), kind(%d) err!\r\n", kind);
        result = DCE_FAILED;
    }
    
    dce_result_code_t dce_code = DCE_RC_OK;
    if (result == DCE_OK)
    {
        sub_recv_dce_p = dce;
    }
    else
    {
        dce_code = DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, dce_code);
    return result;
}

static dce_result_t dce_handle_mqtt_unsub(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (at_mqtt_client == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "%s:%d at_mqtt_client is null!\r\n", __FILE__,__LINE__);
        return DCE_RC_ERROR;
    }
    
    if(at_mqtt_client->state != MQTT_STATE_CONNECTED)
    {
        return AT_MQTT_RET_ERR;
    }
    
    dce_result_t result = DCE_OK;
    if (argc != AT_MQTT_UNSUB_PARA_MAX)
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__,__LINE__);
        result = DCE_INVALID_INPUT;
    }
    
    if (at_mqtt_handle_link_id_valid(argv[AT_MQTT_UNSUB_LINK_ID]) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, LINK_ID_ERR_PRINT, __func__, __LINE__, argv[AT_MQTT_UNSUB_LINK_ID].value.number);
        result = DCE_INVALID_INPUT;
    }
    
    int topic_len = 0;
    if(at_mqtt_handle_topic_valid(argv[AT_MQTT_UNSUB_TOPIC], &topic_len) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__, __LINE__);
        result = DCE_INVALID_INPUT;
    }

    if (at_mqtt_sub_list != NULL)
    {
        int qos = 0;
        if (at_mqtt_sub_topic_query(at_mqtt_sub_list, argv[AT_MQTT_SUB_TOPIC].value.string, &qos) == AT_MQTT_RET_OK)
        {
            if (at_mqtt_send_sub_or_unsub_msg(argv[AT_MQTT_SUB_TOPIC].value.string, qos, CFG_MSG_TYPE_UNSUB) == AT_MQTT_RET_OK)
            {
                at_mqtt_sub_topic_delete(at_mqtt_sub_list, argv[AT_MQTT_SUB_TOPIC].value.string, qos);
            }
            else
            {
                os_printf(LM_CMD, LL_ERR, "%s:%d: at_mqtt_send_sub_or_unsub_msg() return err!\r\n", __func__, __LINE__);
                result = DCE_FAILED;
            }
        }
        else
        {
            dce_emit_information_response(dce, NO_UNSUB, -1);
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "%s:%d: at_mqtt_sub_list is null\r\n", __func__, __LINE__);
        result = DCE_FAILED;
    }
    
    dce_result_code_t dce_code = DCE_RC_OK;
    if (result != DCE_OK)
    {
        dce_code = DCE_RC_ERROR;
    }

    dce_emit_basic_result_code(dce, dce_code);
    return result;
}

static dce_result_t at_mqtt_handle_clean(size_t argc, arg_t* argv)
{
    dce_result_t result = DCE_OK;
    if (argc != 1)
    {
        os_printf(LM_CMD, LL_ERR, PARA_ERR_PRINT, __func__,__LINE__);
        return DCE_INVALID_INPUT;
    }
    
    if (at_mqtt_handle_link_id_valid(argv[0]) != AT_MQTT_RET_OK)
    {
        os_printf(LM_CMD, LL_ERR, LINK_ID_ERR_PRINT, __func__, __LINE__, argv[0].value.number);
        return DCE_INVALID_INPUT;
    }

    if (at_mqtt_sub_list != NULL)
    {
        at_mqtt_sub_list_destroy(at_mqtt_sub_list);
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "%s:%d: at_mqtt_sub_list is null!\r\n", __func__, __LINE__);
        result = DCE_FAILED;
    }

    if (at_mqtt_client == NULL)
    {
        os_printf(LM_CMD, LL_ERR, "%s:%d at_mqtt_client is null!\r\n", __FILE__,__LINE__);
        return DCE_RC_ERROR;
    }
        
    MQTT_CFG_MSG_ST msg_send = {0};
    msg_send.cfg_type = CFG_MSG_TYPE_STOP;
    if (at_mqtt_client->cfg_queue_handle != NULL)
    {
        if(os_queue_send(at_mqtt_client->cfg_queue_handle, (char *)&msg_send, sizeof(msg_send), 0) == 0)
        {
            at_mqtt_client = NULL;
        } 
        else
        {
            return AT_MQTT_RET_ERR;
        }
    }
    else
    {
        os_printf(LM_CMD, LL_ERR, "mqtt_client->cfg_queue_handle is null!\r\n");
        result = DCE_FAILED;
    }

    return result;
}

void skylab_mqtt_cean_status(void){
     if (at_mqtt_sub_list != NULL)
    {
        at_mqtt_sub_list_destroy(at_mqtt_sub_list);
    }
	 else
		 os_printf(LM_APP, LL_INFO,"skylab_mqtt_cean_status at_mqtt_sub_list==NULL\r\n");
	 
	 if (at_mqtt_client == NULL)
	 {
		 os_printf(LM_APP, LL_INFO,"skylab_mqtt_cean_status at_mqtt_client==NULL\r\n");
		 return;
	 }
		 
	 MQTT_CFG_MSG_ST msg_send = {0};
	 msg_send.cfg_type = CFG_MSG_TYPE_STOP;
	 if (at_mqtt_client->cfg_queue_handle != NULL)
	 {
		 if(os_queue_send(at_mqtt_client->cfg_queue_handle, (char *)&msg_send, sizeof(msg_send), 0) == 0)
		 {
			 at_mqtt_client = NULL;
			 os_printf(LM_APP, LL_INFO,"mqtt_cean success\r\n");
		 }else
		 	 os_printf(LM_APP, LL_INFO,"mqtt_cean fail\r\n");
	 }
	 else
		 os_printf(LM_APP, LL_INFO,"at_mqtt_client->cfg_queue_handle == NULL\r\n");
}

static dce_result_t dce_handle_mqtt_clean(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_result_code_t dce_code = DCE_RC_OK;
    dce_result_t result = at_mqtt_handle_clean( argc, argv);
    if (result == DCE_OK)
    {
        at_mqtt_state = AT_MQTT_STATE_NOT_INIT;
    }
    else
    {
        dce_code = DCE_RC_ERROR;
    }
    
    dce_emit_basic_result_code(dce, dce_code);
    return result;
}
unsigned char HexToAscii(unsigned char c){
	if(c > 9){
		return (c+55);     // 16进制 0A 对应的ascii 'A'，两者相差55  -- 可以比照ascii表理解
	}else{
		return (c+48);     // 16进制 01 对应的ascii '1'  两者相差48  -- 可以比照ascii表理解
	}
}

int gpio_timer_handle=0;
uint8_t skylab_timer_enable = 0;
extern uint8_t psm_usr_sleep_mode_status;
void Skylab_TimerOut_Icr(void * data){
	hal_gpio_write(GPIO_NUM_3,0);
	skylab_timer_enable = 0;
	os_printf(LM_APP, LL_INFO,"Skylab_TimerOut_Icr\r\n");
	hal_timer_stop(gpio_timer_handle);
	hal_timer_delete(gpio_timer_handle);
	if(psm_usr_sleep_mode_status){
		psm_set_sleep_mode(LIGHT_SLEEP,7);
	}
}

static void at_mqtt_write_sub_recv_data_to_uart(int topic_len, const char *topic, int data_len, const char *data)
{
	
    if (topic_len == 0)
    {
        return;
    }

    char *topic_local = os_malloc(topic_len + 1);
    if (topic_local == NULL)
    {
        return;
    }
    
    memset(topic_local, 0, topic_len + 1);
    memcpy(topic_local, topic, topic_len);

    char *data_local = os_malloc(data_len + 1);
    if (data_local == NULL)
    {
        os_free(topic_local);
        return;
    }

    memset(data_local, 0, data_len + 1);
    memcpy(data_local, data, data_len);
	
     unsigned char *a = os_malloc(4096);
	if (a == NULL)
    {
        return;
    }
	os_printf(LM_OS, LL_INFO, "data_len=%d\r\n",data_len);
	if(data_len>2048)
		{
	
		data_len=2048;
	}
	//unsigned char temp;
//	for(int i =0 ; i<data_len;i++)
//	{
//		
//	//os_printf(LM_OS, LL_INFO, "data=%02x\r\n",*((unsigned char *)data+i));
//	
//	sprintf((char *)(a+2*i), "%02x",*((unsigned char *)data+i));
//	
//	//os_printf(LM_OS, LL_INFO, "a=%s\r\n",a);
//	//temp = data[i] & 0xf0;   // 取16进制数高位放到 HexToAscii 函数中转成字符
//	//*(a + i*3) = HexToAscii(temp >> 4);
//	//temp = data[i] & 0x0f;   // 取16进制数低位放到 HexToAscii 函数中转成字符
//	//*(a + i*3 + 1) = HexToAscii(temp);
//	//*(a + i*3 + 2) = ' ';  // 添加空格进行区分

//	}
	if(psm_usr_sleep_mode_status){
		psm_set_sleep_mode(SLEEP_OFF, 0);
		os_printf(LM_OS, LL_INFO, "SLEEP_OFF\r\n");
	}
    arg_t arg[AT_MQTT_SUB_RECV_PARA_MAX] = {
        {ARG_TYPE_NUMBER, .value.number = topic_len},
        {ARG_TYPE_STRING, .value.string = topic_local},
        {ARG_TYPE_NUMBER, .value.number = data_len},		//data_len*2
        {ARG_TYPE_STRING, .value.string = (char *)data},  //a
    };
    const char * handle = "MQTTSUBRECV";
	os_printf(LM_OS, LL_INFO, "skylab_timer_enable=%d\r\n",skylab_timer_enable);
	if(skylab_timer_enable == 0){
		gpio_timer_handle = hal_timer_create(100000, Skylab_TimerOut_Icr, 0, 0);
		if(gpio_timer_handle < 0){
			os_printf(LM_APP, LL_INFO, "hal_timer_create error(%d)!\r\n", gpio_timer_handle);
			skylab_timer_enable = 0;
		}else
			skylab_timer_enable = 0;		
	}
	if(skylab_timer_enable == 0){
		skylab_timer_enable = 1;
		hal_gpio_write(GPIO_NUM_3,1);
		os_printf(LM_APP, LL_INFO, "hal_timer_start=%d\r\n",skylab_timer_enable);
		hal_timer_start(gpio_timer_handle);
	}
    TG_dce_emit_extended_result_code_with_args(sub_recv_dce_p, handle, -1, arg, AT_MQTT_SUB_RECV_PARA_MAX, true, false,data_len); //data_len*2
    os_free(topic_local);
    os_free(data_local);
	os_free(a);
}



static const command_desc_t mqtt_commands[] = {
    {"MQTTUSERCFG", &dce_handle_mqtt_set_usercfg, DCE_TEST | DCE_WRITE},
    {"MQTTCLIENTID", &dce_handle_mqtt_set_client_id, DCE_TEST | DCE_WRITE},
    {"MQTTUSERNAME", &dce_handle_mqtt_set_user_name, DCE_TEST | DCE_WRITE},
    {"MQTTPASSWORD", &dce_handle_mqtt_set_password, DCE_TEST | DCE_WRITE},
    {"MQTTCONNCFG", &dce_handle_mqtt_set_conn_cfg, DCE_TEST | DCE_WRITE},
    {"MQTTCONN", &dce_handle_mqtt_connect, DCE_EXEC | DCE_WRITE | DCE_READ},
    {"MQTTPUB", &dce_handle_mqtt_pub, DCE_EXEC | DCE_WRITE},
    {"MQTTPUBRAW", &dce_handle_mqtt_pub_raw, DCE_EXEC},
    {"MQTTSUB", &dce_handle_mqtt_sub, DCE_TEST | DCE_WRITE | DCE_READ},
    {"MQTTUNSUB", &dce_handle_mqtt_unsub, DCE_TEST | DCE_WRITE},
    {"MQTTCLEAN", &dce_handle_mqtt_clean, DCE_TEST | DCE_WRITE}
};


static void at_mqtt_pub_raw_handle(MQTT_PUB_STATUS_E pub_status)
{
    os_timer_stop(at_mqtt_pub_raw_data.uart_rev_timer);
    if (pub_status == MQTT_PUB_SUCCESS)
    {
        target_dce_transmit(PUB_RAW_SUCCESS, strlen(PUB_RAW_SUCCESS));
    }
    else
    {
        target_dce_transmit(PUB_RAW_FAIL, strlen(PUB_RAW_FAIL));
    }
}

static int at_mqtt_event_handle(trs_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            at_mqtt_state = AT_MQTT_STATE_CONNECTED;
            os_printf(LM_CMD, LL_DBG, MQTT_CONN_STRING);
            break;
			
        case MQTT_EVENT_DISCONNECTED:
            if (at_mqtt_cfg.clean_session == 1)
            {
                if (at_mqtt_sub_list != NULL)
                {
                    at_mqtt_sub_list_destroy(at_mqtt_sub_list);
                }
                else
                {
                    os_printf(LM_APP, LL_INFO, "%s:%d: at_mqtt_sub_list is null!\r\n", __func__, __LINE__);
                }
            }
            
            at_mqtt_state = AT_MQTT_STATE_DISCONN;
			os_printf(LM_CMD, LL_DBG, MQTT_DISCONN_STRING);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            at_mqtt_state = AT_MQTT_STATE_CONN_SUB_TOPIC;
            os_printf(LM_CMD, LL_DBG, MQTT_SUB_STRING, event->msg_id);
            break;
        
        case MQTT_EVENT_UNSUBSCRIBED:
			os_printf(LM_CMD, LL_DBG, MQTT_UNSUB_STRING, event->msg_id);
            break;
        
        case MQTT_EVENT_PUBLISHED:
			os_printf(LM_CMD, LL_DBG, MQTT_PUB_STRING, event->msg_id);
            break;
        
        case MQTT_EVENT_DATA:
			
            os_printf(LM_CMD, LL_DBG, MQTT_DATA_STRING);
            at_mqtt_write_sub_recv_data_to_uart(event->topic_len, event->topic, event->data_len, event->data);
		
            if (event->topic_len < PRINT_STRING_MAX_LEN)
            {
                os_printf(LM_CMD, LL_INFO, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
            }
            else
            {
                os_printf(LM_CMD, LL_DBG, MQTT_TOPIC_TOO_LONG_HINT);
            }

            if (event->data_len < PRINT_STRING_MAX_LEN)
            {
                if (event->data_len == 0)
                {
                    os_printf(LM_APP, LL_INFO, "DATA is null!\r\n");
                }
                else
                {
                	
                    os_printf(LM_APP, LL_INFO, "data_len =%d DATA=%.*s\r\n", event->data_len, (unsigned char *)event->data);
                }
            }
            else
            {
                os_printf(LM_CMD, LL_DBG, MQTT_DATA_TOO_LONG_HINT);
            }
            break;
            
        case MQTT_EVENT_ERROR:
			os_printf(LM_CMD, LL_DBG, MQTT_ERROR_STRING);
            break;
        
        default:
            os_printf(LM_CMD, LL_ERR, "mqtt_event_id is err(%d)!", event->event_id);
            break;
    }
    
    return 0;
}

extern void MqttReadLed(void);
void dce_register_mqtt_commands(dce_t* dce)
{
    dce_register_command_group(dce, "MQTT", mqtt_commands, sizeof(mqtt_commands) / sizeof(mqtt_commands[0]), 0);
    at_mqtt_cfg.event_handle = at_mqtt_event_handle;
    at_mqtt_cfg.pub_handle = at_mqtt_pub_raw_handle;
    at_mqtt_cfg.buffer_size = AT_MQTT_BUFFER_SIZE;
    at_mqtt_pubraw_init();
	//hal_gpio_init();
	MqttReadLed();
	gpio_timer_handle = hal_timer_create(100000, Skylab_TimerOut_Icr, 0, 0);
	if(gpio_timer_handle < 0){
		os_printf(LM_APP, LL_INFO, "hal_timer_create error(%d)!\r\n", gpio_timer_handle);
		skylab_timer_enable = 0xFF;
	}
	skylab_timer_enable = 0;
    at_mqtt_sub_list = at_mqtt_sub_list_init();
    MEM_CHECK(at_mqtt_sub_list, {
        return ;
    });
}


