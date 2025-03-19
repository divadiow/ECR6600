#ifndef _TKL_SUBG_RF_H_
#define _TKL_SUBG_RF_H_
#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief radio status enumeration
 */
typedef enum
{
    SUBG_RF_STATE_IDLE = 0,
    SUBG_RF_STATE_TX,
    SUBG_RF_STATE_TX_ING,
    SUBG_RF_STATE_RX,
    SUBG_RF_STATE_RX_ING,
} SUBG_RF_STATE_E;

/**
 * @brief RF send command packet type enumeration
 */
 
typedef enum
{
    SUBG_RF_PACK_DYNAMIC = 0,           //Dynamic length type, mainly for TY_SMESH protocol
    SUBG_RF_PACK_DYNAMIC_WAKEUP,        //Dynamic length type with long preamble
    SUBG_RF_PACK_FIXED,                 //Fixed length type, mainly for RF remote control protocol
    SUBG_RF_PACK_CARRIRE,               //carrier mode
} SUBG_RF_PACK_TYPE_E;

/**
 * @brief Type enumeration of RF event callbacks
 */
typedef enum
{
    SUBG_RF_CB_EVENT_TX_DONE = 0,       //Packet sending complete interrupt event
    SUBG_RF_CB_EVENT_ERROR,             //RF Chip Abnormal Events
} SUBG_RF_CALLBACK_EVENT_E;

/**
 * @brief RF working status enumeration
 */
typedef enum
{
    SUBG_RF_RX_FULL = 0,                //RF full window receive mode
    SUBG_RF_RX_DUTYCYCLE,               //RF Low Power Duty Cycle Receive Mode
    SUBG_RF_IDLE,                       //RF idle state
    SUBG_RF_SLEEP_UNIVERSAL,            //RF Universal Sleep Mode
    SUBG_RF_SLEEP_DETECT_RSSI,          //Sleep Detection Over-the-Air RSSI Threshold Wake Mode for RF Low Power Modes
} SUBG_RF_WORK_STATE_E;


typedef VOID_T (*SUBG_RF_REVIECE)(UINT8_T *rx_buf, UINT8_T buf_len,CHAR_T rx_rssi);
typedef VOID_T (*SUBG_RF_EVENT)(SUBG_RF_CALLBACK_EVENT_E cb_event);


/**
 * @brief rf initialization
 *
 * @param cfg_type: Configuration type
 * @return OPERATE_RET
 */
OPERATE_RET tkl_subg_rf_init(UINT32_T cfg_type);

/**
 * @brief Reset RF to default state
 *
 * @param none
 * @return OPERATE_RET
 */
OPERATE_RET tkl_subg_rf_reinit( VOID_T );

/**
 * @brief get the current working status of rf
 *
 * @param none
 *
 * @return SUBG_RF_STATE_E
 */
SUBG_RF_STATE_E tkl_subg_rf_get_state( VOID_T );

/**
 * @brief rf sends data
 *
 * @param tx_buf send data content
 * @param buf_len send data length
 * @param pack_type send packet type
 * @return OPERATE_RET
 */
OPERATE_RET tkl_subg_send(UINT8_T *tx_buf, UINT8_T buf_len,SUBG_RF_PACK_TYPE_E pack_type );

/**
 * @brief set rf working status
 *
 * @param state working status mode
 *
 * @return set results
 */
OPERATE_RET tkl_subg_rf_goto_state(SUBG_RF_WORK_STATE_E state);

/**
 * @brief get signal quality
 *
 * @param VOID_T
 *
 * @return signal quality
 */
SCHAR_T tkl_subg_rf_rssi_get( VOID_T );

/**
 * @brief set rf transmit power
 *
 * @param rf transmit power size
 *
 * @return OPERATE_RET
 */
OPERATE_RET tkl_subg_rf_set_radio_power(INT8_T radio_power_dbm);

/**
 * @brief get rf transmit power
 *
 * @param VOID_T
 *
 * @return rf transmit power size
 */
INT8_T tkl_subg_rf_get_radio_power( VOID_T );

/**
 * @brief set rf channel
 *
 * @param channel work channel
 *
 * @return VOID_T
 */
VOID_T tkl_subg_rf_set_channel(UINT8_T channel);

/**
 * @brief get rf channels
 *
 * @param VOID_T
 *
 * @return work channel
 */
UINT8_T tkl_subg_rf_get_channel( VOID_T );

/**
 * @brief rf reception processing registration
 *
 * @param fun_cb rf receiving registration function
 * @return VOID_T
 */
VOID_T tkl_subg_rf_rx_register(SUBG_RF_REVIECE fun_cb);

/**
 * @brief rf reception processing registration
 *
 * @param fun_cb rf receiving registration function
 * @return VOID_T
 */
VOID_T tkl_subg_rf_tx_register(SUBG_RF_EVENT fun_cb);


/**
 * @brief set the rf frequency offset according to the temperature the call is called when the normal room temperature is different
 *
 * @param temperature current module working ambient temperature
 *
 * @return VOID_T
 */
VOID_T tkl_subg_rf_set_frequency_deviation(INT16_T temperature);

#ifdef __cplusplus
}
#endif

#endif
