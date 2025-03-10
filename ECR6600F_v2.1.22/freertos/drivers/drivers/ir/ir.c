
#include "ir.h"
#include "chip_pinmux.h"
#include "chip_clk_ctrl.h"
#include "oshal.h"
#include "gpio.h"
#include "cli.h"
#include "pit.h"

unsigned char         receive_num  = 0;
volatile unsigned int custom_value = 0;
volatile unsigned int data_value   = 0;
volatile unsigned int infor_value  = 0;

unsigned int test_time0 = 0;
unsigned int test_time1 = 0;
unsigned int test_time2 = 0;


typedef volatile struct _T_IR_REG_MAP
{
	unsigned int IrCarrier;		/* 0x00 - IR_CARRIER register */	
	unsigned int IrLogicBit;	/* 0x04 - IR_LOGICBIT register */
	unsigned int IrCtrl;		/* 0x08 - IR_CTRL register */
	unsigned int IrFifoClr;		/* 0x0C - IR_FIFOCLR register */
	unsigned int IrStatus;		/* 0x10 - IR_STATUS register */
	unsigned int IrIntStat;		/* 0x14 - IR_INTSTAT register */
	unsigned int IrComCMD;		/* 0x18 - IR_COMCMD_dr register */
	unsigned int IrRepCMD;		/* 0x1C - IR_REPCPM register */
} T_IR_REG_MAP;

typedef volatile struct _T_IR_REG_CFG
{
	unsigned int IrCarrier;		/* 0x00 - IR_CARRIER register */	
	unsigned int IrLogicBit;	/* 0x04 - IR_LOGICBIT register */
	unsigned int IrCtrl;		/* 0x08 - IR_CTRL register */
	unsigned int IrComCMD[32];	/* 0x18 - IR_COMCMD_dr register */
} T_IR_REG_CFG;

T_IR_REG_CFG ir_cfg;


unsigned int ulGetTickCounter( void );


/**    @brief		Spiflash sector erase api.
 *	   @details 	Erase data in flash with specified address and length. The erase unit is 4K byte.
 *     @param[in]	addr--The address where you want to erase data to flash.
 *     @param[in]	length--The length of data to be erased to the specified address of flash.
 *     @return      0--erase ok,-4--erase addr or length not an integral multiple of 4K ,-5--write enable not set,-6--the memory is busy in erase, not erase completely.
 */
void carrier_wave(unsigned int period_val,unsigned int duty_cycle,unsigned int pwm_fmt_low)
{
	unsigned int carr_value = 0; 
	unsigned int cmd = 0;
	
	carr_value = period_val/duty_cycle;
	
	cmd = IR_CARRIER_period(period_val)| IR_CARRIER_invtime(carr_value);

	//set pwm_fmt_low --1:from low to high  '0:from high to low
	if(pwm_fmt_low){
		cmd  = cmd | IR_CARRIER_pwmfmt_low ;
	}
	else{
		cmd  = cmd & 0xFFBFFFFF;
	}
	ir_cfg.IrCarrier = cmd;
}


void CMD_bit_wave(unsigned int r_cmd_num,unsigned int r_cmd_bit_number,unsigned int r_cmd_bit_sequence)
{
	unsigned int cmd = 0;

	//this is bit wave mode

	//set number
	cmd = ((cmd & 0xFFF0FFFF) | IR_COMCMD_bit_number(r_cmd_bit_number-1)); 	
	//os_printf(LM_OS, LL_INFO, "number is 0x%x\r\n",cmd|IR_COMCMD_bit_number(cmd));

	//set bits
	cmd = ((cmd & 0xFFFF0000) | IR_COMCMD_bit_sequence(r_cmd_bit_sequence)); 	
	//os_printf(LM_OS, LL_INFO, "bits is 0x%x\r\n",cmd|IR_COMCMD_bit_sequence(cmd));
	
	ir_cfg.IrComCMD[r_cmd_num] = cmd;
}


void CMD_leader_code_wave(unsigned int r_cmd_num,unsigned int fmt,unsigned int r_cmd_first_ht,unsigned int r_cmd_last_ht)
{
	unsigned int cmd = 0;

	//this is leader code wave mode
	cmd = cmd | 0x02000000;
	
	//set Fmt(mark first or not: 0 mark first,1 space first)
	if(fmt == 0){
		cmd = cmd & 0xFEFFFFFF;				
	}
	else{
		cmd = cmd | IR_COMCMD_space_first;	
	}
			
	//set fhtime
	cmd = ((cmd & 0xFFFFF000) | IR_COMCMD_first_ht(r_cmd_first_ht)); 	
	//os_printf(LM_OS, LL_INFO, "fhtime is 0x%x\r\n",IR_COMCMD_first_ht(cmd));

	//set lhtime
	cmd = ((cmd & 0xFF000FFF) | IR_COMCMD_last_ht(r_cmd_last_ht)); 	
	//os_printf(LM_OS, LL_INFO, "lhtime is 0x%x\r\n",IR_COMCMD_last_ht(cmd));
	
	ir_cfg.IrComCMD[r_cmd_num] = cmd;
	
}


void logicbit_set(unsigned int r_zero_lh,unsigned int r_zero_fh,unsigned int r_one_lh,unsigned int r_one_fh)
{
	unsigned int cmd = 0;
	
	cmd = (cmd & 0x00ffffff)|IR_LOGICBIT_zero_lh(r_zero_lh);
	
	cmd = (cmd & 0xff00ffff)|IR_LOGICBIT_zero_fh(r_zero_fh);
	
	cmd = (cmd & 0xffff00ff)|IR_LOGICBIT_one_lh(r_one_lh);
	
	cmd = (cmd & 0xffffff00)|IR_LOGICBIT_one_fh(r_one_fh);	

	ir_cfg.IrLogicBit = cmd;
	
	//os_printf(LM_OS, LL_INFO, "logicbit set ok\r\n",cmd);
}


//reptime		//repeat code interval,in carrier clock cycles
//cmdie			//interrupt at each command transmit done [0:disable] [1:enable]
//irie			//0:disable interrupt when common cmd FIFO transmit done 1:enable interrupt when common cmd FIFO transmit done
//zerofmt		//0:mark first 1:space first
//onefmt		//0:mark first 1:space first
//irinvout 		//0:not invert output level  1:invert output level
//mode			//0:IR mode  1:PWM mode
//repen 		//enable repeat code transmition
//iren     		//enable ir transmition
void ctrl_cfg(unsigned int reptime, unsigned int cmdie,unsigned int irie,unsigned int zerofmt,unsigned int onefmt,unsigned int irinvout,unsigned int mode,unsigned int repen,unsigned int iren)
{
	unsigned int cmd = 0;

	cmd = IR_CTRL_reptime(reptime);
	if(cmdie){
		cmd = cmd | IR_CTRL_cmdie;}
	
	if(irie){
		cmd = cmd | IR_CTRL_irie;}
	
	if(zerofmt){
		cmd = cmd | IR_CTRL_zerofmt;}
	
	if(onefmt){
		cmd = cmd | IR_CTRL_onefmt;}
	
	if(irinvout){
		cmd = cmd | IR_CTRL_irinvout;}
	
	if(mode){
		cmd = cmd | IR_CTRL_mode;}

	if(repen){
		cmd = cmd | IR_CTRL_repen;}

	if(iren){
		cmd = cmd | IR_CTRL_iren;}
	
	ir_cfg.IrCtrl = cmd;
	//os_printf(LM_OS, LL_INFO, "ctrl set ok\r\n",cmd);
}

void ir_trans_config(void)
{
#if 0
	ir_cfg.IrCarrier   = IR_CLK_24M;	//IR clck = 12mhz  carrier is 38khz
	ir_cfg.IrLogicBit  = 0x15153F15;	//NEC format
	ir_cfg.IrCtrl      = 0x4b;
	
	ir_cfg.IrComCMD[0] = 0x20ab156;		//leader code
	ir_cfg.IrComCMD[1] = 0x00ff9b0;		//custom code
	ir_cfg.IrComCMD[2] = 0x00f1771;		//data code 
	ir_cfg.IrComCMD[3] = 0x2744015;		//stop bit & frame space 
	//CMD can be add or adjusted to fit your needs
#else
	
	unsigned int period_val = 632,duty_cycle = 3,pwm_fmt_low = 0;
	unsigned int zero_lh = 0x15,zero_fh = 0x15,one_lh = 0x3F,one_fh = 0x15;

	//set carrier wave,The goal is to configure a 38 kHz carrier
	carrier_wave(period_val,duty_cycle,pwm_fmt_low); 
	
	//set logic bit (bit wave)
	logicbit_set(zero_lh,zero_fh,one_lh,one_fh);
	
	//set ctrl reg
	ctrl_cfg(0x00,0,1,0,0,1,0,1,1);

	
	//leader code wave ---- leader code
	CMD_leader_code_wave(0,0,0x156,0x0ab);

	//bits wave -----  custom code
	CMD_bit_wave(1,16,0xf9b0);

	//bits wave -----  data code
	CMD_bit_wave(2,16,0x1771);

	//leader code wave ---- stop bit & frame space
	CMD_leader_code_wave(3,0,0x015,0x744);

	
#endif
}

void ir_trans_api(unsigned int cmd_num)
{
	//muse be init first 
	unsigned int status = 0;
	unsigned int i = 0;
	T_IR_REG_MAP * p_ir_reg = (T_IR_REG_MAP * )MEM_BASE_IR;
	
	//PIN_FUNC_SET(IO_MUX_GPIO13, FUNC_GPIO13_IR_OUT);
	
	ir_trans_config();

	p_ir_reg->IrCarrier  = ir_cfg.IrCarrier;
	p_ir_reg->IrLogicBit = ir_cfg.IrLogicBit;
	p_ir_reg->IrCtrl	 = ir_cfg.IrCtrl;
	
	for(i=0; i<cmd_num; i++)
	{
		p_ir_reg->IrComCMD = ir_cfg.IrComCMD[i];
	}

	do
	{
		status = p_ir_reg->IrStatus;
	}while(status != 0);

	p_ir_reg->IrIntStat = 0x3;

}



enum status
{
	infor_code = 1,
	end_code = 2,
	repeat_code = 3,
};
enum status receive_status;	


unsigned int receive_repeat = 0;

static os_sem_handle_t ir_data_process;

unsigned int hal_read_timestamp(void)
{
	unsigned int ret = 0;
	drv_pit_ioctrl(DRV_PIT_CHN_7, DRV_PIT_CTRL_GET_COUNT, (unsigned int)&ret);
	return ret;
}
void hal_ir_data_process(void *pvParameters)
{
	while(1)
	{
//		os_sem_wait(ir_data_process, portMAX_DELAY);
		os_sem_wait(ir_data_process, 0xFFFFFFFF);
	//	os_printf(LM_OS, LL_INFO, " custom_value = 0x%x, data_value =0x%x \n",(infor_value & 0xffff),(infor_value>>16));
		if((infor_value & 0xffff) == COSTOM_CODE_HBS838)
		{
			switch(infor_value>>16)
			{
				case IR_KEY_1:
					os_printf(LM_OS, LL_INFO, "the key is 1-1， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_2:
					os_printf(LM_OS, LL_INFO, "the key is 1-2， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_3:
					os_printf(LM_OS, LL_INFO, "the key is 1-3， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_4:
					os_printf(LM_OS, LL_INFO, "the key is 1-4， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_5:
					os_printf(LM_OS, LL_INFO, "the key is 2-1， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_6:
					os_printf(LM_OS, LL_INFO, "the key is 2-2， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_7:
					os_printf(LM_OS, LL_INFO, "the key is 2-3， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_8:
					os_printf(LM_OS, LL_INFO, "the key is 2-4， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_9:
					os_printf(LM_OS, LL_INFO, "the key is 3-1， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_10:
					os_printf(LM_OS, LL_INFO, "the key is 3-2， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_11:
					os_printf(LM_OS, LL_INFO, "the key is 3-3， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_12:
					os_printf(LM_OS, LL_INFO, "the key is 3-4， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_13:
					os_printf(LM_OS, LL_INFO, "the key is 4-1， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_14:
					os_printf(LM_OS, LL_INFO, "the key is 4-2， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_15:
					os_printf(LM_OS, LL_INFO, "the key is 4-3， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_16:
					os_printf(LM_OS, LL_INFO, "the key is 4-4， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_17:
					os_printf(LM_OS, LL_INFO, "the key is 5-1， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_18:
					os_printf(LM_OS, LL_INFO, "the key is 5-2， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_19:
					os_printf(LM_OS, LL_INFO, "the key is 5-3， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_20:
					os_printf(LM_OS, LL_INFO, "the key is 5-4， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_21:
					os_printf(LM_OS, LL_INFO, "the key is 6-1， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_22:
					os_printf(LM_OS, LL_INFO, "the key is 6-2， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_23:
					os_printf(LM_OS, LL_INFO, "the key is 6-3， receive_repeat = %d \n",receive_repeat);
					break;
				
				case IR_KEY_24:
					os_printf(LM_OS, LL_INFO, "the key is 6-4， receive_repeat = %d \n",receive_repeat);
					break;

				default:
					os_printf(LM_OS, LL_INFO, "the key value is	error\n");
					break;
			}
		}
	}
}

void hal_ir_callback(void *data)
{
//	os_printf(LM_OS, LL_INFO, "entry ir callback ! \n");	
	unsigned int test_time1 = 0;

	test_time1 = test_time0;
	test_time0 = hal_read_timestamp() * CLC_NUM ;
	// leader code 	
	if((IR_LEADER_HIGH_LEVEL_MIN < (test_time1 - test_time0)) && ((test_time1 - test_time0) < IR_LEADER_HIGH_LEVEL_MAX))
	{
		receive_status = 1;
		infor_value = 0;
		receive_repeat = 0;
	}
	
	// custom & data code 
	if(receive_status == 1)
	{
		if((IR_INFOR_LOW_LEVEL_MIN < (test_time1 - test_time0)) && ((test_time1 - test_time0) < IR_INFOR_LOW_LEVEL_MAX))		//logic '0'	
		{
			infor_value = infor_value >> 1;
			receive_num++;
		}
		else if((IR_INFOR_HIGH_LEVEL_MIN < (test_time1 - test_time0)) && ((test_time1 - test_time0) < IR_INFOR_HIGH_LEVEL_MAX))//logic '1'
		{
			infor_value = (infor_value >> 1) | 0x80000000;
			receive_num++;
		}
		if(receive_num == COSTOM_DATA_BITS_NUM)
		{
			receive_num = 0;
			receive_status = 2;
		}
	}
	
	// end code 
	if(receive_status == 2)
	{
		if((IR_STOP_HIGH_LEVEL_MIN<(test_time1 - test_time0)) && ((test_time1 - test_time0)<IR_STOP_HIGH_LEVEL_MAX))
		{
			receive_status = 3;
		}
		
	}

	//repeat code
	if(receive_status == 3)
	{
		if((IR_LEADER_END_LEVEL_MIN < (test_time1 - test_time0)) && ((test_time1 - test_time0) < IR_LEADER_END_LEVEL_MAX))
		{
			receive_repeat++;
			os_sem_post(ir_data_process);
		}	
	}	
	
}


void ir_init(void)
{
	chip_clk_enable((unsigned int)CLK_IR);
	chip_clk_enable((unsigned int)CLK_PIT0);

	os_printf(LM_OS, LL_INFO, "ir init ok! \r\n");

}


void drv_ir_init(void)
{
	unsigned int argv = 0;
	unsigned int data = 0;
	T_GPIO_ISR_CALLBACK p_callback;
	p_callback.gpio_callback =(&hal_ir_callback);
	p_callback.gpio_data = &data;

	drv_gpio_init();

	PIN_FUNC_SET(IO_MUX_GPIO20, FUNC_GPIO20_GPIO20);
	
	drv_gpio_ioctrl(GPIO_NUM_20,DRV_GPIO_CTRL_INTR_MODE,DRV_GPIO_ARG_INTR_MODE_N_EDGE);	//interrupt by negative edge
	drv_gpio_ioctrl(GPIO_NUM_20,DRV_GPIO_CTRL_REGISTER_ISR,(int)&p_callback);
	drv_gpio_ioctrl(GPIO_NUM_20,DRV_GPIO_CTRL_INTR_ENABLE,argv);
	os_task_create( "ir_data_process",  4,2048, (task_entry_t)hal_ir_data_process, NULL);
	ir_data_process = os_sem_create(32, 0);
}




