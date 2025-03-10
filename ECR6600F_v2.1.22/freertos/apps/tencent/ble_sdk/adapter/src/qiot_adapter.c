/****************************************************************************
  * apps/tencent/ble_sdk/adapter/src/qiot_adapter.c
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  ZhengdongXia <xiazhengdong@eswin.com>
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

#include "cli.h"//print
#include "system_wifi.h"
#include "system_network.h"
#include "oshal.h"

#if defined (CONFIG_NV)
#include "system_config.h"
#include "easyflash.h"
#endif

#include "system_wifi.h"
#include "system_network.h"
#include "ecr_qcloud_iot.h"
#include "ble_qiot_export.h"
#include "ble_qiot_import.h"
#include "ble_qiot_config.h"
#include "ble_qiot_service.h"
#include "ble_qiot_common.h"
#include "qiot_adapter.h"
#include "qcloud_iot_import.h"
#include "qcloud_iot_export.h"
#include "ble_qiot_log.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern DeviceInfo qiot_device_info;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ble_get_product_id
 * Description:
 * @brief       get product id
 * @param[out]  product_id: product id information
 * @return  0
 ****************************************************************************/

int ble_get_product_id(char *product_id)
{
    memcpy(product_id, qiot_device_info.product_id,BLE_QIOT_PRODUCT_ID_LEN);
    //ble_qiot_log_i("[eswin]product id is:%s\r\n",qiot_device_info.product_id);
    return 0;
}

/****************************************************************************
 * Name:  ble_get_device_name
 * Description:
 * @brief       get device name
 * @param[out]  device_name: device name information
 * @return  length of device_name
 ****************************************************************************/

int ble_get_device_name(char *device_name)
{
    memcpy(device_name, qiot_device_info.device_name,sizeof(qiot_device_info.device_name));
    //ble_qiot_log_i("[eswin]product name is:%s\r\n",qiot_device_info.device_name);
    return strlen(device_name);
}

/****************************************************************************
 * Name:  ble_get_mac
 * Description:
 * @brief      get device mac address
 * @param[out] mac: device mac address
 * @return  0
 ****************************************************************************/

int ble_get_mac(char *mac)
{
    uint8_t dev_mac[6];
    int ret = wifi_get_mac_addr(STATION_IF, dev_mac);
    if(ret == 0)
    {
        memcpy(mac, dev_mac, 6);
        return 0;
    }
    else
    {
        return -1;
    }
}

/****************************************************************************
 * Name:  ble_get_user_data_mtu_size
 * Description:
 * @brief       set mtu size
 * @return  128
 ****************************************************************************/

uint16_t ble_get_user_data_mtu_size(void)
{
    return 128;
}

/****************************************************************************
 * Name:  ble_services_add
 * Description:
 * @brief      do nothing
 * @param[in]   p_service:
 ****************************************************************************/

void ble_services_add(const qiot_service_init_s *p_service)
{
    // do nothing
    return;
}

/****************************************************************************
 * Name:  qiot_flash_SetString
 * Description:
 * @brief       set data in qiot_flash
 * @param[in]   key: Guideposts for writing elements
 * @param[in]   value: What to write
 * @param[in]   len: the number of char elements in "value"
 * @return  0: success, -1: failure
 ****************************************************************************/

int32_t qiot_flash_SetString(const char* key, const char* value, int32_t len)
{
    int32_t ret;
    ret = customer_set_env_blob(key, value, sizeof(char) * len);/* 这里的len应该为value的总长度 */
    if (ret == 0) {
        ble_qiot_log_i("key = %s, fwrite success!\n\r", key);
    }
    else {
        ble_qiot_log_e("fwrite error!\n\r");
        ret = -1;
    }
    return ret;
}

/****************************************************************************
 * Name:  qiot_flash_GetString
 * Description:
 * @brief       get data in qiot_flash
 * @param[in]   key: Guideposts for writing elements
 * @param[in]   buff: What to write
 * @param[in]   len: the number of char elements in "buff"
 * @return  length of read data
 ****************************************************************************/

int32_t qiot_flash_GetString(const char* key, char* buff, int32_t len)
{
    int32_t read_len;
    read_len = customer_get_env_blob(key, buff, len, NULL);
    if (read_len == 0) {
        ble_qiot_log_i("qiot_flash_GetString, fread failed, key : %s, len = %d, read_len = 0\n\r", key, len);
    } else {
        ble_qiot_log_i("qiot_flash_GetString, key : %s, read_len = %d\n\r", key, read_len);
    }
    return read_len;
}

/****************************************************************************
 * Name:  is_wifi_connect_successful
 * Description:
 * @brief       get status of wifi connected
 * @return  0: success, -1: failure
 ****************************************************************************/

int is_wifi_connect_successful(void)
{
    int wifi_status = 0;
    struct ip_info if_ip;

    memset(&if_ip, 0, sizeof(struct ip_info));
    wifi_status = wifi_get_status(STATION_IF);
    wifi_get_ip_info(STATION_IF, &if_ip);

    if ((wifi_status == STA_STATUS_CONNECTED) && !(ip_addr_isany_val(if_ip.ip))) {
        return 0;
    } else {
        return -1;
    }
}

/****************************************************************************
 * Name:  qiot_device_bind_set_token
 * Description:
 * @brief       ble app send message about setting token state
 * @param[in]   token: wifi token
 ****************************************************************************/

void qiot_device_bind_set_token(const char *token)
{
    ble_send_token_2_sg(token);
    ble_report_set_token_state();
}
