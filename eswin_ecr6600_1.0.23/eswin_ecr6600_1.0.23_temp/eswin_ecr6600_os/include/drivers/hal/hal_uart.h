#ifndef HAL_UART_H
#define HAL_UART_H

#include "uart.h"

int hal_uart_open(E_DRV_UART_NUM uart_num, T_DRV_UART_CONFIG * cfg);
int hal_uart_read(E_DRV_UART_NUM uart_num, unsigned char * buff, int len, int outtime_ms);
int hal_uart_write(E_DRV_UART_NUM uart_num, unsigned char * buff, int len);
#ifdef USE_NEW_USER_MODE
int hal_uart_callback_register(E_DRV_UART_NUM uart_num, T_UART_ISR_CALLBACK * uart_callback, void * uart_data);
#else
int hal_uart_callback_register(E_DRV_UART_NUM uart_num, UART_CALLBACK uart_callback, void * uart_data);
#endif
int hal_uart_get_recv_len(E_DRV_UART_NUM uart_num);
int hal_uart_close(E_DRV_UART_NUM uart_num);


#endif /*HAL_UART_H*/


