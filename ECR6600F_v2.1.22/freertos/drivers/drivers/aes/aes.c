/**
*@file	   aes.c
*@brief    This part of program is AES algorithm by hardware
*@author   zhang haojie
*@data     2021.1.23
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#include <string.h>
#include "chip_pinmux.h"
#include "chip_clk_ctrl.h"
#include "oshal.h"
#include "arch_irq.h"
#include "chip_memmap.h"
#include "aes.h"

#define AES_WORD_SIZE		32
#define AES_BLOCK_SIZE		4 	///< Configure 128 bits, 32 bits each time, and cycle four times
#define AES_WORD_OFFSET		4	///< Each input is 32 bits and contains four bytes 

#define AES_GET_UINT32(n,b,i)                       \
{                                                     			\
    (n) = ( (unsigned int) (b)[(i)    ]       )            \
        | ( (unsigned int) (b)[(i) + 1] <<  8 )         \
        | ( (unsigned int) (b)[(i) + 2] << 16 )        \
        | ( (unsigned int) (b)[(i) + 3] << 24 );       \
}

#define AES_PUT_UINT32(n,b,i)                                    \
{                                                               \
    (b)[(i)    ] = (unsigned char) ( ( (n)       ) & 0xFF );    \
    (b)[(i) + 1] = (unsigned char) ( ( (n) >>  8 ) & 0xFF );    \
    (b)[(i) + 2] = (unsigned char) ( ( (n) >> 16 ) & 0xFF );    \
    (b)[(i) + 3] = (unsigned char) ( ( (n) >> 24 ) & 0xFF );    \
}


/** AES control register*/
#define AES_REG_CTRL_TYPE_CBC	(0<<8)   ///< 0:select CBC algorithm
#define AES_REG_CTRL_TYPE_ECB	(1<<8)   ///< 1:select EBC algorithm

#define AES_REG_CTRL_BYTEORDER_LOW	(0<<6)   ///< Big endian mode selection
#define AES_REG_CTRL_BYTEORDER_HIGH	(1<<6)   ///< Little endian mode selection

#define AES_REG_CTRL_DATASTART_AUTO	(1<<4)   ///< this bit to start encryption/decryption when DATA3 is written through AES_DATA

#define AES_REG_CTRL_MODE_192	(1<<2)   ///< select AES192 mode

#define AES_REG_CTRL_MODE_256	(1<<1)   ///< select AES256 mode
#define AES_REG_CTRL_MODE_128	(0<<1)   ///< select AES128 mode

#define AES_REG_CTRL_ENCRYPTION	(1<<0)   ///< select Decryption mode
#define AES_REG_CTRL_DECRYPTION	(0<<0)   ///< select Encryption mode


/**
*@brief   AES register structure 
*/
typedef  volatile struct _T_AES_REG_MAP
{
    /**0x00, control register*/
	unsigned int Ctrl;			///< 0x00, control register 
	unsigned int Cmd;			///< 0x04, command register 
	unsigned int Status;		///< 0x08, status register 
	unsigned int IEnable;		///< 0x0C, interrupt enable register 
	unsigned int IFlag;		    ///< 0x10, interrupt flag register 
	unsigned int IFlagSet;		///< 0x14, interrupt flag set register 
	unsigned int IFlagClear;	///< 0x18, interrupt flag clear register 
	unsigned int Data;			///< 0x1C, data register 
	unsigned int IV;			///< 0x20, initialization vector register 
	unsigned int Resv0[3];		///< 0x24 ~ 0x2C, reserved register 
	unsigned int Key[8];		///< 0x30, key register 
	unsigned int LastKey[4];	///< 0x50, last key register 
	unsigned int DataCnt;		///< 0x60, data count register 
	unsigned int Dbg;			///< 0x64, debug register 
} T_AES_REG_MAP;

#define ASE_REG_BASE		((T_AES_REG_MAP *)MEM_BASE_AES)

static os_mutex_handle_t drv_aes_mutex;
//static T_AES_REG_MAP *ase_reg_base = (T_AES_REG_MAP *)MEM_BASE_AES;


//int drv_aes_init(void) __attribute__((section(".ilm_text_drv")));
void drv_aes_crypt(const unsigned char input[16], unsigned char output[16]) __attribute__((section(".ilm_text_drv")));


/**
@brief      AES module control register settings
@details    Configure AES_CTRL register, select 128,192 or 256 bit key, 
            select encryption or decryption mode, select EBC or CBC mode, 
            select endian mode and start mode
@param[in]  keybits:Key bits,Contains three:128,192,256
@param[in]  type:Choose ECB or CBC
@param[in]  mode:Choose encrypt or decrypt   
*/
static void aes_set_ctrl(unsigned int keybits, int type, int mode)
{
	/**cfg mode:keybits, encry, ecb, big*/
	unsigned int reg_aes_ctrl = 0;
    
	if (keybits == DRV_AES_KEYBITS_128)
	{
		reg_aes_ctrl |= AES_REG_CTRL_MODE_128;
	}
	else if (keybits == DRV_AES_KEYBITS_192)
	{
		reg_aes_ctrl |= AES_REG_CTRL_MODE_192 ;
	}
	else if (keybits == DRV_AES_KEYBITS_256)
	{
		reg_aes_ctrl |= AES_REG_CTRL_MODE_256;
	}

	reg_aes_ctrl |= mode | type | AES_REG_CTRL_BYTEORDER_HIGH | AES_REG_CTRL_DATASTART_AUTO;
	ASE_REG_BASE->Ctrl = reg_aes_ctrl;

}


/**
@brief      Configure AES_KEY register and AES_IV register
@details    For a 128-bit key, configure 4 registers, AES_KEYLA, AES_KEYLB, AES_KEYLC, and AES_KEYLD; 
            for a 192 or 256-bit key, configure 8 registers，such as AES_KEYLA, AES_KEYLB, AES_KEYLC, AES_KEYLD,
            AES_KEYHA, AES_KEYHB, AES_KEYHC, and AES_KEYHD.Configure the AES_IV register, start from the lower 32 bits, 
            and continuously configure the 128Bit value; EBC mode can skip this step.
@param[in]  *key:Enter key address
@param[in]  keybits:Key bits,Contains three:128,192,256
@param[in]  *iv:Enter iv address   
*/
static void aes_set_key_iv(const unsigned char *key, unsigned int keybits, const unsigned char *iv)
{
    unsigned int i, j, data;

	/**set key*/
	for (i = 0, j = 0; i < (keybits/AES_WORD_SIZE); i++, j++)
	{
		AES_GET_UINT32(data, key, i*AES_WORD_OFFSET);
		ASE_REG_BASE->Key[j] = data;
	}

	/**set iv*/
	if (iv)
	{
		for (i = 0; i < AES_BLOCK_SIZE ; i++)
		{
			AES_GET_UINT32(data, iv, i*AES_WORD_OFFSET);
			ASE_REG_BASE->IV= data;
		}
	}
}


/**
@brief      Set the configuration in ECB and decryption mode
@param[in]  *key:Enter key address
@param[in]  keybits:Key bits,Contains three:128,192,256
@param[in]  mode:Select AES mode, encryption or decryption 
*/
int drv_aes_ecb_setkey(const unsigned char *key, unsigned int keybits, int mode)
{
	if (key == NULL)
	{
		return AES_RET_ENOKEY;
	}

	if ((keybits != DRV_AES_KEYBITS_128) && (keybits != DRV_AES_KEYBITS_192) && (keybits != DRV_AES_KEYBITS_256))
	{
		return AES_RET_EINVAL;
	}
 
	if (mode == DRV_AES_MODE_ENC)
	{
		aes_set_ctrl(keybits, AES_REG_CTRL_TYPE_ECB, AES_REG_CTRL_ENCRYPTION);   ///< Configure AES_CTRL register
		aes_set_key_iv(key, keybits, NULL);   ///< Configure AES_KEY register and Do not set the IV value
	}
	else if (mode == DRV_AES_MODE_DEC)
	{
		aes_set_ctrl(keybits, AES_REG_CTRL_TYPE_ECB, AES_REG_CTRL_DECRYPTION);   ///< Configure AES_CTRL register
		aes_set_key_iv(key, keybits, NULL);   ///< Configure AES_KEY register and Do not set the IV value
	}
	else
	{
		return AES_RET_EMODE;
	}

	return AES_RET_SUCCESS;
}


/**
@brief      Set the configuration in CBC and encryption mode
@param[in]  *key:Enter key address
@param[in]  keybits:Key bits,Contains three:128,192,256
@param[in]  mode:Select AES mode, encryption or decryption 
@param[in]  *iv:Enter iv address
*/
int drv_aes_cbc_setkey(const unsigned char *key, unsigned int keybits, int mode, const unsigned char *iv)
{
	if (key == NULL)
	{
		return AES_RET_ENOKEY;
	}

	if ((keybits != DRV_AES_KEYBITS_128) && (keybits != DRV_AES_KEYBITS_192) && (keybits != DRV_AES_KEYBITS_256))
	{
		return AES_RET_EINVAL;
	}

	if (iv == NULL)
	{
	    	return AES_RET_EIV;
	}

	if (mode == DRV_AES_MODE_ENC)
	{
		aes_set_ctrl(keybits, AES_REG_CTRL_TYPE_CBC, AES_REG_CTRL_ENCRYPTION);	 ///< Configure AES_CTRL register
		aes_set_key_iv(key, keybits, iv);	///< Configure AES_KEY register and AES_IV register
	}
	else if (mode == DRV_AES_MODE_DEC)
	{
		aes_set_ctrl(keybits, AES_REG_CTRL_TYPE_CBC, AES_REG_CTRL_DECRYPTION);	 ///< Configure AES_CTRL register
		aes_set_key_iv(key, keybits, iv);	///< Configure AES_KEY register and AES_IV register
	}
	else
	{
		return AES_RET_EMODE;
	}

	return AES_RET_SUCCESS;
}

/**
@brief      Perform encryption and decryption work
@param[in]  input[]:An array of unencrypted and decrypted data
@param[in]  output[]:After encryption and decryption, the array to store the data
*/
void drv_aes_crypt(const unsigned char input[16], unsigned char output[16])
{
	unsigned int i, data;
	unsigned long flags = system_irq_save();

	/**Write unencrypted and decrypted data to the data register*/
	for (i = 0; i < AES_BLOCK_SIZE; i++)
	{
		AES_GET_UINT32(data, input, i*AES_WORD_OFFSET);	
		ASE_REG_BASE->Data = data;
	}

	while ((ASE_REG_BASE->Status) == 0);
	system_irq_restore(flags);
	
	while ((ASE_REG_BASE->Status) == 1);   ///< AES Running, This bit indicates that the AES module is running an encryption/decryption

	/**Output the data after encryption and decryption*/
	for (i = 0; i < AES_BLOCK_SIZE; i++)
	{
		data = ASE_REG_BASE->Data;
		AES_PUT_UINT32(data, output, i*AES_WORD_OFFSET);
		
	}

}

/**
@brief    Before using aes, enable aes clock
*/
int drv_aes_lock(void)
{
	if (os_mutex_lock(drv_aes_mutex, WAIT_FOREVER))
	{
		return AES_RET_ERROR;
	}
	else
	{
		chip_clk_enable((unsigned int)CLK_AES);

		return AES_RET_SUCCESS;
	}
}

/**
@brief    Before using aes, disable aes clock
*/
int drv_aes_unlock(void)
{
   	chip_clk_disable((unsigned int)CLK_AES);

	if (os_mutex_unlock(drv_aes_mutex))
	{
		return AES_RET_ERROR;
	}
	else
	{
		return AES_RET_SUCCESS;
	}
}

/**
@brief    aes hardware init
*/
int drv_aes_init(void)
{
	if (!drv_aes_mutex)
	{
		drv_aes_mutex = os_mutex_create();
		if (!drv_aes_mutex)
		{
			return AES_RET_ENOMEM;
		}
	}

	return AES_RET_SUCCESS;
}

