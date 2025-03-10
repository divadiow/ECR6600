/**
*@file	   aes.h
*@brief    This part of program is AES algorithm by hardware
*@author   zhang haojie
*@data     2021.1.23
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#ifndef DRV_AES_H
#define DRV_AES_H

#include "chip_configuration.h"


#define AES_RET_SUCCESS		0
#define AES_RET_ERROR			-1
#define AES_RET_ENOKEY			-2
#define AES_RET_ENOMEM			-3
#define AES_RET_EINVAL			-4
#define AES_RET_EMODE			-5
#define AES_RET_EIV				-6


/**Key bits,Contains three:128,192,256*/
#define DRV_AES_KEYBITS_128		128
#define DRV_AES_KEYBITS_192		192
#define DRV_AES_KEYBITS_256		256

/**Select AES mode, encryption or decryption*/
#define DRV_AES_MODE_ENC 1
#define DRV_AES_MODE_DEC 0


/**
@brief    aes hardware init
*/
int drv_aes_init(void);


/**
@brief    Before using aes, enable aes clock
*/
int drv_aes_lock(void);

/**
@brief    Before using aes, disable aes clock
*/
int drv_aes_unlock(void);


/**
@brief      Set the configuration in ECB and decryption mode
@param[in]  *key:Enter key address
@param[in]  keybits:Key bits,Contains three:128,192,256
@param[in]  mode:Select AES mode, encryption or decryption 
*/
int drv_aes_ecb_setkey(const unsigned char *key, unsigned int keybits, int mode);

/**
@brief      Set the configuration in CBC and encryption mode
@param[in]  *key:Enter key address
@param[in]  keybits:Key bits,Contains three:128,192,256
@param[in]  mode:Select AES mode, encryption or decryption 
@param[in]  *iv:Enter iv address
*/
int drv_aes_cbc_setkey(const unsigned char *key, unsigned int keybits, int mode, const unsigned char *iv);

/**
@brief      Perform encryption and decryption work
@param[in]  input[]:An array of unencrypted and decrypted data
@param[in]  output[]:After encryption and decryption, the array to store the data
*/
void drv_aes_crypt(const unsigned char input[16], unsigned char output[16]);


#endif /* DRV_AES_H */

