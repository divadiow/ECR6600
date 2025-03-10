
/**
*@file	   adc.c
*@brief    This part of program is AUX_ADC algorithm Software interface
*@author   weifeiting
*@data     2021.2.3
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#include <stdio.h>
#include "chip_memmap.h"
#include "adc.h"
#include "chip_pinmux.h"
#include "efuse.h"
#include "chip_smupd.h"
#include "oshal.h"
#include "arch_irq.h"
#include "chip_irqvector.h"
#include "chip_clk_ctrl.h"
#include "string.h"
#include "hal_timer.h"
#include "FreeRTOS.h"
#include "timers.h"


/** Define the registers required for aux_adc */
#define AUX_ADC_EN				        CHIP_SMU_PD_AUX_ADC_SEL		///< :aux_adc enable 
#define AUX_ADC_FIFO_WR_CONFIG			CHIP_SMU_PD_AUX_ADC_FIFO_WR_CFG	 ///< :fifo write data configuration signal, control down-sampling rate (re-sampling 2~512 times)
#define AUX_ADC_0						(MEM_BASE_RFC_REG + 0x7C)	 ///< :aux_adc0 register
#define AUX_ADC_1						(MEM_BASE_RFC_REG + 0x80)	 ///< :aux_adc1 register
#define AUX_BB_CLK_GEN					(MEM_BASE_RFC_REG + 0x78)	 ///< :aux_bb clk register
#define AUX_ADC_DATA_OUT                CHIP_SMU_PD_AUX_ADC_OUT	 ///< :aux_adc through data output register
#define AUX_ADC_FIFO_DATA				CHIP_SMU_PD_AUX_ADC_FIFO_DATA	 ///< :aux_adc dynamic fifo output register
#define AUX_ADC_GAIN_CAL_FIX            CHIP_SMU_PD_AUX_GAIN_CAL_FIX	 ///< :aux_adc gain cal fix register
#define AUX_ADC_VOS_DATA_FIX            CHIP_SMU_PD_AUX_VOS_DATA_FIX	 ///< :aux_adc vos data fix register
#define AUX_ADC_CLR           			CHIP_SMU_PD_AUX_ADC_CLR	 ///< :aux_adc intrrupt clear register
#define DFE_CTRL_REG25					0x203164	 ///< :isolate signals which is from ADC to digital power domain��?default 1,isolation
//#define TEST_MODE_EXTPAD_RST_EN			(MEM_BASE_SMU_AON + 0x14) 	 ///< :extpad rst enable
#define DIG_BGM_ADC_0P8_SEL				(MEM_BASE_PCU + 0x14) 	 ///< :aux_adc_0p8_sel，1：lpf。0：no_lpf
#define RF_CTRL_REG_37					(MEM_BASE_RFC_REG + 0x94)
#define RF_CTRL_REG_38					(MEM_BASE_RFC_REG + 0x98) 	 ///< :
#define RF_CTRL_REG_39					(MEM_BASE_RFC_REG + 0x9C) 	 ///< :
#define PAD_PULLUP						(MEM_BASE_SMU_AON + 0x18)
#define PAD_PULLDOWN					(MEM_BASE_SMU_AON + 0x1C)
#define PAD_EN							(MEM_BASE_SMU_AON + 0x20)
#define BANDGAP_OFFSET_ADC_ADDR_EFUSE 	0x0C
#define TEMPERATURE_SENSOR_ADDR_EFUSE 	0x0C

#define DRV_ADC0_ARI_AUX_ADC_EN		0x1
#define DRV_ADC0_ARI_AUX_ADC_PROTECT	0x2
#define DIG_ARI_AUX_ADC_PGA_EN          0x1
#define DIG_ARI_AUX_ADC_SAR_EN          0x1

/** Define variables required to test aux_adc */
#define LSB0  0.39
float VREF_known[5]={895,1792,1536,1024,2047};		///< :5 kinds offset voltage
unsigned int divider_test[8]={0,1,2,3,4,5,6,7};		///< :8 kinds split voltage ratio's input paramter
float input_divider[8]={0.125,0.25,0.375,0.5,0.625,0.75,0.875,1};		///< :8 kinds split voltage ratio
float VOS[5]={0.0};									///< :Vos measurement under 5 gains 
float VREF_test[5]={0.0};							///< :Calibration voltage measurement under 5 gains 
float VOS_ref=0.0;									///< :vos_ref measurement under gain4
float VOS_ref_test[5]={0.0};
unsigned int pga_mode[5]={0x1,0x2,0x4,0x8,0x10};	///< :5 kinds pga mode 
unsigned int divider[5]={0,0,4,6,6};				///< :partial pressure during self-calibration 
unsigned int adc_flag=0;							///< :distinguish the number of self-calibration 
unsigned verr=0;									///< :the residual error when performing Trim in ATE 
int dyna_vout_low = 0 ;								///< :during dynamic test, bit<13:0> of FIFO 
int dyna_vout_high = 0 ;							///< :during dynamic test, bit<29:16> of FIFO
unsigned int div_a = 0;
unsigned int div_b = 0;
// gpio18_reset = 0;
int VERR = 0;

float VOS_TEST[20]={0.0};
float VREF_TEST[20]={0.0};
float VREF_REF_TEST[20]={0.0};

#define ADC_TEMP_LOW                    0
#define ADC_TEMP_NORMAL_TEM                 80
#define ADC_TEMP_SENSOR_TO_TEMP         -4.3285
int create_timer;
TimerHandle_t xTimerTemperature; //xTimerTemperature detected
//static os_sem_handle_t temp_process;
int temp_status = 1;
int input_a = 0;
int input_b = 0;

/** Dynamically test the fifo buffer */
extern void show_adc_dynamic_data();
#define ADC_BUF_MAX_LEN (1000)

typedef struct ADC_DYNAMIC_BUF
{
	int aux_adc_buf[ADC_BUF_MAX_LEN];
	unsigned int adc_buf_idx;
}st_adc_dynamic_buf;

static st_adc_dynamic_buf * g_adc_dynamic_buf ;
#define min(x, y)               (((x) < (y))? (x) : (y))	
int dyn_adc_flag = 0;
int dynamic_data_len = 0;

static os_mutex_handle_t drv_adc_lock_mutex;


/** adc0 register defination */
typedef union 
{
	unsigned int adc_0;
	struct 
	{
		unsigned int dig_ari_aux_adc_en:1;
		unsigned int dig_ari_aux_adc_protect:1;
		unsigned int dig_ari_aux_adc_hdsw_en:1;
		unsigned int reserved3_7:5;
		unsigned int dig_ari_aux_adc_input_divider_b_en:1;
		unsigned int dig_ari_aux_adc_input_divider_range_b_sel:3;
		unsigned int dig_ari_aux_adc_input_divider_a_en:1;
		unsigned int dig_ari_aux_adc_input_divider_range_a_sel:3;
		unsigned int dig_ari_aux_adc_input_signal_b_sel:8;
		unsigned int dig_ari_aux_adc_input_signal_a_sel:8;
	}dig_adc_0;
}DRV_ADC_0 ;

/** adc1 register defination */	
typedef union 
{
		unsigned int adc_1;
		struct
		{
			unsigned int dig_ari_aux_ts_bg_en:1;
			unsigned int dig_ari_aux_ts_bg_rsel:3;
			unsigned int dig_ari_aux_ts_pa_en:1;
			unsigned int dig_ari_aux_ts_pa_rsel:3;
			unsigned int dig_ari_aux_adc_pga_en:1;
			unsigned int dig_ari_aux_adc_bypass_pga:1;
			unsigned int dig_ari_aux_adc_sar_en:1;
			unsigned int reserved11:1;
			unsigned int dig_ari_aux_adc_pga_vcm_sel:3;
			unsigned int reserved15:1;
			unsigned int dig_ari_aux_adc_pga_cap_sel:4;
			unsigned int dig_ari_aux_adc_pga_vbp_ctrl:2;
			unsigned int reserved22_23:2;
			unsigned int dig_ari_aux_adc_pga_gain_sel:5;
			unsigned int reserved29_31:3;
		}dig_adc_1;
}DRV_ADC_1;

/** adc bb clk gen register defination */	
typedef union
{
		unsigned int adc_clk_gen;
		struct
		{
			unsigned int dig_iqdac_clk_gen_sel:1;
			unsigned int dig_iqadc_clk_gen_sel:1;
			unsigned int dig_iqdac_clk_gen_sel_mode:1;
			unsigned int dig_iqadc_clk_gen_sel_mode:1;
			unsigned int dig_abbdiv_dummyldo_ctrl:4;
			unsigned int dig_iqdac_clk_gen_en_rx:1;
			unsigned int dig_iqdac_clk_gen_en_tx:1;
			unsigned int reserver4_31:22;
		}dig_adc_clk_gen;
}DRV_ADC_BB_CLK_GEN;

/**
@brief	 DFE_CTRL_REG25 is isolated power for supply of aux adc's analog and digital
		do must open it when use aux adc function.
*/
void open_adc_power_from_digital(void)
{
	WRITE_REG(DFE_CTRL_REG25,0);
	return;
}

/**
@brief	 DFE_CTRL_REG25 is isolated power for supply of aux adc's analog and digital
		do must open it when use aux adc function.
@details When the signal is high, the CPU reads the o_adc_fifo_data register (0x202084) 4 times in a row,
		 and then writes 1 to the i_aux_adc_clr register (0x202098), clears the o_aux_adc_irq signal, 
		 and completes a data read.
*/
void aux_adc_isr()
{
	unsigned int i = 0;
	int adc_data = 0;
	for(i=0;i<4;i++)
	{
		if(g_adc_dynamic_buf->adc_buf_idx < ADC_BUF_MAX_LEN)/*only for adc test,and need remove it when release version */
		{
			adc_data = READ_REG(AUX_ADC_FIFO_DATA);
			if(((adc_data & 0x2000)>>13) == 1)
			{
				dyna_vout_low = (adc_data & 0x3FFF) | 0xffffc000;

			}
			else
			{
				dyna_vout_low = (adc_data & 0x3FFF);
			}
			g_adc_dynamic_buf->aux_adc_buf[g_adc_dynamic_buf->adc_buf_idx++] = (int)(dyna_vout_low*390.6/input_divider[div_a]) ;
			if(((adc_data & 0x20000000)>>29) == 1)
			{
				dyna_vout_high = ((adc_data & 0x3FFF0000) >>16)| 0xffffc000;

			}
			else
			{
				dyna_vout_high = (adc_data & 0x3FFF0000)>>16;
			}
			g_adc_dynamic_buf->aux_adc_buf[g_adc_dynamic_buf->adc_buf_idx++] =(int)(dyna_vout_high*390.6/input_divider[div_a]);
	
		}
	}
	//11.-----------------clear irq-------------------------- 
	WRITE_REG(AUX_ADC_CLR, 0x1);	
    if(g_adc_dynamic_buf->adc_buf_idx >= ADC_BUF_MAX_LEN )
    {  
        //os_sem_post(aux_adc_process);
        arch_irq_mask(VECTOR_NUM_AUX_ADC);  
       
    }
}

/**
@brief	register interruption of aux_adc.
*/
void aux_adc_irq_init()
{
    g_adc_dynamic_buf = os_zalloc(sizeof(st_adc_dynamic_buf));
	//memset(g_adc_dynamic_buf ->aux_adc_buf,0,ADC_BUF_MAX_LEN *4);
	arch_irq_register(VECTOR_NUM_AUX_ADC, aux_adc_isr);
	arch_irq_unmask(VECTOR_NUM_AUX_ADC);
    //aux_adc_process = os_sem_create(1, 0);
	return;
}

int drv_adc_lock_init(void)
{
	if (!drv_adc_lock_mutex)
	{
		drv_adc_lock_mutex = os_mutex_create();
		if (!drv_adc_lock_mutex)
		{
			return -2;
		}
	}

	return 0;
}

int drv_adc_lock(void)
{
	return os_mutex_lock(drv_adc_lock_mutex, WAIT_FOREVER);	
}


int drv_adc_unlock(void)
{
	return os_mutex_unlock(drv_adc_lock_mutex);
}

	
/**
@brief	AUX ADC supports the self-calibration process. 
Since more intermediate variables need to be stored during the self-calibration process,
it is designed to store the intermediate variables in RAM without relying on external memory. 
In this case, each power failure will cause the loss of intermediate variables, 
and the calibration process must be executed again. Therefore, after each power-on, 
and before calling AUX ADC for the first time, a self-calibration process is performed. 
The self-calibration process takes less than 500us
*/
void drv_adc_init(void)
{
	open_adc_power_from_digital();
	chip_clk_enable(CLK_ADC_CAL_40M);
	chip_clk_enable(CLK_ADC_AUX_40M);
	chip_clk_enable(CLK_AUX_ADC);

	DRV_ADC_0 tadc0 = {0};
	DRV_ADC_1 tadc1 = {0};
	tadc0.adc_0 =  READ_REG(AUX_ADC_0);
	tadc1.adc_1 =  READ_REG(AUX_ADC_1);

	//1. aux adc en
	tadc0.dig_adc_0.dig_ari_aux_adc_en = 1;//
	tadc0.dig_adc_0.dig_ari_aux_adc_protect=1;
	WRITE_REG(AUX_ADC_0, tadc0.adc_0);

	tadc1.dig_adc_1.dig_ari_aux_adc_pga_en=1;//PGA enable
	tadc1.dig_adc_1.dig_ari_aux_adc_sar_en=1;//SAR ADC enable
	
	WRITE_REG(AUX_ADC_EN , 1);
	WRITE_REG(DIG_BGM_ADC_0P8_SEL, READ_REG(DIG_BGM_ADC_0P8_SEL) | BIT(20));/*??main bg��?3?800MV��?3???DD??2��*/
	unsigned int i = 0;
	unsigned int j = 0;

	//2.1 measure vos under 5 gains
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_VREF_BUFFER;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_VREF_BUFFER;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_range_a_sel=divider_test[7];
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_a_en=0;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider_test[7];
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_b_en=0;
	tadc0.dig_adc_0.dig_ari_aux_adc_protect = 1;
	WRITE_REG(AUX_ADC_0, tadc0.adc_0);
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_cap_sel = 7;
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_en = 1;
	tadc1.dig_adc_1.dig_ari_aux_adc_sar_en = 1;
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_vbp_ctrl = 0;

	for(i=0;i<5;i++)
	{
		float sum_1 =0.0;
		tadc1.dig_adc_1.dig_ari_aux_adc_pga_gain_sel=pga_mode[i];//gain
		WRITE_REG(AUX_ADC_1, tadc1.adc_1);
		//__nds32__isb();	
		os_usdelay(1);
		for(j=0;j<20;j++)
		{
			VOS_TEST[j]=(( READ_REG(AUX_ADC_DATA_OUT) & 0xFFF0000)>>16) ;
			sum_1 += VOS_TEST[j];
			os_usdelay(2);
		}
		VOS[i] = sum_1 /20;
		//cli_printf("VOS[%d]=%d\r\n", i,(int)VOS[i]);
	}
	//2.2 gain4 measure vos_ref
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_VREF_BUFFER;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_VREF_BG;
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_gain_sel=pga_mode[3];//gain3
	WRITE_REG(AUX_ADC_0, tadc0.adc_0);
	WRITE_REG(AUX_ADC_1, tadc1.adc_1);
	os_usdelay(2);
	float sum_2 =0.0;
	for(i=0;i<20;i++)
	{
		VREF_REF_TEST[i]=(( READ_REG(AUX_ADC_DATA_OUT) & 0xFFF0000)>>16);
		sum_2 += VREF_REF_TEST[i];
		os_usdelay(2);
		//__nds32__isb();	
	}
	VOS_ref_test[0] = sum_2 /20 ;
	//cli_printf("VOS_ref[0]=%d\r\n", (int)VOS_ref_test[0]);

	VOS_ref= (VOS_ref_test[0] - VOS[3]) / 4 + 5;
	//VOS_ref= (VOS_ref_test[0] - VOS[4]) / 4 + 5;
	//cli_printf("VOS_ref=%d\r\n", (int)VOS_ref );
	//2.3 measure vref under 5 gains  
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_VREF_BUFFER;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_VREF_BUFFER;
	
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_a_en=1;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_b_en=1;
	for(i=0;i<5;i++)
	{
		float sum_3 =0.0;
		tadc1.dig_adc_1.dig_ari_aux_adc_pga_gain_sel=pga_mode[i];//gain
		tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider[i];// divider_a=1/8
		WRITE_REG(AUX_ADC_0, tadc0.adc_0);
		WRITE_REG(AUX_ADC_1, tadc1.adc_1);
		//__nds32__isb();
		os_usdelay(2);
		//cli_printf("vref_test:0x%x 0x%x 0x%x\r\n", READ_REG(AUX_ADC_0), READ_REG(AUX_ADC_1), READ_REG(AUX_ADC_DATA_OUT));

		for(j=0;j<20;j++)
		{
			//VREF_test[i]= (( READ_REG(AUX_ADC_DATA_OUT) & 0xFFF0000)>>16);
			VREF_TEST[j]= (( READ_REG(AUX_ADC_DATA_OUT) & 0xFFF0000)>>16);
			sum_3 += VREF_TEST[j];
			os_usdelay(2);
			//__nds32__isb();	
		}
		VREF_test[i] = sum_3 /20;

		//cli_printf("VREF_TEST[%d]=%d\r\n", i,(int)VREF_test[i]);
	}
   drv_adc_lock_init();
}

void psm_drv_adc_init(void)
{
    open_adc_power_from_digital();
    chip_clk_enable(CLK_ADC_CAL_40M);
    chip_clk_enable(CLK_ADC_AUX_40M);
    chip_clk_enable(CLK_AUX_ADC);
    WRITE_REG(RF_CTRL_REG_37, READ_REG(RF_CTRL_REG_37) | BIT(16));//enable ldo adda
    WRITE_REG(AUX_ADC_EN , 1);
    WRITE_REG(DIG_BGM_ADC_0P8_SEL, READ_REG(DIG_BGM_ADC_0P8_SEL) | BIT(20));
    DRV_ADC_0 tadc0 ;
    DRV_ADC_1 tadc1 ;
    tadc0.adc_0 =  READ_REG(AUX_ADC_0);
    tadc0.adc_0 |= DRV_ADC0_ARI_AUX_ADC_EN | DRV_ADC0_ARI_AUX_ADC_PROTECT;
	WRITE_REG(AUX_ADC_0, tadc0.adc_0);
    tadc1.adc_1 =  READ_REG(AUX_ADC_1);
    tadc1.adc_1 |= (DIG_ARI_AUX_ADC_PGA_EN << 8)|(DIG_ARI_AUX_ADC_SAR_EN << 10);	
    WRITE_REG(AUX_ADC_1, tadc1.adc_1);
}

/**
@brief	calculate the calibration factor.
@param[in]  VERR: absolute error measurement
@param[in]  divider_a: the partial pressure coefficient of input_a
@param[in]  divider_b: the partial pressure coefficient of input_b
@param[in]  gain_mode: gain value
*/
float gain_cal1=0.0, vos_data1 =0.0;

void gain_cal_vos_data_fix_interface(unsigned int signal_a,  int VERR_1,unsigned int divider_a,unsigned int divider_b,unsigned int gain_mode)
{
	int symbal_flag = 0;
	int gain_parameter =1;
	float k=1.0,gain_cal = 0.0,vos_data = 0.0,ref_data;
	float gain[5]={0.5,1.0,2.0,4.0,8.0};/*This section represents a multiple of the voltage amplification */
	float VREF_buffer=2047.0;

	if(gain_mode==1)
	{
		gain_parameter =0;
	}
	else if (gain_mode==2)
	{
		gain_parameter =1;
	}
	else if (gain_mode==4)
	{
		gain_parameter =2;
	}
	else if (gain_mode==8)
	{
		gain_parameter =3;
	}
	else if (gain_mode==16)
	{
		gain_parameter =4;
	}

	//6.-----------------calculate the calibration factor-------------------------- 
	k=(2048.0 + VERR_1 + VOS_ref)/2048.0;
	/*conventional testing need not amplification of the voltage so use gain[1] is ok....
	input_divider[divider_a]:| needs to select a multiple of the software calculation based on the selected divider*/
	gain_cal=(VREF_known[gain_parameter]/(VREF_test[gain_parameter]-VOS[gain_parameter])/gain[gain_parameter])*k;
	//ref_data=VREF_buffer*input_divider[divider_b]*k;
	//vos_data=ref_data-(VOS[gain_parameter]-2048.0)*gain_cal;
	if(signal_a == DRV_ADC_A_INPUT_VREF_BUFFER)
	{
		ref_data=VREF_buffer*input_divider[divider_a]*k;
		vos_data = -(ref_data+(VOS[gain_parameter]-2048.0)*gain_cal);
	}
	else
	{
		ref_data=VREF_buffer*input_divider[divider_b]*k;
		vos_data=ref_data-(VOS[gain_parameter]-2048.0)*gain_cal;
	}
	
	symbal_flag = gain_cal < 0?1:0;
	/*bit15:show symbol bit  bit0~bit9:show decimals part bit10~bit14:show integer part*/
	WRITE_REG(AUX_ADC_GAIN_CAL_FIX, (symbal_flag <<15)|(int)(ABS(gain_cal)*1024));
	symbal_flag = vos_data < 0?1:0;
	/*bit15:show symbol bit  bit0~bit2:show decimals part bit3~bit14:show integer part*/
	WRITE_REG(AUX_ADC_VOS_DATA_FIX, (symbal_flag <<15)|(((int)(ABS(vos_data)>4095?4095:ABS(vos_data)))*8));
	os_usdelay(1);
	gain_cal1 = gain_cal;
	vos_data1 = vos_data;

	return;
}

/**
@brief	aux_adc_div_config
@param[in]  signal_a: signal selection of input_a
@param[in]  signal_b: signal selection of input_b
@param[in]  volt: The maximum value of the current test voltage range
@param[in]  *tadc0:adc config structure
@return	-1: Input voltage is out of range
		 0: Input voltage is within the range
*/ 
int  aux_adc_div_config(DRV_ADC_INPUT_SIGNAL_A_SEL signal_a, DRV_ADC_INPUT_SIGNAL_B_SEL signal_b, DRV_ADC_EXPECT_VOLT volt, DRV_ADC_0 *tadc0)
{
	if(signal_a == DRV_ADC_A_INPUT_VREF_BUFFER)
	{
		tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_a_en=1;
		tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_a_sel=divider_test[3];
	}
	else
	{
		if(volt <= DRV_ADC_EXPECT_MAX_VOLT0)
		{
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_a_en=0;
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_a_sel=divider_test[7];
		}
		else if((DRV_ADC_EXPECT_MAX_VOLT0 < volt)&&(volt <= DRV_ADC_EXPECT_MAX_VOLT1))
		{
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_a_en=1;
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_a_sel=divider_test[3];
		}
		else if((DRV_ADC_EXPECT_MAX_VOLT1 < volt)&&(volt <= DRV_ADC_EXPECT_MAX_VOLT2))
		{
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_a_en=1;
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_a_sel=divider_test[2];
		}
		else if ((DRV_ADC_EXPECT_MAX_VOLT2 < volt)&&(volt <= DRV_ADC_EXPECT_MAX_VOLT3))
		{
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_a_en=1;
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_a_sel=divider_test[1];
		}
		else
		{
			return -1;
		}
	}

	if(signal_b == DRV_ADC_B_INPUT_VREF_BUFFER)
	{
		tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_b_en=1;
		tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider_test[3];
	}
	else
	{
		if(volt <= DRV_ADC_EXPECT_MAX_VOLT0)
		{
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_b_en=0;
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider_test[7];
		}
		else if((DRV_ADC_EXPECT_MAX_VOLT0 < volt)&&(volt <= DRV_ADC_EXPECT_MAX_VOLT1))
		{
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_b_en=1;
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider_test[3];
		}
		else if((DRV_ADC_EXPECT_MAX_VOLT1 < volt)&&(volt <= DRV_ADC_EXPECT_MAX_VOLT2))
		{
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_b_en=1;
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider_test[2];
		}
		else if ((DRV_ADC_EXPECT_MAX_VOLT2 < volt)&&(volt <= DRV_ADC_EXPECT_MAX_VOLT3))
		{
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_b_en=1;
			tadc0->dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider_test[1];
		}
		else
		{
			return -1;
		}
	}

	return 0;
}

/**
@brief	get_volt_calibrate_data,the data between -127 ~ +127，。
@return	value: 0P8V_Bandgap_offset_ADC[7..0]
		-255: efuse parameter error
		-256: no calibrate value In efuse
		0: GET VERR value.
*/
int get_volt_calibrate_data(void)
{
	unsigned int value = 0; 
	int read_flag;
	read_flag = drv_efuse_read(BANDGAP_OFFSET_ADC_ADDR_EFUSE, &value, 4 );
	if(read_flag != 0)
	{
		os_printf(LM_OS, LL_ERR, "EFUSE PARAMETER ERROR!\r\n"); 
		return -255;
	}
	else
	{
		if(((value & 0xFF0000)>>16)  == 0)
		{
			os_printf(LM_OS, LL_ERR, "No calibrate value In efuse!\r\n"); 
			return -256;
		}
	} 
	value = (value & 0xFF0000)>>16;

	if(value >= 128)
	{
		VERR = 128-value ;
	}
	else
	{
		VERR = value;
	}
	return 0;
}

/**
@brief	aux_adc configuration
@param[in]  signal_a: signal selection of input_a
@param[in]  signal_b: signal selection of input_b
@param[in]  volt: The maximum value of the current test voltage range
@return	0: config finish
		-1: configuration error
*/ 
int drv_aux_adc_config(DRV_ADC_INPUT_SIGNAL_A_SEL signal_a,DRV_ADC_INPUT_SIGNAL_B_SEL signal_b,DRV_ADC_EXPECT_VOLT volt)
{
	DRV_ADC_0 tadc0 = {0};
	DRV_ADC_1 tadc1 = {0};
	unsigned int divider_a = 0;
	unsigned int divider_b = 0;
	int volt_flag ;

	tadc0.adc_0 = READ_REG(AUX_ADC_0);
	tadc1.adc_1 = READ_REG(AUX_ADC_1);

    input_a = signal_a;
    input_b = signal_b;

	switch(signal_a)
	{
		case DRV_ADC_A_INPUT_GPIO_0:
			PIN_FUNC_SET(IO_MUX_GPIO14, FUNC_GPIO14_AUX_0);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x4000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x4000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x4000);
			WRITE_REG(RF_CTRL_REG_38, READ_REG(RF_CTRL_REG_38)&(~0x1F000000));
			WRITE_REG(RF_CTRL_REG_39, READ_REG(RF_CTRL_REG_39)&(~0x60000000));
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=signal_a;
			break;
		case DRV_ADC_A_INPUT_GPIO_1:	
			PIN_FUNC_SET(IO_MUX_GPIO15, FUNC_GPIO15_AUX_1);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x8000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x8000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x8000);
			WRITE_REG(RF_CTRL_REG_38, READ_REG(RF_CTRL_REG_38)&(~0x1F000000));
			WRITE_REG(RF_CTRL_REG_39, READ_REG(RF_CTRL_REG_39)&(~0x60000000));
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=signal_a;
			break;
		case DRV_ADC_A_INPUT_GPIO_2:	
			PIN_FUNC_SET(IO_MUX_GPIO20, FUNC_GPIO20_AUX_2);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x100000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x100000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x100000);
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=signal_a;
			break;
		case DRV_ADC_A_INPUT_GPIO_3:
			//gio18_reset = READ_REG(TEST_MODE_EXTPAD_RST_EN);
			WRITE_REG(TEST_MODE_EXTPAD_RST_EN, 0x0);
			os_usdelay(2);
			PIN_FUNC_SET(IO_MUX_GPIO18, FUNC_GPIO18_AUX_3);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x40000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x40000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x40000);
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=signal_a;
			break;
		case DRV_ADC_A_INPUT_VREF_BUFFER:
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_VREF_BUFFER;
			break;
		default:
			break;
	}
			
	switch(signal_b)
	{
		case DRV_ADC_B_INPUT_GPIO_0:
			PIN_FUNC_SET(IO_MUX_GPIO14, FUNC_GPIO14_AUX_0);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x4000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x4000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x4000);
			WRITE_REG(RF_CTRL_REG_38, READ_REG(RF_CTRL_REG_38)&(~0x1F000000));
			WRITE_REG(RF_CTRL_REG_39, READ_REG(RF_CTRL_REG_39)&(~0x60000000));
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=signal_b;
			break;
		case DRV_ADC_B_INPUT_GPIO_1:	
			PIN_FUNC_SET(IO_MUX_GPIO15, FUNC_GPIO15_AUX_1);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x8000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x8000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x8000);
			WRITE_REG(RF_CTRL_REG_38, READ_REG(RF_CTRL_REG_38)&(~0x1F000000));
			WRITE_REG(RF_CTRL_REG_39, READ_REG(RF_CTRL_REG_39)&(~0x60000000));
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=signal_b;
			break;
		case DRV_ADC_B_INPUT_GPIO_2:	
			PIN_FUNC_SET(IO_MUX_GPIO20, FUNC_GPIO20_AUX_2);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x100000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x100000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x100000);
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=signal_b;
			break;
		case DRV_ADC_B_INPUT_GPIO_3:
			WRITE_REG(TEST_MODE_EXTPAD_RST_EN, 0x0);
			os_usdelay(2);
			PIN_FUNC_SET(IO_MUX_GPIO18, FUNC_GPIO18_AUX_3);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x40000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x40000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x40000);
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=signal_b;
			break;
		case DRV_ADC_B_INPUT_VREF_BUFFER:
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_VREF_BUFFER;
			break;
		default:
			break;
	}

	volt_flag = aux_adc_div_config(signal_a, signal_b,volt,&tadc0);
	if(volt_flag == -1)
	{
		return -1;
	}

	WRITE_REG(AUX_ADC_0, tadc0.adc_0);//0x8080B701);//0x480E701);//
	//5.-----------------anolog config pga gain and cap--------------------------
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_gain_sel=0x2;
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_vbp_ctrl=0;
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_cap_sel = 0x7;
	WRITE_REG(AUX_ADC_1, tadc1.adc_1);//0x2072500);//
	os_usdelay(10);
	divider_a =(READ_REG(AUX_ADC_0) & 0xE000)>>13;
	divider_b =(READ_REG(AUX_ADC_0) & 0xE00)>>9;
	div_a = divider_a ;
	div_b = divider_b ;
	gain_cal_vos_data_fix_interface(signal_a, VERR, divider_a,divider_b,0x2);
	return 0;
}

/**
@brief		Static differential test interface function
@return		vout: Differential test output voltage value
*/
int drv_aux_adc_diff()
{	
	int vout = 0;
	int i;
	float  vout_sum = 0.0;
	for(i=0; i<5; i ++)
	{
		vout =(READ_REG(AUX_ADC_DATA_OUT) & 0x3FFF);
		if(((vout & 0x2000)>>13) == 1)
		{
			vout |= 0xffffc000;
		}
		vout = vout * 390.6/input_divider[div_a];
		vout_sum += vout;
		os_usdelay(1);
	}
	vout = (int)(vout_sum /5) - 800000;
	return vout;
}	


/**
@brief	Static single-ended test function
@return	vout: single-ended test output voltage value
*/
int drv_aux_adc_single()
{
	int vout =0;
	int i;
	float  vout_sum = 0.0;
	if(input_a == DRV_ADC_A_INPUT_VREF_BUFFER)//input_b input volt
	{
		os_usdelay(1);
		vout = (READ_REG(AUX_ADC_DATA_OUT) & 0xFFF0000)>>16;
		vout =(int)((-((vout - 2048)*gain_cal1+vos_data1))/input_divider[div_b])*390.6;

	}
	else if(input_b == DRV_ADC_B_INPUT_VREF_BUFFER)//input_a input volt
	{
		for(i=0; i<20; i ++)
		{
			vout =(READ_REG(AUX_ADC_DATA_OUT) & 0x3FFF);
			if((( vout & 0x2000)>>13) == 1)
			{
				vout |= 0xffffc000;
			}
			vout = vout * 390.6/input_divider[div_a];
			vout_sum += vout;
			os_usdelay(1);
		}
		
		vout = (int)(vout_sum /20);
	}
    else
    {
        os_printf(LM_CMD,LL_ERR,"input A or B error!\r\n"); 
    }

	return vout;
}

/**
@brief	Dynamic test interface function,
		note :the voltage value of dynamic test needs to add 800mv to be the real voltage value
*/


int drv_aux_adc_dynamic(int* bufter ,int length)
{   
    if(length > 0)
    {
        dynamic_data_len = min(length, ADC_BUF_MAX_LEN);
    	memcpy(bufter, g_adc_dynamic_buf->aux_adc_buf, dynamic_data_len * 4);
        g_adc_dynamic_buf->adc_buf_idx = 0;
    }
	return 0;
}

int drv_aux_adc_dynamic_init()
{

    if (dyn_adc_flag == 0)
    {
    	memset(g_adc_dynamic_buf->aux_adc_buf,0,ADC_BUF_MAX_LEN*4);
    	aux_adc_irq_init();
        WRITE_REG(AUX_ADC_FIFO_WR_CONFIG , 0x1);
        dyn_adc_flag++ ;
     }
     return 0;
}

/**
@brief	calculate battery voltage
*/
 int vbat_sensor_get()
{
	DRV_ADC_0 tadc0 = {0};
	DRV_ADC_1 tadc1 = {0};

	tadc0.adc_0 = READ_REG(AUX_ADC_0);
	tadc1.adc_1 = READ_REG(AUX_ADC_1);
	int vout = 0;

	//3.-----------------anolog config mux_A/mux_B--------------------------
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_VBAT;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_VREF_BUFFER;
	//4.-----------------anolog config divider--------------------------
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_a_en=1;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_range_a_sel=divider_test[1];
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_b_en=1;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider_test[3];
	tadc0.dig_adc_0.dig_ari_aux_adc_protect=1;

	WRITE_REG(AUX_ADC_0, tadc0.adc_0);
	//5.-----------------anolog config pga gain and cap--------------------------
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_gain_sel=DRV_ADC_PGA_GAIN1_MODE;
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_vbp_ctrl = 0;
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_cap_sel = 7;//Add more filter capacitance
	WRITE_REG(AUX_ADC_1, tadc1.adc_1);
	//6.-----------------measure volt-------------------------- 
	gain_cal_vos_data_fix_interface(1,VERR,1,3,DRV_ADC_PGA_GAIN1_MODE);
	//7.-----------------measure volt-------------------------- 
	vout =(READ_REG(AUX_ADC_DATA_OUT) & 0x3FFF);
	if(((vout & 0x2000)>>13) == 1)
	{
		vout |= 0xffffc000;
	}
	vout = (int)(vout *390.6/input_divider[1]);
	
	return vout;
}
 
 /**
 @brief  tempsensor for PA measures the local temperature of the PA on the chip
 @details  When calibrating, only need to call AUX ADC for measurement at room temperature 27℃, and save 8bits data in EFUSE.
			 As a function of voltage-temperature : Temp*0.002 + X =ts_data_pa*0.003125 ,Where ts_data_pa is the voltage value 
			 corresponding to 27°. X represents the voltage value corresponding to the current temperature.Temp represents 
			 the current temperature
 */
int temprature_sensor_get_pa(void)
{
    drv_adc_lock();
	DRV_ADC_0 tadc0 = {0};
	DRV_ADC_1 tadc1 = {0};

	tadc0.adc_0 = READ_REG(AUX_ADC_0);
	tadc1.adc_1 = READ_REG(AUX_ADC_1);

	int ts_data_pa=0;
	int i;
	float  vout_sum = 0.0;
	open_adc_power_from_digital();
	chip_clk_enable(CLK_ADC_CAL_40M);
	chip_clk_enable(CLK_ADC_AUX_40M);
	chip_clk_enable(CLK_AUX_ADC);
	//1. aux adc en
	tadc0.dig_adc_0.dig_ari_aux_adc_en = 1;//
	tadc0.dig_adc_0.dig_ari_aux_adc_protect=1;
	WRITE_REG(AUX_ADC_0, tadc0.adc_0);

	tadc1.dig_adc_1.dig_ari_aux_adc_pga_en=1;//PGA enable
	tadc1.dig_adc_1.dig_ari_aux_adc_sar_en=1;//SAR ADC enable
	
	WRITE_REG(AUX_ADC_EN , 1);
	WRITE_REG(DIG_BGM_ADC_0P8_SEL, READ_REG(DIG_BGM_ADC_0P8_SEL) | BIT(20));/*??main bg��?3?800MV��?3???DD??2��*/

	//2.-----------------anolog config mux_A/mux_B--------------------------
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_VREF_BUFFER;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_VTEMP_PA;
	tadc1.dig_adc_1.dig_ari_aux_ts_bg_en = 0;/*enable temp bg*/
	tadc1.dig_adc_1.dig_ari_aux_ts_bg_rsel = 0;/*Reduce the partial voltage resistance*/
	tadc0.dig_adc_0.dig_ari_aux_adc_protect = 1;
	//3.-----------------anolog config divider--------------------------
	/*b set input ,a set vref,so a 4/8 divider and divider enable, b 8/8 divider and divider disable*/
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_a_en=1;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_range_a_sel=divider_test[3];
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_b_en=0;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider_test[7];	
	WRITE_REG(AUX_ADC_0, tadc0.adc_0);
	//4.-----------------anolog config pga gain and cap--------------------------
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_gain_sel=DRV_ADC_PGA_GAIN1_MODE;
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_cap_sel=0x7;
	tadc1.dig_adc_1.dig_ari_aux_ts_pa_en = 1;
	tadc1.dig_adc_1.dig_ari_aux_ts_pa_rsel = 0x4;
	WRITE_REG(AUX_ADC_1, tadc1.adc_1);
	//5.-------------calculate the calibration factor------- 
	gain_cal_vos_data_fix_interface(1,VERR,3,7,DRV_ADC_PGA_GAIN1_MODE);
	//7.-------------calculate the calibration factor------- 
	//ts_data_pa =((READ_REG(AUX_ADC_DATA_OUT) & (0xFFF<<16)))>>16;
	for(i=0;i<20;i++)
	{
		ts_data_pa =((READ_REG(AUX_ADC_DATA_OUT) & (0xFFF<<16)))>>16;
		vout_sum += ts_data_pa;
	}
	ts_data_pa = (int)(vout_sum /20);
    drv_adc_unlock();
	return ts_data_pa;
}

/**
@brief		tempsensor and vbat share an interface
@param[in]  mode_flag: distinguish tempsensor in pa、tempsensor in bg and vbat.
*/
int vbat_temp_share_test(unsigned int mode_flag)
{
	unsigned int vout =0;
	if(mode_flag == 0)
	{
		vout =vbat_sensor_get();
	}
	else
	{
		vout =temprature_sensor_get_pa();
	}
	return vout;
}

int get_25_tempsensor_to_volt()
{
	int read_flag;
	unsigned int value = 0;
	read_flag = drv_efuse_read(TEMPERATURE_SENSOR_ADDR_EFUSE, &value, 4 );
	
	if(read_flag != 0)
	{
		os_printf(LM_CMD,LL_ERR,"EFUSE PARAMETER ERROR!\r\n"); 
		return -255;
	}
	else
	{
		if((value & 0xFFFF)  == 0)
		{
			os_printf(LM_CMD,LL_ERR,"No calibrate value In efuse!\r\n"); 
			return -256;
		}
	} 
	value =value & 0xFFFF;

	return value;
}

int get_current_temprature()
{
	int volt_25;
    float current_temp ;
	volt_25 = get_25_tempsensor_to_volt();
    volt_25 += 30;
    
    if (volt_25 <= 30)
    {
         return 0;
    }
	
	int temp_to_volt_now = temprature_sensor_get_pa(); 
	current_temp =(temp_to_volt_now - volt_25) / ADC_TEMP_SENSOR_TO_TEMP + 25;
    //os_printf(LM_CMD,LL_INFO,"temp now=% .1f\r\n", current_temp);
	return (int)current_temp;
}

DRV_ADC_TEMP_TYPE hal_adc_get_temp_type()
{
	unsigned int temp_to_volt_now = 0;
    int volt_25 = 0;
    //return DRV_ADC_TEMP_HIGH;  
	temp_to_volt_now = temprature_sensor_get_pa();
	os_printf(LM_CMD,LL_DBG,"volt now=%d\r\n", temp_to_volt_now);
	volt_25 = get_25_tempsensor_to_volt();
    if(volt_25 <= 0)
    {
		xTimerStop( xTimerTemperature, portMAX_DELAY);
         return DRV_ADC_TEMP_NORMAL;
    }
    else
    {
         volt_25 += 30;
        if(temp_to_volt_now > (volt_25-(ADC_TEMP_SENSOR_TO_TEMP*(25-ADC_TEMP_LOW))))
        {
            os_printf(LM_CMD,LL_DBG,"--Currently at Low Temperature Status--\n");
            return DRV_ADC_TEMP_LOW;  
        }
        else if((temp_to_volt_now <= (volt_25-(ADC_TEMP_SENSOR_TO_TEMP*(25-ADC_TEMP_LOW))))&&(temp_to_volt_now >= (volt_25-(ADC_TEMP_SENSOR_TO_TEMP*(25-ADC_TEMP_NORMAL_TEM)))))
        {
            os_printf(LM_CMD,LL_DBG,"--Currently at Normal Temperature Status--\n");
            return DRV_ADC_TEMP_NORMAL;
        }
        else 
        {       
            os_printf(LM_CMD,LL_DBG,"--Currently at High Temperature Status--\n");
            return DRV_ADC_TEMP_HIGH;
        }  
    }
}

extern void temp_autogain_change();
void hal_tempsensor_process(void *arg)
{
    //temp_status = hal_adc_get_temp_type();
	temp_autogain_change();
}

void time_check_temp()
{
#ifdef CONFIG_PSM_SURPORT
    xTimerTemperature = xTimerCreate
					("Temperature detection",
					1000*5 / portTICK_PERIOD_MS,
					pdTRUE,
					( void * ) 0,
					(TimerCallbackFunction_t)hal_tempsensor_process);
#else
	xTimerTemperature = xTimerCreate
					("Temperature detection",
					1000*60 / portTICK_PERIOD_MS,
					pdTRUE,
					( void * ) 0,
					(TimerCallbackFunction_t)hal_tempsensor_process);

#endif

	if( xTimerTemperature != NULL ) {
        xTimerStart(xTimerTemperature, portMAX_DELAY);
	}
}


#if 0
void show_adc_dynamic_data()
{
	int i = 0;
	if(g_adc_dynamic_buf.adc_buf_idx >= ADC_BUF_MAX_LEN)
	{
		for(i = 0;i<ADC_BUF_MAX_LEN;i++)
		{
			//cli_printf("%d\r\n",(int)(g_adc_dynamic_buf.aux_adc_buf[i]*390.6/input_divider[div_a]));
			cli_printf("vout[%d]=%d\r\n",i,g_adc_dynamic_buf.aux_adc_buf[i]);
		}
		g_adc_dynamic_buf.adc_buf_idx = 0;
		memset(g_adc_dynamic_buf.aux_adc_buf,0,ADC_BUF_MAX_LEN);
	}
}
#endif

#ifdef CONFIG_ADC_HARDWARE
int hal_aux_adc_hardware_test(unsigned int signal_a,unsigned int signal_b,unsigned int dividea_enable,unsigned int divideb_enable,unsigned int divider_a,unsigned int divider_b,unsigned int gain_mode,unsigned int mode_flag)
{
	//step 1:
	unsigned int value;
	PIN_FUNC_SET(IO_MUX_GPIO14, FUNC_GPIO14_AUX_0);
	PIN_FUNC_SET(IO_MUX_GPIO15, FUNC_GPIO15_AUX_1);
    value = READ_REG(TEST_MODE_EXTPAD_RST_EN);
	WRITE_REG(TEST_MODE_EXTPAD_RST_EN, 0x1);
	WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0xC000));
	WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0xC000));
	WRITE_REG(RF_CTRL_REG_38,0x9<<24);           //buffer mode
	WRITE_REG(MEM_BASE_RFC_REG,0x201);      
	//step 2: 
	int i;
	DRV_ADC_0 tadc0 = {0};
	DRV_ADC_1 tadc1 = {0};

	tadc0.adc_0 = READ_REG(AUX_ADC_0);
	tadc1.adc_1 = READ_REG(AUX_ADC_1);

	int  vout = 0;
	float  vout_sum = 0.0;
	//int VERR1 = 6;
	div_a = divider_a;
	div_b = divider_b;

	switch(signal_a)
	{
		case DRV_ADC_A_INPUT_GPIO_0:
			PIN_FUNC_SET(IO_MUX_GPIO14, FUNC_GPIO14_AUX_0);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x4000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x4000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x4000);
			WRITE_REG(RF_CTRL_REG_38, READ_REG(RF_CTRL_REG_38)&(~0x1F000000));
			WRITE_REG(RF_CTRL_REG_39, READ_REG(RF_CTRL_REG_39)&(~0x60000000));
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_GPIO_0;

			break;
		case DRV_ADC_A_INPUT_GPIO_1:	
			PIN_FUNC_SET(IO_MUX_GPIO15, FUNC_GPIO15_AUX_1);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x8000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x8000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x8000);
			WRITE_REG(RF_CTRL_REG_38, READ_REG(RF_CTRL_REG_38)&(~0x1F000000));
			WRITE_REG(RF_CTRL_REG_39, READ_REG(RF_CTRL_REG_39)&(~0x60000000));
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_GPIO_1;
			break;
		case DRV_ADC_A_INPUT_GPIO_2:	
			PIN_FUNC_SET(IO_MUX_GPIO20, FUNC_GPIO20_AUX_2);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x100000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x100000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x100000);
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_GPIO_2;
			break;
		case DRV_ADC_A_INPUT_GPIO_3:
			PIN_FUNC_SET(IO_MUX_GPIO18, FUNC_GPIO18_AUX_3);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x40000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x40000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x40000);
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel= DRV_ADC_A_INPUT_GPIO_3;
			break;
		case DRV_ADC_A_INPUT_VREF_BUFFER:
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_VREF_BUFFER;
			break;
		case DRV_ADC_A_INPUT_VBAT:
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_VBAT;
			break;
		case DRV_ADC_A_INPUT_VTEMP_BG:
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_a_sel=DRV_ADC_A_INPUT_VTEMP_BG;
			break;
	}
	switch(signal_b)
	{
		case DRV_ADC_B_INPUT_GPIO_0:
			PIN_FUNC_SET(IO_MUX_GPIO14, FUNC_GPIO14_AUX_0);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x4000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x4000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x4000);
			WRITE_REG(RF_CTRL_REG_38, READ_REG(RF_CTRL_REG_38)&(~0x1F000000));
			WRITE_REG(RF_CTRL_REG_39, READ_REG(RF_CTRL_REG_39)&(~0x60000000));
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_GPIO_0;
			break;
		case DRV_ADC_B_INPUT_GPIO_1:	
			PIN_FUNC_SET(IO_MUX_GPIO15, FUNC_GPIO15_AUX_1);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x8000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x8000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x8000);
			//WRITE_REG(RF_CTRL_REG_38, READ_REG(RF_CTRL_REG_38)&(~0x1F000000));
			WRITE_REG(RF_CTRL_REG_39, READ_REG(RF_CTRL_REG_39)&(~0x60000000));
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_GPIO_1;
			break;
		case DRV_ADC_B_INPUT_GPIO_2:	
			PIN_FUNC_SET(IO_MUX_GPIO20, FUNC_GPIO20_AUX_2);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x100000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x100000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x100000);
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_GPIO_2;
			break;
		case DRV_ADC_B_INPUT_GPIO_3:
			PIN_FUNC_SET(IO_MUX_GPIO18, FUNC_GPIO18_AUX_3);
			WRITE_REG(PAD_PULLUP, READ_REG(PAD_PULLUP)&(~0x40000));
			WRITE_REG(PAD_PULLDOWN, READ_REG(PAD_PULLDOWN)&(~0x40000));
			WRITE_REG(PAD_EN, READ_REG(PAD_EN)|0x40000);
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_GPIO_3;
			break;
		case DRV_ADC_B_INPUT_VREF_BUFFER:
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_VREF_BUFFER;
			break;
		case DRV_ADC_B_INPUT_VTEMP_PA:
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_VTEMP_PA;
			break;
		case DRV_ADC_B_INPUT_V_TIELOW:
			tadc0.dig_adc_0.dig_ari_aux_adc_input_signal_b_sel=DRV_ADC_B_INPUT_V_TIELOW;
			break;
	}
	
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_a_en=dividea_enable;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_range_a_sel=divider_a;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_b_en=divideb_enable;
	tadc0.dig_adc_0.dig_ari_aux_adc_input_divider_range_b_sel=divider_b;

	WRITE_REG(AUX_ADC_0, tadc0.adc_0);//0x8080B701);//0x480E701);//
	
	//5.-----------------anolog config pga gain and cap--------------------------
	if(gain_mode == 0 )
	{
		tadc1.dig_adc_1.dig_ari_aux_adc_pga_gain_sel=2;
		tadc1.dig_adc_1.dig_ari_aux_adc_bypass_pga=1;
		tadc1.dig_adc_1.dig_ari_aux_adc_pga_en =0;
	}
	else
	{
		tadc1.dig_adc_1.dig_ari_aux_adc_bypass_pga=0;
		tadc1.dig_adc_1.dig_ari_aux_adc_pga_en =1;
		tadc1.dig_adc_1.dig_ari_aux_adc_pga_gain_sel=gain_mode;
	}
	
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_vbp_ctrl=0;
	tadc1.dig_adc_1.dig_ari_aux_adc_pga_cap_sel = 0x7;
	if(signal_b == DRV_ADC_B_INPUT_VTEMP_PA)
	{
		tadc1.dig_adc_1.dig_ari_aux_ts_bg_en = 0;/*enable temp bg*/
		tadc1.dig_adc_1.dig_ari_aux_ts_bg_rsel = 0;/*Reduce the partial voltage resistance*/
		tadc1.dig_adc_1.dig_ari_aux_ts_pa_en = 1;
		tadc1.dig_adc_1.dig_ari_aux_ts_pa_rsel = 0x4;
	}
	if(signal_a == DRV_ADC_A_INPUT_VTEMP_BG )
	{
		tadc1.dig_adc_1.dig_ari_aux_ts_bg_en = 1;/*enable temp bg*/
		tadc1.dig_adc_1.dig_ari_aux_ts_bg_rsel = 0;/*Reduce the partial voltage resistance*/
		tadc1.dig_adc_1.dig_ari_aux_ts_pa_en = 0;
		tadc1.dig_adc_1.dig_ari_aux_ts_pa_rsel = 0x4;
	}

	WRITE_REG(AUX_ADC_1, tadc1.adc_1);
	//6.-----------------calculate the calibration factor-------------------------- 
	//gain_cal_vos_data_fix_interface(signal_a,VERR1,divider_a,divider_b,gain_mode);
	gain_cal_vos_data_fix_interface(signal_a,VERR,divider_a,divider_b,gain_mode);

	if(mode_flag == 0)
	{
		WRITE_REG(AUX_ADC_EN , 1);

		if(signal_a == DRV_ADC_A_INPUT_VREF_BUFFER)//input_b input volt
		{
			os_usdelay(2);
			//vout =(int)((-((vout - 2048)*gain_cal1+vos_data1))/input_divider[divider_b])*390.6;
			for(i=0;i<20;i++)
			{
				vout = (READ_REG(AUX_ADC_DATA_OUT) & 0xFFF0000)>>16;
				vout =(int)((-((vout - 2048)*gain_cal1+vos_data1))/input_divider[divider_b])*390.6;
				vout_sum += vout;
			}
			vout = (int)(vout_sum /20);

		}
		else if(signal_b == DRV_ADC_B_INPUT_VREF_BUFFER)//input_a input volt
		{
			for(i=0; i<20; i ++)
			{
				vout = 0.0 ;
				vout =(READ_REG(AUX_ADC_DATA_OUT) & 0x3FFF);
				if(((READ_REG(AUX_ADC_DATA_OUT) & 0x2000)>>13) == 1)
				{
					vout |= 0xffffc000;
				}
				vout = vout * 390.6/input_divider[divider_a];
				vout_sum += vout;
				os_usdelay(1);
			}
			vout = (int)(vout_sum /20);
		}
		else// diff test
		{
			for(i=0; i<20; i ++)
			{
				vout = 0.0 ;
				vout =(READ_REG(AUX_ADC_DATA_OUT) & 0x3FFF);
				if(((READ_REG(AUX_ADC_DATA_OUT) & 0x2000)>>13) == 1)
				{
					vout |= 0xffffc000;
				}
				vout = vout * 390.6/input_divider[divider_a];
				vout_sum += vout;
				os_usdelay(1);
			}
			vout = (int)(vout_sum /20) - 800000;
		}

		WRITE_REG(TEST_MODE_EXTPAD_RST_EN, value);
		return vout;
	}
	else
	{
		WRITE_REG(AUX_ADC_FIFO_WR_CONFIG , 0x18);
		aux_adc_irq_init();
		WRITE_REG(AUX_ADC_EN , 1);

		WRITE_REG(TEST_MODE_EXTPAD_RST_EN, value);
		return 0;
	}

}	
#endif

