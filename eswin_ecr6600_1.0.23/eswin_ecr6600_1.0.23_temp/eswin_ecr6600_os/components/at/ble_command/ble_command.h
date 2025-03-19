#ifndef __BLE__CMD
#define __BLE__CMD
#include "dce.h"
#include <stdint.h>
/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/*
 * GLOBAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**   @brief reset ble advertising mode and advertising data.
 */
void AT_reset_adv_data(void);

/**   @brief reset ble scan response data.
 */
void AT_reset_scan_rsp_data(void);

/**   @brief set ble advertising mode and advertising data.
 */
void AT_set_adv_data(void);

/**   @brief reset ble scan response data.
 */
void AT_set_scan_rsp_data(void);

/**   @brief start advertising as broadcaster/peripheral role.
 */
void AT_start_advertising(void);

/**   @brief stop advertising as broadcaster/peripheral role.
 */
void AT_stop_advertising(void);

/**   @brief ble start scan as observer/central role.
 */
void AT_start_scan(void);

/**   @brief ble stop scan as observer/central role.
 */
void AT_stop_scan(void);

/**   @brief disconnect current ble connetionble as peripheral role.
 */
void AT_ble_disconnect(void);

/**   @brief the task of network config application.
 */
void AT_test_netcfg_task(void *arg);

/**   @brief update the connect param of current connection.
 */
void AT_connect_param_update(uint16_t intv_min, uint16_t intv_max, uint16_t latency, uint16_t time_out);

/**   @brief get the rssi of connected peer device.
 */
void AT_ble_get_con_rssi(void);

/**   @brief get the rssi of connected peer device with sync way.
 */
void at_init_ble_event_group(void);
void AT_ble_get_con_rssi_sync(void);
void dce_register_ble_commands(dce_t* dce);
dce_result_t dce_handle_SETMAC_command(dce_t *dce,void* group_ctx,int kind,size_t argc,arg_t *argv);
#endif