#include "chip_memmap.h"

#ifndef BIT
#define BIT(x) (1 << (x))
#endif


//#ifndef inw
//#define inw(reg)        (*((volatile unsigned int *) (reg)))
//#endif
//
//#ifndef outw
//#define outw(reg, data) ((*((volatile unsigned int *)(reg)))=(unsigned int)(data))
//#endif


#define IR_BASE                		MEM_BASE_IR  
#define IR_LENGTH                	0x80000						//0x00210000-0x00280000
#define CMD_NUM						4

/* ir register address*/
#define IR_CARRIER               	(IR_BASE + 0x00)
#define IR_LOGICBIT               	(IR_BASE + 0x04)
#define IR_CTRL               		(IR_BASE + 0x08)
#define IR_FIFOCLR              	(IR_BASE + 0x0C)
#define IR_STATUS              		(IR_BASE + 0x10)
#define IR_INTSTAT              	(IR_BASE + 0x14)
#define IR_COMCMD              		(IR_BASE + 0x18)
#define IR_REPCMD              		(IR_BASE + 0x1C)

/* 0x00 - IR_CARRIER register */
#define IR_CARRIER_pwmfmt_low       BIT(22)						//1:from low to high  '0:from high to low
#define IR_CARRIER_invtime(x)       (((x) & 0x7FF) << 11)		//invert output level at invtime
#define IR_CARRIER_period(x)       	(((x) & 0x7FF) << 0)		//defines the carrier period in ir_clk cycles

/* 0x04 - IR_LOGICBIT register */
#define IR_LOGICBIT_zero_lh(x)      (((x) & 0xFF) << 24)		//Defines the last half duration of logic zero in carrier clock cycles. Must be >0
#define IR_LOGICBIT_zero_fh(x)      (((x) & 0xFF) << 16)		//Defines the first half duration of logic zero in carrier clock cycles. Must be >0 
#define IR_LOGICBIT_one_lh(x)       (((x) & 0xFF) << 8)		    //Defines the last half duration of logic one in carrier clock cycles. Must be >0
#define IR_LOGICBIT_one_fh(x)       (((x) & 0xFF) << 0)			//Defines the first half duration of logic one in carrier clock cycles. Must be >0 

/* 0x08 - IR_CTRL register */
#define IR_CTRL_reptime(x)          (((x) & 0x1FFF) << 8)		//repeat code interval,in carrier clock cycles
#define IR_CTRL_cmdie            	BIT(7)						//interrupt at each command transmit done [0:disable] [1:enable]
#define IR_CTRL_irie            	BIT(6)						//0:disable interrupt when common cmd FIFO transmit done 1:enable interrupt when common cmd FIFO transmit done
#define IR_CTRL_zerofmt            	BIT(5)						//0:mark first 1:space first
#define IR_CTRL_onefmt            	BIT(4)						//0:mark first 1:space first
#define IR_CTRL_irinvout            BIT(3)						//0:not invert output level  1:invert output level
#define IR_CTRL_mode           		BIT(2)						//0:IR mode  1:PWM mode
#define IR_CTRL_repen           	BIT(1)						//enable repeat code transmition
#define IR_CTRL_iren	           	BIT(0)						//enable ir transmition

/* 0x0C - IR_FIFOCLR register */
#define IR_FIFOCLR_rcmdfifoclr      BIT(1)						//clear repeat comand fifo
#define IR_FIFOCLR_ccmdfifoclr      BIT(0)						//clear common comand fifo 

/* 0x10 - IR_STATUS register */
#define IR_STATUS_irbusy      		BIT(8)						//ir busy
#define IR_STATUS_rcmdfifocnt(x)    (((x) & 0xF) << 4)			//count in repeat command fifo
#define IR_STATUS_ccmdfifocnt(x)    (((x) & 0xF) << 4)			//count in common command fifo

/* 0x14 - IR_INTSTAT register */
#define IR_INTSTAT_cmd_done         BIT(1)						//flag set when each cmd transmit done
#define IR_INTSTAT_trans_done       BIT(0)						//flag set when common cmd FIFO transmit done

/* 0x18 - IR_COMCMD_dr register */
/*data to transmit,write into common command fifo*/
#define IR_COMCMD_cmd_type   		BIT(25)		      			//bit number 
/*cmd type 1:*/
#define IR_COMCMD_first_ht(x)    	(((x) & 0xFFF) << 0)        //first half time    
#define IR_COMCMD_last_ht(x)    	(((x) & 0xFFF) << 12)       //last half time      
#define IR_COMCMD_space_first   	BIT(24)		      			//mark first or not: 0 mark first,1 space first 

/*cmd type 0:*/
#define IR_COMCMD_bit_sequence(x)   (((x) & 0xFFFF) << 0)	     //bit sequence
#define IR_COMCMD_bit_number(x)   	(((x) & 0xF) << 16)	      	//bit number 

/* 0x1C - IR_REPCPM register */
/*data to transmit,write into common command fifo*/
#define IR_REPCMD_cmd_type   		BIT(25)		      			//bit number 
/*cmd type 1:*/
#define IR_REPCMD_first_ht(x)    	(((x) & 0xFFF) << 0)        //first half time    
#define IR_REPCMD_last_ht(x)    	(((x) & 0xFFF) << 12)       //last half time      
#define IR_REPCMD_space_first   	BIT(24)		      			//mark first or not: 0 mark first,1 space first 

/*cmd type 0:*/
#define IR_REPCMD_bit_sequence(x)   (((x) & 0xFF) << 0)	      	//bit sequence
#define IR_REPCMD_bit_number(x)   	(((x) & 0xF) << 16)	      	//bit number 



#define IR_KEY_1				0xff00
#define IR_KEY_2				0xfe01
#define IR_KEY_3				0xfd02
#define IR_KEY_4				0xfc03
#define IR_KEY_5				0xfb04
#define IR_KEY_6				0xfa05
#define IR_KEY_7				0xf906
#define IR_KEY_8				0xf807
#define IR_KEY_9				0xf708
#define IR_KEY_10				0xf609
#define IR_KEY_11				0xf50a
#define IR_KEY_12				0xf40b
#define IR_KEY_13				0xf30c
#define IR_KEY_14				0xf20d
#define IR_KEY_15				0xf10e
#define IR_KEY_16				0xf00f
#define IR_KEY_17				0xef10
#define IR_KEY_18				0xee11
#define IR_KEY_19				0xed12
#define IR_KEY_20				0xec13
#define IR_KEY_21				0xeb14
#define IR_KEY_22				0xea15
#define IR_KEY_23				0xe916
#define IR_KEY_24				0xe817



#define COSTOM_CODE_HBS838					0xef00
#define COSTOM_DATA_BITS_NUM				32


#define IR_LEADER_HIGH_LEVEL_MAX			13990   	//13.59ms
#define IR_LEADER_HIGH_LEVEL_MIN			13090   

#define IR_LEADER_END_LEVEL_MAX				11990   	//11.32079ms
#define IR_LEADER_END_LEVEL_MIN				11000		

#define IR_LEADER_EMPTY_LEVEL_MAX			105000   	//95.592ms
#define IR_LEADER_EMPTY_LEVEL_MIN			85000		



#define IR_INFOR_HIGH_LEVEL_MAX				2500   		//2.25ms
#define IR_INFOR_HIGH_LEVEL_MIN				2000
#define IR_INFOR_LOW_LEVEL_MAX				1300		//1.125ms
#define IR_INFOR_LOW_LEVEL_MIN				1000

#define IR_STOP_HIGH_LEVEL_MAX				44000   	//41.71ms
#define IR_STOP_HIGH_LEVEL_MIN				40000   	

#define IR_CLK_16M  0x461a5	 	//IR clck = 16mhz  carrier is 38khz
#define IR_CLK_40M  0xaf41c	 	//IR clck = 40mhz  carrier is 38khz
#define IR_CLK_48M  0xd2cf0	 	//IR clck = 48mhz  carrier is 38khz
#define IR_CLK_32M	0x8c34a		//IR clck = 32mhz  carrier is 38khz
#define IR_CLK_24M  0x69278	 	//IR clck = 24mhz  carrier is 38khz
#define IR_CLK_12M  0x3493c	 	//IR clck = 12mhz  carrier is 38khz
#define IR_CLK_10M  0x2b907	 	//IR clck = 10mhz  carrier is 38khz

#define CLC_NUM 	0.0416

void ir_init(void);
void ir_trans_config(void);
void ir_trans_api(unsigned int cmd_num);
void drv_ir_init(void);
unsigned int hal_read_timestamp(void);


