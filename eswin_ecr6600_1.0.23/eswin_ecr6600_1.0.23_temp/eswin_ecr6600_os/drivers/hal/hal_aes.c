#include "chip_memmap.h"
#include "chip_clk_ctrl.h"
#include "arch_irq.h"
#include "aes.h"
#include "oshal.h"
#include "hal_aes.h"


int hal_aes_lock(void)
{	
	return drv_aes_lock();
}

int hal_aes_unlock(void)
{	
	return drv_aes_unlock();
}

int hal_aes_ecb_setkey(const unsigned char *key, unsigned int keybits, int mode)
{
	return drv_aes_ecb_setkey(key, keybits, mode);
}

int hal_aes_cbc_setkey(const unsigned char *key, unsigned int keybits, int mode, const unsigned char *iv)
{
	return drv_aes_cbc_setkey(key, keybits, mode, iv);
}

void hal_aes_crypt(const unsigned char input[16], unsigned char output[16])
{
	return drv_aes_crypt(input, output);
}



