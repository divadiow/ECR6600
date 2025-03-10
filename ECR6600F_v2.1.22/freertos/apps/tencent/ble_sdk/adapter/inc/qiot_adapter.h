/****************************************************************************
  * apps/tencent/ble_sdk/adapter/inc/qiot_adapter.h
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

#ifndef __APP_TENCENT_BLE_SDK_ADAPTER_INC_QIOT_ADAPTER_H
#define __APP_TENCENT_BLE_SDK_ADAPTER_INC_QIOT_ADAPTER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "system_wifi.h"
#include "easyflash.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name:  ble_report_set_token_state
 * Description:
 * @brief      app report set token status
 ****************************************************************************/

void ble_report_set_token_state();

/****************************************************************************
 * Name:  qiot_flash_SetString
 * Description:
 * @brief       set data in qiot_flash
 * @param[in]   key: Guideposts for writing elements
 * @param[in]   value: What to write
 * @param[in]   len: the number of char elements in "value"
 * @return  0: success, -1: failure
 ****************************************************************************/

int32_t qiot_flash_SetString(const char* key, const char* value, int32_t len);

/****************************************************************************
 * Name:  qiot_flash_GetString
 * Description:
 * @brief       get data in qiot_flash
 * @param[in]   key: Guideposts for writing elements
 * @param[in]   buff: What to write
 * @param[in]   len: the number of char elements in "buff"
 * @return  length of read data
 ****************************************************************************/

int32_t qiot_flash_GetString(const char* key, char* buff, int32_t len);

/****************************************************************************
 * Name:  is_wifi_connect_successful
 * Description:
 * @brief       get status of wifi connected
 * @return  0: success, -1: failure
 ****************************************************************************/

int is_wifi_connect_successful(void);

/****************************************************************************
 * Name:  ble_send_token_2_sg
 * Description:
 * @brief       get token and set event group bit
 * @param[in]   token: token of device bind
 ****************************************************************************/
void ble_send_token_2_sg(const char *token);

/****************************************************************************
 * Name:  start_ble_netcfg_task
 * Description:
 * @brief       create ble netcfg task
 * @param[in]   flag: true or false
 ****************************************************************************/

void start_ble_netcfg_task(bool flag);

#endif /* __APP_TENCENT_BLE_SDK_ADAPTER_INC_QIOT_ADAPTER_H */
