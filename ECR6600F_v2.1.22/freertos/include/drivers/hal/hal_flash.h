#ifndef HAL_FLASH_H
#define HAL_FLASH_H


int hal_flash_read(unsigned int addr, unsigned char * buff, unsigned int len);

int hal_flash_write(unsigned int addr, unsigned char * buff, unsigned int len);

int hal_flash_erase(unsigned int addr, unsigned int len);

int hal_flash_otp_read(unsigned int addr, unsigned char * buff, unsigned int len);

int hal_flash_otp_write(unsigned int addr, unsigned char * buff, unsigned int len);

//int hal_flash_otp_erase(unsigned int addr);

int hal_flash_otp_read_aes(unsigned int addr, unsigned char * buff, unsigned int len);

int hal_flash_otp_write_aes(unsigned int addr, unsigned char * buff, unsigned int len);

int hal_flash_otp_lock(unsigned char LB);




#endif /*HAL_FLASH_H*/


