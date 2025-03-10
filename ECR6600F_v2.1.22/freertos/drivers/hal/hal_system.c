
#include <stdlib.h>
#include "stdio.h"
#include "stdint.h"
#include "cli.h"
#include "hal_system.h"
#include "rtc.h"
#include "wdt.h"
#include "pit.h"
#include "hal_gpio.h"
#include "chip_pinmux.h"
#include "oshal.h"
#include "aon_reg.h"
#include <stdlib.h>
#include "chip_clk_ctrl.h"
#include "FreeRTOS.h"
#include "flash.h"
#include "efuse.h"
#include <string.h>
#include "format_conversion.h"
#include "reg_macro_def.h"



//#ifndef IN32
//#define IN32(reg) ( *( (volatile unsigned int *) (reg) ) )
//#endif
//
//#ifndef OUT32
//#define OUT32(offset,value) (*(volatile unsigned int *)(offset) = (unsigned int)(value));
//#endif 
#define REBOOT_HANDLERS_NUM  (3)

typedef struct {
	void *arg;
	reset_handler_t cb;
}reset_handlers_t;


#define BOOT_REASON_ADDR 	(0x201234)
#define BOOT_REASON_BIT 	(0x0)
#define BOOT_REASON_MASK 	(0xF)

#define EXTPAD_RST_EN_OFFSET		(0x10)

RST_TYPE g_system_reboot_reason = RST_TYPE_UNKOWN;
static reset_handlers_t        reset_handlers[REBOOT_HANDLERS_NUM];
void hal_system_reset_handlers_process(void);

void hal_system_reset_key_enable()
{
	WRITE_REG(AON_TEST_MODE_EXT_RST_REG, READ_REG(AON_TEST_MODE_EXT_RST_REG)&(~EXTPAD_RST_EN_OFFSET));
}

void hal_system_reset_key_disable()
{
	WRITE_REG(AON_TEST_MODE_EXT_RST_REG, READ_REG(AON_TEST_MODE_EXT_RST_REG)|(EXTPAD_RST_EN_OFFSET));
}

void hal_set_reset_type(RST_TYPE type)
{
	// 互斥保护
	unsigned long flage = system_irq_save();
	unsigned int value = (READ_REG(BOOT_REASON_ADDR>>BOOT_REASON_BIT)&(~(BOOT_REASON_MASK)));  // 置0
	WRITE_REG(BOOT_REASON_ADDR, value | type<<BOOT_REASON_BIT);
	system_irq_restore(flage);
}

void hal_system_reset(RST_TYPE type)
{
	#ifdef TUYA_SDK_CLI_ADAPTER
	WRITE_REG(0x201224, READ_REG(0x201224)|(1<<2));
	hal_gpio_init();
	PIN_FUNC_SET(IO_MUX_GPIO2, FUNC_GPIO2_GPIO2);
	hal_gpio_dir_set(GPIO_NUM_2, DRV_GPIO_ARG_DIR_OUT);
	hal_gpio_write(GPIO_NUM_2, LEVEL_HIGH);
	#endif
	
	hal_system_reset_handlers_process();

	if(type <= RST_TYPE_UNKOWN)
	{
		hal_set_reset_type(type);
		drv_wdt_chip_reset();
	}
	else
	{
		os_printf(LM_CMD, LL_ERR, "RST_TYPE error\n");
	}

}

void hal_reset_type_init()
{
#ifdef CONFIG_ANALYZE_REBOOT_TYPE
	hal_system_reset_key_enable();
#endif

	if(RST_TYPE_UNKOWN == g_system_reboot_reason)  // init state
	{
		// 互斥保护
		unsigned long flage = system_irq_save();
		g_system_reboot_reason = (RST_TYPE)(READ_REG(BOOT_REASON_ADDR>>BOOT_REASON_BIT)&(BOOT_REASON_MASK));
		if (g_system_reboot_reason == RST_TYPE_HARDWARE_REBOOT && READ_REG(MEM_BASE_RTC+0x18) != 0)
		{
			g_system_reboot_reason = RST_TYPE_UNKOWN;
		}

		unsigned int value = (READ_REG(BOOT_REASON_ADDR>>BOOT_REASON_BIT)&(~(BOOT_REASON_MASK)));
		WRITE_REG(BOOT_REASON_ADDR, value | RST_TYPE_HARDWARE_REBOOT);
		system_irq_restore(flage);
	}
}

RST_TYPE hal_get_reset_type()
{
	return g_system_reboot_reason;
}
#if 0
unsigned int reload_value_before_lock = 0;
unsigned int rtc_32K_restore1 = 0;
unsigned int rtc_32K_restore2 = 0;
unsigned int reload = 0;

//#define CHIP_CLOCK_APB_TEST 24000000
void func_after_irq()
{	
	drv_pit_ioctrl(DRV_PIT_CHN_6, DRV_PIT_CTRL_GET_COUNT, &reload_value_before_lock);
	
	drv_pit_ioctrl(DRV_PIT_CHN_6, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_NONE);
	
	rtc_32K_restore1 =  drv_rtc_get_32K_cnt();
}
void func_before_restore()
{	
	rtc_32K_restore2 =  drv_rtc_get_32K_cnt();

	unsigned int rtc_32K_passed = drv_rtc_get_interval_cnt(rtc_32K_restore1,rtc_32K_restore2);
	unsigned int pit_count_passed = ((rtc_32K_passed*1000)%32768)/1000*CHIP_CLOCK_APB/32768;
	//unsigned int pit_count_passed = ((rtc_32K_passed*1000)%32768)/1000*CHIP_CLOCK_APB/32768;
	unsigned int tick = rtc_32K_passed/32.768/portTICK_PERIOD_MS;
	#if 0
	cli_printf("rtc_32K_passed = %d\n",rtc_32K_passed);
	cli_printf("pit_count_passed = %d\n",pit_count_passed);
	cli_printf("tick = %d\n",tick);
	cli_printf("reload_value_before_lock = %d\n",reload_value_before_lock);
	#endif
	if (reload_value_before_lock > pit_count_passed)
	{
		reload = reload_value_before_lock - pit_count_passed;
	}
	else
	{
		reload = CHIP_CLOCK_APB / configTICK_RATE_HZ - pit_count_passed + reload_value_before_lock;
		tick++;
	}
	#if 0
	cli_printf("reload = %d\n",reload);
	cli_printf("tick adjust = %d\n",tick);
	cli_printf("tick = %d\n",xTaskGetTickCount());
	#endif
	vTaskStepTick(tick);
	
	drv_pit_ioctrl(DRV_PIT_CHN_6, DRV_PIT_CTRL_SET_COUNT, reload);
	
	drv_pit_ioctrl(DRV_PIT_CHN_6, DRV_PIT_CTRL_CH_MODE_ENABLE, DRV_PIT_CH_MODE_ENABLE_TIMER);
	
	drv_pit_ioctrl(DRV_PIT_CHN_6, DRV_PIT_CTRL_SET_COUNT, CHIP_CLOCK_APB/configTICK_RATE_HZ);
}
#endif
#if 0
int hal_system_set_mac(unsigned char *mac)
{
	int ret = -1;	
	ret = drv_spiflash_otp_write_mac(mac);
	return ret;
}

int hal_system_get_mac(unsigned char type, unsigned char *mac)
{
	int ret = -1,i;

	ret = drv_spiflash_otp_read_mac(mac);
	if (ret)
	{
		ret = drv_efuse_read_mac(mac);
		if (ret)
		{	
#ifdef MAC_ADDR_STANDALONE
			ret = 0;
			char tmp_mac[18] = {0};
			memcpy(tmp_mac, (const char*) MAC_ADDR_STANDALONE, 18);
			for (i=0; i<6; i++)
				mac[i] = hex2num(tmp_mac[i*3]) * 0x10 + hex2num(tmp_mac[i*3+1]);
#else
			ret = -1;
#endif
		}
	}


	
	//0:STA
	if (type == 0)
	{
		
	}
	//other:ap
	else
	{
		mac[5] += 1;
	}
	return ret;
}
#endif

void hal_system_reset_handlers_process(void)
{
	int idx;

	for (idx = 0; idx < REBOOT_HANDLERS_NUM; idx++)
	{
		if (reset_handlers[idx].cb)
		{
			reset_handlers[idx].cb(reset_handlers[idx].arg);
		}
	}
}

int hal_system_register_reset_handler(reset_handler_t handle, void *arg)
{
	int idx;

	for (idx = 0; idx < REBOOT_HANDLERS_NUM; idx++)
	{
		if (!reset_handlers[idx].cb)
		{
			reset_handlers[idx].cb = handle;
			reset_handlers[idx].arg = arg;
			return 0;
		}
	}

	return -1;
}
int hal_system_unregister_reset_handler(reset_handler_t handle)
{
	int idx;

	for (idx = 0; idx < REBOOT_HANDLERS_NUM; idx++)
	{
		if (reset_handlers[idx].cb == handle)
		{
			reset_handlers[idx].cb = NULL;
			reset_handlers[idx].arg = NULL;
			return 0;
		}
	}

	return -1;
}

