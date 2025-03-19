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

typedef unsigned char				uint8_t;
typedef unsigned short              uint16_t;
#define __ARRAY_EMPTY

/*
 * GLOBAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */
#if 1
	struct mobile_req_network_status
	{
		uint8_t type;
	};
	//the distribution network status--device respose
	/*@TRACE*/	
	typedef struct decive_rsp_net_status
	{
		uint8_t type;
		uint16_t length;
		uint8_t state;
		uint8_t ssid_length;
		uint8_t SSID[__ARRAY_EMPTY];
			
	}decive_rsp_net_status;
	///SSID issue--mobile requestion
	/*@TRACE*/
	 struct mobile_req_ssid_issue
	{
		uint8_t type;
		uint8_t ssid_length;
		uint8_t SSID[__ARRAY_EMPTY];
	};
		
	///SSID issue--device response
	/*@TRACE*/
	typedef struct 
	{
		uint8_t type;
		uint8_t rsp_result;
	}device_rsp_ssid_issue;
		
	///PASSWORD issue--mobile requestion
	/*@TRACE*/
	struct mobile_req_pwd_issue
	{
		uint8_t type;
		uint8_t pwd_length;
		uint8_t pwd[__ARRAY_EMPTY];
	};
		
	///PASSWORD issue--device response
	/*@TRACE*/
	typedef struct device_rsp_pwd_issue
	{
		uint8_t type;
		uint8_t rsp_result;
	}device_rsp_pwd_issue;
		
	///Start distribution network--mobile requestion
	/*@TRACE*/
	struct moble_req_start_dis_network
	{
		uint8_t type;
		uint8_t wifi_netcfg_type;
	};
		
	///start distribution network--device response
	/*@TRACE*/
	typedef struct 
	{
		uint8_t type;
		uint16_t length;
		uint8_t state;
		uint8_t ssid_length;
		uint8_t ssid[__ARRAY_EMPTY];
	}device_rsp_start_dis_network;
		
	///stop distribution network--mobile requestion
	/*@TRACE*/
	struct mobile_req_stop_dis_network
	{
		uint8_t type;
		
	};
		
	///stop distribute network--device response
	typedef struct 
	{
		uint8_t type;
		uint8_t rsp_result;
	}device_rsp_stop_dis_network;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */


#endif

#endif /* EXAMPLE_NETCFG_S_H */

