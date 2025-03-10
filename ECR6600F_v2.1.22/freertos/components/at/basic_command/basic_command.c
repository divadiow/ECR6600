#include "chip_memmap.h"
#include "adc.h"
#include "at_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
//#include "util_cli_freertos.h"
#include "uart.h"
#include "psm_system.h"
#include "dce.h"
#include "dce_commands.h"
#include "hal_gpio.h"
//#include "soc_pin_mux.h"
#include "at_def.h"
#include "hal_system.h"
#include "system_wifi.h"
#include "rtc.h"
//#include "nv_config.h"
#include "easyflash.h"
#include "event_groups.h"
#include "sdk_version.h"
#include "chip_pinmux.h"
#include "reg_macro_def.h"
#include "flash.h"
#include "system_config.h"
#include "Psm_user.h"
static E_DRV_UART_NUM     uart_num = E_UART_NUM_1;

struct AT_GPIO_PIN
{
	uint8_t pin_select;
	uint8_t trigger_level;
	bool status;
};
struct AT_GPIO_PIN awake_io;

EventGroupHandle_t wakeup_method_event_group = NULL;
static const int WAKEUP_GPIO = BIT2;

static TaskHandle_t   wakeup_task_handle;
static StackType_t	  wakeup_stack[256];
static StaticTask_t   wakeup_tcb;

extern void psm_at_callback(void_fn cb);

#ifndef TEST_MODE_EXTPAD_RST_EN
#define TEST_MODE_EXTPAD_RST_EN  0x201214
#endif
#define SPIFLASH_SECTOR_SIZE			0x1000 	// 4KB

// quxin extern void TrPsmWakeupRegister(void (*callback)(void));

static uint8_t system_config_byte;

#if 0 //quxin 
struct _iomux
{
	unsigned int reg;
	unsigned int off;
	unsigned int msk;
};
bool get_iomux_cfg(unsigned int pin, struct _iomux *cfg)
{
	switch(pin)
	{
		case GPIO_NUM_0:
			cfg->reg = IO_MUX0_GPIO0_REG;
			cfg->msk = IO_MUX0_GPIO0_BITS;
		    cfg->off = IO_MUX0_GPIO0_OFFSET;
			break;
		case GPIO_NUM_1:
			cfg->reg = IO_MUX0_GPIO1_REG;
			cfg->msk = IO_MUX0_GPIO1_BITS;
		    cfg->off = IO_MUX0_GPIO1_OFFSET;
			break;
		case GPIO_NUM_2:
			cfg->reg = IO_MUX0_GPIO2_REG;
			cfg->msk = IO_MUX0_GPIO2_BITS;
		    cfg->off = IO_MUX0_GPIO2_OFFSET;
			break;
		case GPIO_NUM_3:
			cfg->reg = IO_MUX0_GPIO3_REG;
			cfg->msk = IO_MUX0_GPIO3_BITS;
		    cfg->off = IO_MUX0_GPIO3_OFFSET;
			break;
		case GPIO_NUM_4:
			cfg->reg = IO_MUX0_GPIO4_REG;
			cfg->msk = IO_MUX0_GPIO4_BITS;
		    cfg->off = IO_MUX0_GPIO4_OFFSET;
			break;
		case GPIO_NUM_5:
			cfg->reg = IO_MUX0_GPIO5_REG;
			cfg->msk = IO_MUX0_GPIO5_BITS;
		    cfg->off = IO_MUX0_GPIO5_OFFSET;
			break;
		case GPIO_NUM_6:
			cfg->reg = IO_MUX0_GPIO6_REG;
			cfg->msk = IO_MUX0_GPIO6_BITS;
		    cfg->off = IO_MUX0_GPIO6_OFFSET;
			break;		
		case GPIO_NUM_13:
			cfg->reg = IO_MUX0_GPIO13_REG;
			cfg->msk = IO_MUX0_GPIO13_BITS;
		    cfg->off = IO_MUX0_GPIO13_OFFSET;
			break;
		case GPIO_NUM_14:
			cfg->reg = IO_MUX0_GPIO14_REG;
			cfg->msk = IO_MUX0_GPIO14_BITS;
		    cfg->off = IO_MUX0_GPIO14_OFFSET;
			break;
		case GPIO_NUM_15:
			cfg->reg = IO_MUX0_GPIO15_REG;
			cfg->msk = IO_MUX0_GPIO15_BITS;
		    cfg->off = IO_MUX0_GPIO15_OFFSET;
			break;
		
		case GPIO_NUM_20:
			cfg->reg = IO_MUX0_GPIO20_REG;
			cfg->msk = IO_MUX0_GPIO20_BITS;
		    cfg->off = IO_MUX0_GPIO20_OFFSET;
			break;
		case GPIO_NUM_21:
			cfg->reg = IO_MUX0_GPIO21_REG;
			cfg->msk = IO_MUX0_GPIO21_BITS;
		    cfg->off = IO_MUX0_GPIO21_OFFSET;
			break;
		case GPIO_NUM_22:
			cfg->reg = IO_MUX0_GPIO22_REG;
			cfg->msk = IO_MUX0_GPIO22_BITS;
		    cfg->off = IO_MUX0_GPIO22_OFFSET;
			break;
#if (defined _USR_TR6260 || defined _USR_TR6260_3)
		case DRV_GPIO_7:
			cfg->reg = IO_MUX0_GPIO7_REG;
			cfg->msk = IO_MUX0_GPIO7_BITS;
		    cfg->off = IO_MUX0_GPIO7_OFFSET;
			break;
		case DRV_GPIO_8:
			cfg->reg = IO_MUX0_GPIO8_REG;
			cfg->msk = IO_MUX0_GPIO8_BITS;
		    cfg->off = IO_MUX0_GPIO8_OFFSET;
			break;
		case DRV_GPIO_9:
			cfg->reg = IO_MUX0_GPIO9_REG;
			cfg->msk = IO_MUX0_GPIO9_BITS;
		    cfg->off = IO_MUX0_GPIO9_OFFSET;
			break;
		case DRV_GPIO_10:
			cfg->reg = IO_MUX0_GPIO10_REG;
			cfg->msk = IO_MUX0_GPIO10_BITS;
		    cfg->off = IO_MUX0_GPIO10_OFFSET;
			break;
		case DRV_GPIO_11:
			cfg->reg = IO_MUX0_GPIO11_REG;
			cfg->msk = IO_MUX0_GPIO11_BITS;
		    cfg->off = IO_MUX0_GPIO11_OFFSET;
			break;
		case DRV_GPIO_12:
			cfg->reg = IO_MUX0_GPIO12_REG;
			cfg->msk = IO_MUX0_GPIO12_BITS;
		    cfg->off = IO_MUX0_GPIO12_OFFSET;
			break;
#endif

#ifdef _USR_TR6260
		case DRV_GPIO_18:
			cfg->reg = IO_MUX0_GPIO18_REG;
			cfg->msk = IO_MUX0_GPIO18_BITS;
		    cfg->off = IO_MUX0_GPIO18_OFFSET;
			break;
		case DRV_GPIO_19:
			cfg->reg = IO_MUX0_GPIO19_REG;
			cfg->msk = IO_MUX0_GPIO19_BITS;
		    cfg->off = IO_MUX0_GPIO19_OFFSET;
			break;
			
		case DRV_GPIO_23:
			cfg->reg = IO_MUX0_GPIO23_REG;
			cfg->msk = IO_MUX0_GPIO23_BITS;
		    cfg->off = IO_MUX0_GPIO23_OFFSET;
			break;
		case DRV_GPIO_24:
			cfg->reg = IO_MUX0_GPIO24_REG;
			cfg->msk = IO_MUX0_GPIO24_BITS;
		    cfg->off = IO_MUX0_GPIO24_OFFSET;
			break;
#endif
		case DRV_GPIO_16:
		case DRV_GPIO_17:
		default:
            return false;
			break;
	}

    return true;
}
#endif

struct _iomux
{
	unsigned int reg;
	unsigned int off;
	unsigned int msk;
};

void get_iomux_cfg(unsigned int pin, struct _iomux *cfg)
{
	switch(pin)
	{
		case 0:
			cfg->reg = IO_MUX_GPIO0_REG;
			cfg->msk = IO_MUX_GPIO0_BITS;
		    cfg->off = IO_MUX_GPIO0_OFFSET;
			break;

		case 1:
			cfg->reg = IO_MUX_GPIO1_REG;
			cfg->msk = IO_MUX_GPIO1_BITS;
		    cfg->off = IO_MUX_GPIO1_OFFSET;
			break;

		case 2:
			cfg->reg = IO_MUX_GPIO2_REG;
			cfg->msk = IO_MUX_GPIO2_BITS;
		    cfg->off = IO_MUX_GPIO2_OFFSET;
			break;

		case 3:
			cfg->reg = IO_MUX_GPIO3_REG;
			cfg->msk = IO_MUX_GPIO3_BITS;
		    cfg->off = IO_MUX_GPIO3_OFFSET;
			break;

		case 4:
			cfg->reg = IO_MUX_GPIO4_REG;
			cfg->msk = IO_MUX_GPIO4_BITS;
		    cfg->off = IO_MUX_GPIO4_OFFSET;
			break;

		case 5:
			cfg->reg = IO_MUX_GPIO5_REG;
			cfg->msk = IO_MUX_GPIO5_BITS;
		    cfg->off = IO_MUX_GPIO5_OFFSET;
			break;
		
		case 6:
			cfg->reg = IO_MUX_GPIO6_REG;
			cfg->msk = IO_MUX_GPIO6_BITS;
		    cfg->off = IO_MUX_GPIO6_OFFSET;
			break;

		case 7:
			cfg->reg = IO_MUX_GPIO7_REG;
			cfg->msk = IO_MUX_GPIO7_BITS;
		    cfg->off = IO_MUX_GPIO7_OFFSET;
			break;

		case 8:
			cfg->reg = IO_MUX_GPIO8_REG;
			cfg->msk = IO_MUX_GPIO8_BITS;
		    cfg->off = IO_MUX_GPIO8_OFFSET;
			break;
		
		case 9:
			cfg->reg = IO_MUX_GPIO9_REG;
			cfg->msk = IO_MUX_GPIO9_BITS;
		    cfg->off = IO_MUX_GPIO9_OFFSET;
			break;

		case 10:
			cfg->reg = IO_MUX_GPIO10_REG;
			cfg->msk = IO_MUX_GPIO10_BITS;
		    cfg->off = IO_MUX_GPIO10_OFFSET;
			break;

		case 11:
			cfg->reg = IO_MUX_GPIO11_REG;
			cfg->msk = IO_MUX_GPIO11_BITS;
		    cfg->off = IO_MUX_GPIO11_OFFSET;
			break;
		
		case 12:
			cfg->reg = IO_MUX_GPIO12_REG;
			cfg->msk = IO_MUX_GPIO12_BITS;
		    cfg->off = IO_MUX_GPIO12_OFFSET;
			break;

		case 13:
			cfg->reg = IO_MUX_GPIO13_REG;
			cfg->msk = IO_MUX_GPIO13_BITS;
		    cfg->off = IO_MUX_GPIO13_OFFSET;
			break;

		case 14:
			cfg->reg = IO_MUX_GPIO14_REG;
			cfg->msk = IO_MUX_GPIO14_BITS;
		    cfg->off = IO_MUX_GPIO14_OFFSET;
			break;

		case 15:
			cfg->reg = IO_MUX_GPIO15_REG;
			cfg->msk = IO_MUX_GPIO15_BITS;
		    cfg->off = IO_MUX_GPIO15_OFFSET;
			break;

		case 16:
			cfg->reg = IO_MUX_GPIO16_REG;
			cfg->msk = IO_MUX_GPIO16_BITS;
		    cfg->off = IO_MUX_GPIO16_OFFSET;
			break;

		case 17:
			cfg->reg = IO_MUX_GPIO17_REG;
			cfg->msk = IO_MUX_GPIO17_BITS;
		    cfg->off = IO_MUX_GPIO17_OFFSET;
			break;

		case 18:
			cfg->reg = IO_MUX_GPIO18_REG;
			cfg->msk = IO_MUX_GPIO18_BITS;
		    cfg->off = IO_MUX_GPIO18_OFFSET;
			break;
		
		case 20:
			cfg->reg = IO_MUX_GPIO20_REG;
			cfg->msk = IO_MUX_GPIO20_BITS;
		    cfg->off = IO_MUX_GPIO20_OFFSET;
			break;	

		case 21:
			cfg->reg = IO_MUX_GPIO21_REG;
			cfg->msk = IO_MUX_GPIO21_BITS;
		    cfg->off = IO_MUX_GPIO21_OFFSET;
			break;

		case 22:
			cfg->reg = IO_MUX_GPIO22_REG;
			cfg->msk = IO_MUX_GPIO22_BITS;
		    cfg->off = IO_MUX_GPIO22_OFFSET;
			break;

		case 23:
			cfg->reg = IO_MUX_GPIO23_REG;
			cfg->msk = IO_MUX_GPIO23_BITS;
		    cfg->off = IO_MUX_GPIO23_OFFSET;
			break;

		case 24:
			cfg->reg = IO_MUX_GPIO24_REG;
			cfg->msk = IO_MUX_GPIO24_BITS;
		    cfg->off = IO_MUX_GPIO24_OFFSET;
			break;

		case 25:
			cfg->reg = IO_MUX_GPIO25_REG;
			cfg->msk = IO_MUX_GPIO25_BITS;
		    cfg->off = IO_MUX_GPIO25_OFFSET;
			break;

		default:
			while(1);
	}	
}

static unsigned int set_gpio_function(unsigned int gpio_num)
{
	unsigned int reg,offset;
	unsigned int bit = 7;

	//must set first 
	if((gpio_num >= 0) && (gpio_num <= 7))
	{
		reg = CHIP_SMU_PD_PIN_MUX_0;
	}
	else if((gpio_num >= 8) && (gpio_num <= 15))
	{
		reg = CHIP_SMU_PD_PIN_MUX_1;
	}
	else if((gpio_num >= 16) && (gpio_num <= 23) && (gpio_num != 19))
	{
		reg = CHIP_SMU_PD_PIN_MUX_2;
		if(gpio_num == 18)
		{
			bit = 3;
		}
	}
	else if((gpio_num >= 24) && (gpio_num <= 25))
	{
		reg = CHIP_SMU_PD_PIN_MUX_3;
	}
	else
	{
		os_printf(LM_APP, LL_ERR, "this num = [%d] no GPIO function! \n",gpio_num);
		return CMD_RET_FAILURE;
	}
	
	offset = ((gpio_num % 8) * 4);
	PIN_MUX_SET(reg, bit,offset,1);
	return CMD_RET_SUCCESS;
}

uint8_t get_system_config(void)
{
    uint8_t value = 0;

    //quxin ef_get_env_blob(NV_SYSMSG_DEF, &value, sizeof(value), NULL);
    if (value > 0 && value <= 3) {
        system_config_byte |= value;
    }

    system_printf("system_config_byte %d\n", system_config_byte);
    return system_config_byte;
}

uint8_t set_system_config(uint8_t type, uint8_t value)
{
    if (value < 0 || value > 3) {
        return DCE_FAILED;
    }

    switch (type) {
        case COMMAND_DEF:
            //quxin if (ef_set_env_blob(NV_SYSMSG_DEF, &value, sizeof(value)) > 0)
            {
                 return DCE_FAILED;
            }
        case COMMAND_CUR:
            system_config_byte = value;
            //quxin ef_get_env_blob(NV_SYSMSG_DEF, &value, sizeof(value), NULL);
            system_printf("system_value_byte %d\n", value);
            break;
        default:
            return DCE_INVALID_INPUT;
    }

    system_printf("system_config_byte %d\n", system_config_byte);

    return DCE_OK;
}

dce_result_t dce_handle_GMR(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    // char version[20] = {0};    
    char result_str[64] = {0};

    dce_emit_information_response(dce, "AT version:" VERSION_STRING, -1);
    //quxin if (ef_get_env_blob(NV_PRODUCT_VERSIONEXT, version, sizeof(version) - 1, NULL) > 0) 
    // {
        sprintf(result_str, "SDK version:%s", sdk_version);
        dce_emit_information_response(dce, result_str, -1);

        sprintf(result_str, "Bin version:%s", RELEASE_VERSION);
        dce_emit_information_response(dce, result_str, -1);
        // sprintf(result_str, "compile time:%s %s", __DATE__, __TIME__);
        // dce_emit_information_response(dce, result_str, -1);
    // }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_CMD(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    int igrp;
    int icmd;
    char response[128];
	int basic_cmd = 0;

    for (igrp = 0; igrp < dce->command_groups_count; igrp++)
    {
        const command_group_t *group = dce->command_groups + igrp;

        snprintf(response, sizeof(response), "%s Command:", group->groupname);
        dce_emit_information_response(dce, response, -1);
        for (icmd = 0; icmd < group->commands_count; icmd++)
        {
            const command_desc_t *command = group->commands + icmd;
            snprintf(response, sizeof(response), " AT+%s", command->name);
            dce_emit_information_response(dce, response, -1);
        }
		if(basic_cmd == 0)
		{
			snprintf(response, sizeof(response), "%s\n%s\n%s"," AT"," ATE0"," ATE1");
			dce_emit_information_response(dce, response, -1);
			basic_cmd ++;
		}
    }

    return DCE_OK;
}

//quxin extern TR_PSM_CONTEXT psmCtx;
dce_result_t dce_handle_SLEEP(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
#ifdef CONFIG_PSM_SURPORT
	uint8_t sleep_mode = 0;
    if (kind & DCE_READ)
    {
       /* if(psmCtx.psmFlag == SYS_AMT_MODE)
        {
            dce_emit_extended_result_code(dce, "+SLEEP:0", -1, 1);
        }
        else if(psmCtx.psmFlag == SYS_NORMAL_MODE)
        {
            arg_t result[] = {
                    {ARG_TYPE_NUMBER, .value.number=psmCtx.sleepmode+1},
                };
            dce_emit_extended_result_code_with_args(dce, "SLEEP", -1, result, 1, 1, false);

        }*/
        if(psm_check_psm_enable())
        {
			if(PSM_SLEEP_GET(LIGHT_SLEEP_EN))
			{
				sleep_mode = 1;
			}
			else if(PSM_SLEEP_GET(MODEM_SLEEP_EN))
			{
				sleep_mode = 2;
			}
			else
			{
				sleep_mode = 0;
			}
		}
		else
		{
			sleep_mode = 0;
		}
        
        arg_t result[] = {
            {ARG_TYPE_NUMBER, .value.number=sleep_mode},
        };
        dce_emit_extended_result_code_with_args(dce, "SLEEP", -1, result, 1, 1, false);
        //dce_emit_extended_result_code(dce, "+SLEEP:0", -1, 1);
    }
    else if ((kind & DCE_WRITE) && argc == 1 && argv[0].type == ARG_TYPE_NUMBER)
    {
        if(argv[0].value.number == 0)       // disable
        {
        	psm_sleep_mode_ena_op(true, 0);
			psm_set_psm_enable(0);
			psm_pwr_mgt_ctrl(0);
			psm_sleep_mode_ena_op(true, 0);
            os_printf(LM_APP, LL_INFO, "close sleep\r\n");
        }
        else if(argv[0].value.number == 1)  // 进入lightsleep模式
        {
			psm_set_psm_enable(1);
			PSM_SLEEP_SET(LIGHT_SLEEP_EN);
			PSM_SLEEP_CLEAR(MODEM_SLEEP_EN);
            os_printf(LM_APP, LL_INFO, "set lightsleep\r\n");
        }
        else if(argv[0].value.number == 2)  // 进入modemsleep模式
        {
			psm_set_psm_enable(1);
			PSM_SLEEP_SET(MODEM_SLEEP_EN);
			PSM_SLEEP_CLEAR(LIGHT_SLEEP_EN);
            os_printf(LM_APP, LL_INFO, "set modemsleep\r\n");
        }
        else
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
#else
    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    os_printf(LM_APP, LL_INFO, "not config use psm , enable psm firstly\r\n");
    return DCE_OK;
#endif

}

static void gpio_wakeup_task(void *parameter)
{
    EventBits_t ret;

    while (1)
    {   
		ret = xEventGroupWaitBits(wakeup_method_event_group, WAKEUP_GPIO , pdTRUE, pdFALSE, portMAX_DELAY);
		if((ret & WAKEUP_GPIO ) == 0)
		{
			continue;
		}
		
        // system_printf("---------------------------------- get gpio wakeup event \r\n");
		psm_set_device_status(PSM_DEVICE_GPIO, PSM_DEVICE_STATUS_ACTIVE);

        vTaskDelay(pdMS_TO_TICKS(1000));

		psm_set_device_status(PSM_DEVICE_GPIO, PSM_DEVICE_STATUS_IDLE);
    }
}

static void gpio_wakeup_task_init(void)
{
    wakeup_task_handle = xTaskCreateStatic(gpio_wakeup_task,"gpio wake task",256,NULL,5,&wakeup_stack[0],&wakeup_tcb);
}

#if 1 
static void wakeup_callback(void)
{
#if 0 // quxin 
    struct _iomux cfg = {0,0,0};

    get_iomux_cfg(pin_select, &cfg);
    PIN_MUX_SET(cfg.reg, cfg.msk, cfg.off, 1);
#endif
	if(awake_io.status != true)return;
	
    drv_gpio_write(awake_io.pin_select, awake_io.trigger_level);
	os_printf(LM_APP, LL_INFO, "%s %d ##io num=%d gpio set %s \n", __func__,__LINE__,awake_io.pin_select,awake_io.trigger_level ? "high" : "low");
    xEventGroupSetBits(wakeup_method_event_group, WAKEUP_GPIO);
}
#endif

static void set_pin_mode(int pin, int level)
{
	if(pin > GPIO_NUM_MAX || level > 2)return;
	awake_io.pin_select = pin;
	awake_io.trigger_level = level;
	//WRITE_REG(AON_PAD_MODE_REG, READ_REG(AON_PAD_MODE_REG) | (1 << awake_io.pin_select));
	BIT_SET(AON_PAD_MODE_REG, awake_io.pin_select);
	set_gpio_function(awake_io.pin_select);
    drv_gpio_ioctrl(awake_io.pin_select, DRV_GPIO_CTRL_PULL_TYPE, DRV_GPIO_ARG_PULL_UP);
    drv_gpio_ioctrl(awake_io.pin_select, DRV_GPIO_CTRL_SET_DIRCTION, DRV_GPIO_ARG_DIR_OUT);
	drv_gpio_write(awake_io.pin_select, awake_io.trigger_level ? 0x0 : 0x1);
	awake_io.status = true;
}

static void clear_pin_mode()
{
	//WRITE_REG(AON_PAD_MODE_REG, READ_REG(AON_PAD_MODE_REG) & (~(1 << awake_io.pin_select)));
	BIT_CLR(AON_PAD_MODE_REG, awake_io.pin_select);
	awake_io.pin_select = 0xff;
	awake_io.trigger_level = 0xff;
	awake_io.status = false;
}

dce_result_t dce_handle_WAKEUPGPIO(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if ((kind & DCE_WRITE) && argc == 1 && argv[0].type == ARG_TYPE_NUMBER && argv[0].value.number == 0)
    {
	    //quxin OUT32(SOC_PCU_CTRL10, IN32(SOC_PCU_CTRL10) & ~(0x1 << 2)); // 关闭上升沿唤醒
        //quxin OUT32(SOC_PCU_CTRL10, IN32(SOC_PCU_CTRL10) & ~(0x1 << 3)); // 关闭下升沿唤醒
        psm_wake_clear_trigger_level();
		clear_pin_mode();
    }
    else if ((kind & DCE_WRITE) && 
        argc == 3 && 
        argv[0].type == ARG_TYPE_NUMBER && 
        argv[1].type == ARG_TYPE_NUMBER && 
        argv[2].type == ARG_TYPE_NUMBER &&
        (argv[0].value.number >= 0 && argv[0].value.number <= 1) &&                 // 使能
        (argv[1].value.number >= 0 && argv[1].value.number <= GPIO_NUM_MAX) &&      // GPIO
        (argv[2].value.number >= 0 && argv[2].value.number <= 3) )                  // 触发方式
    {
    	clear_pin_mode();
        if(argv[0].value.number == 0)
        {
            //quxin OUT32(SOC_PCU_CTRL10, IN32(SOC_PCU_CTRL10) & ~(0x1 << 2)); // 关闭上升沿唤醒
            //quxin OUT32(SOC_PCU_CTRL10, IN32(SOC_PCU_CTRL10) & ~(0x1 << 3)); // 关闭下升沿唤醒
			psm_wake_clear_trigger_level();
			
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            return DCE_OK;
        }
		else
		{
			psm_gpio_wakeup_config(argv[1].value.number, 1);
			psm_wake_trigger_level(argv[2].value.number);
		}
    }
    else if ((kind & DCE_WRITE) && 
        argc == 5 && 
        argv[0].type == ARG_TYPE_NUMBER && 
        argv[1].type == ARG_TYPE_NUMBER && 
        argv[2].type == ARG_TYPE_NUMBER &&
        argv[3].type == ARG_TYPE_NUMBER && 
        argv[4].type == ARG_TYPE_NUMBER &&
        (argv[0].value.number >= 0 && argv[0].value.number <= 1) &&                 // 使能
        (argv[1].value.number >= 0 && argv[1].value.number <= GPIO_NUM_MAX) &&      // GPIO
        (argv[2].value.number >= 0 && argv[2].value.number <= 3) &&                 // 触发方式
        (argv[3].value.number >= 0 && argv[3].value.number <= GPIO_NUM_MAX) &&      // 唤醒后的GPIO
        (argv[4].value.number >= 0 && argv[4].value.number <= 1)                    // 输出高低电平
        )
    {
        if(argv[0].value.number == 0)
        {
            //quxin OUT32(SOC_PCU_CTRL10, IN32(SOC_PCU_CTRL10) & ~(0x1 << 2)); // 关闭上升沿唤醒
            //quxin OUT32(SOC_PCU_CTRL10, IN32(SOC_PCU_CTRL10) & ~(0x1 << 3)); // 关闭下升沿唤醒
			psm_wake_clear_trigger_level();
			clear_pin_mode();
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            return DCE_OK;
        }
        else
        {
            //quxin gpio_wakeup_config(argv[1].value.number, DRV_GPIO_WAKEUP_EN);
            set_pin_mode(argv[3].value.number, argv[4].value.number);
			psm_gpio_wakeup_config(argv[1].value.number, 1);
			psm_wake_trigger_level(argv[2].value.number);

        }
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }
    
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_GSLP(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
#ifdef CONFIG_PSM_SURPORT

	wifi_work_mode_e mode = wifi_get_opmode();
	if (mode != WIFI_MODE_STA && mode != WIFI_MODE_AP_STA)
	{
		os_printf(LM_APP, LL_INFO, "not in sta or sta+ap mode\n");
		dce_emit_basic_result_code(dce, DCE_RC_ERROR);
		return DCE_RC_OK;
	}

    if ((kind & DCE_WRITE) && argc == 1 && argv[0].type == ARG_TYPE_NUMBER && argv[0].value.number > 0)
    {
    	unsigned int time = argv[0].value.number * 33 ;
		drv_rtc_set_alarm_relative(time);

        dce_emit_basic_result_code(dce, DCE_RC_OK);
        psm_enter_sleep(DEEP_SLEEP);
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
#else
    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    os_printf(LM_APP, LL_INFO, "not config use psm , enable psm firstly\r\n");
    return DCE_OK;
#endif
}

dce_result_t dce_handle_RST(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
   // cli_printf("RST RST RST \n");
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);
    return DCE_OK;
}

dce_result_t dce_handle_RESTORE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
	unsigned int env_customer_addr,env_customer_len;
	partion_info_get(PARTION_NAME_NV_CUSTOMER, &env_customer_addr, &env_customer_len);
	drv_spiflash_erase(env_customer_addr, env_customer_len);
    dce_emit_basic_result_code(dce, DCE_RC_OK);
	hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);
    return DCE_OK;
}


typedef enum 
{
    BAUDRATE_ERROR = 0,
    DATABIT_ERROR ,
    STOPBITS_ERROR,
    PARITY_ERROR,
    FLOW_CONTROL_ERROR,

    UART_OK,
}uart_error_number;

static uart_error_number uart_param_check(arg_t* argv)
{
    // 110 14400 76800 460800 921600
    const int valid_baudrates[] = {160, 300, 600, 1200, 2400, 4800, 9600, 
                                          14400, 19200, 38400, 43000, 57600, 76800, 
                                          115200, 128000, 230400, 256000, 380400, 460800, 
                                          921600, 1000000, 2000000};
    const int valid_baudrates_count = sizeof(valid_baudrates)/sizeof(int);
    const int valid_databits[] = {8};
    const int valid_databits_count = sizeof(valid_databits)/sizeof(int);
    const int valid_stopbits[] = {1, 3};
    const int valid_stopbits_count = sizeof(valid_stopbits)/sizeof(int);
    const int valid_parity[] = {0, 1, 2};
    const int valid_parity_count = sizeof(valid_parity)/sizeof(int);
    const int valid_flow_control[] = {0, 1};
    const int valid_flow_control_count = sizeof(valid_flow_control)/sizeof(int);
    int i = 0;
	if (921600 == argv[0].value.number)
	{
		// quxin if (REG32(CLK_MUX_BASE) != 0x15)
		// quxin {
		// quxin 	 return BAUDRATE_ERROR;
		// quxin }
	}
    for (i = 0; i < valid_baudrates_count; ++i)
    {
        if (argv[0].value.number == valid_baudrates[i])
            break;
    }
    if(i == valid_baudrates_count) {
        return BAUDRATE_ERROR;
    }
    for (i = 0; i < valid_databits_count; ++i)
    {
        if (argv[1].value.number == valid_databits[i])
            break;
    }
    if(i == valid_databits_count) {
        return DATABIT_ERROR;
    }
    for (i = 0; i < valid_stopbits_count; ++i)
    {   
        //judge it by datasheet
        if (argv[2].value.number == valid_stopbits[i]){
            break;
        }
            
    }
    if(i == valid_stopbits_count) {
        return STOPBITS_ERROR;
    }
    for (i = 0; i < valid_parity_count; ++i)
    {
        if (argv[3].value.number == valid_parity[i])
            break;
    }
    if(i == valid_parity_count) {
        return PARITY_ERROR;
    }
    for (i = 0; i < valid_flow_control_count; ++i)
    {
        if (argv[4].value.number == valid_flow_control[i])
            break;
    }
    if(i == valid_flow_control_count) {
        return FLOW_CONTROL_ERROR;
    }

    return UART_OK;
}

#define uart_alias_databits(databits) (databits-5)

static int uart_alias_stopbit(int databits, int stopbits)
{   
    //if stop bit is 1.5,databits must be 5 bit
    if(stopbits == 2 && databits != 5){
        return -1;
    }
    //if stop bit is 2,databits must not be 5 bit
    if(stopbits == 3 && databits == 5){
        return -1;
    }
    if(stopbits == 1){
        return 0;
    } 
    return 1;

}

dce_result_t dce_handle_UART(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    char handle[] = "UART_CUR";
    T_DRV_UART_CONFIG config,config_def;
    bool is_flash = false;
    u32_t uart_base_reg;

#ifdef CONFIG_AT_UART_0
    uart_base_reg = MEM_BASE_UART0;
    uart_num = E_UART_NUM_0;
#endif
#ifdef CONFIG_AT_UART_1
    uart_base_reg = MEM_BASE_UART1;
    uart_num = E_UART_NUM_1;
#endif
#ifdef CONFIG_AT_UART_2
    uart_base_reg = MEM_BASE_UART2;
    uart_num = E_UART_NUM_2;
#endif    
    if(DCE_RC_OK != dce_handle_flashEn_arg(kind, &argc, argv, &is_flash))  
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_RC_ERROR;
    }
    if (kind & DCE_READ)
    {
        drv_uart_get_config(uart_base_reg, &config);
        arg_t result_cur[] = {
                            {ARG_TYPE_NUMBER, .value.number=config.uart_baud_rate},
                            {ARG_TYPE_NUMBER, .value.number=config.uart_data_bits },
                            {ARG_TYPE_NUMBER, .value.number=config.uart_stop_bits},
                            {ARG_TYPE_NUMBER, .value.number=config.uart_parity_bit},
                            {ARG_TYPE_NUMBER, .value.number=config.uart_flow_ctrl}
                        };
        dce_emit_extended_result_code_with_args(dce, handle, -1, result_cur, 5, 1, false);
        memcpy(handle,"UART_DEF",sizeof("UART_DEF"));
        hal_system_get_config(CUSTOMER_NV_UART_CONFIG, &(config_def), sizeof(config_def));
        if((config_def.uart_stop_bits == 0) || (config_def.uart_stop_bits == 1))
        {
            config_def.uart_stop_bits = 1;
        }
        else
        {
            config_def.uart_stop_bits = 3;
        }

        arg_t result_def[] = {
                            {ARG_TYPE_NUMBER, .value.number=config_def.uart_baud_rate},
                            {ARG_TYPE_NUMBER, .value.number=(config_def.uart_data_bits + 5)},
                            {ARG_TYPE_NUMBER, .value.number=config_def.uart_stop_bits},
                            {ARG_TYPE_NUMBER, .value.number=config_def.uart_parity_bit},
                            {ARG_TYPE_NUMBER, .value.number=config_def.uart_flow_ctrl}
                        };
        dce_emit_extended_result_code_with_args(dce, handle, -1, result_def, 5, 1, false);
    }
    else if((kind & DCE_WRITE))
    {
        if (argc == 5 && 
            argv[0].type == ARG_TYPE_NUMBER &&
            argv[1].type == ARG_TYPE_NUMBER &&
            argv[2].type == ARG_TYPE_NUMBER &&
            argv[3].type == ARG_TYPE_NUMBER &&
            argv[4].type == ARG_TYPE_NUMBER)
        {
            config.uart_baud_rate  = argv[0].value.number;
            config.uart_data_bits  = argv[1].value.number;
            config.uart_stop_bits  = argv[2].value.number;
            config.uart_parity_bit = argv[3].value.number;
            config.uart_flow_ctrl  = argv[4].value.number; //todo

            if(UART_OK != uart_param_check(argv))
            {
                os_printf(LM_APP, LL_INFO, "unsupported parameter\n");
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_OK;
            } 

            if((uart_num == 1) && (config.uart_flow_ctrl == 1))
            {
                chip_uart1_pinmux_cfg(1); 
            }
            if((uart_num == 0) && (config.uart_flow_ctrl == 1))
            {
                chip_uart0_pinmux_cfg(UART0_RX_USED_GPIO5, UART0_TX_USED_GPIO6, 1);
            }

            int reg_stop_bit = uart_alias_stopbit(config.uart_data_bits,config.uart_stop_bits);
            if(reg_stop_bit == -1)
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_OK;
            }
            drv_uart_set_lineControl(uart_base_reg, uart_alias_databits(config.uart_data_bits),config.uart_parity_bit, reg_stop_bit,0);
            drv_uart_set_baudrate(uart_base_reg,  config.uart_baud_rate);
            drv_uart_set_flowcontrol(uart_base_reg,config.uart_flow_ctrl);

            if(is_flash == true)
            {
                config.uart_tx_mode     = UART_TX_MODE_STREAM;
                config.uart_rx_mode     = UART_RX_MODE_USER;
                config.uart_data_bits   = config.uart_data_bits - 5;
                hal_system_set_config(CUSTOMER_NV_UART_CONFIG, &config, sizeof(config));
            }      
        } 
        else 
        {
            os_printf(LM_APP, LL_INFO, "invalid arguments\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        } 
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_SYSRAM(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    char line[128] = {0};

    if (kind == DCE_READ)
    {
        sprintf(line, "+SYSRAM:%ld,%ld", xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
        dce_emit_extended_result_code(dce, line, -1, 1);
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }
     
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_SYSADC(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

#define GPIO_GET_MODE(reg, offset, mask) ((READ_REG(reg)>>offset)&mask)

void MqttReadLed(void){
	struct _iomux cfg = {0,0,0};
	get_iomux_cfg(GPIO_NUM_3, &cfg);
	PIN_MUX_SET(cfg.reg, cfg.msk, cfg.off, 1);
	drv_gpio_ioctrl(GPIO_NUM_3, DRV_GPIO_CTRL_PULL_TYPE, DRV_GPIO_ARG_PULL_DOWN);
	hal_gpio_dir_set(GPIO_NUM_3, 1);
	hal_gpio_write(GPIO_NUM_3, 0);

	struct _iomux cfg_4 = {0,0,0};
	get_iomux_cfg(GPIO_NUM_4, &cfg_4);
	PIN_MUX_SET(cfg_4.reg, cfg_4.msk, cfg_4.off, 1);
	drv_gpio_ioctrl(GPIO_NUM_4, DRV_GPIO_CTRL_PULL_TYPE, DRV_GPIO_ARG_PULL_DOWN);
	hal_gpio_dir_set(GPIO_NUM_4, 1);
	hal_gpio_write(GPIO_NUM_4, 0);
	
}

dce_result_t dce_handle_SYSIOSETCFG(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (argc == 3 && 
        argv[0].type == ARG_TYPE_NUMBER &&
        argv[1].type == ARG_TYPE_NUMBER &&
        argv[2].type == ARG_TYPE_NUMBER)
    {
        unsigned int pin  = argv[0].value.number;
        unsigned int mode = argv[1].value.number;
		unsigned int pull = argv[2].value.number;

		if((pin > GPIO_NUM_MAX) || (pin == GPIO_NUM_19 ) || (mode > 7) || (pull >= DRV_GPIO_PULL_TYPE_MAX))
		{
	        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
	        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
		}
		
		struct _iomux cfg = {0,0,0};
		get_iomux_cfg(pin, &cfg);
		PIN_MUX_SET(cfg.reg, cfg.msk, cfg.off, mode);

		// quxin config.GPIO_PullEn   = (pull ? (DRV_GPIO_PULL_ENABLE)1 : (DRV_GPIO_PULL_ENABLE)0);
		if(pull == 1)
		{
    	    drv_gpio_ioctrl(pin, DRV_GPIO_CTRL_PULL_TYPE, DRV_GPIO_ARG_PULL_UP);
		}
		else
		{
			drv_gpio_ioctrl(pin, DRV_GPIO_CTRL_PULL_TYPE, DRV_GPIO_ARG_PULL_DOWN);
		}

	}
	else
	{
        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
	}

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}


dce_result_t dce_handle_SYSIOGETCFG(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)
    {
    	arg_t result[3];
        unsigned int pin  = argv[0].value.number;

		if((pin > GPIO_NUM_MAX ) || (pin == GPIO_NUM_19 ))
		{
	        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
	        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
		}

#if 0 //quxin 
		struct _iomux cfg = {0,0,0};
		if(!get_iomux_cfg(pin, &cfg))
        {
            os_printf(LM_APP, LL_INFO, "unsupported gpio\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
        }
#endif		
		struct _iomux cfg = {0,0,0};
		get_iomux_cfg(pin, &cfg);
		
		result[0].type = ARG_TYPE_NUMBER;
		result[1].type = ARG_TYPE_NUMBER;
		result[2].type = ARG_TYPE_NUMBER;
		result[0].value.number = pin;
		result[1].value.number = GPIO_GET_MODE(cfg.reg,cfg.off,cfg.msk);

		result[2].value.number = drv_gpio_pull_read(pin);

		dce_emit_extended_result_code_with_args(dce, "SYSIOGETCFG", -1, result, 3, 1, false);

	}
	else
	{
        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
	}
	
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_SYSGPIODIR(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (argc == 2 && argv[0].type == ARG_TYPE_NUMBER && argv[1].type == ARG_TYPE_NUMBER)
    {
        unsigned int pin  = argv[0].value.number;
        unsigned int dir  = argv[1].value.number;

		if((pin > GPIO_NUM_MAX)||(dir >= GPIO_DIR_MAX))
		{
	        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
	        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
		}

#if 0 //quxin 
		struct _iomux cfg = {0,0,0};
		if(!get_iomux_cfg(pin, &cfg))
        {
            os_printf(LM_APP, LL_INFO, "unsupported gpio\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
        }
		if(GPIO_GET_MODE(cfg.reg,cfg.off,cfg.msk) != 1)	
		{
	        dce_emit_information_response(dce, "NOT GPIO MODE!", -1);
	        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
		}
#endif
		if(pin == 16)
		{
			PIN_MUX_SET_REG(TEST_MODE_EXTPAD_RST_EN,PIN_MUX_GET_REG(TEST_MODE_EXTPAD_RST_EN)&~TEST_MODE_ENABLE);	
		}
		if(pin == 18)
		{
			PIN_MUX_SET_REG(TEST_MODE_EXTPAD_RST_EN,PIN_MUX_GET_REG(TEST_MODE_EXTPAD_RST_EN)&~EXTPAD_RST_ENABLE);
		}
		hal_gpio_dir_set(pin, dir);
	}
	else
	{
        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
	}
	
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_SYSGPIOWRITE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (argc == 2 && argv[0].type == ARG_TYPE_NUMBER && argv[1].type == ARG_TYPE_NUMBER)
    {
        uint32_t pin  = argv[0].value.number;
        unsigned int value  = argv[1].value.number;

		if((pin > GPIO_NUM_MAX)||(value >= LEVEL_MAX))
		{
	        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
	        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
		}
		
		uint32_t dir = hal_gpio_dir_get(pin);
		
		if((dir != GPIO_OUTPUT))
		{
	        dce_emit_information_response(dce, "NOT OUTPUT!", -1);
	        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
		}
		
		hal_gpio_write(pin, value);
	}
	else
	{
        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
	}

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_SYSGPIOREAD(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (argc == 1 && argv[0].type == ARG_TYPE_NUMBER)
    {
        uint32_t pin  = argv[0].value.number;

		if(pin > GPIO_NUM_MAX)
		{
	        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
	        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
		}

#if 0 //quxin 
		struct _iomux cfg = {0,0,0};
		if(!get_iomux_cfg(pin, &cfg))
        {
            os_printf(LM_APP, LL_INFO, "unsupported gpio\n");
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
        }
		
		if(GPIO_GET_MODE(cfg.reg,cfg.off,cfg.msk) != 1)
		{
			dce_emit_information_response(dce, "NOT GPIO MODE!", -1);
	        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
	        return DCE_OK;
		}
#endif
		uint32_t dir,value;
        dir   = hal_gpio_dir_get(pin);
        value = hal_gpio_read(pin);

		arg_t result[3];		
		result[0].type = ARG_TYPE_NUMBER;
		result[1].type = ARG_TYPE_NUMBER;
		result[2].type = ARG_TYPE_NUMBER;
		result[0].value.number = pin;
		result[1].value.number = dir;
		result[2].value.number = value;

		dce_emit_extended_result_code_with_args(dce, "SYSGPIOREAD", -1, result, 3, 1, false);
	}
	else
	{
        os_printf(LM_APP, LL_INFO, "invalid arguments\n");
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
	}
	
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_SYSMSG_CUR(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (argc == 1 && argv[0].type == ARG_TYPE_NUMBER) {
        if(argv[0].value.number > 3 || argv[0].value.number < 0){
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        set_system_config(COMMAND_CUR, argv[0].value.number);
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

dce_result_t dce_handle_SYSMSG_DEF(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if (argc == 1 && argv[0].type == ARG_TYPE_NUMBER) {
        if(argv[0].value.number > 3 || argv[0].value.number < 0){
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        set_system_config(COMMAND_DEF, argv[0].value.number);
    } else {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    }

    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

struct flash {
    unsigned int addr;
    unsigned int len;
    unsigned char buff[4];
    unsigned int offset;
    unsigned int writeoff;
} flash_op_info;

void dce_handle_flash_data(void *addr, const char *data, size_t len)
{
    unsigned char value;
    char *next;
    dce_t *dce = addr;

    if (flash_op_info.offset < 4)
    {
        flash_op_info.buff[flash_op_info.offset] = *data;
        if ((flash_op_info.buff[flash_op_info.offset] == ' ') || ((flash_op_info.len == 1) && (flash_op_info.offset == 2)))
        {
            if (flash_op_info.offset == 0)
                return;
            if ((flash_op_info.offset != 2) || !isxdigit(flash_op_info.buff[0]) || !isxdigit(flash_op_info.buff[1]))
            {
                flash_op_info.offset = 4;
                return;
            }
            flash_op_info.buff[flash_op_info.offset] = '\0';
            value = strtol((const char *)flash_op_info.buff, &next,16);
            drv_spiflash_write(flash_op_info.addr + flash_op_info.writeoff, &value, sizeof(value));
            flash_op_info.writeoff += 1;
            flash_op_info.offset = 0;
            flash_op_info.len--;

            if (flash_op_info.len == 0)
            {
                dce_register_data_input_cb(NULL);
                target_dec_switch_input_state(COMMAND_STATE);
                memset(&flash_op_info, 0, sizeof(flash_op_info));

                memcpy(dce->command_line_buf, "ATT", strlen("ATT"));
                dce->command_line_length = 3;
                target_dce_request_process_command_line(dce);
                return;
            }
            return;
        }
        flash_op_info.offset++;
        return;
    }
    else
    {
        if (*data == '\r')
        {
            memset(&flash_op_info, 0, sizeof(flash_op_info));
            dce_register_data_input_cb(NULL);
            target_dec_switch_input_state(COMMAND_STATE);
            memcpy(dce->command_line_buf, "ATTT", strlen("ATTT"));
            dce->command_line_length = 4;
            target_dce_request_process_command_line(dce);
            return;
        }
    }
}

dce_result_t dce_handle_SYSFLASH(dce_t *dce, void *group_ctx, int kind, size_t argc, arg_t *argv)
{
    unsigned char data[128];
    char buff[512];
    int index;
    int offset = 0;
    unsigned int readlen = 0;
	unsigned int start_addr,end_addr;
	unsigned int ca_crt_addr,ca_crt_len,nv_customer_addr,nv_customer_len;
    flash_op_info.addr = 0;
    flash_op_info.len = 0;

    if (kind == DCE_READ)
    {
        memset(buff, 0, sizeof(buff));
        flash_op_info.len = snprintf(buff, sizeof(buff), "%s", "+SYSFLASH:\n");
        partion_get_string_part(buff + flash_op_info.len, sizeof(buff) - flash_op_info.len);
        dce_emit_extended_result_code(dce, buff, -1, 1);
    }

    if (argc == 4 && argv[0].type == ARG_TYPE_NUMBER && argv[1].type == ARG_TYPE_STRING &&
        argv[2].type == ARG_TYPE_NUMBER && argv[3].type == ARG_TYPE_NUMBER)
    {
        if (partion_info_get((char *)(argv[1].value.string), &flash_op_info.addr, &flash_op_info.len) != 0)
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
        if ((partion_info_get(PARTION_NAME_CA_CRT, &ca_crt_addr, &ca_crt_len) != 0) || (partion_info_get(PARTION_NAME_NV_CUSTOMER, &nv_customer_addr, &nv_customer_len)!=0))
        {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
        }
		start_addr = ca_crt_addr;
		end_addr   = nv_customer_addr + nv_customer_len;

       if ((flash_op_info.len == 0) || (argv[3].value.number > flash_op_info.len) || ((argv[2].value.number + argv[3].value.number) > flash_op_info.len)  || \
	   	    (argv[0].value.number != 2 && (flash_op_info.addr < start_addr || flash_op_info.addr >= end_addr)))
       {
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_OK;
       }
	   
	   flash_op_info.addr += argv[2].value.number;
	   flash_op_info.len   = argv[3].value.number;

       if (argv[0].value.number == 0)
       /* flash erase */
       {
            if ((flash_op_info.addr % SPIFLASH_SECTOR_SIZE) || ((flash_op_info.len) % SPIFLASH_SECTOR_SIZE))
            {
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_OK;
            }
            drv_spiflash_erase(flash_op_info.addr, flash_op_info.len);
       }
	   
       if(argv[0].value.number == 1)
       /* flash write */
       {
            memset(buff, 0, sizeof(buff));
            readlen = snprintf(buff, sizeof(buff), "%s", ">");
            dce_emit_extended_result_code(dce, buff, readlen, 1);
            target_dec_switch_input_state(ONLINE_DATA_STATE);
            dce_register_data_input_cb(dce_handle_flash_data);
            return DCE_OK;
       }

       if (argv[0].value.number == 2)
       /* flash read */
       {
            memset(data, 0, sizeof(data));
            while (flash_op_info.len)
            {
                readlen = (flash_op_info.len > sizeof(data)) ? sizeof(data) : flash_op_info.len;
                drv_spiflash_read(flash_op_info.addr, data, readlen);
                for (index = 0; index < readlen; index++)
                {
                    offset += snprintf(buff + offset, sizeof(buff), "%02x ", data[index]);
                    if (!((index + 1) % 16))
                        offset += snprintf(buff + offset, sizeof(buff), "%s", "\n");
                }
                dce_emit_extended_result_code(dce, buff, -1, 1);
                offset = 0;
                flash_op_info.addr += readlen;
                flash_op_info.len -= readlen;
            }
       }
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

extern uint8_t psm_usr_sleep_mode_status;
dce_result_t dce_handle_SYS_LIGHTSEELP(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
	uint16_t time  = argv[0].value.number;
	target_dce_transmit("[DA]SLEEP:1\r\n", strlen("[DA]SLEEP:1\r\n"));
	os_printf(LM_APP, LL_INFO, "dce_handle_SYS_SEELP(%d)=%s\r\n",time,argv[0].value.string);
	psm_set_sleep_mode(LIGHT_SLEEP,7);
	psm_usr_sleep_mode_status = 1;
	return DCE_OK;
}

dce_result_t dce_handle_SYS_VER(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
	char line[32] = {0};
    if (kind == DCE_READ)
    {
        sprintf(line, "VER:W0327.1.2");
        dce_emit_extended_result_code(dce, line, -1, 1);
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
	return DCE_OK;
}

dce_result_t dce_handle_SYSTEMP(dce_t *dce, void *group_ctx, int kind, size_t argc, arg_t *argv)
{
    char line[32] = {0};

    if (kind == DCE_READ)
    {
        sprintf(line, "+SYSTEMP:%d", get_current_temprature());
        dce_emit_extended_result_code(dce, line, -1, 1);
    }
    else
    {
        dce_emit_basic_result_code(dce, DCE_RC_ERROR);
        return DCE_OK;
    }
    dce_emit_basic_result_code(dce, DCE_RC_OK);
    return DCE_OK;
}

static const command_desc_t basic_commands[] = {
    {"SYSRAM",       &dce_handle_SYSRAM,       DCE_READ},
    {"VER",    	 &dce_handle_SYS_VER,       DCE_READ},
    {"SYSIOSETCFG",  &dce_handle_SYSIOSETCFG,  DCE_WRITE},
    {"SYSIOGETCFG",  &dce_handle_SYSIOGETCFG,  DCE_READ},
    {"SYSGPIODIR",   &dce_handle_SYSGPIODIR,   DCE_WRITE},
    {"SYSGPIOWRITE", &dce_handle_SYSGPIOWRITE, DCE_WRITE},
    {"SYSGPIOREAD",  &dce_handle_SYSGPIOREAD,  DCE_READ},
    {"SYS_SLEEP",  	 &dce_handle_SYS_LIGHTSEELP,  DCE_WRITE},
//    {"SYSMSG_CUR",   &dce_handle_SYSMSG_CUR,   DCE_READ},
//    {"SYSMSG_DEF",   &dce_handle_SYSMSG_DEF,   DCE_READ},
    {"SYSTEMP",      &dce_handle_SYSTEMP,      DCE_READ},
    {"SYSFLASH",     &dce_handle_SYSFLASH,     DCE_READ | DCE_WRITE },
    {"UART",         &dce_handle_UART,         DCE_READ | DCE_WRITE },
    {"RST",          &dce_handle_RST,          DCE_EXEC },
    {"RESTORE",      &dce_handle_RESTORE,      DCE_EXEC },
    {"GMR",          &dce_handle_GMR,          DCE_EXEC },
//    {"GSLP",         &dce_handle_GSLP,         DCE_WRITE },
//    {"SLEEP",        &dce_handle_SLEEP,        DCE_READ | DCE_WRITE },
//    {"WAKEUPGPIO",   &dce_handle_WAKEUPGPIO,   DCE_WRITE },
    {"CMD",          &dce_handle_CMD,          DCE_EXEC},
};

void dce_register_basic_commands(dce_t *dce)
{
    dce_register_command_group(dce, "BASIC", basic_commands, sizeof(basic_commands) / sizeof(command_desc_t), 0);

    if(wakeup_method_event_group == NULL)
        wakeup_method_event_group = xEventGroupCreate();
    gpio_wakeup_task_init();
    // quxin TrPsmWakeupRegister(wakeup_callback);
    psm_at_callback(wakeup_callback);
}



