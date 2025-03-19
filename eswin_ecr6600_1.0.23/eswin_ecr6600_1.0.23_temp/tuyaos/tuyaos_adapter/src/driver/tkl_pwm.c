/*============================================================================
 *                                                                            *
 * Copyright (C) by Tuya Inc                                                  *
 * All rights reserved                                                        *
 *                                                                            *
 *                                                                            *
 =============================================================================*/

/*============================ INCLUDES ======================================*/
#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "chip_pinmux.h"
#include "tkl_pwm.h"
#include "hal_pwm.h"
#include "oshal.h"
#include "gpio.h"

/*============================ MACROS ========================================*/
#define PWM_DEV_NUM 7
#define PWM_DUTY_CYCLE_FACTOR 1000 
#define PWM_MULTI_CH_DUTY_MAX 970

//#define PWM_POLARITTY_INIT_CNT  2

/*============================ TYPES =========================================*/


/*============================ PROTOTYPES ====================================*/


    
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/
typedef struct 
{
    unsigned int gpio_num;
    unsigned int pwm_channel_num;
    unsigned int pwm_used_gpio;
    unsigned char  used;
}PWM_CFG_T;

PWM_CFG_T pwm_cfg_list[PWM_DEV_NUM] = {
    {GPIO_NUM_22, TUYA_PWM_NUM_0, PWM_CH0_USED_GPIO22, 1},
    {GPIO_NUM_23, TUYA_PWM_NUM_1, PWM_CH1_USED_GPIO23, 1},
    {GPIO_NUM_24, TUYA_PWM_NUM_2, PWM_CH2_USED_GPIO24, 1},
    {GPIO_NUM_25, TUYA_PWM_NUM_3, PWM_CH3_USED_GPIO25, 1},
    {GPIO_NUM_4,  TUYA_PWM_NUM_4, PWM_CH4_USED_GPIO4,  0},   /* AXYU, AXYU-IPEX */
    {GPIO_NUM_14, TUYA_PWM_NUM_4, PWM_CH4_USED_GPIO14, 1},   /* AXY3S, AXY2S, AXY3L */
    {GPIO_NUM_15, TUYA_PWM_NUM_5, PWM_CH5_USED_GPIO15, 1}
};

TUYA_PWM_BASE_CFG_T g_pwm_info[DRV_PWM_CHN5 + 1];
int pwm_set_gpio_used(unsigned int pin)
{
    int i;
    unsigned int ch_num = TUYA_PWM_NUM_MAX;

    if (pin > GPIO_NUM_MAX)
        return OPRT_COM_ERROR;

    for (i = 0; i < PWM_DEV_NUM; i++) {
        if (pwm_cfg_list[i].gpio_num == pin) {
            ch_num = pwm_cfg_list[i].pwm_channel_num;
            break;
        }
    }

    if (ch_num >= TUYA_PWM_NUM_MAX) {
        return OPRT_COM_ERROR;
    }

    for (i = 0; i < PWM_DEV_NUM; i++) {
        if (pwm_cfg_list[i].pwm_channel_num == ch_num) {
            pwm_cfg_list[i].used = 0;
         }
    }
    
    for (i = 0; i < PWM_DEV_NUM; i++) {
        if (pwm_cfg_list[i].gpio_num == pin) {
            pwm_cfg_list[i].used = 1;
            break;
        }
    }
    
    return OPRT_OK;
}

int pwm_find_channel(unsigned int pin, unsigned int *ch_num, unsigned int *used_gpio)
{
    if ((ch_num == NULL) || (used_gpio == NULL))
    {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    
    unsigned char i = 0;
    for(i = 0; i < PWM_DEV_NUM; i++) 
    {
        if(pwm_cfg_list[i].gpio_num == pin && pwm_cfg_list[i].used) 
        {
            *ch_num = pwm_cfg_list[i].pwm_channel_num;
            *used_gpio = pwm_cfg_list[i].pwm_used_gpio;
            break;
        }
    }

    if(i >= PWM_DEV_NUM){
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

static int pwm_ch_find_gpio(unsigned int ch_num, unsigned int *used_gpio)
{
    if (used_gpio == NULL)
    {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    
    unsigned char i = 0;
    for(i = 0; i < PWM_DEV_NUM; i++) 
    {
        if(pwm_cfg_list[i].pwm_channel_num == ch_num && pwm_cfg_list[i].used)
        {
            *used_gpio = pwm_cfg_list[i].pwm_used_gpio;
            break;
        }
    }

    if(i >= PWM_DEV_NUM){
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief pwm init
 * 
 * @param[in] port: pwm port
 * @param[in] cfg: pwm config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_init(UINT32_T ch_id, CONST TUYA_PWM_BASE_CFG_T *cfg)
{
    unsigned int used_gpio = 0;
    UINT_T used_duty = 0;
    UINT_T used_cycle = 1;

    int cfg_ret = pwm_ch_find_gpio(ch_id, &used_gpio);
    if (cfg_ret != OPRT_OK)
    {
        os_printf(LM_APP, LL_ERR, "pwm_ch_find_gpio() err, cfg->pin is %d \r\n", ch_id);
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    chip_pwm_pinmux_cfg(used_gpio);

    int ret = hal_pwm_init(ch_id);
    if (ret != PWM_RET_SUCCESS)
    {
        os_printf(LM_APP, LL_ERR, "hal_pwm_init return err(%d)! \r\n", ret);
        return OPRT_COM_ERROR;
    }
    if(cfg->cycle > 1000) {
        if(cfg->cycle < cfg->duty) {
            os_printf(LM_APP, LL_ERR, "accuracy err! \r\n");
            return OPRT_COM_ERROR;
        }
        used_cycle = cfg->cycle / 1000;
        os_printf(LM_APP, LL_INFO, "cycle only support 1000\r\n");
    }

    used_duty = cfg->duty / used_cycle;

    ret = hal_pwm_config(ch_id, cfg->frequency, used_duty);
    if (ret != PWM_RET_SUCCESS)
    {
        os_printf(LM_APP, LL_ERR, "hal_pwm_config() return err(%d)! \r\n", ret);
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    
    g_pwm_info[ch_id].duty = used_duty;
    g_pwm_info[ch_id].frequency = cfg->frequency;
    g_pwm_info[ch_id].polarity = cfg->polarity;
    return OPRT_OK;
}

OPERATE_RET tkl_pwm_start(UINT32_T ch_id)
{
    int ret = hal_pwm_start(ch_id);
    if (ret != PWM_RET_SUCCESS)
    {
        os_printf(LM_APP, LL_ERR, "hal_pwm_start() return err(%d)! \r\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

OPERATE_RET tkl_pwm_stop(UINT32_T ch_id)
{
    int ret = hal_pwm_stop(ch_id);
    if (ret != PWM_RET_SUCCESS)
    {
        os_printf(LM_APP, LL_ERR, "hal_pwm_stop() return err(%d)! \r\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

/**
 * @brief set pwm info
 * 
 * @param[in] port: pwm port
 * @param[in] info: pwm info
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_info_set(UINT32_T ch_id, CONST TUYA_PWM_BASE_CFG_T *info)
{
    UINT_T used_duty = 0;
    UINT_T used_cycle = 1;

    if(info->cycle > 1000) {
        if(info->cycle < info->duty) {
            os_printf(LM_APP, LL_ERR, "accuracy err! \r\n");
            return OPRT_COM_ERROR;
        }
        used_cycle = info->cycle / 1000;
        os_printf(LM_APP, LL_INFO, "cycle only support 1000\r\n");
    }
    used_duty = info->duty / used_cycle;

    int ret = hal_pwm_config(ch_id, info->frequency, used_duty);
    if (ret != PWM_RET_SUCCESS)
    {
        os_printf(LM_APP, LL_ERR, "hal_pwm_config() return err(%d)! \r\n", ret);
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    ret = hal_pwm_update(ch_id);
    if (ret != PWM_RET_SUCCESS)
    {
        os_printf(LM_APP, LL_ERR, "hal_pwm_config() return err(%d)! \r\n", ret);
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    g_pwm_info[ch_id].duty = used_duty;
    g_pwm_info[ch_id].frequency = info->frequency;
    g_pwm_info[ch_id].polarity = info->polarity;
    return OPRT_OK;
}

/**
 * @brief get pwm info
 * 
 * @param[in] port: pwm port
 * @param[out] info: pwm info
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_info_get(UINT32_T ch_id, TUYA_PWM_BASE_CFG_T *info)
{
    info->duty = g_pwm_info[ch_id].duty;
    info->frequency = g_pwm_info[ch_id].frequency;
    info->polarity = g_pwm_info[ch_id].polarity;
    return OPRT_OK;
}

OPERATE_RET tkl_pwm_deinit(UINT32_T ch_id)
{
    int ret = hal_pwm_close(ch_id);
    if (ret != PWM_RET_SUCCESS)
    {
        os_printf(LM_APP, LL_ERR, "hal_pwm_deinit() return err(%d)! \r\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

/**
 * @brief multiple pwm channel start
 *
 * @param[in] ch_id: pwm channal id list
 * @param[in] num  : num of pwm channal to start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_multichannel_start(TUYA_PWM_NUM_E *ch_id, UINT8_T num)
{
    int ret = OPRT_OK;
    unsigned int cold_duty, warm_duty;
    
    if (2 == num) {
        if ((ch_id[0] > PWM_DEV_NUM) || (ch_id[1] > PWM_DEV_NUM))
        {
            return OPRT_INVALID_PARM;
        }

        if (g_pwm_info[ch_id[0]].frequency != g_pwm_info[ch_id[1]].frequency) {
            return OPRT_COM_ERROR;
        }
        
        cold_duty = (unsigned int)(g_pwm_info[ch_id[0]].duty);    
        warm_duty = (unsigned int)(g_pwm_info[ch_id[1]].duty);   
        if ((cold_duty + warm_duty) > PWM_DUTY_CYCLE_FACTOR) {
            os_printf(LM_APP, LL_ERR, "cold_duty(%d) + warm_duty(%d) > max_duty(%d)\r\n", cold_duty, warm_duty, PWM_DUTY_CYCLE_FACTOR);
            return OPRT_COM_ERROR;
        }
        
        if (drv_pwm_mutex_start(ch_id[0], ch_id[1]) != PWM_RET_SUCCESS)
        {
            ret = OPRT_COM_ERROR;
        }
    } else {
        return OPRT_NOT_SUPPORTED;
    }
    return ret;
} 

/**
 * @brief multiple pwm channel stop
 *
 * @param[in] ch_id: pwm channal id list
 * @param[in] num  : num of pwm channal to stop
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_multichannel_stop(TUYA_PWM_NUM_E *ch_id, UINT8_T num)
{
    if (2 == num) {
        if ((ch_id[0] > PWM_DEV_NUM) || (ch_id[1] > PWM_DEV_NUM))
        {
            return OPRT_INVALID_PARM;
        }

        tkl_pwm_stop(ch_id[0]);
        tkl_pwm_stop(ch_id[1]);
        return OPRT_OK;
    } 

    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief pwm duty set
 * 
 * @param[in] ch_id: pwm channal id, id index starts at 0
 * @param[in] duty:  pwm duty cycle
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_duty_set(TUYA_PWM_NUM_E ch_id, UINT32_T duty)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief pwm frequency set
 * 
 * @param[in] ch_id: pwm channal id, id index starts at 0
 * @param[in] frequency: pwm frequency
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_frequency_set(TUYA_PWM_NUM_E ch_id, UINT32_T frequency)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief pwm polarity set
 * 
 * @param[in] ch_id: pwm channal id, id index starts at 0
 * @param[in] polarity: pwm polarity
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_polarity_set(TUYA_PWM_NUM_E ch_id, TUYA_PWM_POLARITY_E polarity)
{
    return OPRT_NOT_SUPPORTED;
}
#if 0
#include "cli.h"
#include "tkl_pinmux.h"
int fhost_iot_pwm_x(cmd_tbl_t *t, int argc, char *argv[])
{
    int ch_num, ret, on;
   TUYA_PWM_BASE_CFG_T cfg_info;

   cfg_info.polarity = 0;
   cfg_info.frequency = 1000000;   //ÆµÂÊ1MHZ
   cfg_info.duty = 400;          //Õ¼¿Õ±È100%
   cfg_info.cycle = 1000;
   
   ch_num = atoi(argv[1]);
   on = atoi(argv[2]);

    //tkl_io_pinmux_config(0x04, TUYA_PWM0); // for axyu
    
    os_printf(LM_APP, LL_ERR, "%s: ch_num %d on %d\r\n", __func__, ch_num, on);
    if (!on)
        cfg_info.duty = 0;

    ret = tkl_pwm_init(ch_num, &cfg_info);
    if (ret != 0) {
        os_printf(LM_APP, LL_ERR, "%s: call tkl_pwm_init failed\r\n", __func__);
        return -1;
    }
#if 0
    ret = tkl_pwm_init(ch_num + 1, &cfg_info);
    if (ret != 0) {
        os_printf(LM_APP, LL_ERR, "%s: call tkl_pwm_init failed\r\n", __func__);
        return -1;
    }
    
    int a[3] = { 0 };
    a[0] = 4;
    a[1] = 5;
    ret = tkl_pwm_multichannel_start(a, 2);
    if (ret != 0) {
        os_printf(LM_APP, LL_ERR, "%s: call tkl_pwm_multichannel_start failed\r\n", __func__);
        return -1;    
    }
#else
    ret = tkl_pwm_start(ch_num);
    if (ret != 0) {
        os_printf(LM_APP, LL_ERR, "%s: call tkl_pwm_init failed\r\n", __func__);
        return -1;
    }
#endif	
    return 0;
}
CLI_CMD(pwm_x, fhost_iot_pwm_x, "pwm testing", "pwm_x");
#endif

