#include "hal_gpio.h"

int hal_gpio_init()
{
	int ret;
	ret = drv_gpio_init();
	return ret;
}

int hal_gpio_read(int gpio_num)
{
	int ret;
	ret = drv_gpio_read( gpio_num );
	return ret;
}

int hal_gpio_write(int gpio_num, int gpio_level)
{
	int ret;
	ret = drv_gpio_write( gpio_num, gpio_level);
	return ret;
}

int hal_gpio_dir_set(int gpio_num, int dir)
{
	int ret;
	ret = drv_gpio_ioctrl(gpio_num,DRV_GPIO_CTRL_SET_DIRCTION,dir);
	return ret;
}

int hal_gpio_dir_get(int gpio_num)
{
	int ret;
	ret = drv_gpio_dir_get(gpio_num);
	return ret; 
}

int hal_gpio_set_pull_mode(int gpio_num, int pull_mode)
{
	int ret;
	ret = drv_gpio_ioctrl(gpio_num,DRV_GPIO_CTRL_PULL_TYPE,pull_mode);
	return ret; 
}

int hal_gpio_intr_enable(int gpio_num, int enable)
{
	int ret;
	if(enable == 0)
	{
		ret = drv_gpio_ioctrl(gpio_num,DRV_GPIO_CTRL_INTR_DISABLE,enable);
	}
	else if(enable == 1)
	{
		ret = drv_gpio_ioctrl(gpio_num,DRV_GPIO_CTRL_INTR_ENABLE,enable);
	}
	else
	{
		return GPIO_RET_PARAMETER_ERROR;
	}
	return ret; 
}

int hal_gpio_intr_mode_set(int gpio_num, int intr_mode)
{
	int ret;
	ret = drv_gpio_ioctrl(gpio_num, DRV_GPIO_CTRL_INTR_MODE,intr_mode);
	return ret; 
}

int hal_gpio_callback_register(int gpio_num, void (* gpio_callback)(void *), void * gpio_data)
{
	int ret ;
	T_GPIO_ISR_CALLBACK p_callback;
	p_callback.gpio_callback = gpio_callback;
	p_callback.gpio_data = gpio_data;
	
	ret = drv_gpio_ioctrl(gpio_num,DRV_GPIO_CTRL_REGISTER_ISR,(int)&p_callback);

	return ret;
}

int hal_gpio_set_debounce_time(int gpio_num, int time_ns)
{
	int ret;
	ret = drv_gpio_ioctrl(gpio_num, DRV_GPIO_CTRL_SET_DEBOUNCE,time_ns);
	return ret; 
}

int hal_gpio_set_debounce_close(int gpio_num)
{
	int ret = 0;
	ret = drv_gpio_ioctrl(gpio_num, DRV_GPIO_CTRL_CLOSE_DEBOUNCE,ret);
	return ret; 
}




