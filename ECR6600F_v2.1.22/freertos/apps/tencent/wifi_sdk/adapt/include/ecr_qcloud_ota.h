/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/include/ecr_qcloud_ota.h
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  WenchengCao <caowencheng@eswin.com>
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

#ifndef __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_OTA_H
#define __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_OTA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "qcloud_iot_export.h"
#include "data_template_client.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define OTA_CLIENT_TASK_NAME        "ota_esp_task"
#define OTA_CLIENT_TASK_STACK_BYTES 4096
#define OTA_CLIENT_TASK_PRIO        3
#define ECR_OTA_BUF_LEN             2048
#define MAX_OTA_RETRY_CNT           3
#define MAX_SIZE_OF_FW_VERSION      32

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern bool g_ota_task_running;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

//#define SUPPORT_RESUMING_DOWNLOAD     1

/****************************************************************************
 * Name:  enable_ota_task
 * Description:
 * @brief       Start OTA background task to do firmware update
 * @param[in]   dev_info:     Qcloud IoT device info
 * @param[in]   mqtt_client:  Qcloud IoT MQTT client handle
 * @param[in]   version:      Local firmware version
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int enable_ota_task(DeviceInfo *dev_info, void *mqtt_client, char *version);

/****************************************************************************
 * Name:  disable_ota_task
 * Description:
 * @brief       Stop OTA background task
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int disable_ota_task(void);

/****************************************************************************
 * Name:  is_fw_downloading
 * Description:
 * @brief       Check if OTA task is in downloading state or not
 * @return  true when downloading, or false for not downloading
 ****************************************************************************/

bool is_fw_downloading(void);

/****************************************************************************
 * Name:  ecr_ota_update_task
 * Description:
 * @brief       main OTA cycle
 * @param[in]   pvParameters: OTA context data, it save OTA update process information
 ****************************************************************************/

extern void ecr_ota_update_task(void *pvParameters);

/****************************************************************************
 * Name:  ecr_qcloud_ota_ctrl
 * Description:
 * @brief
 * @param[in]   client_chan: data template client
 * @return  0: success, -1: failure
 ****************************************************************************/

int ecr_qcloud_ota_ctrl(Qcloud_IoT_Template *client_chan);

// extern OTAContextData sg_ota_ctx ;
#endif  /* __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_OTA_H */
