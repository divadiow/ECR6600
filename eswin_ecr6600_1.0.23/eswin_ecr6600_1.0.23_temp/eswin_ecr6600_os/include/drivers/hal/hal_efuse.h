#ifndef HAL_EFUSE_H
#define HAL_EFUSE_H


int hal_efuse_read(unsigned int addr,unsigned int * value, unsigned int length);
int hal_efuse_write(unsigned int addr, unsigned int value, unsigned int mask);

int hal_efuse_write_mac(unsigned char * mac);
int hal_efuse_read_mac(unsigned char * mac);


#endif /*HAL_EFUSE_H*/


