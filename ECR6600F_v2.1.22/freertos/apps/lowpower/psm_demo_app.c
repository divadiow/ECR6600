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
#include "psm_system.h"
#include "chip_pinmux.h"

signed int psm_wakeup_func_cb(void)
{
	signed int ret = true;
	/*for example hold gpio1 keep output when sleep*/
	if(!psm_hold_gpio_intf(GPIO_NUM_1,PSM_SLEEP_OUT,DRV_GPIO_ARG_DIR_OUT))
	{
		ret = -1;
	}
	/*for example hold gpio0 keep input when sleep*/
	if(!psm_hold_gpio_intf(GPIO_NUM_0,PSM_SLEEP_OUT,DRV_GPIO_ARG_DIR_IN))
	{
		ret = -1;
	}
	return ret;
}

signed int psm_sleep_func_cb(void)
{
	signed int ret = true;
	/*for example hold gpio1 keep output when sleep*/
	if(!psm_hold_gpio_intf(GPIO_NUM_1,PSM_SLEEP_IN,DRV_GPIO_ARG_DIR_OUT))
	{
		ret = -1;
	}
	/*for example hold gpio0 keep input when sleep*/
	if(!psm_hold_gpio_intf(GPIO_NUM_0,PSM_SLEEP_IN,DRV_GPIO_ARG_DIR_IN))
	{
		ret = -1;
	}
	return ret;	
}

void psm_demo_main()
{
	/*base module init*/
	//
	//
	
	/**    @brief		init callback func for sleep/wakeup restore
	*	   @param[in]	psm_stat  when into sleep param:PSM_SLEEP_IN,wakeup:PSM_SLEEP_OUT
	*	   @param[in]	psm_func_cb psm_cb	  callback type: typedef signed int (*psm_func_cb)(void);
	*					and callback realize can reference func psm_wakeup_func_cb and psm_sleep_func_cb
	*					(this etc for hold gpio stats when sleep)
	*	   @return		false/true.
	*/
	psm_proc_init_cb(PSM_SLEEP_IN,psm_sleep_func_cb);
	psm_proc_init_cb(PSM_SLEEP_OUT,psm_wakeup_func_cb);

}

