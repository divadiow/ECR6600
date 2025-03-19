/**
 * @file tuya_os_adapt_queue.c
 * @brief 队列操作接口
 *
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 *
 */

#include "tkl_queue.h"
#include "tkl_system.h"
#include "tkl_memory.h"
#include "tuya_error_code.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "semphr.h"
#include "oshal.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
//typedef struct {
//    xQueueHandle queue;
//} QUEUE_MANAGE, *P_QUEUE_MANAGE;


/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/**
 * @brief create queue
 *
 * @param[out]     queue      queue to be create
 * @param[in]      size       the deep of the queue
 * @return  OPRT_OK: SUCCESS other:fail
 */
OPERATE_RET tkl_queue_create_init(TKL_QUEUE_HANDLE *queue, INT_T msgsize, INT_T msgcount)
{
//    P_QUEUE_MANAGE pQueueManage;

    if (NULL == queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

	*queue=(TKL_QUEUE_HANDLE) os_queue_create(NULL, msgcount, msgsize, 0);

	return ((NULL == *queue) ? OPRT_OS_ADAPTER_QUEUE_CREAT_FAILED : OPRT_OK);

//    *queue = NULL;
//
//    pQueueManage = (P_QUEUE_MANAGE)tuya_os_adapt_system_malloc(sizeof(QUEUE_MANAGE));
//    if (pQueueManage == NULL) {
//        return OPRT_OS_ADAPTER_MALLOC_FAILED;
//    }
//
//    pQueueManage->queue = xQueueCreate(number_of_messages, message_size);
//
//    if (pQueueManage->queue == NULL) {
//        tuya_os_adapt_system_free(pQueueManage);
//        return OPRT_OS_ADAPTER_QUEUE_CREAT_FAILED;
//    } else {
//        *queue = (TKL_QUEUE_HANDLE)pQueueManage;
//    }
//
//    return OPRT_OK;
}

/**
 * @brief free queue
 *
 * @param[in]     queue      queue to be free
 * @return  void
 */
VOID_T tkl_queue_free(CONST TKL_QUEUE_HANDLE queue)
{
//    P_QUEUE_MANAGE pQueueManage;

//    if (NULL == queue) {
//        return OPRT_OS_ADAPTER_INVALID_PARM;
//    }
	if(queue)
	{
		os_queue_destory((os_queue_handle_t) queue);
	}
//	pQueueManage = (P_QUEUE_MANAGE)queue;
//
//    if (uxQueueMessagesWaiting(pQueueManage->queue)) {
//        portNOP();
//    }
//
//    vQueueDelete(pQueueManage->queue);
//    tuya_os_adapt_system_free(pQueueManage);
}

/**
 * @brief post msg to queue in timeout ms
 *
 * @param[in]      queue      queue to post
 * @param[in]      msg        msg to post
 * @param[in]      timeout    max time to wait for msg(ms), TUYA_OS_ADAPT_QUEUE_FOREVER means forever wait
 * @return  int OPRT_OK:success    other:fail
 */
//int tuya_os_adapt_queue_post(TKL_QUEUE_HANDLE queue, void *msg, unsigned int timeout)
//{
//    int ret = pdPASS;
//    P_QUEUE_MANAGE pQueueManage;
//	signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
//    
//    if (NULL == queue) {
//        return OPRT_OS_ADAPTER_INVALID_PARM;
//    }
//
//	pQueueManage = (P_QUEUE_MANAGE)queue;
//    
//    if(FALSE == tuya_os_adapt_system_isrstatus()) {
//        ret = xQueueSend( pQueueManage->queue, &msg, (timeout == TUYA_OS_ADAPT_QUEUE_FOREVER) ? portMAX_DELAY : (timeout/ tuya_os_adapt_get_tickratems()));
//    } else {
//        ret = xQueueSendFromISR( pQueueManage->queue, &msg, &xHigherPriorityTaskWoken);
//        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
//    }
//    
//	return ((pdPASS == ret) ? OPRT_OK : OPRT_OS_ADAPTER_QUEUE_SEND_FAIL);
//}

/**
 * @brief fetch msg from queue in timeout ms
 *
 * @param[in]      queue      queue to post
 * @param[out]     msg        msg to fetch
 * @param[in]      timeout    max time to wait for msg(ms), TUYA_OS_ADAPT_QUEUE_FOREVER means forever wait
 * @return  int OPRT_OK:success    other:fail
 */
//int tuya_os_adapt_queue_fetch(TKL_QUEUE_HANDLE queue, void **msg, unsigned int timeout)
//{
//	int ret = pdTRUE;
//    void *dummyptr;
//    P_QUEUE_MANAGE pQueueManage;
//    
//    if (NULL == queue) {
//        return OPRT_OS_ADAPTER_INVALID_PARM;
//    }
//	
//	pQueueManage = (P_QUEUE_MANAGE)queue;
//	if (NULL == msg) {
//		msg = &dummyptr;
//	}	
//
//    ret = xQueueReceive(pQueueManage->queue, &(*msg), (timeout == TUYA_OS_ADAPT_QUEUE_FOREVER) ? portMAX_DELAY : (timeout / tuya_os_adapt_get_tickratems()));
//	if (pdTRUE != ret) {
//        *msg = NULL;
//	}
//
//	return ((pdTRUE == ret) ? OPRT_OK : OPRT_OS_ADAPTER_QUEUE_RECV_FAIL);
//}

/**
 * @brief fetch msg from queue in timeout ms, copy data to queue
 *
 * @param[in]      queue      queue to post
 * @param[in]      msg        msg to post
 * @param[in]      timeout    max time to wait for msg(ms), TUYA_OS_ADAPT_QUEUE_FOREVER means forever wait
 * @return  int OPRT_OK:success    other:fail
 */
OPERATE_RET tkl_queue_post(CONST TKL_QUEUE_HANDLE queue, VOID_T *data, UINT_T timeout)
{
//    int ret = pdPASS;
//    P_QUEUE_MANAGE pQueueManage;
//	signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    
    if (NULL == queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    if(TKL_QUEUE_WAIT_FROEVER == timeout)
    {
    	timeout = WAIT_FOREVER;
    }
    int ret= os_queue_send((os_queue_handle_t) queue, data, 0, timeout);

    return ((ret < 0)? OPRT_OS_ADAPTER_QUEUE_SEND_FAIL : OPRT_OK);
	

//	pQueueManage = (P_QUEUE_MANAGE)queue;
//    
//    if(FALSE == tuya_os_adapt_system_isrstatus()) {
//        ret = xQueueSend( pQueueManage->queue, msg, (timeout == TUYA_OS_ADAPT_QUEUE_FOREVER) ? portMAX_DELAY : (timeout/ tuya_os_adapt_get_tickratems()));
//    } else {
//        ret = xQueueSendFromISR( pQueueManage->queue, msg, &xHigherPriorityTaskWoken);
//        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
//    }
//    
//	return ((pdPASS == ret) ? OPRT_OK : OPRT_OS_ADAPTER_QUEUE_SEND_FAIL);
}

/**
 * @brief fetch msg from queue in timeout ms, copy data from queue
 *
 * @param[in]      queue      queue to post
 * @param[out]     msg        msg to fetch
 * @param[in]      timeout    max time to wait for msg(ms), TUYA_OS_ADAPT_QUEUE_FOREVER means forever wait
 * @return  int OPRT_OK:success    other:fail
 */
OPERATE_RET tkl_queue_fetch(CONST TKL_QUEUE_HANDLE queue, VOID_T *msg, UINT_T timeout)

{
//	int ret = pdTRUE;
//    P_QUEUE_MANAGE pQueueManage;
    
    if (NULL == queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    if(TKL_QUEUE_WAIT_FROEVER == timeout)
    {
    	timeout = WAIT_FOREVER;
    }
	int ret= os_queue_receive((os_queue_handle_t) queue, msg, 0, timeout);

    return ((ret < 0)? OPRT_OS_ADAPTER_QUEUE_RECV_FAIL : OPRT_OK);
//	pQueueManage = (P_QUEUE_MANAGE)queue;
//
//    ret = xQueueReceive(pQueueManage->queue, msg, (timeout == TUYA_OS_ADAPT_QUEUE_FOREVER) ? portMAX_DELAY : (timeout / tuya_os_adapt_get_tickratems()));
//
//	return ((pdTRUE == ret) ? OPRT_OK : OPRT_OS_ADAPTER_QUEUE_RECV_FAIL);
}


