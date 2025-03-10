/**
 * @file cmd_sntp.c
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-6-9
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
#include "cli.h"
#include "sntp.h"
#include "sntp_tr.h"
#include "hal_rtc.h"

/*--------------------------------------------------------------------------
* 	                                           	Local Macros
--------------------------------------------------------------------------*/
/** Description of the macro */

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
extern char server[SNTP_MAX_SERVERS][SNTP_SERVER_DNS_NAME_MAX_LEN + 1];
extern unsigned int sntp_period;//ms, Must not be beolw 60 seconds by specification
extern float timezone;

/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/
int set_sntp_fun(cmd_tbl_t *t, int argc, char *argv[])
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "on") == 0)
		{
			if(sntp_enabled())
			{		  
				os_printf(LM_CMD, LL_ERR, "SNTP is already enabled\n");
			}
			else
			{
				sntp_start();
			}
		}
		else if (strcmp(argv[1], "off") == 0)
		{
			if(sntp_enabled())
			{
				sntp_stop();
			}
			else
			{
				os_printf(LM_CMD, LL_ERR, "SNTP is already disabled\n");
			}
		}
		else if ((strcmp(argv[1], "tz") == 0) && argc == 3)
		{
			float tz = atof(argv[2]);
			set_timezone(tz);
		}
		else if(strcmp(argv[1], "period")==0)
		{
			unsigned int prd = atoi(argv[2]);
			set_sntp_period(prd*1000);
		}
		else if ((strcmp(argv[1], "server") == 0) && (argc == 4))
		{
			ip_addr_t addr = {0};
			unsigned char id = 0;

			id = atoi(argv[2]);
			if(id >= SNTP_MAX_SERVERS)
			{
				goto sntp_help;
			}

			if (strlen(argv[3]) > SNTP_SERVER_DNS_NAME_MAX_LEN)
			{
				os_printf(LM_CMD, LL_ERR, "server name can't beyond %d chars\n", SNTP_SERVER_DNS_NAME_MAX_LEN);
			}
			memset(server[id], 0, SNTP_SERVER_DNS_NAME_MAX_LEN);
			memcpy(server[id], argv[3], strlen(argv[3]));
			#ifdef CONFIG_IPV6
			if(ip4addr_aton(server[id], &addr.u_addr.ip4))
			#else
			if(ip4addr_aton(server[id], &addr))
			#endif
			{
				sntp_setserver(id, &addr);
			}
			else
			{
				set_servername(id, server[id]);
			}
		}
        else if ((strcmp(argv[1], "spectime") == 0) && (argc == 3))
        {
            int time = atoi(argv[2]);
            unsigned int time_sec;
            DATE_RTC cur_time = {0};
 
            set_utc_offset_time(time);
            time_sec = hal_rtc_get_time();
            hal_rtc_utctime_to_date(time_sec, &cur_time);
            os_printf(LM_APP, LL_INFO, "CurrentTime: week[%d] %4d-%02d-%02d %02d:%02d:%02d\n", 
                cur_time.wday, cur_time.year, cur_time.month, cur_time.day, cur_time.hour, cur_time.min, cur_time.sec);
        }
		else
		{
			goto sntp_help;
		}
	}
	else
	sntp_help:
	{
		unsigned char id;
		unsigned int ip = 0;
		const ip_addr_t *pip = NULL;
		os_printf(LM_CMD, LL_INFO, "sntp on|off|tz|period|server\n");
		os_printf(LM_CMD, LL_INFO, "================================\n");
		os_printf(LM_CMD, LL_INFO, "SNTP: %s\n", sntp_enabled()?"Enabled":"Disabled");
		os_printf(LM_CMD, LL_INFO, "TimeZone: %s%f\n", timezone > 0 ?"+":"", timezone);
		os_printf(LM_CMD, LL_INFO, "UpdatePeriod: %ds\n", sntp_period/1000);
		os_printf(LM_CMD, LL_INFO, "OperateMode: %s\n",sntp_getoperatingmode()?"listen":"poll");
		os_printf(LM_CMD, LL_INFO, "ServerMaxNumber: %d\n",SNTP_MAX_SERVERS);
		for(id=0; id < SNTP_MAX_SERVERS; id++)
		{
			pip = sntp_getserver(id);
			if(pip)
			{
				#ifdef CONFIG_IPV6
				ip = pip->u_addr.ip4.addr;
				#else
				ip = pip->addr;
				#endif
			}
			else   
			{
				ip = 0;
			}
			os_printf(LM_CMD, LL_INFO, "Server%d: %d.%d.%d.%d, %s, %s\n", id+1,ip&0xff,(ip>>8)&0xff,(ip>>16)&0xff,ip>>24,sntp_getservername(id),sntp_getreachability(id)?"success":"idle or try");
		}
		os_printf(LM_CMD, LL_INFO, "================================\n");
	}

	return CMD_RET_SUCCESS;
}

CLI_CMD(sntp, set_sntp_fun,  "sntp on|off|tz|period|server|spectime",	"sntp [command]");

