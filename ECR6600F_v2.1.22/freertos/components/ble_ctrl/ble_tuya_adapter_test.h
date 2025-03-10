#ifndef BLE_CTRL_H
#define BLE_CTRL_H

#include "stdint.h"

#include <stdbool.h>

//hz-低功耗时，只支持蓝牙配网，不支持蓝牙控制，该宏的作用只是框住相关代码，便于管理
#define ONLY_SUPPORT_BT_CONFIG      (1)

/**
typedef enum {
    TY_BLE_EVENT_DISCONNECTED,   // APP断开连接 
    TY_BLE_EVENT_CONNECTED,      // APP连接上设备
    TY_BLE_EVENT_RX_DATA,        // 接收到APP数据
    TY_BLE_EVENT_ADV_READY,      // start adv.
}ty_ble_cb_event_t;

typedef enum {
    TY_BLE_MODE_PERIPHERAL,
    TY_BLE_MODE_CENTRAL,
}ty_ble_mode_t;

typedef struct{
	uint8_t len;
	uint8_t *data;
}tuya_ble_data_buf;

typedef void (*ty_ble_msg_cb)(int id, ty_ble_cb_event_t e, tuya_ble_data_buf *buf);

typedef struct {
	char name[APP_DEVICE_NAME_LEN]; //蓝牙名字
	ty_ble_mode_t mode; //配网应用作为外设，暂不支持配置
	int link_num; // 最大支持客户端连接数，//编译时确定,暂不支持配置
	ty_ble_msg_cb cb; // 事件和数据回调函数
	tuya_ble_data_buf adv; // 广播内容
	tuya_ble_data_buf scan_rsp; // 客户端(APP)扫描响应数据
}ty_ble_param;

//typedef VOID (*TY_BT_SCAN_ADV_CB)(CHAR_T *data, UINT_T len); 
typedef struct {
	uint8_t scan_type; // 扫描类型
	uint8_t* name; // 蓝牙名字，设置为TY_BT_SCAN_BY_NAME时生效
	uint8_t bd_addr[6]; //蓝牙的MAC，设置为TY_BT_SCAN_BY_MAC时生效
	uint8_t rssi; //扫描到信号强度
	uint8_t channel; //扫描到，返回信道
	uint8_t timeout_s; //扫描超时，在规定的时间没扫到，返回失败，单位是second
	//TY_BT_SCAN_ADV_CB scan_adv_cb;
}scan_info;
*/

#define	DEVICE_NAME_LEN		(16)

typedef enum {
    TY_BT_SCAN_BY_NAME = 0x01,
    TY_BT_SCAN_BY_MAC = 0x02,
    TY_BT_SCAN_BY_ADV = 0x03, //接收蓝牙广播
}ty_bt_scan_type_t;

typedef enum {
    TY_BT_EVENT_DISCONNECTED,   /* APP断开连接 */
    TY_BT_EVENT_CONNECTED,      /* APP连接上设备 */
    TY_BT_EVENT_RX_DATA,        /* 接收到APP数据*/
    TY_BT_EVENT_ADV_READY,      /* start adv. */
}ty_bt_cb_event_t;

typedef enum {
    TY_BT_MODE_PERIPHERAL,
    TY_BT_MODE_CENTRAL,
	TY_BT_MODE_MESH_PROVISIONER,
    TY_BT_MODE_MESH_DEVICE,
}ty_bt_mode_t;

typedef struct{
	uint8_t len;
	uint8_t *data;
}tuya_ble_data_buf_t;

typedef void (*TY_BT_MSG_CB)(int id, ty_bt_cb_event_t e, tuya_ble_data_buf_t *buf);

typedef struct {
    char name[DEVICE_NAME_LEN];
    ty_bt_mode_t mode;
    uint8_t link_num;
    TY_BT_MSG_CB cb;
    tuya_ble_data_buf_t adv;
    tuya_ble_data_buf_t scan_rsp;
}ty_bt_param_t;

typedef void (*TY_BT_SCAN_ADV_CB)(char *data, uint8_t len, uint8_t* mac, uint8_t type);
typedef struct {
    char scan_type; /* ref ty_bt_scan_type_t. */
    char name[DEVICE_NAME_LEN];
    uint8_t bd_addr[6];
    int8_t rssi;
    uint8_t channel;
    uint8_t timeout_s; /* second. */
    TY_BT_SCAN_ADV_CB scan_adv_cb;
}ty_bt_scan_info_t;


/**
 * @brief tuya_os_adapt_bt_port_init
 * 
 * @param[in] p 
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_port_init(ty_bt_param_t *p);

/**
 * @brief tuya_os_adapt_bt_port_deinit
 * 
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_port_deinit(void);

/**
 * @brief tuya_os_adapt_bt_gap_disconnect
 * 
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_gap_disconnect(void);

/**
 * @brief tuya_os_adapt_bt_send
 * 
 * @param[in] data: send buffer
 * @param[in] len: send buffer length
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_send(const unsigned char *data, const unsigned char len);

/**
 * @brief tuya_os_adapt_bt_reset_adv
 * 
 * @param[out] adv 
 * @param[out] scan_resp 
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_reset_adv(tuya_ble_data_buf_t *adv, tuya_ble_data_buf_t *scan_resp);

/**
 * @brief tuya_os_adapt_bt_get_rssi
 * 
 * @param[out] rssi 
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_get_rssi(signed char *rssi);

/**
 * @brief tuya_os_adapt_bt_start_adv
 * 
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_start_adv(void);

/**
 * @brief tuya_os_adapt_bt_stop_adv
 * 
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_stop_adv(void);

/**
 * @brief 
 * 
 * @param ty_bt_scan_info_t 
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_assign_scan(ty_bt_scan_info_t *info);


/**
 * @brief 
 * 
 * @param  
 * @return OPERATE_RET 
 */
int tuya_os_adapt_bt_get_ability();

//给sdk使用：
int tuya_os_adapt_bt_scan_init(TY_BT_SCAN_ADV_CB scan_adv_cb);
int tuya_os_adapt_bt_start_scan();
int tuya_os_adapt_bt_stop_scan();

/* add begin: by sunkz, interface regist */
//int tuya_os_adapt_reg_bt_intf(void);
/* add end */

#endif /*BLE_CTRL_H*/

