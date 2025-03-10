#ifndef BLE_THREAD_H
#define BLE_THREAD_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/


/* reade only compile flag : start */
extern const volatile char ble_lib_compile_date[];
extern const volatile char ble_lib_compile_time[];
/* reade only compile flag : end */


int ble_main(void);
int ble_destory();
int ble_apps_init(void);

void psm_ble_sw_set_sleep_status(bool sleep_en);
bool psm_ble_sw_get_sleep_status(void);

#if defined(CONFIG_BLE_EXAMPLES_NONE)
#define BLE_APPS_INIT() 
#else
#define BLE_APPS_INIT() ble_apps_init()
#endif

#endif /* BLE_THREAD_H */
