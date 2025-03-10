/**
*@file	   adc.h
*@brief    This part of program is AUX_ADC function declaration and variable definition
*@author   wei feiting
*@data     2021.2.3
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/
#include <string.h>

//#ifndef BIT
//#define BIT(x) (1 << (x))
//#endif

#define CONFIG_ADC_HARDWARE
/**
 * @brief AUX_ADC choice of a and b input signals
 * @details The detail description.
 */
typedef enum 
{
	DRV_ADC_A_INPUT_GPIO_0 = 0x1,
	DRV_ADC_A_INPUT_GPIO_1 = 0x2,
	DRV_ADC_A_INPUT_GPIO_2 = 0x4,
	DRV_ADC_A_INPUT_GPIO_3 = 0x8,
	DRV_ADC_A_INPUT_VBAT = 0x10,
	DRV_ADC_A_INPUT_VTEMP_BG = 0x20,
	DRV_ADC_A_INPUT_VREF_BG = 0x40,
	DRV_ADC_A_INPUT_VREF_BUFFER = 0x80,
}DRV_ADC_INPUT_SIGNAL_A_SEL;

typedef enum 
{
	DRV_ADC_B_INPUT_GPIO_0 = 0x1,
	DRV_ADC_B_INPUT_GPIO_1 = 0x2,
	DRV_ADC_B_INPUT_GPIO_2 = 0x4,
	DRV_ADC_B_INPUT_GPIO_3 = 0x8,
	DRV_ADC_B_INPUT_V_TIELOW = 0x10,
	DRV_ADC_B_INPUT_VTEMP_PA = 0x20,
	DRV_ADC_B_INPUT_VREF_BG = 0x40,
	DRV_ADC_B_INPUT_VREF_BUFFER = 0x80,
}DRV_ADC_INPUT_SIGNAL_B_SEL;

/**
 * @brief AUX_ADC choice of pga gain
 * @details The detail description.
 */
typedef enum 
{
	DRV_ADC_PGA_GAIN0_MODE = 0x1,//-6dB
	DRV_ADC_PGA_GAIN1_MODE = 0x2,//0dB
	DRV_ADC_PGA_GAIN2_MODE = 0x4,//6dB
	DRV_ADC_PGA_GAIN3_MODE = 0x8,//12dB
	DRV_ADC_PGA_GAIN4_MODE = 0x10,//18dB
	DRV_ADC_INPUTL_MAX
}DRV_ADC_PGA_GAIN_MODE;

typedef enum 
{
	DRV_ADC_EXPECT_MAX_VOLT0 = 900,
	DRV_ADC_EXPECT_MAX_VOLT1 = 1800,
	DRV_ADC_EXPECT_MAX_VOLT2 = 2400,
	DRV_ADC_EXPECT_MAX_VOLT3 = 3600,
}DRV_ADC_EXPECT_VOLT;

typedef enum 
{
	DRV_ADC_TEMP_LOW,
	DRV_ADC_TEMP_NORMAL,
	DRV_ADC_TEMP_HIGH,

	DRV_ADC_TEMP_MAX
}DRV_ADC_TEMP_TYPE;
//#define ABS(x)      (((x) > 0) ? (x) : (0 - (x)))
//#ifndef REG
//#ifndef READ_REG
//#define READ_REG(offset)        (*(volatile unsigned int*)(offset))
//#endif
//#ifndef WRITE_REG
//#define WRITE_REG(offset,value) (*(volatile unsigned int*)(offset) = (unsigned int)(value));
//#endif
//#endif

/**
@brief	AUX ADC supports the self-calibration process. 
Since more intermediate variables need to be stored during the self-calibration process,
it is designed to store the intermediate variables in RAM without relying on external memory. 
In this case, each power failure will cause the loss of intermediate variables, 
and the calibration process must be executed again. Therefore, after each power-on, 
and before calling AUX ADC for the first time, a self-calibration process is performed. 
The self-calibration process takes less than 500us
*/
void drv_adc_init(void);

/**
@brief	calculate the calibration factor.
@param[in]  VERR: absolute error measurement
@param[in]  divider_a: the partial pressure coefficient of input_a
@param[in]  divider_b: the partial pressure coefficient of input_b
@param[in]  gain_mode: gain value
*/
void gain_cal_vos_data_fix_interface(unsigned int signal_a, int VERR_1,unsigned int divider_a,unsigned int divider_b,unsigned int gain_mode);

/**
@brief	aux_adc configuration
@param[in]  signal_a: signal selection of input_a
@param[in]  signal_b: signal selection of input_b
@param[in]  volt: The maximum value of the current test voltage range
*/
int drv_aux_adc_config(DRV_ADC_INPUT_SIGNAL_A_SEL signal_a,DRV_ADC_INPUT_SIGNAL_B_SEL signal_b,DRV_ADC_EXPECT_VOLT volt);
/**
@brief	get_volt_calibrate_data,the data between -127 ~ +127，。
@return	value: 0P8V_Bandgap_offset_ADC[7..0]
		-255: efuse parameter error
		-256: no calibrate value In efuse
		0: GET VERR value.
*/
int get_volt_calibrate_data(void);

/**
@brief		Static differential test interface function
@return		vout: Differential test output voltage value
*/
int drv_aux_adc_diff();

/**
@brief	Static single-ended test function
@return	vout: single-ended test output voltage value
*/
int drv_aux_adc_single();

/**
@brief	Dynamic test interface function,
		note :the voltage value of dynamic test needs to add 800mv to be the real voltage value
*/
int drv_aux_adc_dynamic(int* bufter ,int length);

/**
@brief	calculate battery voltage
*/
 int vbat_sensor_get();

 /**
 @brief  tempsensor for PA measures the local temperature of the PA on the chip
 @details  When calibrating, only need to call AUX ADC for measurement at room temperature 27℃, and save 8bits data in EFUSE.
			 As a function of voltage-temperature : Temp*0.002 + X =ts_data_pa*0.003125 ,Where ts_data_pa is the voltage value 
			 corresponding to 27°. X represents the voltage value corresponding to the current temperature.Temp represents 
			 the current temperature
 */
int temprature_sensor_get_pa(void);

 /**
@brief	It measures the local temperature of Bandgap on the chip. The voltage-temperature (V-T) curve shown 
		by this result needs to be determined by two points.
@details  During calibration, it is necessary to call AUX ADC for measurement at room temperature 27°C 
			and high temperature 80°C and save 2x8bits data in EFUSE. 
			According to Temp*K + =ts_data_pa*0.003125 to get the current temperature value.
*/
int temprature_sensor_get_bg();

 /**
@brief		tempsensor and vbat share an interface
@param[in]  mode_flag: distinguish tempsensor in pa、tempsensor in bg and vbat.
*/
 int vbat_temp_share_test(unsigned int mode_flag);
 //void show_adc_dynamic_data();
 int hal_aux_adc_hardware_test(unsigned int signal_a,unsigned int signal_b,unsigned int dividea_enable,unsigned int divideb_enable,unsigned int divider_a,unsigned int divider_b,unsigned int gain_mode,unsigned int mode_flag);
 int get_current_temprature();
 int get_25_tempsensor_to_volt();
 DRV_ADC_TEMP_TYPE hal_adc_get_temp_type();
 void time_check_temp();

 int drv_aux_adc_dynamic_init();
 int drv_adc_lock(void);
 int drv_adc_unlock(void); void psm_drv_adc_init(void);
