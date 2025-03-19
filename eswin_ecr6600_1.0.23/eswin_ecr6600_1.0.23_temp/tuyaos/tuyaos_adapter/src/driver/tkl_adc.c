/*============================================================================
*                                                                            *
* Copyright (C) by Tuya Inc                                                  *
* All rights reserved                                                        *
*                                                                            *
=============================================================================*/


/*============================ INCLUDES ======================================*/
#include "tkl_adc.h"
#include "tkl_output.h"
#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
// #include "saradc_pub.h"
// #include "drv_model_pub.h"
#include "FreeRTOS.h"
#include "task.h"
#include "gpio.h"
#include "oshal.h"
#include "hal_adc.h"

/*============================ MACROS ========================================*/
#define ADC_DEV_NUM         1
//#define ADC_BUF_SIZE        1
#define ADC_MV_TO_UV        1000
/*============================ TYPES =========================================*/
typedef struct
{
    const TUYA_ADC_NUM_E adc_num;
    const UINT32_T gpio_num;
    int adc_int_flag;
    const UINT32_T adc_gpio;
    const UINT32_T adc_ch;
}adc_pin_map_t;

/*============================ PROTOTYPES ====================================*/

/*============================ LOCAL VARIABLES ===============================*/
static UINT32_T g_adc_used_gpio_num = 0;
//static unsigned short adc_buf[ADC_BUF_SIZE];
typedef enum
{
    ADC_CH0,
    ADC_CH1,
    ADC_CH2,
    ADC_CH_MAX,
}ADC_CH_E;

static adc_pin_map_t adc_pin_map[] = {
    {TUYA_ADC_NUM_0, TUYA_GPIO_NUM_20, 0, DRV_ADC_A_INPUT_GPIO_2, ADC_CH2},
    {TUYA_ADC_NUM_0, TUYA_GPIO_NUM_14, 0, DRV_ADC_A_INPUT_GPIO_0, ADC_CH0}, 
    {TUYA_ADC_NUM_0, TUYA_GPIO_NUM_15, 0, DRV_ADC_A_INPUT_GPIO_1, ADC_CH1} 
};

const char ADC_PIN_MAP_NUM = sizeof(adc_pin_map) / sizeof(adc_pin_map[0]);

//通过GPIO查找num和ch
int adc_find_port_ch(UINT32_T adc_gpio_num, UINT32_T *adc_num, UINT32_T *adc_ch)
{
    int i = 0;
    for(i = 0; i< ADC_PIN_MAP_NUM; i++)
    {
        if (adc_gpio_num == adc_pin_map[i].gpio_num)
        {
            *adc_num = adc_pin_map[i].adc_num;
            *adc_ch = adc_pin_map[i].adc_ch;
            break;
        }
    }
    if(i >= ADC_PIN_MAP_NUM)
    {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

//初始化时查找通道接口
static int adc_find_gpio_by_channel(TUYA_ADC_NUM_E adc_num, UINT32_T adc_ch, UINT32_T *used_gpio, UINT32_T *init_flag)
{
    if ((used_gpio == NULL) || (init_flag == NULL))
    {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    int i = 0;
    for (i = 0; i < ADC_PIN_MAP_NUM; i++)
    {
        if ((adc_num == adc_pin_map[i].adc_num) && (adc_ch & BIT(adc_pin_map[i].adc_ch)))
        {
            *used_gpio = adc_pin_map[i].adc_gpio;
            adc_pin_map[i].adc_int_flag = 1;
            *init_flag = adc_pin_map[i].adc_int_flag;
            break;
        }
    }

    if(i >= ADC_PIN_MAP_NUM)
    {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}


//读数据时查找通道接口
static int adc_check_input_gpio_valid(TUYA_ADC_NUM_E port_num, UINT32_T used_gpio, UINT32_T *init_flag)
{
    int i = 0;
    for (i = 0; i < ADC_PIN_MAP_NUM; i++)
    {
        if ((port_num == adc_pin_map[i].adc_num) && (used_gpio == adc_pin_map[i].adc_gpio))
        {
            *init_flag = adc_pin_map[i].adc_int_flag;
            break;
        }
    }

    if(i >= ADC_PIN_MAP_NUM)
    {
        return OPRT_NOT_FOUND;
    }

    return OPRT_OK;
}


//deinit时查找通道接口
static int adc_find_channel_deinit(TUYA_ADC_NUM_E adc_num, UINT32_T used_gpio, UINT32_T *init_flag)
{
    if (init_flag == NULL)
    {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    int i = 0;
    for (i = 0; i < ADC_PIN_MAP_NUM; i++)
    {
        if ((adc_num == adc_pin_map[i].adc_num) && (used_gpio == adc_pin_map[i].adc_gpio))
        {
            adc_pin_map[i].adc_int_flag = 0;
            *init_flag = adc_pin_map[i].adc_int_flag;
            break;
        }
    }

    if(i >= ADC_DEV_NUM){
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/

OPERATE_RET tkl_adc_init(TUYA_ADC_NUM_E port_num, TUYA_ADC_BASE_CFG_T *cfg)
{
    int ret = 0;
    unsigned int init_flag = 0;

    ret = adc_find_gpio_by_channel(port_num, cfg->ch_list.data, &g_adc_used_gpio_num, &init_flag);
    if ((ret != OPRT_OK) || (init_flag != 1))
    {
        os_printf(LM_CMD, LL_ERR, "gpio_num: %d is't support adc! \n", port_num);
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    return OPRT_OK;
}

/**
 * @brief: ADC 适配转换，ADC检测范围0~3.3V
 * @param[in]: 
 * @return: OPRT_OK
 */
OPERATE_RET tkl_adc_read_data(TUYA_ADC_NUM_E port_num, INT32_T *buff, UINT16_T len)
{
    int ret = 0;
    unsigned int init_flag = 0;

    ret = adc_check_input_gpio_valid(port_num , g_adc_used_gpio_num, &init_flag);
    if (ret != OPRT_OK || (init_flag != 1))
    {
        os_printf(LM_CMD, LL_ERR,"adc not init! \n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    
    int volt = hal_adc_get_single((DRV_ADC_INPUT_SIGNAL_A_SEL)g_adc_used_gpio_num, DRV_ADC_B_INPUT_VREF_BUFFER, DRV_ADC_EXPECT_MAX_VOLT3);
    volt += (ADC_MV_TO_UV / 2);
    volt /=  ADC_MV_TO_UV;  //四舍五入，转化为mv
    *buff = (UINT_T)volt;
    return OPRT_OK;
}

OPERATE_RET tkl_adc_deinit(TUYA_ADC_NUM_E port_num)
{
    int ret = 0;
    unsigned int init_flag = 0;

    ret = adc_find_channel_deinit(port_num, g_adc_used_gpio_num, &init_flag);
    if ((ret != OPRT_OK) || (init_flag != 0))
    {
        os_printf(LM_CMD, LL_ERR,"adc_find_channel_deinit error \n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    g_adc_used_gpio_num = 0;
    return OPRT_OK;
}

/**
 * @brief get adc width
 * 
 * @param[in] port_num: adc unit number

 *
 * @return adc width
 */
UINT8_T tkl_adc_width_get(TUYA_ADC_NUM_E port_num)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief get adc reference voltage
 * 
 * @param[in] NULL

 *
 * @return adc reference voltage(bat: mv)
 */
UINT32_T tkl_adc_ref_voltage_get(TUYA_ADC_NUM_E port_num)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief adc get temperature
 *
 * @return temperature(bat: 'C)
 */
INT32_T tkl_adc_temperature_get(VOID_T)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief read single channel
 *
 * @param[in] port_num: adc unit number
 * @param[in] ch_num: channel number in one unit
 * @param[out] buf: convert result buffer
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 *
 */
OPERATE_RET tkl_adc_read_single_channel(TUYA_ADC_NUM_E port_num, UINT8_T ch_id, INT32_T *data)
{
    return tkl_adc_read_data(port_num, data, 1);
}

OPERATE_RET tkl_adc_read_voltage(TUYA_ADC_NUM_E port_num, INT32_T *buff, UINT16_T len)
{
    return OPRT_NOT_SUPPORTED;
}

#if 0
void cli_printf(const char *f, ...);
#include "cli.h"
//#include "pinmux.h"
static void tkl_adc_test_cb(void *arg)
{
    int ret;
    TUYA_ADC_BASE_CFG_T cfg;
    UINT_T buff;

    memset(&cfg, 0, sizeof(cfg));

    ret = tkl_io_pin_to_func(TUYA_GPIO_NUM_15, 1);
    if (ret < 0) {
        //os_printf(0, 4, "%s:%d call tkl_io_pin_to_func failed\n", __func__, __LINE__);
    }

    int adc_num = ret >> 8;
    int adc_ch = ret & 0xff;
    //os_printf(0, 4, "%s:%d ret 0x%x adc_num %d adc_ch %d\n", __func__, __LINE__, ret, adc_num, adc_ch);

    cfg.ch_list.data = BIT(adc_ch);
    ret = tkl_adc_init(adc_num, &cfg);
    if (ret < 0) {
        os_printf(LM_APP, LL_ERR,"%s: call tkl_adc_init failed\n", __func__);
        return ret;
    }

    while (1)
    {
        ret = tkl_adc_read_data(TUYA_ADC_NUM_0, &buff, 1);
        if (ret < 0) {
            os_printf(LM_APP, LL_ERR,"%s: call tkl_adc_read_data failed\n", __func__);
            return;
        }

        os_printf(LM_APP, LL_ERR, "ADC gpio %d register value = %d\n", g_adc_used_gpio_num, buff);
        tkl_system_sleep(2000);
    }
    
}

int fhost_iot_adc_test_cmd(cmd_tbl_t *t, int argc, char *argv[])
{
    int ret;

    ret = os_task_create("adc_test", 4, 1024, tkl_adc_test_cb, NULL);
    if (ret < 0) {
        os_printf(LM_APP, LL_ERR,"%s: call os_task_create failed\n", __func__);
    }
    
    return ret;
}
CLI_CMD(adc_test, fhost_iot_adc_test_cmd, "adc_test", "adc test");

#endif


