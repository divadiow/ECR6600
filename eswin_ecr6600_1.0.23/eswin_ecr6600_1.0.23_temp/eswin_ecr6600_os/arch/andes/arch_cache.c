

#include <nds32_intrinsic.h>
#include "arch_defs.h"












enum cache_t{ICACHE, DCACHE};


#define CACHE_CTL_ICACHE_ON                      (0x1UL << CACHE_CTL_offIC_EN)
#define CACHE_CTL_DCACHE_ON                     (0x1UL << CACHE_CTL_offDC_EN)


static inline unsigned long CACHE_SET(enum cache_t cache)
{
	if(cache == ICACHE)
	{
		return 64 << ((__nds32__mfsr(NDS32_SR_ICM_CFG) & ICM_CFG_mskISET) >> ICM_CFG_offISET);
	}
	else
	{
		return 64 << ((__nds32__mfsr(NDS32_SR_DCM_CFG) & DCM_CFG_mskDSET) >> DCM_CFG_offDSET);
	}
}

static inline unsigned long CACHE_WAY(enum cache_t cache)
{

	if(cache == ICACHE)
	{
		return 1 + ((__nds32__mfsr(NDS32_SR_ICM_CFG) & ICM_CFG_mskIWAY) >> ICM_CFG_offIWAY);
	}
	else
	{
		return 1 + ((__nds32__mfsr(NDS32_SR_DCM_CFG) & DCM_CFG_mskDWAY) >> DCM_CFG_offDWAY);
	}
}

static inline unsigned long CACHE_LINE_SIZE(enum cache_t cache){

	if(cache == ICACHE)
	{
		return 8 << (((__nds32__mfsr(NDS32_SR_ICM_CFG) & ICM_CFG_mskISZ) >> ICM_CFG_offISZ) - 1);
	}
	else
	{
		return 8 << (((__nds32__mfsr(NDS32_SR_DCM_CFG) & DCM_CFG_mskDSZ) >> DCM_CFG_offDSZ) - 1);
	}
}

void nds32_icache_flush(void)
{
	unsigned long end;
	unsigned long cache_line = CACHE_LINE_SIZE(ICACHE);

	end = CACHE_WAY(ICACHE) * CACHE_SET(ICACHE) * CACHE_LINE_SIZE(ICACHE);

	do {
		end -= cache_line;
		__nds32__cctlidx_wbinval(NDS32_CCTL_L1I_IX_INVAL, end);
	} while (end > 0);

	__nds32__isb();
}

void nds32_dcache_invalidate(void)
{
	__nds32__cctl_l1d_invalall();
	__nds32__msync_store();
	__nds32__dsb();
}

void  arch_icache_enable(int On)
{
	unsigned int reg = __nds32__mfsr(NDS32_SR_CACHE_CTL);

	if(On)
	{
		if(reg & CACHE_CTL_ICACHE_ON)
		{
			return;
		}
		else
		{
			nds32_icache_flush();
			reg |= CACHE_CTL_ICACHE_ON;
		}
	}
	else
	{
		if(reg & CACHE_CTL_ICACHE_ON)
		{
			reg &= ~CACHE_CTL_ICACHE_ON;
		}
		else
		{
			return;
		}
	}

	/* handle i-cache */
	__nds32__mtsr(reg, NDS32_SR_CACHE_CTL);
}



void arch_dcache_enable(int On)
{
	unsigned int reg = __nds32__mfsr(NDS32_SR_CACHE_CTL);

	if(On)
	{
		if(reg & CACHE_CTL_DCACHE_ON)
		{
			return;
		}
		else
		{
			nds32_dcache_invalidate();
			reg |= CACHE_CTL_DCACHE_ON;
		}
	}
	else
	{
		if(reg & CACHE_CTL_DCACHE_ON)
		{
			reg &= ~CACHE_CTL_DCACHE_ON;
		}
		else
		{
			return;
		}
	}

	/* handle d-cache */
	__nds32__mtsr(reg, NDS32_SR_CACHE_CTL);
}




