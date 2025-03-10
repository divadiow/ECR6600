#include "hal_adc.h"

void hal_adc_init(void)
{
	drv_adc_init(); 
}

int hal_adc_config(DRV_ADC_INPUT_SIGNAL_A_SEL signal_a,DRV_ADC_INPUT_SIGNAL_B_SEL signal_b,DRV_ADC_EXPECT_VOLT volt)
{
	drv_aux_adc_config(signal_a, signal_b, volt);
	return 0;
}

int hal_adc_single()
{
	int vout;
	vout = drv_aux_adc_single();
	return vout;
}

int hal_adc_diff()
{
	int vout;
	vout = drv_aux_adc_diff();
	return vout;
}

int hal_adc_dynamic_diff( int* buff ,int len)
{
	drv_aux_adc_dynamic((int *) buff , len);
	return 0;
}

int hal_adc_vbat()
{
	int vout;
    drv_adc_lock();
	vout = vbat_temp_share_test(0);
    drv_adc_unlock();
	return vout;
}

int hal_adc_tempsensor()
{
	int temp;
	temp = get_current_temprature();
	return temp;
}

int hal_adc_get_single(DRV_ADC_INPUT_SIGNAL_A_SEL signal_a,DRV_ADC_INPUT_SIGNAL_B_SEL signal_b,DRV_ADC_EXPECT_VOLT volt)
{
	int vout;
    drv_adc_lock();
    hal_adc_config(signal_a, signal_b, volt);
	vout = drv_aux_adc_single();
    drv_adc_unlock();
	return vout;
}

int hal_adc_get_diff(DRV_ADC_INPUT_SIGNAL_A_SEL signal_a,DRV_ADC_INPUT_SIGNAL_B_SEL signal_b,DRV_ADC_EXPECT_VOLT volt)
{
    int vout;
    drv_adc_lock();
    hal_adc_config(signal_a, signal_b, volt);
    vout = drv_aux_adc_diff();
    drv_adc_unlock();
    return vout;
}

