/**
* @file       pwm.h 
* @author     Wangchao
* @date       2021-1-22
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#ifndef __PWM_H__
#define __PWM_H__

#include "pit.h"

#define PWM_RET_SUCCESS		0
#define PWM_RET_ERROR			-1
#define PWM_RET_EINVAL			-2

/** PWM CHANNEL NUMBER */
#define DRV_PWM_CHN0			DRV_PIT_CHN_0
#define DRV_PWM_CHN1			DRV_PIT_CHN_1
#define DRV_PWM_CHN2			DRV_PIT_CHN_2
#define DRV_PWM_CHN3			DRV_PIT_CHN_3
#define DRV_PWM_CHN4			DRV_PIT_CHN_4
#define DRV_PWM_CHN5			DRV_PIT_CHN_5
#define DRV_PWM_MAX				6

/**    @brief		Open pwm.
*	   @details 	Open the pwm channel, config the pit channel to pwm mode.
*	   @param[in]	chn_pwm    Specifies the channel number to open 
*	   @return  	0--Open pwm succeed, -1--Open pwm failed
*/
int drv_pwm_open(unsigned int chn_pwm);

/**    @brief		Start pwm.
*	   @details 	Start the corresponding pwm channel to make pwm generate waveform according to the configured parameters.
*	   @param[in]	chn_pwm    Specifies the channel number to start
*	   @return  	0--Enable pwm succeed, -1--Enable pwm failed
*/
int drv_pwm_start(unsigned int chn_pwm);

/**    @brief		Stop pwm.
*	   @param[in]	chn_pwm    Specifies the channel number to stop
*	   @return  	0--Disable pwm succeed, -1--Disable pwm failed
*/
int drv_pwm_stop(unsigned int chn_pwm);

/**    @brief		Config pwm.
*	   @details 	The pwm channel value, frequency and duty cycle are statically configured, and the parameter settings are saved to the memory variable, 
*                       which will not be directly set to the bottom register. The pwm start interface will make the configuration here to the bottom register and take effect.
*                       If pwm has been started, use pwm update interface to make the configuration take effect immediately.
*	   @param[in]	chn_pwm    Specifies the channel number to config
*	   @param[in]	freq    Desired pwm frequency
*	   @param[in]	duty_ratio   The ratio of pulse occupancy time to total time in a cycle
*	   @return  	0--Config reload values succeed, -1--Config reload values failed
*/
int drv_pwm_config(unsigned int chn_pwm, unsigned int freq, unsigned int duty_ratio);

/**    @brief		Close pwm.
*	   @details 	Close the pwm channel and release the pit channel.
*	   @param[in]	chn_pwm  Specifies the channel number to close 
*/
int drv_pwm_close(unsigned int chn_pwm);

/**    @brief		pwm update.
*	   @details 	Make the static configuration of pwm equipment take effect immediately. If the pwm device has been started,  the parameters of the static configuration 
*                       will take effect immediately without restarting the pwm device. If the pwm device is not started, the configuration wil not take effect until the pwm is started.
*	   @param[in]	chn_pwm    Specifies the channel number to update
*	   @return  	0--Update pwm succeed, -1--Update pwm failed
*/
int drv_pwm_update(unsigned int chn_pwm);


#define hal_pwm_init(pwm_ch)		drv_pwm_open(pwm_ch)
#define hal_pwm_config(pwm_ch, freq, ratio)		drv_pwm_config(pwm_ch, freq, ratio)
#define hal_pwm_start(pwm_ch)		drv_pwm_start(pwm_ch)
#define hal_pwm_stop(pwm_ch)		drv_pwm_stop(pwm_ch)
#define hal_pwm_close(pwm_ch)		drv_pwm_close(pwm_ch)
#define hal_pwm_update(pwm_ch)		drv_pwm_update(pwm_ch)


#endif

