/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/include/ecr_qcloud_softap.h
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  WentaoLi <liwentao@eswin.com>
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

#ifndef __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_SOFTAP_H
#define __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_SOFTAP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "lwip/sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WIFI_CONFIG_PROTO_VERSION   "2.0"
#define SOFTAP_TASK_NAME            "softAP_mqtt_task"
#define SOFTAP_TASK_STACK_BYTES     4096
#define SOFTAP_TASK_PRIO            2
#define WAIT_APP_BIND_TRUE          0x55
#define WAIT_APP_BIND_FALSE         0xFF
#define WAIT_BIND                   "wait_bind"
#define WIFI_CONFIG_WAIT_APP_BIND_STATE 1

/****************************************************************************
 * Types
 ****************************************************************************/

typedef enum wifi_config_last_app_bind_state {
    LAST_APP_BIND_STATE_SUCCESS, /* last time app bind state is binded */
    LAST_APP_BIND_STATE_FAILED,  /* last time app bind state is not binded */
    LAST_APP_BIND_STATE_NOBIND,  /* last time app bind state is no app bind device */
    LAST_APP_BIND_STATE_ERROR    /* last time app bind state is func error */
} WIFI_CONFIG_LAST_APP_BIND_STATE;


/* unified socket type for TCP/UDP */
typedef struct {
    int              socket_id;
    struct sockaddr *socket_addr;
    socklen_t        addr_len;
} comm_peer_t1;

typedef enum {
    EVENT_WAIT_TIMEOUT      = 0, /* Nothing happen but just waiting timeout */
    EVENT_WIFI_CONNECTED    = 1, /* Station has connected to target AP */
    EVENT_WIFI_DISCONNECTED = 2, /* Station has been disconnected from target AP */
    EVENT_SMARTCONFIG_STOP  = 3, /* Smartconfig stop event before connected */
} eWiFiConfigEvent1;

typedef enum {
    ESWIN_WIFI_CONFIG_SUCCESS  = 0, /* WiFi config and MQTT connect success */
    ESWIN_WIFI_CONFIG_GOING_ON = 1, /* WiFi config and MQTT connect is going on */
    ESWIN_WIFI_CONFIG_FAILED   = 2, /* WiFi config and MQTT connect failed */
} eWiFiConfigState1;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name:  ecr_softap_mode_config
 * Description:
 * @brief       device start softAP mode configure, initialize wifi configure, set ip configure
 * @param[in]   ssid: the name of device softAP
 * @param[in]   psw: password for connecting the device access point
 * @param[in]   ch: softAP channel
 * @return  0 when success, or err code for failure when fail to start softAP
 ****************************************************************************/

int ecr_softap_mode_config(const char *ssid, const char *psw, uint8_t ch);

/****************************************************************************
 * Name:  ecr_softap_connect_router
 * Description:
 * @brief       stop softAP mode, and start STA mode
 * @param[in]   ssid: the name of device softAP
 * @param[in]   psw: password for connecting the device access point
 * @param[in]   channel: channel
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_softap_connect_router(const char *ssid, const char *psw, uint8_t channel);

/****************************************************************************
 * Name:  ecr_query_wifi_config_state
 * Description:
 * @brief       query wifi configure status
 * @return  device status: 0 when success, or err code for failure
 ****************************************************************************/

int ecr_query_wifi_config_state(void);

/****************************************************************************
 * Name:  ecr_sta_connect_router
 * Description:
 * @brief       connect the device to router
 * @param[in]   ssid: the name of desination router
 * @param[in]   passwd: password for connecting the router
 * @param[in]   channel: channel
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_sta_connect_router(char *ssid, char *passwd, uint8_t channel);

/****************************************************************************
 * Name:  ecr_is_wifi_config_successful
 * Description:
 * @brief       qurey whether the network is connected to the router successfully
 * @return  0 when successfully access, or err code for failure
 ****************************************************************************/

int ecr_is_wifi_config_successful(void);

/****************************************************************************
 * Name:  ecr_start_softap
 * Description:
 * @brief       start softAP mode
 * @param[in]   ssid: the name of device softAP
 * @param[in]   psw: password for connecting the device access point
 * @param[in]   ch: channel
 * @return 0 when success, or err code for failure
 ****************************************************************************/

int  ecr_start_softap(const char *ssid, const char *psw, uint8_t ch);

/****************************************************************************
 * Name:  mqtt_query_app_bind_result
 * Description:
 * @brief       device query app last bind result
 * @return  wifi config last app bind state
 ****************************************************************************/

WIFI_CONFIG_LAST_APP_BIND_STATE mqtt_query_app_bind_result(void);

/****************************************************************************
 * Name:  ecr_start_softap_log
 * Description:
 * @brief       Start a softAP(fixed SSID/PSW) and UDP server to upload log to Wechat mini program
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_start_softap_log(void);

//============================ WiFi running log upload ===========================//

/****************************************************************************
 * Name:  ecr_send_dev_log_to_app
 * Description:
 * @brief        upload wifi log to Wechat mini program
 * @param[in]   peer: unified socket type for UDP
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int  ecr_send_dev_log_to_app(comm_peer_t1 *peer);

/****************************************************************************
 * Name:  ecr_get_and_post_error_log
 * Description:
 * @brief       get and post error log
 * @param[in]   peer: unified sock type for UDP
 * @return  0 when success, others: the number of current connnect error
 ****************************************************************************/

int  ecr_get_and_post_error_log(comm_peer_t1 *peer);

/****************************************************************************
 * Name:  ecr_send_error_log_to_app
 * Description:
 * @brief       send error log to Wechat mini program
 * @param[in]   peer: unified sock type
 * @param[in]   record: current connect error
 * @param[in]   record: error log type
 * @param[in]   err_sub_id: error log sub type
 * @return  0 when success, others: the number of current connect error
 ****************************************************************************/

int  ecr_send_error_log_to_app(comm_peer_t1 *peer, uint8_t record, uint16_t c, int32_t err_sub_id);

//============================ WiFi running log upload ===========================//

#endif  /* __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_SOFTAP_H */
