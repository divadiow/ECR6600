/**
 * @file tuya_ble_hci.h
 * @brief 关于Tuya蓝牙协议栈HCI Transport适配接口
 * @author timecheng
 * @copyright Copyright(C),2021-2021, 涂鸦科技 www.tuya.com
 * 
 */
#ifndef _TUYA_HCI_TRANSPORT_H_
#define _TUYA_HCI_TRANSPORT_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

struct os_mbuf;

/**************************     Tuya Ble HCI Transport           **********************************/
/*
                            --------   Tuya HOST STACK    --------
                            
                             HCI CMD                 ACL DATA
                                I           O           IO
                                |           |           |
                                |           |           |
                                |           |           |
                                O           I           IO
                                        HCI EVENT    ACL DATA
                                        
                            --------  CONTROLLER STACK   --------
*/
/***********************    Tuya Ble HCI Transport Interface        *****************************/
#ifndef TUYA_BLE_HCI_CMD_BUF_COUNT
#define TUYA_BLE_HCI_CMD_BUF_COUNT   (2)
#endif

#ifndef TUYA_BLE_HCI_EVT_HI_BUF_COUNT
#define TUYA_BLE_HCI_EVT_HI_BUF_COUNT   (24)/*88CK*/  //to large
#endif

#ifndef TUYA_BLE_HCI_EVT_LO_BUF_COUNT
#define TUYA_BLE_HCI_EVT_LO_BUF_COUNT   (4)
#endif

// ACL Data Buffer Count
#ifndef TUYA_BLE_ACL_BUF_COUNT
#define TUYA_BLE_ACL_BUF_COUNT          (12)//24  0924 MODIFY BY CC
#endif

// ACL Data Buffer Size
#if TUYA_BK_HOST_ALLACATION
#ifndef TUYA_BLE_ACL_BUF_SIZE
#define TUYA_BLE_ACL_BUF_SIZE           48//(255)
#endif
#else
#ifndef TUYA_BLE_ACL_BUF_SIZE
#define TUYA_BLE_ACL_BUF_SIZE           (255)
#endif
#endif

#if TUYA_BK_HOST_ALLACATION
#ifndef TUYA_BLE_HCI_EVT_BUF_SIZE
#define TUYA_BLE_HCI_EVT_BUF_SIZE       (50)
#endif
#else
#ifndef TUYA_BLE_HCI_EVT_BUF_SIZE
#define TUYA_BLE_HCI_EVT_BUF_SIZE       (70)
#endif
#endif

#if TUYA_BK_HOST_ALLACATION
#ifndef TUYA_BLE_HCI_CMD_SZ
#define TUYA_BLE_HCI_CMD_SZ             50
#endif
#else
#ifndef TUYA_BLE_HCI_CMD_SZ
#define TUYA_BLE_HCI_CMD_SZ             (260)
#endif
#endif

#ifndef TUYA_BLE_HCI_DATA_HDR_SZ
#define TUYA_BLE_HCI_DATA_HDR_SZ        (4)
#endif

#define TUYA_BLE_HCI_BUF_EVT_LO         (1)
#define TUYA_BLE_HCI_BUF_EVT_HI         (2)

/* Host-to-controller command. */
#define TUYA_BLE_HCI_BUF_CMD            (3)


/** Callback function types; executed when HCI packets are received. */
typedef int tuya_ble_hci_rx_cmd_fn(uint8_t *cmd, void *arg);
typedef int tuya_ble_hci_rx_acl_fn(void *acl_pkt, void *arg);

/**
 * Sends an HCI event from the controller to the host.
 *
 * @param cmd                   The HCI event to send.  This buffer must be
 *                                  allocated via tuya_ble_hci_buf_alloc().
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_ll_evt_tx(uint8_t *hci_ev, uint8_t len);

/**
 * Sends ACL data from controller to host.
 *
 * @param om                    The ACL data packet to send.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_ll_acl_tx(uint8_t *acl_pkt,uint8_t len);

/**
 * Sends an HCI command from the host to the controller.
 *
 * @param cmd                   The HCI command to send.  This buffer must be
 *                                  allocated via tuya_ble_hci_buf_alloc().
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_hs_cmd_tx(uint8_t *cmd, uint16_t len);

/**
 * Sends ACL data from host to controller.
 *
 * @param om                    The ACL data packet to send.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_hs_acl_tx(uint8_t *acl_pkt, uint16_t len);



/**
 * Configures the HCI transport to operate with a controller.  The transport
 * will execute specified callbacks upon receiving HCI packets from the host.
 *
 * @param cmd_cb                The callback to execute upon receiving an HCI
 *                                  command.
 * @param cmd_arg               Optional argument to pass to the command
 *                                  callback.
 * @param acl_cb                The callback to execute upon receiving ACL
 *                                  data.
 * @param acl_arg               Optional argument to pass to the ACL
 *                                  callback.
 */
void tuya_ble_hci_cfg_ll(tuya_ble_hci_rx_cmd_fn *cmd_cb, void *cmd_arg,
                         tuya_ble_hci_rx_acl_fn *acl_cb, void *acl_arg);

/**
 * Configures the HCI transport to operate with a host.  The transport will
 * execute specified callbacks upon receiving HCI packets from the controller.
 *
 * @param evt_cb                The callback to execute upon receiving an HCI
 *                                  event.
 * @param evt_arg               Optional argument to pass to the event
 *                                  callback.
 * @param acl_cb                The callback to execute upon receiving ACL
 *                                  data.
 * @param acl_arg               Optional argument to pass to the ACL
 *                                  callback.
 */
void tuya_ble_hci_cfg_hs(tuya_ble_hci_rx_cmd_fn *evt_cb, void *evt_arg,
                         tuya_ble_hci_rx_acl_fn *acl_cb, void *acl_arg);

#if 0 // For First Version, we will not do flow control
/**
 * Configures a callback to get executed whenever an ACL data packet is freed.
 * The function is called immediately before the free occurs.
 *
 * @param cb                    The callback to configure.
 * @param arg                   An optional argument to pass to the callback.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              BLE_ERR_UNSUPPORTED if the transport does not
 *                                  support this operation.
 */
int tuya_ble_hci_set_acl_free_cb(os_mempool_put_fn *cb, void *arg);
#endif

/**
 * Resets the HCI module to a clean state.  Frees all buffers and reinitializes
 * the underlying transport.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_reset(void);

/**
 * Init the HCI module
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_init(void);


#ifdef __cplusplus
}
#endif

#endif /* H_HCI_TRANSPORT_ */
