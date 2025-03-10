
#include <stdint.h>
#include <string.h>           // in order to use memset
#include "oshal.h"
#include "hci_interface.h"
#include "tuya_ble_hci.h"


/* HCI packet types */
static tuya_ble_hci_rx_cmd_fn *tuya_ble_hci_rx_cmd_hs_cb;
static void *tuya_ble_hci_rx_cmd_hs_arg;

//static tuya_ble_hci_rx_cmd_fn *tuya_ble_hci_rx_cmd_ll_cb;
//static void *tuya_ble_hci_rx_cmd_ll_arg;

static tuya_ble_hci_rx_acl_fn *tuya_ble_hci_rx_acl_hs_cb;
static void *tuya_ble_hci_rx_acl_hs_arg;

//static tuya_ble_hci_rx_acl_fn *tuya_ble_hci_rx_acl_ll_cb;
//static void *tuya_ble_hci_rx_acl_ll_arg;


void TRACE_PACKET_DEBUG(uint8_t type, uint8_t *buf, uint16_t len)
{
	os_printf(LM_BLE, LL_INFO, " ");

	for(uint16_t i = 0; i < len; i++)
	{
		os_printf(LM_BLE, LL_INFO, "%02x ", *(buf+i));
	}
	os_printf(LM_BLE, LL_INFO, "\r\n");
}


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
int tuya_ble_hci_ll_evt_tx(uint8_t *hci_ev, uint8_t len)
{
	int rc = -1;
	uint8_t pkt_len = len;
	uint8_t *pkt = (uint8_t*) os_malloc(pkt_len);
	if(pkt == NULL)
	{
		return -1;
	}
	memcpy(pkt, hci_ev, pkt_len);
	os_printf(LM_BLE, LL_INFO, "ctrl->host EVT");
	TRACE_PACKET_DEBUG(BLE_HCI_EVT_MSG_TYPE, hci_ev, len);

	if(tuya_ble_hci_rx_cmd_hs_cb != NULL)
		rc = tuya_ble_hci_rx_cmd_hs_cb(pkt, (void *)&pkt_len);

	if (pkt != NULL)
	{
		os_free(pkt);
		pkt = NULL;
	}
	return rc;
}

/**
 * Sends ACL data from controller to host.
 *
 * @param om                    The ACL data packet to send.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_ll_acl_tx(uint8_t *acl_pkt,uint8_t len)
{
	int rc = -1;
	uint8_t pkt_len = len;
	uint8_t *pkt = (uint8_t*) os_malloc(pkt_len);
	if(pkt == NULL)
	{
		return -1;
	}
	memcpy(pkt, acl_pkt, pkt_len);
	os_printf(LM_BLE, LL_INFO, "ctrl->host ACL");
	TRACE_PACKET_DEBUG(BLE_HCI_ACL_MSG_TYPE, acl_pkt, len);

	if(tuya_ble_hci_rx_acl_hs_cb != NULL)
		rc = tuya_ble_hci_rx_acl_hs_cb(pkt, (void *)&pkt_len);

	if (pkt != NULL)
	{
		os_free(pkt);
		pkt = NULL;
	}

	return rc;
}


void tuya_ble_hci_ll_tx(uint8_t type, uint8_t *pkt, uint16_t pkt_len)
{
	if(type == BLE_HCI_EVT_MSG_TYPE)
	{
		tuya_ble_hci_ll_evt_tx(pkt, pkt_len);

	}
	else if(type == BLE_HCI_ACL_MSG_TYPE)
	{
		tuya_ble_hci_ll_acl_tx(pkt, pkt_len);
	}
	else
	{
		os_printf(LM_APP, LL_INFO, "%s [hci type error]\r\n", __func__);
		return;
	}


}

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
int tuya_ble_hci_hs_cmd_tx(uint8_t *cmd, uint16_t len)
{
	os_printf(LM_BLE, LL_INFO, "host->ctrl CMD");
	TRACE_PACKET_DEBUG(BLE_HCI_CMD_MSG_TYPE, cmd, len);

	return hci_adapt_ll_rx(BLE_HCI_CMD_MSG_TYPE, cmd, len);
}


/**
 * Sends ACL data from host to controller.
 *
 * @param om                    The ACL data packet to send.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_hs_acl_tx(uint8_t *acl_pkt, uint16_t len)
{
	os_printf(LM_BLE, LL_INFO, "host->ctrl ACL");
	TRACE_PACKET_DEBUG(BLE_HCI_ACL_MSG_TYPE, acl_pkt, len);

	return hci_adapt_ll_rx(BLE_HCI_ACL_MSG_TYPE, acl_pkt, len);
}


/* Controller Use */
void tuya_ble_hci_cfg_ll(tuya_ble_hci_rx_cmd_fn *cmd_cb, void *cmd_arg,
                         tuya_ble_hci_rx_acl_fn *acl_cb, void *acl_arg)
{
    os_printf(LM_APP, LL_INFO, "tuya_ble_hci_cfg_ll\n");
#if 0
    tuya_ble_hci_rx_cmd_ll_cb   = cmd_cb;
    tuya_ble_hci_rx_cmd_ll_arg  = cmd_arg;
    tuya_ble_hci_rx_acl_ll_cb   = acl_cb;
    tuya_ble_hci_rx_acl_ll_arg  = acl_arg;
#endif
}


/* Host Use */
void tuya_ble_hci_cfg_hs(tuya_ble_hci_rx_cmd_fn *cmd_cb, void *cmd_arg,
                         tuya_ble_hci_rx_acl_fn *acl_cb, void *acl_arg)
{
    os_printf(LM_APP, LL_INFO, "tuya_ble_hci_cfg_hs\n");

    tuya_ble_hci_rx_cmd_hs_cb   = cmd_cb;
    tuya_ble_hci_rx_cmd_hs_arg  = cmd_arg;
    tuya_ble_hci_rx_acl_hs_cb   = acl_cb;
    tuya_ble_hci_rx_acl_hs_arg  = acl_arg;
}



/**
 * Resets the HCI module to a clean state.  Frees all buffers and reinitializes
 * the underlying transport.
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_reset(void)
{
    os_printf(LM_APP, LL_INFO, "tuya_ble_hci_reset\n");
	hci_adapt_reset();
	return 0;
}


/**
 * Init the HCI module
 *
 * @retval 0                    success
 * @retval Other                fail
 *                              A BLE_ERR_[...] error code on failure.
 */
int tuya_ble_hci_init(void)
{
    // Initialize the controller
    os_printf(LM_APP, LL_INFO, "tuya_ble_hci_init\n");
	hci_adapt_register_ll_tx_cb(tuya_ble_hci_ll_tx);
	return 0;
}


/* Deinit the controller and hci interface*/
/* Host Use */
int tuya_ble_hci_deinit(void)
{
    // DeInitialize the controller
    os_printf(LM_APP, LL_INFO, "tuya_ble_hci_deinit\n");
    return 0;
}
