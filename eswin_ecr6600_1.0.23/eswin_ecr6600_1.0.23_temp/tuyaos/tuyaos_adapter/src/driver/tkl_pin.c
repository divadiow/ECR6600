 /*============================================================================
 *                                                                            *
 * Copyright (C) by Tuya Inc                                                  *
 * All rights reserved                                                        *
 *                                                                            *
 =============================================================================*/


/*============================ INCLUDES ======================================*/
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "hal_gpio.h"
#include "chip_pinmux.h"
#include "cli.h"
#include "tkl_gpio.h"

#if defined(CONFIG_PSM_SURPORT) && defined(CONFIG_TUYA_DOORCONTACT)	
extern void psm_gpio_wakeup_config(unsigned int pin, unsigned int enable);
extern void psm_set_aon_gpio(unsigned int pin , unsigned int type);
#endif

/*============================ MACROS ========================================*/
#define PIN_DEV_CHECK_ERROR_RETURN(__PIN, __ERROR)                          \
    if (__PIN >= sizeof(pinmap)/sizeof(pinmap[0])) {                        \
        return __ERROR;                                                     \
    }

typedef void (*tuya_pin_irq_cb)(void *args);

/*============================ TYPES =========================================*/
typedef struct {
    int gpio;
	tuya_pin_irq_cb cb;
    void *args;
} pin_dev_map_t;

/*============================ PROTOTYPES ====================================*/

/*============================ LOCAL VARIABLES ===============================*/
static pin_dev_map_t pinmap[] = {
    //! PA
    {GPIO_NUM_0,  NULL, NULL}, {GPIO_NUM_1,  NULL, NULL}, {GPIO_NUM_2,  NULL, NULL}, {GPIO_NUM_3,  NULL, NULL}, 
    {GPIO_NUM_4,  NULL, NULL}, {GPIO_NUM_5,  NULL, NULL}, {GPIO_NUM_6,  NULL, NULL}, {GPIO_NUM_7,  NULL, NULL}, 
    {GPIO_NUM_8,  NULL, NULL}, {GPIO_NUM_9,  NULL, NULL}, {GPIO_NUM_10, NULL, NULL}, {GPIO_NUM_11, NULL, NULL}, 
    {GPIO_NUM_12, NULL, NULL}, {GPIO_NUM_13, NULL, NULL}, {GPIO_NUM_14, NULL, NULL}, {GPIO_NUM_15, NULL, NULL}, 
    {GPIO_NUM_16, NULL, NULL}, {GPIO_NUM_17, NULL, NULL}, {GPIO_NUM_18, NULL, NULL}, {GPIO_NUM_19, NULL, NULL}, 
    {GPIO_NUM_20, NULL, NULL}, {GPIO_NUM_21, NULL, NULL}, {GPIO_NUM_22, NULL, NULL}, {GPIO_NUM_23, NULL, NULL}, 
    {GPIO_NUM_24, NULL, NULL}, {GPIO_NUM_25, NULL, NULL}
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/


void pin_func_set_gpio(int gpio_num)
{
    switch(gpio_num)
    {
        case GPIO_NUM_0:
            PIN_FUNC_SET(IO_MUX_GPIO0, FUNC_GPIO0_GPIO0);
            break;
        case GPIO_NUM_1:
            PIN_FUNC_SET(IO_MUX_GPIO1, FUNC_GPIO1_GPIO1);
            break;
        case GPIO_NUM_2:
            PIN_FUNC_SET(IO_MUX_GPIO2, FUNC_GPIO2_GPIO2);
            break;
        case GPIO_NUM_3:
            PIN_FUNC_SET(IO_MUX_GPIO3, FUNC_GPIO3_GPIO3);
            break;
        case GPIO_NUM_4:
            PIN_FUNC_SET(IO_MUX_GPIO4, FUNC_GPIO4_GPIO4);
            break;
        case GPIO_NUM_5:
            PIN_FUNC_SET(IO_MUX_GPIO5, FUNC_GPIO5_GPIO5);
            break;
        case GPIO_NUM_6:
            PIN_FUNC_SET(IO_MUX_GPIO6, FUNC_GPIO6_GPIO6);
            break;
        case GPIO_NUM_7:
            PIN_FUNC_SET(IO_MUX_GPIO7, FUNC_GPIO7_GPIO7);
            break;
        case GPIO_NUM_8:
            PIN_FUNC_SET(IO_MUX_GPIO8, FUNC_GPIO8_GPIO8);
            break;
        case GPIO_NUM_9:
            PIN_FUNC_SET(IO_MUX_GPIO9, FUNC_GPIO9_GPIO9);
            break;
        case GPIO_NUM_10:
            PIN_FUNC_SET(IO_MUX_GPIO10, FUNC_GPIO10_GPIO10);
            break;
        case GPIO_NUM_11:
            PIN_FUNC_SET(IO_MUX_GPIO11, FUNC_GPIO11_GPIO11);
            break;
        case GPIO_NUM_12:
            PIN_FUNC_SET(IO_MUX_GPIO12, FUNC_GPIO12_GPIO12);
            break;
        case GPIO_NUM_13:
            PIN_FUNC_SET(IO_MUX_GPIO13, FUNC_GPIO13_GPIO13);
            break;
        case GPIO_NUM_14:
            PIN_FUNC_SET(IO_MUX_GPIO14, FUNC_GPIO14_GPIO14);
            break;
        case GPIO_NUM_15:
            PIN_FUNC_SET(IO_MUX_GPIO15, FUNC_GPIO15_GPIO15);
            break;
        case GPIO_NUM_16:
            PIN_FUNC_SET(IO_MUX_GPIO16, FUNC_GPIO16_GPIO16);
            break;
        case GPIO_NUM_17:
            PIN_FUNC_SET(IO_MUX_GPIO17, FUNC_GPIO17_GPIO17);
            break;
        case GPIO_NUM_18:
            PIN_FUNC_SET(IO_MUX_GPIO18, FUNC_GPIO18_GPIO18);
            break;
        case GPIO_NUM_20:
            PIN_FUNC_SET(IO_MUX_GPIO20, FUNC_GPIO20_GPIO20);
            break;
        case GPIO_NUM_21:
            PIN_FUNC_SET(IO_MUX_GPIO21, FUNC_GPIO21_GPIO21);
            break;
        case GPIO_NUM_22:
            PIN_FUNC_SET(IO_MUX_GPIO22, FUNC_GPIO22_GPIO22);
            break;
        case GPIO_NUM_23:
            PIN_FUNC_SET(IO_MUX_GPIO23, FUNC_GPIO23_GPIO23);
            break;
        case GPIO_NUM_24:
            PIN_FUNC_SET(IO_MUX_GPIO24, FUNC_GPIO24_GPIO24);
            break;
        case GPIO_NUM_25:
            PIN_FUNC_SET(IO_MUX_GPIO25, FUNC_GPIO25_GPIO25);
            break;
        default:
            return;    
    }
    os_printf(LM_APP, LL_INFO, "gpio_num: %d set GPIO func OK!\r\n", gpio_num);
}


OPERATE_RET tkl_gpio_init(UINT32_T pin_id, CONST TUYA_GPIO_BASE_CFG_T *cfg)
{
    os_printf(LM_APP, LL_INFO, "enter pin_dev_init(), pin:%d \r\n", pin_id);
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_OS_ADAPTER_INVALID_PARM);
	int gpio_num = pinmap[pin_id].gpio;	

    pin_func_set_gpio(gpio_num);
	int init_status = -1;
    //! set pin init level
    switch (cfg->level) {
        case TUYA_GPIO_LEVEL_LOW:
            init_status = DRV_GPIO_LEVEL_LOW;
            break;

        case TUYA_GPIO_LEVEL_HIGH:
            init_status = DRV_GPIO_LEVEL_HIGH;
            break;
        default:
            os_printf(LM_APP, LL_INFO, "PARA PIN level err, level = %d!\r\n", cfg->level);
            break;
    }

	if (init_status != -1)
	{
		os_printf(LM_APP, LL_INFO, "pin_dev_init(), gpio_num:%d, init_status:%d\r\n", gpio_num, init_status);
		hal_gpio_write(gpio_num, init_status);
	}

    enum GPIO_DIR gpio_dir = GPIO_INPUT; 
    //! set pin direction
    switch (cfg->direct)  {
    case TUYA_GPIO_INPUT:
        gpio_dir = GPIO_INPUT;
        break;
    case TUYA_GPIO_OUTPUT:
        gpio_dir = GPIO_OUTPUT;
        break;
    default:
		os_printf(LM_APP, LL_INFO, "PARA PIN direct err!, direct = %d\r\n", cfg->direct);
        return OPRT_NOT_SUPPORTED;
    }

	os_printf(LM_APP, LL_INFO, "pin_dev_init(), gpio_num:%d, gpio_dir:%d\r\n", gpio_num, gpio_dir);
	hal_gpio_dir_set(gpio_num, gpio_dir);
	int pull_type = -1; 
	//! set pin mdoe
    switch (cfg->mode) {
        case TUYA_GPIO_PULLUP:
		    pull_type = DRV_GPIO_ARG_PULL_UP;
//        bk_gpio_cfg = bk_gpio_cfg == INPUT_NORMAL ? INPUT_PULL_UP : bk_gpio_cfg;
            break;
        case TUYA_GPIO_PULLDOWN:
		    pull_type = DRV_GPIO_ARG_PULL_DOWN;
//        bk_gpio_cfg = bk_gpio_cfg == INPUT_NORMAL ? INPUT_PULL_DOWN : bk_gpio_cfg;
            break;
        default:
            os_printf(LM_APP, LL_INFO, "PARA PIN mode err, mode = %d!\r\n", cfg->mode);
            return OPRT_NOT_SUPPORTED;
            break;
    }

	if (pull_type != -1)
	{
		os_printf(LM_APP, LL_INFO, "pin_dev_init(), gpio_num:%d, pull_type:%d\r\n", gpio_num, pull_type);
		hal_gpio_set_pull_mode(gpio_num, pull_type);
#if defined(CONFIG_PSM_SURPORT) && defined(CONFIG_TUYA_DOORCONTACT)		
		psm_set_aon_gpio(gpio_num, pull_type);
#endif		
	}
	
    return OPRT_OK;
}

OPERATE_RET tkl_gpio_write(UINT32_T pin_id, TUYA_GPIO_LEVEL_E level)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_OS_ADAPTER_INVALID_PARM);
    int ret = hal_gpio_write(pinmap[pin_id].gpio, level);
    if (ret == GPIO_RET_SUCCESS)
    {
        return OPRT_OK;
    }
    else
    {
      return OPRT_COM_ERROR;
    }
}

OPERATE_RET tkl_gpio_read(UINT32_T pin_id, TUYA_GPIO_LEVEL_E *level)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);

    *level = hal_gpio_read(pinmap[pin_id].gpio);

    return OPRT_OK;
}

/**
 * @brief gpio irq init
 * NOTE: call this API will not enable interrupt
 * 
 * @param[in] port: gpio port 
 * @param[in] cfg:  gpio irq config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_init(UINT32_T pin_id, CONST TUYA_GPIO_IRQ_T *cfg)
{
    int trigger = 0;
    OPERATE_RET ret;

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_OS_ADAPTER_INVALID_PARM);
	int gpio_num = pinmap[pin_id].gpio;	
    pin_func_set_gpio(gpio_num);
    ret = hal_gpio_dir_set(gpio_num, GPIO_INPUT);
    if (ret != GPIO_RET_SUCCESS)
    {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    switch (cfg->mode) {
        case TUYA_GPIO_IRQ_RISE:
            trigger = DRV_GPIO_ARG_INTR_MODE_P_EDGE; 
            break;
        case TUYA_GPIO_IRQ_FALL:
            trigger = DRV_GPIO_ARG_INTR_MODE_N_EDGE; 
            break;
        case TUYA_GPIO_IRQ_LOW:
            trigger = DRV_GPIO_ARG_INTR_MODE_LOW; 
            break;
        case TUYA_GPIO_IRQ_HIGH:
            trigger = DRV_GPIO_ARG_INTR_MODE_HIGH; 
            break;
        default: return OPRT_OS_ADAPTER_NOT_SUPPORTED;
    }

    pinmap[pin_id].cb = cfg->cb;
    pinmap[pin_id].args = cfg->arg;
    ret = hal_gpio_callback_register(pinmap[pin_id].gpio, pinmap[pin_id].cb, pinmap[pin_id].args);
    if (ret != GPIO_RET_SUCCESS)
    {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    ret = hal_gpio_intr_mode_set(pinmap[pin_id].gpio, trigger);
    if (ret != GPIO_RET_SUCCESS)
    {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    ret = hal_gpio_intr_enable(pinmap[pin_id].gpio, 1);
    if (ret != GPIO_RET_SUCCESS)
    {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief gpio irq enable
 * 
 * @param[in] port: gpio port 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_enable(UINT32_T pin_id)
{
    int result = hal_gpio_intr_enable(pinmap[pin_id].gpio, 1);
    OPERATE_RET ret;

    if (result== GPIO_RET_SUCCESS)
    {
        ret = OPRT_OK;
    }
    else
    {
        ret = OPRT_COM_ERROR;
    }

    return ret;
}

/**
 * @brief gpio irq disable
 * 
 * @param[in] port: gpio port 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_disable(UINT32_T pin_id)
{
    int result = hal_gpio_intr_enable(pinmap[pin_id].gpio, 0);

    OPERATE_RET ret;

    if (result== GPIO_RET_SUCCESS)
    {
        ret = OPRT_OK;
    }
    else
    {
        ret = OPRT_COM_ERROR;
    }

    return ret;
}


OPERATE_RET tkl_gpio_deinit(UINT32_T pin_id)
{
    return OPRT_NOT_SUPPORTED;
}
#if 1
//extern int tkl_gpio_init(UINT32_T pin_id, CONST TUYA_GPIO_BASE_CFG_T *cfg);
int fhost_iot_gpio_tes_x(cmd_tbl_t *t, int argc, char *argv[])
{
    int ch_id;
    unsigned char state = 0;
    TUYA_GPIO_BASE_CFG_T cfg;

    memset(&cfg, 0, sizeof(cfg));
    //cfg.pin = GPIO_NUM_20;
    cfg.mode = TUYA_GPIO_PULLUP;
    //cfg.mode = TUYA_GPIO_PULLDOWN;
    cfg.direct = TUYA_GPIO_OUTPUT;
    cfg.level = TUYA_GPIO_LEVEL_HIGH;
    //cfg.level = TUYA_GPIO_LEVEL_LOW;

    
    ch_id = atoi(argv[1]);
    state = atoi(argv[2]);
    cfg.level = state;

    os_printf(LM_APP, LL_INFO, "ch_id %d state %d\r\n", ch_id, state);
    os_printf(LM_APP, LL_INFO, "mode %d direct %d level %d\r\n", cfg.mode, cfg.direct, cfg.level);
    tkl_gpio_init(ch_id, &cfg);

    return 0;
}

CLI_CMD(gpio_x, fhost_iot_gpio_tes_x, "gpio test", "gpio_x");

int fhost_iot_cpu_sleep(cmd_tbl_t *t, int argc, char *argv[])
{
    int mode = atoi(argv[1]);
    os_printf(LM_APP, LL_INFO, "mode %d\r\n", mode);
    tkl_cpu_sleep_mode_set(1, mode);
    return 0;
}

CLI_CMD(cpu_sleep, fhost_iot_cpu_sleep, "cpu sleep", "cpu_sleep");
#endif