/**
 * @file tuya_os_adapt_system.c
 * @brief 操作系统相关接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#define _UNI_SYSTEM_GLOBAL

#include "tkl_output.h"
#include "tkl_system.h"
#include "tkl_watchdog.h"
#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
//#include "arch_irq.h"
#include "hal_gpio.h"
#include "chip_pinmux.h"
#include "FreeRTOS.h"
#include "task.h"
#include "oshal.h"
#include "hal_wdt.h"
#include "hal_system.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif
#include "tkl_sleep.h"
#if defined(CONFIG_PSM_SURPORT) && defined(CONFIG_TUYA_DOORCONTACT)
#include "tkl_wakeup.h"
#endif
/***********************************************************
*************************micro define***********************
***********************************************************/
#ifndef BIT
#define BIT(x) (1U << (x))
#endif

#define TKL_WAKEUP_LEVE_LOW_GPIO_NUM_0      BIT(8)
#define TKL_WAKEUP_LEVE_LOW_GPIO_NUM_1      BIT(9)
#define TKL_WAKEUP_LEVE_LOW_GPIO_NUM_2      BIT(10)
#define TKL_WAKEUP_LEVE_LOW_GPIO_NUM_17     BIT(6)

#define TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_0     BIT(3)
#define TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_1     BIT(4)
#define TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_2     BIT(5)
#define TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_17    BIT(1)

#define TKL_WAKEUP_SOURCE_TYPE_INT          BIT(11)

#define IS_TKL_GPIO_NUM_0_LOW_LEVEL_WAKEUP(v) (((v) & TKL_WAKEUP_LEVE_LOW_GPIO_NUM_0) == TKL_WAKEUP_LEVE_LOW_GPIO_NUM_0)
#define IS_TKL_GPIO_NUM_1_LOW_LEVEL_WAKEUP(v) (((v) & TKL_WAKEUP_LEVE_LOW_GPIO_NUM_1) == TKL_WAKEUP_LEVE_LOW_GPIO_NUM_1)
#define IS_TKL_GPIO_NUM_2_LOW_LEVEL_WAKEUP(v) (((v) & TKL_WAKEUP_LEVE_LOW_GPIO_NUM_2) == TKL_WAKEUP_LEVE_LOW_GPIO_NUM_2)
#define IS_TKL_GPIO_NUM_17_LOW_LEVEL_WAKEUP(v) (((v) & TKL_WAKEUP_LEVE_LOW_GPIO_NUM_17) == TKL_WAKEUP_LEVE_LOW_GPIO_NUM_17)

#define IS_TKL_GPIO_NUM_0_HIGH_LEVEL_WAKEUP(v) (((v) & TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_0) == TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_0)
#define IS_TKL_GPIO_NUM_1_HIGH_LEVEL_WAKEUP(v) (((v) & TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_1) == TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_1)
#define IS_TKL_GPIO_NUM_2_HIGH_LEVEL_WAKEUP(v) (((v) & TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_2) == TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_2)
#define IS_TKL_GPIO_NUM_17_HIGH_LEVEL_WAKEUP(v) (((v) & TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_17) == TKL_WAKEUP_LEVE_HIGH_GPIO_NUM_17)

#define IS_TKL_TIMER_INT_WAKEUP(v) (((v) & TKL_WAKEUP_SOURCE_TYPE_INT) == TKL_WAKEUP_SOURCE_TYPE_INT)

#define IS_TKL_GPIO_NUM_0_WAKEUP(v) (IS_TKL_GPIO_NUM_0_LOW_LEVEL_WAKEUP(v) || IS_TKL_GPIO_NUM_0_HIGH_LEVEL_WAKEUP(v))
#define IS_TKL_GPIO_NUM_1_WAKEUP(v) (IS_TKL_GPIO_NUM_1_LOW_LEVEL_WAKEUP(v) || IS_TKL_GPIO_NUM_1_HIGH_LEVEL_WAKEUP(v))
#define IS_TKL_GPIO_NUM_2_WAKEUP(v) (IS_TKL_GPIO_NUM_2_LOW_LEVEL_WAKEUP(v) || IS_TKL_GPIO_NUM_2_HIGH_LEVEL_WAKEUP(v))
#define IS_TKL_GPIO_NUM_17_WAKEUP(v) (IS_TKL_GPIO_NUM_17_LOW_LEVEL_WAKEUP(v) || IS_TKL_GPIO_NUM_17_HIGH_LEVEL_WAKEUP(v))

#define IS_TKL_GPIO_HIGH_LEVEL_WAKEUP(v) (IS_TKL_GPIO_NUM_0_HIGH_LEVEL_WAKEUP(v) || IS_TKL_GPIO_NUM_1_HIGH_LEVEL_WAKEUP(v) || IS_TKL_GPIO_NUM_2_HIGH_LEVEL_WAKEUP(v) || IS_TKL_GPIO_NUM_17_HIGH_LEVEL_WAKEUP(v))
#if defined(CONFIG_PSM_SURPORT) && defined(CONFIG_TUYA_DOORCONTACT)
extern unsigned int psm_get_wakeup_gpio();
#endif
extern size_t xPortGetMinimumEverFreeHeapSize( void );
extern size_t xPortGetFreeHeapSize( void );
extern int arch_irq_context(void);
/***********************************************************
*************************variable define********************
***********************************************************/
bool is_lp_enable = false;



/***********************************************************
*************************extern define********************
***********************************************************/
extern void ota_platform_reset(void);
extern void srand(unsigned int seed);
extern int rand(void);
#if defined(CONFIG_PSM_SURPORT) && defined(CONFIG_TUYA_DOORCONTACT)
extern void psm_set_aon_gpio(unsigned int pin , unsigned int type);
extern void pin_func_set_gpio(int gpio_num);
extern void psm_enable_lower_deep_mode(bool status);
#endif
/***********************************************************
*************************function define********************
***********************************************************/
static void tkl_watchdog_isr(void)
{
	hal_set_reset_type(RST_TYPE_SOFTWARE_WDT_TIMEOUT);
}

/**
 * @brief tuya_os_adapt_system_sleep用于系统sleep
 * 
 * @param[in] msTime sleep time is ms
 */
VOID_T tkl_system_sleep(UINT_T num_ms)
{
    os_msleep(num_ms);
}


/**
 * @brief tuya_os_adapt_system_isrstatus用于检查系统是否处于中断上下文
 * 
 * @return true 
 * @return false 
 */
bool tuya_os_adapt_system_isrstatus(void)
{
    bool ret = false;
    if(arch_irq_context() != 0)
    {
    	ret = true;
    }

    return ret;
}


/**
 * @brief tuya_os_adapt_system_reset用于重启系统
 * 
 */
void tkl_system_reset(void)
{
    hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);
}

/**
 * @brief tuya_os_adapt_system_getheapsize用于获取堆大小/剩余内存大小
 * 
 * @return int <0: don't support  >=0: current heap size/free memory
 */
int tkl_system_getheapsize(void)
{
  return  (int)xPortGetFreeHeapSize();
}

/**
 * @brief tuya_os_adapt_system_getMiniheapsize/最小剩余内存大小
 * 
 * @return int <0: don't support  >=0: mini heap size/free memory
 */
int tkl_system_getmaxblocksize(void)
{
   return xPortGetMinimumEverFreeHeapSize();
}

/**
 * @brief tuya_os_adapt_system_get_rst_info用于获取硬件重启原因
 * 
 * @return 硬件重启原因
 */
TUYA_RESET_REASON_E tkl_system_get_reset_reason(CHAR_T** ext_info)
{
    TUYA_RESET_REASON_E tuya_rst_reason = TUYA_RESET_REASON_UNSUPPORT;
    RST_TYPE reset_type = hal_get_reset_type();
    os_printf(LM_APP, LL_INFO, "%s: reset_type %d\r\n", __func__, reset_type);
    switch (reset_type)
    {
        case RST_TYPE_POWER_ON:
        {   
            tuya_rst_reason = TUYA_RESET_REASON_POWERON;
            break;
        }
        
        case RST_TYPE_FATAL_EXCEPTION: //tuya consider as soft watchdog
        {
            tuya_rst_reason = TUYA_RESET_REASON_SW_WDOG;
            break;
        }
        
        case RST_TYPE_SOFTWARE_REBOOT:
        {
            tuya_rst_reason = TUYA_RESET_REASON_SOFTWARE;
            break;
        }
        
        case RST_TYPE_HARDWARE_REBOOT:
        {
            tuya_rst_reason = TUYA_RESET_REASON_FAULT;
            break;
        }
        
        case RST_TYPE_OTA:
        {
            tuya_rst_reason = TUYA_RESET_REASON_BROWNOUT;
            break;
        }
        
        case RST_TYPE_WAKEUP:
        {
            tuya_rst_reason = TUYA_RESET_REASON_DEEPSLEEP;
            break;
        }
        
        case RST_TYPE_HARDWARE_WDT_TIMEOUT:
        {
            tuya_rst_reason = TUYA_RESET_REASON_HW_WDOG;
            break;
        }

        case RST_TYPE_SOFTWARE_WDT_TIMEOUT:
        {
            tuya_rst_reason = TUYA_RESET_REASON_SW_WDOG;
            break;
        }

        default:
        {
            return TUYA_RESET_REASON_UNSUPPORT;
        }
    }

    return tuya_rst_reason;
}

/**
 * @brief init random
 *
 * @param  void
 * @retval void
 */
void tuya_os_adapt_srandom(void)
{
    return;
}


/**
 * @brief tuya_os_adapt_get_random_data用于获取指定条件下的随机数
 * 
 * @param[in] range 
 * @return 随机值
 */
int tkl_system_get_random(const unsigned int range)
{
       return (int)rand()%range;
}

/**
 * @brief tuya_os_adapt_set_cpu_lp_mode用于设置cpu的低功耗模式
 * 
 * @param[in] en 
 * @param[in] mode
 * @return int 0=成功，非0=失败
 */
OPERATE_RET tkl_cpu_sleep_mode_set(BOOL_T enable, TUYA_CPU_SLEEP_MODE_E mode)
{
    os_printf(LM_APP, LL_INFO, "%s: enable %d mode %d\r\n", __func__, enable, mode);
#ifdef CONFIG_PSM_SURPORT
	switch (mode)
	{
		case TUYA_CPU_SLEEP:
			if (enable)
			{
				PSM_SLEEP_SET(MODEM_SLEEP);
			}
			else
			{
				PSM_SLEEP_CLEAR(MODEM_SLEEP);
			}
		break;
		case TUYA_CPU_DEEP_SLEEP:
#ifdef CONFIG_TUYA_DOORCONTACT
            if (enable) {
                psm_enable_lower_deep_mode(true);
                psm_deep_sleep(0);
            }
#else
			if (enable) {
			 	PSM_SLEEP_SET(DEEP_SLEEP);
			} else {
				PSM_SLEEP_CLEAR(DEEP_SLEEP);
			}
#endif			
		break;
		default:
			return OPRT_OS_ADAPTER_INVALID_PARM;
		break;
	}
#endif

	return OPRT_OK;
}

/**
 * @brief 用于初始化并运行watchdog
 * 
 * @param[in] timeval watch dog检测时间间隔：如果timeval大于看门狗的
 * 最大可设置时间，则使用平台可设置时间的最大值，并且返回该最大值
 * @return int [out] 实际设置的看门狗时间
 */
UINT_T tkl_watchdog_init(TUYA_WDOG_BASE_CFG_T *cfg)
{
    unsigned int effective_time = 0;
    drv_wdt_isr_register(tkl_watchdog_isr);
    hal_wdt_init(cfg->interval_ms, &effective_time);
    return effective_time;
}

/**
 * @brief 用于刷新watch dog
 * 
 */
OPERATE_RET tkl_watchdog_refresh(VOID_T)
{
    hal_wdt_feed();
    return OPRT_OK;
}

/**
 * @brief 用于停止watch dog
 * 
 */
OPERATE_RET tkl_watchdog_deinit(VOID_T)
{
    hal_wdt_stop();
    return OPRT_OK;
}

int tuya_os_adapt_system_delay_ms(unsigned int num_ms)
{
    os_msdelay(num_ms);
    return OPRT_OK;
}


/**
* @brief Get system free heap size
*
* @param none
*
* @return heap size
*/
INT_T tkl_system_get_free_heap_size(VOID_T)
{
    return  (int)xPortGetFreeHeapSize();
}

SYS_TICK_T tkl_system_get_tick_count(VOID_T)
{
    return (SYS_TICK_T)xTaskGetTickCount();
}

SYS_TIME_T tkl_system_get_millisecond(VOID_T)
{
    return tkl_system_get_tick_count() * portTICK_RATE_MS;
}

/**
* @brief system delay
*
* @param[in] msTime: time in MS
*
* @note This API is used for system sleep.
*
* @return VOID
*/
VOID_T tkl_system_delay(UINT_T num_ms)
{
    tkl_system_sleep(num_ms);
}

OPERATE_RET tkl_system_get_cpu_info(TUYA_CPU_INFO_T **cpu_ary, INT32_T *cpu_cnt)
{
	OPRT_NOT_SUPPORTED;
}

#if defined(CONFIG_PSM_SURPORT) && defined(CONFIG_TUYA_DOORCONTACT)
/**
 * @brief wake up source set
 * 
 * @param[in] param: wake up source set,
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wakeup_source_set(CONST TUYA_WAKEUP_SOURCE_BASE_CFG_T  *param)
{
    unsigned int value, sleep_time;
    TUYA_GPIO_BASE_CFG_T cfg;
    int gpio_num, direct, mode, level;

    if (NULL == param) {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_WAKEUP_SOURCE_GPIO == param->source) {
        memset(&cfg, 0, sizeof(cfg));
        cfg.direct = TUYA_GPIO_INPUT;
        if (TUYA_GPIO_LEVEL_HIGH == param->wakeup_para.gpio_param.level) {
            cfg.mode = TUYA_GPIO_PULLDOWN;
            cfg.level = TUYA_GPIO_LEVEL_LOW;
        } else if (TUYA_GPIO_LEVEL_LOW == param->wakeup_para.gpio_param.level) {
            cfg.mode = TUYA_GPIO_PULLUP;
            cfg.level = TUYA_GPIO_LEVEL_HIGH;
        } else {
            return OPRT_NOT_SUPPORTED;
        }

        gpio_num = param->wakeup_para.gpio_param.gpio_num;
        direct = (TUYA_GPIO_INPUT == cfg.direct) ? GPIO_INPUT : GPIO_OUTPUT;
        mode = (TUYA_GPIO_PULLUP == cfg.mode) ? DRV_GPIO_ARG_PULL_UP : DRV_GPIO_ARG_PULL_DOWN;
        level = (TUYA_GPIO_LEVEL_HIGH == cfg.level) ? DRV_GPIO_LEVEL_HIGH : DRV_GPIO_LEVEL_LOW;

        // os_printf(LM_APP, LL_INFO, "%s: gpio_num %d direct %d mode %d level %d\r\n",
        //     __func__, gpio_num, direct, mode, level);
        
        pin_func_set_gpio(gpio_num);
        psm_gpio_wakeup_config(gpio_num, 1);
        hal_gpio_dir_set(gpio_num, direct);
        //hal_gpio_set_pull_mode(gpio_num, mode);
        //hal_gpio_write(gpio_num, level);
        //psm_set_aon_gpio(gpio_num, mode);
    } else if (TUYA_WAKEUP_SOURCE_TIMER == param->source) {
        // os_printf(LM_APP, LL_INFO, "%s: timer_num %d mode %d ms %d\r\n",
        //     __func__, param->wakeup_para.timer_param.timer_num,
        //     param->wakeup_para.timer_param.mode, param->wakeup_para.timer_param.ms);
        psm_enable_lower_deep_mode(true);
        if (param->wakeup_para.timer_param.ms < 1000) {
            sleep_time = 1;
        } else {
            sleep_time = (param->wakeup_para.timer_param.ms + 1000 - 1) / 1000;
        }
        psm_deep_sleep(sleep_time);
    } else {
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

/**
 * @brief wake up source clear
 * 
 * @param[in] param:  wake up source clear,
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wakeup_source_clear(CONST TUYA_WAKEUP_SOURCE_BASE_CFG_T *param)
{
    if (NULL == param) {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_WAKEUP_SOURCE_GPIO == param->source) {
        psm_gpio_wakeup_config(param->wakeup_para.gpio_param.gpio_num, 0);
    } else if (TUYA_WAKEUP_SOURCE_TIMER == param->source) {
        psm_deep_sleep(0);
    } else {
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_wakeup_source_type_get(TUYA_WAKEUP_SOURCE_BASE_CFG_T *param)
{
    unsigned int value;

    value = psm_get_wakeup_gpio();

    if (IS_TKL_TIMER_INT_WAKEUP(value)) {
        param->source = TUYA_WAKEUP_SOURCE_TIMER;
        //os_printf(LM_APP, LL_INFO, "%s: value 0x%x timer int wakeup %d\r\n", __func__, value, param->source);
        return OPRT_OK;
    }
    
    if (IS_TKL_GPIO_NUM_0_WAKEUP(value)) {
        param->wakeup_para.gpio_param.gpio_num = TUYA_GPIO_NUM_0;
    } else if (IS_TKL_GPIO_NUM_1_WAKEUP(value)) {
        param->wakeup_para.gpio_param.gpio_num = TUYA_GPIO_NUM_1;
    } else if (IS_TKL_GPIO_NUM_2_WAKEUP(value)) {
        param->wakeup_para.gpio_param.gpio_num = TUYA_GPIO_NUM_2;
    } else if (IS_TKL_GPIO_NUM_17_WAKEUP(value)) {
        param->wakeup_para.gpio_param.gpio_num = TUYA_GPIO_NUM_17;
    } else {
        //os_printf(LM_APP, LL_INFO, "%s: get invalud wakeup params(value=0x%x)\r\n", __func__, value);
        return OPRT_COM_ERROR;
    }

    param->source = TUYA_WAKEUP_SOURCE_GPIO;
    param->wakeup_para.gpio_param.level = IS_TKL_GPIO_HIGH_LEVEL_WAKEUP(value) ? TUYA_GPIO_LEVEL_HIGH : TUYA_GPIO_LEVEL_LOW;

    //os_printf(LM_APP, LL_INFO, "%s: value 0x%x wakeup source %d gpio_num %d level %d\r\n",
    //    __func__, value, param->source, param->wakeup_para.gpio_param.gpio_num, param->wakeup_para.gpio_param.level);

    return OPRT_OK;
}
#endif

#if 0
#include "cli.h"
int fhost_iot_wakeup_test_x(cmd_tbl_t *t, int argc, char *argv[])
{
    TUYA_WAKEUP_SOURCE_BASE_CFG_T cfg;

    memset(&cfg, 0, sizeof(cfg));
#if 1
    cfg.source = TUYA_WAKEUP_SOURCE_TIMER;
    cfg.wakeup_para.timer_param.mode = TUYA_TIMER_MODE_ONCE;
    cfg.wakeup_para.timer_param.timer_num = 0;
    cfg.wakeup_para.timer_param.ms = atoi(argv[1]);
#else
    cfg.source = TUYA_WAKEUP_SOURCE_GPIO;
    cfg.wakeup_para.gpio_param.gpio_num = atoi(argv[1]);
    cfg.wakeup_para.gpio_param.level = atoi(argv[2]);
#endif
    if (tkl_wakeup_source_set(&cfg) < 0) {
         os_printf(LM_APP, LL_INFO, "%s: call tkl_wakeup_source_set failed", __func__);
    }
       
    return 0;
}

CLI_CMD(wakeup_x, fhost_iot_wakeup_test_x, "wakeup test", "wakeup_x");

#endif