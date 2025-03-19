/**
 * @file cmd_wifi.c
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-6-8
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
#include "system_def.h"
#include "system_config.h"
#include "format_conversion.h"


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

/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/

#ifdef CONFIG_WIRELESS_WPA_SUPPLICANT
extern sys_err_t wifi_stop_station(void);
static int cmd_wifi_stop_station(cmd_tbl_t *t, int argc, char *argv[])
{
	wifi_stop_station();
	return CMD_RET_SUCCESS;
}
CLI_CMD(wifi_stop_sta, cmd_wifi_stop_station,  "wifi_stop_sta", "wifi_stop_sta");


extern int wpa_cmd_receive(int vif_id, int argc, char *argv[]);
static int cmd_wpa(cmd_tbl_t *t, int argc, char *argv[])
{
	//os_printf(LM_CMD, LL_INFO, "--wpa cli argc %d, %s %s\n", argc, argv[0] , argc > 1 ? argv[1] : "");
	if (wpa_cmd_receive(0, argc, argv) == 0)
		return CMD_RET_SUCCESS;

	return CMD_RET_FAILURE;
}

static int cmd_wpb(cmd_tbl_t *t, int argc, char *argv[])
{
	//os_printf(LM_CMD, LL_INFO, "--wpb cli argc %d, %s %s\n", argc, argv[0] , argc > 1 ? argv[1] : "");
	if (wpa_cmd_receive(1, argc, argv) == 0)
		return CMD_RET_SUCCESS;

	return CMD_RET_FAILURE;
}

CLI_CMD(wpa, cmd_wpa,  "wpa cli",	 "wpa [command]");
CLI_CMD(wpb, cmd_wpb,  "wpb cli",	 "wpb [command]");
#endif  //CONFIG_WIRELESS_WPA_SUPPLICANT



#ifdef CONFIG_LWIP
extern void net_al_start_dhcp();
int cmd_dhcp_op(cmd_tbl_t *t, int argc, char *argv[])
{
	net_al_start_dhcp(argc, argv);
	return CMD_RET_SUCCESS;
}

CLI_CMD(dhcp, cmd_dhcp_op, "start/stop dhcp", "dhcp start/stop");


extern void net_al_start_dhcps();
int cmd_dhcps_op(cmd_tbl_t *t, int argc, char *argv[])
{
	net_al_start_dhcps(argc, argv);
	return CMD_RET_SUCCESS;
}

CLI_CMD(dhcps, cmd_dhcps_op, "start/stop dhcp", "dhcp start/stop");

extern void ping_run(char *cmd);
int cmd_ping(cmd_tbl_t *t, int argc, char *argv[])
{
    int i, len = 0;
    char buf[512] = {0};

    for (i = 1; i < argc; i++)
    {
        len += snprintf(buf + len, sizeof(buf) - len, "%s ", argv[i]);
    }

    ping_run(buf);
    return CMD_RET_SUCCESS;
}
CLI_CMD(ping, cmd_ping, "ping command", "ping [IP address]");

extern void wifi_ifconfig(char* cmd);
int cmd_ifconfig(cmd_tbl_t *t, int argc, char *argv[])
{
	int i, len = 0;
	char buf[64] = {0};

    for (i = 1; i < argc; i++)
    {
        len += snprintf(buf + len, sizeof(buf) - len, "%s ", argv[i]);
    }

	wifi_ifconfig(buf);
	return CMD_RET_SUCCESS;
}
CLI_CMD(ifconfig, cmd_ifconfig, "ifconfig command", "ifconfig -h");
#endif  //CONFIG_LWIP



#ifdef CONFIG_WIFI_PRODUCT_FHOST

int set_mac_func(cmd_tbl_t *t, int argc, char *argv[])
{
	if(argc != 2)
	{
		os_printf(LM_CMD, LL_ERR, "ARG NUM(: %d) IS ERROR!!\n", argc);
		return CMD_RET_FAILURE;
	}	
	
	if(hal_system_set_mac(MAC_SAVE_TO_AMT,argv[1]))
	{
		return CMD_RET_FAILURE;
	}	
	return CMD_RET_SUCCESS;
}
CLI_CMD(set_mac, set_mac_func, "set_mac 00:11:22:33:44:55", "set_mac 00:11:22:33:44:55");


int get_mac_func(cmd_tbl_t *t, int argc, char *argv[])
{
	//int i = 0;
	//char tmp_mac[18] = {0};
	unsigned char mac[6] = {0};
	if(!hal_system_get_config(STA_MAC,mac,sizeof(mac)))
	{
		return CMD_RET_FAILURE;
	}
	#if 0
	for (i=0; i<6; i++)
	{
		mac[i] = hex2num(tmp_mac[i*3]) * 0x10 + hex2num(tmp_mac[i*3+1]);
	}
	#endif
	os_printf(LM_CMD, LL_ERR, "sta mac = %02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	memset(mac,0,6);
	hal_system_get_sta_mac(mac);
	os_printf(LM_CMD, LL_INFO, "sta %02x:%02x:%02x:%02x:%02x:%02x\n,", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	memset(mac,0,6);
	hal_system_get_ap_mac(mac);
	os_printf(LM_CMD, LL_INFO, "ap %02x:%02x:%02x:%02x:%02x:%02x\n,", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	memset(mac,0,6);
	hal_system_get_ble_mac(mac);
	os_printf(LM_CMD, LL_INFO, "ble %02x:%02x:%02x:%02x:%02x:%02x\n,", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return CMD_RET_SUCCESS;
}
CLI_CMD(get_mac, get_mac_func, "get_mac", "get_mac");

#endif // CONFIG_WIFI_PRODUCT_FHOST




extern int call_wifi_dbg_func(int argc, char *argv[]);
int wifi_dbg_cmd(cmd_tbl_t *t, int argc, char *argv[])
{
    call_wifi_dbg_func(argc - 1, argv + 1);
    return CMD_RET_SUCCESS;
}
CLI_CMD(wd, wifi_dbg_cmd, "wifi debug", "wd [para]");

#ifdef NX_ESWIN_LMAC_TEST
	int modem_cmd(cmd_tbl_t *t, int argc, char *argv[]);
	CLI_CMD(lmt, modem_cmd, "", "");
	CLI_CMD(rf, modem_cmd, "", "");
	CLI_CMD(macbyp, modem_cmd, "", "");
#endif



