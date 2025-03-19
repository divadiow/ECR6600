#include "chip_memmap.h"
#include "chip_clk_ctrl.h"
#include "arch_irq.h"
#include "trng.h"
#include "oshal.h"
#include "hal_trng.h"


unsigned int hal_trng_get(void)
{
	unsigned int trng_value = 0;
	trng_value = drv_trng_get();
	return trng_value;
}

unsigned int hal_prng_get(void)
{
	unsigned int prng_value = 0;
	prng_value = drv_prng_get();
	return prng_value;
}



