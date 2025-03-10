/**
* @file       uart.c
* @brief      uart driver code  
* @author     Wangchao
* @date       2021-1-22
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#include <string.h>
#include "arch_irq.h"
#include "chip_irqvector.h"
#include "chip_configuration.h"
#include "chip_clk_ctrl.h"
#include "uart.h"
#include "oshal.h"
#include "dma.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#include "rtc.h"
#endif

DIST_STA_IP
T_DRV_UART_CONFIG uart_dev_config[3]={{0}}; //uart0/1/2

#ifdef CONFIG_FPGA
#define UART_RATE(n)		(40000000 / (n) / 8 )
#else

#if defined (CONFIG_CPU_CLK_SRC_40m)
#define UART_RATE(n)		( 40000000 / (n) / 8 )
#else
#define UART_RATE(n)		( 80000000 / (n) / 8 )
#endif

#endif


/**********************************************************
 *
 *				   uart hardware struct & info
 *
 **********************************************************/ 

/** UART ISR ID */
#define DRV_UART_ISR_MASK				0xF
#define DRV_UART_ISR_MODEM_STATUS	0x0
#define DRV_UART_ISR_TX_FINISH			0x2
#define DRV_UART_ISR_RX_TIMEOUT		0xC
#define DRV_UART_ISR_RX_READY			0x4
#define DRV_UART_ISR_LINE_STATUS		0x6

#define DRV_UART_LSR_RDR			0x01 	/** Data Ready */
#define DRV_UART_LSR_THRE			0x20 	/** THR/FIFO Empty */
#define DRV_UART_LSR_TEMT			0x40	 /** THR/TSR Empty */

#define DRV_UART_IER_ETHEI			0x02 	/** Enable transmitter holding register interrupt*/
#define DRV_UART_IER_ERBII			0x01 	/** Enable received data available interrupt and the character timerout interrupt*/

#define DRV_UART_LCR_DLAB			0x00000080

#define DRV_UART_FCR_RFIFOT(X)		((X)<<6)	/** FIFO RESET*/
#define DRV_UART_FCR_FIFORST		0x00000007	/** FIFO RESET*/
#define DRV_UART_FCR_DMAE			0x00000008	/** dma enable*/

#define DRV_UART_MCR_AFE			(0x22)		/** auto flow control enable*/

#define DRV_UART_FIFO_DEPTH		(CHIP_CFG_UART_FIFO_DEPTH)

#define SOFTWARE_RESET_REG          0x00202014

#define DRV_UART0_RESET_ON          (1<<15)
#define DRV_UART1_RESET_ON          (1<<16)
#define DRV_UART2_RESET_ON          (1<<17)

#define UART_DIVISOR_VALUE_13   13
#define UART_DIVISOR_VALUE_11   11
#define UART_DIVISOR_VALUE_3    3
#define UART_OSCR_8            8
#define UART_OSCR_18           18

/**
 * @brief Uart register map.
 */
typedef  struct _T_UART_REG_MAP
{
	volatile unsigned int IdRev;		///< 0x00, ID and Revision Register
	volatile unsigned int Resv0[3];		///< 0x04~0x0C, Reserved 
	volatile unsigned int Cfg;			///< 0x10, Hardware Configure Register
	volatile unsigned int OSCR;			///< 0x14, Over Sapmple Control Register	
	volatile unsigned int Resv1[2];		///< 0x18~0x1C, Reserved 

	union 
	{
		volatile unsigned int RBR;		///< 0x20, DLAB=0, Receiver Buffer Register (Read only) 
		volatile unsigned int THR;		///< 0x20, DLAB=0, Transmitter Holding Register (Write only) 
		volatile unsigned int DLL;		///< 0x20, DLAB=1, Divisor Latch LSB 
	} Mux0;

	union 
	{
		volatile unsigned int IER;		///< 0x24, DLAB=0, Interrupt Enable Register 
		volatile unsigned int DLM;		///< 0x24, DLAB=1, Divisor Latch MSB  
	} Mux1;

	union 
	{
		volatile unsigned int IIR;		///< 0x28, Interrupt Identification Register (Read only) 
		volatile unsigned int FCR;		///< 0x28, FIFO Control Register (Write only) 
	} Mux2;

	volatile unsigned int LCR;			///< 0x2C, Line Control Register 
	volatile unsigned int MCR;			///< 0x30, Modem Control Register 
	volatile unsigned int LSR;			///< 0x34, Line Status Register
	volatile unsigned int MSR;			///< 0x38, Modem Status Register 
	volatile unsigned int SCR;			///< 0x3C, Scratch Register 
} T_UART_REG_MAP;


/**
 * @brief Uart device.
 */
typedef struct _T_DRV_UART_DEV
{
	T_UART_REG_MAP * uart_reg_base;
	unsigned int uart_vector_num;

	/** tx parameter  */
	os_mutex_handle_t uart_tx_mutex;
	os_sem_handle_t uart_tx_sem;
	unsigned char uart_tx_fifo_depth;
	unsigned char uart_tx_mode;
	unsigned char uart_tx_dma_chn;	
	char * uart_tx_addr;
	unsigned int uart_tx_len;

	/** rx parameter  */
	os_sem_handle_t uart_rx_sem;
	unsigned char uart_rx_mode;
	unsigned char uart_rx_overrun;
	unsigned char uart_rx_dma_chn;		
	char * uart_rx_user_addr;
	char * uart_rx_buf_addr;
	unsigned int uart_rx_buf_size;
	unsigned int uart_rx_buf_rd;
	unsigned int uart_rx_buf_wr;
	unsigned int uart_rx_isr_threshold;
#ifdef USE_NEW_USER_MODE
	T_UART_ISR_CALLBACK uart_rx_callback;
#else
	UART_CALLBACK uart_rx_callback;
#endif
} T_DRV_UART_DEV;

static T_DRV_UART_DEV *uart_dev[E_UART_NUM_MAX];



/**    @brief       Uart set baud.
 *     @details     Configure the desired baud rate to MSB and LSB.
 *     @param[in]   *p_uart_dev   Uart device structure pointer 
 *     @param[in]   baud   Desired baud rate
 */
static void drv_uart_set_baud(T_DRV_UART_DEV * p_uart_dev, unsigned int baud)
{
	T_UART_REG_MAP * p_uart_reg = p_uart_dev->uart_reg_base;
	int uart_baud = UART_RATE(baud);

#if !defined (CONFIG_CPU_CLK_SRC_40m)
	if(baud == BAUD_RATE_806400)
	{
		uart_baud = UART_DIVISOR_VALUE_13;
	}

	if(baud == BAUD_RATE_921600)
	{
		uart_baud = UART_DIVISOR_VALUE_11;
	}

	if(baud == BAUD_RATE_1500000)
	{
		p_uart_reg->OSCR = UART_OSCR_18;
		uart_baud = UART_DIVISOR_VALUE_3;
	}
	else
#endif
	{
		p_uart_reg->OSCR = UART_OSCR_8;
	}

	p_uart_reg->LCR = p_uart_reg->LCR | DRV_UART_LCR_DLAB;
	p_uart_reg->Mux0.DLL = (uart_baud >> 0) & 0xff;
	p_uart_reg->Mux1.DLM = (uart_baud >> 8) & 0xff;
	p_uart_reg->LCR = p_uart_reg->LCR & (~DRV_UART_LCR_DLAB);
}


/**    @brief       Uart rx buf update.
 *     @details     Update and return the set received length minus the length of data to be received in the buffer.
 *     @param[in]   *p_uart_dev    Uart device structure pointer 
 *     @param[in]   len            Length of received data
 *     @param[in]   *len_remain0   Pointer to the remaining length that can be read in the buffer
 *     @param[in]   *len_remain1   Pointer to the remaining length that can be read in the buffer
 *	   @return 		left -- the set received length minus the length of data to be received in the buffer
 */
static unsigned int drv_uart_rx_buf_update(T_DRV_UART_DEV * p_uart_dev, unsigned int len, 
										unsigned int *len_remain0, unsigned int *len_remain1)
{

	unsigned int left, remain;
	unsigned long  flags = system_irq_save();

	if (p_uart_dev->uart_rx_buf_wr >= p_uart_dev->uart_rx_buf_rd)
	{
		*len_remain0 = p_uart_dev->uart_rx_buf_wr - p_uart_dev->uart_rx_buf_rd;
		*len_remain1 = 0;
	}
	else
	{
		*len_remain0 = p_uart_dev->uart_rx_buf_size - p_uart_dev->uart_rx_buf_rd;
		*len_remain1 = p_uart_dev->uart_rx_buf_wr;
	}

	remain = *len_remain0 + *len_remain1;

	if (len > remain)
	{
		p_uart_dev->uart_rx_isr_threshold = len - remain;
		p_uart_dev->uart_rx_user_addr += remain;
	}
	else
	{
		p_uart_dev->uart_rx_isr_threshold = 0;
	}

	left =  p_uart_dev->uart_rx_isr_threshold;

	system_irq_restore(flags);

	return left;
}

/**    @brief		Uart rx buf read.
*	   @details 	Uart reads data of length len.
*	   @param[in]	*p_uart_dev   Uart device structure pointer
*	   @param[in]	len           Length of received data
*/
static void drv_uart_rx_buf_read(T_DRV_UART_DEV * p_uart_dev, char * buf, unsigned int len)
{
	unsigned long flags;
	
	memcpy(buf , p_uart_dev->uart_rx_buf_addr + p_uart_dev->uart_rx_buf_rd, len);
	flags = system_irq_save();
	p_uart_dev->uart_rx_buf_rd = (p_uart_dev->uart_rx_buf_rd + len) % p_uart_dev->uart_rx_buf_size;
	system_irq_restore(flags);		
}

/**    @brief		Uart rx reset.
*	   @details 	Uart receive reset.
*	   @param[in]	*p_uart_dev   Uart device structure pointer
*/
static void drv_uart_receive_reset(T_DRV_UART_DEV * p_uart_dev)
{
	unsigned long flags;
	unsigned int  temp;	
	T_UART_REG_MAP * p_uart_reg = p_uart_dev->uart_reg_base;

	if (p_uart_dev->uart_rx_mode == UART_RX_MODE_INTR)
	{
		flags = system_irq_save();
		p_uart_dev->uart_rx_buf_rd = 0;
		p_uart_dev->uart_rx_buf_wr= 0;
		p_uart_dev->uart_rx_overrun = 0;
		temp = p_uart_dev->uart_rx_isr_threshold ;
		//p_uart_dev->uart_rx_isr_threshold = 0;
		p_uart_reg->Mux2.FCR = 2;
		system_irq_restore(flags);

		if (temp)
		{
			os_sem_post(p_uart_dev->uart_rx_sem);
		}
	}
}



/**    @brief       Uart tx ready.
 *     @details     Judge whether txfifo is empty, and put data into txfifo when it is empty.
 *     @param[in]   uart_base   Uart base address
 *     @return      return  1--tx not ready, return 0--tx ready
 */
int drv_uart_tx_ready(unsigned int uart_base)
{
	T_UART_REG_MAP * p_uart_reg = (T_UART_REG_MAP *)uart_base;
	return !(p_uart_reg->LSR & DRV_UART_LSR_THRE);
}

/**    @brief       Uart tx FIFO and tsr empty.
 *     @details     Judge whether txfifo and Transmitter Shift Register are both empty.
 *     @param[in]   uart_base   Uart base address
 *     @return      return  0--tx not ready, return 1--tx ready
 */
int drv_uart_tx_fifo_and_tsr_empty()
{
	int status = 1 ;
	for(int i = 0;i<E_UART_NUM_MAX;i++)
	{
		if (uart_dev[i])
		{			
			T_UART_REG_MAP * p_uart_reg = (T_UART_REG_MAP *)uart_dev[i]->uart_reg_base;
			int flag = (p_uart_reg->LSR & DRV_UART_LSR_TEMT) == 0 ? 0 : 1;
			status &= flag;
		}
	}	
	return status;
}

/**    @brief       Uart tx put.
 *     @details     Put the data into the Transmitter Holding Register and transfer it out.
 *     @param[in]   uart_base  Uart base address
 *     @param[in]   c     Transmit character data
 */
void drv_uart_tx_putc(unsigned int uart_base, char c)
{
	T_UART_REG_MAP * p_uart_reg = (T_UART_REG_MAP *)uart_base;
	p_uart_reg->Mux0.THR = c;
}

/**    @brief       Uart rx get.
 *     @details     Get the value of register Receiver Buffer Register.
 *     @param[in]   uart_base   Uart base address
 *     @return      Returns the value of Receiver Buffer Register
 */
char drv_uart_rx_getc(unsigned int uart_base)
{
	T_UART_REG_MAP * p_uart_reg = (T_UART_REG_MAP *)uart_base;
	return (char)p_uart_reg->Mux0.RBR;
}

/**    @brief       Uart rx ready.
 *     @details     When the return value of the function is 1, it is ready to receive data.
 *     @param[in]   uart_base   Uart base address
 *     @return      return 1--rx ready, return 0--rx not ready
 */
unsigned char drv_uart_rx_tstc(unsigned int uart_base)
{
	T_UART_REG_MAP * p_uart_reg = (T_UART_REG_MAP *)uart_base;
	return p_uart_reg->LSR & DRV_UART_LSR_RDR;
}

/**    @brief       Uart is sent by polling.
 *     @details     The data in the buffer is sent out through the THR register.
 *     @param[in]   uart_num  Specifies the uart number to open, using E_DRV_UART_NUM type 
 *     @param[in]   *buf      The buf pointer points to the address of the data to be transmitted
 *     @param[in]   len       Length of data transmitted
 *     @return      0--Transmit succeed, other--Transmit failed
 */
int  drv_uart_send_poll(E_DRV_UART_NUM uart_num, char * buf, unsigned int len)
{
	unsigned int i = 0;

	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];
	T_UART_REG_MAP * p_uart_reg = p_uart_dev->uart_reg_base;

	while(drv_uart_tx_ready((unsigned int)p_uart_reg));
	while(len)
	{
		if (p_uart_dev->uart_tx_fifo_depth == 0)
		{
			while(drv_uart_tx_ready((unsigned int)p_uart_reg));
			p_uart_dev->uart_tx_fifo_depth = DRV_UART_FIFO_DEPTH;
		}

		while (p_uart_dev->uart_tx_fifo_depth > 0)
		{
			p_uart_dev->uart_tx_fifo_depth--;

			if  (p_uart_dev->uart_tx_mode == UART_TX_MODE_STREAM)
			{
				if(buf[i] == '\n')
				{
					p_uart_reg->Mux0.THR = '\r';

					if (p_uart_dev->uart_tx_fifo_depth == 0)
					{
						while(drv_uart_tx_ready((unsigned int)p_uart_reg));
						p_uart_dev->uart_tx_fifo_depth = DRV_UART_FIFO_DEPTH;
					}
					p_uart_dev->uart_tx_fifo_depth--;
				}
			}

			p_uart_reg->Mux0.THR = buf[i++];

			if(--len == 0)
			{
				break;
			}
		}
		
	}
	return 0;
}

/**    @brief       Uart tx isr in dma.
 *     @details     Under the dma function,uart tx interrupt transmission mode.
 *     @param[in]   *data   Transmit data pointer
 */
void drv_uart_send_dma_isr(void * data)
{
	T_DRV_UART_DEV * p_uart_dev = (T_DRV_UART_DEV * )data;
	os_sem_post(p_uart_dev->uart_tx_sem);
}

/**    @brief       Uart is sent by dma.
 *     @details     Under the dma function, the source address buffer data is put into the destination address THR register for transmission.
 *	   @param[in]	uart_num	 Specifies the uart number to open, using E_DRV_UART_NUM type
 *	   @param[in]	*buf		 Buffer pointer, which stores the data transmit 
 *	   @param[in]	len		 Length of data transmitted
 *     @return      0--Transmit succeed, other--Transmit failed
 */
int drv_uart_send_dma(E_DRV_UART_NUM uart_num, char * buf, unsigned int len)
{
	T_DMA_CFG_INFO dma_cfg_info;
	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];
	T_UART_REG_MAP * p_uart_reg = p_uart_dev->uart_reg_base;

	dma_cfg_info.dst = (unsigned int)&p_uart_reg->Mux0.THR;
	dma_cfg_info.src = (unsigned int)buf;
	dma_cfg_info.len = len;

	if (uart_num == E_UART_NUM_0)
	{
		dma_cfg_info.mode = DMA_CHN_UART0_TX;
	}
	else if (uart_num == E_UART_NUM_1)
	{
		dma_cfg_info.mode = DMA_CHN_UART1_TX;
	}
	else if (uart_num == E_UART_NUM_2)
	{
		dma_cfg_info.mode = DMA_CHN_UART2_TX;
	}

	drv_dma_cfg(p_uart_dev->uart_tx_dma_chn, &dma_cfg_info);
	drv_dma_start(p_uart_dev->uart_tx_dma_chn);

	os_sem_wait(p_uart_dev->uart_tx_sem, WAIT_FOREVER);
	return 0;
}

/**    @brief       Uart is sent by interrupt.
 *     @details     The data in the buffer is sent out through the THR register.
 *	   @param[in]	 uart_num	 Specifies the uart number to open, using E_DRV_UART_NUM type
 *	   @param[in]	 *buf		 Buffer pointer, which stores the data transmit 
 *	   @param[in]	 len		 Length of data transmitted
 *     @return      0--Transmit succeed, other--Transmit failed
 */
int drv_uart_send_intr(E_DRV_UART_NUM uart_num, char * buf, unsigned int len)
{
	unsigned int i = 0;
	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];
	T_UART_REG_MAP * p_uart_reg = p_uart_dev->uart_reg_base;

	p_uart_reg->Mux1.IER = (~DRV_UART_IER_ETHEI) & p_uart_reg->Mux1.IER;
	p_uart_dev->uart_tx_len = len;
    
	while(drv_uart_tx_ready((unsigned int)p_uart_reg));
	p_uart_dev->uart_tx_fifo_depth = DRV_UART_FIFO_DEPTH;

	while (p_uart_dev->uart_tx_fifo_depth)
	{
		p_uart_dev->uart_tx_fifo_depth--;
		p_uart_reg->Mux0.THR = buf[i++];
		p_uart_dev->uart_tx_len--;

		if( p_uart_dev->uart_tx_len == 0)
		{
			break;
		}
	}

	if (p_uart_dev->uart_tx_len)
	{
		p_uart_dev->uart_tx_addr = &buf[i];
		p_uart_reg->Mux1.IER = DRV_UART_IER_ETHEI |p_uart_reg->Mux1.IER;
		os_sem_wait(p_uart_dev->uart_tx_sem, WAIT_FOREVER);
	}

	return 0;
}


/**    @brief		Uart receive interrupt.
*	   @details 	Under the action of CPU, uart receives the data of len length.
*	   @param[in]	uart_num    Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	*buf        Buffer pointer, the base address of the buffer used to receive data    
*	   @param[in]	len         Length of received data
*	   @param[in]	ms_timeout  Set the timeout value(ms)
*	   @return  	non negative value--Actual receive length, other--Receive failed
*/
static int drv_uart_receive_intr(E_DRV_UART_NUM uart_num, char * buf, unsigned int len, unsigned int ms_timeout)
{
	unsigned int len_remain0, len_remain1;
	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];

	p_uart_dev->uart_rx_user_addr = buf;

	if (drv_uart_rx_buf_update(p_uart_dev, len, &len_remain0, &len_remain1) == 0)
	{
		if (len <= len_remain0)
		{
			drv_uart_rx_buf_read(p_uart_dev, buf, len);
		}
		else
		{
			drv_uart_rx_buf_read(p_uart_dev, buf, len_remain0);
			drv_uart_rx_buf_read(p_uart_dev, buf + len_remain0, len - len_remain0);
		}
	}
	else
	{
		drv_uart_rx_buf_read(p_uart_dev, buf, len_remain0);
		if (len_remain1)
		{
			drv_uart_rx_buf_read(p_uart_dev, buf + len_remain0, len_remain1);
		}

		if (os_sem_wait(p_uart_dev->uart_rx_sem, ms_timeout))
		{
			/**  time out  */
			unsigned long flags = system_irq_save();
			len -= p_uart_dev->uart_rx_isr_threshold;
			p_uart_dev->uart_rx_isr_threshold = 0;
			system_irq_restore(flags);
			if (os_sem_get(p_uart_dev->uart_rx_sem))
			{
				os_sem_wait(p_uart_dev->uart_rx_sem, ms_timeout);
			}
		}
	}

	return len;
}

/**    @brief       Uart rx isr in dma.
 *     @details     Under the DMA function,uart rx interrupt receive mode.
 *     @param[in]   *data  Received data
 */
static void drv_uart_receive_dma_isr(void *data)
{
	T_DRV_UART_DEV * p_uart_dev = (T_DRV_UART_DEV * )data;
	os_sem_post(p_uart_dev->uart_rx_sem);
}

/**    @brief		Uart receiving data under dma.
*	   @details 	Under the action of dma, uart transfers the data from the original address RBR register to the destination address buffer.
*	   @param[in]	uart_num    Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	*buf        Buffer pointer, the base address of the buffer used to receive data    
*	   @param[in]	len         Length of received data
*	   @param[in]	ms_timeout  Set the timeout value(ms)
*	   @return  	non negative value--Actual receive length, other--Receive failed
*/
static unsigned int drv_uart_receive_dma(unsigned int uart_num, char * buf, unsigned int len, unsigned int ms_timeout)
{
	unsigned int left;	
	T_DMA_CFG_INFO dma_cfg_info;
	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];
	T_UART_REG_MAP * p_uart_reg = p_uart_dev->uart_reg_base;

	dma_cfg_info.src = (unsigned int)&p_uart_reg->Mux0.RBR;
	dma_cfg_info.dst = (unsigned int)buf;
	dma_cfg_info.len = len;

	if (uart_num == E_UART_NUM_0)
	{
		dma_cfg_info.mode = DMA_CHN_UART0_RX;
	}
	else if (uart_num == E_UART_NUM_1)
	{
		dma_cfg_info.mode = DMA_CHN_UART1_RX;
	}
	else if (uart_num == E_UART_NUM_2)
	{
		dma_cfg_info.mode = DMA_CHN_UART2_RX;
	}

	drv_dma_cfg(p_uart_dev->uart_rx_dma_chn, &dma_cfg_info);
	drv_dma_start(p_uart_dev->uart_rx_dma_chn);

	if (os_sem_wait(p_uart_dev->uart_rx_sem, ms_timeout) == 0)
	{
		return len;
	}
	else
	{
		unsigned long flags = system_irq_save();
		left = drv_dma_stop(p_uart_dev->uart_rx_dma_chn);
		system_irq_restore(flags);
		if (os_sem_get(p_uart_dev->uart_rx_sem))
		{
			os_sem_wait(p_uart_dev->uart_rx_sem, ms_timeout);
		}
		return len - left;
	}
}

/**    @brief		Uart tx/rx interrupts default.
*	   @details 	Default uart tx/rx interrupts before uart tx/rx starts.
*	   @param[in]	vector  Register interrupt vector number
*/
static void drv_uart_default_isr(int vector)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = arch_irq_save();
	#endif
	
	unsigned int i=0;
	T_DRV_UART_DEV * p_uart_dev;
	T_UART_REG_MAP * p_uart_reg;
#ifdef CONFIG_PSM_SURPORT
	unsigned int psm_uart_id;
#endif
	switch (vector)
	{
		case VECTOR_NUM_UART0:
			p_uart_dev = uart_dev[E_UART_NUM_0];
			#ifdef CONFIG_PSM_SURPORT
			psm_uart_id = PSM_DEVICE_UART0;
			#endif
			break;
		case VECTOR_NUM_UART1:
			p_uart_dev = uart_dev[E_UART_NUM_1];
			#ifdef CONFIG_PSM_SURPORT
			psm_uart_id = PSM_DEVICE_UART1;
			#endif
			break;
		case VECTOR_NUM_UART2:
			p_uart_dev = uart_dev[E_UART_NUM_2];
			#ifdef CONFIG_PSM_SURPORT
			psm_uart_id = PSM_DEVICE_UART2;
			#endif
			break;
		default:
			while(1);
	}
	#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(psm_uart_id,PSM_DEVICE_STATUS_ACTIVE);
	psm_uart_last_time_op(true, drv_rtc_get_32K_cnt());
	#endif

	p_uart_reg = p_uart_dev->uart_reg_base;

	switch (p_uart_reg->Mux2.IIR & DRV_UART_ISR_MASK)	
	{
		case DRV_UART_ISR_MODEM_STATUS:
			// to do
			break;

		case DRV_UART_ISR_TX_FINISH:
			p_uart_dev->uart_tx_fifo_depth = DRV_UART_FIFO_DEPTH;
			p_uart_reg->Mux1.IER = (~DRV_UART_IER_ETHEI) & p_uart_reg->Mux1.IER;

			while ( p_uart_dev->uart_tx_fifo_depth )
			{
				p_uart_dev->uart_tx_fifo_depth--;
				p_uart_reg->Mux0.THR = p_uart_dev->uart_tx_addr[i++];
				p_uart_dev->uart_tx_len--;

				if(p_uart_dev->uart_tx_len == 0)
				{
					os_sem_post(p_uart_dev->uart_tx_sem);
					#ifdef CONFIG_SYSTEM_IRQ
					system_irq_restore(flags);
					#endif
					return;
				}
			}
			p_uart_dev->uart_tx_addr += DRV_UART_FIFO_DEPTH;
			p_uart_reg->Mux1.IER = DRV_UART_IER_ETHEI |p_uart_reg->Mux1.IER;
			break;

		case DRV_UART_ISR_RX_TIMEOUT:
		case DRV_UART_ISR_RX_READY:
			if (p_uart_dev->uart_rx_mode == UART_RX_MODE_USER)
			{
#ifdef USE_NEW_USER_MODE
				if (p_uart_dev->uart_rx_callback.uart_callback)
				{
					p_uart_dev->uart_rx_callback.uart_callback(p_uart_dev->uart_rx_callback.uart_data);
				}
				else
				{
					/** discard rx data */
					i = p_uart_reg->Mux0.RBR;
					#ifdef CONFIG_SYSTEM_IRQ
					system_irq_restore(flags);
					#endif
					return;
				}
#else
				if (!p_uart_dev->uart_rx_callback)
				{
					/** discard rx data */
					i = p_uart_reg->Mux0.RBR;
					#ifdef CONFIG_SYSTEM_IRQ
					system_irq_restore(flags);
					#endif
					return;
				}
				
				while (p_uart_reg->LSR & DRV_UART_LSR_RDR)
				{
					p_uart_dev->uart_rx_callback((char)p_uart_reg->Mux0.RBR);
				}
#endif
			}
			else if (p_uart_dev->uart_rx_mode == UART_RX_MODE_INTR)
			{
				while (p_uart_reg->LSR & DRV_UART_LSR_RDR)
				{
					if (p_uart_dev->uart_rx_overrun == 0)
					{
						if (p_uart_dev->uart_rx_isr_threshold)
						{
							*(p_uart_dev->uart_rx_user_addr++) = (char)p_uart_reg->Mux0.RBR;			
							p_uart_dev->uart_rx_isr_threshold--;
							if (p_uart_dev->uart_rx_isr_threshold == 0)
							{
								os_sem_post(p_uart_dev->uart_rx_sem);
								//p_uart_dev->uart_rx_buf_wr = 0;
								//p_uart_dev->uart_rx_buf_rd = 0;
							}
						}
						else
						{
							p_uart_dev->uart_rx_buf_addr[p_uart_dev->uart_rx_buf_wr++] = (char)p_uart_reg->Mux0.RBR;
							p_uart_dev->uart_rx_buf_wr = (p_uart_dev->uart_rx_buf_wr) % p_uart_dev->uart_rx_buf_size;

							if (p_uart_dev->uart_rx_buf_wr == p_uart_dev->uart_rx_buf_rd)
							{
								p_uart_dev->uart_rx_overrun = 1;
							}
						}
					}
					else
					{
						/** discard rx data */
						i = p_uart_reg->Mux0.RBR;
					}
				}
			}
			break;
			
		case DRV_UART_ISR_LINE_STATUS:
			// to do
			break;

		default:
			while(1);
	}
	#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(psm_uart_id,PSM_DEVICE_STATUS_IDLE);
	#endif

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}

/**    @brief		Open uart device.
*	   @details 	Specifies uart_num, configure the corresponding clock, base address, data transmission bit, clear FIFO, configure baud rate,
*                   whether to start flow control. Then, register the uart tx/rx interrupt when the cpu is working, select whether the uart tx/rx 
*                   is working in cpu mode or dma mode, and register the uart tx/rx interrupt in dma mode.
*	   @param[in]	uart_num   Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	*cfg       The initialization configuration of UART
*      @return      0--Open succeed, other--Open failed
*/
int  drv_uart_open(E_DRV_UART_NUM uart_num, T_DRV_UART_CONFIG * cfg)
{
	T_DRV_UART_DEV * p_uart_dev;
	T_UART_REG_MAP * p_uart_reg;
	if (uart_dev[uart_num])
	{
		return UART_RET_EBUSY;
	}
	else
	{
		p_uart_dev = (T_DRV_UART_DEV *)os_zalloc(sizeof(T_DRV_UART_DEV));
		if (!p_uart_dev)
		{
			return UART_RET_ENOMEM;
		}
		else
		{
			uart_dev[uart_num] = p_uart_dev;
		}
	}

	switch (uart_num)
	{
		case E_UART_NUM_0:
			chip_clk_enable(CLK_UART0_UCLK);
			chip_clk_enable(CLK_UART0_APB);
			WRITE_REG(SOFTWARE_RESET_REG, READ_REG(SOFTWARE_RESET_REG) & ~DRV_UART0_RESET_ON);
			WRITE_REG(SOFTWARE_RESET_REG, READ_REG(SOFTWARE_RESET_REG) | DRV_UART0_RESET_ON);
			p_uart_dev->uart_reg_base = (T_UART_REG_MAP * )MEM_BASE_UART0;
			p_uart_dev->uart_vector_num = VECTOR_NUM_UART0;
			break;

		case E_UART_NUM_1:
			chip_clk_enable(CLK_UART1_UCLK);
			chip_clk_enable(CLK_UART1_APB);
	     	WRITE_REG(SOFTWARE_RESET_REG, READ_REG(SOFTWARE_RESET_REG) & ~DRV_UART1_RESET_ON);
			WRITE_REG(SOFTWARE_RESET_REG, READ_REG(SOFTWARE_RESET_REG) | DRV_UART1_RESET_ON);
			p_uart_dev->uart_reg_base = (T_UART_REG_MAP * )MEM_BASE_UART1;
			p_uart_dev->uart_vector_num = VECTOR_NUM_UART1;
			break;
			
		case E_UART_NUM_2:
			chip_clk_enable(CLK_UART2_UCLK);
			chip_clk_enable(CLK_UART2_APB);
		    WRITE_REG(SOFTWARE_RESET_REG, READ_REG(SOFTWARE_RESET_REG) & ~DRV_UART2_RESET_ON);
		    WRITE_REG(SOFTWARE_RESET_REG, READ_REG(SOFTWARE_RESET_REG) | DRV_UART2_RESET_ON);
			p_uart_dev->uart_reg_base = (T_UART_REG_MAP * )MEM_BASE_UART2;
			p_uart_dev->uart_vector_num = VECTOR_NUM_UART2;
			break;

		default:
			return UART_RET_EINVAL;
	}

	/** uart hardware init */
	p_uart_reg = p_uart_dev->uart_reg_base;
	p_uart_reg->LCR = cfg->uart_data_bits | cfg->uart_stop_bits | cfg->uart_parity_bit;
	p_uart_reg->Mux2.FCR = DRV_UART_FCR_FIFORST;
	p_uart_reg->Mux1.IER = 0;
	drv_uart_set_baud(p_uart_dev, cfg->uart_baud_rate);

	if (cfg->uart_flow_ctrl)
	{
		p_uart_reg->MCR = p_uart_reg->MCR  | DRV_UART_MCR_AFE;
	}
	else
	{
		p_uart_reg->MCR = p_uart_reg->MCR  & (~DRV_UART_MCR_AFE);
	}

	arch_irq_register(p_uart_dev->uart_vector_num, (void_fn)drv_uart_default_isr);
	arch_irq_clean(p_uart_dev->uart_vector_num);
	arch_irq_unmask(p_uart_dev->uart_vector_num);

	/** uart tx */
	if (cfg->uart_tx_mode == UART_TX_MODE_INTR)
	{
		p_uart_dev->uart_tx_sem = os_sem_create(1, 0);
		if (!p_uart_dev->uart_tx_sem)
		{
			return UART_RET_ERROR;
		}
	}
	else if (cfg->uart_tx_mode == UART_TX_MODE_DMA)
	{
		p_uart_dev->uart_tx_sem = os_sem_create(1, 0);
		if (!p_uart_dev->uart_tx_sem)
		{
			return UART_RET_ERROR;
		}

		p_uart_dev->uart_tx_dma_chn = (unsigned char)drv_dma_ch_alloc();
		if ( p_uart_dev->uart_tx_dma_chn < 0)
		{
			return UART_RET_ERROR;
		}

		drv_dma_isr_register(p_uart_dev->uart_tx_dma_chn, drv_uart_send_dma_isr, (void  *)p_uart_dev);

		p_uart_reg->Mux2.FCR = DRV_UART_FCR_DMAE|DRV_UART_FCR_FIFORST;
	}

	p_uart_dev->uart_tx_mutex = os_mutex_create();
	if (p_uart_dev->uart_tx_mutex == NULL)
	{
		return UART_RET_ERROR;
	}

	p_uart_dev->uart_tx_mode = cfg->uart_tx_mode;
	p_uart_dev->uart_tx_fifo_depth = DRV_UART_FIFO_DEPTH;
	
	/** uart rx */
	p_uart_dev->uart_rx_mode = cfg->uart_rx_mode;
	if (p_uart_dev->uart_rx_mode == UART_RX_MODE_USER)
	{
		p_uart_reg->Mux1.IER = DRV_UART_IER_ERBII;
		if (cfg->uart_tx_mode == UART_TX_MODE_DMA)
		{
			p_uart_reg->Mux2.FCR = DRV_UART_FCR_RFIFOT(0) | DRV_UART_FCR_FIFORST | DRV_UART_FCR_DMAE;
		}
		else
		{
			p_uart_reg->Mux2.FCR = DRV_UART_FCR_RFIFOT(0) | DRV_UART_FCR_FIFORST;
		}
		return UART_RET_SUCCESS;
	}

	p_uart_dev->uart_rx_sem= os_sem_create(1, 0);
	if (!p_uart_dev->uart_rx_sem)
	{
		return UART_RET_EINVAL;
	}

	if (cfg->uart_rx_mode == UART_RX_MODE_INTR)
	{
		p_uart_reg->Mux1.IER = DRV_UART_IER_ERBII;
		p_uart_dev->uart_rx_buf_size = cfg->uart_rx_buf_size;
		p_uart_dev->uart_rx_buf_addr = (char *)os_malloc(p_uart_dev->uart_rx_buf_size);
	}
	else if (cfg->uart_rx_mode == UART_RX_MODE_DMA)
	{
		p_uart_dev->uart_rx_dma_chn = (unsigned char)drv_dma_ch_alloc();
		if ( p_uart_dev->uart_rx_dma_chn < 0)
		{
			return UART_RET_ENODMA;
		}

		drv_dma_isr_register(p_uart_dev->uart_rx_dma_chn, drv_uart_receive_dma_isr, (void  *)p_uart_dev);
		p_uart_reg->Mux2.FCR = DRV_UART_FCR_DMAE|DRV_UART_FCR_FIFORST;
	}
	return UART_RET_SUCCESS;
}

/**    @brief		Close uart device.
*	   @param[in]	uart_num   Specifies the uart number to open, using E_DRV_UART_NUM type
*      @return      0--Close succeed, other--Close failed
*/
int drv_uart_close(E_DRV_UART_NUM uart_num)
{  	
	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];
	T_UART_REG_MAP * p_uart_reg = p_uart_dev->uart_reg_base;
	if (!uart_dev[uart_num])
	{
	    return UART_RET_ERROR;
	}
	while(drv_uart_rx_tstc((unsigned int)p_uart_reg));
	while(drv_uart_tx_ready((unsigned int)p_uart_reg));
	os_msdelay(1);

	arch_irq_mask(p_uart_dev->uart_vector_num);
	arch_irq_clean(p_uart_dev->uart_vector_num);
	arch_irq_register(p_uart_dev->uart_vector_num, 0);
	switch (uart_num)
	{
		case E_UART_NUM_0:
			chip_clk_disable(CLK_UART0_UCLK);
			chip_clk_disable(CLK_UART0_APB);
			break;

		case E_UART_NUM_1:
			chip_clk_disable(CLK_UART1_UCLK);
			chip_clk_disable(CLK_UART1_APB); 
			break;
			
		case E_UART_NUM_2:
			chip_clk_disable(CLK_UART2_UCLK);
			chip_clk_disable(CLK_UART2_APB);
			break;

		default:
			return UART_RET_EINVAL;
	}

	if(p_uart_dev->uart_rx_sem)
	{
	    os_sem_destroy(p_uart_dev->uart_rx_sem);
	}
	if(p_uart_dev->uart_rx_buf_addr)
	{
		os_free(p_uart_dev->uart_rx_buf_addr);
		p_uart_dev->uart_rx_buf_addr = NULL;
	}


	if(p_uart_dev->uart_tx_mutex)
	{
	   os_mutex_destroy(p_uart_dev->uart_tx_mutex);
	}

	if(p_uart_dev->uart_tx_sem)
	{
	    os_sem_destroy(p_uart_dev->uart_tx_sem);
	}

	if(uart_dev[uart_num])
	{
		os_free(uart_dev[uart_num]);
	    uart_dev[uart_num] = NULL;
	}	
	return UART_RET_SUCCESS;
}

/**    @brief		Uart transmit data.
*	   @details 	Select uart to transmit data in the way of poll/intr/dma.
*	   @param[in]	uart_num    Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	*buf        Buffer pointer, which stores the data transmit 
*	   @param[in]	len         Length of data transmitted
*      @return      ret=0--Transmit succeed, ret=-1--Transmit failed
*/
int drv_uart_write(E_DRV_UART_NUM uart_num, char * buf, unsigned int len)
{
	int ret;
	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];

	os_mutex_lock(p_uart_dev->uart_tx_mutex, WAIT_FOREVER);

	switch (p_uart_dev->uart_tx_mode)
	{
		case UART_TX_MODE_POLL:
		case UART_TX_MODE_STREAM:
			ret = drv_uart_send_poll(uart_num, buf, len);
			break;

		case UART_TX_MODE_INTR:
			ret =  drv_uart_send_intr(uart_num, buf, len);
			break;

		case UART_TX_MODE_DMA:
			ret =  drv_uart_send_dma(uart_num, buf, len);
			break;

		default:
			ret = UART_RET_ERROR;
			break;
	}

	os_mutex_unlock(p_uart_dev->uart_tx_mutex);
	return ret;
}

/**    @brief		Uart receive data.
*	   @details 	Select uart to receive data in the way of cpu interrupt or dma.
*	   @param[in]	uart_num    Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	*buf        Buffer pointer, the base address of the buffer used to receive data    
*	   @param[in]	len         Length of received data
*	   @param[in]	ms_timeout  Set the timeout value(ms)
*	   @return  	non negative value--Actual receive length, other--Receive failed
*/
int drv_uart_read(E_DRV_UART_NUM uart_num, char * buf, unsigned int len, unsigned int ms_timeout)
{
	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];

	switch (p_uart_dev->uart_rx_mode)
	{
		case UART_RX_MODE_INTR:
			return drv_uart_receive_intr(uart_num, buf, len, ms_timeout);

		case UART_RX_MODE_DMA:
			return drv_uart_receive_dma(uart_num, buf, len, ms_timeout);

		default:
			return UART_RET_ERROR;
	}
}

/**    @brief		Handle uart different events.
*	   @details 	The events handled include reg_base, set baud, isr_register, rx_reset.
*	   @param[in]	uart_num    Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	event       Control event type
*	   @param[in]	*arg    Control parameters, the specific meaning is determined according to the event
*	   @return  	0--Handle event succeed, -1--Handle event fail
*/
int drv_uart_ioctrl(E_DRV_UART_NUM uart_num, int event, void * arg)
{
	T_UART_REG_MAP * p_uart_reg;
	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];

	if ((uart_num > E_UART_NUM_MAX) || !p_uart_dev)
	{
		return UART_RET_EINVAL;
	}

	p_uart_reg = p_uart_dev->uart_reg_base;

	switch (event)
	{
		case DRV_UART_CTRL_GET_REG_BASE:
			*((unsigned int *)arg) =  (unsigned int)p_uart_reg;
			break;

		case DRV_UART_CTRL_SET_BAUD_RATE:
			drv_uart_set_baud(p_uart_dev, *((unsigned int *)arg));
			break;

		case DRV_UART_CTRL_REGISTER_RX_CALLBACK:
#ifdef USE_NEW_USER_MODE
			if (arg)
			{
				T_UART_ISR_CALLBACK * p_callback = (T_UART_ISR_CALLBACK *)arg;
				p_uart_dev->uart_rx_callback.uart_callback = p_callback->uart_callback;
				p_uart_dev->uart_rx_callback.uart_data = p_callback->uart_data;
			}
			else
			{
				return UART_RET_EINVAL;
			}


#else
			p_uart_dev->uart_rx_callback = (UART_CALLBACK)arg;
			//arch_irq_register(p_uart_dev->uart_vector_num, (void_fn)arg);
			//arch_irq_clean(p_uart_dev->uart_vector_num);
			//arch_irq_unmask(p_uart_dev->uart_vector_num);
#endif
			break;

		case DRV_UART_CTRL_RX_RESET:
			drv_uart_receive_reset(p_uart_dev);			
			break;
/*
		case DRV_UART_CTRL_RX_INTR_ENABLE:
			p_uart_reg->Mux1.IER = DRV_UART_IER_ERBII;
			break;
*/
		default:
			return UART_RET_ERROR;
	}

	return 0;
}

/**    @brief		Gets the length of the data to be received.
*	   @param[in]	uart_num   Specifies the uart number to open, using E_DRV_UART_NUM type
*      @return      len--Length of data to be received
*/
int drv_uart_get_recv_len(E_DRV_UART_NUM uart_num)
{
	T_DRV_UART_DEV * p_uart_dev = uart_dev[uart_num];
	int len;

	if (p_uart_dev->uart_rx_buf_wr >= p_uart_dev->uart_rx_buf_rd)
	{
		len = p_uart_dev->uart_rx_buf_wr - p_uart_dev->uart_rx_buf_rd;
		
	}
	else
	{
		len = p_uart_dev->uart_rx_buf_size - p_uart_dev->uart_rx_buf_rd + p_uart_dev->uart_rx_buf_wr;
	}
	return len;
}

static int at_get_stopbit(int databits,int stopbits)
{
	if(stopbits == 0){
		return 1;
	} else{
		if(databits == 0) 
           return 2;
        else 
           return 3;
	}
	return 1;
}

void drv_uart_set_lineControl(unsigned int regBase, 
			unsigned int databits,	/* 0--5bits, 1--6bits, 2--7bits, 3--8bits */
			unsigned int parity,	/* 0--no parity, 1--odd parity, 2--even parity*/
			unsigned int stopbits,	/* 0--1bit stopbits, 1--the num of stopbits is based on the databits*/
			unsigned int bc		/* break control */)
{

	unsigned int value = READ_REG(regBase + 0x2c);
	
	value &= 0xFFFFFF80;

	value |= (databits & 3) | ((stopbits & 1) << 2) | ((bc & 1) << 6);

	if(parity == 1)
		value |= 0x8;
	else if(parity == 2)
		value |= 0x18;
	else if(parity == 3)
		value |= 0x28;
	else if(parity == 4)
		value |= 0x38;

	WRITE_REG(regBase + 0x2c, value);
	if(regBase == MEM_BASE_UART0){
	uart_dev_config[0].uart_data_bits = databits+5;
	uart_dev_config[0].uart_parity_bit   = parity;
	uart_dev_config[0].uart_stop_bits = at_get_stopbit(databits,stopbits);
	} else if(regBase == MEM_BASE_UART1){
	uart_dev_config[1].uart_data_bits = databits+5;
	uart_dev_config[1].uart_parity_bit	 = parity;
	uart_dev_config[1].uart_stop_bits = at_get_stopbit(databits,stopbits);
	} else if(regBase == MEM_BASE_UART2){
	uart_dev_config[2].uart_data_bits = databits+5;
	uart_dev_config[2].uart_parity_bit	 = parity;
	uart_dev_config[2].uart_stop_bits = at_get_stopbit(databits,stopbits);
	}
}

int drv_uart_get_config(unsigned int regBase, T_DRV_UART_CONFIG * config)
{
	if(regBase == MEM_BASE_UART0){
		memcpy(config, &uart_dev_config[0],sizeof(T_DRV_UART_CONFIG));
	} else if(regBase == MEM_BASE_UART1){
		memcpy(config, &uart_dev_config[1],sizeof(T_DRV_UART_CONFIG));
	} else if(regBase == MEM_BASE_UART2){
		memcpy(config, &uart_dev_config[2],sizeof(T_DRV_UART_CONFIG));
	} else {
		return -1;
	}
	return 0;
}

void drv_uart_set_baudrate(unsigned int regBase, unsigned int baud)
{
	int uart_baud = UART_RATE(baud);
	unsigned int value = READ_REG(regBase + 0x2c);
	int osc;
	
	if(regBase == MEM_BASE_UART0){
		uart_dev_config[0].uart_baud_rate = baud;
	} else if(regBase == MEM_BASE_UART1){
		uart_dev_config[1].uart_baud_rate = baud;
	} else if(regBase == MEM_BASE_UART2){
		uart_dev_config[2].uart_baud_rate = baud;
	}
	
#if !defined (CONFIG_CPU_CLK_SRC_40m)
	if(baud == BAUD_RATE_806400)
	{
		uart_baud = UART_DIVISOR_VALUE_13;
	}

	if(baud == BAUD_RATE_921600)
	{
		uart_baud = UART_DIVISOR_VALUE_11;
	}

	if(baud == BAUD_RATE_1500000)
	{
		osc = UART_OSCR_18;
		uart_baud = UART_DIVISOR_VALUE_3;
	}
	else
#endif
	{
		osc = UART_OSCR_8;
	}

	WRITE_REG(regBase + 0x14, osc);	
	WRITE_REG(regBase + 0x2c, 0x80|value);	
	WRITE_REG(regBase + 0x20, (uart_baud >> 0) & 0xff);
	WRITE_REG(regBase + 0x24, (uart_baud >> 8) & 0xff);
	WRITE_REG(regBase + 0x2c, value & 0xFFFFFF7F);
}

int drv_uart_set_flowcontrol(unsigned int regBase,unsigned int m_flow_control)
{
    unsigned int value = READ_REG(regBase + 0x34);

    if(regBase == MEM_BASE_UART0){
    	uart_dev_config[0].uart_flow_ctrl = m_flow_control;
    } else if(regBase == MEM_BASE_UART1){
    	uart_dev_config[1].uart_flow_ctrl = m_flow_control;
    } else if(regBase == MEM_BASE_UART2){
    	uart_dev_config[2].uart_flow_ctrl = m_flow_control;
    }
    if (m_flow_control)
    {
        WRITE_REG(regBase + 0x30, DRV_UART_MCR_AFE|value);	
    }
    else
    {
        WRITE_REG(regBase + 0x30, value & ~DRV_UART_MCR_AFE);	
    }
    return 0;
}

int drv_uart_get_regbase(E_DRV_UART_NUM uart_num){
	return (int)uart_dev[uart_num]->uart_reg_base;
}



