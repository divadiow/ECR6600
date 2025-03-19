/**
 ****************************************************************************************
 *
 * @file rtos_al.h
 *
 * @brief Declaration of RTOS abstraction layer.
 * The functions declared here shall be implemented in the RTOS folder and call the RTOS
 * corresponding functions.
 *
 * Copyright (C) RivieraWaves 2017-2018
 *
 ****************************************************************************************
 */

#ifndef RTOS_AL_H_
#define RTOS_AL_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rtos_def.h"
#include <stdbool.h>
/**
 * RTOS task identifier
 */
enum rtos_task_id {
    IDLE_TASK = 0,
    WIFI_TASK,
    SUPPLICANT_TASK,
    IP_TASK,
    APPLICATION_TASK,
    TG_SEND_TASK,
    DATA_TEST_TASK,
    MAX_TASK,
    UNDEF_TASK = 255,
};



// Forward declarations
struct vif_info_tag;

/*
 * FUNCTIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Get the current RTOS time, in ms.
 *
 * @param[in] isr  Indicate if this is called from ISR.
 *
 * @return The current RTOS time
 ****************************************************************************************
 */
uint32_t rtos_now(bool isr);

/**
 ****************************************************************************************
 * @brief Allocate memory.
 *
 * @param[in] size Size, in bytes, to allocate.
 *
 * @return Address of allocated memory on success and NULL if error occurred.
 ****************************************************************************************
 */
void *rtos_malloc(uint32_t size);

/**
 ****************************************************************************************
 * @brief Free memory.
 *
 * @param[in] ptr Memory buffer to free. MUST have been allocated with @ref rtos_malloc
 ****************************************************************************************
 */
void rtos_free(void *ptr);

/**
 ****************************************************************************************
 * @brief Get HEAP Memory information. (For debug purpose only)
 *
 * @param[out] total_size    Updated with HEAP memory size.
 * @param[out] free_size     Updated with the currently available memory.
 * @param[out] min_free_size Updated with the lowest level of free memory reached.
 ****************************************************************************************
 */
void rtos_heap_info(int *total_size, int *free_size, int *min_free_size);

/**
 ****************************************************************************************
 * @brief Create a RTOS task.
 *
 * @param[in] func Pointer to the task function
 * @param[in] name Name of the task
 * @param[in] task_id ID of the task
 * @param[in] stack_depth Required stack depth for the task
 * @param[in] params Pointer to private parameters of the task function, if any
 * @param[in] prio Priority of the task
 * @param[out] task_handle Handle of the task, that might be used in subsequent RTOS
 *                         function calls
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int rtos_task_create(rtos_task_fct func,
                     const char * const name,
                     enum rtos_task_id task_id,
                     const uint16_t stack_depth,
                     void * const params,
                     rtos_prio prio,
                     rtos_task_handle * const task_handle);
/**
 ****************************************************************************************
 * @brief Create a RTOS message queue.
 *
 * @param[in]  elt_size Size, in bytes, of one queue element
 * @param[in]  nb_elt   Number of element to allocate for the queue
 * @param[out] queue    Update with queue handle on success
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 */
int rtos_queue_create(int elt_size, int nb_elt, rtos_queue *queue);

/**
 ****************************************************************************************
 * @brief Check if a RTOS message queue is empty or not.
 * This function can be called both from an ISR and a task.
 *
 * @param[in]  queue   Queue handle
 *
 * @return true if queue is empty, false otherwise.
 ****************************************************************************************
 */
bool rtos_queue_is_empty(rtos_queue queue);

/**
 ****************************************************************************************
 * @brief Check if a RTOS message queue is full or not.
 * This function can be called both from an ISR and a task.
 *
 * @param[in]  queue   Queue handle
 *
 * @return true if queue is full, false otherwise.
 ****************************************************************************************
 */
bool rtos_queue_is_full(rtos_queue queue);

/**
 ****************************************************************************************
 * @brief Get the number of messages pending a queue.
 * This function can be called both from an ISR and a task.
 *
 * @param[in]  queue   Queue handle
 *
 * @return The number of messages pending in the queue.
 ****************************************************************************************
 */
int rtos_queue_cnt(rtos_queue queue);

/**
 ****************************************************************************************
 * @brief Write a message at the end of a RTOS message queue.
 *
 * @param[in]  queue   Queue handle
 * @param[in]  msg     Message to copy in the queue. (It is assume that buffer is of the
 *                     size specified in @ref rtos_queue_create)
 * @param[in]  timeout Maximum duration to wait, in ms, if queue is full. 0 means do not
 *                     wait and -1 means wait indefinitely.
 * @param[in]  isr     Indicate if this is called from ISR. If set, @p timeout parameter
 *                     is ignored.
 *
 * @return 0 on success and != 0 if error occurred (i.e queue was full and maximum
 * duration has been reached).
 ****************************************************************************************
 */
int rtos_queue_write(rtos_queue queue, void *msg, int timeout, bool isr);

/**
 ****************************************************************************************
 * @brief Read a message from a RTOS message queue.
 *
 * @param[in]  queue   Queue handle
 * @param[in]  msg     Buffer to copy into. (It is assume that buffer is of the
 *                     size specified in @ref rtos_queue_create)
 * @param[in]  timeout Maximum duration to wait, in ms, if queue is empty. 0 means do not
 *                     wait and -1 means wait indefinitely.
 * @param[in]  isr     Indicate if this is called from ISR. If set, @p timeout parameter
 *                     is ignored.
 *
 * @return 0 on success and != 0 if error occurred (i.e queue was empty and maximum
 * duration has been reached).
 ****************************************************************************************
 */
int rtos_queue_read(rtos_queue queue, void *msg, int timeout, bool isr);

/**
 ****************************************************************************************
 * @brief Creates and returns a new semaphore.
 * The "busy" argument indicates whether the semaphore shall be made busy or not right
 * after its creation.
 *
 * @param[out] semaphore Semaphore handle returned by the function
 * @param[in]  busy      Flag indication the state of the semaphore after creation
 *                       (true: busy, false otherwise)
 *
 * @return 0 on success and != 0 otherwise.
 ****************************************************************************************
 */
int rtos_semaphore_create(rtos_semaphore *semaphore, bool busy);
int rtos_semaphore_destroy(rtos_semaphore semaphore);

/**
 ****************************************************************************************
 * @brief Signal the semaphore the handle of which is passed as parameter.
 *
 * @param[in]  semaphore Semaphore handle
 * @param[in]  isr       Indicate if this is called from ISR
 *
 * @return 0 on success and != 0 otherwise.
 ****************************************************************************************
 */
int rtos_semaphore_signal(rtos_semaphore semaphore, bool isr);
int rtos_semaphore_wait(rtos_semaphore semaphore, uint32_t timeout);

/**
 ****************************************************************************************
 * @brief Creates and returns a new mutex.
 *
 * @param[out] mutex Mutex handle returned by the function
 *
 * @return 0 on success and != 0 otherwise.
 ****************************************************************************************
 */
int rtos_mutex_create(rtos_mutex *mutex);

/**
 ****************************************************************************************
 * @brief Lock a mutex.
 *
 * @param[in]  mutex Mutex handle
 ****************************************************************************************
 */
void rtos_mutex_lock(rtos_mutex mutex);

/**
 ****************************************************************************************
 * @brief unLock a mutex.
 *
 * @param[in]  mutex Mutex handle
 ****************************************************************************************
 */
void rtos_mutex_unlock(rtos_mutex mutex);

/**
 ****************************************************************************************
 * @brief Enter a critical section.
 * This function returns the previous protection level that is then used in the
 * @ref rtos_unprotect function call in order to put back the correct protection level
 * when exiting the critical section. This allows nesting the critical sections.
 *
 * @return  The previous protection level
 ****************************************************************************************
 */
uint32_t rtos_protect(void);

/**
 ****************************************************************************************
 * @brief Exit a critical section.
 * This function restores the previous protection level.
 *
 * @param[in]  protect The protection level to restore.
 ****************************************************************************************
 */
void rtos_unprotect(uint32_t protect);

/**
 ****************************************************************************************
 * @brief Launch the RTOS scheduler.
 * This function is supposed not to return as RTOS will switch the context to the highest
 * priority task inside this function.
 ****************************************************************************************
 */
void rtos_start_scheduler(void);

/**
 ****************************************************************************************
 * @brief Init RTOS
 *
 * Initialize RTOS layers before start.
 *
 * @return 0 on success and != 0 if error occurred
 ****************************************************************************************
 */
int rtos_init(void);

/**
 ****************************************************************************************
 * @brief Associate a task id to a task handle
 *
 * Only needed for task that are not created using @ref rtos_task_create.
 *
 * @param[in] task_id Task id to associate. Ignored if >= @ref MAX_TASK
 * @param[in] handle Task handle to associate with the ID.
 *
 ****************************************************************************************
 */
void rtos_trace_update_id(enum rtos_task_id task_id, rtos_task_handle handle);

/**
 ****************************************************************************************
 * @brief Change the priority of a task
 * This function cannot be called from an ISR.
 *
 * @param[in] handle Task handle
 * @param[in] priority New priority to set to the task
 *
 ****************************************************************************************
 */
void rtos_priority_set(rtos_task_handle handle, rtos_prio priority);








#endif // RTOS_H_
