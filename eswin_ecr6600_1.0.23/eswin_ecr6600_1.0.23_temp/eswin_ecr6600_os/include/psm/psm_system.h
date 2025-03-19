/*******************************************************************************
 * Copyright by eswin 
 *
 * File Name:  
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       v0.1
 * Author:        
 * Date:          
 * History 0:      
 *     Date: 
 *     Version:
 *     Author: 
 *     Modification:  
  ********************************************************************************/

#ifndef __PSM_SYSTEM_H__
#define __PSM_SYSTEM_H__

#include "psm_mode_ctrl.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include <stdlib.h>

#include "task.h"



/* reade only compile flag : start */
extern const volatile char psm_lib_compile_date[];
extern const volatile char psm_lib_compile_time[];
/* reade only compile flag : end */



enum psm_state_bit_pos
{
	PSM_MODEM_SLEEP	=0,
	PSM_WFI_SLEEP,
	PSM_LIGHT_SLEEP,
	PSM_DEEP_SLEEP,
};

#define PSM_SLEEP_GET(bit_pos) (psm_infs.sleep_mode_ena & (1<< PSM_##bit_pos))
#define PSM_SLEEP_SET(bit_pos) (psm_infs.sleep_mode_ena |= 1<< PSM_##bit_pos)
#define PSM_SLEEP_CLEAR(bit_pos) (psm_infs.sleep_mode_ena &= ~(1<< PSM_##bit_pos))
#define CONFIG_ECR_BLE_PSM 1 

//#define IO_DEBUG 1		//for scope debug,default 0 else 1
#define CONFIG_TWT_POWER_SAVE 1 //for twt powersave and chip sleep enable

/*beacon period is 102ms*/
#define BEACON_PERIOD (102)
#define BEACON_PERIOD_US (102400)
#define BEACON_PERIOD2RTC (3342)/*102MS/(1S/32768)*/
#define ACTIVE_TIME_CW (14000)/*time= before sleep time + after sleep time*/
#define BEACON_LOST_THR (20)/*beacon lost threshold*/
#define TWT_BEACON_LOST_THR (8)
#define LIGHT_SLEEP_TIME_THR (10)/*sleep time > 10ms goto light sleep*/
#define RTC_CNT_TO_US (30.5)
#define ACTIVE_TIME_CW_1 (6000)
#define POWERON_TIME (4300)

#define vPortSuppressTicksAndSleep( xExpectedIdleTime )  psm_schedule_idle_cb(xExpectedIdleTime)

/*-----------------------------------------------------------
    |STATE                  |   WIFI    |   BLE     |
    |WIFI_ACTIVE_BLE_ACTIVE |    0      |    0      |   0
    |WIFI_ACTIVE_BLE_DOZE   |    0      |    1      |   1
    |WIFI_DOZE_BLE_ACTIVE   |    1      |    0      |   2
    |WIFI_DOZE_BLE_DOZE     |    1      |    1      |   3
-------------------------------------------------------------
*/
enum PS_STATUS{
   //wifi is active and ble is active  
   WIFI_ACTIVE_BLE_ACTIVE=0,   
   //wifi is active and ble is doze
   WIFI_ACTIVE_BLE_DOZE,   
   //wifi is doze and ble is active
   WIFI_DOZE_BLE_ACTIVE ,
   //wifi is doze and ble is doze
   WIFI_DOZE_BLE_DOZE ,
   //max status,should't in this status 
   WIFI_BLE_STATUS_MAX,
};

typedef enum
{
	PSM_DEVICE_GPIO = 0,
	PSM_DEVICE_I2C,
	PSM_DEVICE_SPI,
	PSM_DEVICE_SPI_FLASH,
	PSM_DEVICE_ADC,
	PSM_DEVICE_SDIO_SLAVE,
	PSM_DEVICE_SDIO_HOST,
	PSM_DEVICE_DMA,   
	PSM_DEVICE_I2S,
	PSM_DEVICE_WATCHDOG,
	PSM_DEVICE_WIFI_STA,
	PSM_DEVICE_WIFI_AP,
	PSM_DEVICE_UART0,
	PSM_DEVICE_UART1,
	PSM_DEVICE_UART2,
	PSM_DEVICE_PWM, 
	PSM_DEVICE_TIMER,  
	PSM_DEVICE_ECC,
	PSM_DEVICE_EFUSE,
	PSM_DEVICE_BLE,
	PSM_MAX_DEVICE
}PSM_DEV_NUM;


typedef enum 
{
	PSM_OFF_LINE=0,
	PSM_CONNECTING,
	PSM_CONNECTED,
	NETWORK_STATUS_MAX,
}NETWORK_STATUS;

enum WIFI_BLE_EN{
    SLEEP_DISABLE = 0,
    SLEEP_ENABLE,
};

enum SLEEP_COMMON_STATUS
{
    PSM_COMMON_ACTIVE =0,
    PSM_COMMON_DOZE,
    PSM_COMMON_MAX,
};
typedef enum 
{
	PSM_DEVICE_STATUS_IDLE = 0,
	PSM_DEVICE_STATUS_ACTIVE,
	PSM_DEVICE_STATUS_MAX,
}PSM_DEV_STATUS;

typedef enum 
{
    BEACON_LOST= 0,
    BEACON_NORMAL,
}BEACON_REC_STATUS;

typedef enum 
{
    NULLDATA_OK= 0,
    NULLDATA_SENDING,
    NULLDATA_FAIL,
}SEND_NULLDATA_STATUS;

struct psm_wifi_info
{
	bool wifi_psm_en;				//wifi enable
	bool rtc_error_cal;
	unsigned int last_beacon_time;			//wifi
	unsigned int bcn_int;			//beacon interval, us, get from mm_tbtt_compute()
	unsigned int dtim_bcn_avg;
    unsigned int rtc_actual_val;
	unsigned int twt_schedul_time;
	unsigned int twt_wake_interval; 
	unsigned int twt_wake_dur;
	int	twt_beacon_offset;
	int	twt_rtc_error;
	unsigned int mult_of_100ms;
	unsigned short vif_idx;
	unsigned char listen_interval;
	unsigned char listen_interval_req;
    unsigned char dtim_period;
    unsigned char dtim_cnt;//maybe dtim1/dtim10/dtim20 etc
	unsigned char wifi_prev_status;		//previous wifi and ble status
	unsigned char wifi_now_status;		//now wifi and ble status
	unsigned char wifi_next_status;		//next wifi and ble status
	unsigned int net_status;	//network status
    BEACON_REC_STATUS beacon_status;
    unsigned char beacon_lost_cnt;
    unsigned char beacon_normal_cnt;
    unsigned char pspoll_flag;
    unsigned char send_nulldata_0;
    unsigned char send_nulldata_1;
    unsigned char nulldata_0_failcnt;
    unsigned char nulldata_1_failcnt;
    bool          send_pwr;
	bool          twt_status;
	void *twt_flow;
	unsigned long long peer_tsf; 
	unsigned long long local_tsf_outsleep;
	unsigned long long local_tsf_beacon;
	unsigned long long tsf_AP;
	int tsf_offset;
	unsigned int beacon_duration;
};

struct psm_ble_info
{
	bool ble_psm_en;				//ble enable
	bool ble_psm_store_flag;
	unsigned int ci;				//ble
	unsigned char ble_prev_status;		//previous wifi and ble status
	unsigned char ble_now_status;		//now wifi and ble status
	unsigned char ble_next_status;		//next wifi and ble status
};

struct psm_info
{
	struct psm_wifi_info wifi_infs;
    struct psm_ble_info ble_infs;
	//uint32_t prevent_sleep;
	unsigned int expect_sleep_time; //us
	unsigned int next_sleep_time_point; //32k cnt
	unsigned int pit_when_sleep;
	unsigned int rtc_when_sleep;
    unsigned int rtc_clac_sleep;
	unsigned int device_status;		//all device status
	unsigned int deep_sleep_time;	//set time for deepsleep
	unsigned int cpu_freq;
	unsigned char crystal_26m_or_40m;	//0:40M 1:26M
	
	unsigned char prev_status;		//previous wifi and ble status
	unsigned char now_status;		//now wifi and ble status
	unsigned char next_status;		//next wifi and ble status
	unsigned char sleep_status;	//record the sleep status
	//unsigned char deep_sleep_en;
    //unsigned char light_sleep_en;
    unsigned char sleep_mode;
	unsigned char sleep_mode_ena;
    unsigned char have_sleep_flag;
	unsigned int uart_last_time;
	unsigned int rtc_cb_input;
	void_fn dev_restore_cb[PSM_MAX_DEVICE];
    bool    is_rtc_compenstion;
	bool	is_rtc_update;
	//bool isr_from_beacon;		//
};
struct psm_dbg_wifi_info
{
	unsigned int cnt_deep_sleep;
	unsigned int cnt_light_sleep;
	unsigned int cnt_wfi_sleep;
	unsigned int cnt_modem_sleep;
	unsigned int cnt_rec_beacon;
	
};
struct psm_dbg_ble
{

};
struct psm_dbg_info
{
	struct psm_dbg_wifi_info psm_dbg_wifi;
};

struct psm_dbg_handler_t{
    const char *module;
    const char *cmd;
    int (*func)(int argc, char *argv[]);
};
#define _PSM_HANDLE_DEF_(m, cmd, f) #m, #cmd, psm_ ## m ## _ ## f

struct psm_dbg_info psm_dbg_infs;
#define _PSM_DBG_RCD_(mode) (++(psm_dbg_infs.psm_dbg_wifi.cnt_ ## mode))
#define _PSM_DBG_CLR_(mode) (do{psm_dbg_infs.psm_dbg_wifi.cnt_ ## mode=0;}while)
#define PSM_SHOW_FORMAT(x,val) os_printf(LM_WIFI, LL_INFO, "%-25s: 0x%x\n", #x, val);

struct psm_info psm_infs;

extern unsigned int psm_wifi_ps_to_doze(void);
extern unsigned int psm_wifi_ps_to_active(void);
//extern unsigned int psm_check_sleep_wifi();
extern void psm_store_mac_timer_reg(unsigned int *reg_buff);
extern void psm_restore_mac_timer_reg(unsigned int *reg_buff);
extern bool psm_check_dev_station();
//extern int check_wifi_link_on(int vif);
extern void psm_sleep_store_reg(void);
extern void psm_wakeup_modem_restore(void);
extern void psm_send_sleep_pkt();
extern void psm_send_nulldata(uint8_t psmode, const char *func, int line);
extern uint32_t ulGetTickCounter(void);
extern void psm_pwr_mgt_ctrl(int pwrmgt);
extern int psm_dpsm_info(int argc, char *argv[]);
extern void enable_modem_interrupt(void);
extern void rf_channel_set(int bw, int ch);
extern void psm_mask_modem_intc();
extern void psm_disable_mac_timer();
extern bool psm_check_sleep_wifi();
extern void ap_cap_clear_interface();
extern void psm_twt_clear(uint16_t reason);
extern void psm_twt_reconnect();
extern int psm_pwr_mgt_get();
extern unsigned int drv_rtc_32k_format_add(unsigned int cnt,unsigned int value);
extern unsigned int drv_rtc_32k_format_sub(unsigned int cnt,unsigned int value);
extern  void phy_set_ch_restore();
extern void psm_set_lowpower(void);
extern void psm_set_normal(void);
extern void psm_set_twt(int en, unsigned char mode, unsigned short mantissa, unsigned char exp);
extern void psm_unmask_modem_intc();
extern void psm_enable_mac_timer();
extern bool psm_ble_wifi_sleep_check(void);
extern uint8_t psm_ble_content_store(void);
extern uint8_t psm_ble_content_store(void);
extern void psm_clear_trx_rcd();
extern void psm_clear_ucrx_timer();
extern void rwnx_reg_store(void);
extern bool psm_check_tx_finish();
extern int psm_check_wifi_status();
#ifdef CONFIG_ECR_BLE
extern void rf_ble_restore();
#endif
void psm_wifi_ble_init();
int psm_set_sleep_next_status(unsigned char next_status);
unsigned int psm_auto_adjust_sleep_time(unsigned int *need_sleep_time,unsigned int *next_sleep_time_point);
bool psm_check_all_device_idle();
bool psm_check_single_device_idle(unsigned int dev_id);
bool psm_check_psm_enable();
bool psm_set_psm_enable(unsigned int enable);
bool psm_set_wifi_beacon_status(unsigned int status);
bool psm_set_last_beacon_time(unsigned int air_time);
unsigned int psm_cal_beacon_period_us(uint64_t tsf_AP, uint32_t bcn_int);
void psm_clear_g_tsf_buf(void);
unsigned int psm_set_device_status(PSM_DEV_NUM dev_id,PSM_DEV_STATUS dev_status);
void psm_update_beacon_cnt();
int psm_dbg_func_cb(int argc, char *argv[]);
unsigned char psm_wifi_set_pspoll_status(unsigned char status);
void psm_send_ap_pwr();
unsigned int psm_set_wifi_next_status(unsigned char next_status);
bool psm_get_rtc_compenstion_stat();
void psm_set_rtc_compenstion_stat(bool stat);
void psm_check_nodata_long_time();
bool psm_get_rtc_compenstion_stat();
void psm_pit_compenstion();
bool psm_check_sleep_wifi(void);
void psm_set_send_pwr_stat(bool stat);
bool psm_get_send_pwr_stat();
unsigned int psm_set_wifi_prev_status(unsigned char prev_status);
bool psm_is_stat_sleep();
unsigned char psm_get_wifi_next_status(void);
void psm_modem_clk_restore();
int psm_get_sleep_mode();
bool psm_set_sleep_mode(unsigned int sleep_mode, unsigned char listen_interval);
bool psm_set_deep_sleep(unsigned int sleep_time);
int psm_rtc_bias_timer_init(unsigned int time);
int psm_get_cpu_freq(void);
bool psm_get_ble_psm_flag();
void psm_light_sleep(uint64_t sleep_time);
unsigned char psm_get_next_sleep_status();
unsigned char psm_generate_next_status(void);
unsigned char psm_get_listen_interval();
void psm_clear_beacon_lost_cnt();
unsigned int psm_set_wifi_status(NETWORK_STATUS status);
unsigned int psm_get_wifi_status();
#ifdef CONFIG_TUYA_DOORCONTACT
void psm_deep_sleep(unsigned int sleep_time);
void psm_enable_lower_deep_mode(bool status);
#endif	//CONFIG_TUYA_DOORCONTACT
#endif
