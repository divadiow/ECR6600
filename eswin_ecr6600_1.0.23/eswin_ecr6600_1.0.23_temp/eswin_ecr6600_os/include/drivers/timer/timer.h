/**
* @file       timer.h 
* @author     Wangchao
* @date       2021-1-22
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#ifndef __DRV_TIMER_H__
#define __DRV_TIMER_H__

#include "pit.h"

#define TIMER_RET_SUCCESS			0
#define TIMER_RET_ERROR			-1
#define TIMER_RET_EINVAL			-2

/** PIT TIMER NUMBER */
#define DRV_TIMER0			DRV_PIT_CHN_0
#define DRV_TIMER1			DRV_PIT_CHN_1
#define DRV_TIMER2			DRV_PIT_CHN_2
#define DRV_TIMER3			DRV_PIT_CHN_3
#define DRV_TIMER_MAX		4

#define DRV_TIMER_CTRL_START	 				0
#define DRV_TIMER_CTRL_STOP	 				1
#define DRV_TIMER_CTRL_SET_MODE	 			2
#define DRV_TIMER_CTRL_SET_PERIOD		 		3	// us
#define DRV_TIMER_CTRL_SET_FREQ		 		4
#define DRV_TIMER_CTRL_REG_CALLBACK	 		5
#define DRV_TIMER_CALCULATE_CURRRET_TIME	6


#define DRV_TIMER_CTRL_MODE_ONESHOT	 		0
#define DRV_TIMER_CTRL_MODE_PERIOD	 		1

/**
 * @brief Timer interrupt callback.
 */
typedef struct _T_TIMER_ISR_CALLBACK
{
	void (* timer_callback)(void *);
	void *timer_data;

}T_TIMER_ISR_CALLBACK;

/**    @brief		Handle pit timer different events.
*	   @details 	The events handled include enable pit timer, disable pit timer, enable pit channel interrupt, 
                    config timer interrupt mode, set pit timer period and frequency, config interrupt callback function.
*	   @param[in]	chn_timer    Specifies the channel number to open
*	   @param[in]	event       Control event type
*	   @param[in]	* arg    Pointer to control parameters, the specific meaning is determined according to the event
*	   @return  	ret=0--Handle event succeed, ret=other--Handle event failed
*/
int drv_timer_ioctrl(unsigned int chn_timer, int event, void * arg);

/**    @brief		Open pit timer.
*	   @details 	Open and register the related interrupt of pit timer.
*	   @param[in]	chn_timer    Specifies the channel number to open 
*/
int drv_timer_open(unsigned int chn_timer);

/**    @brief		Close pit timer.
*	   @details 	Disable pit timer channel and pit timer interrupt.
*	   @param[in]	chn_timer    Specifies the channel number to open 
*/
int drv_timer_close(unsigned int chn_timer);



#endif

