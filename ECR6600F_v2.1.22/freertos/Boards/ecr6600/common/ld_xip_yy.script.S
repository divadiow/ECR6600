/* wangc add xip link flie */


MEMORY
{
  ilm_memory	(rwx) : org = 0x00010000, len = 192K
  dlm_memory	(!rx) : org = 0x00060000, len = 0x1fa00
#if !defined(CONFIG_WIFI_PRODUCT_FHOST) && !defined(CONFIG_ESWIN_SDIO)
  iram_memory  	(rwx) : org = 0x000a0000, len = 64K
#else
  iram_memory  	(rwx) : org = 0x00080000, len = 190K

#endif
#if defined(XIP_ADDR_OFFSET)
  xip_memory  	(rx)  : org = 0x40800000+XIP_ADDR_OFFSET+0x100, len = 4M
#else
  xip_memory  	(rx)  : org = 0x40807100, len = 4M
#endif
}

ENTRY(_start)
SECTIONS
{
  .text : 
    {
	KEEP(*(.vector_init ));
	*libboard.a:(.text .text.*)
	*libpsm.a:(.text .text.*)

	TEXT_BEGIN_DRV = .;
	*( .ilm_text_drv )
	*libdrivers.a:dma.o(.text .text.*)
	*libdrivers.a:efuse.o(.text .text.*)
	*libdrivers.a:flash*.o(.text .text.*)

	*libdrivers.a:ir.o(.text .text.*)
	*libdrivers.a:i2c.o(.text .text.*)
	*libdrivers.a:i2s.o(.text .text.*)
	*libdrivers.a:pit.o(.text .text.*)
	*libdrivers.a:pwm.o(.text .text.*)
	*libdrivers.a:rtc.o(.text .text.*)

	*libdrivers.a:sdhci.o(.text .text.*)
	*libdrivers.a:spi_master.o(.text .text.*)
	*libdrivers.a:spi_slave_service.o(.text .text.*)
	*libdrivers.a:spi_slave.o(.text .text.*)

	*libdrivers.a:timer.o(.text .text.*)
	*libdrivers.a:trng.o(.text .text.*)
	*libdrivers.a:uart.o(.text .text.*)
	*libdrivers.a:wdt.o(.text .text.*)

	*libdrivers.a:hal_system.o(.text .text.*)
	TEXT_SIZE_DRV = . - TEXT_BEGIN_DRV;

	TEXT_BEGIN_WIFI = .;
	*( .ilm_text_wifi )
	TEXT_SIZE_WIFI = . - TEXT_BEGIN_WIFI;

	TEXT_BEGIN_BLE = .;
	*( .ilm_text_ble )
	TEXT_SIZE_BLE = . - TEXT_BEGIN_BLE;

	*libps_wifi.a:hal_machw.o(.text .text.*)
	*libps_wifi.a:intc.o(.text .text.*)
	*libarch.a:(.text .text.*)
	//*libdrivers.a:(.text .text.*)
	*libos.a:(.text .text.*)
	*libps_wifi.a:hal_rf_6600.o(.text .text.*)
	//*libps_wifi.a:phy_custom_rf.o(.text .text.*)
	//*libps_wifi.a:fhost_tx.o(.text .text.*)
	//*libps_wifi.a:net_al.o(.text .text.*)
	//*libps_wifi.a:tx_*.o(.text .text.*)
	*libps_wifi.a:rxl_*.o(.text .text.*)
	//*libps_wifi.a:txl_*.o(.text .text.*)
	*libps_wifi.a:txl_buffer_shared.o(.text .text.*)
        *libps_wifi.a:txl_frame_shared.o(.text .text.*)
        *libps_wifi.a:txl_cntrl.o(.text .text.*)
        *libps_wifi.a:txl_cfm.o(.text .text.*)
        *libps_wifi.a:txl_agg.o(.text .text.*)
        *libps_wifi.a:txl_he.o(.text .text.*)
        //*libps_wifi.a:txl_buffer.o(.text .text.*)
        //*libps_wifi.a:txl_frame.o(.text .text.*)

	*libps_wifi.a:mm_timer.o(.text .text.*)
	*libps_ble.a:sch_alarm.o(.text .text.*)
	*libps_ble.a:sch_arb.o(.text .text.*)
	*libps_ble.a:sch_plan.o(.text .text.*)
	*libps_ble.a:sch_prog.o(.text .text.*)
	*libps_ble.a:sch_slice.o(.text .text.*)
	*libps_ble.a:lld_adv.o(.text .text.*)
	*libps_ble.a:lld_con.o(.text .text.*)
	*libps_ble.a:lld_scan.o(.text .text.*)
	*libps_ble.a:lld_init.o(.text .text.*)
	*libps_ble.a:lld.o(.text .text.*)
	*libps_ble.a:rwip_driver.o(.text .text.*)
	*libps_ble.a:rwble.o(.text .text.*)
	*libps_ble.a:ble_util_buf.o(.text .text.*)
	*libps_wifi.a:co_list_ble.o(.text .text.*)
	//*libps_wifi.a:ke.o(.text .text.*)
	//*libps_wifi.a:ke_event.o(.text .text.*)
	//*libps_wifi.a:ke_mem_freertos.o(.text .text.*)
	//*libps_wifi.a:ke_msg.o(.text .text.*)
	//*libps_wifi.a:ke_queue.o(.text .text.*)
	//*libps_wifi.a:ke_task.o(.text .text.*)
	//*libps_wifi.a:ke_timer.o(.text .text.*)
	
	*(.ex9.itable )
	. = ALIGN(0x4);
	PROVIDE(_eilm = .);
    } > ilm_memory

  .bss :
    {
       . = ALIGN(4);
       PROVIDE(_sbss = .);
       *(.sbss_w .sbss_w.* )
       *(.bss*)
       *(COMMON)
       *(.scommon_*)
       . = ALIGN(4);
       PROVIDE(_ebss = .);
    } > dlm_memory

    SHARED (NOLOAD):
    {
        _sshram = . ;
        *(LARAM)
        *(.dma.data)
        *(SHAREDRAM)
        *(SHAREDRAMIPC)
    } > iram_memory
	
	.data :
    {
       . = ALIGN(4);
       PROVIDE(_data_load = LOADADDR(.data));
       PROVIDE(_sdata = .);
       *(.sdata*);
	   . = ALIGN(4);
       *(.srodata*);
       . = ALIGN(4);
       *(.data*);
	   . = ALIGN(0x4);
       PROVIDE(_edata = .);
       . = ALIGN(0x4);
	   _eshram = . ;
    }  > iram_memory

	.xip :
    {
        *(.xip ) 
        *(.text .text.*)
        __etext1 = .;
        *(.rodata .rodata.* ) 
		*(.version.data);
        KEEP(*(SORT(.cli_cmd* )))
    } > xip_memory

    _end = .;

    PROVIDE (_stack = 0xb0000);  /* main stack */
    PROVIDE (buf_start_0 =  _eilm);
    PROVIDE (buf_start_1 =  _ebss); 
    PROVIDE (buf_start_2 =  _eshram); 
    PROVIDE (buf_len_0 =  0x40000 - buf_start_0); 
    PROVIDE (buf_len_1 =  0x7fac0 - buf_start_1);  /* jump bootrom rodata */
    PROVIDE (buf_len_2 =  LENGTH(iram_memory) - ( _eshram - _sshram));

    #include "lib_rom.h"
}
