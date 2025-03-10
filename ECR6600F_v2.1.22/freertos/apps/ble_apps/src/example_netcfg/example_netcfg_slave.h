/**
 ****************************************************************************************
 *
 * @file example_netcfg_slave.h
 *
 * @brief slave netcfg example Header.
 *
 * @par Copyright (C):
 *      ESWIN 2015-2020
 *
 ****************************************************************************************
 */

#ifndef EXAMPLE_NETCFG_S_H
#define EXAMPLE_NETCFG_S_H
#include "system_wifi_def.h"
//the distribution network status--device respose
/*@TRACE*/	
typedef struct decive_rsp_net_status
{
	uint8_t type;
	uint8_t state;	
}decive_rsp_net_status;

///SSID issue--device response
/*@TRACE*/
typedef struct 
{
	uint8_t type;
	uint8_t rsp_result;
}device_rsp_ssid_issue;

///PASSWORD issue--device response
/*@TRACE*/
typedef struct device_rsp_pwd_issue
{
	uint8_t type;
	uint8_t rsp_result;
}device_rsp_pwd_issue;
	
///start distribution network--device response
/*@TRACE*/
typedef struct 
{
	uint8_t type;
	uint8_t state;
	uint8_t ssid_length;
	uint8_t ssid[WIFI_SSID_MAX_LEN];
}device_rsp_start_dis_network;
	
///stop distribute network--device response
typedef struct 
{
	uint8_t type;
	uint8_t rsp_result;
}device_rsp_stop_dis_network;

#endif /* EXAMPLE_NETCFG_S_H */

