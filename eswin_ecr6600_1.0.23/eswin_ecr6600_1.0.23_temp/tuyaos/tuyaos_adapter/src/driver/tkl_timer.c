#include "tkl_timer.h"
#include "tkl_memory.h"
#include "tkl_output.h"
#include "oshal.h"
#include "hal_timer.h"
#include "timer.h"

/*============================ MACROS ========================================*/
#define TIMER_DEV_NUM           (DRV_TIMER_MAX - 1)  //phy use 1
#define DEFAUT_PERIOD 1000 //us
/*============================ TYPES =========================================*/

typedef struct {
    TUYA_TIMER_BASE_CFG_T cfg;
    UINT_T us;
} TIMER_CFG_T;

typedef struct {
    uint8_t time_handle; 
    TIMER_CFG_T *timer_cfg;
} TIMER_DEV_T;

/*============================ LOCAL VARIABLES ===============================*/
static TIMER_DEV_T s_timer_dev[TIMER_DEV_NUM] = {
        {DRV_TIMER0, NULL},{DRV_TIMER1, NULL},{DRV_TIMER2, NULL}
};

#define CHECK_TIMER_PARAM(ID) do {                                  \
    if (ID >= TIMER_DEV_NUM) {                                      \
        os_printf(LM_APP, LL_ERR, "error timer id %d\r\n",ID);      \
        return OPRT_INVALID_PARM;                                   \
    }                                                               \
    if (NULL == s_timer_dev[ID].timer_cfg) {                        \
        os_printf(LM_APP, LL_ERR, "timer %d not init\r\n",ID);      \
        return OPRT_INVALID_PARM;                                   \
    }                                                               \
} while (0);

/**
 * @brief timer init
 * 
 * @param[in] timer_id timer id
 * @param[in] cfg timer configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_init(UINT32_T timer_id, TUYA_TIMER_BASE_CFG_T *cfg)
{
    int one_shot;
    int timer_handle;
    if (timer_id >= TIMER_DEV_NUM) {                             
        //os_printf(LM_APP, LL_ERR, "tkl_timer_init(), timer id %d err!\r\n",timer_id);       
        return OPRT_INVALID_PARM;                       
    } 

    if (s_timer_dev[timer_id].timer_cfg == NULL) {
        s_timer_dev[timer_id].timer_cfg = os_malloc(sizeof(TIMER_CFG_T));
        if(NULL == s_timer_dev[timer_id].timer_cfg) {
            return OPRT_MALLOC_FAILED;
        }
    }

    memset(s_timer_dev[timer_id].timer_cfg, 0, sizeof(TIMER_CFG_T));
    s_timer_dev[timer_id].timer_cfg->cfg = *cfg;
    one_shot = (TUYA_TIMER_MODE_ONCE == s_timer_dev[timer_id].timer_cfg->cfg.mode) ? 1 : 0;
    timer_handle = hal_timer_create(DEFAUT_PERIOD, s_timer_dev[timer_id].timer_cfg->cfg.cb,
                s_timer_dev[timer_id].timer_cfg->cfg.args, one_shot);
    if (timer_handle == TIMER_RET_ERROR) {
        //os_printf(LM_APP, LL_ERR, "hal_timer_create() return err!\r\n"); 
        os_free(s_timer_dev[timer_id].timer_cfg);
        s_timer_dev[timer_id].timer_cfg = NULL;
        return OPRT_INVALID_PARM;
    }
    
    s_timer_dev[timer_id].time_handle = timer_handle;
    //os_printf(LM_APP, LL_INFO, "hal_timer_create() return timer_handle is %d!\r\n", timer_handle); 
    
    return OPRT_OK; 
}

/**
 * @brief timer start
 * 
 * @param[in] timer_id timer id
 * @param[in] us when to start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_start(UINT32_T timer_id, UINT_T us)
{
    CHECK_TIMER_PARAM(timer_id);
    s_timer_dev[timer_id].timer_cfg->us = us;
    int ret = TIMER_RET_SUCCESS;

    ret = hal_timer_change_period(s_timer_dev[timer_id].time_handle, s_timer_dev[timer_id].timer_cfg->us);
    if (ret != TIMER_RET_SUCCESS) {
        os_printf(LM_APP, LL_ERR, "hal_timer_change_period() return err (%d)!\r\n", ret);
        return OPRT_COM_ERROR;
    }

    ret = hal_timer_start(s_timer_dev[timer_id].time_handle);
    if (ret != TIMER_RET_SUCCESS) {
        os_printf(LM_APP, LL_ERR, "hal_timer_start() return err (%d)!\r\n", ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief timer stop
 * 
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_stop(UINT32_T timer_id)
{
    CHECK_TIMER_PARAM(timer_id);
    hal_timer_stop(s_timer_dev[timer_id].time_handle);
    return OPRT_OK;
}

/**
 * @brief timer deinit
 * 
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_deinit(UINT32_T timer_id)
{
    CHECK_TIMER_PARAM(timer_id);
    hal_timer_delete(s_timer_dev[timer_id].time_handle); 
    os_free(s_timer_dev[timer_id].timer_cfg);
    s_timer_dev[timer_id].timer_cfg = NULL;
    return OPRT_OK;
}

/**
 * @brief timer get
 * 
 * @param[in] timer_id timer id
 * @param[out] ms timer interval
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_get(UINT32_T timer_id, UINT_T *us)
{
    CHECK_TIMER_PARAM(timer_id);
    *us = s_timer_dev[timer_id].timer_cfg->us;
    return OPRT_OK;
}

/**
 * @brief current timer get
 * 
 * @param[in] timer_id timer id
 * @param[out] us timer 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_get_current_value(TUYA_TIMER_NUM_E timer_id, UINT_T *us)
{
    CHECK_TIMER_PARAM(timer_id);
    OPERATE_RET adapt_ret = OPRT_OK;
   int hal_ret = hal_timer_get_current_time(s_timer_dev[timer_id].time_handle, us);
    if (hal_ret != TIMER_RET_SUCCESS)
    {
        adapt_ret = OPRT_COM_ERROR;
    }


    return adapt_ret;
}


#if 0  //timer test 
#include "tkl_system.h"
#include "cli.h"
static void test_timer_cb(void *args)
{
    int timer_id = (int)args;
    os_printf(LM_APP, LL_ERR, "======timer %d irq=======\r\n",timer_id);
}

static void timer_test_set(int id, TUYA_TIMER_MODE_E mode, int us)
{
    TUYA_TIMER_BASE_CFG_T cfg = {
        .mode = mode,
        .cb = test_timer_cb,
        .args = (void *)id
    };
    tkl_timer_init(id, &cfg);
    tkl_timer_start(id,us);
}

void tkl_timer_test(int timer_id, int period) 
{
    os_printf(LM_APP, LL_ERR, "start timer test timer_id %d\r\n", timer_id);
    os_printf(LM_APP, LL_ERR, "free heap:%d\r\n",tkl_system_get_free_heap_size());
    //timer_test_set(0,TUYA_TIMER_MODE_PERIOD,500000);
    timer_test_set(timer_id,period,500000);
    
    //timer_test_set(1,TUYA_TIMER_MODE_PERIOD,600000);
    //timer_test_set(2,TUYA_TIMER_MODE_PERIOD,700000);

    tkl_system_sleep(2000);
    
    //tkl_timer_deinit(timer_id);
    //tkl_system_sleep(2000);
    //tkl_timer_deinit(1);
    //tkl_system_sleep(2000);
    //tkl_timer_deinit(2);
    //tkl_system_sleep(2000);
    os_printf(LM_APP, LL_ERR, "free heap:%d\r\n",tkl_system_get_free_heap_size());
}

int fhost_iot_timer_x(cmd_tbl_t *t, int argc, char *argv[])
{
    int timer_id = atoi(argv[1]);
    int period = atoi(argv[2]);
	tkl_timer_test(timer_id, period);
    return 0;
}
CLI_CMD(timer_x, fhost_iot_timer_x, "timer testing", "timer_x");
#endif


