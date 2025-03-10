/**
*@file	   wdt.h
*@brief    This part of program is WDT algorithm by hardware
*@author   
*@data     2021.1.23
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#ifndef DRV_WDT_H
#define DRV_WDT_H

#include "chip_configuration.h"

/**Enable or disable the watchdog interrupt*/
#define WDT_CTRL_INTENABLE      1 	///< enable the watchdog interrupt
#define WDT_CTRL_INTDISABLE     0	///< disable the watchdog interrupt

/**Enable or disable the watchdog reset*/
#define WDT_CTRL_RSTENABLE      1	///< enable the watchdog reset
#define WDT_CTRL_RSTDISABLE     0	///< disable the watchdog reset

/**The time interval of the interrupt stage*/
#define WDT_CTRL_INTTIME_0		0		// 1/512 sec ~ 2ms
#define WDT_CTRL_INTTIME_1		1		// 1/128 sec ~ 8ms
#define WDT_CTRL_INTTIME_2		2		// 1/32 sec ~ 31ms
#define WDT_CTRL_INTTIME_3		3		// 1/16 sec ~ 62ms
#define WDT_CTRL_INTTIME_4		4		// 1/8 sec ~ 125ms
#define WDT_CTRL_INTTIME_5		5		// 1/4 sec ~ 250ms
#define WDT_CTRL_INTTIME_6		6		// 1/2 sec ~ 500ms
#define WDT_CTRL_INTTIME_7		7		// 1 sec
#define WDT_CTRL_INTTIME_8		8		// 4 sec
#define WDT_CTRL_INTTIME_9		9		// 16 sec
#define WDT_CTRL_INTTIME_10	    10		// 64 sec
#define WDT_CTRL_INTTIME_11	    11		// 4min 16sec  ~ 256s
#define WDT_CTRL_INTTIME_12	    12		// 17min 4sec  ~ 1024s
#define WDT_CTRL_INTTIME_13	    13		// 1hour 8min 16sec  ~ 4096s
#define WDT_CTRL_INTTIME_14	    14		// 4hour 33min 4sec  ~ 16384s
#define WDT_CTRL_INTTIME_15	    15		// 18hour 12min 16sec ~ 65536s

/**The time interval of the reset stage*/
#define WDT_CTRL_RSTTIME_0		0		// 1/256 sec
#define WDT_CTRL_RSTTIME_1		1		// 1/128 sec
#define WDT_CTRL_RSTTIME_2		2		// 1/64 sec 
#define WDT_CTRL_RSTTIME_3		3		// 1/32 sec
#define WDT_CTRL_RSTTIME_4		4		// 1/16 sec
#define WDT_CTRL_RSTTIME_5		5		// 1/8 sec
#define WDT_CTRL_RSTTIME_6		6		// 1/4 sec 
#define WDT_CTRL_RSTTIME_7		7		// 1/2 sec

typedef enum {
	WDT_RET_OK = 0,
	WDT_RET_ERR = -1,
} WDT_RET_CODE;


//#ifndef inw
//#define inw(reg)        (*((volatile unsigned int *) (reg)))
//#endif
//
//#ifndef outw
//#define outw(reg, data) ((*((volatile unsigned int *)(reg)))=(unsigned int)(data))
//#endif


void drv_wdt_isr_register(void(*callfunc)(void));

WDT_RET_CODE drv_wdt_feed(void);

/** @brief    Start the watchdog timer
*   @details  Before using the watchdog timer,
			  need to configure the register
*/
WDT_RET_CODE drv_wdt_start(int int_period, int rst_period, int int_enable, int rst_enable);

/** @brief    Stop the watchdog timer
*   @details  When the application completes the watchdog operation,
			  the watchdog device can be stopped
*/
WDT_RET_CODE drv_wdt_stop(void);

/** @brief    Watchdog timer reset chip
*   @details  When the application completes the watchdog operation,
			  the watchdog device can be stopped
*/
WDT_RET_CODE drv_wdt_chip_reset(void);

WDT_RET_CODE drv_wdt_restore(void);


#endif /* DRV_WDT_H */

