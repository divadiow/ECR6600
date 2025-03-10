/**
* @file       rtc.h 
* @author     Houjie
* @date       2021-2-8
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#ifndef __RTC_H__
#define __RTC_H__

#if 0
#include "arch_irq.h"
#include "chip_memmap.h"
#include "chip_irqvector.h"
#endif

#ifndef void_fn
typedef void (*void_fn)(void);
#endif

/** unit convert */
#define DAY_TO_HOUR	24
#define HOUR_TO_MIN	60
#define MIN_TO_SEC	60
#define SEC_TO_32K	32768
#define SEC_TO_MS	1000
#define ALARM_CLK					32768
#define RTC_32k_CNT_MS_MASK(X)	((X) & 0x7FFF)		
#define MS_TO_ALARM_CNT(X)	(((X) * ALARM_CLK) / 1000)
#define US_TO_ALARM_CNT(X)	(((X) * ALARM_CLK) / 1000000) //maybe <<15 is more effective than * 32768
#define ALARM_CNT_TO_US(X)	(((X) * 1000000) / ALARM_CLK)


/** RTC TYPE  used in rtc isr register or unregister*/
typedef enum _RTC_TYPE{
    DRV_RTC_NORMAL=0,
    DRV_RTC_ALARM,
    DRV_RESERVER0,
    DRV_RTC_DAY=3,
    DRV_RTC_HOUR,
    DRV_RTC_MIN,
    DRV_RTC_SEC,
    DRV_RTC_HSEC,
    DRV_RTC_ISR_MAX,
}RTC_TYPE;

/**
 * @brief RTC time structure.
 */
struct rtc_time
{
	unsigned int day;
	unsigned int hour;
	unsigned int min;
	unsigned int sec;
	unsigned int cnt_32k;
};

/**    @brief		initial rtc .
*	   @return  	0--initial succeed, -1--initial failed
*/
int drv_rtc_init();

/**    @brief		register interrupt .
*	   @param[in]	type    Rtc interrupt type 
*	   @param[in]	cb    	isr callback function 
*	   @return  	0--Register succeed, -1--Register failed
*/
int drv_rtc_isr_register(RTC_TYPE type,void_fn cb);

/**    @brief		Interrupt unregister.
*	   @param[in]	type    Rtc interrupt type 
*	   @return  	0--Unregister succeed, -1--Unregister failed
*/
int drv_rtc_isr_unregister(RTC_TYPE type);

/**    @brief		Get rtc time.
*	   @param[in]	*time    rtc_time structure pointer 
*	   @return  	0--Unregister succeed, -1--Unregister failed
*/
int drv_rtc_get_time(struct rtc_time *time);

/**    @brief		Set rtc time. note:cnt_32k is readonly.when time set,cnt_32k is automatically set to zero
*	   @param[in]	*time    rtc_time structure pointer 
*	   @return  	0--Set time succeed, -1--Set time failed
*/
int drv_rtc_set_time(struct rtc_time *time);

/**    @brief		Set rtc alarm. note:Only hour,minutes,seconds and cnt_32K are supported for configuring alarm clocks,day is unsupported
*	   @param[in]	*time    rtc_time structure pointer 
*	   @return  	0--Set alarm succeed, -1--Set alarm failed
*/
int drv_rtc_set_alarm(struct rtc_time *time);

/**    @brief		Get rtc alarm time. note:get alarm clock about hour,minutes,seconds and cnt_32K.
*	   @param[in]	*time    rtc_time structure pointer 
*	   @return  	0--Get alarm time succeed, -1--Get alarm time failed
*/
int drv_rtc_get_alarm(struct rtc_time *time);
unsigned int drv_rtc_get_32K_cnt(void);
unsigned int drv_rtc_set_alarm_relative(unsigned int value);
int drv_rtc_get_alarm_cnt(void);
unsigned int drv_rtc_get_interval_cnt(unsigned       int pre_rtc,unsigned int now_rtc);
void calculate_rtc_task();
#endif

