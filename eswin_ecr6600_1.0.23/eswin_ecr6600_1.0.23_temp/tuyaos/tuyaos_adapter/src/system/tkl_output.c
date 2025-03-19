/**
 * @file tuya_os_adapt_output.c
 * @brief 日志操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include "stdarg.h"
#include "string.h"
#include "tkl_output.h"
#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "cli.h"
#include "rtc.h"
#include "hal_uart.h"
#include "oshal.h"

/***********************************************************
*************************micro define***********************
***********************************************************/

#if defined(ENABLE_LOG_OUTPUT_FORMAT) && (ENABLE_LOG_OUTPUT_FORMAT == 1)
#ifndef MAX_SIZE_OF_DEBUG_BUF
#define MAX_SIZE_OF_DEBUG_BUF (1024)
#endif

STATIC CHAR_T s_output_buf[MAX_SIZE_OF_DEBUG_BUF] = {0};
#endif

/* 终端输出函数 */
#define OutputPrint printf
#define MAX_LOG_LEN 256
/***********************************************************
*************************variable define********************
***********************************************************/


static int log_print_on=1;

char log_print_buffer[MAX_LOG_LEN];

/***********************************************************
*************************function define********************
***********************************************************/
/*
//log print
void cli_vprintf(const char *f,  va_list ap)
{
    if(!log_print_on)
    {
    	return;
    }
    
	unsigned long flags;
    int len=0;
    static int printf_timeStamp=1;
    struct rtc_time time={0};

    flags = system_irq_save();
    if(printf_timeStamp)
    {
        printf_timeStamp=0;
        drv_rtc_get_time(&time);
        len = snprintf( &log_print_buffer, MAX_LOG_LEN, "[%02d:%02d:%02d.%03d]", time.hour,time.min,time.sec,time.cnt_32k/33);
    }
    len += vsnprintf(&log_print_buffer[len], MAX_LOG_LEN-len, f, ap);
    if(log_print_buffer[len-1] == 0x0a)
    {
        printf_timeStamp=1;
    }

    drv_uart_send_poll(E_UART_NUM_0, log_print_buffer, len);
    system_irq_restore(flags); 

}


int printf(char *f, ...)
{
	va_list ap;
	va_start(ap,f);
	cli_vprintf(f,ap);
	va_end(ap);
}

void cli_printf(const char *f, ...)
{
	va_list ap;
	va_start(ap,f);
	
	cli_vprintf(f,ap);

	va_end(ap);
}

*/



/**
 * @brief tuya_os_adapt_output_log用于输出log信息
 * 
 * @param[in] str log buffer指针
 */
VOID_T tkl_log_output(CONST CHAR_T *str, ...)
{
    if(str == NULL) {
        return;
    }

#if defined(ENABLE_LOG_OUTPUT_FORMAT) && (ENABLE_LOG_OUTPUT_FORMAT == 1)
    va_list ap;

    va_start(ap, str);
    vsnprintf(s_output_buf, MAX_SIZE_OF_DEBUG_BUF,str,ap);
    va_end(ap);
    os_printf(LM_APP, LL_INFO, "%s",s_output_buf);
#else
    os_printf(LM_APP, LL_INFO, "%s",str);
#endif

/*
    int len=strlen(str);
    unsigned long flags;
   
    flags = system_irq_save();
    drv_uart_send_poll(E_UART_NUM_0, str, len);
    system_irq_restore(flags); 
    //OutputPrint((char *)str);
*/
}


/**
 * @brief 用于关闭原厂sdk默认的log口
 * 
 */
OPERATE_RET tkl_log_close(VOID_T)
{
    log_print_on=0;
    return OPRT_OK;
}

/**
 * @brief 用于恢复原厂sdk默认的log口
 * 
 */
OPERATE_RET tkl_log_open(VOID_T)
{
    log_print_on=1;
    return OPRT_OK;
}


