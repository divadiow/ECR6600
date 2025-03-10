#include <gpio/gpio.h>
#include <chip_smupd.h>
#include <chip_pinmux.h>
#include "joylink_extern.h"
#include "joylink_extern_led.h"
#include "pwm.h"
#include <stdlib.h>
#include "joylink_flash.h"

  unsigned int ch1 = 1;
  unsigned int ch2 = 2;
  unsigned int ch3 = 3;
void joylink_dev_led_init()
{
  chip_pwm_pinmux_cfg(PWM_CH1_USED_GPIO23);
  chip_pwm_pinmux_cfg(PWM_CH2_USED_GPIO24);
  chip_pwm_pinmux_cfg(PWM_CH3_USED_GPIO25);
  drv_pwm_open(ch1);
  drv_pwm_open(ch2);
  drv_pwm_open(ch3);
    /* 使能设备 */
  drv_pwm_start(ch1);
  drv_pwm_start(ch2);
  drv_pwm_start(ch3);
}
int joylink_dev_led_ctrl(user_dev_status_t *userDev)
{
  unsigned int ratio_bright = userDev->BrightLED;
  unsigned int freq = 1000;
  unsigned int rgb_red;
  unsigned int rgb_green;
  unsigned int rgb_blue;
  unsigned int ratio_red;
  unsigned int ratio_green;
  unsigned int ratio_blue;
#if defined(CONFIG_JD_FREERTOS_PAL)
  //若LED开关打开
  if(userDev->Power)
  {
    //灯色判断,设置RGB强度
    switch (userDev->Color)
    {
    case LED_RED:
      rgb_red = 255; 
      rgb_green = 0;
      rgb_blue = 0;
      break;

    case LED_ORANGE:
      rgb_red = 255; 
      rgb_green = 165;
      rgb_blue = 0;
      break;

    case LED_BLUE:
      rgb_red = 0; 
      rgb_green = 0;
      rgb_blue = 255;
      break;

    case LED_PURPLE:
      rgb_red = 160; 
      rgb_green = 32;
      rgb_blue = 240;
      break;

    case LED_GREEN:
      rgb_red = 0; 
      rgb_green = 255;
      rgb_blue = 0;
      break;

    case LED_YELLOW:
      rgb_red = 255; 
      rgb_green = 255;
      rgb_blue = 0;
      break;

    case LED_SYAN:
      rgb_red = 0; 
      rgb_green = 255;
      rgb_blue = 255;
      break;

    case LED_WARN_WHITE:
      rgb_red = 255; 
      rgb_green = 222;
      rgb_blue = 173;
      break;

    case LED_COLD_WHITE:
      rgb_red = 200; 
      rgb_green = 180;
      rgb_blue = 255;
      break;

    default:
      rgb_red = 255; //pink
      rgb_green = 20;
      rgb_blue = 147;
      break;
    }
    ratio_red = abs(rgb_red * 1000 * ratio_bright/255/100 - 1);
    ratio_green = abs(rgb_green * 1000 * ratio_bright/255/100 - 1);
    ratio_blue = abs(rgb_blue * 1000 * ratio_bright/255/100 - 1);
  }

  //关LED
  else
  {
    ratio_red = 0;
    ratio_green = 0;
    ratio_blue = 0;
  }
  jl_platform_printf("===============ratio_red = %d,ratio_green = %d,ratio_blue = %d\n",ratio_red,ratio_green,ratio_blue);

  //设置pwm占空比值
  int ret_red = drv_pwm_config(ch1, freq, ratio_red);
  if (ret_red != 0)
  {
    jl_platform_printf("RED_LED color control fail:config led\n");
    return -1;
  }
  int ret_green = drv_pwm_config(ch2, freq, ratio_green);
  if (ret_green != 0)
  {
    jl_platform_printf("GREEN_LED color control fail:config led\n");
    return -1;
  }
  int ret_blue = drv_pwm_config(ch3, freq, ratio_blue);
  if (ret_blue != 0)
  {
    jl_platform_printf("BLUE_LED color control fail:config led\n");
    return -1;
  }
  return 0;
#endif
}
