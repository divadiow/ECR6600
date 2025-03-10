


#ifndef __ARCH_CPU_H__
#define __ARCH_CPU_H__



#define ARCH_DCACHE_ENABLE		1
#define ARCH_DCACHE_DISABLE		0


#define ARCH_ICACHE_ENABLE			1
#define ARCH_ICACHE_DISABLE		0


void arch_cpu_init(void);
void arch_dcache_enable(int On);
void  arch_icache_enable(int On);

#endif

