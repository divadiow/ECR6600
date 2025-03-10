#include "chip_memmap.h"
#include "chip_clk_ctrl.h"
#include "arch_irq.h"
#include "hash.h"
#include "oshal.h"
#include "hal_hash.h"




int hal_hash_sha256(const unsigned char *input, unsigned int ilen, unsigned char output[32])
{
	return drv_hash_sha256_ret(input, ilen, output);
}

int hal_hash_sha512(const unsigned char *input, unsigned int ilen, unsigned char output[64])
{
	return drv_hash_sha512_ret(input, ilen, output);
}






