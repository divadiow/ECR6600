/* wangc add ram link flie */


MEMORY
{
	ilm_memory    (rwx) : org = 0x00010000, len = 192K
	dlm_memory    (!rx) : org = 0x00060000, len = 128K
	iram_memory   (rwx) : org = 0x00080000, len = 192K
}

ENTRY(_start)
SECTIONS
{
  .text : 
    {
       	KEEP(*(.vector_init ));
	*libboard.a:(.text .text.*)
	*libpsm.a:(.text .text.*)
	*libarch.a:(.text .text.*)
	*libdrivers.a:(.text .text.*)
	*libos.a:(.text .text.*)
	*libps_wifi.a:(.text .text.*)
	*libps_ble.a:hci*.o(.text .text.*)
	*libps_ble.a:sch*.o(.text .text.*)
	*libps_ble.a:ble_thread.o(.text .text.*)
	*libps_ble.a:ecc_p256.o(.text .text.*)
	
	*libps_ble.a:dbg*.o(.rodata .rodata.*)
	*libps_ble.a:hci*.o(.rodata .rodata.*)
	*libps_ble.a:llm*.o(.rodata .rodata.*)
	*libps_ble.a:lld*.o(.rodata .rodata.*)
	*libps_ble.a:ecc_p256.o(.rodata .rodata.*)
	
       	*(.ex9.itable )
       	. = ALIGN(0x4);
       	PROVIDE(_eilm = .);
    } > ilm_memory


  .data : AT(0x60800)
    {
	. = ALIGN(4);
	PROVIDE(_data_load = LOADADDR(.data));
       	PROVIDE(_sdata = .);
       	*(.sdata*);
	   	. = ALIGN(4);
       	*(.srodata*);
       	. = ALIGN(4);
       	*(.rodata .rodata.* ) 
        KEEP(*(SORT(.cli_cmd* )))
	. = ALIGN(4);
	PROVIDE(_rodata = .);
       	*(.data*);
	. = ALIGN(0x4);
	*(.version.data);
       	PROVIDE(_edata = .);
       	. = ALIGN(0x4);
    }  > dlm_memory


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

  .text_iram :
  	{
    	*(.text .text.*)
	} > iram_memory

  SHARED (NOLOAD):
    {
        _sshram = . ;
        *(.dma.data)
        *(SHAREDRAM)
        *(SHAREDRAMIPC)
        _eshram = . ;
    } > iram_memory

    _end = .;

    PROVIDE (_stack = 0xb0000);
    PROVIDE (buf_start_0 =  _eilm);
    PROVIDE (buf_start_1 =  _ebss); 
    PROVIDE (buf_start_2 =  _eshram); 
    PROVIDE (buf_len_0 =  0x40000 - buf_start_0); 
    PROVIDE (buf_len_1 =  0x7fac0 - buf_start_1); 
    PROVIDE (buf_len_2 =  LENGTH(iram_memory) - ( _eshram - _sshram)); 
	#include "lib_rom.h"
}
