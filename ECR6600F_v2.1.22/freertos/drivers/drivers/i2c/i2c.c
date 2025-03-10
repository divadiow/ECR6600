/**
*@file	   i2c.c
*@brief    This part of program is I2C algorithm Software interface
*@author   weifeiting
*@data     2021.1.29
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#include "i2c.h"  
#include "chip_memmap.h"
#include "chip_pinmux.h"
#include "chip_irqvector.h"
#include "arch_irq.h"
#include "chip_smupd.h"
#include "dma.h"
#include "chip_clk_ctrl.h"
#include "oshal.h"
#include "string.h"


/**
*@brief   I2C register structure 
*/
typedef struct _T_I2C_REG_MAP{
	volatile unsigned int	IdRev;		//0x00, ID and Revision Register
	volatile unsigned int	rev0[3];	//0x04~0x0C, reserved reg
	volatile unsigned int	Cfg;		//0x10, Configuration Register
	volatile unsigned int	IntEn;		//0x14, Interrupt Enable Register
	volatile unsigned int	Status;		//0x18, Status Register
	volatile unsigned int	Addr;		//0x1C, Addr Register
	volatile unsigned int	Data;		//0x20, Data Register
	volatile unsigned int	Ctrl;		//0x24, Control Register
	volatile unsigned int	Cmd;		//0x28, Command Register	
	volatile unsigned int	Setup;		//0x2C, Setup Register
	volatile unsigned int	TPM;		//0x30, I2C Timing Parameter Multiplier Register
}T_I2C_REG_MAP;

/** I2C_cmd_definition*/
#define I2C_CMD_NoAction        0	///< 0:no action
#define I2C_CMD_IssueDataTrans  1	///< 1:issue a data transaction (Master only)
#define I2C_CMD_RespWithACK     2	///< 2:respond with an ACK to the received byte
#define I2C_CMD_RespWithNACK    3	///< 3:respond with a NACK to the received byte
#define I2C_CMD_ClearFIFO       4	///< 4:clear the FIFO
#define I2C_CMD_Reset           5	///< 5:reset the I 2 C controller

/** I2C_direction_definition*/
#define I2C_Direction_Transmitter     0x000		///< 0:master Transmitter
#define I2C_Direction_Receiver        0x100		///< 0x100:master Receiver
#define I2C_Dir_Slave_Transmitter     0x100		///< 0x100:slave Transmitter
#define I2C_Dir_Slave_Receiver        0x000		///< 0x0:slave Receiver

/** I2C_flags_definition */
#define I2C_FLAG_FIFOEmpty               0x01	///< 0x01:Indicates that the FIFO is empty
#define I2C_FLAG_FIFOFull                0x02	///< 0x02:Indicates that the FIFO is full
#define I2C_FLAG_FIFOHalf                0x04	///< 0x04:Transmitter: Indicates that the FIFO is half-empty.Receiver: Indicates that the FIFO is half-full
#define I2C_FLAG_AddrHit                 0x08	///< 0x08:Master: indicates that a slave has responded to the transaction.Slave: indicates that a transaction is targeting the controller
#define I2C_FLAG_ArbLose                 0x10	///< 0x10:Indicates that the controller has lost the bus arbitration(master mode only)
#define I2C_FLAG_Stop                    0x20	///< 0x20:Indicates that a STOP Condition has been transmitted/received
#define I2C_FLAG_Start                   0x40	///< 0x40:Indicates that a START Condition or a repeated START condition has been transmitted/received
#define I2C_FLAG_ByteTrans               0x80	///< 0x80:Indicates that a byte of data has been transmitted
#define I2C_FLAG_ByteRecv                0x100	///< 0x100:Indicates that a byte of data has been received
#define I2C_FLAG_Cmpl                    0x200	///< 0x200:Transaction Completion
#define I2C_FLAG_ACK                     0x400	///< 0x400:Indicates the type of the last received/transmitted acknowledgement bit
#define I2C_FLAG_BusBusy                 0x800	///< 0x800:Indicates that the bus is busy
#define I2C_FLAG_GenCall                 0x1000	///< 0x1000:Indicates that the address of the current transaction is a general call address
#define I2C_FLAG_LineSCL                 0x2000	///< 0x2000:Indicates the current status of the SCL line on the bus
#define I2C_FLAG_LineSDA                 0x4000	///< 0x4000:Indicates the current status of the SDA line on the bus
#define I2C_FLAG_All                     0x7FFF	///< 0x7FFF:All status is set to 1

/** I2C_interrupts_definition  */
#define I2C_IT_Disable                   0x00	///< 0x00:Clear interrupt
#define I2C_IT_FIFOEmpty                 0x01	///< 0x01:Set to enable the FIFO Empty Interrupt
#define I2C_IT_FIFOFull                  0x02	///< 0x02:Set to enable the FIFO Full Interrupt
#define I2C_IT_FIFOHalf                  0x04	///< 0x04:Set to enable the FIFO Half Interrupt
#define I2C_IT_AddrHit                   0x08	///< 0x08:Set to enable the Address Hit Interrupt
#define I2C_IT_ArbLose                   0x10	///< 0x10:Set to enable the Arbitration Lose Interrupt
#define I2C_IT_Stop                      0x20	///< 0x20:Set to enable the STOP Condition Interrupt
#define I2C_IT_Start                     0x40	///< 0x40:Set to enable the START Condition Interrupt
#define I2C_IT_ByteTrans                 0x80	///< 0x80:Set to enable the Byte Transmit Interrupt
#define I2C_IT_ByteRecv                  0x100	///< 0x100:Set to enable the Byte Receive Interrupt
#define I2C_IT_Cmpl                      0x200	///< 0x200:Set to enable the Completion Interrupt
#define I2C_IT_All                       0x03FF

/** I2C variable definition */
#define APB_CLOCK   24000000
#define APB_CLOCK_BASE (APB_CLOCK/1000000)
#define APB_MOD_BASE 1000
#define I2C_MAX(a,b) (a)>=(b)?(a):(b)
#define I2C_WAIT_FOREVER 0xffffffff
#define DRV_I2C_FIFO_DEPTH		4
#define I2C_BUF_SIZE	1024


volatile unsigned char completed=0;
static unsigned char dma_i2c_src[I2C_BUF_SIZE] __attribute__((section(".dma.data")));
static unsigned char dma_i2c_dst[I2C_BUF_SIZE] __attribute__((section(".dma.data")));

typedef struct _T_DRV_I2C_DEV
{
    T_I2C_REG_MAP * i2c_reg_base;
    unsigned int i2c_dma_chan;
    unsigned int i2c_init_flag;
    os_sem_handle_t i2c_process;
} T_DRV_I2C_DEV;

static T_DRV_I2C_DEV g_i2c_dev;

/**
@brief   Write this register with the following values to perform the corresponding actions
@param[in]  cmd:Enter different cmd to execute different commands,cmd corresponds to the above I2C_cmd_definition
*/
void I2C_Cmd(unsigned	char  cmd)
{
	g_i2c_dev.i2c_reg_base->Cmd =  cmd;
}

/**
@brief   Write this register with the following values to perform the corresponding actions
@param[in]  dir:I2C transmission direction
@param[in]  count: transmist length
*/
void I2C_TransConfig(unsigned short int dir, unsigned char count)
{
	unsigned int temp=0x1E00;
	temp |= dir | count;
	g_i2c_dev.i2c_reg_base->Ctrl = temp;
}

/**
@brief   Read the current interrupt status of I2C
@param[in]  I2C_FLAG: Different states
*/
unsigned int I2C_GetFlag(unsigned int I2C_FLAG)
{
	return  ((g_i2c_dev.i2c_reg_base->Status) & I2C_FLAG);
}

/**
@brief   clear interrupt status 
@param[in]  I2C_FLAG: Different states
*/
void I2C_ClearFlag(unsigned int I2C_FLAG)
{
	g_i2c_dev.i2c_reg_base->Status =  I2C_FLAG;
}

/**
@brief   Judgment completion interrupt,and clear the completion interrupt status
*/
void I2C_Flag_cmpl(void)
{
	while(I2C_GetFlag(I2C_FLAG_Cmpl) != I2C_FLAG_Cmpl); 
	I2C_ClearFlag(I2C_FLAG_Cmpl); 
}

/**
@brief   i2c dma callback
*/
void i2c_dma_callback_isr(void *data)
{
	os_sem_post(g_i2c_dev.i2c_process);
}

/**
@brief   When fifo is empty in tx, enter interrupt
*/
void i2c_tx_isr(void)
{
#if 0
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
	T_I2C_REG_MAP * p_i2c_reg = (T_I2C_REG_MAP * )MEM_BASE_I2C;
	arch_irq_clean(VECTOR_NUM_I2C);
	unsigned int status,tx_depth;
	unsigned int i=0;
	status = I2C_GetFlag(I2C_FLAG_All);
	tx_depth = DRV_I2C_FIFO_DEPTH;
	p_i2c_reg->IntEn = 0x0;
	
	while(tx_depth)
	{
		os_printf(LM_OS, LL_INFO, "FIFO empty1111\r\n");
		tx_depth--;
		//while(I2C_GetFlag(I2C_FLAG_FIFOFull) == I2C_FLAG_FIFOFull); 
		p_i2c_reg->Data = dma_i2c_src[i++];
		tx_remain_length--;
		if(tx_remain_length == 0)
		{
			os_sem_post(i2c_process);
			return;
		}
	}
	dma_i2c_src += DRV_I2C_FIFO_DEPTH;
	p_i2c_reg->IntEn = I2C_FLAG_FIFOEmpty;

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
#endif
}

/**
@brief  I2C initial configuration
@details Mainly configure the transmission speed,
		 master/slave mode, address bits, etc.
@return	   -2--i2c setup speed is not 100Kbps\400Kbps\1Mbps, 
		   -1--i2c dma mode allocate channel failure,
		    0--i2c init config success.
*/
int  i2c_init_cfg(I2C_InitTypeDef* I2C_InitStruct)
{
	unsigned int temp =0;
    unsigned char t_sudat = 0;
    unsigned char t_sp = 0;
    unsigned char t_hddat = 0;
    unsigned char t_sclratio = 0;
    unsigned int t_sclhi = 0;
	unsigned long flags = arch_irq_save();
	g_i2c_dev.i2c_reg_base = (T_I2C_REG_MAP * )MEM_BASE_I2C;

	if (g_i2c_dev.i2c_init_flag == 1)
	{
		arch_irq_restore(flags);
		return 0;
	}
	else
	{
		g_i2c_dev.i2c_init_flag = 1;
	}

	chip_clk_enable(CLK_I2C);

	chip_i2c_pinmux_cfg(I2C_InitStruct->scl_num, I2C_InitStruct->sda_num);
	g_i2c_dev.i2c_process = os_sem_create(1, 0);
	/*reset*/;
	I2C_Cmd(I2C_CMD_Reset); 	  
	os_usdelay(10);
	I2C_Cmd(I2C_CMD_NoAction);

	if(I2C_InitStruct->I2C_Mode == I2C_Mode_Master)
	{
		if(I2C_InitStruct->I2C_Speed == I2C_Speed_100K)
		{
			t_sp = 0;
			t_sclratio = 1;
			t_sudat = ((250*APB_CLOCK_BASE)/APB_MOD_BASE) - 4 - t_sp;
            t_hddat = ((300*APB_CLOCK_BASE)/APB_MOD_BASE) - 4 - t_sp;
			t_sclhi = (APB_CLOCK/I2C_Speed_100K - 8 - 2 * t_sp)/(1 + t_sclratio);
		}
		else if(I2C_InitStruct->I2C_Speed == I2C_Speed_400K)
		{
			t_sp = 0;
			t_sclratio = 2;
			t_sudat = ((100*APB_CLOCK_BASE)/APB_MOD_BASE) - 4 - t_sp;
            t_hddat = ((300*APB_CLOCK_BASE)/APB_MOD_BASE) - 4 - t_sp;
			t_sclhi = (APB_CLOCK/I2C_Speed_400K - 8 - 2 * t_sp)/(1 + t_sclratio);
		}
		else if(I2C_InitStruct->I2C_Speed == I2C_Speed_1M)
		{
			t_sp = 1;
			t_sclratio = 2;
			t_sudat = ((50*APB_CLOCK_BASE)/APB_MOD_BASE) - 4 - t_sp;
            t_hddat = 0;
            t_sclratio = 1;	
			t_sclhi = (APB_CLOCK/I2C_Speed_1M - 8 - 2 * t_sp)/(1 + t_sclratio);
		}
		else
		{
			os_printf(LM_OS, LL_ERR, "i2c setup speed don't support\r\n");
			arch_irq_restore(flags);
			return -2;
		}
		
		temp = (t_sudat<<24) | (t_sp<<21) | (t_hddat<<16) | (t_sclratio<<13) | (t_sclhi<<4)
			| I2C_InitStruct->I2C_DMA | I2C_InitStruct->I2C_AddressBit | I2C_InitStruct->I2C_Mode;
		temp |= 0x1;
		g_i2c_dev.i2c_reg_base->Setup = temp;
	}
	else
	{
		temp = I2C_InitStruct->I2C_DMA | I2C_InitStruct->I2C_Mode;
		temp |= 0x1;
		g_i2c_dev.i2c_reg_base->Setup = temp;
	}

	if(I2C_InitStruct->I2C_DMA == I2C_DMA_Enable)
	{
		g_i2c_dev.i2c_dma_chan=drv_dma_ch_alloc();
		if (g_i2c_dev.i2c_dma_chan < 0)
		{
			arch_irq_restore(flags);
			return -1;
		}
		drv_dma_isr_register(g_i2c_dev.i2c_dma_chan, i2c_dma_callback_isr, NULL);
		
	}
	else
	{
		#if 0
		arch_irq_register(VECTOR_NUM_I2C, (void *)i2c_tx_isr);
		arch_irq_clean(VECTOR_NUM_I2C);
		arch_irq_unmask(VECTOR_NUM_I2C);
		#endif
	}
	arch_irq_restore(flags);
	return 0;
}

/**
@brief  I2C close
@details Mainly release the DMA channel,clear init flag,and stop I2C
@return	    0--i2c close.
*/
int drv_i2c_close()
{
	unsigned long flags = arch_irq_save();
	if (g_i2c_dev.i2c_init_flag == 0)
	{
		arch_irq_restore(flags);
		return 0;
	}
	else
	{
		g_i2c_dev.i2c_init_flag = 0;
	}

	drv_dma_stop(g_i2c_dev.i2c_dma_chan);
	chip_clk_disable(CLK_I2C);
	if(g_i2c_dev.i2c_process)
	{
	    os_sem_destroy(g_i2c_dev.i2c_process);
	}
	arch_irq_restore(flags);
	return 0;
}

/**
@brief     Statement i2c master transmit data
@details    When I2C master receive data , First, the host needs to send cmd to the slave first, 		
			and then read a certain length of data
@param[in]  *slave_addr: slave device address
@param[in]  *bufptr: The data read by the master is stored in this data
@param[in]  len: The length of the data the host wants to read
@param[in]  cmd_len:Command length of slave
@return	   -1--i2c bus is busy,
		   -2--i2c set up receive length error,
		    0--i2c read process completion.
*/
int i2c_master_read(unsigned int slave_addr,unsigned char *bufptr, unsigned int len,unsigned int cmd_len)
{
	if(len >255 || len <0 )
	{
		os_printf(LM_OS, LL_ERR, "Tansfer length Error\n");
		return -2;
	}

	g_i2c_dev.i2c_reg_base->Addr = slave_addr;
	unsigned int i; 
	if(g_i2c_dev.i2c_reg_base->Status & BIT(11))
		return -1;
	if(i2c_master.I2C_DMA==I2C_DMA_Disable)
	{
	    unsigned long flags = arch_irq_save();
		if(cmd_len)
		{
			os_msdelay(1);
			I2C_TransConfig(I2C_Direction_Transmitter, cmd_len); 
			I2C_Cmd(I2C_CMD_IssueDataTrans);
			while(I2C_GetFlag(I2C_FLAG_FIFOFull) == I2C_FLAG_FIFOFull); 
			g_i2c_dev.i2c_reg_base->Data = *bufptr++;
			while(I2C_GetFlag(I2C_FLAG_Cmpl) != I2C_FLAG_Cmpl); 
			I2C_ClearFlag(I2C_FLAG_Cmpl); 
		}
		os_msdelay(2);
		I2C_TransConfig(I2C_Direction_Receiver, len);	
		I2C_Cmd(I2C_CMD_IssueDataTrans);
		for(i=0;i<len;i++)	
		{
			while(I2C_GetFlag(I2C_FLAG_FIFOEmpty) == I2C_FLAG_FIFOEmpty); 
			*(bufptr++)=g_i2c_dev.i2c_reg_base->Data;
		}
        arch_irq_restore(flags);
	}
	else
	{
		memcpy(dma_i2c_dst, bufptr, cmd_len);
		T_DMA_CFG_INFO	i2c_tx_cfg;
		T_DMA_CFG_INFO	i2c_rx_cfg;
		if(cmd_len)
		{
			i2c_tx_cfg.src=(unsigned int)dma_i2c_dst;
			i2c_tx_cfg.dst=(unsigned int)&g_i2c_dev.i2c_reg_base->Data;
			i2c_tx_cfg.len=cmd_len;
			i2c_tx_cfg.mode=DMA_CHN_I2C_TX;
			drv_dma_cfg(g_i2c_dev.i2c_dma_chan,  &i2c_tx_cfg );
			drv_dma_start(g_i2c_dev.i2c_dma_chan);
			I2C_TransConfig(I2C_Direction_Transmitter,1); 
			I2C_Cmd(I2C_CMD_IssueDataTrans);
			os_sem_wait(g_i2c_dev.i2c_process, I2C_WAIT_FOREVER);
			os_msdelay(1);
		}
		drv_dma_status_clean(g_i2c_dev.i2c_dma_chan);
		i2c_rx_cfg.src=(unsigned int)&g_i2c_dev.i2c_reg_base->Data;
		i2c_rx_cfg.dst=(unsigned int)(dma_i2c_dst+cmd_len);
		i2c_rx_cfg.len=len;
		i2c_rx_cfg.mode=DMA_CHN_I2C_RX;
		drv_dma_cfg(g_i2c_dev.i2c_dma_chan,  &i2c_rx_cfg );
		drv_dma_start(g_i2c_dev.i2c_dma_chan);
		I2C_TransConfig(I2C_Direction_Receiver, len);
		I2C_Cmd(I2C_CMD_IssueDataTrans);
		os_sem_wait(g_i2c_dev.i2c_process, I2C_WAIT_FOREVER);
		os_msdelay(2);
		memcpy(bufptr+cmd_len,dma_i2c_dst+cmd_len,len);
	}
	return 0;
}

/**
@brief     Statement i2c master transmit data
@details    	
@param[in]  *slave_addr :slave device address
@param[in]  *bufptr : Buffer for sending data
@param[in]  len : The length of the data sent by the host
@return	   -1--i2c bus is busy, 
		   -2--i2c trnsfer length error,
		    0--i2c write process completion.
*/
int i2c_master_write(unsigned int slave_addr,unsigned char *bufptr, unsigned int len)
{
	if(len >255 || len <0 )
	{
		os_printf(LM_OS, LL_ERR, "Tansfer length Error\n");
		return -2;
	}

	g_i2c_dev.i2c_reg_base->Addr = slave_addr;
	unsigned int i = 0;

	if(g_i2c_dev.i2c_reg_base->Status & BIT(11))
		   return -1;

	
	if(i2c_master.I2C_DMA == I2C_DMA_Disable)
	{
	    unsigned long flags = arch_irq_save();
		I2C_TransConfig(I2C_Direction_Transmitter, len); 
		I2C_Cmd(I2C_CMD_IssueDataTrans);

		for(i=0;i < len;i++)
		{
			while(I2C_GetFlag(I2C_FLAG_FIFOFull) == I2C_FLAG_FIFOFull); // wait for fifo not full
			g_i2c_dev.i2c_reg_base->Data = *bufptr++;
		}
		I2C_Flag_cmpl();
        arch_irq_restore(flags);
	}
	else
	{
		memcpy(dma_i2c_src, bufptr, len);
		T_DMA_CFG_INFO	i2c_tx_cfg;
		i2c_tx_cfg.src = (unsigned int)dma_i2c_src;
		i2c_tx_cfg.dst = (unsigned int)&g_i2c_dev.i2c_reg_base->Data;
		i2c_tx_cfg.len = len;
		i2c_tx_cfg.mode = DMA_CHN_I2C_TX;
		drv_dma_cfg(g_i2c_dev.i2c_dma_chan,  &i2c_tx_cfg );
		drv_dma_start(g_i2c_dev.i2c_dma_chan);
		I2C_TransConfig(I2C_Direction_Transmitter,len); 
		I2C_Cmd(I2C_CMD_IssueDataTrans);
		os_sem_wait(g_i2c_dev.i2c_process, I2C_WAIT_FOREVER);
		os_msdelay(2);
	}
	return 0;
}

/**
@brief     Statement i2c slave transmit data
@details    	
@param[in]  *bufptr : Buffer for sending data
@param[in]  len : The length of the data sent by the host
*/
void i2c_slave_write(unsigned char *bufptr, unsigned int len)
{
	unsigned int i;

	I2C_TransConfig(I2C_Dir_Slave_Transmitter, len); 
	I2C_Cmd(I2C_CMD_IssueDataTrans);

	for(i=0;i < len;i++)
	{
		while(I2C_GetFlag(I2C_FLAG_FIFOFull) == I2C_FLAG_FIFOFull); // wait for fifo not full
		g_i2c_dev.i2c_reg_base ->Data =  *bufptr++;
	}
	while(completed==0);// wait transaction complete
	completed=0;
	I2C_Flag_cmpl();
}

