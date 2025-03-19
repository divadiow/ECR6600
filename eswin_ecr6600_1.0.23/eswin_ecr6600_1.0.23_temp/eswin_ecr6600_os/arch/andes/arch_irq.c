
#include <nds32_intrinsic.h>
#include "arch_defs.h"
#include "chip_irqvector.h"
#include "arch_irq.h"
#include "oshal.h"


#define SR_CLRB32(reg, bit)		\
{					\
	int mask = __nds32__mfsr(reg)& ~(1<<bit);\
        __nds32__mtsr(mask, reg);	 \
	__nds32__dsb();				 \
}

#define SR_SETB32(reg,bit)\
{\
	int mask = __nds32__mfsr(reg)|(1<<bit);\
	__nds32__mtsr(mask, reg);			\
	__nds32__dsb();						\
}

void arch_irq_clean(int vector_num)
{
	if ( vector_num == VECTOR_NUM_SW )
	{
		SR_CLRB32(NDS32_SR_INT_PEND, 16);
	}
	else
	{
		SR_SETB32(NDS32_SR_INT_PEND2, vector_num);
	}
}

void arch_irq_mask(int vector_num)
{
	SR_CLRB32(NDS32_SR_INT_MASK2, vector_num);
}

void arch_irq_unmask(int vector_num)
{
	SR_SETB32(NDS32_SR_INT_MASK2, vector_num);
}



extern void *ISR_TABLE[VECTOR_NUMINTRS];
void arch_irq_register(int vector_num, void_fn fn)
{
#if defined(CONFIG_RUNTIME_DEBUG)
	extern void comm_irq_isr_withtrace(int vector_num);
	extern void irq_isr_register_withtrace(int vector_num, void *fn);
	irq_isr_register_withtrace(vector_num, fn);
	ISR_TABLE[vector_num] = (void *)comm_irq_isr_withtrace;
#else
	ISR_TABLE[vector_num] = fn;
#endif


}

#ifndef CONFIG_SYSTEM_IRQ
unsigned long arch_irq_save( void )
{
	/* By default, the kerenl has the highest priority */
	/* It's mean that we don't support kernel preemptive */
	//unsigned long flags = __nds32__mfsr(NDS32_SR_PSW);
	//__nds32__gie_dis();
	
	unsigned long flags = system_irq_save();
        
    return flags;
}
#endif

void arch_irq_restore( unsigned long flags )
{
	system_irq_restore(flags);
	/*
	if (flags & PSW_mskGIE)
	{
		__nds32__gie_en();
	}
	*/
}


/****************************************************************************
 * Name: arch_irq_context
 *
 * Description: Return > 0 is we are currently executing in the interrupt handler context.
 *                   Return = 0 is not in the interrupt handler context.
 *
 ****************************************************************************/
int arch_irq_context(void)
{
	return __nds32__mfsr(NDS32_SR_PSW) & PSW_mskINTL;
}

