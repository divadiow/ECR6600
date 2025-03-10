/****************************************************************************
  * apps/tencent/ble_sdk/adapter/inc/qiot_netcfg.h
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

#ifndef __APP_TENCENT_BLE_SDK_ADAPTER_INC_QIOT_NETCFG_H
#define __APP_TENCENT_BLE_SDK_ADAPTER_INC_QIOT_NETCFG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "event_groups.h"
#include "bluetooth.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define EVENTBIT_0 (1<<0) /* 事件位 ble 配网使能 */
#define EVENTBIT_1 (1<<1) /* ble 获得ssid，passwd，并联网 */

/****************************************************************************
 * Types
 ****************************************************************************/

void qiot_start_advertising(void);

EventGroupHandle_t EventGroupHandler; /* 事件标志组句柄 */

#endif /* __APP_TENCENT_BLE_SDK_ADAPTER_INC_QIOT_NETCFG_H */

