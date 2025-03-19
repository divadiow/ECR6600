

#include <string.h>
#include "chip_clk_ctrl.h"
#include "arch_cpu.h"
#include "pit.h"
#include "efuse.h"
#include "flash.h"
#include "aes.h"
#include "oshal.h"
#include "encrypt_lock.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "rtc.h"
#include "hal_system.h"
#include "hal_rtc.h"
#include "chip_configuration.h"

void chip_c_init(void)
{
	extern unsigned int _sbss, _ebss;

	unsigned int *dest;

	extern unsigned char _sdata, _edata, _data_load;
	unsigned int size = &_edata - &_sdata;

	/* Copy data section from LMA to VMA */
	if(&_sdata != &_data_load)
	{
		memcpy(&_sdata, &_data_load, size);
	}

	/* clear .bss */
	for (dest = &_sbss; dest < &_ebss; )
	{
	    *dest++ = 0;
	}
}
 
#if defined(CONFIG_PSM_SURPORT) && defined(CONFIG_TUYA_DOORCONTACT)
extern void psm_por_reset();
#include "aon_reg.h"
static void psm_pad_gpio_status_init()
{
	WRITE_REG(AON_PAD_PE_REG,READ_REG(AON_PAD_PE_REG) & (~GPIO20_PAD_GPPIO_MODE));	//ADC:gpio20 restore to default
	WRITE_REG(AON_PAD_PS_REG,READ_REG(AON_PAD_PS_REG) & (~GPIO20_PAD_GPPIO_MODE));	
    WRITE_REG(AON_PAD_IE_REG , 0x3ffffff);
    WRITE_REG(AON_PAD_MODE_REG ,0x20000);
}
#endif
void chip_startup(void)
{
extern int main(void);
	chip_c_init();
	#if defined(CONFIG_PSM_SURPORT) && defined(CONFIG_TUYA_DOORCONTACT)
	psm_por_reset();
	#endif
	arch_cpu_init(); 
	drv_pit_init();

#if defined(CONFIG_EFUSE)
		drv_efuse_init();
#endif //CONFIG_EFUSE

	chip_clk_init();

	vPortHeapInit();

	/* To do bufore hal_rtc_init 
	 * It is determined via the RTC enable register that if reset key restart*/
	hal_reset_type_init(); 
	
#if defined(CONFIG_RTC)
	//drv_rtc_init();
	hal_rtc_init();
#endif //CONFIG_RTC

#ifdef CONFIG_AES
	drv_aes_init();
#endif //CONFIG_AES

#ifdef CONFIG_ENCRYPT_LOCK
	drv_encrypt_lock_init();
#endif //

#if defined(CONFIG_GPIO)
	drv_gpio_init();
#endif //CONFIG_GPIO
	
#ifdef CONFIG_FLASH
	drv_spiFlash_init();
#endif //CONFIG_FLASH

#ifdef CONFIG_CPU_DCACHE_ENABLE
	arch_dcache_enable(ARCH_DCACHE_ENABLE);
#endif

#ifdef CONFIG_CPU_ICACHE_ENABLE
	arch_icache_enable(ARCH_ICACHE_ENABLE);
#endif

    init_ate_efuse_info();
#if defined(CONFIG_PSM_SURPORT) && defined(CONFIG_TUYA_DOORCONTACT)
	psm_pad_gpio_status_init();
#endif
#ifdef CONFIG_SYSTEM_IRQ
	sys_irq_sw_set();
#endif
	main();
}
