/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 * Tuan PM <tuanpm at live dot com>
 */

#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "mqtt_config.h"


#include "mqtt_msg.h"
#include "transport.h"
#include "transport_tcp.h"
#include "transport_ssl.h"
#include "transport_ws.h"
#include "platform_tr6600.h"
#include "mqtt_outbox.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MQTT_CONN_STRING "MQTT_EVENT_CONNECTED!\r\n"
#define MQTT_DISCONN_STRING "MQTT_EVENT_DISCONNECTED!\r\n"
#define MQTT_SUB_STRING "MQTT_EVENT_SUBSCRIBED, msg_id=%d;\r\n"
#define MQTT_UNSUB_STRING "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d;\r\n"
#define MQTT_PUB_STRING "MQTT_EVENT_PUBLISHED, msg_id=%d;\r\n"
#define MQTT_DATA_STRING "MQTT_EVENT_DATA!\r\n"
#define MQTT_ERROR_STRING "MQTT_EVENT_ERROR!\r\n"
#define MQTT_TOPIC_TOO_LONG_HINT "The length of TOPIC exceeds the length of the print buffer, so do't printf!\r\n"
#define MQTT_DATA_TOO_LONG_HINT "The length of DATA exceeds the length of the print buffer, so do't printf!\r\n"

#define PRINT_STRING_MAX_LEN (CLI_PRINT_BUFFER_SIZE - 32) 

typedef enum{
    MQTT_PUB_SUCCESS = 0,
    MQTT_PUB_FAILED   
}MQTT_PUB_STATUS_E;

typedef void (* mqtt_pub_callback_t)(MQTT_PUB_STATUS_E pub_status);
typedef struct trs_mqtt_client* trs_mqtt_client_handle_t;

/**
 * @brief MQTT event types.
 *
 * User event handler receives context data in `trs_mqtt_event_t` structure with
 *  - `user_context` - user data from `trs_mqtt_client_config_t`
 *  - `client` - mqtt client handle
 *  - various other data depending on event type
 *
 */
typedef enum {
    MQTT_EVENT_ERROR = 0,
    MQTT_EVENT_CONNECTED,          /*!< connected event, additional context: session_present flag */
    MQTT_EVENT_DISCONNECTED,       /*!< disconnected event */
    MQTT_EVENT_SUBSCRIBED,         /*!< subscribed event, additional context: msg_id */
    MQTT_EVENT_UNSUBSCRIBED,       /*!< unsubscribed event */
    MQTT_EVENT_PUBLISHED,          /*!< published event, additional context:  msg_id */
    MQTT_EVENT_DATA,               /*!< data event, additional context:
                                        - msg_id               message id
                                        - topic                pointer to the received topic
                                        - topic_len            length of the topic
                                        - data                 pointer to the received data
                                        - data_len             length of the data for this event
                                        - current_data_offset  offset of the current data for this event
                                        - total_data_len       total length of the data received
                                         */
} trs_mqtt_event_id_t;

typedef enum {
    MQTT_TRANSPORT_UNKNOWN = 0x0,
    MQTT_TRANSPORT_OVER_TCP,      /*!< MQTT over TCP, using scheme: ``mqtt`` */
    MQTT_TRANSPORT_OVER_SSL,      /*!< MQTT over SSL, using scheme: ``mqtts`` */
    MQTT_TRANSPORT_OVER_WS,       /*!< MQTT over Websocket, using scheme:: ``ws`` */
    MQTT_TRANSPORT_OVER_WSS       /*!< MQTT over Websocket Secure, using scheme: ``wss`` */
} trs_mqtt_transport_t;

/**
 * MQTT event configuration structure
 */
typedef struct {
    trs_mqtt_event_id_t event_id;       /*!< MQTT event type */
    trs_mqtt_client_handle_t client;    /*!< MQTT client handle for this event */
    void *user_context;                 /*!< User context passed from MQTT client config */
    char *data;                         /*!< Data asociated with this event */
    int data_len;                       /*!< Lenght of the data for this event */
    int total_data_len;                 /*!< Total length of the data (longer data are supplied with multiple events) */
    int current_data_offset;            /*!< Actual offset for the data asociated with this event */
    char *topic;                        /*!< Topic asociated with this event */
    int topic_len;                      /*!< Length of the topic for this event asociated with this event */
    int msg_id;                         /*!< MQTT messaged id of message */
    int session_present;                /*!< MQTT session_present flag for connection event */
} trs_mqtt_event_t;

typedef trs_mqtt_event_t* trs_mqtt_event_handle_t;

typedef int (* mqtt_event_callback_t)(trs_mqtt_event_handle_t event);

/**
 * MQTT client configuration structure
 */
typedef struct {
    mqtt_event_callback_t event_handle;     /*!< handle for MQTT events */
    mqtt_pub_callback_t pub_handle;         /*!< handle for pub success or failed */
    const char *host;                       /*!< MQTT server domain (ipv4 as string) */
    const char *uri;                        /*!< Complete MQTT broker URI */
    uint32_t port;                          /*!< MQTT server port */
    const char *client_id;                  /*!< default client id */
    const char *username;                   /*!< MQTT username */
    const char *password;                   /*!< MQTT password */
    const char *lwt_topic;                  /*!< LWT (Last Will and Testament) message topic (NULL by default) */
    const char *lwt_msg;                    /*!< LWT message (NULL by default) */
    int lwt_qos;                            /*!< LWT message qos */
    int lwt_retain;                         /*!< LWT retained message flag */
    int lwt_msg_len;                        /*!< LWT message length */
    int clean_session;              /*!< mqtt clean session, default clean_session is true */
    int keepalive;                          /*!< mqtt keepalive, default is 120 seconds */
    bool disable_auto_reconnect;            /*!< this mqtt client will reconnect to server (when errors/disconnect). Set disable_auto_reconnect=true to disable */
    void *user_context;                     /*!< pass user context to this option, then can receive that context in ``event->user_context`` */
    int task_prio;                          /*!< MQTT task priority, default is 5, can be changed in ``make menuconfig`` */
    int task_stack;                         /*!< MQTT task stack size, default is 6144 bytes, can be changed in ``make menuconfig`` */
    int buffer_size;                        /*!< size of MQTT send/receive buffer, default is 1024 */
    const char *cert_pem;                   /*!< Pointer to certificate data in PEM format for server verify (with SSL), default is NULL, not required to verify the server */
    const char *client_cert_pem;            /*!< Pointer to certificate data in PEM format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_key_pem` has to be provided. */
    const char *client_key_pem;             /*!< Pointer to private key data in PEM format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_cert_pem` has to be provided. */
    trs_mqtt_transport_t transport;         /*!< overrides URI transport */
} trs_mqtt_client_config_t;

typedef struct mqtt_state
{
    mqtt_connect_info_t *connect_info;
    uint8_t *in_buffer;
    uint8_t *out_buffer;
    int in_buffer_length;
    int out_buffer_length;
    uint32_t message_length;
    uint32_t message_length_read;
    mqtt_message_t *outbound_message;
    mqtt_connection_t mqtt_connection;
    uint16_t pending_msg_id;
    int pending_msg_type;
    int pending_publish_qos;
    int pending_msg_count;
} mqtt_state_t;

typedef struct {
    mqtt_event_callback_t event_handle;
    mqtt_pub_callback_t pub_handle;
    int task_stack;
    int task_prio;
    char *uri;
    char *host;
    char *path;
    char *scheme;
    int port;
    bool auto_reconnect;
    void *user_context;
    int network_timeout_ms;
} mqtt_config_storage_t;

typedef enum {
    MQTT_STATE_ERROR = -1,
    MQTT_STATE_UNKNOWN = 0,
    MQTT_STATE_INIT,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_WAIT_TIMEOUT,
} mqtt_client_state_t;

struct trs_mqtt_client {
    transport_list_handle_t transport_list;
    transport_handle_t transport;
    os_queue_handle_t cfg_queue_handle;
    mqtt_config_storage_t *config;
    mqtt_state_t  mqtt_state;
    mqtt_connect_info_t connect_info;
    mqtt_client_state_t state;
    long long keepalive_tick;
    long long reconnect_tick;
    int wait_timeout_ms;
    int auto_reconnect;
    trs_mqtt_event_t event;
    bool run;
    bool wait_for_ping_resp;
    char heartbeat_count;
    outbox_handle_t outbox;
};


typedef enum
{
   CFG_MSG_TYPE_SUB = 1,
   CFG_MSG_TYPE_UNSUB,   
   CFG_MSG_TYPE_PUB, 
   CFG_MSG_TYPE_PUB_RAW, 
   CFG_MSG_TYPE_STOP, 
}MQTT_CFG_MSG_TYPE;


typedef struct 
{
    MQTT_CFG_MSG_TYPE cfg_type;
    const char *topic;
    int topic_len;
    const char *data;
    int data_len;
    int qos;
    int retain;
} MQTT_CFG_MSG_ST;


typedef enum
{
    SET_TOPIC = 1,
    SET_DATA,
}SET_TYPE_EN;


trs_mqtt_client_handle_t trs_mqtt_client_init(const trs_mqtt_client_config_t *config);
int trs_mqtt_client_set_uri(trs_mqtt_client_handle_t client, const char *uri);
int trs_mqtt_client_start(trs_mqtt_client_handle_t client);
int trs_mqtt_client_stop(trs_mqtt_client_handle_t client);
int trs_mqtt_client_subscribe(trs_mqtt_client_handle_t client, const char *topic, int qos);
int trs_mqtt_client_unsubscribe(trs_mqtt_client_handle_t client, const char *topic);
int trs_mqtt_client_publish(trs_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain);
int trs_mqtt_client_destroy(trs_mqtt_client_handle_t client);
int trs_mqtt_client_get_states(trs_mqtt_client_handle_t client);
char* trs_mqtt_client_get_url(trs_mqtt_client_handle_t client);
int set_mqtt_msg(SET_TYPE_EN type, MQTT_CFG_MSG_ST *msg, const char *data, int data_len);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif
