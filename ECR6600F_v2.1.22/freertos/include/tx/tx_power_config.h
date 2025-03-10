#ifndef TX_POWER_CFOFIG
#define TX_POWER_CONFIG
#include <stdint.h>
#include <adc.h>
#ifndef DFE_VAR  
#define __DFE extern  
#define __ARR1(x,y,z)
#define __ARR2(a0,a1,a2,a3,a4,a5,a6,a7)
#define __ARR3(b0,b1,b2,b3,b4,b5)
#define __VAR1(x)
#else  
#define __DFE  
#define __ARR1(x,y,z) ={(x),(y),(z),}
#define __ARR2(a0,a1,a2,a3,a4,a5,a6,a7) {a0,a1,a2,a3,a4,a5,a6,a7}
#define __ARR3(b0,b1,b2,b3,b4,b5) = {b0,b1,b2,b3,b4,b5}
#define __VAR1(x) = {x}
#endif

#define MCS_NUM 8
#define CAL_INDEX 5


__DFE uint32_t pa_bias_6600[DRV_ADC_TEMP_MAX]  __ARR1(0x47FF3CE,/*low temp*/ 0x47FF3CE,/*normal temp*/ 0x47FF4CB/*high temp*/);
__DFE uint32_t pa_bias_6600a_ldo[DRV_ADC_TEMP_MAX]  __ARR1(0x47FF3D3,/*low temp*/0x47FF3CF,/*normal temp*/ 0x47FF3CC/*high temp*/);
__DFE uint32_t pa_bias_6600a_dcdc[DRV_ADC_TEMP_MAX]  __ARR1(0x47FF3D3,/*low temp*/0x47FF3CF,/*normal temp*/ 0x47FF3CC/*high temp*/);

#ifdef CONFIG_CUSTOM_YY
__DFE uint32_t g_cfo [DRV_ADC_TEMP_MAX]  __ARR1(0x155,/*low temp*/0x166,/*normal temp*/ 0x177/*high temp*/);
#else
__DFE uint32_t g_cfo [DRV_ADC_TEMP_MAX]  __ARR1(0x144,/*low temp*/0x155,/*normal temp*/ 0x1cc/*high temp*/);
#endif

__DFE uint32_t pa_bias[DRV_ADC_TEMP_MAX] ;

__DFE uint32_t fcc_var __VAR1(0);
__DFE uint32_t fcc_var_ble __VAR1(0);



//#if !defined(CONFIG_ESWIN_SDIO)
//#include "amt_interface.h"
//wifi tx power
#ifdef CONFIG_CUSTOM_YY
__DFE uint8_t g_calIndex[CAL_INDEX][MCS_NUM] __ARR3(__ARR2(0, 0, 0, 0, 0, 0, 0, 0),/* 11b mcs 1m - 11m */
													__ARR2(0, 0, 0, 0, 0, 0, 0, 0),/* 11g 6m - 54m */
													__ARR2(0, 0, 0, 0, 0, 0, 0, 0),/* 11n mcs 0 - 7 */
													__ARR2(0, 0, 0, 0, 0, 0, 0, 0),/* 11n_40M mcs 0 - 7 */
													__ARR2(0, 0, 0, 0, 0, 0, 0, 0),/* 11ax mcs 0 - 7 */
													);
#else
__DFE uint8_t g_calIndex[CAL_INDEX][MCS_NUM] __ARR3(__ARR2(0, 0, 0, 0, 0, 0, 0, 0),/* 11b mcs 1m - 11m */
													__ARR2(3, 3, 3, 2, 2, 2, 1, 1),/* 11g 6m - 54m */
													__ARR2(3, 3, 3, 2, 2, 1, 0, 0),/* 11n mcs 0 - 7 */
													__ARR2(2, 2, 2, 2, 2, 1, 0, 0),/* 11n_40M mcs 0 - 7 */
													__ARR2(3, 3, 3, 2, 2, 1, 0, 0),/* 11ax mcs 0 - 7 */
													);
#endif
//#endif

/** Format: [15:13] Var, [12:8]:Dig [7:4]:DA [3:0]:PA 
 * Var: Tx power variance between ch0 and ch39
         linear value
         How to set: 
		   P_ch0, P_ch39 are Tx power with the same gain at ch0/39 in dB 
		   Var = 10^((P_ch0 - P_ch39)/20)
 * Dig : digital gain to fine tune BLE tx power
 *       Linear value, valid range [0xC, 0x15]
 *       How to change:
 *         Dig1: new value, Dig2 old value
 *         Tx power change in dB: 20log10(Dig1/Dig2)
 *         For example, setting changed from 0xE to 0xF
 *         Tx power change: +0.6dB
 * 
 * DA and PA are RF gain settings. Customer: do NOT change
 *
 * The default level is high level when work mode.
 * customer can modify the high level as their wants power level.
 */
//__DFE uint16_t ble_tx_power_index[3] __ARR1(0x6f28,/*lower level, 6dbm*/0x6f2a, /*average level, 8dbm*/ 0x6f2d/*high level, 10dbm*/);
__DFE uint16_t ble_tx_power_index[3] __ARR1(0x6d28,/*lower level, 6dbm*/0x6f2a, /*average level, 8dbm*/ 0x6f2d/*high level, 10dbm*/);


//ble tx power

#endif
