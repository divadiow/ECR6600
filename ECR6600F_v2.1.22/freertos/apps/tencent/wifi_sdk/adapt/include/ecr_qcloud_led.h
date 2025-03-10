/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/include/ecr_qcloud_led.h
 *
 *   Copyright (C) 2021 ESWIN. All rights reserved.
 *   Author:  FeixiongZhang<zhangfeixiong@eswin.com>
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

#ifndef __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_LED_H
#define __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_LED_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "qcloud_iot_export.h"
#include "data_template_client.h"

#if defined(__cplusplus)
extern "C" {
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_STR_NAME_LEN1     (64)

/****************************************************************************
 * Types
 ****************************************************************************/

typedef struct _ProductDataDefine {
    TYPE_DEF_TEMPLATE_BOOL   m_light_switch;
    TYPE_DEF_TEMPLATE_ENUM   m_color;
    TYPE_DEF_TEMPLATE_INT    m_brightness;
    TYPE_DEF_TEMPLATE_STRING m_name[MAX_STR_NAME_LEN1 + 1];
} ProductDataDefine;

typedef enum {
    eCOLOR_RED   = 0,
    eCOLOR_GREEN = 1,
    eCOLOR_BLUE  = 2,
} eColor;

extern DeviceInfo sg_devInfo;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  _setup_connect_init_params
 * Description:
 * @brief       setup MQTT construct parameters
 * @param[out]   initParams: save product information to "initParams"
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int _setup_connect_init_params(TemplateInitParams *initParams);

/****************************************************************************
 * Name:  ecr_qcloud_led_ctrl
 * Description:
 * @brief       main led control cycle
 * @param[in]   client: data template client
 * @return  0 when success, or err code for failure
 ****************************************************************************/

int ecr_qcloud_led_ctrl(Qcloud_IoT_Template *client);

/****************************************************************************
 * Name:  ecr_get_led_info
 * Description:
 * @brief       get led status of product, and save it to "sg_ProductData"
 * @param[out]   sg_ProductData: get product information from NV to "sg_ProductData"
 ****************************************************************************/

void ecr_get_led_info(ProductDataDefine *sg_ProductData);

#if defined(__cplusplus)
}
#endif
#endif  /* __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_LED_H */

