#include "oshal.h"
#include "hal_pwm.h"


#if 0

int hal_pwm_init(unsigned int pwm_ch)
{    
	return drv_pwm_open(pwm_ch);
}



int hal_pwm_start(unsigned int pwm_ch)
{
	return drv_pwm_start(pwm_ch);
}


int hal_pwm_stop(unsigned int pwm_ch)
{
	return drv_pwm_stop(pwm_ch);
}


int hal_pwm_config(unsigned int pwm_ch,unsigned int freq,unsigned int ratio)
{
	return drv_pwm_config(pwm_ch, freq, ratio);    
}

int hal_pwm_close(unsigned int pwm_ch)
{
	return drv_pwm_close(pwm_ch);    
}

int hal_pwm_update(unsigned int pwm_ch)
{
	return drv_pwm_update( pwm_ch );
}

#endif
