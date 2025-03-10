/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/include/ecr_qcloud_led_driver.h
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

#ifndef __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_LED_DRIVER_H
#define __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_LED_DRIVER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ecr_qcloud_led.h"

#if defined(__cplusplus)
extern "C" {
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LED_RED 0
#define LED_GREEN 1
#define LED_BLUE 2

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ecr_led_diver_init
 * Description:
 * @brief       pwm init
 ****************************************************************************/

void ecr_led_diver_init();

/****************************************************************************
 * Name:  ecr_led_diver_ctrl
 * Description:
 * @brief       pwm control rgb led
 * @param[in]   userDev: struct of product data info
 * @return  0: success; 1: fail
 ****************************************************************************/

int ecr_led_diver_ctrl(ProductDataDefine *userDev);

#if defined(__cplusplus)
}
#endif
#endif  /* __APPS_TENCENT_WIFI_SDK_ADAPT_INCLUDE_ECR_QCLOUD_LED_DRIVER_H */
