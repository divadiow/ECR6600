
#include "uart.h"
#include "oshal.h"
#include "hal_uart.h"


int hal_uart_open(E_DRV_UART_NUM uart_num, T_DRV_UART_CONFIG * cfg)
{
	return drv_uart_open(uart_num, cfg);
}

int hal_uart_read(E_DRV_UART_NUM uart_num, unsigned char * buff, int len, int outtime_ms)
{
	return drv_uart_read(uart_num, (char *)buff, len, outtime_ms); 
}

int hal_uart_write(E_DRV_UART_NUM uart_num, unsigned char * buff, int len)
{
	return drv_uart_write(uart_num, (char *)buff, len);
}

int hal_uart_get_recv_len(E_DRV_UART_NUM uart_num)
{
	return drv_uart_get_recv_len(uart_num);
}

int hal_uart_close(E_DRV_UART_NUM uart_num)
{
	return drv_uart_close(uart_num);
}


#ifdef USE_NEW_USER_MODE
int hal_uart_callback_register(E_DRV_UART_NUM uart_num, T_UART_ISR_CALLBACK *uart_callback, void * uart_data)
#else
int hal_uart_callback_register(E_DRV_UART_NUM uart_num, UART_CALLBACK uart_callback, void * uart_data)
#endif
{
	return drv_uart_ioctrl(uart_num, DRV_UART_CTRL_REGISTER_RX_CALLBACK, uart_callback);
}


