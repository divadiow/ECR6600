/*******************************************************************************
 * Copyright by eswin 
 *
 * File Name:  
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       v0.1
 * Author:        
 * Date:          
 * History 0:      
 *     Date: 
 *     Version:
 *     Author: 
 *     Modification:  
  ********************************************************************************/

#ifndef __PSM_USER_H__
#define __PSM_USER_H__

#include "psm_mode_ctrl.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include <stdlib.h>

#include "task.h"

void psm_set_lowpower(void);
void psm_set_normal(void);
bool psm_wakeup_gpio_conf(unsigned int gpio_n, unsigned int enable);
bool psm_set_deep_sleep(unsigned int sleep_time);
bool psm_set_sleep_mode(unsigned int sleep_mode, unsigned char listen_interval);
bool psm_switch_rf_state(bool rf_state);
#endif
