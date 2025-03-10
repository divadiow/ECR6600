/**
* @file       uart.h 
* @author     Wangchao
* @date       2021-1-22
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/


#ifndef DRV_UART_H
#define DRV_UART_H

#include "chip_memmap.h"

//#ifndef inw
//#define inw(reg)        (*((volatile unsigned int *) (reg)))
//#endif
//
//#ifndef outw
//#define outw(reg, data) ((*((volatile unsigned int *)(reg)))=(unsigned int)(data))
//#endif

#define UART_RET_SUCCESS		0
#define UART_RET_ERROR			-1
#define UART_RET_EINVAL		-2
#define UART_RET_EBUSY			-3
#define UART_RET_ENOMEM		-4
#define UART_RET_ENODMA		-5

#define USE_NEW_USER_MODE	

#ifdef USE_NEW_USER_MODE
typedef struct _T_UART_ISR_CALLBACK
{
	void (* uart_callback)(void *);
	void *uart_data;
} T_UART_ISR_CALLBACK;

#else
typedef void (*UART_CALLBACK)(char);
#endif

/*  UART BAUDRATE SETTING */
#define BAUD_RATE_2400                          2400
#define BAUD_RATE_4800                          4800
#define BAUD_RATE_9600                          9600
#define BAUD_RATE_19200                         19200
#define BAUD_RATE_38400                         38400
#define BAUD_RATE_57600                         57600
#define BAUD_RATE_115200                       115200
#define BAUD_RATE_230400                       230400
#define BAUD_RATE_460800                       460800
#define BAUD_RATE_806400                       806400
#define BAUD_RATE_921600                       921600
#define BAUD_RATE_1500000                      1500000
#define BAUD_RATE_2000000                      2000000
  
/*  UART DATA BIT SETTING */
#define UART_DATA_BITS_5			0x00
#define UART_DATA_BITS_6			0x01
#define UART_DATA_BITS_7			0x02
#define UART_DATA_BITS_8			0x03

/** UART NUMBER OF STOP BITS */
#define UART_STOP_BITS_1			0x00	
#define UART_STOP_BITS_OTHER		0x04	/** based on the DATA bit setting, When DATA bit = 0, STOP bit is 1.5 bits */
											/** based on the DATA bit setting, When DATA bit = 1, 2, 3, STOP bit is 2 bits */ 


/** UART PARITY BIT SETTING */
#define UART_PARITY_BIT_NONE		0x00
#define UART_PARITY_BIT_ODD		    0x08
#define UART_PARITY_BIT_EVEN		0x18


/** UART FLOW CONTROL SETTING */
#define UART_FLOW_CONTROL_DISABLE		0x00
#define UART_FLOW_CONTROL_ENABLE		0x20

/** UART TX SETTING */
#define UART_TX_MODE_POLL				0x00
#define UART_TX_MODE_STREAM			    0x01
#define UART_TX_MODE_INTR				0x02
#define UART_TX_MODE_DMA				0x03

/** UART RX SETTING */
#define UART_RX_MODE_INTR				0x00
#define UART_RX_MODE_DMA				0x01
#define UART_RX_MODE_USER				0x02

/**
 * @brief Uart device config
 */
typedef struct _T_DRV_UART_CONFIG
{
	unsigned int uart_baud_rate;
	unsigned int uart_data_bits;
	unsigned int uart_stop_bits;
	unsigned int uart_parity_bit;
	unsigned int uart_flow_ctrl;
	unsigned int uart_tx_mode;
	unsigned int uart_rx_mode;
	unsigned int uart_rx_buf_size;

} T_DRV_UART_CONFIG;

/**
 * @brief Uart device number.
 */
typedef enum _E_DRV_UART_NUM
{
	E_UART_NUM_0 = 0,
	E_UART_NUM_1 = 1,
	E_UART_NUM_2 = 2,
	E_UART_SELECT_BY_KCONFIG = 3,
	E_UART_NUM_MAX = E_UART_SELECT_BY_KCONFIG

} E_DRV_UART_NUM;


/**    @brief       Uart tx ready.
 *     @details     Judge whether txfifo is empty, and put data into txfifo when it is empty.
 *     @param[in]   uart_base   Uart base address
 *     @return      return  1--tx not ready, return 0--tx ready
 */
int drv_uart_tx_ready(unsigned int uart_base);

/**    @brief       Uart tx FIFO and tsr empty.
 *     @details     Judge whether txfifo and Transmitter Shift Register are both empty.
 *     @param[in]   uart_base   Uart base address
 *     @return      return  0--tx not ready, return 1--tx ready
 */
int drv_uart_tx_fifo_and_tsr_empty();

/**    @brief       Uart tx put.
 *     @details     Put the data into the Transmitter Holding Register and transfer it out.
 *     @param[in]   uart_base  Uart base address
 *     @param[in]   c     Transmit character data
 */
void drv_uart_tx_putc(unsigned int uart_base,  char c);

/**    @brief       Uart rx ready.
 *     @details     When the return value of the function is 1, it is ready to receive data.
 *     @param[in]   uart_base   Uart base address
 *     @return      return 1--rx ready, return 0--rx not ready
 */
unsigned char drv_uart_rx_tstc (unsigned int uart_base);

/**    @brief       Uart rx get.
 *     @details     Get the value of register Receiver Buffer Register.
 *     @param[in]   uart_base   Uart base address
 *     @return      Returns the value of Receiver Buffer Register
 */
char drv_uart_rx_getc (unsigned int uart_base);

/**    @brief       Uart is sent by polling.
 *     @details     The data in the buffer is sent out through the THR register.
 *     @param[in]   uart_num  Specifies the uart number to open, using E_DRV_UART_NUM type 
 *     @param[in]   *buf      The buf pointer points to the address of the data to be transmitted
 *     @param[in]   len       Length of data transmitted
 *     @return      0--Transmit succeed, other--Transmit failed
 */
int  drv_uart_send_poll(E_DRV_UART_NUM uart_num, char * buf, unsigned int len);

/**    @brief       Uart is sent by interrupt.
 *     @details     The data in the buffer is sent out through the THR register.
 *     @param[in]   uart_num  Specifies the uart number to open, using E_DRV_UART_NUM type 
 *     @param[in]   *buf      The buf pointer points to the address of the data to be transmitted
 *     @param[in]   len       Length of data transmitted
 *     @return      0--Transmit succeed, other--Transmit failed
 */
int drv_uart_send_intr(E_DRV_UART_NUM uart_num, char * buf, unsigned int len);

//void drv_uart_default_isr(int vector);

/**    @brief		Open uart device.
*	   @details 	Specifies uart_num, configure the corresponding clock, base address, data transmission bit, clear FIFO, configure baud rate,
*                   whether to start flow control. Then, register the uart tx/rx interrupt when the cpu is working, select whether the uart tx/rx 
*                   is working in cpu mode or dma mode, and register the uart tx/rx interrupt in dma mode.
*	   @param[in]	uart_num    Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	*cfg        The initialization configuration of UART
*      @return      0--Open succeed, other--Open failed
*/
int drv_uart_open(E_DRV_UART_NUM uart_num, T_DRV_UART_CONFIG * cfg);

/**    @brief		Uart transmit data.
*	   @details 	Select uart to transmit data in the way of poll/intr/dma.
*	   @param[in]	uart_num    Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	*buf        Buffer pointer, which stores the data transmit 
*	   @param[in]	len         Length of data transmitted
*      @return      ret=0--Transmit succeed, ret=-1--Transmit failed
*/
int drv_uart_write(E_DRV_UART_NUM uart_num, char * buf, unsigned int len);

/**    @brief		Uart receive data.
*	   @details 	Select uart to receive data in the way of cpu interrupt or dma.
*	   @param[in]	uart_num    Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	*buf        Buffer pointer, the base address of the buffer used to receive data    
*	   @param[in]	len         Length of received data
*	   @param[in]	ms_timeout  Set the timeout value(ms)
*	   @return  	non negative value--Actual receive length, other--Receive failed
*/
int drv_uart_read(E_DRV_UART_NUM uart_num, char * buf, unsigned int len, unsigned int ms_timeout);

/**    @brief		Close uart device.
*	   @param[in]	uart_num   Specifies the uart number to open, using E_DRV_UART_NUM type
*      @return      0--Close succeed, other--Close failed
*/
int drv_uart_close(E_DRV_UART_NUM uart_num);

/**    @brief		Gets the length of the data to be received.
*	   @param[in]	uart_num   Specifies the uart number to open, using E_DRV_UART_NUM type
*      @return      len--Length of data to be received
*/
int drv_uart_get_recv_len(E_DRV_UART_NUM uart_num);



#define DRV_UART_CTRL_GET_REG_BASE 		0
#define DRV_UART_CTRL_SET_BAUD_RATE     1
#define DRV_UART_CTRL_REGISTER_RX_CALLBACK 		2
#define DRV_UART_CTRL_RX_RESET	 		3
//#define DRV_UART_CTRL_RX_INTR_ENABLE	 	4

/**    @brief		Handle uart different events.
*	   @details 	The events handled include reg_base, set baud, isr_register, rx_reset.
*	   @param[in]	uart_num    Specifies the uart number to open, using E_DRV_UART_NUM type
*	   @param[in]	event       Control event type
*	   @param[in]	*arg    Control parameters, the specific meaning is determined according to the event
*	   @return  	0--Handle event succeed, -1--Handle event failed
*/
int drv_uart_ioctrl(E_DRV_UART_NUM uart_num, int event, void * arg);


/** UART AT CONTROL SETTING */
void drv_uart_set_lineControl(unsigned int regBase, unsigned int databits, unsigned int parity, unsigned int stopbits, unsigned int bc);
int drv_uart_get_config(unsigned int regBase, T_DRV_UART_CONFIG * config);
void drv_uart_set_baudrate(unsigned int regBase, unsigned int baud);
int drv_uart_set_flowcontrol(unsigned int regBase,unsigned int m_flow_control);


#endif /* DRV_UART_H */

