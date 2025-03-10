
#include "cli.h"
#include "rtc.h"
#include "telnet.h"
#include "rtos_debug.h"
#include "chip_pinmux.h"
#include "hal_wdt.h"


//#include "chip_irqvector.h"
//#include "chip_memmap.h"
//#include "arch_irq.h"
//#include "uart.h"
//#include "oshal.h"
//#include "flash.h"
//#include "amtNV.h"
//#include "ir.h"
//#include "pit.h"
//#include "psm_system.h"
//#include "rtos_debug.h"
//#include "telnet.h"
//#include "nvds.h"
//#include "hal_system.h"
//#include "adc.h"




/* cli mode info 
 * Must modified enum: _E_CLI_CMD_MODE @ cli.h
 * Must modified enum: _E_CLI_CMD_MODE @ cli.h
 * Must modified enum: _E_CLI_CMD_MODE @ cli.h
 */
char *prompt_symbol_t[] = {
	[0] = "__all", // E_ALL
	[1] = "__amt", // E_AMT
	[2] = "standalone", // E_STANDALONE
	[3] = "transport", // E_TRANSPORT
	[4] = "lmac_test", // E_LMAC_TEST
	[5] = "at", // E_AT
	/* 按格式，添加新的实例名 */
};


CLI_DEV s_cli_dev = {0};



void cli_printf(const char *f, ...)
{
        int len=0;
        struct rtc_time time={0};
        CLI_DEV *p_cli_dev = &s_cli_dev;
        if(p_cli_dev->is_open == 0)
        {
           return ;
		}
        va_list ap;
        va_start(ap,f);

        //flags = system_irq_save();
        if(p_cli_dev->prefix_open && p_cli_dev->newline_flag == 1)
        {
			p_cli_dev->cli_print_buffer[0] = '\n';
			len = 1;
        	p_cli_dev->newline_flag = 0;
			p_cli_dev->printf_timeStamp = 1;
        }
		
        if(p_cli_dev->printf_timeStamp && p_cli_dev->prefix_open)
        {
			p_cli_dev->printf_timeStamp=0;
			drv_rtc_get_time(&time);
			len += snprintf( &p_cli_dev->cli_print_buffer[len], CLI_PRINT_BUFFER_SIZE-1-len, "[%02d:%02d:%02d.%03d]", time.hour,time.min,time.sec,time.cnt_32k/33);
        }
		
        len += vsnprintf( &p_cli_dev->cli_print_buffer[len], CLI_PRINT_BUFFER_SIZE-1-len, f, ap);
        if(p_cli_dev->cli_print_buffer[len-1] == 0x0a)
        {
			p_cli_dev->printf_timeStamp=1;
        }
		if(telnet_log_write(p_cli_dev->cli_print_buffer, len) < 0)
		{
        	drv_uart_send_poll(p_cli_dev->cli_uart_num, p_cli_dev->cli_print_buffer, len);
		}

        //system_irq_restore(flags); 

        va_end(ap);
}

void cli_vprintf(const char *f,  va_list ap)
{
        //unsigned long flags;
        int len=0;
        
        struct rtc_time time={0};
        CLI_DEV *p_cli_dev = &s_cli_dev;
        if(p_cli_dev->is_open == 0)
        {
             return ;
		}

        if(p_cli_dev->prefix_open && p_cli_dev->newline_flag == 1)
        {
			p_cli_dev->cli_print_buffer[0] = '\n';
			len = 1;
        	p_cli_dev->newline_flag = 0;
			p_cli_dev->printf_timeStamp=1;
        }

        //flags = system_irq_save();
        if(p_cli_dev->printf_timeStamp && p_cli_dev->prefix_open)
        {
            p_cli_dev->printf_timeStamp=0;
            drv_rtc_get_time(&time);
            len += snprintf( &p_cli_dev->cli_print_buffer[len], CLI_PRINT_BUFFER_SIZE+1-len, "[%02d:%02d:%02d.%03d]", time.hour,time.min,time.sec,time.cnt_32k/33);
        }
        len += vsnprintf( &p_cli_dev->cli_print_buffer[len], CLI_PRINT_BUFFER_SIZE+1-len, f, ap);
        if(p_cli_dev->cli_print_buffer[len-1] == 0x0a)
        {
        	p_cli_dev->printf_timeStamp=1;
        }
		if(telnet_log_write(p_cli_dev->cli_print_buffer, len) < 0)
		{
        	drv_uart_send_poll(p_cli_dev->cli_uart_num, p_cli_dev->cli_print_buffer, len);
		}

        //system_irq_restore(flags); 

}


static int util_cmd_parse_line(char *s, char *argv[])
{
	int argc = 0;

	while (argc < CLI_CMD_MAX_ARGV)
	{

		while(( *s == ' ' ) || ( *s == '\t' ))
		{
			s++;
		}

		if (*s == '\0')
		{
			goto out;
		}

		argv[argc++] = s;

		while (*s && ( *s != ' ' ) && ( *s != '\t' ))
		{
			s++;
		}

		if (*s == '\0')
		{
			goto out;
		}

		*s++ = '\0';
	}

	cli_printf("Too many args\n");

 out:
	argv[argc] = NULL;
	return argc;
}



struct cli_cmd *find_cmd(char *cmd)
{
	struct cli_cmd *t;

	for (t = cli_cmd_start(); t < cli_cmd_end(); t++)
	{
		if (t->flag == 0 && strcmp(cmd, t->name) == 0 && (s_cli_dev.mode & t->mode))
		{
			return t;
		}
	}

	return NULL;
}



cmd_tbl_t *find_sub_cmd(cmd_tbl_t *parent, char *cmd)
{
	cmd_tbl_t *t;

	for (t = parent + 1; (t < cli_cmd_end() && (t->flag & 1) && (s_cli_dev.mode & t->mode)); t++)
	{
		if (!strcmp(t->name, cmd))
			return t;
	}

	return NULL;
}

static void cmd_process(int argc, char *argv[])
{
	cmd_tbl_t *p, *t;
	int rc = 0;

	t = find_cmd(argv[0]);
	if (!t)
	{
		cli_printf("Unknown command: %s\n", argv[0]);
		return;
	}
	else if(!t->f->used)
	{
		cli_printf("%s UNUSED!!!\n", argv[0]);
		return;
	}
	else if (t->handler == NULL || ((rc = t->handler(t, argc, argv)) == CMD_RET_UNHANDLED))
	{

		if (argc < 2)
		{
			cli_printf("%s needs sub-command\n", argv[0]);
			return;
		}

		p = t;
		t = find_sub_cmd(p, argv[1]);
		if (t == NULL)
		{
			cli_printf("%s is not a sub-command of %s\n", argv[1], argv[0]);
			return;
		}else if(!t->f->used)
		{
			cli_printf("%s %s UNUSED!!!\n", argv[0], argv[1]);
			return;
		}
		else if (t->handler == NULL)
		{
			cli_printf("no hanlder for %s\n", argv[0]);
		}
		argc--;
		argv++;

		rc = t->handler(t, argc, argv);
	}

	switch (rc) 
	{
		case CMD_RET_SUCCESS:
			cli_printf("OK\n");
			break;
		case CMD_RET_FAILURE:
			cli_printf("ERROR\n");
			break;
		case CMD_RET_USAGE:
			cli_printf("Usage: %s\n", t->usage);
			break;
		default:
			break;
	}
}



void  cli_cmd_handle(char *cmd)
{
	char *argv[CLI_CMD_MAX_ARGV] = {NULL, };
	int argc;

	if (!cmd || *cmd == '\0')
	{
		cli_printf("Command arg null\n");
		return;
	}

	if (strlen(cmd) >= CLI_CMD_RECEIVE_BUF_LEN)
	{
		cli_printf("Command too long\n");
		return;
	}

	argc = util_cmd_parse_line(cmd, argv);
	if (argc == 0)
	{
		cli_printf("Command arg error\n");
		return;
	}
	
	#ifdef TUYA_SDK_CLI_ADAPTER
	extern int proc_onecmd(int argc, char *argv[]);
	if(proc_onecmd(argc,argv))
	{
		cmd_process(argc, argv);
	}
	#else
	cmd_process(argc, argv);

	#endif

	

}



#if !CLI_NEW_HANDLE

static void print_prompt()
{
	CLI_DEV *p_cli_dev = &s_cli_dev;
	if(p_cli_dev->prefix_open)
	{
		cli_printf("[%d]%s:", p_cli_dev->cmd_line++, PROMPT_SYMBOL);
		p_cli_dev->newline_flag = 1;
	}
	else
	{
		p_cli_dev->cmd_line++;
	}
}


static void cli_hook_in_isr(char * cmd, int cmdlen)
{
	if(0 != cmd[cmdlen])
	{
		return ;
	}
	else if(0 == strcmp(cmd, "assert_in_isr"))
	{
		system_assert(0);
	}
}

void  cli_task(void *arg)
{
	char cli_data[CLI_CMD_RECEIVE_BUF_LEN];
	CLI_DEV *p_cli_dev = &s_cli_dev;
	
	while(1)
	{
		os_queue_receive(p_cli_dev->cli_queue_handle, (char *)cli_data, 0, WAIT_FOREVER);
		cli_cmd_handle(cli_data);
		print_prompt();
	}
}

void cli_cmd_deliver(char *cmd)
{
	CLI_DEV *p_cli_dev = &s_cli_dev;
	
	if(os_queue_send(p_cli_dev->cli_queue_handle, cmd, strlen(cmd), 0))
	{
		cli_printf("cli task enqueue failed!\n");
	}
}

void cli_uart_isr(void * data)
{
	CLI_DEV *p_cli_dev = &s_cli_dev;
	unsigned int uart_base_reg = p_cli_dev->cli_uart_base;
	char cli_output_buffer[CLI_CMD_RECEIVE_BUF_LEN];
	unsigned char cli_output_rd = 0;
	unsigned char cli_output_wr = 0;

	while (drv_uart_rx_tstc(uart_base_reg) || (cli_output_rd != cli_output_wr))
	{
		if (drv_uart_rx_tstc(uart_base_reg))
		{
			char c = drv_uart_rx_getc(uart_base_reg);
			switch(c)
			{
				case BACKSP_KEY:
					if (p_cli_dev->cli_isr_index)
					{
						cli_output_buffer[cli_output_wr++] = '\b';
						cli_output_buffer[cli_output_wr++] = ' ';
						cli_output_buffer[cli_output_wr++] = '\b';
						p_cli_dev->cli_isr_index--;
					}
					break;

				case RETURN_KEY:
					if(p_cli_dev->echo_open)
					{
						cli_output_buffer[cli_output_wr++] = 0xd;
						cli_output_buffer[cli_output_wr++] = 0xa;
					}
					if (p_cli_dev->cli_isr_index > 0)
					{
						p_cli_dev->cli_isr_buffer[p_cli_dev->cli_isr_index] = '\0';
						cli_hook_in_isr((char *)p_cli_dev->cli_isr_buffer, p_cli_dev->cli_isr_index);
						if(os_queue_send(p_cli_dev->cli_queue_handle, (char *)p_cli_dev->cli_isr_buffer, p_cli_dev->cli_isr_index, 0))
						{
							cli_output_wr += snprintf(&cli_output_buffer[cli_output_wr],
								CLI_CMD_RECEIVE_BUF_LEN-cli_output_wr, "cli task enqueue failed!\n");
						}
						p_cli_dev->cli_isr_index = 0;
					}

					if(p_cli_dev->prefix_open)
					{
						struct rtc_time time = {0};
						drv_rtc_get_time(&time);
						cli_output_wr += snprintf(&cli_output_buffer[cli_output_wr], CLI_CMD_RECEIVE_BUF_LEN-cli_output_wr,
							"[%02d:%02d:%02d.%03d][%d]%s:", time.hour,time.min,time.sec,time.cnt_32k/33, p_cli_dev->cmd_line++, PROMPT_SYMBOL);
						p_cli_dev->newline_flag = 1;
					}
					else
					{
						p_cli_dev->cmd_line++;
					}
					break;

				case '\n':
					break;

				default:
					if (c > 0x1F && c < 0x7F && p_cli_dev->cli_isr_index < CLI_CMD_RECEIVE_BUF_LEN)
					{
						p_cli_dev->cli_isr_buffer[p_cli_dev->cli_isr_index++] = c;
						if(p_cli_dev->echo_open)
						{
							cli_output_buffer[cli_output_wr++] = c;
						}
					}
					break;
			}
		}

		if (cli_output_rd != cli_output_wr)
		{
			if (drv_uart_tx_ready(uart_base_reg) == 0)
			{
				int len = (32 > (cli_output_wr - cli_output_rd))? (cli_output_wr - cli_output_rd):32;
				int i;
				for (i=0; i<len; i++)
				{
					drv_uart_tx_putc(uart_base_reg, cli_output_buffer[cli_output_rd++]);
				}
			}
		}
	}
}

#else


void cli_buf_handle(void)
{
	CLI_DEV *p_cli_dev = &s_cli_dev;
	unsigned int uart_base_reg = p_cli_dev->cli_uart_base;
	unsigned char wr_temp, rd_temp = p_cli_dev->cli_buf_rd;

	while (1)
	{
		unsigned int flag;

		wr_temp = p_cli_dev->cli_buf_wr;

		if (wr_temp == rd_temp)
		{
			return;			
		}

		while (wr_temp != rd_temp)
		{
			char c;

			if (wr_temp != rd_temp)
			{
				c = p_cli_dev->cli_buf_base[rd_temp++];
				rd_temp %= CLI_CMD_RECEIVE_BUF_LEN;
			}
			else
			{
				continue;
			}

			switch(c)
			{
				case BACKSP_KEY:
					if (p_cli_dev->cli_isr_index)
					{
						drv_uart_send_poll(p_cli_dev->cli_uart_num, "\b \b", 3);
						p_cli_dev->cli_isr_index--;
					}
					break;

				case RETURN_KEY:
					if (p_cli_dev->echo_open)
					{
						drv_uart_send_poll(p_cli_dev->cli_uart_num, "\n", 1);

						if (p_cli_dev->cli_isr_index)
						{
							p_cli_dev->newline_flag = 0;
						}
					}

					if (p_cli_dev->cli_isr_index)
					{
						p_cli_dev->cli_isr_buffer[p_cli_dev->cli_isr_index] = '\0';
						cli_cmd_handle(p_cli_dev->cli_isr_buffer);
						//print_prompt();
						p_cli_dev->cli_isr_index = 0;
					}

					if(p_cli_dev->prefix_open)
					{
						int len;
						struct rtc_time time = {0};
						drv_rtc_get_time(&time);
						
						len = snprintf(&p_cli_dev->cli_print_buffer[0], CLI_PRINT_BUFFER_SIZE, 
							"[%02d:%02d:%02d.%03d][%d]%s:", time.hour,time.min,time.sec,time.cnt_32k/33, 
							p_cli_dev->cmd_line++, PROMPT_SYMBOL);
						
						drv_uart_send_poll(p_cli_dev->cli_uart_num, &p_cli_dev->cli_print_buffer[0], len);

						p_cli_dev->newline_flag = 1;
					}
					else
					{
						p_cli_dev->cmd_line++;
					}
					break;

				case '\n':
					break;

				default:
					if (c > 0x1F && c < 0x7F && p_cli_dev->cli_isr_index < CLI_CMD_RECEIVE_BUF_LEN)
					//if (p_cli_dev->cli_isr_index < CLI_CMD_RECEIVE_BUF_LEN)
					{
						p_cli_dev->cli_isr_buffer[p_cli_dev->cli_isr_index++] = c;
						if(p_cli_dev->echo_open)
						{
							while(drv_uart_tx_ready(uart_base_reg));
							drv_uart_tx_putc(uart_base_reg, c);
						}
					}
					break;
			}
		}

		flag = system_irq_save();
		p_cli_dev->cli_buf_rd = rd_temp;
		system_irq_restore(flag);
	}


}

void  cli_task(void *arg)
{
	while(1)
	{
		os_sem_wait(s_cli_dev.cli_sem, WAIT_FOREVER);
		cli_buf_handle();
	}
}


void cli_cmd_deliver(char *cmd)
{}

int g_assert_index = 0;
static char s_assert_in_isr[] = "assert_in_isr";
void cli_uart_isr(void * data)
{
	CLI_DEV *p_cli_dev = &s_cli_dev;
	unsigned int uart_base_reg = p_cli_dev->cli_uart_base;
	char temp_char;

	while (drv_uart_rx_tstc(uart_base_reg))
	{
		temp_char = drv_uart_rx_getc(uart_base_reg);

		if (temp_char != s_assert_in_isr[g_assert_index])
		{
			g_assert_index = 0;
		}
		else
		{
			if (++g_assert_index == 13)
			{
				hal_wdt_stop();
				system_assert(0);
			}
		}
		
		if (((p_cli_dev->cli_buf_wr + 1) % CLI_CMD_RECEIVE_BUF_LEN) == p_cli_dev->cli_buf_rd)
		{
			drv_uart_send_poll(p_cli_dev->cli_uart_num, "cli isr no buf!\n", 16);
			continue;
		}
		
		p_cli_dev->cli_buf_base[p_cli_dev->cli_buf_wr++] = temp_char;
		p_cli_dev->cli_buf_wr %= CLI_CMD_RECEIVE_BUF_LEN;
	}

#if 1
	os_sem_post(p_cli_dev->cli_sem);
#else
	if(os_sem_post(p_cli_dev->cli_sem))
	{
		drv_uart_send_poll(p_cli_dev->cli_uart_num, "cli sem failed!\n", 15);
	}
#endif
}

#endif

void component_cli_init(E_DRV_UART_NUM uart_num)
{
	T_DRV_UART_CONFIG config;
	CLI_DEV *p_cli_dev = &s_cli_dev;
	T_UART_ISR_CALLBACK callback;

	config.uart_baud_rate = 115200;
	config.uart_data_bits = UART_DATA_BITS_8;
	config.uart_stop_bits = UART_STOP_BITS_1;
	config.uart_parity_bit = UART_PARITY_BIT_NONE;
	config.uart_flow_ctrl = UART_FLOW_CONTROL_DISABLE;
	config.uart_tx_mode = UART_TX_MODE_STREAM;
	config.uart_rx_mode = UART_RX_MODE_USER;

	if(E_UART_SELECT_BY_KCONFIG == uart_num)
	{
#ifdef CONFIG_CLI_UART_0
		uart_num = E_UART_NUM_0;
#endif
#ifdef CONFIG_CLI_UART_1
		uart_num = E_UART_NUM_1;
#endif
#ifdef CONFIG_CLI_UART_2
		uart_num = E_UART_NUM_2;
#endif
	}
	
	switch(uart_num)
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

#if CLI_NEW_HANDLE
	if((p_cli_dev->cli_sem != 0) && (uart_num != p_cli_dev->cli_uart_num))
#else
	if((p_cli_dev->cli_queue_handle != 0) && (uart_num != p_cli_dev->cli_uart_num))
#endif
	{
		drv_uart_close(p_cli_dev->cli_uart_num);
		drv_uart_open(uart_num, &config);
	}
	else
	{
		drv_uart_open(uart_num, &config);
	}

	callback.uart_callback = cli_uart_isr;
	callback.uart_data = (void *)p_cli_dev;

	drv_uart_ioctrl(uart_num, DRV_UART_CTRL_REGISTER_RX_CALLBACK, &callback);
	drv_uart_ioctrl(uart_num, DRV_UART_CTRL_GET_REG_BASE, (void *)(&p_cli_dev->cli_uart_base));

	p_cli_dev->cli_uart_num = uart_num;
#if CLI_NEW_HANDLE
	if(p_cli_dev->cli_sem == 0)
		p_cli_dev->cli_sem = os_sem_create(1, 0);
#else
	if(p_cli_dev->cli_queue_handle == 0)
		p_cli_dev->cli_queue_handle = os_queue_create("queue_cli", CLI_CMD_RECEIVE_BUF_NUM, CLI_CMD_RECEIVE_BUF_LEN, 0);
#endif
	if(p_cli_dev->cli_task_handle == 0)
		p_cli_dev->cli_task_handle = os_task_create("cli-task", FHOST_CLI_PRIORITY, FHOST_CLI_STACK_SIZE, (task_entry_t)cli_task, NULL);
	p_cli_dev->is_open = 1;
	p_cli_dev->echo_open = 1;
	p_cli_dev->newline_flag = 0;
	p_cli_dev->cmd_line = 0;
	p_cli_dev->prefix_open = 1;
	p_cli_dev->printf_timeStamp = 1;

	int i=1;
	for(; i<E_MAX_NUM; i++)
	{
		if(strcmp(PROMPT_SYMBOL, prompt_symbol_t[i]) == 0)
		{
			p_cli_dev->mode = 1<<(i-1);
			break;
		}
	}
	if(i==E_MAX_NUM)
	{
		p_cli_dev->mode = E_OTHER;
	}
}

void AmtInit()
{
#ifdef CONFIG_AMT_UART_0
    component_cli_init(E_UART_NUM_0);
#elif CONFIG_AMT_UART_1
    component_cli_init(E_UART_NUM_1);
#elif CONFIG_AMT_UART_2
    component_cli_init(E_UART_NUM_2);
#endif

    s_cli_dev.prefix_open = 0;
    s_cli_dev.mode = E_AMT;
}

