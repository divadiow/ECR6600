
#include "flash.h"

int hal_flash_read(unsigned int addr, unsigned char * buff, unsigned int len)
{
	int ret;
	ret = drv_spiflash_read(addr, buff, len);
	return ret;
}

int hal_flash_write(unsigned int addr, unsigned char * buff, unsigned int len)
{
	int ret;
	ret = drv_spiflash_write(addr, buff, len);
	return ret;
}

int hal_flash_erase(unsigned int addr, unsigned int len)
{
	int ret;
	ret = drv_spiflash_erase(addr, len);
	return ret;
}

int hal_flash_otp_read(unsigned int addr, unsigned char * buff, unsigned int len)
{
	int ret;
	ret = drv_spiflash_OTP_Read(addr, len, buff);
	return ret;
}

int hal_flash_otp_write(unsigned int addr, unsigned char * buff, unsigned int len)
{
	int ret;
	ret = drv_spiflash_OTP_Write( addr, len, (unsigned char *) buff);
	return ret;
}

#if 0
int hal_flash_otp_erase(unsigned int addr)
{
	int ret;
	ret = drv_spiflash_OTP_Erase(addr);
	return ret;
}
#endif


int hal_flash_otp_read_aes(unsigned int addr, unsigned char * buff, unsigned int len)
{
	int ret;
	ret = drv_OTP_AES_Read(addr, len, buff);
	return ret;
}

int hal_flash_otp_write_aes(unsigned int addr, unsigned char * buff, unsigned int len)
{
	int ret;
	ret = drv_OTP_AES_Write( addr, len, (unsigned char *) buff);
	return ret;
}


int hal_flash_otp_lock(unsigned char otp_block)
{
	int ret;
	ret = drv_spiflash_OTP_Lock(otp_block);
	return ret;
}



