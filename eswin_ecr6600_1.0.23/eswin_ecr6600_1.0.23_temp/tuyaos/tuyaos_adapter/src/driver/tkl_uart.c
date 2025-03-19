 /*============================================================================
 *                                                                            *
 * Copyright (C) by Tuya Inc                                                  *
 * All rights reserved                                                        *
 *                                                                            *
 *                                                                            *
 =============================================================================*/

/*============================ INCLUDES ======================================*/

#include "tkl_uart.h"
//#include "tuya_os_adapter_error_code.h"
#include "hal_uart.h"
#include "oshal.h"
//#include "drv_model_pub.h"
//#include "uart_pub.h"
//#include "BkDriverUart.h"
#include "chip_pinmux.h"
#include "tkl_semaphore.h"
/*============================ MACROS ========================================*/
// #define UART_DEV_NUM            2

/*============================ TYPES =========================================*/
#define MAX_UART_NUM 3

UINT_T uart_port_num[MAX_UART_NUM] = {0xFF, 0xFF, 0xFF};
unsigned int uart_base_reg[MAX_UART_NUM];
TUYA_UART_IRQ_CB uart_rx_cb[MAX_UART_NUM];



INT_T tkl_uart_write(TUYA_UART_NUM_E port, VOID_T *buff, UINT16_T len);


/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/
void tuya_uart_rx_isr(void *port_num)
{
    UINT_T uart_num = *((UINT_T *)port_num);
    //int ret;

    if (uart_num >= MAX_UART_NUM) {
        return;
    }

    if (uart_rx_cb[uart_num] != NULL) {
        uart_rx_cb[uart_num](uart_num);
    }

}


/**
 * @brief 用于初始化串口
 * 
 * @param[in]  uart     串口句柄
 * @param[in]  cfg      串口配置结构体
 */
OPERATE_RET tkl_uart_init(TUYA_UART_NUM_E port, TUYA_UART_BASE_CFG_T *cfg)
{
	E_DRV_UART_NUM uart_num;
	T_DRV_UART_CONFIG	 uart_cfg;

	if (port >= MAX_UART_NUM )
	{
		return OPRT_INVALID_PARM;
	}
    //! data bits
    if (TUYA_UART_DATA_LEN_5BIT == cfg->databits) {
        uart_cfg.uart_data_bits = UART_DATA_BITS_5;
    } else if (TUYA_UART_DATA_LEN_6BIT == cfg->databits) {
        uart_cfg.uart_data_bits = UART_DATA_BITS_6;
    } else if (TUYA_UART_DATA_LEN_7BIT ==  cfg->databits) {
        uart_cfg.uart_data_bits = UART_DATA_BITS_7;
    } else if (TUYA_UART_DATA_LEN_8BIT == cfg->databits) {
        uart_cfg.uart_data_bits = UART_DATA_BITS_8;
    } else {
        return OPRT_INVALID_PARM;
    }
    //! stop bits
    if (TUYA_UART_STOP_LEN_1BIT == cfg->stopbits) {
        uart_cfg.uart_stop_bits = UART_STOP_BITS_1;
    } else if (TUYA_UART_STOP_LEN_2BIT == cfg->stopbits) {
        uart_cfg.uart_stop_bits = UART_STOP_BITS_OTHER;
    } else {
        return OPRT_INVALID_PARM;
    }
    //!  parity bits
    if (TUYA_UART_PARITY_TYPE_NONE == cfg->parity) {
        uart_cfg.uart_parity_bit = UART_PARITY_BIT_NONE;
    } else if (TUYA_UART_PARITY_TYPE_EVEN == cfg->parity) {
        uart_cfg.uart_parity_bit = UART_PARITY_BIT_EVEN;
    } else if (TUYA_UART_PARITY_TYPE_ODD == cfg->parity) {
        uart_cfg.uart_parity_bit = UART_PARITY_BIT_ODD;
    } else {
        return OPRT_INVALID_PARM;
    }
    //! baudrate
    uart_cfg.uart_baud_rate = cfg->baudrate;
	uart_cfg.uart_flow_ctrl = UART_FLOW_CONTROL_DISABLE;
	uart_cfg.uart_tx_mode = UART_TX_MODE_STREAM;
	uart_cfg.uart_rx_mode = UART_RX_MODE_USER;

	switch(port)
	{
		case E_UART_NUM_0:
			//chip_uart0_pinmux_cfg(UART0_RX_USED_GPIO5, UART0_TX_USED_GPIO22, 0);
			break;
		case E_UART_NUM_1:
			chip_uart1_pinmux_cfg(0); // 0:Do not use CTS RTS
			break;
		case E_UART_NUM_2:
			chip_uart2_pinmux_cfg(UART2_TX_USED_GPIO13);
			break;
		default:
			uart_num = E_UART_NUM_0;
			break;
	}
	(void)hal_uart_open(uart_num, &uart_cfg);
#ifdef USE_NEW_USER_MODE
	T_UART_ISR_CALLBACK callback;
    uart_port_num[uart_num] = uart_num;
	callback.uart_callback = tuya_uart_rx_isr;
	callback.uart_data = (void *)&uart_port_num[uart_num];
	(void)drv_uart_ioctrl(uart_num, DRV_UART_CTRL_REGISTER_RX_CALLBACK, &callback);

#else
	drv_uart_ioctrl(uart_num, DRV_UART_CTRL_REGISTER_RX_CALLBACK, tuya_uart_isr);
#endif

	drv_uart_ioctrl(uart_num, DRV_UART_CTRL_GET_REG_BASE, (void *)&uart_base_reg[port]);

    return OPRT_OK;
}


OPERATE_RET tkl_uart_deinit(TUYA_UART_NUM_E port_num)
{
	if (port_num >= MAX_UART_NUM )
	{
		return OPRT_INVALID_PARM;
	}

	return drv_uart_close(port_num);
}


/**
 * @brief 用于发送一个字节
 * 
 * @param[in]  uart     串口句柄
 * @param[in]  byte     要发送的字节
 */



INT_T tkl_uart_write(TUYA_UART_NUM_E port, VOID_T *buff, UINT16_T len)
{
    INT_T i;
    UINT8_T *data = (UINT8_T *)buff;

	if (port >= MAX_UART_NUM)
	{
		return OPRT_INVALID_PARM;
	}

    for (i = 0; i < len; i++) {
        while(drv_uart_tx_ready(uart_base_reg[port]));
        drv_uart_tx_putc(uart_base_reg[port], data[i]);
    }

    return len;
}



INT_T tkl_uart_read(TUYA_UART_NUM_E port, VOID_T *buff, UINT16_T len)
{
    if (len > 1) {
        return OPRT_COM_ERROR;
    }

	if (port >= MAX_UART_NUM)
	{
		return OPRT_INVALID_PARM;
	}

	if (drv_uart_rx_tstc(uart_base_reg[port]))
	{
		*(UINT8_T *)buff = drv_uart_rx_getc(uart_base_reg[port]);
        return 1;
	}

    return 0;
}


VOID tkl_uart_rx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB rx_cb)
{
    if (port_id >= MAX_UART_NUM) {
        return;
    }

    /* 允许没初始化过直接调用，也能正确注册接收中断 */
    /* 用在cli_station场景 */
    if (uart_port_num[port_id] != port_id) {
        uart_port_num[port_id] = port_id;
        T_UART_ISR_CALLBACK callback;
	    callback.uart_callback = tuya_uart_rx_isr;
	    callback.uart_data = (void *)&uart_port_num[port_id];
	    drv_uart_ioctrl(port_id, DRV_UART_CTRL_REGISTER_RX_CALLBACK, &callback);
        drv_uart_ioctrl(port_id, DRV_UART_CTRL_GET_REG_BASE, (void *)&uart_base_reg[port_id]);
    }

    uart_rx_cb[port_id] = rx_cb;
}


OPERATE_RET tkl_uart_set_tx_int(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    port_id = port_id;
    enable = enable;
    return OPRT_OK;
}


VOID tkl_uart_tx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB tx_cb)
{
    port_id = port_id;
    tx_cb = tx_cb;
}

