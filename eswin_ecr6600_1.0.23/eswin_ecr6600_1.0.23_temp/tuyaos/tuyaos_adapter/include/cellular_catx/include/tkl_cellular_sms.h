/**
 * @file tkl_cellular_sms.h
 * @author www.tuya.com
 * @brief 蜂窝模组短信API实现接口。
 *
 * @copyright Copyright (c) tuya.inc 2021
 */

#ifndef __TKL_CELLULAR_SMS_H__
#define __TKL_CELLULAR_SMS_H__

#include <stdint.h>
#include "tuya_cloud_types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 收发短信时，手机号码最大长度
 */
#define TUYA_SMS_PHONENUM_LEN_MAX    42

/**
 * @brief 短信内容最大长度
 */
#define TUYA_SMS_MSG_LEN_MAX    160

/**
 * @brief 短信内容编码格式
 */
typedef enum{
    TUYA_SMS_ISO8859_1,
    TUYA_SMS_UTF8,
    TUYA_SMS_UTF16BE,
    TUYA_SMS_UTF16LE,
    TUYA_SMS_GSM,
    TUYA_SMS_CP936   //GBK编码
} TUYA_CELLULAR_SMS_ENCODE_E;

/**
 * @brief 发送短信结构体定义
 */
typedef struct
{
    CHAR_T phone[TUYA_SMS_PHONENUM_LEN_MAX];  /*< 接收短信的手机号 */
    UINT16_T msg_len;                        /*< 短信长度 */
    UINT16_T msg[TUYA_SMS_MSG_LEN_MAX];     /*< 短信内容 */
    TUYA_CELLULAR_SMS_ENCODE_E   sms_encode;
} TUYA_CELLULAR_SMS_SEND_T;

/**
 * @brief 短信时间戳信息结构体定义
 */
typedef struct
{
    UINT16_T year;  /*< 年 */
    UINT8_T month;  /*< 月 */
    UINT8_T day;    /*< 日 */
    UINT8_T hour;   /*< 时 */
    UINT8_T minute; /*< 分 */
    UINT8_T second; /*< 秒 */
    UINT8_T zone;   /*< 时区 */
} TUYA_CELLULAR_SMS_TIMESTAMP_T;

/**
 * @brief 接收短信结构体定义
 */
typedef struct
{
    CHAR_T phone[TUYA_SMS_PHONENUM_LEN_MAX];  /*< 发送短信的手机号码 */
    TUYA_CELLULAR_SMS_TIMESTAMP_T  date;       /*< 短信接收时间 */
    INT_T msg_len;                             /*< 短信内容长度 */
    CHAR_T msg[TUYA_SMS_MSG_LEN_MAX];          /*< 短信内容 */
} TUYA_CELLULAR_SMS_RECV_T;



/**
 * @brief 短信接收回调函数接口原型
 * @note 回调函数中不要调用阻塞性函数，msg结构体内存由系统
 *       负责分配和释放。
 * @param simId sim卡ID
 * @param msg 短信结构体
 * @return 无
 */
typedef VOID_T (*TUYA_CELLULAR_SMS_CB)(UINT8_T sim_id, TUYA_CELLULAR_SMS_RECV_T* msg);


/**
 * @brief 发送短信
 *
 * @param simId sim卡ID
 * @param smsMsg 发送的短信
 *
 * @return  0 发送成功 其它 发送失败
 */
OPERATE_RET tkl_cellular_sms_send(UINT8_T sim_id, TUYA_CELLULAR_SMS_SEND_T* sms_msg);

/**
 * @brief 注册短信接收回调函数
 * @note 该函数需在SIM卡激活前注册，否则可能引起异常或短信丢失。
 *
 * @param callback 短信接收回调函数
 *
 * @return  0 注册成功 其它 注册失败
 */
OPERATE_RET tkl_cellular_sms_recv_cb_register(TUYA_CELLULAR_SMS_CB callback);

/**
 * @brief 设置短信接收时静音
 * @note 默认情况下，收到短信时，会通过扬声器播放提示音。通过该函数，可设置接收
 *       短信时，不播放提示音。
 *
 * @param mute TRUE 静音 FALSE 短信接收时打开提示音
 *
 * @return 0 设置成功 其它 设置失败
 */
OPERATE_RET tkl_cellular_sms_mute(BOOL_T mute);


/**
 * 短信内容编码转换
 *
 * When \a from_charset or \a to_charset is unknown or unsupported,
 * return NULL.
 * The returned pointer is allocated inside, caller should free it.
 *
 * At most \a from_size byte count will be processed. When \a from_size
 * is -1, process till null chracter.
 *
 * null character will always be inserted into the output string.
 * Invalid characters are replaced silently.
 *
 * \a to_size can be NULL in case that the output byte count is not needed.
 *
 * @param from          input string
 * @param from_size     input string byte count
 * @param from_chset    input string charset
 * @param to_chset      output string charset
 * @param to_size       output string byte count
 * @return
 *      - NULL: invalid parameters
 *      - output string
 */
VOID *tkl_cellular_sms_convert_str(CONST VOID *from, INT_T from_size,
    TUYA_CELLULAR_SMS_ENCODE_E from_chset, TUYA_CELLULAR_SMS_ENCODE_E to_chset, INT_T *to_size);

#ifdef __cplusplus
}
#endif

#endif
