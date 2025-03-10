

#ifndef _CLI11_H_
#define _CLI11_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "uart.h"
#include "rtc.h"
#include "stdarg.h"
#include "oshal.h"
#include "easyflash.h"

//jira[ECR6600-930] Reduce interrupt processing time
#define CLI_NEW_HANDLE		1


#define	CMD_RET_SUCCESS 	0
#define	CMD_RET_FAILURE 	1
#define	CMD_RET_UNHANDLED 	2
#define	CMD_RET_USAGE  		-1


#define	CMD_ATTR_TOP 		(0)
#define	CMD_ATTR_SUB 		(1)

#define ESCAPE_KEY 			0x1b
#define BACKSP_KEY 			0x08
#define TAB_KEY    			0x09
#define RETURN_KEY 			0x0D
#define DELETE_KEY 			0x7F
#define BELL       			0x07

#define CLI_CMD_RECEIVE_BUF_NUM			(4)
#define CLI_CMD_RECEIVE_BUF_LEN			(128)
#define CLI_CMD_MAX_ARGV				(18)
#define CLI_PRINT_BUFFER_SIZE			(512)
#define CLI_PRINT_UART_FIFO_DEPTH		(32)




/* 该枚举变量和prompt_symbol_t@cli.c中的字符串一一对应，字符串为编译实例的文件夹名，运行时自动匹配 */
typedef enum _E_CLI_CMD_MODE
{
	// 为了使一个命令归属于多个实例，使用bit标识不同的实例。最多支持32个实测。
	E_ALL = 0xFFFFFFFF,			// 0
	E_AMT = 0x1,				// 1
	E_STANDALONE = 0x2,			// 2
	E_TRANSPORT = 0x4,			// 3
	E_LMAC_TEST = 0x8,			// 4
	E_AT = 0x10,				// 5
	/* 按格式添加实例枚举变量，
	 * 修改E_MAX_NUM
	 */
	
	E_OTHER = 0x80000000,		// Revert
	E_MAX_NUM = 6  				// 6 当前实例的最大值，边界条件。修改时请维护该MAX值，保证始终为枚举值的数量

} E_CLI_CMD_MODE;

typedef struct cli_cmd_f
{
	bool used;
} cmd_tbl_f_t;

typedef struct cli_cmd
{
	const char *name;
	int (*handler)(struct cli_cmd *t, int argc, char *argv[]);
	const char *desc;
	const char *usage;
	int flag;
	E_CLI_CMD_MODE mode;
	cmd_tbl_f_t * f;
} cmd_tbl_t;


typedef struct _CLI_DEV
{
	int cli_task_handle;
#if CLI_NEW_HANDLE
	os_sem_handle_t cli_sem;
#else
	os_queue_handle_t cli_queue_handle;
#endif
	unsigned int cli_uart_base;
	char cli_print_buffer[CLI_PRINT_BUFFER_SIZE];
	char cli_isr_buffer[CLI_CMD_RECEIVE_BUF_LEN];
#if CLI_NEW_HANDLE
	char cli_buf_base[CLI_CMD_RECEIVE_BUF_LEN * CLI_CMD_RECEIVE_BUF_NUM];
	unsigned short cli_buf_wr;
	unsigned short cli_buf_rd;
#endif
	unsigned short cli_isr_index;
//	unsigned short cli_pro_index;
	E_DRV_UART_NUM cli_uart_num;
	int cmd_line;
    char is_open;
	char echo_open;
	char newline_flag;
	int prefix_open;
	int printf_timeStamp;
	E_CLI_CMD_MODE mode;
} CLI_DEV;


#define cli_cmd_start()							\
({									\
	static char start[0] __aligned(4) __attribute__((used,	\
		section(".cli_cmd_1")));			\
	(struct cli_cmd *)&start;					\
})


#define cli_cmd_end()							\
({									\
	static char end[0] __aligned(4) __attribute__((used,		\
		section(".cli_cmd_3")));				\
	(struct cli_cmd *)&end;						\
})



#define cli_cmd_declare(cmd)						\
	struct cli_cmd _cli_cmd_2_##cmd			\
	__attribute__((used, section(".cli_cmd_2_"#cmd), aligned(4)))




#define cli_subcmd_start(cmd)						\
({									\
	static char start[0] __aligned(4) __attribute__((used,	\
		section(".cli_cmd_2_"#cmd"_1")));			\
	(struct cli_cmd *)&start;					\
})

#define cli_subcmd_end(cmd)						\
({									\
	static char end[0] __aligned(4) __attribute__((used,		\
		section(".cli_cmd_2_"#cmd"_3")));			\
	(struct cli_cmd *)&end;						\
})





#define cli_subcmd_declare(cmd, subcmd)					\
	struct cli_cmd _cli_cmd_2_##cmd##_2_##subcmd	\
	__attribute__((used, 						\
		       section(".cli_cmd_2_"#cmd"_2_"#subcmd), aligned(4)))


#define CLI_CMD(cmd, fn, d, u)				\
	struct cli_cmd_f _cli_cmd_2_##cmd##_f = {	\
		.used = true,						\
	};											\
	cli_cmd_declare(cmd) = {				\
		.name = #cmd,				\
		.handler = fn, 				\
		.desc = d, 				\
		.usage = u,				\
		.flag = CMD_ATTR_TOP,	\
		.mode = E_ALL, \
		.f = &_cli_cmd_2_##cmd##_f, \
	}
	
#define CLI_CMD_F(cmd, fn, d, u, flage)	\
		struct cli_cmd_f _cli_cmd_2_##cmd##_f = {	\
			.used = flage,						\
		};											\
		cli_cmd_declare(cmd) = {				\
			.name = #cmd,				\
			.handler = fn,				\
			.desc = d,				\
			.usage = u, 			\
			.flag = CMD_ATTR_TOP,	\
			.mode = E_ALL, \
			.f = &_cli_cmd_2_##cmd##_f, \
		}


#define CLI_CMD_M(cmd, fn, d, u, m)				\
	cli_cmd_declare(cmd) = {				\
		.name = #cmd,				\
		.handler = fn, 				\
		.desc = d, 				\
		.usage = u,				\
		.flag = CMD_ATTR_TOP,	\
		.mode = m, \
	}


#define CLI_SUBCMD(cmd, subcmd,fn, d, u)			\
		struct cli_cmd_f _cli_cmd_2_##cmd##_2_##subcmd##_f = {	\
			.used = true,						\
		};														\
		cli_subcmd_declare(cmd, subcmd) = { 		\
			.name = #subcmd,			\
			.handler = fn,				\
			.desc = d,				\
			.usage = u, 			\
			.flag = CMD_ATTR_SUB,	\
			.mode = E_ALL, \
			.f = &_cli_cmd_2_##cmd##_2_##subcmd##_f,\
		}
		
#define CLI_SUBCMD_F(cmd, subcmd,fn, d, u, flage)			\
				struct cli_cmd_f _cli_cmd_2_##cmd##_2_##subcmd##_f = {	\
					.used = flage,						\
				};														\
				cli_subcmd_declare(cmd, subcmd) = { 		\
					.name = #subcmd,			\
					.handler = fn,				\
					.desc = d,				\
					.usage = u, 			\
					.flag = CMD_ATTR_SUB,	\
					.mode = E_ALL, \
					.f = &_cli_cmd_2_##cmd##_2_##subcmd##_f,\
				}



#define CLI_SUBCMD_M(cmd, subcmd,fn, d, u, m)			\
	cli_subcmd_declare(cmd, subcmd) = { 		\
		.name = #subcmd,			\
		.handler = fn, 				\
		.desc = d,	 			\
		.usage = u,	 			\
		.flag = CMD_ATTR_SUB,	\
		.mode = m, \
	}

#define CLI_CMD_GET(cmd) &_cli_cmd_2_##cmd
#define CLI_SUBCMD_GET(cmd, subcmd) &_cli_cmd_2_##cmd##_2_##subcmd



//void cli_printf(const char *f, ...);
//void cli_vprintf(const char *f,  va_list ap);
void component_cli_init(E_DRV_UART_NUM uart_num);
void cli_cmd_deliver(char *cmd);
struct cli_cmd *find_cmd(char *cmd);
cmd_tbl_t *find_sub_cmd(cmd_tbl_t *parent, char *cmd);





#endif
