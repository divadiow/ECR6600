


#include <nds32_intrinsic.h>
#include "arch_defs.h"
#include "chip_memmap.h"


 
#ifndef VECTOR_BASE
#define VECTOR_BASE	0x00010000
#endif

#define PSW_MSK                                         \
        (PSW_mskGIE | PSW_mskINTL | PSW_mskPOM | PSW_mskAEN | PSW_mskIFCON | PSW_mskCPL)
#define PSW_INIT                                        \
        (0x0UL << PSW_offGIE                            \
         | 0x0UL << PSW_offINTL                         \
         | 0x1UL << PSW_offPOM                          \
         | 0x0UL << PSW_offAEN                          \
         | 0x0UL << PSW_offIFCON                        \
         | 0x7UL << PSW_offCPL)

#define IVB_MSK                                         \
        (IVB_mskEVIC | IVB_mskESZ | IVB_mskIVBASE)
#define IVB_INIT                                        \
        ((VECTOR_BASE >> IVB_offIVBASE) << IVB_offIVBASE\
         | 0x1UL << IVB_offESZ                          \
         | 0x0UL << IVB_offEVIC)

#define CACHE_NONE              0
#define CACHE_WRITEBACK         2
#define CACHE_WRITETHROUGH      3

#define CACHE_MODE              CACHE_WRITEBACK


#define MMU_CTL_MSK                                     \
        (MMU_CTL_mskD                                   \
         | MMU_CTL_mskNTC0                              \
         | MMU_CTL_mskNTC1                              \
         | MMU_CTL_mskNTC2                              \
         | MMU_CTL_mskNTC3                              \
         | MMU_CTL_mskTBALCK                            \
         | MMU_CTL_mskMPZIU                             \
         | MMU_CTL_mskNTM0                              \
         | MMU_CTL_mskNTM1                              \
         | MMU_CTL_mskNTM2                              \
         | MMU_CTL_mskNTM3)

#define MMU_CTL_INIT                                    \
        (0x0UL << MMU_CTL_offD                          \
         | 0x0UL << MMU_CTL_offNTC0              \
         | (CACHE_MODE) << MMU_CTL_offNTC1                     \
         | 0x0UL << MMU_CTL_offNTC2                     \
         | 0x0UL << MMU_CTL_offNTC3                     \
         | 0x0UL << MMU_CTL_offTBALCK                   \
         | 0x0UL << MMU_CTL_offMPZIU                    \
         | 0x0UL << MMU_CTL_offNTM0                     \
         | 0x1UL << MMU_CTL_offNTM1                     \
         | 0x2UL << MMU_CTL_offNTM2                     \
         | 0x3UL << MMU_CTL_offNTM3)




#define PRI1_DEFAULT            0xFFFFFFCF
#define PRI2_DEFAULT            0xFF03FFFF


 /* This must be a leaf function, no child function */
void _nds32_init_mem(void) __attribute__((naked));
void _nds32_init_mem(void)
{
	/* Enable DLM */
	__nds32__mtsr_dsb(MEM_BASE_DLM | 0x1, NDS32_SR_DLMB);
}



void arch_cpu_init(void)
{
	unsigned int reg;

	/* Enable BTB & RTP since the default setting is disabled. */
	reg = __nds32__mfsr(NDS32_SR_MISC_CTL) & ~(MISC_CTL_makBTB | MISC_CTL_makRTP);
	__nds32__mtsr(reg, NDS32_SR_MISC_CTL);

	/* Set PSW GIE/INTL to 0, superuser & CPL to 7 */
	reg = (__nds32__mfsr(NDS32_SR_PSW) & ~PSW_MSK) | PSW_INIT;
	__nds32__mtsr(reg, NDS32_SR_PSW);

	/* Set PPL2FIX_EN to 0 to enable Programmable Priority Level */
	__nds32__mtsr(0x0, NDS32_SR_INT_CTRL);

	/* Set vector size: 16 byte, base: VECTOR_BASE, mode: IVIC */
	reg = (__nds32__mfsr(NDS32_SR_IVB) & ~IVB_MSK) | IVB_INIT;
	__nds32__mtsr(reg, NDS32_SR_IVB);

	reg = __nds32__mfsr(NDS32_SR_INT_MASK) | INT_MASK_mskIDIVZE;
	__nds32__mtsr_dsb(reg, NDS32_SR_INT_MASK);
	/* Mask and clear hardware interrupts */
	__nds32__mtsr(0x0, NDS32_SR_INT_MASK2);
	__nds32__mtsr(-1, NDS32_SR_INT_PEND2);

	/* MMU initialization: NTC0~NTC3, NTM0~NTM3 */
	reg = (__nds32__mfsr(NDS32_SR_MMU_CTL) & ~MMU_CTL_MSK) | MMU_CTL_INIT;
	__nds32__mtsr_dsb(reg, NDS32_SR_MMU_CTL);

	/* Set default Hardware interrupts priority */
	__nds32__mtsr(PRI1_DEFAULT, NDS32_SR_INT_PRI);
	__nds32__mtsr(PRI2_DEFAULT, NDS32_SR_INT_PRI2);
}
 






