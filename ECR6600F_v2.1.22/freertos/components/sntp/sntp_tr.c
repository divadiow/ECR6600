/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name: sntp_tr.c   
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       v0.1
 * Author:        liuwei
 * Date:          2019-4-24
 * History 1:      
 *     Date: 
 *     Version:
 *     Author: 
 *     Modification:  
 * History 2: 
  ********************************************************************************/

/****************************************************************************
* 	                                           Include files
****************************************************************************/

#include <string.h>
#include "rtc.h"
#include "sntp.h"
#include "easyflash.h"
//#include "nv_config.h"
#include "cli.h"
#include "sntp_tr.h"
#include "hal_rtc.h"
#include "system_config.h"
/****************************************************************************
* 	                                           Local Macros
****************************************************************************/

/****************************************************************************
* 	                                           Local Types
****************************************************************************/

/****************************************************************************
* 	                                           Local Constants
****************************************************************************/
unsigned int sntp_period=SNTP_UPDATE_DELAY;//ms, Must not be beolw 60 seconds by specification
char server[SNTP_MAX_SERVERS][SNTP_SERVER_DNS_NAME_MAX_LEN + 1] = {0};

#define TIME_ZONE_MAX 12

#define SNTP_UPDATE_PERIOD_MIN_MS 60000
#define SNTP_UPDATE_PERIOD_MAX_MS 0x3FFFFFFF

int g_increase_time;
int g_sntp_sync;
float timezone = 0;

/****************************************************************************
* 	                                           Local Function Prototypes
****************************************************************************/

/****************************************************************************
* 	                                          Global Constants
****************************************************************************/

/****************************************************************************
* 	                                          Global Variables
****************************************************************************/

/****************************************************************************
* 	                                          Global Function Prototypes
****************************************************************************/

/****************************************************************************
* 	                                          Function Definitions
****************************************************************************/
int set_system_time(unsigned int newtime)
{
    int unix_time = (int)newtime;

    unix_time += (int)(timezone * ONE_HOUR2SEC) + g_increase_time;
    if (unix_time < 0)
    {
        return SNTP_RET_ERR;
    }

    hal_rtc_set_time(unix_time);
    os_printf(LM_APP, LL_INFO, "set time ok!timezone=%f,newtime=%d\r\n", timezone, unix_time);

    DATE_RTC cur_time = {0};
    int ret = hal_rtc_utctime_to_date(hal_rtc_get_time(), &cur_time);
    if (ret == 0)
    {
        os_printf(LM_APP, LL_INFO, "CurrentTime: week[%d] %4d-%02d-%02d %02d:%02d:%02d\n", 
            cur_time.wday,
            cur_time.year,
            cur_time.month,
            cur_time.day,
            cur_time.hour,
            cur_time.min,
            cur_time.sec);
    }
    else
    {
        os_printf(LM_APP, LL_INFO, "hal_rtc_utctime_to_date() return err[%d]!\r\n", ret);
    }

    g_sntp_sync = 1;

    return SNTP_RET_OK;
}

int set_timezone(float tz)
{
    unsigned int cur_time = 0;
    unsigned int irq = 0;
    char nv_str[TIME_ZONE_IN_NV_STRING_LEN] = {0};
    char *pv = NULL;

    if ((tz < -TIME_ZONE_MAX) || (tz > TIME_ZONE_MAX))
    {
        os_printf(LM_APP, LL_INFO, "timezone should be between -12 to 12\n");
        return SNTP_RET_ERR;
    }

    snprintf(nv_str, sizeof(nv_str), "%.1f", tz);
    if ((pv = strstr(nv_str, ".")) != NULL)
    {
        cur_time = atoi(pv + 1);
        if (cur_time != 5 && cur_time != 0)
        {
            os_printf(LM_APP, LL_INFO, "timezone should be between -12 to 12 with dot 5\n");
            return SNTP_RET_ERR;
        }
    }
    
    if (g_sntp_sync == 1)
    {
        cur_time = hal_rtc_get_time();
        if (timezone < 0)
        {
            if ((unsigned int)(cur_time - (unsigned int)(timezone * ONE_HOUR2SEC)) < cur_time)
            {
                os_printf(LM_APP, LL_INFO, "1:sntp set timezone error :%d\n", cur_time);
                return SNTP_RET_ERR;
            }
        }

        cur_time -= (unsigned int)(timezone * ONE_HOUR2SEC);

        if (tz > 0)
        {
            if ((unsigned int)(cur_time + (unsigned int)(tz * ONE_HOUR2SEC)) < cur_time)
            {
                os_printf(LM_APP, LL_INFO, "2:sntp set timezone error :%d\n", cur_time);
                return SNTP_RET_ERR;
            }
        }
    
        cur_time += (unsigned int)(tz * ONE_HOUR2SEC);
        hal_rtc_set_time(cur_time);
        irq = system_irq_save();
        timezone = tz;
        system_irq_restore(irq);
    }

    if (hal_system_set_config(CUSTOMER_NV_SNTP_TIMEZONE, nv_str, sizeof(nv_str)) < 0)
    {
        os_printf(LM_APP, LL_INFO, "customer nv sntp timezone set failed!\n");
    }
    //customer_set_env_blob(NV_SNTP_TIMEZONE, nv_str, sizeof(nv_str));
    os_printf(LM_APP, LL_INFO, "set_timezone, val: %s\n", nv_str);
    if (sntp_enabled())
    {   
        sntp_restart();
    }
    
    return SNTP_RET_OK;
}

float get_timezone(void)
{
    return timezone;
}

int set_sntp_period(unsigned int period_MS)
{
    char nv_str[SNTP_PERIOD_IN_NV_STRING_LEN] = {0};
    
    unsigned int period = 0;
    if (period_MS > SNTP_UPDATE_PERIOD_MAX_MS)
    {   
        period = SNTP_UPDATE_PERIOD_MAX_MS;
        os_printf(LM_APP, LL_INFO, "period too large ,set to max =%dms!\n", SNTP_UPDATE_PERIOD_MAX_MS);
    }
    else if (period_MS < SNTP_UPDATE_PERIOD_MIN_MS) 
    {
        period = SNTP_UPDATE_PERIOD_MIN_MS;
        os_printf(LM_APP, LL_INFO, "period too small ,set to min =%dms!\n", SNTP_UPDATE_PERIOD_MIN_MS);
    }
    else
    {
        period = period_MS;
    }
    sntp_period = period;

    snprintf(nv_str, sizeof(nv_str), "%d", period);
    //customer_set_env_blob(NV_SNTP_UPDATEPERIOD, nv_str, sizeof(nv_str));
    if (hal_system_set_config(CUSTOMER_NV_SNTP_UPDATEPERIOD, nv_str, sizeof(nv_str)) < 0)
    {
        os_printf(LM_APP, LL_INFO, "customer nv sntp update period set failed!\n");
    }

    if (sntp_enabled())
    {
        sntp_restart();
    }

    return SNTP_RET_OK;
}

unsigned int get_sntp_period(void)
{
	return sntp_period;
}

int set_servername(unsigned char idx, const char *s)
{
	EfErrCode ret = 0;
    if ((idx >= SNTP_MAX_SERVERS) || (s == NULL) || (strlen(s) == 0) || (strlen(s) > SNTP_SERVER_DNS_NAME_MAX_LEN))
    {
		os_printf(LM_APP, LL_INFO, "args err!\r\n");
        return SNTP_RET_ERR;
    }
    
   //ret = customer_set_env_blob(NV_SNTP_SERVER, s, strlen(s));
	ret = hal_system_set_config(CUSTOMER_NV_SNTP_SERVER,(void *)s,strlen(s));
    if (ret != EF_NO_ERR)
    {
        os_printf(LM_APP, LL_INFO, "set nv (%s) err!\r\n", CUSTOMER_NV_SNTP_SERVER);
        return SNTP_RET_ERR;
    }
    
    memcpy(server[idx], s, strlen(s));
    sntp_setservername(idx, server[idx]);
    return SNTP_RET_OK;
}

char *get_servername(unsigned char idx)
{
    if (idx >= SNTP_MAX_SERVERS)
    {
		os_printf(LM_APP, LL_INFO, "server id too large!\r\n");
        return NULL;
    }

	return server[idx];
}

void sntp_load_nv(void)
{
    int ret = 0;
    char time_zone_nv[TIME_ZONE_IN_NV_STRING_LEN] = {0};
    char period_nv[SNTP_PERIOD_IN_NV_STRING_LEN] = {0};

    ret = hal_system_get_config(CUSTOMER_NV_SNTP_TIMEZONE, time_zone_nv, sizeof(time_zone_nv));
    if (ret <= 0)
    {
        timezone = 0;
        os_printf(LM_APP, LL_INFO, "time zone NV read err, resotre default value = %f!\n", timezone);
    }
    else
    {
        timezone = atof(time_zone_nv);
        if ((timezone < -TIME_ZONE_MAX) || (timezone > TIME_ZONE_MAX))
        {
            timezone = 0;
            os_printf(LM_APP, LL_INFO, "time zone in NV is err, resotre default value =%f!\n", timezone);
        }
    }

    ret = hal_system_get_config(CUSTOMER_NV_SNTP_UPDATEPERIOD, period_nv, sizeof(period_nv));
    if (ret <= 0)
    {
        sntp_period = SNTP_UPDATE_DELAY;
        os_printf(LM_APP, LL_INFO, "sntp period NV read err, resotre default value = %d!\n", sntp_period);
    }
    else
    {
        sntp_period = atoi(period_nv);
        if ((sntp_period < SNTP_UPDATE_PERIOD_MIN_MS) || (sntp_period > SNTP_UPDATE_PERIOD_MAX_MS))
        {   
            sntp_period = SNTP_UPDATE_DELAY;
            os_printf(LM_APP, LL_INFO, "sntp period NV is err, resotre default value = %d!\n", sntp_period);
        }
    }

    ret = hal_system_get_config(CUSTOMER_NV_SNTP_SERVER, server[0], SNTP_SERVER_DNS_NAME_MAX_LEN);
    if(ret <= 0)
    {
        memset(server[0], 0, 32);
        memcpy(server[0], SNTP_DEFAULT_SERVER, sizeof(SNTP_DEFAULT_SERVER));
        os_printf(LM_APP, LL_INFO, "sntp server NV is err, resotre default value = %s!\n", SNTP_DEFAULT_SERVER);
    }

    sntp_setservername(0, server[0]);
    hal_system_get_config(CUSTOMER_NV_SNTP_INCREASETIME, &g_increase_time, sizeof(g_increase_time));
    os_printf(LM_APP, LL_INFO, "sntp increase time %d\n", g_increase_time);
}

static int sntp_increase_time(int time)
{
    unsigned int time_sec;
    DATE_RTC cur_time = {0};

    if (g_sntp_sync == 0)
    {
        os_printf(LM_APP, LL_ERR, "current time may not correct.\n");
        return SNTP_RET_ERR;
    }

    time_sec = hal_rtc_get_time();
    hal_rtc_utctime_to_date(time_sec, &cur_time);
    time_sec += time;
    if (hal_rtc_set_time(time_sec) < 0)
    {
        os_printf(LM_APP, LL_ERR, "rtc set time error.\n");
        return SNTP_RET_ERR;
    }

    if (sntp_enabled())
    {
        sntp_restart();
    }

    return SNTP_RET_OK;
}

int set_utc_offset_time(int time)
{
    int time_offset;

    if (time != ONE_HOUR2SEC && time != -ONE_HOUR2SEC && time != 0)
    {
        os_printf(LM_APP, LL_ERR, "time should be xONE_HOUR2SEC(3600/0/-3600).\n");
        return SNTP_RET_ERR;
    }

    if (g_increase_time == time)
    {
        os_printf(LM_APP, LL_ERR, "set same time %d.\n", time);
        return SNTP_RET_OK;
    }

    time_offset = time + (-1) * g_increase_time;

    if (sntp_increase_time(time_offset) == SNTP_RET_OK)
    {
        g_increase_time = time;
        hal_system_set_config(CUSTOMER_NV_SNTP_INCREASETIME, &g_increase_time, sizeof(g_increase_time));
        return SNTP_RET_OK;
    }

    return SNTP_RET_ERR;
}

