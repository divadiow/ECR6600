#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "gpio.h"

int hal_gpio_init();
int hal_gpio_read(int gpio_num);
int hal_gpio_write(int gpio_num, int gpio_level);
//int hal_gpio_get_output_value(int gpio_num);
int hal_gpio_dir_set(int gpio_num, int dir);
int hal_gpio_dir_get(int gpio_num);
int hal_gpio_set_pull_mode(int gpio_num, int pull_mode);
int hal_gpio_intr_enable(int gpio_num, int enbale);
int hal_gpio_intr_mode_set(int gpio_num, int intr_mode);
int hal_gpio_callback_register(int gpio_num, void (* gpio_callback)(void *), void * gpio_data);
int hal_gpio_set_debounce_time(int gpio_num, int time_ns);
int hal_gpio_set_debounce_close(int gpio_num);

#endif /*HAL_GPIO_H*/


