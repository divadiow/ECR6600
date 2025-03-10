#include "hal_efuse.h"
#include "efuse.h"


int hal_efuse_read(unsigned int addr,unsigned int * value, unsigned int length)
{
	int ret;
	ret = drv_efuse_read(addr, value, length);
	return ret;
}

int hal_efuse_write(unsigned int addr, unsigned int value, unsigned int mask)
{
	int ret;
	ret = drv_efuse_write(addr, value, mask);
	return ret;
}

#if 0
int hal_efuse_write_mac(unsigned char * mac)
{
	int ret;
	ret = drv_efuse_write_mac(mac);
	return ret;
}

int hal_efuse_read_mac(unsigned char * mac)
{
	int ret;
	ret = drv_efuse_read_mac(mac);
	return ret;
}
#endif

