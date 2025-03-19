#ifndef __HAL_SYSTEM_H__
#define __HAL_SYSTEM_H__



typedef enum {
    RST_TYPE_POWER_ON = 0,
    RST_TYPE_FATAL_EXCEPTION, 
    RST_TYPE_SOFTWARE_REBOOT,
    RST_TYPE_HARDWARE_REBOOT,
    RST_TYPE_OTA,
    RST_TYPE_WAKEUP,
    RST_TYPE_SOFTWARE, // start from app, GPIO holdon
    RST_TYPE_HARDWARE_WDT_TIMEOUT, //reserve
    RST_TYPE_SOFTWARE_WDT_TIMEOUT, //reserve
	RST_TYPE_UNKOWN // Possible reasons: 1. PC pointer 0 address jump
} RST_TYPE;


void hal_system_reset_key_enable();
void hal_system_reset_key_disable();
void hal_reset_type_init();
void hal_set_reset_type(RST_TYPE type);

void hal_system_reset(RST_TYPE type);
RST_TYPE hal_get_reset_type();
void func_after_irq();
void func_before_restore();
//int hal_system_set_mac(unsigned char *mac);
//int hal_system_get_mac(unsigned char type, unsigned char *mac);

#endif//__HAL_SYSTEM_H__