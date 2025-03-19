/**
 * @file tkl_cellular_call.h
 * @author www.tuya.com
 * @brief 蜂窝模组电话拨号API实现接口。
 *
 * @copyright Copyright (c) tuya.inc 2021
 */

#ifndef __TKL_CELLULAR_CALL_H__
#define __TKL_CELLULAR_CALL_H__

#include <stdint.h>
#include "tuya_cloud_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * 信号音类型定义
 */
typedef enum
{
    TUYA_TONE_DTMF_0 = 0,                /*< DTMF '0' */
    TUYA_TONE_DTMF_1,                    /*< DTMF '1' */
    TUYA_TONE_DTMF_2,                    /*< DTMF '2' */
    TUYA_TONE_DTMF_3,                    /*< DTMF '3' */
    TUYA_TONE_DTMF_4,                    /*< DTMF '4' */
    TUYA_TONE_DTMF_5,                    /*< DTMF '5' */
    TUYA_TONE_DTMF_6,                    /*< DTMF '6' */
    TUYA_TONE_DTMF_7,                    /*< DTMF '7' */
    TUYA_TONE_DTMF_8,                    /*< DTMF '8' */
    TUYA_TONE_DTMF_9,                    /*< DTMF '9' */
    TUYA_TONE_DTMF_A,                    /*< DTMF 'A' */
    TUYA_TONE_DTMF_B,                    /*< DTMF 'B' */
    TUYA_TONE_DTMF_C,                    /*< DTMF 'C' */
    TUYA_TONE_DTMF_D,                    /*< DTMF 'D' */
    TUYA_TONE_DTMF_SHARP,                /*< DTMF '#' */
    TUYA_TONE_DTMF_STAR,                 /*< DTMF '*' */
} TUYA_TONE_TYPE_E;


/**
 * @brief 电话号码最大长度
 * @note 电话号码为字符串形式，该长度包含'\0'字符。
 */
#define TKL_CELLULAR_CALLNUM_LEN_MAX  42

/**
 * @brief 呼叫回调状态
 */
typedef enum
{
    TUYA_CALL_IND_CALLIN = 1,       /*< 呼入指示 */
    TUYA_CALL_IND_RELEASE,          /*< 呼叫释放指示 */
    TUYA_CALL_IND_CALL_RSP_OK,      /*< 呼出应答成功指示 */
    TUYA_CALL_IND_CALL_RSP_FAIL,    /*< 呼出应答失败指示 */
    TUYA_CALL_IND_RING,             /*< 呼出对方振铃指示 */
    TUYA_CALL_IND_ACCEPT_CALL_OK,   /*< 呼入应答响应成功指示 */
    TUYA_CALL_IND_ACCEPT_CALL_FAIL, /*< 呼入应答响应失败指示 */
    TUYA_CALL_IND_RELEASE_RSP,      /*< 呼叫释放响应指示 */
    TUYA_CALL_IND_DTMF_KEY,         /*< 获取收到的DTMF对应的键值*/
    TUYA_CALL_IND_MAX
} TUYA_CELLULAR_CALL_IND_STATE_E;

/**
 * @brief 呼叫回调处理函数原型
 * @param state 呼叫状态
 * @param simId sim卡ID
 * @param callNum 电话号码
 * @return 无
 */
typedef VOID_T (*TKL_CELLULAR_CALL_CB)(TUYA_CELLULAR_CALL_IND_STATE_E state,
                                        UINT8_T simId, CHAR_T callNum[TKL_CELLULAR_CALLNUM_LEN_MAX]);

/**
 * @brief 通话过程中，对方的按键输入侦测回调函数定义，用于侦测对方的DTMF输入
 * @param dtmf
 * @return 无
 */
typedef VOID_T (*TKL_CELLULAR_CALL_KTDETECH_CB)(TUYA_TONE_TYPE_E dtmf);

/**
 * @brief 查询呼叫服务是否可用
 * @note 该函数用于查询呼叫服务是否可用，只有在呼叫服务可用时，才能使用使用
 *       其它呼叫服务接口。
 *
 * @param simId sim卡ID
 *
 * @return 1 服务可用 0 服务不可用
 */
OPERATE_RET tkl_cellular_call_service_available(UINT8_T sim_id);

/**
 * @brief 呼叫拨号接口函数
 *
 * @param simId sim卡ID号
 * @param callNUm 呼叫号码，字符串形式
 *
 * @return  0 发起呼叫成功 其它 失败
 */
OPERATE_RET tkl_cellular_call(UINT8_T sim_id, CHAR_T callNum[TKL_CELLULAR_CALLNUM_LEN_MAX]);

/**
 * @brief 外部呼叫到来时，应答接听接口函数
 *
 * @param 无
 *
 * @return   0 应答成功 其它 失败
 */

OPERATE_RET tkl_cellular_call_answer(UINT8_T sim_id);

/**
 * @brief 呼叫通话后，挂机接口函数
 *
 * @param 无
 *
 * @return  0 挂机成功 其它 失败
 */

OPERATE_RET tkl_cellular_call_hungup(UINT8_T sim_id);

/**
 * @brief 注册用户定义的呼叫回调处理函数
 *
 * @param callback 呼叫回调处理函数
 *
 * @return 0 注册成功 其它 注册失败
 */
OPERATE_RET tkl_cellular_call_cb_register(TKL_CELLULAR_CALL_CB callback);

/**
 * @brief 设置呼入时铃声静音
 *
 * @param mute TRUE 静音 FALSE 非静音
 *
 * @return 0 设置成功 其它 设置失败
 */
OPERATE_RET tkl_cellular_call_set_callin_mute(BOOL_T mute);


/**
 * @brief 启动或者关闭volte功能
 *
 * @param enable TRUE启用volte,false关闭volte
 *
 * @return 0 设置成功 其它 设置失败
 */
OPERATE_RET tkl_cellular_call_set_volte(UINT8_T sim_id,BOOL_T enable);


/**
 * @brief 播放电话的拨号音

 * @param tone 拨号音的特征值

 * @param duration 播放音的持续时间

 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_call_play_tone(TUYA_TONE_TYPE_E tone, INT_T duration);


/**
 * @brief 停止电话的拨号音
 * @param 无
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_call_stop_tone(VOID);


/**
 * @brief 将DTMF数字按键音频发送到语音通道
 * @note 1、该函数用于实现10086之类的语音交互，语音提示后，用户操作按键，调用该函数
 *       将按键音频发送到语音通道，该函数只能在通话状态后调用，否则将引起系统异常。
 *       2、dtmfTone只能是TKL_TONE_DTMF_0 ~ TKL_TONE_DTMF_STAR中的一种。
 *
 * @param dtmfTone 拨号按键
 * @param duration 音频持续时长
 *
 * @return 0 成功
 *        -1 dtmfTone 类型错误
 *        -2 分配内存失败
 *    OPRT_TIMEOUT 获取信号量超时
 *    OPRT_BASE_OS_ADAPTER_REG_NULL_ERROR 函数未适配
 */
OPERATE_RET tkl_cellular_call_dtmf2voice(TUYA_TONE_TYPE_E dtmfTone, UINT_T duration);



/**
 * @brief 设置语音通话音频的音量
 * @param vol （0~100）
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_call_set_voice_vol(INT_T vol);


/**
 * @brief 获取语音通话音频的音量
 * @param vol （0~100）
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_call_get_voice_vol(PINT_T vol);


/**
 * @brief 通话过程中，上行音频是否设置静音
 * When the current voice call is finished, the property will be kept.
 * 当前的语音呼叫结束后，这个属性会被保存下来。下次再进行语音通话的过程中，如果
 * 上次被设置为静音，则继续静音。但是重启后，会被默认恢复成非静音
 * @param TRUE for mute uplink of voice call
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_call_set_voice_mute(BOOL_T mute);


/**
 * @brief 获取通话过程中的上行声音的静音状态。
 * @param mute : TRUE 静音
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_call_get_voice_mute(PBOOL_T mute);

/**
 * @brief 注册DMTF侦测回调函数
 * @param cb :  回调函数
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_call_reg_KTDetect(TKL_CELLULAR_CALL_KTDETECH_CB cb);

/**
 * @brief 控制DTMF侦测功能是否使能，系统默认未使能。（开启这个功能，音频相关业务过程中，会增加系统负荷）
 * @param enable : TRUE 使能 FALSE 禁止
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_call_ctrl_KTDetect(BOOL_T enable);

#ifdef __cplusplus
}
#endif

#endif
