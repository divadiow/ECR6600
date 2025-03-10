#ifndef HAL_WDT_H
#define HAL_WDT_H

#include "wdt.h"



WDT_RET_CODE hal_wdt_init(const unsigned int time, unsigned int *effective_time);
WDT_RET_CODE hal_wdt_start();
WDT_RET_CODE hal_wdt_stop();
WDT_RET_CODE hal_wdt_feed();

#endif /*HAL_WDT_H*/


