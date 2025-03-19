/**
 * @file tkl_cellular_player.h
 * @author www.tuya.com
 * @brief 蜂窝模组基础功能API实现接口。
 *
 * @copyright Copyright (c) tuya.inc 2022
 */
#ifndef __TKL_CELLULAR_PLAYER_H__
#define __TKL_CELLULAR_PLAYER_H__
#include <stdint.h>
#include "tuya_cloud_types.h"
#include "tkl_cellular_call.h"
#ifdef __cplusplus
extern "C" {
#endif



/**
 * \brief audio player event
 *
 * There are no separated event for error. When fatal errors occur, audio
 * player will be stopped automatically.
 */
typedef enum
{
    TKL_AUPLAYER_EVENT_FINISHED = 1, ///< playback finished, or fatal error
} TKL_AUDIO_PLAYER_EVENT_E;


typedef void (*TKL_AUDIO_PLAYER_CB)(void *param, TKL_AUDIO_PLAYER_EVENT_E event);

typedef enum
{
    /**
     * Placeholder for not in playing.
     */
    TKL_AUDEV_PLAY_TYPE_NONE = 0,
    /**
     * Play to local audio path.
     */
    TKL_AUDEV_PLAY_TYPE_LOCAL,
    /**
     * Play to uplink remote during voice call.
     */
    TKL_AUDEV_PLAY_TYPE_VOICE,
    /**
     * Play to local audio path during poc mode.
     */
    TKL_AUDEV_PLAY_TYPE_POC,
} TKL_AUDIO_PLAYER_TYPE_E;

typedef enum
{
    TKL_AUSAMPLE_S16,
} TKL_AUDIO_SAMPLE_FORMAT_T;

/**
 * \brief audio frame
 *
 * When it is used for format only, only \p sampleFormat, \p channelCount
 * and \p sampleRate are used.
 */
typedef struct
{
    /**
     * sample format
     */
    TKL_AUDIO_SAMPLE_FORMAT_T sampleFormat;
    /**
     * channel count, only 1 and 2 are valid now
     */
    UINT8_T channelCount;

    UINT8_T flags;
    /**
     * sample rate in Hz
     */
    UINT_T sampleRate;
    /**
     * time stamp of the beginning of data, in unit of sample count.
     * <tt>time_stamp = sample_stamp/sample_rate</tt>
     */
    UINT_T sampleStamp;
    /**
     * sample pointer, it can be casted to the data type according to \p sampleFormat.
     * For 2 channels, the format is LRLR...
     */
    PUINT_T data;
    /**
     * byte count (not sample count)
     */
    UINT_T bytes;
} TKL_AUDIO_FRAME_T;

typedef enum
{
    TKL_AUFRAME_FLAG_START = (1 << 0), ///< indicate start of stream
    TKL_AUFRAME_FLAG_END = (1 << 1),   ///< indicate end of stream
} TKL_AUDIO_FRAME_FLAGS_T;

typedef struct
{
    /**
     * \brief get audio frame
     *
     * When more data are needed, audio device will call \p get_frame
     * for audio frames.
     *
     * At the end of stream, callee should return empty audio frame
     * with \p TKL_AUFRAME_FLAG_END.
     *
     * \param param     player operation context
     * \param frame     audio frame, filled by callee
     * \return
     *      - true on success
     *      - false on error
     */
    BOOL_T (*get_frame)(PVOID_T param, TKL_AUDIO_FRAME_T *frame);

    /**
     * \brief indicate audio data consumed
     *
     * This will be called by audio device, to indicate byte count consumed
     * in audio frame get by \p get_frame.
     *
     * \param param     player operation context
     * \param bytes     byte count
     */
    VOID (*data_consumed)(PVOID_T param, UINT_T bytes);
    /**
     * \brief handle play event from audio device
     *
     * \param param     player operation context
     * \param event     audio device play event
     */
    VOID (*handle_event)(PVOID_T param, TKL_AUDIO_PLAYER_EVENT_E event);
} TKL_AUDIO_PLAY_OPS_T;

/**
 * \brief audio player status
 */
typedef enum
{
    TKL_AUPLAYER_STATUS_IDLE,     ///< playback not started
    TKL_AUPLAYER_STATUS_PLAY,     ///< playback is started
    TKL_AUPLAYER_STATUS_PAUSE,    ///< playback is paused
    TKL_AUPLAYER_STATUS_FINISHED, ///< playback is finished
} TKL_AUDIO_PLAYER_STATUS_E;

/**
 * \brief audio stream format
 */
typedef enum
{
    TY_AUSTREAM_FORMAT_UNKNOWN, ///< placeholder for unknown format
    TY_AUSTREAM_FORMAT_PCM,     ///< raw PCM data
    TY_AUSTREAM_FORMAT_WAVPCM,  ///< WAV, PCM inside
    TY_AUSTREAM_FORMAT_MP3,     ///< MP3
    TY_AUSTREAM_FORMAT_AMRNB,   ///< AMR-NB
    TY_AUSTREAM_FORMAT_AMRWB,   ///< AMR_WB
    TY_AUSTREAM_FORMAT_SBC,     ///< bt SBC
} TKL_AUDIO_STREAM_FORMAT_E;

/**
 * @brief 音频输入设备类型定义
 */
typedef enum
{
    TKL_AUDEV_INPUT_MAINMIC = 0, /*< 主麦克 */
    TKL_AUDEV_INPUT_AUXMIC = 1,  /*< 辅麦克 */
    TKL_AUDEV_INPUT_DUALMIC = 2, /*< 主麦克 + 辅麦克 */
    TKL_AUDEV_INPUT_HPMIC_L = 3, /*< 头戴左麦克 */
    TKL_AUDEV_INPUT_HPMIC_R = 4, /*< 头戴右麦克 */
} TKL_AUDEV_INPUT_TYPE;

/**
 * @brief 音频输出设备类型定义
 */
typedef enum
{
    TKL_AUDEV_OUTPUT_RECEIVER = 0,  /*< 接收器 */
    TKL_AUDEV_OUTPUT_HEADPHONE = 1, /*< 头戴 */
    TKL_AUDEV_OUTPUT_SPEAKER = 2,   /*< 扬声器 */
} TKL_AUDEV_OUTPUT_TYPE;

/**
 * @brief 播放一个音频文件
 * @param type  audio 播放类型
 * 注意：当类型为TKL_AUDEV_PLAY_TYPE_VOICE时候，音频源只支持8K采样率，8bit/16bit/32bit,
 * 单声道。否则播放的声音异常
 * @param format    音频文件格式
 * @param file_path 音频文件（需要包含完整的路径）
 * @param cb        播放的事件回调函数
 * @param cb_ctx    回调函数的参数
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_start(TKL_AUDIO_PLAYER_TYPE_E type, TKL_AUDIO_STREAM_FORMAT_E format, PCHAR_T file_path,
                                     TKL_AUDIO_PLAYER_CB cb, PVOID_T cb_ctx);



/**
 * @brief 暂停正在播放的音频
 * @param 无
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_pause(VOID);

/**
 * @brief 恢复暂停的的音频
 * @param 无
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_resume(VOID);

/**
 * @brief 停止正在播放的的音频
 * @param 无
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_stop(VOID);

/**
 * @brief 设置音频播放的音量
 * @param vol （0~100）
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_set_vol(INT_T vol);

/**
 * @brief 获取音频播放的音量
 * @param vol （0~100）
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_get_vol(PINT_T vol);

/**
 * @brief 设置音频输出设备的是否静音。这个属性不会被保存到flash中。
 * 每次重启后，都会变成非静音
 * @param mute : TRUE 静音
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_set_mute(BOOL_T mute);

/**
 * @brief 获取音频输出设备的是否静音。
 * @param mute : TRUE 静音
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_get_mute(PBOOL_T mute);


/**
 * @brief 播放一个音频内存
 * @param type  audio 播放类型.
 * 注意：当类型为TKL_AUDEV_PLAY_TYPE_VOICE时候，音频源只支持8K采样率，8bit/16bit/32bit,
 * 单声道。否则播放的声音异常
 * @param format    音频文件格式
 * @param buff 音频内存数据指针
 * @param size 音频数据大小
 * @param cb        播放的事件回调函数
 * @param cb_ctx    回调函数的参数
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_mem_start(TKL_AUDIO_PLAYER_TYPE_E type,
                                         TKL_AUDIO_STREAM_FORMAT_E format, PBYTE_T buff,
                                         UINT_T size,TKL_AUDIO_PLAYER_CB cb, PVOID_T cb_ctx);


/**
 * @brief 设置音频输出设备
 * @param channle TKL_AUDEV_OUTPUT_TYPE
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_set_audio_output(TKL_AUDEV_OUTPUT_TYPE chan);

/**
 * @brief 获取音频输出设备
 * @param channle TKL_AUDEV_OUTPUT_TYPE
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_get_audio_output(TKL_AUDEV_OUTPUT_TYPE *chan);

/**
 * @brief 设置音频输入设备
 * @param channle TKL_AUDEV_INPUT_TYPE
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_set_audio_input(TKL_AUDEV_INPUT_TYPE chan);

/**
 * @brief 获取音频输入设备
 * @param channle TKL_AUDEV_INPUT_TYPE
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_get_audio_input(TKL_AUDEV_INPUT_TYPE *chan);

/**
 * @brief 获取音频当前的播放状态
 * @param status TKL_AUDIO_PLAYER_STATUS_E
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_get_status(TKL_AUDIO_PLAYER_STATUS_E *status);


/**
 * @brief 播放流媒体音频
 * @param type TKL_AUDIO_PLAYER_TYPE_E
 * 注意：当类型为TKL_AUDEV_PLAY_TYPE_VOICE时候，音频源只支持8K采样率，16bit,
 * 单声道。否则播放的声音异常
 * @param playOps 参考 \p TKL_AUDIO_PLAY_OPS_T
 * @param playCtx playOps回调函数的输入参数
 * @param frame 音频帧，只有流信息会被使用到。具体参考 \p TKL_AUDIO_PLAYER_TYPE_E
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_start_stream(TKL_AUDIO_PLAYER_TYPE_E type, CONST TKL_AUDIO_PLAY_OPS_T* playOps,
                                            PVOID_T playCtx, CONST TKL_AUDIO_FRAME_T* frame);

/**
 * @brief 停止流媒体播放
 *
 * @return 0 成功，其他失败
 */
OPERATE_RET tkl_cellular_player_stop_stream(VOID);


#ifdef __cplusplus
}
#endif

#endif
