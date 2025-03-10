/**
*@file	   hash.c
*@brief    This part of program is HASH algorithm by hardware
*@author   wang chao
*@data     2021.1.23
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#include <string.h>
#include "chip_smupd.h"
#include "chip_clk_ctrl.h"
#include "oshal.h"
#include "arch_irq.h"
#include "hash.h"
#include "encrypt_lock.h"

#define USE_ROM_INIT_PARA	1


#define UL64(x) x##ULL

#define GET_UINT32_BE(n,b,i)                            \
do {                                                    \
    (n) = ( (unsigned int) (b)[(i)    ] << 24 )             \
        | ( (unsigned int) (b)[(i) + 1] << 16 )             \
        | ( (unsigned int) (b)[(i) + 2] <<  8 )             \
        | ( (unsigned int) (b)[(i) + 3]       );            \
} while( 0 )


#define PUT_UINT32_BE(n,b,i)                            \
do {                                                    \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
} while( 0 )


/** HASH configure register*/
#define HASH_CFG_LAST		(1<<24)  ///< This processing message contains the end
#define HASH_CFG_SHA512	    (1<<2)   ///< Choose SHA256 algorithm
#define HASH_CFG_SHA256	    (1<<1)   ///< Choose SHA512 algorithm

/**<RAM1 is used by HASH*/
//#define DRV_HASH_RAM_ADDR   0x20207c 
#define DRV_HASH_RAM_VALUE  0X0 


/**
*@brief   HASH register structure 
*/
typedef  struct _T_HASH_REG_MAP
{
	volatile unsigned int Ctrl;			///< 0x00, control Register*/
	volatile unsigned int Cfg;			///< 0x04, Configure Register
	volatile unsigned int SR_1;			///< 0x08, status 1 Register
	volatile unsigned int SR_2;			///< 0x0C, status 2 Register
	volatile unsigned int Resv0[4];		///< 0x10~0x1C, Reserved register
	volatile unsigned int PcrLen[4];	///< 0x20~0x2C, length register
	volatile unsigned int Out[16];		///< 0x30~0x6C, hash output register
	volatile unsigned int In[16];		///< 0x70~0xAC, hash input register
	volatile unsigned int Ver;			///< 0xB0, version register
	volatile unsigned int Resv1[19];	///< 0xB4~0xFC, Reserved register
	volatile unsigned int MdIn[32];		///< 0x100~0x17C, Message input register
} T_HASH_REG_MAP;


#define HASH_REG_BASE		((T_HASH_REG_MAP *)MEM_BASE_HASH)

#define DRV_SHA256_BLOCK_SIZE		64
#define DRV_SHA512_BLOCK_SIZE		128

#define DRV_SHA256_INIT_VALUE_ADDR	0x0000AD38	///< Store the base address of sha256 initial value in ROM 
#define DRV_SHA512_INIT_VALUE_ADDR	0x0000AD58	///< Store the base address of sha512 initial value in ROM

#define DRV_SHA256_INIT_VALUE_SIZE	8	///< Eight 32-bit initial hashing values
#define DRV_SHA512_INIT_VALUE_SIZE	16	///< Sixteen 32-bit initial hashing values(Split eight 64-bit hash initial values)

#define DRV_SHA256_OUTPUT_BLOCK_SIZE	32	///< Generates a 32-byte array, or 256 bits 
#define DRV_SHA512_OUTPUT_BLOCK_SIZE	64	///< Generates a 64-byte array, or 512 bits


/**
@brief     Write the initial value of HASH to the hash input register
@details   For SHA256:In the first 8 prime numbers(2,3,5,7,11,13,17,19) in natural numbers,
           take the first 32 bits of the decimal part of the square root.
           For SHA512:In the first 8 prime numbers(2,3,5,7,11,13,17,19) in natural numbers,
           take the first 64 bits of the decimal part of the square root.
*/
static void drv_sha256_internal_starts(void)
{
	
	HASH_REG_BASE->SR_2 = 0;				  ///< clear completion status
	HASH_REG_BASE->Cfg = HASH_CFG_SHA256;	  ///< cfg sha256
	
#if USE_ROM_INIT_PARA
	for(int i = 0; i < DRV_SHA256_INIT_VALUE_SIZE; i++)
	{
		HASH_REG_BASE->In[i] = READ_REG(DRV_SHA256_INIT_VALUE_ADDR + i*4);  ///< Write the initial value of sha256 into the hash input register 
	}
#else
	HASH_REG_BASE->In[0] = 0x6A09E667;  ///< The first 32 bits of the square root of the prime number 2
	HASH_REG_BASE->In[1] = 0xBB67AE85;  ///< The first 32 bits of the square root of the prime number 3
	HASH_REG_BASE->In[2] = 0x3C6EF372;  ///< The first 32 bits of the square root of the prime number 5
	HASH_REG_BASE->In[3] = 0xA54FF53A;  ///< The first 32 bits of the square root of the prime number 7
	HASH_REG_BASE->In[4] = 0x510E527F;  ///< The first 32 bits of the square root of the prime number 11
	HASH_REG_BASE->In[5] = 0x9B05688C;  ///< The first 32 bits of the square root of the prime number 13
	HASH_REG_BASE->In[6] = 0x1F83D9AB;  ///< The first 32 bits of the square root of the prime number 17
	HASH_REG_BASE->In[7] = 0x5BE0CD19;  ///< The first 32 bits of the square root of the prime number 19
#endif
	
}


static void drv_sha512_internal_starts(void)
{
	
	HASH_REG_BASE->SR_2 = 0;				  ///< clear completion status
	HASH_REG_BASE->Cfg = HASH_CFG_SHA512;	  ///< cfg sha512
	
#if USE_ROM_INIT_PARA
	for(int i = 0; i < DRV_SHA512_INIT_VALUE_SIZE; i++)
	{
		HASH_REG_BASE->In[i] = READ_REG(DRV_SHA512_INIT_VALUE_ADDR + i*4);  ///< Write the initial value of sha512 into the hash input register 
	}
#else
	HASH_REG_BASE->In[0] = (unsigned int)(UL64(0x6A09E667F3BCC908) >> 32);
	HASH_REG_BASE->In[1] = (unsigned int)(UL64(0x6A09E667F3BCC908));
	HASH_REG_BASE->In[2] = (unsigned int)(UL64(0xBB67AE8584CAA73B) >> 32);
	HASH_REG_BASE->In[3] = (unsigned int)(UL64(0xBB67AE8584CAA73B));
	HASH_REG_BASE->In[4] = (unsigned int)(UL64(0x3C6EF372FE94F82B) >> 32);
	HASH_REG_BASE->In[5] = (unsigned int)(UL64(0x3C6EF372FE94F82B));
	HASH_REG_BASE->In[6] = (unsigned int)(UL64(0xA54FF53A5F1D36F1) >> 32);
	HASH_REG_BASE->In[7] = (unsigned int)(UL64(0xA54FF53A5F1D36F1));
	HASH_REG_BASE->In[8] = (unsigned int)(UL64(0x510E527FADE682D1) >> 32);
	HASH_REG_BASE->In[9] = (unsigned int)(UL64(0x510E527FADE682D1));
	HASH_REG_BASE->In[10] = (unsigned int)(UL64(0x9B05688C2B3E6C1F) >> 32);
	HASH_REG_BASE->In[11] = (unsigned int)(UL64(0x9B05688C2B3E6C1F));
	HASH_REG_BASE->In[12] = (unsigned int)(UL64(0x1F83D9ABFB41BD6B) >> 32);
	HASH_REG_BASE->In[13] = (unsigned int)(UL64(0x1F83D9ABFB41BD6B));
	HASH_REG_BASE->In[14] = (unsigned int)(UL64(0x5BE0CD19137E2179) >> 32);
	HASH_REG_BASE->In[15] = (unsigned int)(UL64(0x5BE0CD19137E2179));
#endif

}


static void drv_sha256_internal_process(const unsigned char *input, unsigned char output[32])
{
	unsigned int i, j, data;

	if (input)
	{
		for (i = 0, j = 0; i < DRV_SHA256_BLOCK_SIZE; i += 4, j += 1)
		{
			GET_UINT32_BE(data, input, i);
			HASH_REG_BASE->MdIn[j] = data;   ///< Write data to HASH_LP_MDIN register
		}
	}
	
	HASH_REG_BASE->Ctrl = 1;             ///< startup
	
	while (HASH_REG_BASE->SR_2 == 0);    ///< Read the status register to determine whether the operation is over
	HASH_REG_BASE->SR_2 = 0;             ///< Response complete signal

	if (output)
	{
		for (i = 0, j = 0; i < DRV_SHA256_OUTPUT_BLOCK_SIZE; i += 4, j += 1)
		{
			PUT_UINT32_BE(HASH_REG_BASE->Out[j], output, i);   ///< Read HASH_LP_OUT register
		}
	}
}

static void drv_sha256_internal_done(const unsigned char *input, unsigned int ilen, unsigned int total, unsigned char output[32])
{
	if (ilen % 4)
	{
		unsigned char buffer[DRV_SHA256_BLOCK_SIZE] = {0};
		memcpy(buffer, input, ilen);
		buffer[ilen++] = 0x80;

		if (ilen > 56)
		{
			drv_sha256_internal_process(buffer, NULL);
			memset(buffer, 0, DRV_SHA256_BLOCK_SIZE);
		}
		
		PUT_UINT32_BE( total<<3,  buffer, 60 );

		drv_sha256_internal_process(buffer, output);
	}
	else
	{
		if (ilen <= DRV_SHA256_BLOCK_SIZE) 
		{
			if (ilen < DRV_SHA256_BLOCK_SIZE)
			{
				HASH_REG_BASE->Cfg |= HASH_CFG_LAST; 
			}
			HASH_REG_BASE->PcrLen[0] = total;  ///< Write the message length into the HASH_LP_PCR_LEN register
			HASH_REG_BASE->PcrLen[1] = 0;

			drv_sha256_internal_process(input, output);
		}

		if (ilen == DRV_SHA256_BLOCK_SIZE) 
		{
			HASH_REG_BASE->Cfg |= HASH_CFG_LAST;
	
			drv_sha256_internal_process(NULL, output);
		}
	}
}



int drv_sha256_vector(unsigned int num_elem, const unsigned char * addr[], const unsigned int  * plength, unsigned char * output)
{
	unsigned char data_temp[DRV_SHA256_BLOCK_SIZE];
	int i, len_cur, len_elem;
	unsigned int offset = 0, index = 0, cnt = 0, total = 0;

	if ((addr == NULL) || (plength == NULL))
	{
		return HASH_RET_ENOINPUT;
	}

	if (*plength == 0)
	{
		return HASH_RET_ELEN;
	}

	if (output == NULL)
	{
		return HASH_RET_ENOOUTPUT;
	}

	for (i=0; i<num_elem; i++)
	{
		total += plength[i];
	}

	cnt = total/DRV_SHA256_BLOCK_SIZE;
	if (total % DRV_SHA256_BLOCK_SIZE)
	{
		cnt++;
	}

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_HASH);  ///< Enable HASH module clock
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_HASH_RAM_VALUE);   ///<RAM1 is used by HASH

	drv_sha256_internal_starts();  ///< The initial value of the SHA-256 algorithm is moved into the input register

	for (i = 0; i < cnt; i++)
	{
		if (offset)
		{
			len_cur = MIN(len_elem - offset, DRV_SHA256_BLOCK_SIZE);
			memcpy(data_temp, addr[index] + offset, len_cur);
			offset += len_cur;
			if (offset == len_elem)
			{
				index++;
			}
		}
		else
		{
			len_cur = 0;
		}

		while ((len_cur != DRV_SHA256_BLOCK_SIZE) && (index != num_elem))
		{
			len_elem = plength[index];

			if ((len_cur + len_elem) <= DRV_SHA256_BLOCK_SIZE)
			{
				offset = 0;
				memcpy(data_temp + len_cur, addr[index], len_elem);
				len_cur += len_elem;
				++index;
			}
			else
			{
				offset = DRV_SHA256_BLOCK_SIZE - len_cur;
				memcpy(data_temp + len_cur, addr[index], offset);
				len_cur = DRV_SHA256_BLOCK_SIZE;
			}
		}

		if ((i+1) == cnt)
		{
			drv_sha256_internal_done(data_temp, len_cur, total, output);
		}
		else
		{
			drv_sha256_internal_process(data_temp, NULL);
		}
	}

	chip_clk_disable((unsigned int)CLK_HASH);  ///< disable HASH module clock
	drv_encrypt_unlock();

	return 0;
}


/**
@brief      Calculate the hash value of SHA256   
@param[in]  *input:Pointer to store input data
@param[in]  ilen:Length of input data
@param[out] output[]:Array of SHA256 values    
*/
int drv_hash_sha256_ret(const unsigned char *input, unsigned int ilen, unsigned char output[32])
{
	unsigned int offset = 0, total = ilen;

	if (input == NULL)
	{
		return HASH_RET_ENOINPUT;
	}

	if (ilen == 0)
	{
		return HASH_RET_ELEN;
	}

	if (output == NULL)
	{
		return HASH_RET_ENOOUTPUT;
	}

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_HASH);  ///< Enable HASH module clock
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_HASH_RAM_VALUE);   ///<RAM1 is used by HASH

	drv_sha256_internal_starts();  ///< The initial value of the SHA-256 algorithm is moved into the input register

	while(1)
	{
		if (ilen <= DRV_SHA256_BLOCK_SIZE)
		{
			drv_sha256_internal_done(input+offset, ilen, total, output);
			break;
		}
		else
		{
			drv_sha256_internal_process(input+offset, NULL);
			ilen -= DRV_SHA256_BLOCK_SIZE;
			offset += DRV_SHA256_BLOCK_SIZE;
		}
	}

	chip_clk_disable((unsigned int)CLK_HASH);  ///< disable HASH module clock
	drv_encrypt_unlock();

	return HASH_RET_SUCCESS;
}



static void drv_sha512_internal_process(const unsigned char *input, unsigned char output[64])
{
	unsigned int i, j, data;

	if (input)
	{
		for (i = 0, j = 0; i < DRV_SHA512_BLOCK_SIZE; i += 4, j += 1)
		{
			GET_UINT32_BE(data, input, i);
			HASH_REG_BASE->MdIn[j] = data;   ///< Write data to HASH_LP_MDIN register
		}
	}
	
	HASH_REG_BASE->Ctrl = 1;             ///< startup
	
	while (HASH_REG_BASE->SR_2 == 0);    ///< Read the status register to determine whether the operation is over
	HASH_REG_BASE->SR_2 = 0;             ///< Response complete signal

	if (output)
	{
		for (i = 0, j = 0; i < DRV_SHA512_OUTPUT_BLOCK_SIZE; i += 4, j += 1)
		{
			PUT_UINT32_BE(HASH_REG_BASE->Out[j], output, i);   ///< Read HASH_LP_OUT register
		}
	}
}

static void drv_sha512_internal_done(const unsigned char *input, unsigned int ilen, unsigned int total, unsigned char output[64])
{
	if (ilen % 4)
	{
		unsigned char buffer[DRV_SHA512_BLOCK_SIZE] = {0};
		memcpy(buffer, input, ilen);
		buffer[ilen++] = 0x80;

		if (ilen >  112)
		{
			drv_sha512_internal_process(buffer, NULL);
			memset(buffer, 0, DRV_SHA512_BLOCK_SIZE);
		}
		
		PUT_UINT32_BE( total<<3,  buffer, 124 );

		drv_sha512_internal_process(buffer, output);
	}
	else
	{
		if (ilen <= DRV_SHA512_BLOCK_SIZE)
		{
			if (ilen < DRV_SHA512_BLOCK_SIZE)
			{
				HASH_REG_BASE->Cfg |= HASH_CFG_LAST;  
			}
			HASH_REG_BASE->PcrLen[0] = total;  ///< Write the message length into the HASH_LP_PCR_LEN register
			HASH_REG_BASE->PcrLen[1] = 0;

			drv_sha512_internal_process(input, output);
		}

		if (ilen == DRV_SHA512_BLOCK_SIZE)
		{
			HASH_REG_BASE->Cfg |= HASH_CFG_LAST;
			
			drv_sha512_internal_process(NULL, output);
		}
	}
}


int drv_sha512_vector(unsigned int num_elem, const unsigned char * addr[], const unsigned int  * plength, unsigned char * output)
{
	unsigned char data_temp[DRV_SHA512_BLOCK_SIZE] = {0};
	int i, len_cur, len_elem;
	unsigned int offset = 0, index = 0, cnt = 0, total = 0;

	if ((addr == NULL) || (plength == NULL))
	{
		return HASH_RET_ENOINPUT;
	}

	if (*plength == 0)
	{
		return HASH_RET_ELEN;
	}

	if (output == NULL)
	{
		return HASH_RET_ENOOUTPUT;
	}

	for (i=0; i<num_elem; i++)
	{
		total += plength[i];
	}

	cnt = total/DRV_SHA512_BLOCK_SIZE;
	if (total % DRV_SHA512_BLOCK_SIZE)
	{
		cnt++;
	}

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_HASH);  ///< Enable HASH module clock
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_HASH_RAM_VALUE);   ///<RAM1 is used by HASH

	drv_sha512_internal_starts();  ///< The initial value of the SHA-512 algorithm is moved into the input register

	for (i = 0; i < cnt; i++)
	{
		if (offset)
		{
			len_cur = MIN(len_elem - offset, DRV_SHA512_BLOCK_SIZE);
			memcpy(data_temp, addr[index] + offset, len_cur);
			offset += len_cur;
			if (offset == len_elem)
			{
				index++;
			}
		}
		else
		{
			len_cur = 0;
		}

		while ((len_cur != DRV_SHA512_BLOCK_SIZE) && (index != num_elem))
		{
			len_elem = plength[index];

			if ((len_cur + len_elem) <= DRV_SHA512_BLOCK_SIZE)
			{
				offset = 0;
				memcpy(data_temp + len_cur, addr[index], len_elem);
				len_cur += len_elem;
				++index;
			}
			else
			{
				offset = DRV_SHA512_BLOCK_SIZE - len_cur;
				memcpy(data_temp + len_cur, addr[index], offset);
				len_cur = DRV_SHA512_BLOCK_SIZE;
			}
		}

		if ((i+1) == cnt)
		{
			drv_sha512_internal_done(data_temp, len_cur, total, output);
		}
		else
		{
			drv_sha512_internal_process(data_temp, NULL);
		}
	}

	chip_clk_disable((unsigned int)CLK_HASH);  ///< disable HASH module clock
	drv_encrypt_unlock();

	return 0;
}


/**
@brief      Calculate the hash value of SHA512   
@param[in]  *input:Pointer to store input data
@param[in]  ilen:Length of input data
@param[out] output[]:Array of SHA512 values    
*/
int drv_hash_sha512_ret(const unsigned char *input, unsigned int ilen, unsigned char output[64])
{
	unsigned int offset = 0, total = ilen;

	if (input == NULL)
	{
		return HASH_RET_ENOINPUT;
	}

	if (ilen == 0)
	{
		return HASH_RET_ELEN;
	}

	if (output == NULL)
	{
		return HASH_RET_ENOOUTPUT;
	}

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_HASH);  ///< Enable HASH module clock
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_HASH_RAM_VALUE);   ///<RAM1 is used by HASH

	drv_sha512_internal_starts();  ///< The initial value of the SHA-512 algorithm is moved into the input register

	while(1)
	{
		if (ilen <= DRV_SHA512_BLOCK_SIZE)
		{
			drv_sha512_internal_done(input+offset, ilen, total, output);
			break;
		}
		else
		{
			drv_sha512_internal_process(input+offset, NULL);
			ilen -= DRV_SHA512_BLOCK_SIZE;
			offset += DRV_SHA512_BLOCK_SIZE;
		}
	}

	chip_clk_disable((unsigned int)CLK_HASH);  ///< disable HASH module clock
	drv_encrypt_unlock();

	return HASH_RET_SUCCESS;
}


