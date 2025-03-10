#include <stdio.h>
#include "errno.h"

#include "system_def.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "portmacro.h"

#include "mqtt_client.h"
#include "mqtt_command.h"

#include "url_parser.h"
#include "oshal.h"
#include "at_common.h"


/* using uri parser */

#define MQTT_QUEUE_LENGTH 8
#define MQTT_REV_MSG_TIMEOUT 0
#define MQTT_HEART_BEAT_MAX 3

#define MAX_REMAINING_LENGTH_BYTES 4

uint ping_resp = 0;
uint ping_num = 0;


static int trs_mqtt_dispatch_event(trs_mqtt_client_handle_t client);
static int trs_mqtt_set_config(trs_mqtt_client_handle_t client, const trs_mqtt_client_config_t *config);
static int trs_mqtt_destroy_config(trs_mqtt_client_handle_t client);
static int trs_mqtt_connect(trs_mqtt_client_handle_t client, int timeout_ms);

static int trs_mqtt_read_msg(trs_mqtt_client_handle_t client);
static int trs_mqtt_get_packet_len(trs_mqtt_client_handle_t client, int* value);

static int trs_mqtt_abort_connection(trs_mqtt_client_handle_t client);
static int trs_mqtt_client_ping(trs_mqtt_client_handle_t client);
static char *create_string(const char *ptr, int len);



static int trs_mqtt_set_config(trs_mqtt_client_handle_t client, const trs_mqtt_client_config_t *config)
{
    //Copy user configurations to client context
    int err = 0;
    mqtt_config_storage_t *cfg = mqtt_calloc(1, sizeof(mqtt_config_storage_t));

    client->config = cfg;

    cfg->task_prio = config->task_prio;
    if (cfg->task_prio <= 0) {
        cfg->task_prio = MQTT_TASK_PRIORITY;
    }

    cfg->task_stack = config->task_stack;
    if (cfg->task_stack == 0) {
        cfg->task_stack = MQTT_TASK_STACK;
    }
    err = -1;
    if (config->host) {
        cfg->host = os_strdup(config->host);
        MEM_CHECK(cfg->host, goto _mqtt_set_config_failed);
    }
    cfg->port = config->port;

    if (config->username) {
        client->connect_info.username = os_strdup(config->username);
        MEM_CHECK(client->connect_info.username, goto _mqtt_set_config_failed);
    }

    if (config->password) {
        client->connect_info.password = os_strdup(config->password);
        MEM_CHECK(client->connect_info.password, goto _mqtt_set_config_failed);
    }

    if (config->client_id) {
        client->connect_info.client_id = os_strdup(config->client_id);
    } else {
        client->connect_info.client_id = platform_create_id_string();
    }
    MEM_CHECK(client->connect_info.client_id, goto _mqtt_set_config_failed);
    os_printf(LM_APP, LL_INFO, "MQTT:client_id=%s\n", client->connect_info.client_id);

    if (config->uri) {
        cfg->uri = os_strdup(config->uri);
        MEM_CHECK(cfg->uri, goto _mqtt_set_config_failed);
    }

    if (config->lwt_topic) {
        client->connect_info.will_topic = os_strdup(config->lwt_topic);
        MEM_CHECK(client->connect_info.will_topic, goto _mqtt_set_config_failed);
    }

    if (config->lwt_msg_len) {
        client->connect_info.will_message = mqtt_malloc(config->lwt_msg_len);
        MEM_CHECK(client->connect_info.will_message, goto _mqtt_set_config_failed);
        memcpy(client->connect_info.will_message, config->lwt_msg, config->lwt_msg_len);
        client->connect_info.will_length = config->lwt_msg_len;
    } else if (config->lwt_msg) {
        client->connect_info.will_message = os_strdup(config->lwt_msg);
        MEM_CHECK(client->connect_info.will_message, goto _mqtt_set_config_failed);
        client->connect_info.will_length = strlen(config->lwt_msg);
    }
    else {
        ;
    }

    client->connect_info.will_qos = config->lwt_qos;
    client->connect_info.will_retain = config->lwt_retain;

    client->connect_info.clean_session = 1;
    if (config->clean_session == 0) {
        client->connect_info.clean_session = 0;
    }
    client->connect_info.keepalive = config->keepalive;
    if (client->connect_info.keepalive == 0) {
        client->connect_info.keepalive = MQTT_KEEPALIVE_TICK;
    }
    cfg->network_timeout_ms = MQTT_NETWORK_TIMEOUT_MS;
    cfg->user_context = config->user_context;
    cfg->event_handle = config->event_handle;
    cfg->pub_handle = config->pub_handle;
    cfg->auto_reconnect = true;
    if (config->disable_auto_reconnect) {
        cfg->auto_reconnect = false;
    }

    return err;
_mqtt_set_config_failed:
    trs_mqtt_destroy_config(client);
    return err;
}

static int trs_mqtt_destroy_config(trs_mqtt_client_handle_t client)
{
    mqtt_config_storage_t *cfg = client->config;
    mqtt_free(cfg->host);
    mqtt_free(cfg->uri);
    mqtt_free(cfg->path);
    mqtt_free(cfg->scheme);
    mqtt_free(client->connect_info.will_topic);
    mqtt_free(client->connect_info.will_message);
    mqtt_free(client->connect_info.client_id);
    mqtt_free(client->connect_info.username);
    mqtt_free(client->connect_info.password);
    mqtt_free(client->config);
    return 0;
}

static int trs_mqtt_get_packet_len(trs_mqtt_client_handle_t client, int* value)
{
	unsigned char c;
	int multiplier = 1;
	int len = 0;
	*value = 0;
	int rc;

	do
	{		
		if (++len > MAX_REMAINING_LENGTH_BYTES)
		{
			os_printf(LM_APP, LL_ERR, "The packet length of mqtt exceeds the limit\r\n");
			return -1;
		}

		rc = transport_read(client->transport, (char *)client->mqtt_state.in_buffer + len, 1, client->config->network_timeout_ms);  
		if (rc != 1)
		{
			os_printf(LM_APP, LL_ERR, "Timeout get packet length from mqtt peer\r\n");
			return -1;
		}

		c = client->mqtt_state.in_buffer[len];
		*value += (c & 127) * multiplier;
		multiplier *= 128;
	} while ((c & 128) != 0);

	return len;
}


static int trs_mqtt_read_msg(trs_mqtt_client_handle_t client)
{
   
	
	int len = 0;
	int rem_len = 0;
	int rc = -1;
	int read_len = 0;

	//get packet type and Qos data
	if (client->state == MQTT_STATE_INIT)
	{
	    rc = transport_read(client->transport, (char *)client->mqtt_state.in_buffer, 1, client->config->network_timeout_ms);
	}
	else if (client->state == MQTT_STATE_CONNECTED)
	{
	    rc = transport_read(client->transport, (char *)client->mqtt_state.in_buffer, 1, 10);
	}
	
	if (rc == 0)
	{
		return rc;
	}

	if (rc != 1)
	{ 
		os_printf(LM_APP, LL_ERR, "read packet type from mqtt peer failed rc=%d\r\n", rc);
		return -1;
	}
	len = trs_mqtt_get_packet_len(client, &rem_len);
	if (len < 0)
		return -1;

	if (rem_len > 0)
	{		
		os_printf(LM_APP, LL_DBG, "get packet length(%d[%d]) from mqtt peer\r\n", rem_len, len);
		if(rem_len > (client->mqtt_state.in_buffer_length - 1 - len))  
		{
			rem_len = client->mqtt_state.in_buffer_length - 1 - len;
		}

		do
		{
			rc = transport_read(client->transport, (char *)client->mqtt_state.in_buffer + 1 + len + read_len, rem_len - read_len, client->config->network_timeout_ms);
			if (rc < 0)
			{
				os_printf(LM_APP, LL_ERR, "read packet data from mqtt peer failed rc=%d\r\n", rc);
				return -1;
			}
			read_len += rc;
			os_printf(LM_APP, LL_DBG, "mqtt packet data read len %d\r\n",	rc);
		} while (read_len != rem_len);
	}
	
	return rem_len + len + 1;
}
static int trs_mqtt_connect(trs_mqtt_client_handle_t client, int timeout_ms)
{
    int write_len, read_len, connect_rsp_code;
    client->wait_for_ping_resp = false;
    mqtt_msg_init(&client->mqtt_state.mqtt_connection,
                  client->mqtt_state.out_buffer,
                  client->mqtt_state.out_buffer_length);
    client->mqtt_state.outbound_message = mqtt_msg_connect(&client->mqtt_state.mqtt_connection,
                                          client->mqtt_state.connect_info);
    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data,
                                        client->mqtt_state.outbound_message->length);
    os_printf(LM_APP, LL_DBG, "MQTT:Sending MQTT CONNECT message, type: %d, id: %04X\n",
             client->mqtt_state.pending_msg_type,
             client->mqtt_state.pending_msg_id);
    //int i = 0;
    write_len = transport_write(client->transport,
                                (char *)client->mqtt_state.outbound_message->data,
                                client->mqtt_state.outbound_message->length,
                                client->config->network_timeout_ms);
    if (write_len < 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Writing failed, errno= %d\n", errno);
        return -1;
    }
	
	read_len = trs_mqtt_read_msg(client);
	
    if (read_len < 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Error network response\n");
        return -1;
    }

    if (mqtt_get_type(client->mqtt_state.in_buffer) != MQTT_MSG_TYPE_CONNACK) {
        os_printf(LM_APP, LL_DBG, "MQTT:Invalid MSG_TYPE response: %d, read_len: %d\n", mqtt_get_type(client->mqtt_state.in_buffer), read_len);
        return -1;
    }
    connect_rsp_code = mqtt_get_connect_return_code(client->mqtt_state.in_buffer);
    switch (connect_rsp_code) {
        case CONNECTION_ACCEPTED:
            os_printf(LM_APP, LL_INFO, "MQTT:Connected\n");
			target_dce_transmit("[DA]MQTT:1\r\n", strlen("[DA]MQTT:1\r\n"));
            return 0;
        case CONNECTION_REFUSE_PROTOCOL:
            os_printf(LM_APP, LL_INFO, "MQTT:Connection refused, bad protocol\n");
            return -1;
        case CONNECTION_REFUSE_SERVER_UNAVAILABLE:
            os_printf(LM_APP, LL_INFO, "MQTT:Connection refused, server unavailable\n");
            return -1;
        case CONNECTION_REFUSE_BAD_USERNAME:
            os_printf(LM_APP, LL_INFO, "MQTT:Connection refused, bad username or password\n");
            return -1;
        case CONNECTION_REFUSE_NOT_AUTHORIZED:
            os_printf(LM_APP, LL_INFO, "MQTT:Connection refused, not authorized\n");
            return -1;
        default:
            os_printf(LM_APP, LL_INFO, "MQTT:Connection refused, Unknow reason\n");
            return -1;
    }
    return 0;
}

static int trs_mqtt_abort_connection(trs_mqtt_client_handle_t client)
{
    transport_close(client->transport);
    client->wait_timeout_ms = MQTT_RECONNECT_TIMEOUT_MS;
    client->reconnect_tick = platform_tick_get_ms();
    client->state = MQTT_STATE_WAIT_TIMEOUT;
    os_printf(LM_APP, LL_INFO, "MQTT:Reconnect after %d ms\n", client->wait_timeout_ms);
	target_dce_transmit("[DA]MQTT:0\r\n", strlen("[DA]MQTT:0\r\n"));
    client->event.event_id = MQTT_EVENT_DISCONNECTED;
    client->wait_for_ping_resp = false;
    client->heartbeat_count = 0;
    trs_mqtt_dispatch_event(client);
	skylab_mqtt_cean_status();
    return 0;
}

trs_mqtt_client_handle_t trs_mqtt_client_init(const trs_mqtt_client_config_t *config)
{
    trs_mqtt_client_handle_t client = mqtt_calloc(1, sizeof(struct trs_mqtt_client));
    MEM_CHECK(client, return NULL);
    trs_mqtt_set_config(client, config);
    client->transport_list = transport_list_init();
    MEM_CHECK(client->transport_list, goto _mqtt_init_failed);
    transport_handle_t tcp = transport_tcp_init();
    MEM_CHECK(tcp, goto _mqtt_init_failed);
    transport_set_default_port(tcp, MQTT_TCP_DEFAULT_PORT);
    transport_list_add(client->transport_list, tcp, "mqtt");
    if (config->transport == MQTT_TRANSPORT_OVER_TCP) {
        client->config->scheme = create_string("mqtt", 4);
        MEM_CHECK(client->config->scheme, goto _mqtt_init_failed);
    }
#if MQTT_ENABLE_WS
    transport_handle_t ws = transport_ws_init(tcp);
    MEM_CHECK(ws, goto _mqtt_init_failed);
    transport_set_default_port(ws, MQTT_WS_DEFAULT_PORT);
    transport_list_add(client->transport_list, ws, "ws");
    if (config->transport == MQTT_TRANSPORT_OVER_WS) {
        client->config->scheme = create_string("ws", 2);
        MEM_CHECK(client->config->scheme, goto _mqtt_init_failed);
    }
#endif

#if defined(CONFIG_MQTT_SSL)
    transport_handle_t ssl = transport_ssl_init();
    MEM_CHECK(ssl, goto _mqtt_init_failed);
    transport_set_default_port(ssl, MQTT_SSL_DEFAULT_PORT);
    if (config->cert_pem) {
        transport_ssl_set_cert_data(ssl, config->cert_pem, strlen(config->cert_pem));
    }
    if (config->client_cert_pem) {
        transport_ssl_set_client_cert_data(ssl, config->client_cert_pem, strlen(config->client_cert_pem));
    }
    if (config->client_key_pem) {
        transport_ssl_set_client_key_data(ssl, config->client_key_pem, strlen(config->client_key_pem));
    }
    transport_list_add(client->transport_list, ssl, "mqtts");
    if (config->transport == MQTT_TRANSPORT_OVER_SSL) {
        client->config->scheme = create_string("mqtts", 5);
        MEM_CHECK(client->config->scheme, goto _mqtt_init_failed);
    }
#endif

#if MQTT_ENABLE_WSS
    transport_handle_t wss = transport_ws_init(ssl);
    MEM_CHECK(wss, goto _mqtt_init_failed);
    transport_set_default_port(wss, MQTT_WSS_DEFAULT_PORT);
    transport_list_add(client->transport_list, wss, "wss");
    if (config->transport == MQTT_TRANSPORT_OVER_WSS) {
        client->config->scheme = create_string("wss", 3);
        MEM_CHECK(client->config->scheme, goto _mqtt_init_failed);
    }
#endif
    if (client->config->uri) {
        if (trs_mqtt_client_set_uri(client, client->config->uri) != 0) {
            goto _mqtt_init_failed;
        }
    }

    if (client->config->scheme == NULL) {
        client->config->scheme = create_string("mqtt", 4);
        MEM_CHECK(client->config->scheme, goto _mqtt_init_failed);
    }
    client->keepalive_tick = platform_tick_get_ms();
    client->reconnect_tick = platform_tick_get_ms();
    client->wait_for_ping_resp = false;
    int buffer_size = config->buffer_size;
    if (buffer_size <= 0) {
        buffer_size = MQTT_BUFFER_SIZE_BYTE;
    }
    client->mqtt_state.in_buffer = (uint8_t *)mqtt_malloc(buffer_size);
    MEM_CHECK(client->mqtt_state.in_buffer, goto _mqtt_init_failed);
    client->mqtt_state.in_buffer_length = buffer_size;
    client->mqtt_state.out_buffer = (uint8_t *)mqtt_malloc(buffer_size);
    MEM_CHECK(client->mqtt_state.out_buffer, goto _mqtt_init_failed);

    client->mqtt_state.out_buffer_length = buffer_size;
    client->mqtt_state.connect_info = &client->connect_info;
    client->outbox = outbox_init();
    MEM_CHECK(client->outbox, goto _mqtt_init_failed);
    return client;
_mqtt_init_failed:
    trs_mqtt_client_destroy(client);
    return NULL;
}

int trs_mqtt_client_destroy(trs_mqtt_client_handle_t client)
{
    trs_mqtt_destroy_config(client);
    if(client->transport_list){ transport_list_destroy(client->transport_list); }
    if(client->cfg_queue_handle){ os_queue_destory(client->cfg_queue_handle); }
    if(client->outbox) { outbox_destroy(client->outbox); }
    if(client->mqtt_state.in_buffer) { mqtt_free(client->mqtt_state.in_buffer); }
    if(client->mqtt_state.out_buffer) { mqtt_free(client->mqtt_state.out_buffer); }
    if(client) { mqtt_free(client); }
    return 0;
}

static char *create_string(const char *ptr, int len)
{
    char *ret;
    if (len <= 0) {
        return NULL;
    }
    ret = mqtt_calloc(1, len + 1);
    MEM_CHECK(ret, return NULL);
    memcpy(ret, ptr, len);
    return ret;
}

int trs_mqtt_client_set_uri(trs_mqtt_client_handle_t client, const char *uri)
{
    struct http_parser_url puri;
    memset(&puri, 0, sizeof(struct http_parser_url));
    int parser_status = http_parser_parse_url(uri, strlen(uri), 0, &puri);
    if (parser_status != 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Error parse uri = %s\n", uri);
        return -1;
    }

    //os_printf(LM_APP, LL_ERR, "MQTT:uri = %s\n", uri);
    //os_printf(LM_APP, LL_ERR, "MQTT:client->config->scheme = %s\n", client->config->scheme);
    if (client->config->scheme == NULL) {
        client->config->scheme = create_string(uri + puri.field_data[UF_SCHEMA].off, puri.field_data[UF_SCHEMA].len);
    }

    if (client->config->host == NULL) {
        client->config->host = create_string(uri + puri.field_data[UF_HOST].off, puri.field_data[UF_HOST].len);
    }
    //os_printf(LM_APP, LL_ERR, "MQTT:client->config->host = %s\n", client->config->host);
    if (client->config->path == NULL) {
        client->config->path = create_string(uri + puri.field_data[UF_PATH].off, puri.field_data[UF_PATH].len);
    }
    if (client->config->path) {
        transport_handle_t trans = transport_list_get_transport(client->transport_list, "ws");
        if (trans) {
            transport_ws_set_path(trans, client->config->path);
        }
        trans = transport_list_get_transport(client->transport_list, "wss");
        if (trans) {
            transport_ws_set_path(trans, client->config->path);
        }
    }

    if (puri.field_data[UF_PORT].len) {
        client->config->port = strtol((const char*)(uri + puri.field_data[UF_PORT].off), NULL, 10);
    }

    char *user_info = create_string(uri + puri.field_data[UF_USERINFO].off, puri.field_data[UF_USERINFO].len);
    if (user_info) {
        char *pass = strchr(user_info, ':');
        if (pass) {
            pass[0] = 0; //terminal username
            pass ++;
            client->connect_info.password = os_strdup(pass);
        }
        client->connect_info.username = os_strdup(user_info);

        mqtt_free(user_info);
    }

    return 0;
}

static int mqtt_write_data(trs_mqtt_client_handle_t client)
{
    int write_len = transport_write(client->transport,
                                    (char *)client->mqtt_state.outbound_message->data,
                                    client->mqtt_state.outbound_message->length,
                                    client->config->network_timeout_ms);
    // client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    if (write_len <= 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Error write data or timeout, written len = %d\n", write_len);
        return -1;
    }
    /* we've just sent a mqtt control packet, update keepalive counter
     * [MQTT-3.1.2-23]
     */
    client->keepalive_tick = platform_tick_get_ms();
    return 0;
}

static int trs_mqtt_dispatch_event(trs_mqtt_client_handle_t client)
{
    client->event.msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);
    client->event.user_context = client->config->user_context;
    client->event.client = client;

    if (client->config->event_handle) {
        return client->config->event_handle(&client->event);
    }
    return -1;
}



static void deliver_publish(trs_mqtt_client_handle_t client, uint8_t *message, int length)
{
    const char *mqtt_topic, *mqtt_data;
    uint32_t mqtt_topic_length, mqtt_data_length;
    uint32_t mqtt_len, mqtt_offset = 0, total_mqtt_len = 0;
	char topic_buff[128] = {0};
    int len_read;
    //transport_handle_t transport = client->transport;

    do
    {
        if (total_mqtt_len == 0) {
            mqtt_topic_length = length;   
            mqtt_topic = mqtt_get_publish_topic(message, &mqtt_topic_length); 
           
			if(mqtt_topic_length > 128)
			{
				os_printf(LM_APP, LL_ERR, "Topic is too long\r\n");
				return;
			}
			memcpy(topic_buff, mqtt_topic, mqtt_topic_length);

            mqtt_data_length = length;
            mqtt_data = mqtt_get_publish_data(message, &mqtt_data_length);
            total_mqtt_len = client->mqtt_state.message_length - client->mqtt_state.message_length_read + mqtt_data_length;
            mqtt_len = mqtt_data_length;
        } else {
            mqtt_len = len_read;
            mqtt_data = (const char*)client->mqtt_state.in_buffer;
        }

        os_printf(LM_APP, LL_DBG, "MQTT:Get data len= %d, topic len=%d\n", mqtt_len, mqtt_topic_length);
        client->event.event_id = MQTT_EVENT_DATA;
        client->event.data = (char *)mqtt_data;
        client->event.data_len = mqtt_len;
        client->event.total_data_len = total_mqtt_len;
        client->event.current_data_offset = mqtt_offset;
        client->event.topic = (char *)topic_buff;
        client->event.topic_len = mqtt_topic_length;
        trs_mqtt_dispatch_event(client);

        mqtt_offset += mqtt_len;
        if (client->mqtt_state.message_length_read >= client->mqtt_state.message_length) {
            break;
        }

        len_read = transport_read(client->transport,
                                  (char *)client->mqtt_state.in_buffer,
                                  client->mqtt_state.message_length - client->mqtt_state.message_length_read > client->mqtt_state.in_buffer_length ?
                                  client->mqtt_state.in_buffer_length : client->mqtt_state.message_length - client->mqtt_state.message_length_read,
                                  client->config->network_timeout_ms);
        if (len_read <= 0) {
            os_printf(LM_APP, LL_ERR, "MQTT:Read error or timeout: %d\n", errno);
            break;
        }
        client->mqtt_state.message_length_read += len_read;
    } while (1);


}

static bool is_valid_mqtt_msg(trs_mqtt_client_handle_t client, int msg_type, int msg_id)
{
    os_printf(LM_APP, LL_DBG, "MQTT:pending_id=%d, pending_msg_count = %d\n", client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_count);
    if (client->mqtt_state.pending_msg_count == 0) {
        return false;
    }
    if (outbox_delete(client->outbox, msg_id, msg_type) == 0) {
        client->mqtt_state.pending_msg_count --;
        return true;
    }
    if (client->mqtt_state.pending_msg_type == msg_type && client->mqtt_state.pending_msg_id == msg_id) {
        client->mqtt_state.pending_msg_count --;
        return true;
    }

    return false;
}

static void mqtt_enqueue(trs_mqtt_client_handle_t client)
{
    // os_printf(LM_APP, LL_ERR, "MQTT:mqtt_enqueue id: %d, type=%d successful\n", client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_type);
    //lock mutex
    if (client->mqtt_state.pending_msg_count > 0) {
        //Copy to queue buffer
        outbox_enqueue(client->outbox,
                       client->mqtt_state.outbound_message->data,
                       client->mqtt_state.outbound_message->length,
                       client->mqtt_state.pending_msg_id,
                       client->mqtt_state.pending_msg_type,
                       platform_tick_get_ms());
    }
    //unlock
}


static int mqtt_process_receive(trs_mqtt_client_handle_t client)
{
    int read_len;
    uint8_t msg_type;
    uint8_t msg_qos;
    uint16_t msg_id;

    read_len = trs_mqtt_read_msg(client);    

    if (read_len < 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Read error or end of stream\n");
        return -1;
    }

    if (read_len == 0) {
        return 0;
    }

    msg_type = mqtt_get_type(client->mqtt_state.in_buffer);
    msg_qos = mqtt_get_qos(client->mqtt_state.in_buffer);
    msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);

    os_printf(LM_APP, LL_ERR, "MQTT:msg_type=%d, msg_id=%d\n", msg_type, msg_id);
    switch (msg_type)
    {
        case MQTT_MSG_TYPE_SUBACK:
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_SUBSCRIBE, msg_id)) {
                os_printf(LM_APP, LL_INFO, "MQTT:Subscribe successful\n");
                client->event.event_id = MQTT_EVENT_SUBSCRIBED;
                trs_mqtt_dispatch_event(client);
            }
            break;
        case MQTT_MSG_TYPE_UNSUBACK:
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_UNSUBSCRIBE, msg_id)) {
                os_printf(LM_APP, LL_INFO, "MQTT:UnSubscribe successful\n");
                client->event.event_id = MQTT_EVENT_UNSUBSCRIBED;
                trs_mqtt_dispatch_event(client);
            }
            break;
        case MQTT_MSG_TYPE_PUBLISH:
            if (msg_qos == 1) {
                client->mqtt_state.outbound_message = mqtt_msg_puback(&client->mqtt_state.mqtt_connection, msg_id);
            }
            else if (msg_qos == 2) {
                client->mqtt_state.outbound_message = mqtt_msg_pubrec(&client->mqtt_state.mqtt_connection, msg_id);
            }

            if (msg_qos == 1 || msg_qos == 2) {
                os_printf(LM_APP, LL_INFO, "MQTT:Queue response QoS: %d\n", msg_qos);
                if (mqtt_write_data(client) != 0) {
                    os_printf(LM_APP, LL_ERR, "MQTT:Error write qos msg repsonse, qos = %d\n", msg_qos);
                    // TODO: Shoule reconnect?
                    // return -1;
                }
            }
            client->mqtt_state.message_length_read = read_len;
            client->mqtt_state.message_length = mqtt_get_total_length(client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
            os_printf(LM_APP, LL_DBG, "MQTT:deliver_publish, message_length_read=%d, message_length=%d\n", read_len, client->mqtt_state.message_length);

            deliver_publish(client, client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);
            break;
        case MQTT_MSG_TYPE_PUBACK:
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_PUBLISH, msg_id)) {
                os_printf(LM_APP, LL_INFO, "MQTT:received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish\n");
                client->event.event_id = MQTT_EVENT_PUBLISHED;
                trs_mqtt_dispatch_event(client);
            }

            break;
        case MQTT_MSG_TYPE_PUBREC:
            os_printf(LM_APP, LL_INFO, "MQTT:received MQTT_MSG_TYPE_PUBREC\n");
            client->mqtt_state.outbound_message = mqtt_msg_pubrel(&client->mqtt_state.mqtt_connection, msg_id);
            mqtt_write_data(client);
            break;
        case MQTT_MSG_TYPE_PUBREL:
            os_printf(LM_APP, LL_INFO, "MQTT:received MQTT_MSG_TYPE_PUBREL\n");
            client->mqtt_state.outbound_message = mqtt_msg_pubcomp(&client->mqtt_state.mqtt_connection, msg_id);
            mqtt_write_data(client);

            break;
        case MQTT_MSG_TYPE_PUBCOMP:
            os_printf(LM_APP, LL_INFO, "MQTT:received MQTT_MSG_TYPE_PUBCOMP\n");
            if (is_valid_mqtt_msg(client, MQTT_MSG_TYPE_PUBLISH, msg_id)) {
                os_printf(LM_APP, LL_INFO, "MQTT:Receive MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish\n");
                client->event.event_id = MQTT_EVENT_PUBLISHED;
                trs_mqtt_dispatch_event(client);
            }
            break;
        case MQTT_MSG_TYPE_PINGRESP:
            os_printf(LM_APP, LL_DBG, "MQTT:PINGRESP %d\n", ping_resp++);
            client->wait_for_ping_resp = false;
            break;
    }

    return 0;
}

static void trs_mqtt_task(void *pv)
{
    trs_mqtt_client_handle_t client = (trs_mqtt_client_handle_t) pv;
    client->run = true;
					
    //get transport by scheme
    client->transport = transport_list_get_transport(client->transport_list, client->config->scheme);
    if (client->transport == NULL) {
        os_printf(LM_APP, LL_ERR, "MQTT:There are no transports valid, stop mqtt client, config scheme = %s\n", client->config->scheme);
    }
    //default port
    if (client->config->port == 0) {
        client->config->port = transport_get_default_port(client->transport);
    }
    client->state = MQTT_STATE_INIT;
    MQTT_CFG_MSG_ST cfg_msg = {0};
    client->heartbeat_count = 0;
    while (client->run) {
        if (os_queue_receive(client->cfg_queue_handle, (char *)&cfg_msg, sizeof(cfg_msg), MQTT_REV_MSG_TIMEOUT) == 0)
        {
            os_printf(LM_APP, LL_DBG, "MQTT:mqtt receives msg, type:%d;  topic:%p; topic_len:%d; data:%p; data_len: %d; qos:%d; retain:%d \r\n", 
                cfg_msg.cfg_type,
                cfg_msg.topic,
                cfg_msg.topic_len,
                cfg_msg.data,
                cfg_msg.data_len,
                cfg_msg.qos,
                cfg_msg.retain);
            
            if (cfg_msg.cfg_type == CFG_MSG_TYPE_PUB)
            {
                trs_mqtt_client_publish(client, cfg_msg.topic, cfg_msg.data, cfg_msg.data_len, cfg_msg.qos, cfg_msg.retain);                
            }
            else if (cfg_msg.cfg_type == CFG_MSG_TYPE_PUB_RAW)
            {
                MQTT_PUB_STATUS_E pub_status = MQTT_PUB_SUCCESS;
                if (trs_mqtt_client_publish(client, cfg_msg.topic, cfg_msg.data, cfg_msg.data_len, cfg_msg.qos, cfg_msg.retain) < 0)
                {
                    pub_status = MQTT_PUB_FAILED;
                }
                
                if (client->config->pub_handle != NULL)
                {
                    client->config->pub_handle(pub_status);
                }
            }
            else if (cfg_msg.cfg_type == CFG_MSG_TYPE_SUB)
            {
                trs_mqtt_client_subscribe(client, cfg_msg.topic, cfg_msg.qos);
            }
            else if (cfg_msg.cfg_type == CFG_MSG_TYPE_UNSUB)
            {
                trs_mqtt_client_unsubscribe(client, cfg_msg.topic);
            }
            else if (cfg_msg.cfg_type == CFG_MSG_TYPE_STOP)
            {
                trs_mqtt_client_stop(client);
            }
            else
            {
                os_printf(LM_APP, LL_ERR, "MQTT:mqtt receives cfg_msg.cfg_type err(%d).\r\n", cfg_msg.cfg_type);
            }
            
            if (cfg_msg.topic != NULL)
            {
                mqtt_free((void *)cfg_msg.topic);
            }

            if (cfg_msg.data != NULL)
            {
                mqtt_free((void *)cfg_msg.data);
            }
        }
        
        switch ((int)client->state) {
            case MQTT_STATE_INIT:
                if (client->transport == NULL) {
                    os_printf(LM_APP, LL_ERR, "MQTT:There are no transport\n");
                }

                os_printf(LM_APP, LL_INFO, "MQTT:client->config->host = %s; client->config->port = %d;\r\n", client->config->host, client->config->port);
                if (transport_connect(client->transport,
                                      client->config->host,
                                      client->config->port,
                                      client->config->network_timeout_ms) < 0) {
                    os_printf(LM_APP, LL_ERR, "MQTT:Error transport connect\n");
                    trs_mqtt_abort_connection(client);
                    break;
                }
                os_printf(LM_APP, LL_INFO, "MQTT:Transport connected to %s://%s:%d\n", client->config->scheme, client->config->host, client->config->port);
                if (trs_mqtt_connect(client, client->config->network_timeout_ms) != 0) {
                    os_printf(LM_APP, LL_ERR, "MQTT:Error MQTT Connected\n");
                    trs_mqtt_abort_connection(client);
                    break;
                }
                client->event.event_id = MQTT_EVENT_CONNECTED;
                client->state = MQTT_STATE_CONNECTED;
                trs_mqtt_dispatch_event(client);

                break;
            case MQTT_STATE_CONNECTED:
                // receive and process data
                if (mqtt_process_receive(client) == -1) {
                    trs_mqtt_abort_connection(client);
                    break;
                }
                if ((platform_tick_get_ms() - client->keepalive_tick) > (client->connect_info.keepalive * 1000 / MQTT_HEART_BEAT_MAX)) {
                	if(client->wait_for_ping_resp){ //No ping resp
                    	client->wait_for_ping_resp = false;
                        client->heartbeat_count++;
                        os_printf(LM_APP, LL_ERR, "MQTT:No PING_RESP, client->heartbeat_count = %d\r\n", client->heartbeat_count);
                        if (client->heartbeat_count >= MQTT_HEART_BEAT_MAX)
                        {
                            os_printf(LM_APP, LL_ERR, "MQTT:No PING_RESP, heartbeat_count exceeded max(%d), disconnected\r\n", MQTT_HEART_BEAT_MAX);
                            trs_mqtt_abort_connection(client);
                            break;
                        }
                    }
                    else{ //has received ping-resp
                        client->heartbeat_count = 0;
                    }
                    
                	if (trs_mqtt_client_ping(client) == -1) {
                        os_printf(LM_APP, LL_ERR, "MQTT:Can't send ping, disconnected\n");
                        trs_mqtt_abort_connection(client);
                        break;
                    } else {
                    	client->wait_for_ping_resp = true;
                    }
                	os_printf(LM_APP, LL_DBG, "MQTT:PING %d\n", ping_num++);
                }

                //Delete mesaage after 30 senconds
                outbox_delete_expired(client->outbox, platform_tick_get_ms(), OUTBOX_EXPIRED_TIMEOUT_MS);
                //
                outbox_cleanup(client->outbox, OUTBOX_MAX_SIZE);
                break;
            case MQTT_STATE_WAIT_TIMEOUT:

                if (!client->config->auto_reconnect) {
                    break;
                }
                if (platform_tick_get_ms() - client->reconnect_tick > client->wait_timeout_ms) {
                    client->state = MQTT_STATE_INIT;
                    client->reconnect_tick = platform_tick_get_ms();
                    os_printf(LM_APP, LL_DBG, "MQTT:Reconnecting...\n");
                }
                os_printf(LM_APP, LL_INFO, "MQTT:waiting for connnecting\n");
                vTaskDelay(client->wait_timeout_ms / 2 / portTICK_RATE_MS);
                break;
            default:
                os_printf(LM_APP, LL_ERR, "MQTT:ERR,client->state:%d\r\n", client->state);
                break;
        }
    }
    transport_close(client->transport);
    trs_mqtt_client_destroy(client);
    os_printf(LM_APP, LL_INFO, "MQTT:delete mqtt\n");
    vTaskDelete(NULL);
}

int trs_mqtt_client_start(trs_mqtt_client_handle_t client)
{
    if (client->state >= MQTT_STATE_INIT) 
    {
        os_printf(LM_APP, LL_INFO, "MQTT:Client has started\r\n");
        return -1;
    }
    
    if (client->cfg_queue_handle == NULL)
    {
        client->cfg_queue_handle = os_queue_create(NULL, MQTT_QUEUE_LENGTH, sizeof(MQTT_CFG_MSG_ST), 0);
    }
    else
    {   os_printf(LM_APP, LL_INFO, "MQTT:client->cfg_queue_handle:%p\r\n", client->cfg_queue_handle);
        return -1;
    }

    if (xTaskCreate(trs_mqtt_task, "mqtt_task", client->config->task_stack, client, client->config->task_prio, NULL) != pdTRUE) 
    {
        os_printf(LM_APP, LL_ERR, "MQTT:Error create mqtt task\n");
        return -1;
    }
    return 0;
}

int trs_mqtt_client_stop(trs_mqtt_client_handle_t client)
{
    client->run = false;
    os_printf(LM_APP, LL_INFO, "MQTT:current state is %d\n",client->state);
    client->state = MQTT_STATE_UNKNOWN;
    client->mqtt_state.outbound_message = mqtt_msg_disconnect(&client->mqtt_state.mqtt_connection);
    if (mqtt_write_data(client) != 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Error send stop message\n");
        return -1;
    }
    return 0;
}

static int trs_mqtt_client_ping(trs_mqtt_client_handle_t client)
{
    client->mqtt_state.outbound_message = mqtt_msg_pingreq(&client->mqtt_state.mqtt_connection);

    if (mqtt_write_data(client) != 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Error sending PING\n");
        return -1;
    }
    //os_printf(LM_APP, LL_ERR, "MQTT:PING send\n");
    return 0;
}

int trs_mqtt_client_subscribe(trs_mqtt_client_handle_t client, const char *topic, int qos)
{
    if (client->state != MQTT_STATE_CONNECTED) {
        os_printf(LM_APP, LL_ERR, "MQTT:Client has not connected\n");
        return -1;
    }
    mqtt_enqueue(client); //move pending msg to outbox (if have)
    client->mqtt_state.outbound_message = mqtt_msg_subscribe(&client->mqtt_state.mqtt_connection,
                                          topic, qos,
                                          &client->mqtt_state.pending_msg_id);

    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_count ++;

    if (mqtt_write_data(client) != 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Error to subscribe topic=%s, qos=%d\n", topic, qos);
        return -1;
    }

    os_printf(LM_APP, LL_DBG, "MQTT:Sent subscribe topic=%s", topic);
    os_printf(LM_APP, LL_DBG, ", id: %d, type: %d successful\n", client->mqtt_state.pending_msg_id, client->mqtt_state.pending_msg_type);
    return client->mqtt_state.pending_msg_id;
}

int trs_mqtt_client_unsubscribe(trs_mqtt_client_handle_t client, const char *topic)
{
    if (client->state != MQTT_STATE_CONNECTED) {
        os_printf(LM_APP, LL_ERR, "MQTT:Client has not connected\n");
        return -1;
    }
    mqtt_enqueue(client);
    client->mqtt_state.outbound_message = mqtt_msg_unsubscribe(&client->mqtt_state.mqtt_connection,
                                          topic,
                                          &client->mqtt_state.pending_msg_id);
    os_printf(LM_APP, LL_DBG, "MQTT:unsubscribe, topic\"%s\", id: %d\n", topic, client->mqtt_state.pending_msg_id);

    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
    client->mqtt_state.pending_msg_count ++;

    if (mqtt_write_data(client) != 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Error to unsubscribe topic=%s\n", topic);
        return -1;
    }

    os_printf(LM_APP, LL_INFO, "MQTT:Sent Unsubscribe topic=%s, id: %d, successful\n", topic, client->mqtt_state.pending_msg_id);
    return client->mqtt_state.pending_msg_id;
}

int trs_mqtt_client_publish(trs_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain)
{
    uint16_t pending_msg_id = 0;
    if (client->state != MQTT_STATE_CONNECTED) {
        os_printf(LM_APP, LL_ERR, "MQTT:Client has not connected\n");
        return -1;
    }
    
    if (len < 0)
    {
        os_printf(LM_APP, LL_ERR, "trs_mqtt_client_publish() para err!\n");
        return -1;
    }
    else if (len == 0) {
        if (data != NULL)
        {
            len = strlen(data);
        }
    }
    else
    {;}
    
    client->mqtt_state.outbound_message = mqtt_msg_publish(&client->mqtt_state.mqtt_connection,
                                          topic, data, len,
                                          qos, retain,
                                          &pending_msg_id);
    if (qos > 0) {
        client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);
        client->mqtt_state.pending_msg_id = pending_msg_id;
        client->mqtt_state.pending_msg_count++;
        mqtt_enqueue(client);
    }

    if (mqtt_write_data(client) != 0) {
        os_printf(LM_APP, LL_ERR, "MQTT:Error to public data to topic=%s, qos=%d\n", topic, qos);
        return -1;
    }
    return pending_msg_id;
}

int trs_mqtt_client_get_states(trs_mqtt_client_handle_t client)
{
    if(client){
        return (int)client->state;
    } else {
        return -1;
    }
}

char* trs_mqtt_client_get_url(trs_mqtt_client_handle_t client)
{
    if(client){
        return (char* )client->config->uri;
    } else {
        return NULL;
    }
}

int set_mqtt_msg(SET_TYPE_EN type, MQTT_CFG_MSG_ST *msg, const char *data, int data_len)
{
    if ((msg == NULL) || (data == NULL))
    {
        system_printf("In set_mqtt_msg(), para err\r\n");
        return -1;
    }
    
    void * p_tmp = mqtt_malloc(data_len + 1);
    MEM_CHECK(p_tmp, return -1);
    memset(p_tmp, 0 , data_len + 1);
    memcpy(p_tmp, data, data_len);
    if (type == SET_TOPIC)
    {
        msg->topic = p_tmp;
        msg->topic_len = data_len;
    }
    else if (type == SET_DATA)
    {
        msg->data = p_tmp;
        msg->data_len = data_len;
    }
    else
    {
        system_printf("In set_mqtt_msg(), set_type err(%d)\n", type);
    }
    
    return 0;
}


