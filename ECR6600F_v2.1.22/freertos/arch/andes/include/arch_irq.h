

#ifndef __ARCH_IRQ_H__
#define __ARCH_IRQ_H__

#include "oshal.h"

typedef void (*void_fn)(void);


#define arch_irq_trigger_ecall(x)   	__nds32__syscall(x)

#define arch_irq_trigger_sw()   	__nds32__set_pending_swint()

#define arch_irq_global_enable()  __nds32__gie_en()

#define arch_irq_global_disable()  __nds32__gie_dis()




#define IRQ_LEVEL_HIGHEST				0
#define IRQ_LEVEL_SECOND_HIGHEST		1
#define IRQ_LEVEL_SECOND_LOWEST			2
#define IRQ_LEVEL_LOWEST				3


#define arch_irq_set_priority(vector_num, priority)		\
				__nds32__set_int_priority(vector_num, priority)

void arch_irq_clean(int vector_num);


void arch_irq_mask(int vector_num);


void arch_irq_unmask(int vector_num);


void arch_irq_register(int vector_num, void_fn fn);

#ifdef CONFIG_SYSTEM_IRQ
#define arch_irq_save		system_irq_save
#else
unsigned long arch_irq_save( void );
#endif

void arch_irq_restore( unsigned long flags );

/****************************************************************************
 * Name: arch_irq_context
 *
 * Description: Return > 0 is we are currently executing in the interrupt handler context.
 *                   Return = 0 is not in the interrupt handler context.
 *
 ****************************************************************************/
int arch_irq_context(void);

#endif

