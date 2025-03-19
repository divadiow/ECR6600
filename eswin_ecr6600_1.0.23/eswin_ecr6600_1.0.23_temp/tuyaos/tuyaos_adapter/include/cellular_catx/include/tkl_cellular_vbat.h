/**
 * @file tkl_cellular_vbat.h
 * @author www.tuya.com
 * @brief 蜂窝模组电池功能API实现接口。
 *
 * @copyright Copyright (c) tuya.inc 2021
 */

#ifndef __TKL_CELLULAR_VBAT_H__
#define __TKL_CELLULAR_VBAT_H__

#include <stdint.h>
#include "tuya_cloud_types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 电池及充电器消息定义
 */
typedef enum
{
    TKL_CELLULAR_VBAT_CHG_START = 0x01, /*< 开始充电 */
    TKL_CELLULAR_VBAT_CHG_FINISH,       /*< 结束充电 */
    TKL_CELLULAR_VBAT_CHG_WARNING,      /*< 电池电量低，提醒用户充电 */
    TKL_CELLULAR_VBAT_CHG_SHUTDOWN,     /*< 电池电量超低，提醒用户关机 */
    TKL_CELLULAR_VBAT_CHG_BATT_OFF,     /*< 电池拔出 */
    TKL_CELLULAR_VBAT_CHG_CAPACITY,     /*< 电池电量变化通知 */
    TKL_CELLULAR_VBAT_CHG_DISCONNECT,   /*< 充电器拔出 */
    TKL_CELLULAR_VBAT_CHG_FAULT,        /*< 充电故障 */
    TKL_CELLULAR_VBAT_CHG_MAX
} TKL_CELLULAR_VBAT_CHG_MSG_T;

/**
 * @brief 充电器状态定义
 */
typedef enum
{
    TKL_CELLULAR_CHG_STATE_UNCONNECTED = 0x01, /*< 充电器未连接 */
    TKL_CELLULAR_CHG_STATE_CHARGING,           /*< 充电器已连接，充电中 */
    TKL_CELLULAR_CHG_STATE_CHARGE_OVER,        /*< 充电器已连接，充电完成 */
    TKL_CELLULAR_CHG_STATE_MAX
} TKL_CELLULAR_VBAT_CHG_STATE_E;

/**
 * @brief 电池及充电器消息回调函数原型定义
 * @param msg 电池及充电器消息，查看 @TKL_CELLULAR_VBAT_CHG_MSG_T 定义
 * @return 无
 */
typedef VOID (*TKL_CELLULAR_VBAT_CHARGE_CB)(TKL_CELLULAR_VBAT_CHG_MSG_T msg);


/**
 * @brief 获取电池剩余电量百分比
 *
 * @param rsoc 剩余电量百分比
 *
 * @return OPRT_OK 获取成功 其它 获取失败
 */
OPERATE_RET tkl_cellular_vbat_get_rsoc(UINT8_T* rsoc);



/**
 * @brief 获取电池电压
 *
 * @param voltage 当前电池电压，单位mV
 *
 * @return OPRT_OK 获取成功 其它 获取失败
 */
OPERATE_RET tkl_cellular_vbat_get_voltage(UINT_T* voltage);

/**
 * @brief 设置是否开启NTC检测电池温度
 *
 * @param enable NTC检测电池温度开/关
 *
 * @return OPRT_OK 设置成功 其它 设置失败
 */
OPERATE_RET tkl_cellular_vbat_ntc_enable(BOOL_T enable);

/**
 * @brief 设置恒流充电阶段，电池充电电流
 *
 * @param current 充电电流，单位毫安（mA)
 *
 * @return OPRT_OK 设置成功 其它 设置失败
 */
OPERATE_RET tkl_cellular_vbat_set_charge_current(UINT_T current);

/**
 * @brief 获取充电器状态
 *
 * @param 无
 *
 * @return 充电器状态，查看 @TKL_CELLULAR_VBAT_CHG_STATE_E定义
 */

TKL_CELLULAR_VBAT_CHG_STATE_E tkl_cellular_vbat_get_charger_state(VOID);

/**
 * @brief 注册电池及充电器消息回调处理函数
 *
 * @param callback 回调函数
 *
 * @return 0 注册成功 其它 注册失败
 */
OPERATE_RET tkl_cellular_vbat_cb_register(TKL_CELLULAR_VBAT_CHARGE_CB callback);

/**
 * @brief 是否开启电池低电压关机功能
 *
 * @param enable TRUE 开启，FLASE关闭
 *
 * @return OPRT_OK 设置成功 其它 设置失败
 */
OPERATE_RET tkl_cellular_vbat_low_volt_poweroff_enable(BOOL_T enable);

#ifdef __cplusplus
}
#endif

#endif
