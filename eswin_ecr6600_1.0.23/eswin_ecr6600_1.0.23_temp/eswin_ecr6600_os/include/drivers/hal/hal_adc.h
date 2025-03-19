#ifndef HAL_ADC_H
#define HAL_ADC_H

#include "adc.h"

void hal_adc_init(void);
int hal_adc_config(DRV_ADC_INPUT_SIGNAL_A_SEL signal_a,DRV_ADC_INPUT_SIGNAL_B_SEL signal_b,DRV_ADC_EXPECT_VOLT volt);
int hal_adc_single();
int hal_adc_diff();
int hal_adc_dynamic_diff( int* buff ,int len);
int hal_adc_vbat();
int hal_adc_tempsensor();
int hal_adc_get_single(DRV_ADC_INPUT_SIGNAL_A_SEL signal_a,DRV_ADC_INPUT_SIGNAL_B_SEL signal_b,DRV_ADC_EXPECT_VOLT volt);
int hal_adc_get_diff(DRV_ADC_INPUT_SIGNAL_A_SEL signal_a,DRV_ADC_INPUT_SIGNAL_B_SEL signal_b,DRV_ADC_EXPECT_VOLT volt);


#endif /*HAL_ADC_H*/


