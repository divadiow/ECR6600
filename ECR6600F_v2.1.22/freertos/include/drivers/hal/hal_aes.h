#ifndef HAL_AES_H
#define HAL_AES_H

int hal_aes_lock(void);
int hal_aes_unlock(void);
int hal_aes_ecb_setkey(const unsigned char *key, unsigned int keybits, int mode);
int hal_aes_cbc_setkey(const unsigned char *key, unsigned int keybits, int mode, const unsigned char *iv);
void hal_aes_crypt(const unsigned char input[16], unsigned char output[16]);



#endif /*HAL_AES_H*/


