/**
 * @file tkl_cellular_base.h
 * @author www.tuya.com
 * @brief 蜂窝模组基础功能API实现接口。
 *
 * @copyright Copyright (c) tuya.inc 2021
 */
#ifndef __TKL_CELLULAR_BASE_H__
#define __TKL_CELLULAR_BASE_H__
#include <stdint.h>
#include "tuya_cloud_types.h"
#include "tkl_uart.h"
#include "tkl_cellular_mds.h"
#ifdef __cplusplus
extern "C" {
#endif


/* 蜂窝设备基本的IOCTL命令 */

typedef INT_T CELL_IOCTRL_CMD;

// 设置类命令起始值
#define CELL_IOCTL_CMD_SET_START                           0

//打开AP LOG输出
#define CELL_IOCTL_SET_TRACE_ON                            1
//关闭AP LOG输出
#define CELL_IOCTL_SET_TRACE_OFF                           2
//电源键关机功能使能
#define CELL_IOCTL_SET_POWEKEY_ENABLE_POWEROFF            3
//电源键关机功能禁止
#define CELL_IOCTL_SET_POWEKEY_DISABLE_POWEROFF           4
//关闭UART的AT命令服务
#define CELL_IOCTL_SET_ATUART_OFF                          5
//打开UART的AT命令服务
#define CELL_IOCTL_SET_ATUART_ON                          6
//获取RSSI dBm值
#define CELL_IOCTL_GET_RSSIDBM                            7

//获取当前蜂窝设备所在的网络类型
#define CELL_IOCTL_GET_NET_TYPE                           8

/*
获取当前蜂窝设备的参考信号质量，参考AT+CSQ
<rssi>:
    0 113 dBm or less
    1 111 dBm
    2. . . 30 109. . . 53 dBm
    31 51 dBm or greater
    99 not known or not detectable
*/
#define CELL_IOCTL_GET_RSSI                               9

// 设置短信内容格式
#define CELL_IOCTL_SET_SMS_CHAR                           10

// 设置蜂窝拨号IP类型(IPV4,IPV6,IPV4V6)
#define CELL_IOCTL_SET_PDP_TYPE                           11

// 串口自检，一般用于涂鸦产测方案
#define CELL_IOCTL_SET_UART_SELFTEST             0x100
// TF卡自检，一般用于涂鸦产测方案
#define CELL_IOCTL_SET_TFCARD_SELFTEST           0x101
// ADC自检，一般用于涂鸦产测方案
#define CELL_IOCTL_SET_ADC_SELFTEST              0x102
// 麦克风喇叭自检，一般用于涂鸦产测方案
#define CELL_IOCTL_SET_MIC_SELFTEST              0x103

// 耳机插入自检，一般用于涂鸦产测方案
#define CELL_IOCTL_SET_HEADSET_SELFTEST          0x104
// 获取信息类命令起始值
#define CELL_IOCTL_CMD_GET_START                 0x400

// 获取系统的Epoch 时间，单位秒
#define CELL_IOCTL_GET_EPOCH_TIME                0x401
// 获取本次系统上电后的累计时间，单位ms
#define CELL_IOCTL_GET_UP_TIME                   0x402
// 获取本次系统上电后的累计时间,单位us
#define CELL_IOCTL_GET_UP_TIME_US                0x403

// 获取底层蜂窝数据上下行统计
#define CELL_IOCTL_GET_DATA_STATICS              0x404

// USB设备控制

#define TUYA_CELL_USB_DEV_MODE_E    INT_T

enum {
    USB_DEV_MODE_CHARGE_ONLY = 0,
    USB_DEV_MODE_ROM_SERIAL = 1,
    USB_DEV_MODE_8SERIALS =2,
    USB_DEV_MODE_RNDIS= 3,
    USB_DEV_MODE_ECM = 5,
};
typedef struct{
    BOOL_T                      get_contrl;     //TRUE get，false set
    BOOL_T                      enable;         //TRUE enable,false disable
    TUYA_CELL_USB_DEV_MODE_E    usb_mode;
}TUYA_CELL_USB_CTRL;

#define CELL_IOCTL_USB_CTRL                      0x405

#define CELL_IOCTL_SET_REGISTION_HOOK           0x406   //设置一个蜂窝网络注册状态的回调函数到底层

#define CELL_IOCTL_GET_LOCAL_TIME               0x407   //根据基站同步的时间，获取蜂窝的设备的本地时间



#define CELL_IOCTL_INIT_VIRTUAL_AT              0X408   //启动虚拟AT

#define CELL_IOCTL_SEND_VIRTUAL_AT_CMD          0x409   //发送虚拟AT命令


#define CELL_IOCTL_SET_GENERAL_HOOK             0X410   //设置到蜂窝设备中，通用的回调函数

#define CELL_IOCTL_GET_TIMEZONE                 0x411   //根据基站同步的时间，获取时区信息

#define CELL_IOCTL_SET_RRC_RELEASE_TIME         0x412   //设置RRC connect切换到idle的延迟时间

#define CELL_IOCTL_GET_RRC_RELEASE_TIME         0x413   //获取RRC connect切换到idle的延迟时间

#define CELL_IOCTL_GET_SN                       0x414   //获取蜂窝SN号

#define CELL_IOCTL_SET_SN                       0x415   //设置蜂窝SN号

#define CELL_IOCTL_GET_SYSFWKEY                 0x416   //获取基础固件的FW key

#define CELL_IOCTL_MODULE_NAME                  0x417   //获取模组型号

#define CELL_IOCTL_FLASH_BACKUP                 0X418   //平台支持的系统备份功能。可选。

#define CELL_IOCTL_SET_PLMN                     0x419   //设置PLMN，可选

#define CELL_IOCTL_CMD_USR_START                0x1000


/**
 * @brief sim卡状态定义
 */
typedef enum
{
    TKL_NO_SIM,     /*< 无SIM卡 */
    TKL_SIM_READY,  /*< SIM已识别 */
    TKL_SIM_INIT,   /*< SIM卡初始化中>*/
    TKL_SIM_WAIT_PIN,
    TKL_SIM_WAIT_PUK,
} TKL_SIM_STATE_E;

typedef enum{
    TUYA_TSIM_TYPE, /* 物理卡 */
    TUYA_VSIM_TYPE, /* 虚拟卡 */
}TUYA_SIM_TYPE_E;

typedef enum {
  	DOUBLE_SIM_DOUBLE_CHANNEL = 1,
  	DOUBLE_SIM_SINGLE_CHANNEL = 2,
  	SINGLE_SIM_SINGLE_CHANNEL = 4,
}TKL_CELLULAR_ABILITY_E;

typedef enum
{
    TUYA_CELL_NET_TYPE_UNKNOWN = -1,
    TUYA_CELL_NET_TYPE_GSM = 1,
    TUYA_CELL_NET_TYPE_LTE,
    TUYA_CELL_NET_TYPE_3G,
    TUYA_CELL_NET_TYPE_CATM, //CATM1
    TUYA_CELL_NET_TYPE_NB,
}TUYA_CELLULAR_RAT_E;

#define NEIGHBOUR_NUM   4
typedef struct {
	UINT16_T lac;           //LAC
	INT16_T  rx_pwr;
	UINT_T   cellid;        //20bits(eNodeID)+8bits(Cellid)
}TKL_CELL_INFO_T;

typedef struct
{
	UCHAR_T mcc[3];
	UCHAR_T mnc[3];
	TKL_CELL_INFO_T main;
	TKL_CELL_INFO_T neighbour[NEIGHBOUR_NUM];
}TKL_LBS_INFO_T;

typedef struct {
   INT_T fd;
   CHAR_T name[32+1];
   TUYA_UART_BASE_CFG_T cfg;
   UINT8_T              sim_id;
   TUYA_SIM_TYPE_E      sim_type;
}TKL_CELL_INIT_PARAM_T;

typedef struct{
    INT_T cid;
    UINT32_T rx_count;
    UINT32_T tx_count;
}TKL_CELL_DATA_STATISTICS;


typedef  INT_T CELL_GENERAL_MSGID;

#define CELL_GENERAL_MSGID_RTC_SYNC     1
#define CELL_GENERAL_MSGID_RSSI_UPDATE  2

typedef struct{
    CELL_GENERAL_MSGID id;
    VOID *pdata;
    INT_T data_len;
}TKL_CELL_GENERAL_INFO_T;

/**
 * @brief 定义通用蜂窝回调通用
 * @param param 参数
 */
typedef VOID (*TKL_GENERAL_CALLBACK)(TKL_CELL_GENERAL_INFO_T *param);



/**
 * @brief 虚拟AT接收应答回调函数
 * @param resp 返回的AT应答
 * @param len  返回的AT应答数据长度
 */
typedef VOID (*TKL_VIRTAT_RESP)(CHAR_T *resp,INT_T len);

/**
 * @brief sim卡状态变化通知函数原型
 * @param state sim卡状态，查看 @TAL_SIM_STATE_E 定义
 */
typedef VOID (*TKL_SIM_NOTIFY)(TKL_SIM_STATE_E status);


OPERATE_RET tkl_cellular_base_init(TKL_CELL_INIT_PARAM_T *param);


/**
 * @brief 蜂窝网络注册状态变化通知函数原型，该接口是为了svc_netmgr适配
 * @param simId sim卡ID
 * @param state 蜂窝网络状态，查看 @TUYA_CELLULAR_MDS_STATUS_E 定义
 */
typedef VOID (*TKL_REGISTION_NOTIFY)(UINT8_T sim_id,TUYA_CELLULAR_MDS_STATUS_E st);


/**
 * @brief 获取当前设备的通讯能力
 * @param ability @TKL_CELLULAR_ABILITY_E 类型
 * @return 0 成功  其它 失败
 */
OPERATE_RET tkl_cellular_base_get_ability(TKL_CELLULAR_ABILITY_E *ability);

/**
 * @brief 切换当前使能的SIM卡。
 * @param simid SIM卡ID.(0~1)
 * @return 0 成功  其它 失败
 */
OPERATE_RET tkl_cellular_base_switch_sim(UINT8_T sim_id);

/**
 * @brief 注册SIM状态变化通知函数
 * @param fun 状态变化通知函数
 * @return 0 成功  其它 失败
 */

OPERATE_RET tkl_cellular_base_register_sim_state_notify(UINT8_T simd_id,TKL_SIM_NOTIFY fun);

/**
 * @brief 使能或禁止sim卡热拔插
 *
 * @param simId sim卡ID
 * @param enable TRUE 使能 FALSE 禁止
 *
 * @return 0 成功 其它 失败
 */
OPERATE_RET tkl_cellular_base_enable_sim_hotplug(UINT8_T sim_id, BOOL_T enable);

/**
 * @brief 获取SIM卡的状态
 * @param simId sim卡ID
 * @param state 1：正常，0：异常，2：初始化中
 * @return 0 成功 其它 失败
 */
OPERATE_RET tkl_cellular_base_sim_get_status(UINT8_T sim_id, UCHAR_T *state);

/**
 * @brief 获取蜂窝设备当前的通信功能设置
 *
 * @param cfun 获取的通信功能
 *
 * @return 0 成功 其它 失败
 */

OPERATE_RET tkl_cellular_base_get_cfun_mode(UINT8_T simd_id,PINT_T cfun);

/**
 * @brief 设置蜂窝设备的通信功能模式
 *
 * @param cfun 通信功能，取值含义如下：
 *            1：全功能模式
 *            4：飞行模式
 *
 * @return 0 成功 其它 失败
 */

OPERATE_RET tkl_cellular_base_set_cfun_mode(UINT8_T simd_id,INT_T cfun);

/**
 * @brief 获取SIM卡中的国际移动用户识别码
 *
 * @param simid,SIM卡id号(0,1,2...)
 * @param imsi识别码，为16字节的字符串
 *
 * @return 0 成功 其它 失败
 */

OPERATE_RET tkl_cellular_base_get_imsi(UINT8_T sim_id,CHAR_T imsi[15 + 1]);

/**
 * @brief 获取SIM卡的ICCID
 * @param simid
 * @param ICCID识别码，为20字节的字符串
 * @return 0 成功 其它 失败
 */
OPERATE_RET tkl_cellular_base_get_iccid (UINT8_T sim_id,CHAR_T iccid[20 + 1]);
/**
 * @brief 获取SIM卡所在通道设备的IMEI号
 * @param simid
 * @param IMEI识别码，为15字节的字符串
 * @return 0 成功 其它 失败
 */
OPERATE_RET tkl_cellular_base_get_imei (UINT8_T sim_id,CHAR_T imei[15 + 1]);

/**
 * @brief 设置设备的IMEI号
 * @param simid
 * @param IMEI识别码，为15字节的字符串
 * @return 0 成功 其它 失败
 */

OPERATE_RET tkl_cellular_base_set_imei (UINT8_T sim_id,CHAR_T imei[15 + 1]);

/**
 * @brief 获取SIM卡所在通道蜂窝设备的信号接收功率——单位dbm
 * @param simid
 * @param rsrp 返回实际的信号强度(dbm)
 * @return 0 成功 其它 失败
 */

OPERATE_RET tkl_cellular_base_get_rsrp(UINT8_T sim_id,INT_T *rsrp);

/**
 * @brief 获取蜂窝设备SIM卡所在通道的信号噪声比及误码率
 * @param simid
 * @param sinr (0~31)
 * @param bit_error (0~7,99) 99无网络
 * @return 0 成功 其它 失败
 */

OPERATE_RET tkl_cellular_base_get_sinr(UINT8_T sim_id,INT_T *sinr,INT_T *bit_error);

/**
 * @brief SIM卡所在通道LBS的基站信息)
 * @param simid
 * @param lbs 返回基站信息
 * @param neighbour 是否搜索临近基站信息
 * @param timeout 搜索临近基站信息超时时间(一般需要4秒左右)
 * @return 0 成功 其它 失败
 */

OPERATE_RET tkl_cellular_base_get_lbs(UINT8_T sim_id,TKL_LBS_INFO_T *lbs,BOOL_T neighbour,INT_T timeout);

/**
 * @brief 获取当前设备的射频校准状态
 * @param
 * @return TRUE正常，FALSE异常
 */

BOOL_T tkl_cellular_base_rf_calibrated(VOID_T);


/**
 * @brief 使能或禁止sim卡gpio检测
 *
 * @param simId sim卡ID
 * @param enable TRUE 使能 FALSE 禁止
 *
 * @return 0 成功 其它 失败
 */
OPERATE_RET tkl_cellular_base_enable_sim_detect(UINT8_T simid, BOOL_T enable);

/**
 * @brief 获取默认的SIM ID
 *
 *
 * @return 小于0失败，其他SIM ID
 */
INT8_T tkl_cellular_base_get_default_simid(VOID);


/**
 * @brief 蜂窝基础的通用控制功能，一般作为平台提供一些特殊的能力接口
 *
 * @param cmd 参考CELL_IOCTRL_CMD
 * @param argv 平台自定义
 *
 * @return 0 成功 其它 失败
 */
OPERATE_RET tkl_cellular_base_ioctl(INT_T cmd,VOID* argv);

#ifdef __cplusplus
}
#endif

#endif
