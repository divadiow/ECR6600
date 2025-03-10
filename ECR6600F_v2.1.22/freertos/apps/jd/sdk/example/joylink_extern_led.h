#ifndef _JOYLINK_LED_H_
#define _JOYLINK_LED_H_

#define LED_RED 0
#define LED_ORANGE 1
#define LED_BLUE 2
#define LED_PURPLE 3
#define LED_GREEN 4
#define LED_YELLOW 5
#define LED_SYAN 6
#define LED_WARN_WHITE 7
#define LED_COLD_WHITE 8
void joylink_dev_led_init();
int joylink_dev_led_ctrl(user_dev_status_t *userDev);
#endif