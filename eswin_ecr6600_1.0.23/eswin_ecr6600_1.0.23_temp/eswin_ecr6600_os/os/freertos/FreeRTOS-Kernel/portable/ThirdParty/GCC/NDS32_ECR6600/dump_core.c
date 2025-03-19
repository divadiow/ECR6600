#include <stdio.h>
#include <string.h>
#include "ISR_Support.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "oshal.h"
#include "pit.h"
#include "uart.h"
#include "cli.h"
#include "chip_memmap.h"
#include "chip_configuration.h"
#include "debug_core.h"
#include "hal_system.h"
typedef void (*panic_printf)(enum log_mod mod,  enum log_level level, const char *f, ...);


typedef enum _trap_entrypoint
{
	//(1) Trap TLB fill
	eTLB_Fill=1,	
	
	//(2) Trap PTE not present
	ePTE_Not_Present=2,

	//(3) Trap TLB misc
	eTLB_Misc=3,
	
	//(4) Trap TLB VLPT miss
	eTLB_VLPT_Miss=4,
	
	//(5) Trap Machine error
	eMachine_Error=5,
	
	//(6) Trap Debug related
	eDebug_Related=6,
	
	//(7) Trap General exception
	eGeneral_Exception=7,

	//(8) Syscall
	eSyscall=8,

	//(9) Heap Overflow
	eHeap_Overflow=9
}TRAP_ENTRYPONIT;

typedef enum _trap_type
{
	
	 ALIGNMENT						=	0x0, /* I/D */
	 RESERVED_INST					=	0x1,
	 TRAP							=	0x2,
	 ARITHMETIC						=	0x3,
	 PRECISE_BUS_ERROR				=	0x4, /* I/D */
	 IMPRECISE_BUS_ERROR			=	0x5, /* I/D */
	 COPROCESSOR					=	0x6,
	 PRIVILEGED_INST				=	0x7,
	 RESERVED_VALUE					=	0x8,
	 NONEXISTENT_MEM_ADDR			=	0x9, /* I/D */
	 MPZIU_CONTROL					=	0xA,
	 NEXT_PRECISE_STACK_OVERFLOW	=	0xB,
	 HEAP_CORRUPTION				=	0xC

}TRAP_TYPE;


#define NDS32_TRAP_REASON_MASK	0xf
#define NDS32_TRAP_TYPE_MASK	0x10



struct pt_regs
{
	uint32_t ipc;
	uint32_t ipsw;
	
	#if ( configSUPPORT_ZOL == 1 )
		/* ZOL system registers */
		uint32_t lb;
		uint32_t le;
		uint32_t lc;
	#endif

	#if ( configSUPPORT_IFC == 1 )
		/* IFC system register */
		uint32_t ifc_lp;
	#endif
	#if !( ( configSUPPORT_IFC == 1 ) && ( configSUPPORT_ZOL == 1 ) || ( configSUPPORT_IFC != 1 ) && ( configSUPPORT_ZOL != 1 ) )
		/* Dummy word for 8-byte stack alignment */
		uint32_t dummpy;
	#endif
	uint32_t r[26];
	uint32_t p0,p1,fp, gp, lp, sp;
};

struct stack_reg_layout
{
	uint32_t ipc;
	uint32_t ipsw;
	
#if ( configSUPPORT_ZOL == 1 )
		/* ZOL system registers */
		uint32_t lb;
		uint32_t le;
		uint32_t lc;
#endif

#if ( configSUPPORT_IFC == 1 )
		/* IFC system register */
		uint32_t ifc_lp;
#endif


	uint32_t r[26];

	uint32_t fp, gp, lp;

};

struct isr_reg_layout
{
#if ( configSUPPORT_IFC == 1 )
	uint32_t ipc;
	uint32_t ifc_lp;
#endif

#if ( configSUPPORT_FPU == 1 )
	uint32_t fpu[portFPU_REGS/2];
#endif
	uint32_t r0[16];
	uint32_t r1[6];
	uint32_t sp;
};

struct trap_info
{
	TRAP_ENTRYPONIT trap_entrypoint;
	TRAP_TYPE trap_type;
	uint32_t trap_pc;
	uint32_t trap_vaddr;
	const char * trap_reason;
};
struct trap_info g_trap_info;

static const char *nds32_trap_name[] __attribute__((unused)) = {
	"Alignment",
	"Undefined instruction",
	"Trap",
	"Arithmetic",
	"Precise bus error",
	"Imprecise bus error",
	"Coprocessor",
	"Privliged instruction",
	"Reserved value",
	"Invalid memory access",
	"MPZIU control",
	"Stack overflow",
	"Heap corruption"
};


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"
uint32_t stack_frame[8];



#define DUMP_BUFFER_SIZE    192
char dump_buffer[DUMP_BUFFER_SIZE] = {0};


static void uart_dump(const unsigned char * buf, unsigned int len)
{

	int i = 0;
    int uart_tx_fifo_depth = 0;
	extern CLI_DEV s_cli_dev;
    unsigned int p_uart_reg = (unsigned int )s_cli_dev.cli_uart_base;


	while(len)
	{
		if (uart_tx_fifo_depth == 0)
		{
			while(drv_uart_tx_ready((unsigned int)p_uart_reg));
			uart_tx_fifo_depth = CHIP_CFG_UART_FIFO_DEPTH;
		}

		while (uart_tx_fifo_depth > 0)
		{
			uart_tx_fifo_depth--;

			
			drv_uart_tx_putc((unsigned int)p_uart_reg, buf[i++]);

			if(--len == 0)
			{
				break;
			}
		}
	}

}


void dump_printf(enum log_mod mod,  enum log_level level,const char *f, ...)
{
    int offset = 0;
    va_list ap;
    va_start(ap,f);

    offset = vsnprintf((char *)dump_buffer, DUMP_BUFFER_SIZE, f, ap);
    uart_dump((unsigned char *)dump_buffer, offset);

    va_end(ap);
	
}

uint32_t get_current_pc(void)
{
    return (uint32_t )__builtin_return_address(0);
}

void dump_stack()
{
	int i=0;
    stack_frame[0]=(uint32_t )__builtin_return_address(0);
    stack_frame[1]=(uint32_t )__builtin_return_address(1);
    stack_frame[2]=(uint32_t )__builtin_return_address(2);
    stack_frame[3]=(uint32_t )__builtin_return_address(3);
    stack_frame[4]=(uint32_t )__builtin_return_address(4);
    stack_frame[5]=(uint32_t )__builtin_return_address(5);
    stack_frame[6]=(uint32_t )__builtin_return_address(6);
    stack_frame[7]=(uint32_t )__builtin_return_address(7);

	os_printf(LM_OS, LL_ASSERT, "Stack Backtrace:\r\n");
	for(i=0; i<sizeof(stack_frame)/sizeof(uint32_t);i++)
	{
		if(stack_frame[i])
		{
			os_printf(LM_OS, LL_ASSERT, "   Frame%d: pc=0x%x\r\n",i,(unsigned int)stack_frame[i]);
		}
	}
}
#pragma GCC diagnostic pop


struct pt_regs *saved_pt;
struct isr_reg_layout * isr_saved_pt;
uint32_t g_flashdump_enable=0;
uint32_t g_flashdump_base;
uint32_t g_flashdump_len;

static int flash_rw_operation(char*data,int datalen,int op_type)
{
	datalen=(g_flashdump_len<datalen)?g_flashdump_len:datalen;
	if(datalen)
	{
		if(0==op_type)
		{
			//hal_spifiash_write((uint8_t*)g_flashdump_base,data,datalen);
		}
		else if(1==op_type)
		{
			//hal_spiflash_read((uint8_t*)g_flashdump_base,data,datalen);
		}
		g_flashdump_base+=datalen;
		g_flashdump_len-=datalen;
	}
	return datalen;
}

void show_trap_reason(panic_printf dump_show, uint32_t itype, uint32_t faddr, uint32_t pc)
{
	uint32_t trap_type = itype&NDS32_TRAP_TYPE_MASK;
	uint32_t trap_reason=itype&NDS32_TRAP_REASON_MASK;
	switch (trap_reason) {
		case ALIGNMENT:
			dump_show(LM_OS, LL_ASSERT,"Unaligned access (%c) at pc=[<%08x>] addr=[<%08x>]\r\n", 			(trap_type)? 'I' : 'D', (unsigned int)pc, (unsigned int)faddr);
			break;
		case NONEXISTENT_MEM_ADDR:
			dump_show(LM_OS, LL_ASSERT,"Invalid memory access (%c) at pc=[<%08x>] target=[<%08x>]?\r\n",	(trap_type)? 'I' : 'D', (unsigned int)pc, (unsigned int)faddr);
			break;
		case PRECISE_BUS_ERROR:
		case IMPRECISE_BUS_ERROR:
			dump_show(LM_OS, LL_ASSERT,"%s bus error (%c) at pc=[<%08x>] addr=[<%08x>]\r\n",				((trap_reason) == PRECISE_BUS_ERROR) ? "" : "Imprecise",(trap_type) ? 'I' : 'D', (unsigned int)pc, (unsigned int)faddr);
			break;
		case ARITHMETIC:
			dump_show(LM_OS, LL_ASSERT,"%s at pc=[<%08x>]\r\n",											(((itype>>16)&0xF) == 1) ? "Division by zero" : "Integer overflow",  (unsigned int)pc);
			break;
		default:
			dump_show(LM_OS, LL_ASSERT,"%s at pc=[<%08x>]\r\n", 											nds32_trap_name[trap_reason], (unsigned int)pc);
			break;
	}

}

void show_dumpstack(panic_printf dump_show,struct trap_info *ptrap_info, TaskStatus_t *ptsk, struct pt_regs* pregs, uint32_t* pframe)
{

		int i = 0;
		
		dump_show(LM_OS, LL_ASSERT,"Process: %s (pid=%d, threadinfo=%p, prio=%d, baseprio=%d, stack limit=%p, stack remain=%d),trap entrypoint=%d\r\n", \
					ptsk->pcTaskName, ptsk->xTaskNumber, ptsk->xHandle, ptsk->uxCurrentPriority, ptsk->uxBasePriority, ptsk->pxStackBase,ptsk->usStackHighWaterMark,ptrap_info->trap_entrypoint);

		show_trap_reason(dump_show,ptrap_info->trap_type,ptrap_info->trap_vaddr,pregs->ipc);			
		
		for (i = 0; i < 24; i += 4) {
			dump_show(LM_OS, LL_ASSERT,"r%-2d: %08x  r%-2d: %08x  r%-2d: %08x  r%-2d: %08x\r\n",
			       i, (unsigned int)pregs->r[i],
			       i + 1,  (unsigned int)pregs->r[i+1],
			       i + 2,  (unsigned int)pregs->r[i+2],
			       i + 3,  (unsigned int)pregs->r[i+3]);
		}
		dump_show(LM_OS, LL_ASSERT,"r24: %08x  r25: %08x  p0 : %08x  p1 : %08x\r\n",
		       (unsigned int)pregs->r[24], (unsigned int)pregs->r[25], (unsigned int)pregs->p0, (unsigned int)pregs->p1);
		dump_show(LM_OS, LL_ASSERT,"fp : %08x  gp : %08x  lp : %08x  sp : %08x\r\n",
		       (unsigned int)pregs->fp, (unsigned int)pregs->gp, (unsigned int)pregs->lp, (unsigned int)pregs->sp);
		dump_show(LM_OS, LL_ASSERT,"pc : %08x  psw: %08x\r\n", (unsigned int)pregs->ipc, (unsigned int)pregs->ipsw);

		
		dump_show(LM_OS, LL_ASSERT,"Stack Backtrace:\n");
		for(i=0; i<sizeof(stack_frame)/sizeof(uint32_t);i++)
		{
			if(pframe[i])
			{
				dump_show(LM_OS, LL_ASSERT,"   Frame%d: pc=0x%x\r\n",i,(unsigned int)pframe[i]);
			}
		}
		
		dump_show(LM_OS, LL_ASSERT,"Backtrace is over\r\n");

}


void flash_dump(struct pt_regs *pregs,TaskStatus_t *ptsk)
{

	if(0==g_flashdump_enable)
	{
		return ;
	}
	
	//partion_info_get("ota", &g_flashdump_base, &g_flashdump_len); 
	if(g_flashdump_base)
	{
		//hal_spiflash_erase((uint32_t *)g_flashdump_base,g_flashdump_len);
		

		flash_rw_operation((char*)&g_trap_info,sizeof(g_trap_info),0);
		flash_rw_operation((char*)pregs,sizeof(struct pt_regs ),0);
		flash_rw_operation((char*)stack_frame,sizeof(stack_frame),0);
		flash_rw_operation((char*)ptsk,sizeof(TaskStatus_t),0);
		flash_rw_operation((char*)ptsk->pcTaskName,configMAX_TASK_NAME_LEN,0);

		//flash_rw_operation((const unsigned char *)0x200000,0x24000,FLASH_DUMPOP_WRITE);
		//flash_rw_operation((const unsigned char *)sysHeap[1].pucStartAddress,sysHeap[1].xSizeInBytes,FLASH_DUMPOP_WRITE);
	
	}

}

void flash_dumpstack()
{
	
	char taskname[configMAX_TASK_NAME_LEN]={0};
	struct trap_info tmp_trapinfo={0};
	struct pt_regs regs={0};	
	TaskStatus_t tsk={0};
	uint32_t * pframe = os_malloc(sizeof(stack_frame));
	
	if(g_flashdump_base)
	{
		//partion_info_get("ota", &g_flashdump_base, &g_flashdump_len); 
		flash_rw_operation((char*)&tmp_trapinfo,sizeof(tmp_trapinfo),1);
		flash_rw_operation((char*)&regs,sizeof(struct pt_regs ),1);
		flash_rw_operation((char*)pframe,sizeof(stack_frame),0);
		flash_rw_operation((char*)&tsk,sizeof(TaskStatus_t),1);
		flash_rw_operation((char*)taskname,configMAX_TASK_NAME_LEN,1);
		taskname[configMAX_TASK_NAME_LEN-1]='\0';
		tsk.pcTaskName=taskname;
		
		os_printf(LM_OS, LL_ASSERT,"\r\n------------------------- [flash dump stack begin] -------------------------\r\n");
		
		
		show_dumpstack(os_printf,&tmp_trapinfo, &tsk, &regs, pframe);
		
		
		os_printf(LM_OS, LL_ASSERT,"\r\n------------------------- [flash dump stack end] -------------------------\r\n");
	
	}
	os_free(pframe);
}



//extern E_DRV_UART_NUM cli_uart_num;

void ram_dump()
{
	extern CLI_DEV s_cli_dev;
	unsigned int uart_base_reg = (unsigned int )s_cli_dev.cli_uart_base;
	unsigned int num=0;
	char cmd[32];
#if defined(CONFIG_LD_SCRIPT_XIP)
	unsigned int ilm_len=0x30000;
	unsigned int dlm_len=0x20000;
#else
	unsigned int ilm_len=0x30000;
	unsigned int dlm_len=0x20000;
#endif
	unsigned int iram_len=0x30000;
	
	while(1)
	{
			while(drv_uart_rx_tstc(uart_base_reg))
			{
				cmd[num] = drv_uart_rx_getc(uart_base_reg);
				if(num==sizeof(cmd)-1)
				{
					cmd[num]='\r';
				}
				if(cmd[num]=='\r')
				{
					cmd[num]='\0';
					if(strstr(cmd,"ramdump"))
					{
						
						//dump_printf(LM_OS, LL_CRT,"ramdump is on the way\r\n");
					
						//ilm base 0x10000 len 0x30000=192K
						//dlm base 0x60000 len 0x20000=128K
						//ilm base 0x80000 len 0x30000=192K
						/*
							#define MEM_BASE_ILM0					0x00010000
							#define MEM_BASE_ILM1					0x00018000
							#define MEM_BASE_ILM2					0x00020000

							#define MEM_BASE_DLM					0x00060000
							#define MEM_BASE_RAM0					0x00080000
							#define MEM_BASE_RAM1					0x000A0000
						*/
						dump_printf(LM_OS, LL_ASSERT,"ramdump ilm from 0x%x ,len 0x%x\r\n",MEM_BASE_ILM0,ilm_len);
						uart_dump((unsigned char*)MEM_BASE_ILM0,ilm_len);
						
						dump_printf(LM_OS, LL_ASSERT,"ramdump dlm from 0x%x ,len 0x%x\r\n",MEM_BASE_DLM,dlm_len);
						uart_dump((unsigned char*)MEM_BASE_DLM,dlm_len);
						
						dump_printf(LM_OS, LL_ASSERT,"ramdump iram from 0x%x ,len 0x%x\r\n",MEM_BASE_RAM0,iram_len);
						uart_dump((unsigned char*)MEM_BASE_RAM0,iram_len);
						
						//dump_printf(LM_OS, LL_ASSERT,"ramdump is over\r\n");
					}
					else if(strstr(cmd,"reboot"))
					{
						//TODO
						hal_system_reset(RST_TYPE_SOFTWARE_REBOOT);
					}
					num=0;
				}
				else
				{
					num++;
				}
				
			}
		}

}

void core_dump(struct pt_regs *pregs,TaskStatus_t *ptsk)
{
	//flash_dump(pregs, ptsk);
	ram_dump();
	
}
extern uint32_t ISR_STACK_BASE[];
extern uint32_t xISRStack[];
void frame_backtrace(struct pt_regs *pt, TaskStatus_t *tsk)
{
	uint32_t  pc;
	uint32_t  fp;
	uint32_t  sp = (uint32_t)pt;
	uint32_t  stack_base = (uint32_t)ISR_STACK_BASE;
	uint32_t  stack_end = (uint32_t)xISRStack;
	uint32_t  loop=0;
	uint32_t  limit=sizeof(stack_frame)/sizeof(uint32_t);
	uint32_t  exception_in_isr = 0;


	pc=pt->ipc;
	fp=pt->fp;
	if((sp >= stack_base) && (sp <= stack_end))
	{
		exception_in_isr=1;
	}
	else
	{
		stack_base=(uint32_t)tsk->pxStackBase;
		stack_end=(uint32_t)tsk->pxStackBase+1024*10;	
	}

	
	memset(stack_frame,0x00,sizeof(stack_frame));
	while(loop<limit)
	{
		stack_frame[loop]=pc;
		loop=loop+1;

		if(fp>=stack_base&&fp<=stack_end)
		{
			pc=*((uint32_t *)fp-1);
			fp=*((uint32_t *)fp-3);
		}
		else
		{
			if(exception_in_isr)
			{
				isr_saved_pt=(struct isr_reg_layout *)fp;
			}
			break;
		}
	}
	
}


void handle_general_exception(struct pt_regs *pregs)
{

	TaskStatus_t tsk;

	saved_pt=pregs;
	
	hal_set_reset_type(RST_TYPE_FATAL_EXCEPTION);

	g_trap_info.trap_type 	= __nds32__mfsr(NDS32_SR_ITYPE);
	g_trap_info.trap_vaddr  = __nds32__mfsr(NDS32_SR_EVA);
	g_trap_info.trap_pc 	= pregs->ipc;
	g_trap_info.trap_reason = nds32_trap_name[g_trap_info.trap_type&NDS32_TRAP_REASON_MASK];

	vTaskGetTaskInfo(NULL, &tsk, pdTRUE, eRunning);

	frame_backtrace(pregs,&tsk);
	

	dump_printf(LM_OS, LL_ASSERT,"\r\n------------------------- [cut here] -------------------------\r\n");

	show_dumpstack(dump_printf,&g_trap_info, &tsk, pregs, stack_frame);

	dump_printf(LM_OS, LL_ASSERT,"Now system halted, Do crash ramdump...\r\n");

	core_dump(pregs,&tsk);


}

//#define offsetof(TYPE,MEMBER) ((size_t)&((TYPE*)0)->MEMBER)
void trap_Heap_Error(int task_handle,unsigned char error_type)
{
	
	StaticTask_t* pxTcb = (StaticTask_t*)task_handle;
	uint32_t itype;
	unsigned long flags;
	uint32_t * sp=pxTcb->pxDummy1;
	struct pt_regs regs={0};
	struct stack_reg_layout * reg_layout;
	
	flags = system_irq_save();
	
	g_trap_info.trap_entrypoint=eHeap_Overflow;

	#if ( configSUPPORT_FPU == 1 )
		/* FPU registers */
		sp += portFPU_REGS;
	#endif

	#if ( ( configSUPPORT_IFC == 1 ) && ( configSUPPORT_ZOL == 1 ) || ( configSUPPORT_IFC != 1 ) && ( configSUPPORT_ZOL != 1 ) )
		/* Dummy word for 8-byte stack alignment */
		sp += 1;
	#endif

	reg_layout = (struct stack_reg_layout *)sp;

	memcpy(&regs,reg_layout, offsetof(struct pt_regs,p0));
	memcpy(&regs.fp, &reg_layout->fp,12);
	regs.sp=(int)sp+sizeof(struct stack_reg_layout);

	if(0==error_type)
	{
		itype = (__nds32__mfsr(NDS32_SR_ITYPE) & ~0xf) | NEXT_PRECISE_STACK_OVERFLOW;
		__nds32__mtsr(itype, NDS32_SR_ITYPE);
	}
	else if(1==error_type)
	{
		itype = (__nds32__mfsr(NDS32_SR_ITYPE) & ~0xf) | HEAP_CORRUPTION;
		__nds32__mtsr(itype, NDS32_SR_ITYPE);
	}
	handle_general_exception(&regs);

	system_irq_restore(flags);

}


//(1) Trap TLB fill
void __attribute__((naked)) 	trap_TLB_Fill(void)		
{
	g_trap_info.trap_entrypoint=eTLB_Fill;
	asm("la $r0, trap_General_Exception");
	asm("jr5	$r0");

}

//(2) Trap PTE not present
void __attribute__((naked)) 	trap_PTE_Not_Present(void) 	
{
	
	g_trap_info.trap_entrypoint=eTLB_Fill;
	asm("la $r0, trap_General_Exception");
	asm("jr5	$r0");

}

//(3) Trap TLB misc
void __attribute__((naked)) 	trap_TLB_Misc(void)	
{
	
	g_trap_info.trap_entrypoint=eTLB_Misc;
	asm("la $r0, trap_General_Exception");
	asm("jr5	$r0");

}

//(4) Trap TLB VLPT miss
void __attribute__((naked)) 	trap_TLB_VLPT_Miss(void)
{
	
	g_trap_info.trap_entrypoint=eTLB_VLPT_Miss;
	asm("la $r0, trap_General_Exception");
	asm("jr5	$r0");

}

//(5) Trap Machine error
void __attribute__((naked)) 	trap_Machine_Error(void)
{
	
	g_trap_info.trap_entrypoint=eMachine_Error;
	asm("la $r0, trap_General_Exception");
	asm("jr5	$r0");

}

//(6) Trap Debug related
void __attribute__((naked)) 	trap_Debug_Related(void)
{
	
	g_trap_info.trap_entrypoint=eDebug_Related;
	asm("la $r0, trap_General_Exception");
	asm("jr5	$r0");

}

//(7) Trap General exception
void __attribute__((naked)) 	trap_General_Exception(void)
{
	
	g_trap_info.trap_entrypoint=eGeneral_Exception;

#if !( ( configSUPPORT_IFC == 1 ) && ( configSUPPORT_ZOL == 1 ) || ( configSUPPORT_IFC != 1 ) && ( configSUPPORT_ZOL != 1 ) )
		/* Dummy word for 8-byte stack alignment */
		asm("push	$r0");
#endif

	
#if ( configSUPPORT_IFC == 1 )
/* IFC system register */
		asm("mfusr	$r0, $ifc_lp");
		asm("push	$r0");
#endif

#if ( configSUPPORT_ZOL == 1 )
		asm("mfusr	$r0, $lb");
		asm("mfusr	$r1, $le");
		asm("mfusr	$r2, $lc");
		asm("pushm	$r0, $r2");
#endif
		asm("mfsr	$r0, $ipc");
		asm("mfsr	$r1, $ipsw");
		asm("pushm	$r0, $r1");

		handle_general_exception((void *) __nds32__get_current_sp());

}

//(8) Syscall
void __attribute__((naked)) 	trap_Syscall(void) 			
{
	g_trap_info.trap_entrypoint=eSyscall;

	asm("la $r0, trap_General_Exception");
	asm("jr5	$r0");

}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, const char *pcTaskName)
{
    dump_printf(LM_OS, LL_ASSERT,"***ERROR*** A stack overflow in task %p:%s has been detected.\r\n", pxTask,pcTaskName);
	
	trap_Heap_Error((int)pxTask,0);
}
void vApplicationStackOverflowCheck(int task_handle)				
{
	StaticTask_t* pxTcb = (StaticTask_t*)task_handle;
	const uint32_t * const pulStack = ( uint32_t * ) pxTcb->pxDummy6;                       
    const uint32_t ulCheckValue = ( uint32_t ) 0xa5a5a5a5;                                        
                                                                                                  
    if( ( pulStack[ 0 ] != ulCheckValue ) ||                                                      
        ( pulStack[ 1 ] != ulCheckValue ) ||                                                      
        ( pulStack[ 2 ] != ulCheckValue ) ||                                                      
        ( pulStack[ 3 ] != ulCheckValue ) )                                                       
    {                                                                                             
        vApplicationStackOverflowHook( ( TaskHandle_t ) task_handle, (char *)pxTcb->ucDummy7 ); 
    } 
}

void abort()
{
	system_assert(0);
}

int dump_core_assert(void)
{

	system_assert(0);
	
	return 0;
}
#define IRQ_SWITCH_NUM 10
struct task_switch_s
{
	unsigned int irq;
	unsigned int ulclk;
	struct isr_reg_layout cur_isr_layout;
};
struct task_switch_s xIrqSwitchStats[IRQ_SWITCH_NUM];
struct isr_reg_layout *g_cur_irq=NULL;

int irq_in(unsigned int irq,uint32_t stack)
{
	static uint8_t ucxTaskSwitchIdx=0;

	if(g_cur_irq)
	{
		system_assert(0);
	}
	xIrqSwitchStats[ucxTaskSwitchIdx].irq=irq;
	xIrqSwitchStats[ucxTaskSwitchIdx].ulclk=drv_pit_get_tick();
	g_cur_irq=&(xIrqSwitchStats[ucxTaskSwitchIdx].cur_isr_layout);
	g_cur_irq->sp=stack;
	memcpy((char*)g_cur_irq,(char *)stack,sizeof(struct isr_reg_layout)-4);
	
	ucxTaskSwitchIdx=(ucxTaskSwitchIdx+1)%IRQ_SWITCH_NUM;

	return irq;
}
int irq_out(uint32_t stack)
{
	
	if(stack!=g_cur_irq->sp||memcmp((char*)g_cur_irq,(char *)stack,sizeof(struct isr_reg_layout)-4))
	{
		system_assert(0);
	}
	g_cur_irq=NULL;

	return 0;
}

void os_store_regs(int task_handle)
{
	StaticTask_t* pxTcb = (StaticTask_t*)task_handle;
	uint32_t * sp=pxTcb->pxDummy1;
	struct stack_reg_layout * reg_layout=(struct stack_reg_layout *)pxTcb->reg_dummy;
	
#if ( configSUPPORT_FPU == 1 )
		/* FPU registers */
		sp += portFPU_REGS;
#endif

#if ( ( configSUPPORT_IFC == 1 ) && ( configSUPPORT_ZOL == 1 ) || ( configSUPPORT_IFC != 1 ) && ( configSUPPORT_ZOL != 1 ) )
		/* Dummy word for 8-byte stack alignment */
		sp += 1;
#endif
	memcpy((char*)reg_layout,(char*)sp, sizeof(pxTcb->reg_dummy));
	

	
}
void os_check_regs(int task_handle)
{
	StaticTask_t* pxTcb = (StaticTask_t*)task_handle;
	uint32_t * sp=pxTcb->pxDummy1;
	struct stack_reg_layout * reg_layout=(struct stack_reg_layout *)pxTcb->reg_dummy;
	
#if ( configSUPPORT_FPU == 1 )
		/* FPU registers */
		sp += portFPU_REGS;
#endif

#if ( ( configSUPPORT_IFC == 1 ) && ( configSUPPORT_ZOL == 1 ) || ( configSUPPORT_IFC != 1 ) && ( configSUPPORT_ZOL != 1 ) )
		/* Dummy word for 8-byte stack alignment */
		sp += 1;
#endif


	if(memcmp((char*)reg_layout,(char*)sp, sizeof(pxTcb->reg_dummy)))
	{
		system_assert(0);
	}
}

