/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/src/ecr_qcloud_wifi_log_handler.c
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  MengCai <caimeng@eswin.com>
 *   Revision history:
 *   2021.08.25 - Initial version.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "qcloud_iot_export_log.h"
#include "qcloud_iot_import.h"
#include "ecr_qcloud_softap.h"
#include "qcloud_wifi_config_internal.h"
#include "lwip/sockets.h"
#include "cJSON.h"

#ifdef WIFI_ERR_LOG_POST

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ERR_LOG_QUEUE_SIZE 16
#define VALID_MAGIC_CODE   0xF00DBEEF

#ifdef WIFI_LOG_UPLOAD
#define LOG_QUEUE_SIZE 10
#define LOG_ITEM_SIZE  128
static void *g_dev_log_queue = NULL;    /* FreeRTOS msg queue */
#endif  /* WIFI_LOG_UPLOAD */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/************** WiFi config error msg collect and post feature ******************/
static const char *g_err_log[] = {
    "Everything OK!",               /* SUCCESS_TYPE = 0,*/
    "MQTT connect error!",          /* ERR_MQTT_CONNECT = 1, */
    "APP command error!",           /* ERR_APP_CMD = 2,*/
    "WIFI boarding stop!",          /* ERR_BD_STOP = 3,*/
    "RTOS task error!",             /* ERR_OS_TASK = 4,*/
    "RTOS queue error!",            /* ERR_OS_QUEUE = 5,*/
    "WIFI STA init error!",         /* ERR_WIFI_STA_INIT = 6,*/
    "WIFI AP init error!",          /* ERR_WIFI_AP_INIT = 7,*/
    "WIFI start error!",            /* ERR_WIFI_START = 8,*/
    "WIFI config error!",           /* ERR_WIFI_CONFIG = 9,*/
    "WIFI connect error!",          /* ERR_WIFI_CONNECT = 10,*/
    "WIFI disconnect error!",       /* ERR_WIFI_DISCONNECT = 11,*/
    "WIFI AP STA error!",           /* ERR_WIFI_AP_STA = 12,*/
    "ESP smartconfig start error!", /* ERR_SC_START = 13,*/
    "ESP smartconfig data error!",  /* ERR_SC_DATA = 14, */
    "SRV socket open error!",       /* ERR_SOCKET_OPEN = 15,*/
    "SRV socket bind error!",       /* ERR_SOCKET_BIND = 16,*/
    "SRV socket listen error!",     /* ERR_SOCKET_LISTEN = 17,*/
    "SRV socket fcntl error!",      /* ERR_SOCKET_FCNTL = 18,*/
    "SRV socket accept error!",     /* ERR_SOCKET_ACCEPT = 19,*/
    "SRV socket recv error!",       /* ERR_SOCKET_RECV = 20,*/
    "SRV socket select error!",     /* ERR_SOCKET_SELECT = 21,*/
    "SRV socket send error!",       /* ERR_SOCKET_SEND = 22,*/
    "MQTT Token send error!",       /* ERR_TOKEN_SEND = 23,*/
    "MQTT Token reply error!",      /* ERR_TOKEN_REPLY = 24,*/
    "MQTT Token recv timeout!",     /* ERR_TOKEN_RECV = 25,*/
    "Product Region fault error!",  /* ERR_REGION_FAULT = 26, */
    "Device Info error!",           /* ERR_DEVICE_INFO = 27,*/
};

typedef struct {
    uint8_t  record;
    uint8_t  reserved;
    uint16_t err_id;     /* error msg Id */
    int32_t  err_sub_id; /* error msg sub Id */
} err_log_t;

#endif  /* WIFI_ERR_LOG_POST */

static bool sg_error_happen = false; /* error happen flag */
static void * g_err_log_queue = NULL;/* FreeRTOS msg queue */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  init_error_log_queue
 * Description:
 * @brief       create error log queue
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int init_error_log_queue(void)
{
    sg_error_happen = false;

#ifdef WIFI_ERR_LOG_POST
    g_err_log_queue = xQueueCreate(ERR_LOG_QUEUE_SIZE, sizeof(err_log_t));
    if (g_err_log_queue == NULL) {
        Log_e("xQueueCreate failed");
        return ERR_OS_QUEUE;
    }

#endif  /* WIFI_ERR_LOG_POST */
    return 0;
}

/****************************************************************************
 * Name:  push_error_log
 * Description:
 * @brief       send error log queue
 * @param[in]   err_id: error log id
 * @param[in]   err_sub_id: error sub id
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int push_error_log(uint16_t err_id, int32_t err_sub_id)
{
    sg_error_happen = true;

#ifdef WIFI_ERR_LOG_POST
    if (g_err_log_queue == NULL) {
        Log_e("log queue not initialized and push_error_log called");
        return ERR_OS_QUEUE;
    }

    err_log_t err_log = {.record = CUR_ERR, .err_id = err_id, .err_sub_id = err_sub_id};
    Log_e("Boarding error happen: %s (%u, %d)", g_err_log[err_log.err_id], err_log.err_id, err_log.err_sub_id);
    /* unblocking send */
    int ret = xQueueGenericSend(g_err_log_queue, &err_log, 0, queueSEND_TO_BACK);
    if (ret != pdPASS) {
        Log_e("xQueueGenericSend failed: %d", ret);
        return ERR_OS_QUEUE;
    }
#else
    Log_e("error happen, err_id: %u err_sub_id: %d", err_id, err_sub_id);
#endif  /* WIFI_ERR_LOG_POST */
    return 0;
}

/****************************************************************************
 * Name:  ecr_send_error_log_to_app
 * Description:
 * @brief       send error log to Wechat mini program
 * @param[in]   peer: socket type
 * @param[in]   record: current error
 * @param[in]   err_id: error log type
 * @param[in]   err_sub_id: error log sub type
 * @return  0 when success, or err code for socket send failure
 ****************************************************************************/

int ecr_send_error_log_to_app(comm_peer_t1 *peer, uint8_t record, uint16_t err_id, int32_t err_sub_id)
{
#ifdef WIFI_ERR_LOG_POST
    int  ret;
    char msg_str[64]   = {0};
    char json_str[128] = {0};

    HAL_Snprintf(msg_str, sizeof(msg_str), "%s (%u, %d)", g_err_log[err_id], err_id, err_sub_id);

    cJSON *reply_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(reply_json, "cmdType", (int)CMD_DEVICE_REPLY);
    cJSON_AddStringToObject(reply_json, "deviceReply", record == CUR_ERR ? "Current_Error" : "Previous_Error");
    cJSON_AddStringToObject(reply_json, "log", msg_str);

    HAL_Snprintf(json_str, sizeof(json_str),"{\"cmdType\":\"%d\",\"deviceReply\":\"%s\",\"log\":\"%s\"}",  //此处可能存在问题，需要调试
        (int)CMD_DEVICE_REPLY,  record == CUR_ERR ? "Current_Error" : "Previous_Error", msg_str);

    /* append msg delimiter */
    strcat(json_str, "\r\n");

    ret = sendto(peer->socket_id, json_str, strlen(json_str), 0, peer->socket_addr, peer->addr_len);
    if (ret < 0) {
        Log_e("send error: %s", strerror(errno));
    } else
        Log_w("send error msg: %s", json_str);

    cJSON_Delete(reply_json);
    return ret;
#else
    return 0;
#endif  /* WIFI_ERR_LOG_POST */
}

/****************************************************************************
 * Name:  ecr_get_and_post_error_log
 * Description:
 * @brief      get and post error log
 * @param[in]   peer: socket
 * @return  number of error
 ****************************************************************************/

int ecr_get_and_post_error_log(comm_peer_t1 *peer)
{
    int err_cnt = 0;
#ifdef WIFI_ERR_LOG_POST
    err_log_t            err_msg;
    signed portBASE_TYPE rc;
    do {
        rc = xQueueReceive(g_err_log_queue, &err_msg, 0);
        if (rc == pdPASS) {
            ecr_send_error_log_to_app(peer, err_msg.record, err_msg.err_id, err_msg.err_sub_id);
            if (err_msg.record == CUR_ERR)
                err_cnt++;
        }
    } while (rc == pdPASS);
#endif  /* WIFI_ERR_LOG_POST */
    return err_cnt;
}

/************** WiFi config error msg collect and post feature ******************/

/****************************************************************************
 * Name:  init_dev_log_queue
 * Description:
 * @brief       create device log queue
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int init_dev_log_queue(void)
{
#ifdef WIFI_LOG_UPLOAD
    if (g_dev_log_queue) {
        Log_d("re-enter, reset queue");
        xQueueReset(g_dev_log_queue);
        return 0;
    }

    g_dev_log_queue = xQueueCreate(LOG_QUEUE_SIZE, LOG_ITEM_SIZE);
    if (g_dev_log_queue == NULL) {
        Log_e("xQueueCreate failed");
        return ERR_OS_QUEUE;
    }
#endif  /* WIFI_LOG_UPLOAD */
    return 0;
}

/****************************************************************************
 * Name:  delete_dev_log_queue
 * Description:
 * @brief       delete device log queue, and free the memory
 ****************************************************************************/

void delete_dev_log_queue(void)
{
#ifdef WIFI_LOG_UPLOAD
    vQueueDelete(g_dev_log_queue);
    g_dev_log_queue = NULL;
#endif  /* WIFI_LOG_UPLOAD */
}

/****************************************************************************
 * Name:  push_dev_log
 * Description:
 * @brief       send lastest log in queue
 * @param[in]   func: function name that call it
 * @param[in]   line: line number that call this function
 * @param[in]   fmt: log information
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int push_dev_log(const char *func, const int line, const char *fmt, ...)
{
#ifdef WIFI_LOG_UPLOAD
    if (g_dev_log_queue == NULL) {
        Log_e("log queue not initialized!");
        return ERR_OS_QUEUE;
    }

    char log_buf[LOG_ITEM_SIZE];
    memset(log_buf, 0, LOG_ITEM_SIZE);

    // only keep the latest LOG_QUEUE_SIZE log
    uint32_t log_cnt = (uint32_t)uxQueueMessagesWaiting(g_dev_log_queue);
    if (log_cnt == LOG_QUEUE_SIZE) {
        // pop the oldest one
        xQueueReceive(g_dev_log_queue, log_buf, 0);
        HAL_Printf("<<< POP LOG: %s", log_buf);
    }

    char *o = log_buf;
    memset(log_buf, 0, LOG_ITEM_SIZE);
    o += HAL_Snprintf(o, LOG_ITEM_SIZE, "%u|%s(%d): ", HAL_GetTimeMs(), func, line);

    va_list ap;
    va_start(ap, fmt);
    HAL_Vsnprintf(o, LOG_ITEM_SIZE - 3 - strlen(log_buf), fmt, ap);
    va_end(ap);

    strcat(log_buf, "\r\n");

    /* unblocking send */
    int ret = xQueueGenericSend(g_dev_log_queue, log_buf, 0, queueSEND_TO_BACK);
    if (ret != pdPASS) {
        Log_e("xQueueGenericSend failed: %d", ret);
        return ERR_OS_QUEUE;
    }

#endif /* WIFI_LOG_UPLOAD */
    return 0;
}

/****************************************************************************
 * Name:  ecr_send_dev_log_to_app
 * Description:
 * @brief       send device log to Wechat mini program
 * @param[in]   peer: socket
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_send_dev_log_to_app(comm_peer_t1 *peer)
{
    int ret = 0;
#ifdef WIFI_LOG_UPLOAD

    if (g_dev_log_queue == NULL) {
        Log_e("log queue not initialized!");
        return ERR_OS_QUEUE;
    }

    uint32_t log_cnt = (uint32_t)uxQueueMessagesWaiting(g_dev_log_queue);
    if (log_cnt == 0)
        return 0;

    size_t max_len  = (log_cnt * LOG_ITEM_SIZE) + 32;
    char * json_buf = HAL_Malloc(max_len);
    if (json_buf == NULL) {
        Log_e("malloc failed!");
        return -1;
    }

    memset(json_buf, 0, max_len);

    char                 log_buf[LOG_ITEM_SIZE];
    signed portBASE_TYPE rc;
    do {
        memset(log_buf, 0, LOG_ITEM_SIZE);
        rc = xQueueReceive(g_dev_log_queue, log_buf, 0);
        if (rc == pdPASS) {
            strcat(json_buf, log_buf);
        }
    } while (rc == pdPASS);

    HAL_Printf("to reply: %s\r\n", json_buf);

    int i = 0;
    for (i = 0; i < 2; i++) {
        ret = sendto(peer->socket_id, json_buf, strlen(json_buf), 0, peer->socket_addr, peer->addr_len);
        if (ret < 0) {
            Log_e("send error: %s", strerror(errno));
            break;
        }
        HAL_SleepMs(500);
    }

#endif /* WIFI_LOG_UPLOAD */
    return ret;
}
