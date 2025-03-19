#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "aes.h"

#include "FreeRTOS.h"
#include "task.h"
#include "oshal.h"
#include "hal_aes.h"


/**Definition of encryption and decryption types*/
#define AES_ECB  0
#define AES_CBC  1

/**The test key length is 128 bits*/
static const unsigned char utest_aes_key_128 [16] =
{
	0x0f, 0x15, 0x71, 0xc9, 0x47, 0xd9, 0xe8, 0x59, 0x0c, 0xb7, 0xad, 0xd6, 0xaf, 0x7f, 0x67, 0x98
};

/**The test key length is 192 bits*/
static const unsigned char utest_aes_key_192[24] =
{
	0x0f, 0x15, 0x71, 0xc9, 0x47, 0xd9, 0xe8, 0x59, 0x0c, 0xb7, 0xad, 0xd6, 0xaf, 0x7f, 0x67, 0x98,
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
};

/**The test key length is 256 bits*/
static const unsigned char utest_aes_key_256[32] =
{
	0x0f, 0x15, 0x71, 0xc9, 0x47, 0xd9, 0xe8, 0x59, 0x0c, 0xb7, 0xad, 0xd6, 0xaf, 0x7f, 0x67, 0x98,
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00
};

/**Test data 1*/
static const unsigned char utest_aes_plain_text_1[16] =
{
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};

/**Test data 2*/
static const unsigned char utest_aes_plain_text_2[16] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

/**Encrypted initialization vector*/
static const unsigned char utest_aes_iv_encrypt[16] =
{
	0xEB, 0xDF, 0x2B, 0x26, 0xD0, 0xC8, 0x54, 0x18, 0x29, 0x14, 0x7E, 0x82, 0x49, 0xB5, 0x43, 0x82 
};


/**In ECB mode,the key length is 128 bits, the encrypted data corresponding to test data 1*/
static const unsigned char utest_aes_cipher_text_1_ecb_128[16] =
{
	0xff, 0x0b, 0x84, 0x4a, 0x08, 0x53, 0xbf, 0x7c, 0x69, 0x34, 0xab, 0x43, 0x64, 0x14, 0x8f, 0xb9
};

/**In ECB mode,the key length is 192 bits, the encrypted data corresponding to test data 1*/
static const unsigned char utest_aes_cipher_text_1_ecb_192[16] =
{
	0x54, 0xE2, 0x49, 0x7B, 0x07, 0xC6, 0xA5, 0x3E, 0x55, 0x17, 0xA8, 0xAA, 0xAD, 0x49, 0x2E, 0x39 
};

/**In ECB mode,the key length is 256 bits, the encrypted data corresponding to test data 1*/
static const unsigned char utest_aes_cipher_text_1_ecb_256[16] =
{
	0xD2, 0xD0, 0xC2, 0xB2, 0x17, 0xB5, 0x9F, 0xF1, 0xBB, 0xD1, 0x24, 0x18, 0xD3, 0x7B, 0x0E, 0x9B 
};


/**In ECB mode,the key length is 128 bits, the encrypted data corresponding to test data 2*/
static const unsigned char utest_aes_cipher_text_2_ecb_128[16] =
{
	0xEB, 0xDF, 0x2B, 0x26, 0xD0, 0xC8, 0x54, 0x18, 0x29, 0x14, 0x7E, 0x82, 0x49, 0xB5, 0x43, 0x82 
};

/**In ECB mode,the key length is 192 bits, the encrypted data corresponding to test data 2*/
static const unsigned char utest_aes_cipher_text_2_ecb_192[16] =
{
	0xC5, 0x47, 0x7F, 0x79, 0x92, 0xA7, 0x3A, 0x60, 0x98, 0xFD, 0x76, 0xF0, 0x75, 0x9D, 0x0E, 0xEB
};

/**In ECB mode,the key length is 256 bits, the encrypted data corresponding to test data 2*/
static const unsigned char utest_aes_cipher_text_2_ecb_256[16] =
{
	0xCB, 0xE2, 0x26, 0xB4, 0x7B, 0x38, 0x9D, 0x84, 0x86, 0x62, 0x7C, 0x02, 0x7D, 0x71, 0x45, 0x59
};


/**In CBC mode,the key length is 128 bits, the encrypted data corresponding to test data 1*/
static const unsigned char utest_aes_cipher_text_1_cbc_128[16] =
{
	0xA0, 0x5C, 0x1B, 0xEE, 0xD6, 0x0E, 0x11, 0xA9, 0x2F, 0xD0, 0xBF, 0x52, 0xF3, 0xD2, 0xC8, 0x14
};

/**In CBC mode,the key length is 192 bits, the encrypted data corresponding to test data 1*/
static const unsigned char utest_aes_cipher_text_1_cbc_192[16] =
{
	0x99, 0x9C, 0x35, 0xAA, 0x76, 0x3E, 0xB1, 0xE5, 0x4F, 0xF8, 0xCE, 0x98, 0xA2, 0xEB, 0x6C, 0x0B 
};

/**In CBC mode,the key length is 256 bits, the encrypted data corresponding to test data 1*/
static const unsigned char utest_aes_cipher_text_1_cbc_256[16] =
{
	0xAF, 0xBB, 0x5D, 0x4F, 0x4E, 0x5D, 0x8B, 0x5D, 0x59, 0x9D, 0x6F, 0xE2, 0x5B, 0x0F, 0xF5, 0xBA
};


/**In CBC mode,the key length is 128 bits, the encrypted data corresponding to test data 2*/
static const unsigned char utest_aes_cipher_text_2_cbc_128[16] =
{
	0x8D, 0xD6, 0x98, 0x09, 0xBF, 0xE5, 0xAC, 0x09, 0xBE, 0x0B, 0x96, 0xE4, 0xF4, 0x69, 0x3B, 0x33
};

/**In CBC mode,the key length is 192 bits, the encrypted data corresponding to test data 2*/
static const unsigned char utest_aes_cipher_text_2_cbc_192[16] =
{
	0x27, 0xE6, 0x9E, 0x28, 0x1D, 0x5C, 0xDD, 0xA9, 0x93, 0xD6, 0x35, 0x92, 0x12, 0x1A, 0xC0, 0xEC
};

/**In CBC mode,the key length is 256 bits, the encrypted data corresponding to test data 2*/
static const unsigned char utest_aes_cipher_text_2_cbc_256[16] =
{
	0x38, 0x55, 0xAD, 0xF7, 0x12, 0x2F, 0x7A, 0xC8, 0xBA, 0xAE, 0x9B, 0xD0, 0x59, 0x4F, 0x77, 0xE1
};
	


static int utest_aes_lock(cmd_tbl_t *t, int argc, char *argv[])
{
	if (hal_aes_lock() == 0)
	{
		os_printf(LM_CMD,LL_INFO,">> unit test aes, aes lock pass!\r\n");
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,">> unit test aes, aes lock failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_aes, lock, utest_aes_lock, "unit test aes lock", "ut_aes lock");

static int utest_aes_unlock(cmd_tbl_t *t, int argc, char *argv[])
{
	if (hal_aes_unlock() == 0)
	{
		os_printf(LM_CMD,LL_INFO,">> unit test aes, aes unlock pass!\r\n");
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,">> unit test aes, aes unlock failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_aes, unlock, utest_aes_unlock, "unit test aes unlock", "ut_aes unlock");



/**
@brief     AES encryption test
@details   First, in the encryption mode, determine whether it is ECB or CBC.Then judge it is 128, 192, 156 bits.
           Finally, encrypt the two sets of test data and verify that they are correct
@return    0--End of calculation  
*/
static int utest_aes_encryption(cmd_tbl_t *t, int argc, char *argv[])
{  
	unsigned char cbuf[16];
	int mode, keybits;

	if (argc >= 3)
	{
		mode = (int)strtoul(argv[1], NULL, 0);
		keybits = (int)strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,">> unit test aes, err: no enough argt!\r\n");
		return 0;
	}

	if (mode == AES_ECB)
	{
    	switch (keybits)
    	{
        	case 128:
				hal_aes_ecb_setkey(utest_aes_key_128, DRV_AES_KEYBITS_128, DRV_AES_MODE_ENC); 
				
				hal_aes_crypt(utest_aes_plain_text_1, cbuf);	
				if (memcmp(utest_aes_cipher_text_1_ecb_128, cbuf, 16) == 0)
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_128 data-1 pass!\r\n");
				}
				else
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_128 data-1 failed!\r\n");
				}
	
				hal_aes_crypt(utest_aes_plain_text_2, cbuf);
				if (memcmp(utest_aes_cipher_text_2_ecb_128, cbuf, 16) == 0)
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_128 data-2 pass!\r\n\r\n");
				}
				else
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_128 data-2 failed!\r\n\r\n");
				}
				
				break;
				
			case 192:
				hal_aes_ecb_setkey(utest_aes_key_192, DRV_AES_KEYBITS_192, DRV_AES_MODE_ENC);

				hal_aes_crypt(utest_aes_plain_text_1, cbuf);  
            	if (memcmp(utest_aes_cipher_text_1_ecb_192, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_192 data-1 pass!\r\n");
            	}
            	else
            	{
               		os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_192 data-1 failed!\r\n");
            	}	
		
				hal_aes_crypt(utest_aes_plain_text_2, cbuf);	
				if (memcmp(utest_aes_cipher_text_2_ecb_192, cbuf, 16) == 0)
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_192 data-2 pass!\r\n\r\n");
				}
				else
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_192 data-2 failed!\r\n\r\n");
				}		
				
				break;
				
			case 256:
				hal_aes_ecb_setkey(utest_aes_key_256, DRV_AES_KEYBITS_256, DRV_AES_MODE_ENC);
			
				hal_aes_crypt(utest_aes_plain_text_1, cbuf);
            	if (memcmp(utest_aes_cipher_text_1_ecb_256, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_256 data-1 pass!\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_256 data-1 failed!\r\n");
            	}   		
			
	        	hal_aes_crypt(utest_aes_plain_text_2, cbuf);
            	if (memcmp(utest_aes_cipher_text_2_ecb_256, cbuf, 16) == 0)
            	{
                	os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_256 data-2 pass!\r\n\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_ecb_encrypt keybit_256 data-2 failed!\r\n\r\n");
            	}
				
				break;

			default:
				break;
    	}		
	}

	else if (mode == AES_CBC)
	{
    	switch (keybits)
    	{
        	case 128:	
				hal_aes_cbc_setkey(utest_aes_key_128, DRV_AES_KEYBITS_128, DRV_AES_MODE_ENC, utest_aes_iv_encrypt);
  			
				hal_aes_crypt(utest_aes_plain_text_1, cbuf);
        		if (memcmp(utest_aes_cipher_text_1_cbc_128, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_128 data-1 pass!\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_128 data-1 failed!\r\n");
            	}   
		
				hal_aes_crypt(utest_aes_plain_text_2, cbuf);
           		if (memcmp(utest_aes_cipher_text_2_cbc_128, cbuf, 16) == 0)
     	    	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_128 data-2 pass!\r\n\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_128 data-2 failed!\r\n\r\n");
            	}		
				
				break;
				
			case 192:
				hal_aes_cbc_setkey(utest_aes_key_192, DRV_AES_KEYBITS_192, DRV_AES_MODE_ENC, utest_aes_iv_encrypt);
			
				hal_aes_crypt(utest_aes_plain_text_1, cbuf);
            	if (memcmp(utest_aes_cipher_text_1_cbc_192, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_192 data-1 pass!\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_192 data-1 failed!\r\n");
            	}   	
			
				hal_aes_crypt(utest_aes_plain_text_2, cbuf);
            	if (memcmp(utest_aes_cipher_text_2_cbc_192, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_192 data-2 pass!\r\n\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_192 data-2 failed!\r\n\r\n");
           		}	
				
				break;
				
			case 256:
				hal_aes_cbc_setkey(utest_aes_key_256, DRV_AES_KEYBITS_256, DRV_AES_MODE_ENC, utest_aes_iv_encrypt);
			
				hal_aes_crypt(utest_aes_plain_text_1, cbuf);
            	if (memcmp(utest_aes_cipher_text_1_cbc_256, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_256 data-1 pass!\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_256 data-1 failed!\r\n");
            	}   
		
	        	hal_aes_crypt(utest_aes_plain_text_2, cbuf);
            	if (memcmp(utest_aes_cipher_text_2_cbc_256, cbuf, 16) == 0)
            	{
                	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_256 data-2 pass!\r\n\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_encrypt keybit_256 data-2 failed!\r\n\r\n");
            	}
				
				break;

			default:
				break;
    	}	
	} 

	return 0;
}

CLI_SUBCMD(ut_aes, enc, utest_aes_encryption, "unit test aes enc", "ut_aes enc [mode] [keybits]");



/**
@brief     AES decryption test
@details   First, in the decryption mode, determine whether it is ECB or CBC.Then judge it is 128, 192, 156 bits.
           Finally, decrypt the two sets of test data and verify that they are correct
@return    0--End of calculation  
*/
static int utest_aes_decryption(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned char cbuf[16];
	int mode, keybits;

	if (argc >= 3)
	{
		mode = (int)strtoul(argv[1], NULL, 0);
		keybits = (int)strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test aes, err: no enough argt!\r\n");
		return 0;
	}

	if (mode == AES_ECB)
	{
    	switch (keybits)
    	{
        	case 128:
				hal_aes_ecb_setkey(utest_aes_key_128, DRV_AES_KEYBITS_128, DRV_AES_MODE_DEC);
			
				hal_aes_crypt(utest_aes_cipher_text_1_ecb_128, cbuf);	
				if (memcmp(utest_aes_plain_text_1, cbuf, 16) == 0)
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_128 data-1 pass!\r\n");
				}
				else
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_128 data-1 failed!\r\n");
				}
		
				hal_aes_crypt(utest_aes_cipher_text_2_ecb_128, cbuf);
				if (memcmp(utest_aes_plain_text_2, cbuf, 16) == 0)
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_128 data-2 pass!\r\n\r\n");
				}
				else
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_128 data-2 failed!\r\n\r\n");
				}		
				
				break;
				
			case 192:
				hal_aes_ecb_setkey(utest_aes_key_192, DRV_AES_KEYBITS_192, DRV_AES_MODE_DEC);
			
				hal_aes_crypt(utest_aes_cipher_text_1_ecb_192, cbuf);  
            	if (memcmp(utest_aes_plain_text_1, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_192 data-1 pass!\r\n");
            	}
            	else
            	{
                	os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_192 data-1 failed!\r\n");
            	}
			
				hal_aes_crypt(utest_aes_cipher_text_2_ecb_192, cbuf);	
				if (memcmp(utest_aes_plain_text_2, cbuf, 16) == 0)
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_192 data-2 pass!\r\n\r\n");
				}
				else
				{
					os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_192 data-2 failed!\r\n\r\n");
				}		
				
				break;
				
			case 256:
				hal_aes_ecb_setkey(utest_aes_key_256, DRV_AES_KEYBITS_256, DRV_AES_MODE_DEC);
				
				hal_aes_crypt(utest_aes_cipher_text_1_ecb_256, cbuf);
            	if (memcmp(utest_aes_plain_text_1, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_256 data-1 pass!\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_256 data-1 failed!\r\n");
            	}   
			
	        	hal_aes_crypt(utest_aes_cipher_text_2_ecb_256, cbuf);
            	if (memcmp(utest_aes_plain_text_2, cbuf, 16) == 0)
            	{
               		os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_256 data-2 pass!\r\n\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_ecb_decrypt keybit_256 data-2 failed!\r\n\r\n");
           		}
				
				break;

			default:
				break;
    	}	
	}

	else if (mode == AES_CBC)
	{
    	switch (keybits)
    	{
        	case 128:	
				hal_aes_cbc_setkey(utest_aes_key_128, DRV_AES_KEYBITS_128, DRV_AES_MODE_DEC, utest_aes_iv_encrypt);
  				
				hal_aes_crypt(utest_aes_cipher_text_1_cbc_128, cbuf);
        		if (memcmp(utest_aes_plain_text_1, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_128 data-1 pass!\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_128 data-1 failed!\r\n");
            	} 
				
				hal_aes_crypt(utest_aes_cipher_text_2_cbc_128, cbuf);
          		if (memcmp(utest_aes_plain_text_2, cbuf, 16) == 0)
     	    	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_128 data-2 pass!\r\n\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_128 data-2 failed!\r\n\r\n");
            	}	
				
				break;
				
			case 192:
				hal_aes_cbc_setkey(utest_aes_key_192, DRV_AES_KEYBITS_192, DRV_AES_MODE_DEC, utest_aes_iv_encrypt);
				
				hal_aes_crypt(utest_aes_cipher_text_1_cbc_192, cbuf);
            	if (memcmp(utest_aes_plain_text_1, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_192 data-1 pass!\r\n");
           		}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_192 data-1 failed!\r\n");
            	}   	
				
				hal_aes_crypt(utest_aes_cipher_text_2_cbc_192, cbuf);
            	if (memcmp(utest_aes_plain_text_2, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_192 data-2 pass!\r\n\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_192 data-2 failed!\r\n\r\n");
           		}	
				
				break;
				
			case 256:
				hal_aes_cbc_setkey(utest_aes_key_256, DRV_AES_KEYBITS_256, DRV_AES_MODE_DEC, utest_aes_iv_encrypt);
				
				hal_aes_crypt(utest_aes_cipher_text_1_cbc_256, cbuf);
            	if (memcmp(utest_aes_plain_text_1, cbuf, 16) == 0)
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_256 data-1 pass!\r\n");
            	}
            	else
            	{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_256 data-1 failed!\r\n");
            	}   
			
	        	hal_aes_crypt(utest_aes_cipher_text_2_cbc_256, cbuf);
            	if (memcmp(utest_aes_plain_text_2, cbuf, 16) == 0)
            	{
                	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_256 data-2 pass!\r\n\r\n");
            	}
            	else
           		{
	            	os_printf(LM_CMD,LL_INFO,">> aes_cbc_decrypt keybit_256 data-2 failed!\r\n\r\n");
            	}
				
				break;

			default:
				break;
    	}
	}
	

	return 0;
}

CLI_SUBCMD(ut_aes, dec, utest_aes_decryption, "unit test aes dec", "ut_aes dec [mode] [keybits]");


CLI_CMD(ut_aes, NULL, "unit test aes", "test_aes");














