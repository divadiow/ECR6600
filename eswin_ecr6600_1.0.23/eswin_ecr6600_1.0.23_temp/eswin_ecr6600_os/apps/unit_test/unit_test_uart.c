#include "cli.h"
#include "hal_uart.h"
#include "stdlib.h"

#include "oshal.h"
#include "chip_pinmux.h"
#include "uart.h"
#define TEST_UART_BUF_SIZE	2048

static unsigned char  uart_buffer[TEST_UART_BUF_SIZE]  __attribute__((section(".dma.data")));

void utest_uart_isr(void * data)
{
	os_printf(LM_CMD,LL_INFO,"unit test uart, uart-isr comes!\r\n");
}

static int utest_uart_open(cmd_tbl_t *t, int argc, char *argv[])
{
	int uart_baudrate, tx_mode, rx_mode;
	chip_uart1_pinmux_cfg(0);

	if (argc >= 4)
	{
		uart_baudrate = (int)strtoul(argv[1], NULL, 0);
		tx_mode = (int)strtoul(argv[2], NULL, 0);
		rx_mode = (int)strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, err: no uart-num input!\r\n");
		return 0;
	}
	
	T_DRV_UART_CONFIG config;	
	config.uart_baud_rate = uart_baudrate;
	config.uart_data_bits = UART_DATA_BITS_8;
	config.uart_stop_bits = UART_STOP_BITS_1;
	config.uart_parity_bit = UART_PARITY_BIT_NONE;
	config.uart_flow_ctrl = UART_FLOW_CONTROL_DISABLE;
	config.uart_tx_mode = tx_mode;
	config.uart_rx_mode = rx_mode;
	config.uart_rx_buf_size = 1024;	

	int i;
	for(i=0; i<TEST_UART_BUF_SIZE; i++)
	{
		uart_buffer[i] = (unsigned char)i%256;
	}

	if (hal_uart_open(1, &config) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, uart1 open ok!\r\n");
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, uart1 open failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_uart, open, utest_uart_open, "unit test uart open", "ut_uart open [uart-baudrate] [tx_mode] [rx_mode]");


static int utest_uart_read(cmd_tbl_t *t, int argc, char *argv[])
{
    int  uart_len;

	if (argc >= 2)
	{
		uart_len = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, err: no enough argc!\r\n");
		return 0;
	}
	
    if(uart_len > TEST_UART_BUF_SIZE)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, err: input length error!\r\n");
	}

	if (hal_uart_read(1, uart_buffer, uart_len, WAIT_FOREVER) == uart_len)
	{
		int i;
		for(i=0; i<uart_len; i++)
		{
			os_printf(LM_CMD,LL_INFO,"\r\nuartbuffer[%d] == 0x%x\r\n",i,uart_buffer[i]);
		}
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, uart1 rx ok!\r\n");
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, uart1 rx failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_uart, read, utest_uart_read, "unit test uart read", "ut_uart read [uart-len]");


static int utest_uart_write(cmd_tbl_t *t, int argc, char *argv[])
{
    int uart_len;

	if (argc >= 2)
	{
		uart_len = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, err: no enough argc!\r\n");
		return 0;
	}
	
    if(uart_len > TEST_UART_BUF_SIZE)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, err: input length error!\r\n");
	}

	if (hal_uart_write(1, uart_buffer, uart_len) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, uart1 tx ok!\r\n");
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, uart1 tx failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_uart, write, utest_uart_write, "unit test uart write", "ut_uart write [uart-len]");




static int utest_uart_close(cmd_tbl_t *t, int argc, char *argv[])
{
	if (hal_uart_close(1) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, uart1 close ok!\r\n");
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test uart, uart1 close failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_uart, close, utest_uart_close, "unit test uart close", "ut_uart close");



CLI_CMD(ut_uart, NULL, "unit test uart", "test_uart");

