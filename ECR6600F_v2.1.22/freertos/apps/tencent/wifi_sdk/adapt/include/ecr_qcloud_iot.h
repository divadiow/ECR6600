/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/include/ecr_qcloud_iot.h
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  FeixiongZhang<zhangfeixiong@eswin.com>
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

#ifndef __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_IOT_H
#define __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_IOT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "system_wifi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AP_STATION                  "tcloud_ecr"
#define SG_DEVICE_NAME              "ecr6600"
#define SG_DEVICE_ID                "YHVR5K7T9H"
#define SG_DEVICE_SECRET            "jQN+JwDvQ+UK490DzwUWFQ=="

#if defined(__cplusplus)
extern "C" {
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

typedef enum {
    e_SMART_LIGHT = 0,
    e_GATEWAY     = 1,
    e_RAW_DATA    = 2,
    e_MQTT        = 3,
    e_DEFAULT
} eType;

typedef enum {
    OTA_SUCCESS               = 0,
    OTA_STATE_INIT            = 1,
    OTA_NULL_FAILED           = 2,
    OTA_PRE_FAILED            = 3,
    OTA_MALLOC_FAILED         = 4,
    OTA_START_DOWNLODE_FAILED = 5,
    OTA_STOP_TRAN             = 6,
    OTA_REVEICE_TIMEOUT       = 7,
    OTA_REVEICE_FAILED        = 8,
    OTA_SAVE_DATE_FAILED      = 9,
    OTA_DISCONNECT            = 10,
    OTA_FIRMWARE_FAILED       = 11,
    OTA_POST_DATA_FAILED      = 12,
} e_Ota_Type;

//int qcloud_iot_explorer_demo(eDemoType eType);
int qcloud_iot_hub(void);
#if defined(__cplusplus)
}
#endif

/****************************************************************************
 * Types
 ****************************************************************************/

typedef struct {
    int wifi_connected;
    int app_bind_state;
} wifi_bind_u;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ecr_get_stored_wifi_config
 * Description:
 * @brief       get wifi config from NV
 * @param[out]  wifi_config: wifi_config_t struct, include ssid/password/channel/bssid[]
 * @return  0: success; 1: fail
 ****************************************************************************/

int ecr_get_stored_wifi_config(wifi_config_u *wifi_config);

/****************************************************************************
 * Name:  ecr_sntp
 * Description:
 * @brief       network time sync
 ****************************************************************************/

void ecr_sntp(void);

/****************************************************************************
 * Name:  ecr_system_reset_count
 * Description:
 * @brief       determine wifi config procedure enter AP mode or STA mode by count restart for 10s
 * @param[in]   value: count of last restart
 * @return  count of restart button for 10s
 ****************************************************************************/

int ecr_system_reset_count(char value);

/****************************************************************************
 * Name:  ecr_get_app_bind_status
 * Description:
 * @brief      get app bind status from tencent cloud
 * @param[in]   wifi_bind: wifi config status and app bind status
 * @param[in]   chanel: wifi router channel
 * @param[in]   wifi_config: configrue information of device connect to wifi router
 * @return  struct of wifi_bind, include wifi_connected and app_bind_state
 ****************************************************************************/

wifi_bind_u ecr_get_app_bind_status(wifi_bind_u wifi_bind, int chanel, wifi_config_u wifi_config);

#endif  /* __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_IOT_H */
