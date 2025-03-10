/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/src/ecr_qcloud_softap.c
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  WentaoLi <liwentao@eswin.com>
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
#include <time.h>
#include <unistd.h>
#include  "system_wifi.h"
#include "freertos/event_groups.h"
#include "easyflash.h"
#include "qcloud_wifi_config.h"
#include "qcloud_iot_import.h"
#include "qcloud_iot_export.h"
#include "qcloud_wifi_config_internal.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "cJSON.h"
#include "ecr_qcloud_softap.h" //自定义

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_TOKEN_LENGTH 32

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool sg_token_received      = false;
static bool sg_mqtt_task_run       = false;
static bool sg_comm_task_run       = false;
static bool sg_wifi_config_success = false;
static bool sg_log_task_run        = false;

static int sg_mqtt_task_cnt = 0;
static int sg_comm_task_cnt = 0;
static int sg_log_task_cnt  = 0;

#if defined(CONFIG_BLE_TENCENT_ADAPTER)
static bool ble_token_received      = false;
#endif

//标志事件组
static EventGroupHandle_t sg_wifi_event_group;
static const int          CONNECTED_BIT           = BIT0;
static const int          STA_DISCONNECTED_BIT    = BIT1;

static unsigned char sg_wait_app_bind_device_state = 0x00;

static char sg_token_str[MAX_TOKEN_LENGTH + 1] = {0};

#if defined(CONFIG_BLE_TENCENT_ADAPTER)
static char ble_token_str[MAX_TOKEN_LENGTH + 1] = {0};
#endif

static char sg_region_str[MAX_SIZE_OF_REGION + 1] = {"china"};
static DeviceInfo sg_device_info = {0};  //设备信息

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct TokenHandleData {
    bool sub_ready;
    bool send_ready;
    int  reply_code;
} TokenHandleData;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#if defined(CONFIG_BLE_TENCENT_ADAPTER)
void ble_send_token_2_sg(const char *token)
{
    ble_token_received = true;
    strncpy(ble_token_str, token, MAX_TOKEN_LENGTH);
    xEventGroupSetBits(sg_wifi_event_group, CONNECTED_BIT);
}
#endif /* CONFIG_BLE_TENCENT_ADAPTER */

/****************************************************************************
 * Name:  ecr_query_wifi_config_state
 * Description:
 * @brief       query wifi configure status
 * @return  0: success, 1: going on, 2: failed
 ****************************************************************************/

int ecr_query_wifi_config_state(void)
{
    if (sg_wifi_config_success)
        return ESWIN_WIFI_CONFIG_SUCCESS;
    else if (sg_mqtt_task_run)
        return ESWIN_WIFI_CONFIG_GOING_ON;
    else
        return ESWIN_WIFI_CONFIG_FAILED;
}

/****************************************************************************
 * Name:  ecr_is_wifi_config_successful
 * Description:
 * @brief       judge device connect wifi status
 * @return  0: success, -1: error
 ****************************************************************************/

int ecr_is_wifi_config_successful(void)
{
    int wifi_status = 0;
    struct ip_info if_ip;

    memset(&if_ip, 0, sizeof(struct ip_info));
    // 原因：在AP或STA模式下WiFi配置后，获取的wifi_status可能依然为3(STA_STATUS_START),
    // 以及wifi_status为5的状况下，ip依然为空未拿到sta下的IP，此时返回值则会导致配网失败。
    // 修改：改为如下循环，可覆盖(5,0) (5,1) (3,1)等可能
    int wait_cnt = 5; //最大等待时间5秒
    int flag = 1;
    do {
        wifi_status = wifi_get_status(STATION_IF);
	HAL_SleepMs(1000);
        wifi_get_ip_info(STATION_IF, &if_ip);
        flag = ip_addr_isany_val(if_ip.ip);  // 与0值比较，为空时返回1，否则返回0
        // Log_i(">>>>>>>wifi_status is %d", wifi_status);
        // Log_i(">>>>>>>ip_addr_isany_val is %d", flag);
    } while (wifi_status != STA_STATUS_CONNECTED && flag != 0 && wait_cnt--);
    
    if ((wifi_status == STA_STATUS_CONNECTED) && !(ip_addr_isany_val(if_ip.ip))) {
        return 0;
    } else {
        return -1;
    }
}

/****************************************************************************
 * Name:  ecr_sta_connect_router
 * Description:
 * @brief       set wifi mode to STA mode, dcevice connect wifi router
 * @param[in]   ssid: wifi router ssid
 * @param[in]   passwd: router password
 * @param[in]   channel: router channel
 * @return  success 0, others failed
 ****************************************************************************/

int ecr_sta_connect_router(char *ssid, char *passwd, uint8_t channel)
{
    wifi_set_opmode(WIFI_MODE_STA);//STA模式

    //config sta_mode info
    wifi_config_u sta_config;
    memset(&sta_config, 0, sizeof(sta_config));

    memcpy(sta_config.sta.ssid, ssid, strlen(ssid));
    sta_config.sta.channel = channel;
    if (passwd && strlen((const char *)passwd) >= 5) {
        Log_i("set passwd [%s]\n", passwd);
        memcpy(sta_config.sta.password, passwd, strlen(passwd));
    }
    wifi_stop_station();// 修复WiFi station start error.
    // step 2 connect to the router
    int ret = wifi_start_station(&sta_config);
	if (SYS_OK != ret) {
        Log_e("wifi_start_station err:%d\n",ret);
        return ret;
    }
    return 0;
}

/****************************************************************************
 * Name:  ecr_softap_connect_router
 * Description:
 * @brief       stop softAP mode, entry STA mode, and save wifi configure to NV
 * @param[in]   ssid: router ssid
 * @param[in]   psw: router password
 * @param[in]   channel: router channel
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_softap_connect_router(const char *ssid, const char *psw, uint8_t channel)
{
    wifi_stop_softap();
    int ret = ecr_sta_connect_router((char *)ssid, (char *)psw, channel);
	if (SYS_OK != ret) {
        Log_e("wifi_start_station err:%d\n",ret);
        return ret;
    }

    Log_i("tecent_softap_sta is OK \n");

     /* save it to flash */
    customer_set_env_blob("StaSSID", ssid, WIFI_SSID_MAX_LEN-1);
    customer_set_env_blob("StaPW", psw, WIFI_PWD_MAX_LEN-1);

    HAL_SleepMs(5000);
    if (SYS_OK == ecr_is_wifi_config_successful()) {
        Log_i(" sg_wifi_event_group is set : CONNECTED_BIT\n");
        xEventGroupSetBits(sg_wifi_event_group, CONNECTED_BIT);
    } else {
        Log_e(" sg_wifi_event_group is set : STA_DISCONNECTED_BIT\n");
        xEventGroupClearBits(sg_wifi_event_group, CONNECTED_BIT);
        xEventGroupSetBits(sg_wifi_event_group, STA_DISCONNECTED_BIT);
    }
    return 0;
}

//============================ Qcloud app TCP/UDP functions begin ===========================//

/****************************************************************************
 * Name:  app_reply_dev_info
 * Description:
 * @brief       reply json data to app/Wechat mini program
 * @param[in]   peer: socket
 * @return  0 when success, 1: error and can not proceed, -1: error but can proceed
 ****************************************************************************/

static int app_reply_dev_info(comm_peer_t1 *peer)
{
    int ret;
    cJSON *reply_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(reply_json, "cmdType", (int)CMD_DEVICE_REPLY);
    cJSON_AddStringToObject(reply_json, "productId", sg_device_info.product_id);
    cJSON_AddStringToObject(reply_json, "deviceName", sg_device_info.device_name);
    cJSON_AddStringToObject(reply_json, "productRegion", sg_device_info.region);
    cJSON_AddStringToObject(reply_json, "protoVersion", WIFI_CONFIG_PROTO_VERSION);

    char json_str[256] = {0};
    HAL_Snprintf(json_str, sizeof(json_str),"{\"cmdType\":\"%d\",\"productId\":\"%s\",\"deviceName\":\"%s\",\"protoVersion\":\"%s\"}",
        (int)CMD_DEVICE_REPLY, sg_device_info.product_id, sg_device_info.device_name, WIFI_CONFIG_PROTO_VERSION);

    /* append msg delimiter */
    strcat(json_str, "\r\n");
    cJSON_Delete(reply_json);

    int udp_resend_cnt = 3;
udp_resend:
    ret = sendto(peer->socket_id, json_str, strlen(json_str), 0, peer->socket_addr, peer->addr_len);//与原sdk接口不同
    if (ret < 0) {
        Log_e("send error: %s\n", strerror(errno));
        PUSH_LOG("send error: %s", strerror(errno));
        push_error_log(ERR_SOCKET_SEND, errno);
        return 1;
    }
    /* UDP packet could be lost, send it again */
    /* NOT necessary for TCP */
    if (peer->socket_addr != NULL && --udp_resend_cnt) {
        HAL_SleepMs(1000);
        goto udp_resend;
    }

    Log_i("Report dev info: %s\n", json_str);
    return 0;
}

/****************************************************************************
 * Name:  app_handle_recv_data
 * Description:
 * @brief       parse received data, and select smartconfig mode or softAP mode
 * @param[in]   peer: socket
 * @param[in]   pdata: received json-formatted data
 * @param[in]   len: length of pdata
 * @return  1: success, -1: failure
 ****************************************************************************/

static int app_handle_recv_data(comm_peer_t1 *peer, char *pdata, int len)
{
    int    ret;
    cJSON *root = cJSON_Parse(pdata);
    if (!root) {
        Log_e("JSON Err: %s\n", cJSON_GetErrorPtr());
        PUSH_LOG("JSON Err: %s", cJSON_GetErrorPtr());
        ecr_send_error_log_to_app(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_JSON_FORMAT);
        return -1;
    }

    cJSON *cmd_json = cJSON_GetObjectItem(root, "cmdType");
    if (cmd_json == NULL) {
        Log_e("Invalid cmd JSON: %s\n", pdata);
        PUSH_LOG("Invalid cmd JSON: %s", pdata);
        cJSON_Delete(root);
        ecr_send_error_log_to_app(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_JSON_FORMAT);
        return -1;
    }

    cJSON *region_json = cJSON_GetObjectItem(root, "region");
    if (region_json) {
        memset(sg_region_str, 0, sizeof(sg_region_str));
        strncpy(sg_region_str, region_json->valuestring, MAX_SIZE_OF_REGION);
    }

    int cmd = cmd_json->valueint;
    switch (cmd) {
        /* Token only for smartconfig  */
        case CMD_TOKEN_ONLY: {
            cJSON *token_json = cJSON_GetObjectItem(root, "token");

            if (token_json) {
                memset(sg_token_str, 0, sizeof(sg_token_str));
                strncpy(sg_token_str, token_json->valuestring, MAX_TOKEN_LENGTH);
                sg_token_received = true;
                cJSON_Delete(root);

                app_reply_dev_info(peer);

                /* 1: Everything OK and we've finished the job */
                return 1;
            } else {
                cJSON_Delete(root);
                Log_e("invlaid token: %s", pdata);
                PUSH_LOG("invlaid token: %s", pdata);
                ecr_send_error_log_to_app(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_AP_INFO);
                return -1;
            }
        } break;

        /* SSID/PW/TOKEN for softAP */
        case CMD_SSID_PW_TOKEN: {
            cJSON *ssid_json  = cJSON_GetObjectItem(root, "ssid");
            cJSON *psw_json   = cJSON_GetObjectItem(root, "password");
            cJSON *token_json = cJSON_GetObjectItem(root, "token");

            if (ssid_json && psw_json && token_json) {
                memset(sg_token_str, 0, sizeof(sg_token_str));
                strncpy(sg_token_str, token_json->valuestring, MAX_TOKEN_LENGTH);
                sg_token_received = true;

                app_reply_dev_info(peer);   //发送返回消息至app
                // sleep a while before changing to STA mode
                HAL_SleepMs(1000);

                Log_i("STA to connect SSID:%s PASSWORD:%s\n", ssid_json->valuestring, psw_json->valuestring);
                PUSH_LOG("SSID:%s|PSW:%s|TOKEN:%s", ssid_json->valuestring, psw_json->valuestring,
                         token_json->valuestring);

                //跳转进入sta配置
                int ret = ecr_softap_connect_router((char *)ssid_json->valuestring, (char *)psw_json->valuestring, 1);
                if (ret) {
                    Log_e("wifi_sta_connect failed: %d\n", ret);
                    PUSH_LOG("wifi_sta_connect failed: %d", ret);
                    push_error_log(ERR_WIFI_AP_STA, ret);
                    cJSON_Delete(root);
                    return -1;
                }
                cJSON_Delete(root);

                /* return 1 as device alreay switch to STA mode and unable to recv cmd anymore
                 * 1: Everything OK and we've finished the job */
                return 1;
            } else {
                cJSON_Delete(root);
                Log_e("invlaid ssid/password/token!");
                PUSH_LOG("invlaid ssid/password/token!");
                ecr_send_error_log_to_app(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_AP_INFO);
                return -1;
            }
        } break;

        case CMD_LOG_QUERY:
            ret = ecr_send_dev_log_to_app(peer);
            Log_i("ecr_send_dev_log_to_app ret: %d", ret);
            return 1;

        default: {
            cJSON_Delete(root);
            Log_e("invalid cmd: %s\n", pdata);
            PUSH_LOG("invalid cmd: %s", pdata);
            ecr_send_error_log_to_app(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_JSON_FORMAT);
        } break;
    }

    return -1;
}

/****************************************************************************
 * Name:  ecr_udp_server_task
 * Description:
 * @brief       receive the message from app/Wechat mini program by udp
 * @param[in]   pvParameters:
 ****************************************************************************/

void ecr_udp_server_task(void *pvParameters)
{
    int  ret, server_socket = -1;
    char addr_str[1024] = {0};
    sg_comm_task_cnt++;

    /* stay longer than 5 minutes to handle error log */
    uint32_t server_count = WAIT_CNT_5MIN_SECONDS / SELECT_WAIT_TIME_SECONDS + 5;

    // 1. 参数配置
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(APP_SERVER_PORT);
    inet_ntoa_r(server_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    //2. 创建套接字
    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (server_socket < 0) {
        Log_e("socket failed: errno %d\n", errno);
        PUSH_LOG("socket failed: errno %d", errno);
        push_error_log(ERR_SOCKET_OPEN, errno);
        goto end_of_task;
    }

    //3. 绑定套接字, 指定绑定的IP地址和端口号
    ret = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        Log_e("bind failed: errno %d\n", errno);
        PUSH_LOG("bind failed: errno %d", errno);
        push_error_log(ERR_SOCKET_BIND, errno);
        goto end_of_task;
    }

    Log_i("UDP server socket listening...\n");
    fd_set      sets;
    comm_peer_t1 peer_client = {
        .socket_id   = server_socket,
        .socket_addr = NULL,    //与原接口不同，已修改
        .addr_len    = 0,
    };

    int select_err_cnt = 0;
    int recv_err_cnt   = 0;
    while (sg_comm_task_run && --server_count) {   //当通信任务运行时且5分钟倒计时 sg_comm_task_run && --server_count
        FD_ZERO(&sets);   //清空可读文件句柄集
        FD_SET(server_socket, &sets); //将套接字添加到 sets 类型集合中

        //超时时间设置
        struct timeval timeout;
        timeout.tv_sec  = SELECT_WAIT_TIME_SECONDS;
        timeout.tv_usec = 0;

        int ret = select(server_socket + 1, &sets, NULL, NULL, &timeout); //非阻塞模式， 3s超时时间
        if (ret > 0) {
            select_err_cnt = 0;
            struct sockaddr_in source_addr;
            uint               addrLen        = sizeof(source_addr);
            char               rx_buffer[256] = {0};

            int len = recvfrom(server_socket, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT,
                               (struct sockaddr *)&source_addr, (socklen_t *)addrLen);

            // Error occured during receiving
            if (len < 0) {
                recv_err_cnt++;
                Log_w("recvfrom error: %d, cnt: %d\n", errno, recv_err_cnt);
                if (recv_err_cnt > 3) {
                    Log_e("recvfrom error: %d, cnt: %d\n", errno, recv_err_cnt);
                    PUSH_LOG("recvfrom error: %d, cnt: %d", errno, recv_err_cnt);
                    push_error_log(ERR_SOCKET_RECV, errno);
                    break;
                }
                continue;
            }
            // Connection closed
            else if (len == 0) {
                recv_err_cnt = 0;
                Log_w("Connection is closed by peer\n");
                PUSH_LOG("Connection is closed by peer");
                continue;
            }
            // Data received
            else {
                recv_err_cnt = 0;
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                rx_buffer[len] = 0;
                Log_i("Received %d bytes from <%s:%u> msg: %s\n", len, addr_str, source_addr.sin_port, rx_buffer);
                PUSH_LOG("%s", rx_buffer);
                peer_client.socket_addr = (struct sockaddr *)&source_addr;  //与原接口不同，已修改
                peer_client.addr_len    = sizeof(source_addr);

                // send error log here, otherwise no chance for previous error
                ecr_get_and_post_error_log(&peer_client);

                ret = app_handle_recv_data(&peer_client, rx_buffer, len);
                if (ret == 1) {
                    Log_w("Finish app cmd handle\n");
                    PUSH_LOG("Finish app cmd handle");
                }
                break;

                // only one round of data recv/send
                // ecr_get_and_post_error_log(&peer_client);
                //continue;
            }
        } else if (0 == ret) {
            select_err_cnt = 0;
            Log_d("wait for read...\n");
            if (peer_client.socket_addr != NULL) {
                ecr_get_and_post_error_log(&peer_client);
            }
            continue;
        } else {
            select_err_cnt++;
            Log_w("select-recv error: %d, cnt: %d\n", errno, select_err_cnt);
            if (select_err_cnt > 3) {
                Log_e("select-recv error: %d, cnt: %d\n", errno, select_err_cnt);
                PUSH_LOG("select-recv error: %d, cnt: %d", errno, select_err_cnt);
                push_error_log(ERR_SOCKET_SELECT, errno);
                break;
            }
            HAL_SleepMs(500);
        }
    }

end_of_task:
    if (server_socket != -1) {
        Log_w("Shutting down UDP server socket\n");
        shutdown(server_socket, 0);
        close(server_socket);
    }

    sg_comm_task_run = false;
    Log_i("UDP server task quit\n");
    sg_comm_task_cnt--;
    vTaskDelete(NULL);
}

//============================ Qcloud app TCP/UDP functions end ===========================//

/****************************************************************************
 * Name:  ecr_softap_mode_config
 * Description:
 * @brief       start ap mode
 * @param[in]   ssid: device access point ssid
 * @param[in]   psw: device access point password
 * @param[in]   ch: device channel
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_softap_mode_config(const char *ssid, const char *psw, uint8_t ch)
{
    // 设置标志事件组
    sg_wifi_event_group = xEventGroupCreate();
    if (sg_wifi_event_group == NULL) {
        Log_e("xEventGroupCreate failed!\n");
        PUSH_LOG("xEventGroupCreate failed!");
        return -1;
    }
    xEventGroupClearBits(sg_wifi_event_group, CONNECTED_BIT | STA_DISCONNECTED_BIT);

    //预设wifi信息赋值
    wifi_config_u wificonfig;
    memset((void *)&wificonfig, 0, sizeof(wifi_config_u));
    strcpy((char *)wificonfig.ap.ssid, ssid);
    wificonfig.ap.max_connect = 3;
    wificonfig.ap.channel = ch;

    //根据有无设置psw来设置加密方式
    if (psw) {
        strcpy((char *)wificonfig.ap.password, psw);
        wificonfig.ap.authmode = AUTH_WPA_WPA2_PSK;
    } else {
        wificonfig.ap.authmode = AUTH_OPEN;
    }

    wifi_set_opmode(WIFI_MODE_AP);  /* set wifi mode */

    //wifi初始化后，才可配置AP/STA
    while (!wifi_is_ready()){
        Log_e("err! set AP/STA information must after wifi initialized!\n");
        PUSH_LOG("err! set AP/STA information must after wifi initialized!");
        HAL_SleepMs(50); //延时执行，直到WiFi就绪
    }

    //调用系统接口，初始化设置ap
    int ret = wifi_start_softap(&wificonfig);
    if (SYS_OK != ret) {
        Log_e("wifi_start_softap errno:%d\n",ret);
        PUSH_LOG("wifi_start_softap errno:%d",ret);
        return ret;
    }

    //修改设备ip地址
    ip_info_t ip_info;
    memset(&ip_info, 0, sizeof(ip_info));
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    set_softap_ipconfig(&ip_info);

    return 0;
}

//============================ MQTT communication functions begin ===========================//

/****************************************************************************
 * Name:  ecr_wifi_wait_event
 * Description:
 * @brief       query the status of station connected device AP
 * @param[in]   timeout_ms: wait timeout
 * @return  0: nothing happen, 1: station has connected to target AP, 2: station has been disconnected from AP
 ****************************************************************************/

int ecr_wifi_wait_event(unsigned int timeout_ms)
{
    EventBits_t uxBits =
        xEventGroupWaitBits(sg_wifi_event_group, CONNECTED_BIT | STA_DISCONNECTED_BIT, true, false,
                            timeout_ms / portTICK_RATE_MS);
    if (uxBits & CONNECTED_BIT) {
        return EVENT_WIFI_CONNECTED;
    }

    if (uxBits & STA_DISCONNECTED_BIT) {
        return EVENT_WIFI_DISCONNECTED;
    }

    return EVENT_WAIT_TIMEOUT;
}

/****************************************************************************
 * Name:  _get_reg_dev_info
 * Description:
 * @brief       get the device info or do device dynamic register
 * @param[in]   dev_info: device information
 * @return  0: success, -1: invalid
 ****************************************************************************/

static int _get_reg_dev_info(DeviceInfo *dev_info)
{
    if (strlen(dev_info->product_id) == 0 ||
            strlen(dev_info->device_name) == 0) {
             Log_e("invalid product_id or device_name");
        PUSH_LOG("invalid product_id or device_name");
        return -1;
    }

    /* just demo condition of dynamic register device
    device_secret == "YOUR_IOT_PSK" and product_secret != "YOUR_PRODUCT_SECRET"
    user can customize different policy here */
    if (!strncmp(dev_info->device_secret, "YOUR_IOT_PSK", MAX_SIZE_OF_DEVICE_SECRET) &&
        strncmp(dev_info->product_secret, "YOUR_PRODUCT_SECRET", MAX_SIZE_OF_PRODUCT_SECRET)) {

        /*===============设备动态注册于保存==========*/
        // int ret = IOT_DynReg_Device(dev_info);//动态注册
        // if (ret) {
        //     Log_e("dynamic register device failed: %d", ret);
        //     // //PUSH_LOG("dynamic register device failed: %d", ret);
        //     return ret;
        // }

        // // save the dev info
        // ret = HAL_SetDevInfo(dev_info);  //保存注册信息
        // if (ret) {
        //     Log_e("HAL_SetDevInfo failed: %d", ret);
        //     return ret;
        // }

        // // delay a while
        // HAL_SleepMs(500);
    }

    return QCLOUD_RET_SUCCESS;
}

/****************************************************************************
 * Name:  _mqtt_event_handler
 * Description:
 * @brief
 * @param[in]   pclient: mqtt data template client
 * @param[in]   handle_context: mqtt event handler
 * @param[in]   msg: mqtt event message from qcloud
 ****************************************************************************/

static void _mqtt_event_handler(void *pclient, void *handle_context, MQTTEventMsg *msg)
{
    MQTTMessage *    mqtt_messge = (MQTTMessage *)msg->msg;
    uintptr_t        packet_id   = (uintptr_t)msg->msg;
    TokenHandleData *app_data    = (TokenHandleData *)handle_context;

    switch (msg->event_type) {
        case MQTT_EVENT_DISCONNECT:
            Log_w("MQTT disconnect");
            PUSH_LOG("MQTT disconnect");
            break;

        case MQTT_EVENT_RECONNECT:
            Log_i("MQTT reconnected");
            PUSH_LOG("MQTT reconnected");
            break;

        case MQTT_EVENT_PUBLISH_RECVEIVED:
            Log_i("unhandled msg arrived: topic=%s", mqtt_messge->ptopic);
            break;

        case MQTT_EVENT_SUBCRIBE_SUCCESS:
            Log_d("mqtt topic subscribe success");
            app_data->sub_ready = true;
            break;

        case MQTT_EVENT_SUBCRIBE_TIMEOUT:
            Log_i("mqtt topic subscribe timeout");
            app_data->sub_ready = false;
            break;

        case MQTT_EVENT_SUBCRIBE_NACK:
            Log_i("mqtt topic subscribe NACK");
            app_data->sub_ready = false;
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            Log_i("publish success, packet-id=%u", (unsigned int)packet_id);
            app_data->send_ready = true;
            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            Log_i("publish timeout, packet-id=%u", (unsigned int)packet_id);
            app_data->send_ready = false;
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            Log_i("publish nack, packet-id=%u", (unsigned int)packet_id);
            app_data->send_ready = false;
            break;

        default:
            Log_i("unknown event type: %d", msg->event_type);
            break;
    }
}

/****************************************************************************
 * Name:  _setup_mqtt_connect
 * Description:
 * @brief       setup mqtt connection and return client handle
 * @param[in]   dev_info: device propery information
 * @param[in]   app_data: token handle data
 ****************************************************************************/

static void *_setup_mqtt_connect(DeviceInfo *dev_info, TokenHandleData *app_data)
{
    MQTTInitParams init_params       = DEFAULT_MQTTINIT_PARAMS;
    init_params.device_name          = dev_info->device_name;
    init_params.product_id           = dev_info->product_id;
    init_params.device_secret        = dev_info->device_secret;
    init_params.region               = dev_info->region;
    init_params.event_handle.h_fp    = _mqtt_event_handler;
    init_params.event_handle.context = (void *)app_data;
    void *client = IOT_MQTT_Construct(&init_params);
    if (client != NULL) {
        Log_i("Cloud Device Construct Success");
        return client;
    }

    // construct failed, give it another try
    if (0 == ecr_is_wifi_config_successful()) {
        Log_e("SAT is connected to AP. init_params.err_code = %d! Try one more time!", init_params.err_code);
        HAL_SleepMs(1000);
    } else {
        Log_e("Wifi lost connection! Wait and retry!");
        HAL_SleepMs(2000);
    }

    client = IOT_MQTT_Construct(&init_params);
    if (client != NULL) {
        Log_i("Cloud Device Construct Success");
        return client;
    }

    Log_e("Device %s/%s connect failed: %d",
        dev_info->product_id, dev_info->device_name, init_params.err_code);
    PUSH_LOG("Device %s/%s connect failed: %d",
        dev_info->product_id, dev_info->device_name, init_params.err_code);
    push_error_log(ERR_MQTT_CONNECT, init_params.err_code);
    return NULL;
}

/****************************************************************************
 * Name:  _on_message_callback
 * Description:
 * @brief       callback when MQTT msg arrives
 * @param[in]   pClient: mqtt data template client
 * @param[in]   message: message from mqtt
 * @param[in]   userData: token handle data, include reply code
 ****************************************************************************/

static void _on_message_callback(void *pClient, MQTTMessage *message, void *userData)
{
    if (message == NULL) {
        Log_e("msg null");
        return;
    }

    if (message->topic_len == 0 && message->payload_len == 0) {
        Log_e("length zero");
        return;
    }

    Log_i("recv msg topic: %s", message->ptopic);

    uint32_t msg_topic_len = message->payload_len + 4;
    char *   buf           = (char *)HAL_Malloc(msg_topic_len);
    if (buf == NULL) {
        Log_e("malloc %u bytes failed", msg_topic_len);
        return;
    }

    memset(buf, 0, msg_topic_len);
    memcpy(buf, message->payload, message->payload_len);
    Log_i("msg payload: %s", buf);

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        Log_e("Parsing JSON Error");
        // push_error_log(ERR_TOKEN_REPLY, ERR_APP_CMD_JSON_FORMAT);
    } else {
        cJSON *method_json = cJSON_GetObjectItem(root, "method");
        cJSON *code_json = cJSON_GetObjectItem(root, "code");
        if (NULL != method_json) {
            if (0 == strcmp("bind_device", method_json->valuestring)) {
                Log_d("method [bind_device] msg down");
                sg_wait_app_bind_device_state = WAIT_APP_BIND_FALSE;
            } else if (0 == strcmp("bind_device_query_reply", method_json->valuestring)) {
                if (NULL != code_json) {
                    Log_d("method [bind_device_query_reply] bind code = %d", code_json->valueint);
                    TokenHandleData *app_data = (TokenHandleData *)userData;
                    app_data->reply_code      = code_json->valueint;
                }
            } else if (0 == strcmp("app_bind_token_reply", method_json->valuestring)) {
                if (code_json) {
                    Log_d("method [app_bind_token_reply] bind code = %d", code_json->valueint);
                    TokenHandleData *app_data = (TokenHandleData *)userData;
                    app_data->reply_code      = code_json->valueint;
                    Log_d("token reply code = %d", code_json->valueint);

                    if (app_data->reply_code) {
                        Log_e("token reply error: %d", code_json->valueint);
                        PUSH_LOG("token reply error: %d", code_json->valueint);
                        push_error_log(ERR_TOKEN_REPLY, app_data->reply_code);
                    }
                } else {
                    Log_e("Parsing reply code Error");
                    push_error_log(ERR_TOKEN_REPLY, ERR_APP_CMD_JSON_FORMAT);
                }
            }
        }
        cJSON_Delete(root);
    }
    HAL_Free(buf);
}

/****************************************************************************
 * Name:  _subscribe_topic_wait_result
 * Description:
 * @brief
 * @param[in]   client: mqtt data template client
 * @param[in]   dev_info: device propery information
 * @param[in]   app_data: token handle data
 * @return  0 when success, or err code for failure
 ****************************************************************************/

static int _subscribe_topic_wait_result(void *client, DeviceInfo *dev_info, TokenHandleData *app_data)
{
    char topic_name[128] = {0};
    // int size = HAL_Snprintf(topic_name, sizeof(topic_name), "%s/%s/data", dev_info->product_id,
    // dev_info->device_name);
    int size = HAL_Snprintf(topic_name, sizeof(topic_name), "$thing/down/service/%s/%s", dev_info->product_id,
                            dev_info->device_name);
    if (size < 0 || size > sizeof(topic_name) - 1) {
        Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
        return QCLOUD_ERR_FAILURE;
    }
    SubscribeParams sub_params    = DEFAULT_SUB_PARAMS;
    sub_params.qos                = QOS0;
    sub_params.on_message_handler = _on_message_callback;
    sub_params.user_data          = (void *)app_data;
    int rc                        = IOT_MQTT_Subscribe(client, topic_name, &sub_params);
    if (rc < 0) {
        Log_e("MQTT subscribe failed: %d", rc);
        return rc;
    }

    int wait_cnt = 2;
    while (!app_data->sub_ready && (wait_cnt-- > 0)) {
        // wait for subscription result
        rc = IOT_MQTT_Yield(client, 1000);
        if (rc) {
            Log_e("MQTT error: %d", rc);
            return rc;
        }
    }

    if (wait_cnt > 0) {
        return QCLOUD_RET_SUCCESS;
    } else {
        Log_w("wait for subscribe result timeout!");
        return QCLOUD_ERR_FAILURE;
    }
}

/****************************************************************************
 * Name:  _publish_token_msg
 * Description:
 * @brief       publish MQTT message
 * @param[in]   client: mqtt data template client
 * @param[in]   dev_info: device propery information
 * @param[in]   token_str: client token
 * @return  >0 when success, or err code for failure
 ****************************************************************************/

static int _publish_token_msg(void *client, DeviceInfo *dev_info, char *token_str)
{
    char topic_name[128] = {0};
    // int size = HAL_Snprintf(topic_name, sizeof(topic_name), "%s/%s/data", dev_info->product_id,
    // dev_info->device_name);
    int size = HAL_Snprintf(topic_name, sizeof(topic_name), "$thing/up/service/%s/%s", dev_info->product_id,
                            dev_info->device_name);
    if (size < 0 || size > sizeof(topic_name) - 1) {
        Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
        return QCLOUD_ERR_FAILURE;
    }

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = QOS1;

    char topic_content[256] = {0};
    size                    = HAL_Snprintf(topic_content, sizeof(topic_content),
                        "{\"method\":\"app_bind_token\",\"clientToken\":\"%s-%u\",\"params\": {\"token\":\"%s\"}}",
                        dev_info->device_name, HAL_GetTimeMs(), token_str);
    if (size < 0 || size > sizeof(topic_content) - 1) {
        Log_e("payload content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_content));
        return QCLOUD_ERR_MALLOC;
    }

    pub_params.payload     = topic_content;
    pub_params.payload_len = strlen(topic_content);

    return IOT_MQTT_Publish(client, topic_name, &pub_params);
}

/****************************************************************************
 * Name:  _send_token_wait_reply
 * Description:
 * @brief       send publish token messsage and wait mqtt reply
 * @param[in]   client: mqtt data template client
 * @param[in]   dev_info: device propery information
 * @param[in]   app_data: token handle data
 * @return  0 when success, or err code for failure
 ****************************************************************************/

static int _send_token_wait_reply(void *client, DeviceInfo *dev_info, TokenHandleData *app_data)
{
    int ret      = 0;
    int wait_cnt = 20;

    // for smartconfig, we need to wait for the token data from app
    while (!sg_token_received && (wait_cnt-- > 0)) {
        IOT_MQTT_Yield(client, 1000);
        Log_i("wait for token data...");
    }

    if (!sg_token_received) {
        Log_e("Wait for token data timeout");
        PUSH_LOG("Wait for token data timeout");
        return ERR_TOKEN_RECV;
    }

    // compare region of user and region of product
    if (strncmp(dev_info->region, sg_region_str, MAX_SIZE_OF_REGION)) {
        Log_e("Region fault: %s != %s", dev_info->region, sg_region_str);
        PUSH_LOG("Region fault: %s != %s", dev_info->region, sg_region_str);
        return ERR_REGION_FAULT;
    }

    wait_cnt = 3;
publish_token:
    ret = _publish_token_msg(client, dev_info, sg_token_str); // publish MQTT msg
    if (ret < 0 && (wait_cnt-- > 0)) {
        Log_e("Client publish token failed: %d", ret);
        if ((0 == ecr_is_wifi_config_successful()) && IOT_MQTT_IsConnected(client)) {
            IOT_MQTT_Yield(client, 500);
        } else {
            Log_e("Wifi or MQTT lost connection! Wait and retry!");
            IOT_MQTT_Yield(client, 2000);
        }
        goto publish_token;
    }

    if (ret < 0) {
        Log_e("pub token failed: %d", ret);
        PUSH_LOG("pub token failed: %d", ret);
        return ret;
    }

    wait_cnt = 5;
    while (!app_data->send_ready && (wait_cnt-- > 0)) {
        IOT_MQTT_Yield(client, 1000);
        Log_i("wait for token sending result...");
    }

    ret = 0;
    if (!app_data->send_ready) {
        Log_e("pub token timeout");
        PUSH_LOG("pub token timeout");
        ret = QCLOUD_ERR_MQTT_UNKNOWN;
    }

    return ret;
}


#if WIFI_CONFIG_WAIT_APP_BIND_STATE

/****************************************************************************
 * Name:  _wait_platform_bind_device_msg
 * Description:
 * @brief       wait cloud platform publish bind_device method msg
 * @param[in]   mqtt_client: mqtt client
 * @return  0: qcloud return success, or err code for failure
 ****************************************************************************/

static int _wait_platform_bind_device_msg(void *mqtt_client)
{
    int retry_cnt = 120;
    int ret = QCLOUD_RET_SUCCESS;

    if (NULL == mqtt_client) {
        Log_e("wait method [bind_deive] failed, mqtt_client is NULL");
        return QCLOUD_ERR_FAILURE;
    }

    // wait 2mins
    retry_cnt = 120;
    ret = QCLOUD_ERR_MQTT_REQUEST_TIMEOUT;
    do {
        // bind_device method recved
        if (WAIT_APP_BIND_FALSE == sg_wait_app_bind_device_state) {
            if (0 != customer_set_env_blob(WAIT_BIND, (char *)&sg_wait_app_bind_device_state,
                sizeof(sg_wait_app_bind_device_state))) {
                Log_e("set wait_app_bind_device false failed");
            }
            ret = QCLOUD_RET_SUCCESS;
            break;
        }
        IOT_MQTT_Yield(mqtt_client, 1000);
        Log_d("wait for bind device method state:%d, task_run:%d, try:%d, ret:%d", sg_wait_app_bind_device_state,
              sg_mqtt_task_run, retry_cnt, ret);

    } while (ret && retry_cnt-- && sg_mqtt_task_run);

    if (retry_cnt == 0) {
        Log_e("wait method [bind_deive] timeout");
    }

    return ret;
}

/****************************************************************************
 * Name:  _query_app_bind_device_result
 * Description:
 * @brief       query app bind device result; device publish bind_device_query msg
 * @param[in]   client: mqtt data template client
 * @param[in]   dev_info: device propery information
 * @param[in]   app_data: token handle data
 * @return  0 when success, or err code for failure
 ****************************************************************************/

static int _query_app_bind_device_result(void *client, DeviceInfo *dev_info, TokenHandleData *app_data)
{
    int ret = QCLOUD_RET_SUCCESS;
    int wait_cnt = 5;
    char topic_name[128] = {0};

    // publish bind_device_query msg
    int size = HAL_Snprintf(topic_name, sizeof(topic_name), "$thing/up/service/%s/%s", dev_info->product_id,
                            dev_info->device_name);
    if (size < 0 || size > sizeof(topic_name) - 1) {
        Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
        return QCLOUD_ERR_FAILURE;
    }

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = QOS0;

    char topic_content[256] = {0};
    size                    = HAL_Snprintf(topic_content, sizeof(topic_content),
                        "{\"method\":\"bind_device_query\",\"clientToken\":\"%s-%u\",\"timestamp\": %u}",
                        dev_info->device_name, HAL_GetTimeMs(), HAL_GetTimeMs());
    if (size < 0 || size > sizeof(topic_content) - 1) {
        Log_e("payload content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_content));
        return QCLOUD_ERR_MALLOC;
    }

    pub_params.payload     = topic_content;
    pub_params.payload_len = strlen(topic_content);

publish_query_msg:
    app_data->reply_code = -1;
    app_data->send_ready = false;

    ret = IOT_MQTT_Publish(client, topic_name, &pub_params);

    if (ret < 0 && (wait_cnt-- > 0)) {
        Log_e("Client publish query bind device failed: %d", ret);
        if ((ecr_is_wifi_config_successful()==0) && IOT_MQTT_IsConnected(client)) {
            IOT_MQTT_Yield(client, 500);
        } else {
            Log_e("Wifi or MQTT lost connection! Wait and retry!");
            IOT_MQTT_Yield(client, 2000);
        }
        goto publish_query_msg;
    }

    if (ret < 0) {
        Log_e("pub query bind device failed: %d", ret);
        return ret;
    }

    wait_cnt = 5;
    do {
        IOT_MQTT_Yield(client, 1000);
        Log_i("wait for query bind device result %d, %d,%d...", app_data->reply_code, app_data->send_ready);
    } while ((-1 == app_data->reply_code) && (wait_cnt-- > 0));

    if (wait_cnt != 0) {
        int value = 0;
        if (0 != customer_set_env_blob(WAIT_BIND, (char *)&value, sizeof(value))) {
            Log_e("erase wait_wechat_applet_bind_device false failed");
        }
    }

    if (0 == app_data->reply_code) {
        Log_e("query bind device binded");
        ret = LAST_APP_BIND_STATE_SUCCESS;
    } else {
        Log_e("query bind device not bind code:%d", app_data->reply_code);
        ret = LAST_APP_BIND_STATE_FAILED;
    }

    return ret;
}

/****************************************************************************
 * Name:  mqtt_query_app_bind_result
 * Description:
 * @brief       query last time app bind status
 * @return  LAST_APP_BIND_STATE_SUCCESS: binded
 *          LAST_APP_BIND_STATE_FAILED:  not binded
 *          LAST_APP_BIND_STATE_NOBIND:  no app bind device
 *          LAST_APP_BIND_STATE_ERROR:   func error
 ****************************************************************************/

WIFI_CONFIG_LAST_APP_BIND_STATE mqtt_query_app_bind_result(void)
{
    //get the device info or do device dynamic register
    DeviceInfo  dev_info;
    char wait_app_bind_device_state = 0x00;
    int ret = QCLOUD_RET_SUCCESS;
    void *client = NULL;

    if (0 == customer_get_env_blob("wait_bind",&wait_app_bind_device_state, sizeof(wait_app_bind_device_state), NULL)) {
        if (wait_app_bind_device_state == 0){
            Log_i("get storage failed, no wait_bind key");
            return LAST_APP_BIND_STATE_NOBIND;
        }
    } else {
        sg_wait_app_bind_device_state = wait_app_bind_device_state;
        if (WAIT_APP_BIND_FALSE == sg_wait_app_bind_device_state) {
            Log_i("get storage success not wait app bind state");
            return LAST_APP_BIND_STATE_NOBIND;
        }
    }

    ret = HAL_GetDevInfo(&dev_info);
    if (ret) {
        Log_e("Get device info failed: %d", ret);
        return LAST_APP_BIND_STATE_ERROR;
    }
    //token handle data
    TokenHandleData app_data;
    app_data.sub_ready = false;
    app_data.send_ready = false;
    app_data.reply_code = -1;

    //mqtt connection
    client = _setup_mqtt_connect(&dev_info, &app_data);
    if (client == NULL) {
        ret = IOT_MQTT_GetErrCode();
        Log_e("Cloud Device Construct Failed: %d", ret);
        push_error_log(ERR_MQTT_CONNECT, ret);
        return LAST_APP_BIND_STATE_ERROR;
    }

    //subscribe token reply topic
    ret = _subscribe_topic_wait_result(client, &dev_info, &app_data);
    if (ret < 0) {
        Log_w("Client Subscribe Topic Failed: %d", ret);
        ret = LAST_APP_BIND_STATE_ERROR;
    } else if ((ret = _query_app_bind_device_result(client, &dev_info, &app_data)) < 0) {
        Log_w("query app bind failed: %d", ret);
        ret = LAST_APP_BIND_STATE_FAILED;
    }

    IOT_MQTT_Destroy(&client);
    if (0 == ret) {
        HAL_SleepMs(5000);
    }

    return ret;
}
#endif  /* WIFI_CONFIG_WAIT_APP_BIND_STATE */

/****************************************************************************
 * Name:  mqtt_send_token
 * Description:
 * @brief       mqtt connection, subscribe token reply topic, send token msg from app to device by AP mode, and wait bind device msg
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int mqtt_send_token(void)
{
    // get the device info or do device dynamic register
    DeviceInfo *dev_info = &sg_device_info;
    int              ret = _get_reg_dev_info(dev_info);
    if (ret) {
        Log_e("Get device info failed: %d", ret);
        PUSH_LOG("Get device info failed: %d", ret);
        push_error_log(ERR_DEVICE_INFO, ret);
        return ret;
    }

    // token handle data
    TokenHandleData app_data;
    app_data.sub_ready  = false;
    app_data.send_ready = false;
    app_data.reply_code = 404;

    // mqtt connection
    void *client = _setup_mqtt_connect(dev_info, &app_data);
    if (client == NULL) {
        return QCLOUD_ERR_MQTT_NO_CONN;
    } else {
        Log_i("Device %s/%s connect success", dev_info->product_id, dev_info->device_name);
        PUSH_LOG("Device %s/%s connect success", dev_info->product_id, dev_info->device_name);
    }

    // subscribe token reply topic
    ret = _subscribe_topic_wait_result(client, dev_info, &app_data);
    if (ret < 0) {
        Log_w("Subscribe topic failed: %d", ret);
        PUSH_LOG("Subscribe topic failed: %d", ret);
    }

#if WIFI_CONFIG_WAIT_APP_BIND_STATE
    sg_wait_app_bind_device_state = WAIT_APP_BIND_TRUE;
    /* store wait wechat applet bind success flag to flash */
    if (0 != customer_set_env_blob(WAIT_BIND, (char *)&sg_wait_app_bind_device_state,
            sizeof(sg_wait_app_bind_device_state))) {
        Log_e("set wait_app_bind_device true failed");
    }
#endif  /* WIFI_CONFIG_WAIT_APP_BIND_STATE */

    // publish token msg and wait for reply
    int retry_cnt = 2;
    do {
        ret = _send_token_wait_reply(client, dev_info, &app_data);

        IOT_MQTT_Yield(client, 1000);
    } while (ret && ret != ERR_REGION_FAULT && retry_cnt-- && sg_mqtt_task_run);

    if (ret) {
        switch (ret) {
            case ERR_TOKEN_RECV:
            case ERR_REGION_FAULT:
                push_error_log(ret, QCLOUD_ERR_INVAL);
                break;
            default:
                push_error_log(ERR_TOKEN_SEND, ret);
                break;
        }
    }

#if WIFI_CONFIG_WAIT_APP_BIND_STATE
    if ((QCLOUD_RET_SUCCESS == ret) && (0 == app_data.reply_code)) {
        ret = _wait_platform_bind_device_msg(client);
    } else if ((QCLOUD_RET_SUCCESS == ret) && (0 != app_data.reply_code)) {
        Log_e("app bind device code:%d", app_data.reply_code);
        ret = QCLOUD_ERR_FAILURE;
    }
#else
    if( QCLOUD_RET_SUCCESS == ret ) {
        int bind_state = 0;
        customer_set_env_blob("bind_state", (char *)&bind_state, sizeof(bind_state));
    }
#endif  /* WIFI_CONFIG_WAIT_APP_BIND_STATE */

    IOT_MQTT_Destroy(&client);

    // sleep 5 seconds to avoid frequent MQTT connection
    if (ret == 0)
        HAL_SleepMs(5000);

    return ret;
}

/****************************************************************************
 * Name:  ecr_softap_mqtt_task
 * Description:
 * @brief       app connect decive by softAP mode, send token, and stop softAP mode
 * @param[in]   pvParameters:
 ****************************************************************************/

static void ecr_softap_mqtt_task(void *pvParameters)
{
    uint32_t server_count = WIFI_CONFIG_WAIT_TIME_MS / SOFT_AP_BLINK_TIME_MS;
    bool count_flag = true;
    sg_mqtt_task_cnt++;

    while (sg_mqtt_task_run && (--server_count))
    {
        int state = ecr_wifi_wait_event(SOFT_AP_BLINK_TIME_MS);
        if (state == EVENT_WIFI_CONNECTED) {
            Log_d("WiFi Connected to ap");
            //set_wifi_led_state(LED_ON);  //设置灯的状态
#if defined(CONFIG_BLE_TENCENT_ADAPTER)
            if(ble_token_received)
            {
                sg_token_received = true;
                strncpy(sg_token_str, ble_token_str, MAX_TOKEN_LENGTH);
                Log_i("[eswin] ble send token to sg_token_str");
            }
#endif
            int ret = mqtt_send_token();  //发送token
            if (ret) {
                Log_e("SoftAP: WIFI_MQTT_CONNECT_FAILED: %d", ret);
                PUSH_LOG("SoftAP: WIFI_MQTT_CONNECT_FAILED: %d", ret);
                //set_wifi_led_state(LED_OFF);
            } else {
                Log_i("SoftAP: WIFI_MQTT_CONNECT_SUCCESS");
                PUSH_LOG("SoftAP: WIFI_MQTT_CONNECT_SUCCESS");
                //set_wifi_led_state(LED_ON);
                sg_wifi_config_success = true;
            }
            break;
        }
        else {
            // reduce the wait time as it meet disconnect
            if (count_flag) {
                server_count = WIFI_CONFIG_HALF_TIME_MS / SOFT_AP_BLINK_TIME_MS;
                count_flag = false;
            }
            // Log_i("disconnect event! wait count change to %d", server_count);
        }
        // check comm server task state
        if (!sg_comm_task_run && !sg_token_received) {
            Log_e("comm server task error!");
            PUSH_LOG("comm server task error");
            //set_wifi_led_state(LED_OFF);
            break;
        }
    }
    if (server_count == 0 && !sg_wifi_config_success) {
        Log_w("SoftAP: TIMEOUT");
        PUSH_LOG("SoftAP: TIMEOUT");
        push_error_log(ERR_BD_STOP, ERR_SC_EXEC_TIMEOUT);
        //set_wifi_led_state(LED_OFF);
    }

    wifi_stop_softap();

    sg_comm_task_run = false;
    sg_mqtt_task_run = false;
    Log_i("softAP mqtt task quit");
    sg_mqtt_task_cnt--;
    vTaskDelete(NULL);
}

//============================ MQTT communication functions end ===========================//

/****************************************************************************
 * Name:  ecr_start_softap
 * Description:
 * @brief       softAP main processes
 * @param[in]   ssid: device access point ssid
 * @param[in]   psw: device access point password
 * @param[in]   ch: channel
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_start_softap(const char *ssid, const char *psw, uint8_t ch)
{
    sg_mqtt_task_run = false;
    sg_comm_task_run = false;
    sg_log_task_run = false;

    while (sg_comm_task_cnt > 0 || sg_mqtt_task_cnt > 0 || sg_log_task_cnt > 0) {
        Log_w("Wait for last config finish: %d %d", sg_comm_task_cnt, sg_mqtt_task_cnt, sg_log_task_cnt);
        sg_mqtt_task_run = false;
        sg_comm_task_run = false;
        sg_log_task_run = false;
        HAL_SleepMs(1000);
    }

    init_dev_log_queue();
    init_error_log_queue(); //存在问题
    Log_i("enter softAP mode");
    PUSH_LOG("start softAP: %s", ssid);

    int ret;
    /* 得到产品预设的ID与设备名 */
    ret = HAL_GetDevInfo(&sg_device_info);
    if (ret) {
        Log_e("load device info failed: %d", ret);
        PUSH_LOG("load device info failed: %d", ret);
        push_error_log(ERR_DEVICE_INFO, ret);
        ret = ERR_DEVICE_INFO;
        goto err_exit;
    }

    /*  step1: 进入ap模式 */
    ret = ecr_softap_mode_config(ssid, psw, ch);
    if (ret != SYS_OK) {
        Log_e("eswin_softap_enter_ap_mode failed: %d", ret);
        PUSH_LOG("eswin_softap_enter_ap_mode failed: %d", ret);
        push_error_log(ERR_WIFI_START, ret);
        ret = ERR_WIFI_START;
        goto err_exit;
    }

    /* step2：udp server */
    sg_comm_task_run = true;
    ret = xTaskCreate(ecr_udp_server_task, COMM_SERVER_TASK_NAME, COMM_SERVER_TASK_STACK_BYTES, NULL, 3,
                      NULL);
    if (ret != pdPASS) {
        Log_e("create comm_server_task failed: %d", ret);
        PUSH_LOG("create comm_server_task failed: %d", ret);
        push_error_log(ERR_OS_TASK, ret);
        ret = ERR_OS_TASK;
        goto err_exit;
    }
    /* step3：mqtt server */
    sg_mqtt_task_run = true;
    ret = xTaskCreate(ecr_softap_mqtt_task, SOFTAP_TASK_NAME, SOFTAP_TASK_STACK_BYTES, NULL, 2, NULL);
    if (ret != pdPASS) {
        Log_e("create ecr_softap_mqtt_task failed: %d", ret);
        PUSH_LOG("create ecr_softap_mqtt_task failed: %d", ret);
        push_error_log(ERR_OS_TASK, ret);
        ret = ERR_OS_TASK;
        goto err_exit;
    }

    return 0;

err_exit:
    wifi_stop_softap();

    sg_mqtt_task_run = false;
    sg_comm_task_run = false;

    return ret;
}

#ifdef WIFI_LOG_UPLOAD

/****************************************************************************
 * Name:  log_server_task
 * Description:
 * @brief       device log task by udp server under softAP mode
 * @param[in]   pvParameters:
 ****************************************************************************/

static void log_server_task(void *pvParameters)
{
    int  ret, server_socket = -1;
    char addr_str[128] = {0};
    sg_log_task_cnt++;

    /* stay 6 minutes to handle log */
    uint32_t server_count = 360 / SELECT_WAIT_TIME_SECONDS;

    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(LOG_SERVER_PORT);
    inet_ntoa_r(server_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (server_socket < 0) {
        Log_e("socket failed: errno %d", errno);
        goto end_of_task;
    }

    ret = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        Log_e("bind failed: errno %d", errno);
        goto end_of_task;
    }

    Log_i("LOG server socket listening...");
    fd_set      sets;
    comm_peer_t1 peer_client = {
        .socket_id   = server_socket,
        .socket_addr = NULL,
        .addr_len    = 0,
    };

    int select_err_cnt = 0;
    int recv_err_cnt   = 0;
    while (sg_log_task_run && --server_count) {
        FD_ZERO(&sets);
        FD_SET(server_socket, &sets);
        struct timeval timeout;
        timeout.tv_sec  = SELECT_WAIT_TIME_SECONDS;
        timeout.tv_usec = 0;

        int ret = select(server_socket + 1, &sets, NULL, NULL, &timeout);
        if (ret > 0) {
            select_err_cnt = 0;
            struct sockaddr_in source_addr;
            uint               addrLen        = sizeof(source_addr);
            char               rx_buffer[256] = {0};

            int len = recvfrom(server_socket, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT,
                               (struct sockaddr *)&source_addr, (socklen_t *)addrLen);

            // Error occured during receiving
            if (len < 0) {
                recv_err_cnt++;
                Log_w("recvfrom error: %d, cnt: %d", errno, recv_err_cnt);
                if (recv_err_cnt > 3) {
                    Log_e("recvfrom error: %d, cnt: %d", errno, recv_err_cnt);
                    break;
                }
                continue;
            }
            // Connection closed
            else if (len == 0) {
                recv_err_cnt = 0;
                Log_w("Connection is closed by peer");
                continue;
            }
            // Data received
            else {
                recv_err_cnt = 0;
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                rx_buffer[len] = 0;
                Log_i("Received %d bytes from <%s:%u> msg: %s", len, addr_str, source_addr.sin_port, rx_buffer);

                peer_client.socket_addr = (struct sockaddr *)&source_addr;
                peer_client.addr_len    = sizeof(source_addr);

                if (strncmp(rx_buffer, "{\"cmdType\":3}", 12) == 0) {
                    ret = ecr_send_dev_log_to_app(&peer_client);
                    Log_i("ecr_send_dev_log_to_app ret: %d", ret);
                    break;
                }

                continue;
            }
        } else if (0 == ret) {
            select_err_cnt = 0;
            Log_d("wait for read...");
            continue;
        } else {
            select_err_cnt++;
            Log_w("select-recv error: %d, cnt: %d", errno, select_err_cnt);
            if (select_err_cnt > 3) {
                Log_e("select-recv error: %d, cnt: %d", errno, select_err_cnt);
                break;
            }
            HAL_SleepMs(500);
        }
    }

end_of_task:
    if (server_socket != -1) {
        Log_w("Shutting down LOG server socket");
        shutdown(server_socket, 0);
        close(server_socket);
    }

    // don't destory if mqtt task run
    if (!sg_mqtt_task_run) {
        wifi_stop_softap();
        delete_dev_log_queue();
    }

    sg_log_task_run = false;
    Log_i("LOG server task quit");

    sg_log_task_cnt--;
    vTaskDelete(NULL);
}
#endif  /* WIFI_LOG_UPLOAD */

/****************************************************************************
 * Name:  ecr_start_softap_log
 * Description:
 * @brief       device start log task by udp server
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_start_softap_log(void)
{
#ifdef WIFI_LOG_UPLOAD
    Log_i("enter log softAP mode");

    sg_log_task_run = false;

    /*  step1: 进入ap模式 */
    int ret = ecr_softap_mode_config("ESP-LOG-QUERY", "86013388", 0);
    if (ret != SYS_OK) {
        Log_e("eswin_softap_enter_ap_mode failed: %d", ret);
    }

    sg_log_task_run = true;
    ret = xTaskCreate(log_server_task, "log_server_task", COMM_SERVER_TASK_STACK_BYTES, NULL, COMM_SERVER_TASK_PRIO,
                      NULL);
    if (ret != pdPASS) {
        Log_e("create log_server_task failed: %d", ret);
        goto err_exit;
    }

    return 0;

err_exit:

    wifi_stop_softap();
    delete_dev_log_queue();
    sg_log_task_run = false;
#endif  /* WIFI_LOG_UPLOAD */
    return QCLOUD_ERR_FAILURE;
}
