#ifndef __PSM_WIFI_H__
#define __PSM_WIFI_H__

#include "psm_system.h"

void psm_set_send_pwr_stat(bool stat);
bool psm_get_send_pwr_stat();
unsigned int psm_set_wifi_next_status(unsigned char next_status);
unsigned int psm_set_wifi_prev_status(unsigned char prev_status);
unsigned char psm_get_wifi_next_status(void);

unsigned char psm_wifi_listen_interval_op(bool isSet, unsigned char value);
unsigned int psm_wifi_dtim_bcn_avg_op(bool isSet, unsigned int value);
unsigned char psm_wifi_dtim_period_op(bool isSet, unsigned char value);
unsigned char psm_wifi_dtim_cnt_op(bool isSet, unsigned char value);
unsigned int psm_wifi_bcn_int_op(bool isSet, unsigned int value);
unsigned long long psm_wifi_local_tsf_beacon_op(bool isSet, unsigned long long value);
unsigned long long psm_wifi_tsf_AP_op(bool isSet, unsigned long long value);
unsigned long long psm_wifi_peer_tsf_op(bool isSet, unsigned long long value);
int psm_wifi_tsf_offset_op(bool isSet, int value);
unsigned int psm_wifi_beacon_duration_op(bool isSet, unsigned int value);
unsigned char psm_wifi_send_nulldata_0_op(bool isSet, unsigned char value);
unsigned char psm_wifi_nulldata_0_failcnt_op(bool isSet, unsigned char value);
unsigned char psm_wifi_send_nulldata_1_op(bool isSet, unsigned char value);
unsigned char psm_wifi_nulldata_1_failcnt_op(bool isSet, unsigned char value);
bool psm_wifi_twt_status_op(bool isSet, bool value);
int psm_wifi_twt_rtc_error_op(bool isSet, int value);
int psm_wifi_twt_beacon_offset_op(bool isSet, int value);
unsigned int psm_wifi_twt_wake_interval_op(bool isSet, unsigned int value);
unsigned int psm_wifi_twt_wake_dur_op(bool isSet, unsigned int value);
unsigned int psm_wifi_twt_schedul_time_op(bool isSet, unsigned int value);
void * psm_wifi_twt_flow_op(bool isSet, void * value);
unsigned int psm_wifi_mult_of_100ms_op(bool isSet, unsigned int value);
bool psm_wifi_rtc_error_cal_op(bool isSet, bool value);
unsigned char psm_wifi_beacon_normal_cnt_op(bool isSet, unsigned char value);
unsigned char psm_wifi_beacon_lost_cnt_op(bool isSet, unsigned char value);
BEACON_REC_STATUS psm_wifi_beacon_status_op(bool isSet, BEACON_REC_STATUS value);
void psm_update_beacon_cnt();
void psm_clear_beacon_lost_cnt();
void psm_send_ap_pwr();
unsigned int psm_deep_sleeptime_op(bool isSet, unsigned int value);

#endif
