/**
 * @file system_config.c
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyafeng
 * @date 2021-6-17
 * @version V0.1
 * @par Copyright by http://eswin.com/.
 * @par History 1:
 *      Date:
 *      Version:
 *      Author:
 *      Modification:
 *
 * @par History 2:
 */


/*--------------------------------------------------------------------------
*												Include files
--------------------------------------------------------------------------*/
#include "easyflash.h"
#include "system_config.h"
#include "oshal.h"
#include <string.h>
#include "format_conversion.h"
#include "hal_flash.h"

/*--------------------------------------------------------------------------
* 	                                           	Local Macros
--------------------------------------------------------------------------*/
/** Description of the macro */
#define  MAC_LEN    (6)
/*--------------------------------------------------------------------------
* 	                                           	Local Types
--------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */

/*--------------------------------------------------------------------------
* 	                                           	Local Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                           	Local Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Variables
--------------------------------------------------------------------------*/
/**  Description of global variable  */

/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/
static unsigned char sys_mac[MAC_LEN];
unsigned char readmac_flag = 0;

int hal_system_set_mac(MAC_SAVE_LACATION type, char *mac)
{	
    unsigned char mac_to_otp[6] = {0};    
    int ret = -1;
    int i = 0;
    
	if (hex2num(mac[1])%2)
	{
		os_printf(LM_APP, LL_ERR, "mac error\n");		
		return ret;
	}

	if (type == MAC_SAVE_TO_AMT)
	{
		ret = amt_set_env_blob(STA_MAC, mac, strlen(mac));
	}
	else if(type == MAC_SAVE_TO_OTP)
	{
		for (i = 0; i < MAC_LEN; i++)
		{
			mac_to_otp[i] = hex2num(mac[i*3]) * 0x10 + hex2num(mac[i*3+1]);
		}
		ret = hal_flash_otp_write(MAC_ADDR_IN_OTP,(unsigned char *)mac_to_otp,6);		
	}
	else if(type == MAC_SAVE_TO_EFUSE)
	{
		//not support
		os_printf(LM_APP, LL_INFO, "not support now!\n");
	}
	else
	{
		os_printf(LM_APP, LL_INFO, "parameter error!\n");
	}

    if(ret == 0)
    {
        for (i = 0; i < MAC_LEN ; i++)
        {
            sys_mac[i] = hex2num(mac[i*3]) * 0x10 + hex2num(mac[i*3+1]);
        }        
        readmac_flag = 1;
    }
	return ret;
}
#if CONFIG_APP_AT_COMMAND
extern uint8_t g_temporary_mac[MAC_LEN];
extern uint8_t g_temporary_mac_flag;
#endif
int hal_system_get_sta_mac(unsigned char *mac)
{	
	int ret = -1;
	//1:amt
	int i = 0;
	char tmp_mac[18] = {0};

#if CONFIG_APP_AT_COMMAND
    if (g_temporary_mac_flag) {
        memcpy(mac, g_temporary_mac, MAC_LEN);
        return MAC_LEN;
    }
#endif

    if(readmac_flag == 1)
    {
         memcpy(mac, sys_mac, MAC_LEN);
         return MAC_LEN;
    }
	ret = amt_get_env_blob(STA_MAC, tmp_mac, 18, NULL); 
	if (ret > 0)
	{
		for (i = 0; i < MAC_LEN; i++)
		{
			mac[i] = hex2num(tmp_mac[i * 3]) * 0x10 + hex2num(tmp_mac[i * 3 + 1]);
		}	
		if(!(mac[0] & 0x1))
		{
			os_printf(LM_APP, LL_INFO, "mac in amt\n");
            memcpy(sys_mac, mac, MAC_LEN);
            readmac_flag = 1;
			return MAC_LEN;
		}
		else
		{
			ret = -1;
		}
	}
	//2:otp
	memset(mac,0,MAC_LEN);
	ret = hal_flash_otp_read(MAC_ADDR_IN_OTP,(unsigned char *)mac,MAC_LEN);	
	if((!ret) && (!(mac[0] & 0x1)))
	{
		os_printf(LM_APP, LL_INFO, "mac in otp\n");
        memcpy(sys_mac, mac, MAC_LEN);
        readmac_flag = 1;

		return MAC_LEN;
	}
	else
	{
		ret = -1;
	}
	//3:efuse
	//not support now
	
	//4:default
	memset(mac,0,MAC_LEN);
	memset(tmp_mac,0,18);
	#ifdef MAC_ADDR_STANDALONE
	//00:06:06:00:00:00
	memcpy(tmp_mac, (const char*)MAC_ADDR_STANDALONE, 18);
	for (i = 0; i < MAC_LEN; i++)
	{
		mac[i] = hex2num(tmp_mac[i * 3]) * 0x10 + hex2num(tmp_mac[i * 3 + 1]);
	}
	ret = MAC_LEN;
    memcpy(sys_mac, mac, MAC_LEN);
    readmac_flag = 1;
	os_printf(LM_APP, LL_INFO, "defalut mac\n");
	#endif
	
	return ret;
}
int hal_system_get_ap_mac(unsigned char *mac)
{
	int ret = -1;
	ret = hal_system_get_sta_mac(mac);
	if (ret != MAC_LEN)
	{
		return ret;
	}
	else
	{
		mac[5] ^=  0x01;
	}	
	return ret;
}
int hal_system_get_ble_mac(unsigned char *mac)
{
	int ret = -1;
	ret = hal_system_get_sta_mac(mac);
	if (ret != MAC_LEN)
	{
		return ret;
	}
	else
	{
		//ble mac = sta mac
		
	}	
	return ret;
}



int hal_system_get_config(const char *key, void *buff, unsigned int buff_len)
{	
	int ret = -1;
	
	if(memcmp(key,"amt",3) == 0)
	{
		ret = amt_get_env_blob(key + 3, buff, buff_len, NULL);
	}
	else if(memcmp(key,"mac",3) == 0)
	{
		ret = hal_system_get_sta_mac(buff);
	}
	else if(memcmp(key,"dev",3) == 0)
	{
		ret = develop_get_env_blob(key + 3, buff, buff_len, NULL);
	}
	else if(memcmp(key,"cus",3) == 0)
	{
		ret = customer_get_env_blob(key + 3, buff, buff_len, NULL);
	}
	else if(memcmp(key,"otp",3) == 0)
	{
		ret = 0;
		os_printf(LM_APP, LL_INFO, "otp\n");
	}
	else if(memcmp(key,"efu",3) == 0)
	{
		//reserved
		ret = 0;
		os_printf(LM_APP, LL_INFO, "efu\n");
	}
	
	return ret;
}

int hal_system_set_config(const char *key, void *buff, unsigned int buff_len)
{	
	int ret = -1;
	
	if(memcmp(key,"amt",3) == 0)
	{
		ret = amt_set_env_blob(key + 3, buff, buff_len);
	}
	else if(memcmp(key,"mac",3) == 0)
	{
		ret = amt_set_env_blob(key, buff, buff_len);		
		if (ret <= 0 && buff_len >= 18)
		{
			#ifdef MAC_ADDR_STANDALONE
    		memcpy(buff, (const char*)MAC_ADDR_STANDALONE, 18);
			ret = 18;
			#endif
		}
	}
	else if(memcmp(key,"dev",3) == 0)
	{
		ret = develop_set_env_blob(key + 3, buff, buff_len);
	}
	else if(memcmp(key,"cus",3) == 0)
	{
		ret = customer_set_env_blob(key + 3, buff, buff_len);
	}
	else if(memcmp(key,"otp",3) == 0)
	{
		ret = 0;
		os_printf(LM_APP, LL_INFO, "otp\n");
	}
	else if(memcmp(key,"efu",3) == 0)
	{
		//reserved
		ret = 0;
		os_printf(LM_APP, LL_INFO, "efu\n");
	}
	
	return ret;
}

int hal_system_del_config(const char *key)
{	
	int ret = -1;
	
	if(memcmp(key,"amt",3) == 0)
	{
		ret = amt_del_env(key + 3);
	}
	else if(memcmp(key,"mac",3) == 0)
	{
		ret = amt_del_env(key);	
	}
	else if(memcmp(key,"dev",3) == 0)
	{
		ret = develop_del_env(key + 3);
	}
	else if(memcmp(key,"cus",3) == 0)
	{
		ret = customer_del_env(key + 3);
	}
	else if(memcmp(key,"otp",3) == 0)
	{
		ret = 0;
		os_printf(LM_APP, LL_INFO, "otp\n");
	}
	else if(memcmp(key,"efu",3) == 0)
	{
		//reserved
		ret = 0;
		os_printf(LM_APP, LL_INFO, "efu\n");
	}
	
	return ret;
}


