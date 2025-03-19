#ifndef __TUYA_OS_ADAPT_HOSTAPD_C__
#define __TUYA_OS_ADAPT_HOSTAPD_C__

#include "tuya_hostapd_driver.h"
#include "tuya_hostapd_ioctl.h"

void cli_printf(const char *f, ...);

//#define HAPD_IOCTL_DBG(fmt, args...) cli_printf(fmt, args)
#define HAPD_IOCTL_DBG(fmt, args...)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef int (*TY_IOCTL_FUNC)(int vif_index, TY_WLAN_PARAMS_S *params);

typedef struct {
    int cmd;
    TY_IOCTL_FUNC func; 
} TY_IOCTL_FUNC_S;

extern int fhost_hostapd_ioctl_msg_process(int dev, int vif_index, unsigned int cmd, unsigned long arg);
extern int fhost_hostapd_ioctl_set_process(int dev, int vif_index, unsigned int cmd, unsigned long arg);
extern int fhost_hostapd_ioctl_get_process(int dev, int vif_index, unsigned int cmd, unsigned long arg);

static int tuya_os_adapt_wlan_vif_init(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_vif_deinit(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_scan_req(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_scan_cancel(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_get_scan_results(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_assoc_req(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_disconn_req(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_reg_mgmt_cb(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_reg_event_cb(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_sta_flags(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_sta_conn_state(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_get_conn_ap_stats(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_ap_bcn(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_ssid(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_add_sta(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_remove_sta(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_gen_elem(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_send_mlme(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_get_sta_info(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_clear_stats(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_key(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_get_key(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_privacy(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_clear_stations(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_get_bss_info(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_external_auth_status(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_get_wpa_capacity(int vif_index, TY_WLAN_PARAMS_S *params)
{
    params->u.value = 0;
    return 0;
}

static int tuya_os_adapt_sta_inact_settings(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_msg_process(0, vif_index, params->cmd, (unsigned long)params); 
}

static int tuya_os_adapt_wlan_set_channel(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_get_channel(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_get_process(0, vif_index, params->cmd, (unsigned long)params);
}

//tuya runtime mode 
static int tuya_os_adapt_wlan_set_power(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_tx_fixrate(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_gi(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_bw(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_rts(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_start_statistic(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_stop_statistic(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_set_clear_statistic(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_set_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_get_tx_status(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_get_process(0, vif_index, params->cmd, (unsigned long)params);
}

static int tuya_os_adapt_wlan_get_rx_status(int vif_index, TY_WLAN_PARAMS_S *params)
{
    return fhost_hostapd_ioctl_get_process(0, vif_index, params->cmd, (unsigned long)params);
}

TY_IOCTL_FUNC_S wlan_setparam[] = {
    {TUYA_PARAM_CHANNEL, tuya_os_adapt_wlan_set_channel},
    {TUYA_PARAM_PRIVACY_INVOKED, tuya_os_adapt_wlan_set_privacy},
    {TUYA_PARAM_SET_POWER, tuya_os_adapt_wlan_set_power},
    {TUYA_PARAM_SET_TX_FIXRATE, tuya_os_adapt_wlan_set_tx_fixrate},
    {TUYA_PARAM_SET_GI, tuya_os_adapt_wlan_set_gi},
    {TUYA_PARAM_SET_BW, tuya_os_adapt_wlan_set_bw},
    {TUYA_PARAM_SET_RTS, tuya_os_adapt_wlan_set_rts},
    {TUYA_PARAM_START_STATISTIC, tuya_os_adapt_wlan_set_start_statistic},
    {TUYA_PARAM_STOP_STATISTIC, tuya_os_adapt_wlan_set_stop_statistic},
    {TUYA_PARAM_CLEAR_STATISTIC, tuya_os_adapt_wlan_set_clear_statistic},
};

TY_IOCTL_FUNC_S wlan_getparam[] = {
    {TUYA_PARAM_CHANNEL, tuya_os_adapt_wlan_get_channel},
    {TUYA_PARAM_GET_TX_STATUS, tuya_os_adapt_wlan_get_tx_status},
    {TUYA_PARAM_GET_RX_STATUS, tuya_os_adapt_wlan_get_rx_status},
};

TY_IOCTL_FUNC_S hostapd_ioctl[] = {
    {TUYA_HOSTAPD_WPA_INIT_VIF, tuya_os_adapt_wlan_vif_init},
    {TUYA_HOSTAPD_WPA_DEINIT_VIF, tuya_os_adapt_wlan_vif_deinit},
    {TUYA_HOSTAPD_SCAN_REQ, tuya_os_adapt_wlan_scan_req},
    {TUYA_HOSTAPD_SCAN_CANCEL, tuya_os_adapt_wlan_scan_cancel},
    {TUYA_HOSTAPD_GET_SCAN_RESULT, tuya_os_adapt_wlan_get_scan_results},
    {TUYA_HOSTAPD_ASSOC_REQ, tuya_os_adapt_wlan_assoc_req},
    {TUYA_HOSTAPD_SCAN_ALL_AP, tuya_os_adapt_wlan_scan_req},
    {TUYA_HOSTAPD_SCAN_SPECIAL_AP, tuya_os_adapt_wlan_scan_req},
    {TUYA_HOSTAPD_DISCONN_REQ, tuya_os_adapt_wlan_disconn_req},
    {TUYA_HOSTAPD_MGMT_HOOK_CALLBACK, tuya_os_adapt_wlan_reg_mgmt_cb},
    {TUYA_HOSTAPD_EVENT_HOOK_CALLBACK, tuya_os_adapt_wlan_reg_event_cb},
    {TUYA_HOSTAPD_SET_FLAGS_STA, tuya_os_adapt_wlan_set_sta_flags},
    {TUYA_HOSTAPD_SET_OPER_STATE, tuya_os_adapt_wlan_set_sta_conn_state},
    {TUYA_HOSTAPD_GET_SOFTAP_STATS, tuya_os_adapt_wlan_get_conn_ap_stats},
    {TUYA_HOSTAPD_SET_AP_BCN, tuya_os_adapt_wlan_set_ap_bcn},
    {TUYA_HOSTAPD_SET_SSID, tuya_os_adapt_wlan_set_ssid},
    {TUYA_HOSTAPD_ADD_STA, tuya_os_adapt_wlan_add_sta},
    {TUYA_HOSTAPD_REMOVE_STA, tuya_os_adapt_wlan_remove_sta},
    {TUYA_HOSTAPD_SET_GENERIC_ELEMENT, tuya_os_adapt_wlan_set_gen_elem},
    {TUYA_HOSTAPD_MLME, tuya_os_adapt_wlan_send_mlme},
    {TUYA_HOSTAPD_GET_INFO_STA, tuya_os_adapt_wlan_get_sta_info},
    {TUYA_HOSTAPD_STA_CLEAR_STATS, tuya_os_adapt_wlan_clear_stats},
    {TUYA_SET_ENCRYPTION, tuya_os_adapt_wlan_set_key},
    {TUYA_GET_ENCRYPTION, tuya_os_adapt_wlan_get_key},
    {TUYA_HOSTAPD_FLUSH, tuya_os_adapt_wlan_clear_stations},
    {TUYA_HOSTAPD_GET_BSS_INFO, tuya_os_adapt_wlan_get_bss_info},
    {TUYA_HOSTAPD_EXTERNAL_AUTH_STATUS, tuya_os_adapt_external_auth_status},
    {TUYA_HOSTAPD_GET_WPA_CAPACITY, tuya_os_adapt_get_wpa_capacity},
    {TUYA_HOSTAPD_SET_STA_KEEPALIVE, tuya_os_adapt_sta_inact_settings},
};

static int tuya_os_adapt_hostapd_ioctl(int dev, int vif_index, TY_WLAN_PARAMS_S *params)
{
    int i, ret = -1;

    for (i = 0; i < ARRAY_SIZE(hostapd_ioctl); i++) {
        if (hostapd_ioctl[i].cmd == params->cmd) {
            ret = hostapd_ioctl[i].func(vif_index, params);
            break;
        }
    }

    return ret;
}

static int tuya_os_adapt_wlan_ioctl_set(int dev, int vif_index, TY_WLAN_PARAMS_S *params)
{
    int i, ret = -1;

    for (i = 0; i < ARRAY_SIZE(wlan_setparam); i++) {
        if (wlan_setparam[i].cmd == params->cmd) {
            ret = wlan_setparam[i].func(vif_index, params);
            break;
        }
    }

    return ret;
}

static int tuya_os_adapt_wlan_ioctl_get(int dev, int vif_index, TY_WLAN_PARAMS_S *params)
{
    int i, ret = -1;

    for (i = 0; i < ARRAY_SIZE(wlan_getparam); i++) {
        if (wlan_getparam[i].cmd == params->cmd) {
            ret = wlan_getparam[i].func(vif_index, params);
            break;
        }
    }

    return ret;
}

int tkl_hostap_ioctl_inet(int dev, int vif_index, unsigned int cmd, unsigned long arg)
{
	int ret = -1;
    struct iwreq *iwr = (struct iwreq *)arg;
    TY_WLAN_PARAMS_S *params = (TY_WLAN_PARAMS_S *)iwr->u.data.pointer;
    //unsigned int params_len = iwr->u.data.length;
       
    HAPD_IOCTL_DBG("%s: ioctl cmd 0x%x subcmd %d vif_idx %d params %p len %d\r\n",
        __func__, cmd, params->cmd, params->vif_idx, params, params_len);
    
    switch (cmd) {
    case TUYA_IOCTL_HOSTAPD:
        ret = tuya_os_adapt_hostapd_ioctl(dev, vif_index, params);
        break;
    
    case TUYA_IOCTL_SETPARAM:
        ret = tuya_os_adapt_wlan_ioctl_set(dev, vif_index, params);
        break;

    case TUYA_IOCTL_GETPARAM:
        ret = tuya_os_adapt_wlan_ioctl_get(dev, vif_index, params);
        break;

    default:
        break;
    }

	return ret;
}

#if defined(TUYA_COMMAND_TEST) && (TUYA_COMMAND_TEST == 1)
#include "cli.h"
extern int tuya_iot_wf_gw_unactive(void);
int fhost_iot_wf_gw_unactive_cmd(cmd_tbl_t *t, int argc, char *argv[])
{
    int ret;

    //cli_printf("%s:%d", __func__, __LINE__);
    ret = tuya_iot_wf_gw_unactive();
    if (ret < 0) {
        os_printf(LM_APP, LL_INFO, "%s: IoT reset failed\n", __func__);
    }
    
    return ret;
}
CLI_CMD(iot_reset, fhost_iot_wf_gw_unactive_cmd, "iot reset", "iot reset");

extern void tuya_cpu_set_lp_mode(BOOL_T lp_enable);
extern int tuya_wifi_lp_disable();
extern int tuya_wifi_lp_enable();
extern void tuya_wifi_set_lps_dtim(const unsigned int dtim);
extern void tuya_cpu_lp_disable(void);

int fhost_iot_enable_lowpower(cmd_tbl_t *t, int argc, char *argv[])
{
    int enable, dtim = 1;
    
    if (argc < 2) {
        os_printf(LM_APP, LL_INFO, "%s: input invalid param\n", __func__);
        return -1;
    }

    enable = atoi(argv[1]);
    if (enable) {
        if (argc >= 3)
            dtim = atoi(argv[2]);
        tuya_cpu_set_lp_mode(1);
        tuya_wifi_lp_disable();
        tuya_wifi_set_lps_dtim(dtim);
        tuya_wifi_lp_enable();
        os_printf(LM_APP, LL_INFO, "%s: lowpower mode enabled, dtim %d\n", __func__, dtim);
    } else {
        tuya_cpu_lp_disable();
        tuya_wifi_lp_disable();
        //tuya_wifi_set_lps_dtim(dtim);
        os_printf(LM_APP, LL_INFO, "%s: lowpower mode disabled, dtim %d\n", __func__, dtim);
    }
    
    return 0;
}
CLI_CMD(lp_enable, fhost_iot_enable_lowpower, "iot lowpower enable/disable", "iot_enable_lowpower 1 10");

extern int SetLogManageAttr(int level);
int fhost_iot_log_level(cmd_tbl_t *t, int argc, char *argv[])
{
    int level;
    level = atoi(argv[1]);
	os_printf(LM_APP, LL_INFO, "Set debug log level to %d\n", level);
	SetLogManageAttr(level);
    return 0;
}
CLI_CMD(log_level, fhost_iot_log_level, "Set debug log level(0->err,1->warn,2->notice,3->info,4->debug,5->trace)", "log_level 4");

extern int ty_work_queue_depth(void);
extern int cmmod_get_msg_node_num(void);
extern int tuya_hal_system_getheapsize(void);
extern int sys_get_timer_num(void);
int fhost_iot_dump_q(cmd_tbl_t *t, int argc, char *argv[])
{
	os_printf(LM_APP, LL_INFO, "Number of work queue elems: %d\n", ty_work_queue_depth());
	os_printf(LM_APP, LL_INFO, "Number of commod message elems: %d\n", cmmod_get_msg_node_num());
	os_printf(LM_APP, LL_INFO, "Number of timer elems: %d\n", sys_get_timer_num());
	os_printf(LM_APP, LL_INFO, "Szie of current free heap: %d\n", tuya_hal_system_getheapsize());
    return 0;
}
CLI_CMD(q_dump, fhost_iot_dump_q, "Dump queue depth by Tuya", "q_dump");

#endif

#endif /* __TUYA_OS_ADAPT_HOSTAPD_C__ */
