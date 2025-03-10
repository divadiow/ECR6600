/****************************************************************************
  * apps/tencent/wifi_sdk/adapt/src/ecr_qcloud_led_driver.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <gpio/gpio.h>
#include <chip_pinmux.h>
#include "ecr_qcloud_led_driver.h"
#include "pwm.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

unsigned int ch1 = 1;
unsigned int ch2 = 2;
unsigned int ch3 = 3;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  ecr_led_diver_init
 * Description:
 * @brief       gpio pin multiplex, open pwm and enable pwm channel
 ****************************************************************************/

void ecr_led_diver_init()
{
    chip_pwm_pinmux_cfg(PWM_CH1_USED_GPIO23); /* LED_RED */
    chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO24); /* LED_GREEN */
    chip_pwm_pinmux_cfg(PWM_CH3_USED_GPIO25); /* LED_BLUE */
    drv_pwm_open(ch1);
    drv_pwm_open(ch2);
    drv_pwm_open(ch3);
    drv_pwm_start(ch1);                       /* enable pwm */
    drv_pwm_start(ch2);
    drv_pwm_start(ch3);
}

/****************************************************************************
 * Name:  ecr_led_diver_ctrl
 * Description:
 * @brief       pwm hardware control
 * @param[in]   userDev: product data define
 * @return  0: success, -1: failure
 ****************************************************************************/

int ecr_led_diver_ctrl(ProductDataDefine *userDev)
{
    unsigned int ratio_bright = userDev->m_brightness;
    unsigned int freq = 1000;
    unsigned int rgb_red;
    unsigned int rgb_green;
    unsigned int rgb_blue;
    unsigned int ratio_red;
    unsigned int ratio_green;
    unsigned int ratio_blue;

    /* if open LED switch */
    if(userDev->m_light_switch)
    {
        /* light color judgment, and set RGB value */
        switch (userDev->m_color)
        {
            case LED_RED:
                rgb_red = 255;
                rgb_green = 0;
                rgb_blue = 0;
                break;

            case LED_GREEN:
                rgb_red = 0;
                rgb_green = 255;
                rgb_blue = 0;
                break;

            case LED_BLUE:
                  rgb_red = 0;
                  rgb_green = 0;
                  rgb_blue = 255;
                  break;

            default:
                  rgb_red = 255;  /* pink */
                  rgb_green = 20;
                  rgb_blue = 147;
                  break;
        }
        ratio_red = abs(rgb_red * 1000 * ratio_bright/255/100 - 1);
        ratio_green = abs(rgb_green * 1000 * ratio_bright/255/100 - 1);
        ratio_blue = abs(rgb_blue * 1000 * ratio_bright/255/100 - 1);
    }
    else  /* close LED */
    {
        ratio_red = 0;
        ratio_green = 0;
        ratio_blue = 0;
    }
    HAL_Printf("===============ratio_red = %d,ratio_green = %d,ratio_blue = %d\n",ratio_red,ratio_green,ratio_blue);

    /* set pwm duty ratio */
    int ret_red = drv_pwm_config(ch1, freq, ratio_red);
    if (ret_red != 0)
    {
        Log_e("RED_LED color control fail:config led\n");
        return -1;
    }
    int ret_green = drv_pwm_config(ch2, freq, ratio_green);
    if (ret_green != 0)
    {
        Log_e("GREEN_LED color control fail:config led\n");
        return -1;
    }
    int ret_blue = drv_pwm_config(ch3, freq, ratio_blue);
    if (ret_blue != 0)
    {
        Log_e("BLUE_LED color control fail:config led\n");
        return -1;
    }
    return 0;
}
