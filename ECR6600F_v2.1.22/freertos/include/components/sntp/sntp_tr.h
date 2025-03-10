/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name: sntp_tr.h   
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

#ifndef _SNTP_TR_H
#define _SNTP_TR_H


/****************************************************************************
* 	                                        Include files
****************************************************************************/


/****************************************************************************
* 	                                        Macros
****************************************************************************/
#define SNTP_RET_OK 0
#define SNTP_RET_ERR -1

#define SNTP_SERVER_DNS_NAME_MAX_LEN 32 
#define SNTP_DEFAULT_SERVER "pool.ntp.org"

#define TIME_ZONE_IN_NV_STRING_LEN 6
#define SNTP_PERIOD_IN_NV_STRING_LEN 11


/****************************************************************************
* 	                                        Types
****************************************************************************/

/****************************************************************************
* 	                                        Constants
****************************************************************************/

/****************************************************************************
* 	                                        Global  Variables
****************************************************************************/

/****************************************************************************
* 	                                        Function Prototypes
****************************************************************************/
unsigned int get_sntp_period(void);
int set_sntp_period(unsigned int period);
float get_timezone(void);
int set_timezone(float tz);
int set_servername(unsigned char idx, const char *s);
char *get_servername(unsigned char idx);
int set_system_time(unsigned int newtime);
void sntp_load_nv(void);
int set_utc_offset_time(int time);

#endif/*_SNTP_TR_H*/

