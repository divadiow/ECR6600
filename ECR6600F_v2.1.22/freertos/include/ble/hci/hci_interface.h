/**
 ****************************************************************************************
 *
 * @file hci_interface.h
 *
 * @brief hci interface
 *
 * @par Copyright (C):
 *      ESWIN 2015-2020
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

///UART header: command message type
#define BLE_HCI_CMD_MSG_TYPE                            0x01

///UART header: ACL data message type
#define BLE_HCI_ACL_MSG_TYPE                            0x02

///UART header: Synchronous data message type
#define BLE_HCI_SYNC_MSG_TYPE                           0x03

///UART header: event message type
#define BLE_HCI_EVT_MSG_TYPE                            0x04

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/** @brief the callback of ll send func
 * 	@param[in] hci type
 * 	@param[in] hci ll send packet
 * 	@param[in] packet length
 * 	@return
 */
typedef void (*hci_ll_tx_func)(uint8_t type, uint8_t *pkt, uint16_t pkt_len);


/** @brief  register ll_send_func for HOST
 * 	@param[in] controller send data to HOST func
 *
 * 	@return
 */
void hci_adapt_register_ll_tx_cb(hci_ll_tx_func ll_tx_func);


/** @brief controller receive packet interface
 * 	@param[in] hci type
 * 	@param[in] hci receive packet
 * 	@param[in] packet length
 * 	@return receive success: 0, receive fail: -1
 */
int hci_adapt_ll_rx(uint8_t type, uint8_t *buf, uint16_t len);


/** @brief reset ble stack
 * 	@param[in]
 * 	@return
 */
void hci_adapt_reset(void);

