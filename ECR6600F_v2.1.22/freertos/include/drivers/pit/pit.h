/**
* @file       pit.h 
* @author     Wangchao
* @date       2021-1-22
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#ifndef __PIT_H__
#define __PIT_H__


#define PIT_RET_SUCCESS			0
#define PIT_RET_ERROR			-1

/** PIT CHANNEL SETTING */
#define DRV_PIT_CHN_0			0
#define DRV_PIT_CHN_1			1
#define DRV_PIT_CHN_2			2
#define DRV_PIT_CHN_3			3
#define DRV_PIT_CHN_4			4
#define DRV_PIT_CHN_5			5
#define DRV_PIT_CHN_6			6
#define DRV_PIT_CHN_7			7
#define DRV_PIT_CHN_MAX	    	8

/** PIT CONTROL SETTING */
#define DRV_PIT_CTRL_SET_COUNT			    0
#define DRV_PIT_CTRL_GET_COUNT		        1
#define DRV_PIT_CTRL_INTR_ENABLE		    2
#define DRV_PIT_CTRL_INTR_STATUS_GET	    3
#define DRV_PIT_CTRL_INTR_STATUS_CLEAN	    4
#define DRV_PIT_CTRL_CH_MODE_ENABLE		    5
#define DRV_PIT_CTRL_CH_MODE_SET		    6
#define DRV_PIT_CTRL_CH_ALLOC			    7
#define DRV_PIT_CTRL_CH_RELEASE		    8
#define DRV_PIT_CTRL_CH_STATUS			9

#define DRV_PIT_INTR_ENABLE			     	1
#define DRV_PIT_INTR_DISABLE				0

#define DRV_PIT_CH_MODE_ENABLE_TIMER		1
#define DRV_PIT_CH_MODE_ENABLE_PWM	    	8
#define DRV_PIT_CH_MODE_ENABLE_NONE		    0

#define DRV_PIT_CH_MODE_SET_TIMER		1
#define DRV_PIT_CH_MODE_SET_PWM			4

#define DRV_PIT_TICK_CH_RELOAD	0xFFFFFFFF


/**    @brief		Handle pit different events.
*	   @details 	The events handled include pit channel reload set, get pit channel counter, enable pit channel interrupt, 
                    get pit channel interrupt status, pit channel mode set.
*	   @param[in]	channel_num    Specifies the channel number to open
*	   @param[in]	event       Control event type
*	   @param[in]	*arg    Control parameters, the specific meaning is determined according to the event
*	   @return  	0--Handle event succeed, -1--Handle event failed
*/
int drv_pit_ioctrl(unsigned int channel_num, int event, unsigned int arg);

/**    @brief		Pit0/pit1 initialization.
*	   @details 	Initialize the reset value of pit1-channel3, set the time source of Pit1 as APB clock 
                    and 32-bit timer, and start pit1-channel3 channel 
*      @return      0--Pit initial succeed, 1--Pit initial failed
*/
int drv_pit_init(void);

/**    @brief       Pit delay.
 *     @param[in]   delay   The number of delay, 10000000 is 1s.
 */
void drv_pit_delay( long delay);

unsigned int drv_pit_get_tick(void);

#endif

