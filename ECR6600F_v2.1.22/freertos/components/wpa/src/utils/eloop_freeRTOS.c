/*
 * Event loop based on select() loop
 * Copyright (c) 2002-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"
#include "utils/list.h"
#include <assert.h>
//#include <FreeRTOS.h>
//#include <task.h>
//#include <timers.h>
#include <stdbool.h>
//#include <semphr.h>

#include "common.h"
#include "wpa_trace.h"
#include "eloop.h"
#include "wpa_debug.h"
#include "oshal.h"
#define US_TIME_UNIT			(1000000)
#define SEC_TO_US(x)			(x * US_TIME_UNIT)

struct eloop_sock {
	int sock;
	void *eloop_data;
	void *user_data;
	eloop_sock_handler handler;
};

struct eloop_timeout {
	struct dl_list list;
	struct os_reltime time;
	void *eloop_data;
	void *user_data;
	eloop_timeout_handler handler;
};

struct eloop_global {
	int max_sock;
	int count;
	struct dl_list timeout;
	struct dl_list execute;
	int signal_count;
	struct eloop_signal *signals;
	int signaled;
	int pending_terminate;
	int terminate;
	struct eloop_timeout *next_timeout;
	int task;
	os_sem_handle_t run_signal;
	os_mutex_handle_t timeout_list_lock;
};

static struct eloop_global	eloop;
//static StaticTimer_t		eloop_timer;
static size_t eloop_rearrange_timeout(void);
os_mutex_handle_t eloop_lock = NULL;

#ifdef CONFIG_NO_STDOUT_DEBUG
//#include "system.h"
#ifdef wpa_printf
#undef wpa_printf
static void wpa_printf(int level, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	//os_vprintf(fmt, ap);
	//os_printf("\n");
	va_end(ap);
}
#endif
#endif

int eloop_init(void)
{
	os_memset(&eloop, 0, sizeof(eloop));

	dl_list_init(&eloop.timeout);
	dl_list_init(&eloop.execute);

	eloop.run_signal = os_sem_create(1,0);
	eloop.timeout_list_lock = os_mutex_create();
    eloop_lock = os_mutex_create();

	/*eloop.task = xTaskGetHandle("wpa_supplicant");

	if (!eloop.task) {
		wpa_printf_error(MSG_ERROR, "eloop: Unable to find wpa_supplicant task.\n");
		return -1;
	}*/

	return 0;
}

#if 0
static bool lock_timeout_list()
{
	const int MAX_TIMEOUT = pdMS_TO_TICKS(100);
	return (xSemaphoreTake(eloop.timeout_list_lock, MAX_TIMEOUT) == pdTRUE);
}

static bool try_lock_timeout_list()
{
	return (xSemaphoreTake(eloop.timeout_list_lock, 0) == pdTRUE);
}

static void unlock_timeout_list()
{
	xSemaphoreGive(eloop.timeout_list_lock);
} 
#endif
static void eloop_run_signal()
{
	os_sem_post(eloop.run_signal);
}

static void eloop_wait_for_signal(size_t max)
{
	os_sem_wait(eloop.run_signal, max);
}

static int os_reltime_before_or_same(struct os_reltime *a,
				    struct os_reltime *b)
{
	return (a->sec < b->sec) ||
		(a->sec == b->sec && a->usec <= b->usec);
}


int eloop_sock_requeue(void)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

int eloop_register_read_sock(int sock, eloop_sock_handler handler,
		void *eloop_data, void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

void eloop_unregister_read_sock(int sock)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
}

int eloop_register_sock(int sock, eloop_event_type type,
		eloop_sock_handler handler,
		void *eloop_data, void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

void eloop_unregister_sock(int sock, eloop_event_type type)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
}

static void eloop_remove_timeout(struct eloop_timeout *timeout)
{
	dl_list_del(&timeout->list);
	wpa_trace_remove_ref(timeout, eloop, timeout->eloop_data);
	wpa_trace_remove_ref(timeout, user, timeout->user_data);
	os_free(timeout);
}

static void eloop_remove_timeout_no_free(struct eloop_timeout *timeout)
{
	dl_list_del(&timeout->list);
	wpa_trace_remove_ref(timeout, eloop, timeout->eloop_data);
	wpa_trace_remove_ref(timeout, user, timeout->user_data);
}


size_t eloop_rearrange_timeout(void)
{
	struct eloop_timeout *eto = NULL, *prev = NULL, *first = NULL;
	struct os_reltime now;

	os_get_reltime(&now);

    os_mutex_lock(eloop_lock, WAIT_FOREVER);

	if (dl_list_empty(&eloop.timeout)) {
        os_mutex_unlock(eloop_lock);
		return 0;
	}

	dl_list_for_each_safe(eto, prev, &eloop.timeout, struct eloop_timeout, list) {
		if (os_reltime_before_or_same(&eto->time, &now)) {
			dl_list_del(&eto->list);
			dl_list_add_tail(&eloop.execute, &eto->list);
		}
	}

	first = dl_list_first(&eloop.timeout, struct eloop_timeout, list);

	os_mutex_unlock(eloop_lock);

	if (first)
		return ((first->time.sec - now.sec) * 1000 +
				(first->time.usec - now.usec + 1) / 1000);

	return 0;
}

void eloop_clear_timeout_list(struct dl_list *list) {
	struct eloop_timeout *eto = NULL;

    os_mutex_lock(eloop_lock, WAIT_FOREVER);

	if (!dl_list_empty(list)) {
		dl_list_for_each(eto, list, struct eloop_timeout, list) {
			if (eto != NULL)
				eloop_remove_timeout(eto);
		}
	}
	dl_list_init(&eloop.execute);
	os_mutex_unlock(eloop_lock);
}

static int eloop_trigger_timeout(void)
{
	struct eloop_timeout *timeout = NULL; //, *prev = NULL;
	struct dl_list *head;
	#if 0
	int executed = 0;

	dl_list_for_each_safe(timeout, prev, &eloop.execute,
		struct eloop_timeout, list) {
        os_mutex_lock(eloop_lock, WAIT_FOREVER);
		eloop_remove_timeout_no_free(timeout);
        os_mutex_unlock(eloop_lock);
        timeout->handler(timeout->eloop_data, timeout->user_data);
        os_free(timeout);
		executed++;
	}
	#endif
	do {
		os_mutex_lock(eloop_lock, WAIT_FOREVER);
		head = &eloop.execute;
		if (dl_list_empty(head)) {
			os_mutex_unlock(eloop_lock);
			break;
		}
		timeout = dl_list_entry(head->next, struct eloop_timeout, list);
		eloop_remove_timeout_no_free(timeout);
		os_mutex_unlock(eloop_lock);
		timeout->handler(timeout->eloop_data, timeout->user_data);
		os_free(timeout);
	}while(1);

	return 0;
}



int eloop_register_timeout(unsigned int secs, unsigned int usecs,
		eloop_timeout_handler handler,
		void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout = NULL, *tmp = NULL;
	int cnt = 0;

	timeout = (struct eloop_timeout *) os_zalloc(sizeof(*timeout));

	if (!timeout) {
		wpa_printf_error(MSG_ERROR, "eloop: Failed to allocate new timeout \n");
		return -1;
	}

	if (os_get_reltime(&timeout->time) < 0) {
        wpa_printf_error(MSG_ERROR, "eloop: Failed to get timeout\n");
		os_free(timeout);
		return -1;
	}

	timeout->eloop_data	= eloop_data;
	timeout->user_data	= user_data;
	timeout->handler	= handler;

    os_mutex_lock(eloop_lock, WAIT_FOREVER);

	if (secs == 0 && usecs == 0) {
		dl_list_add_tail(&eloop.execute, &timeout->list);
		eloop_run_signal();
		os_mutex_unlock(eloop_lock);
		return 0;
	}

	timeout->time.sec	+= secs;
	timeout->time.usec	+= usecs;

	while (timeout->time.usec >= SEC_TO_US(1)) {
		timeout->time.sec++;
		timeout->time.usec -= US_TIME_UNIT;
	}

	if (dl_list_empty(&eloop.timeout)) {
		dl_list_add_tail(&eloop.timeout, &timeout->list);
		eloop_run_signal();
	} else {
		cnt = 0;
		dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
			if (os_reltime_before(&timeout->time, &tmp->time)) {
				dl_list_add(tmp->list.prev, &timeout->list);

				if (cnt == 0) // Re-calcuate how much in sleep
					eloop_run_signal();
			os_mutex_unlock(eloop_lock);
			return 0;
			}
			cnt++;
		}
		dl_list_add_tail(&eloop.timeout, &timeout->list);
	}
	os_mutex_unlock(eloop_lock);

	return 0;
}

int eloop_cancel_timeout(eloop_timeout_handler handler,
		void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *prev;
	int removed = 0;
    //unsigned long flags;

    os_mutex_lock(eloop_lock, WAIT_FOREVER);

	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
		struct eloop_timeout, list) {
		if (timeout->handler == handler &&
			(timeout->eloop_data == eloop_data ||
				eloop_data == ELOOP_ALL_CTX) &&
			(timeout->user_data == user_data ||
				user_data == ELOOP_ALL_CTX)) {
			eloop_remove_timeout(timeout);
			removed++;
		}
	}
	if (!dl_list_empty(&eloop.execute)) {
		dl_list_for_each_safe(timeout, prev, &eloop.execute,
			struct eloop_timeout, list) {
			if (timeout->handler == handler &&
				(timeout->eloop_data == eloop_data ||
					eloop_data == ELOOP_ALL_CTX) &&
				(timeout->user_data == user_data ||
					user_data == ELOOP_ALL_CTX)) {
				eloop_remove_timeout(timeout);
				removed++;
			}
		}
	}
	os_mutex_unlock(eloop_lock);

	return removed;
}

int eloop_cancel_timeout_one(eloop_timeout_handler handler,
		void *eloop_data, void *user_data,
		struct os_reltime *remaining)
{
	struct eloop_timeout *timeout, *prev;
	int removed = 0;
	struct os_reltime now;

    os_mutex_lock(eloop_lock, WAIT_FOREVER);

	os_get_reltime(&now);
	remaining->sec = remaining->usec = 0;

	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
		struct eloop_timeout, list) {
		if (timeout->handler == handler &&
			(timeout->eloop_data == eloop_data) &&
			(timeout->user_data == user_data)) {
			removed = 1;
			if (os_reltime_before(&now, &timeout->time))
				os_reltime_sub(&timeout->time, &now, remaining);
			eloop_remove_timeout(timeout);
			break;
		}
	}
	os_mutex_unlock(eloop_lock);
	return removed;
}

int eloop_is_timeout_registered(eloop_timeout_handler handler,
		void *eloop_data, void *user_data)
{
	struct eloop_timeout *tmp;

    os_mutex_lock(eloop_lock, WAIT_FOREVER);

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
			tmp->eloop_data == eloop_data &&
			tmp->user_data == user_data) {
			os_mutex_unlock(eloop_lock);
			return 1;
		}
	}
	os_mutex_unlock(eloop_lock);
	return 0;
}

int eloop_deplete_timeout(unsigned int req_secs, unsigned int req_usecs,
		eloop_timeout_handler handler, void *eloop_data,
		void *user_data)
{
	struct os_reltime now, requested, remaining;
	struct eloop_timeout *tmp;

    os_mutex_lock(eloop_lock, WAIT_FOREVER);

	//wpa_printf(MSG_INFO, "eloop: %s", __func__);

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
		    tmp->eloop_data == eloop_data &&
		    tmp->user_data == user_data) {
			requested.sec = req_secs;
			requested.usec = req_usecs;
			os_get_reltime(&now);
			os_reltime_sub(&tmp->time, &now, &remaining);
			if (os_reltime_before(&requested, &remaining)) {
				eloop_cancel_timeout(handler, eloop_data,
						     user_data);
				eloop_register_timeout(requested.sec,
						       requested.usec,
						       handler, eloop_data,
						       user_data);
				os_mutex_unlock(eloop_lock);
				return 1;
			}
			os_mutex_unlock(eloop_lock);
			return 0;
		}
	}
	os_mutex_unlock(eloop_lock);
	return -1;
}

#if 0
int eloop_replenish_timeout(unsigned int req_secs, unsigned int req_usecs,
			    eloop_timeout_handler handler, void *eloop_data,
			    void *user_data)
{
	struct os_reltime now, requested, remaining;
	struct eloop_timeout *tmp;
	int ret = -1;

	if (!lock_timeout_list())
		return -1;

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
		    tmp->eloop_data == eloop_data &&
		    tmp->user_data == user_data) {
			requested.sec = req_secs;
			requested.usec = req_usecs;
			os_get_reltime(&now);
			os_reltime_sub(&tmp->time, &now, &remaining);
			if (os_reltime_before(&remaining, &requested)) {
				eloop_cancel_timeout(handler, eloop_data,
						     user_data);
                //TODO: should unlock first.
				eloop_register_timeout(requested.sec,
						       requested.usec,
						       handler, eloop_data,
						       user_data);
				unlock_timeout_list();
				return 1;
			}
			unlock_timeout_list();
			return 0;
		}
	}
	unlock_timeout_list();
	return -1;
}
#endif

int eloop_register_signal(int sig, eloop_signal_handler handler,
		void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

int eloop_register_signal_terminate(eloop_signal_handler handler,
		void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

int eloop_register_signal_reconfig(eloop_signal_handler handler,
		void *user_data)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	return 0;
}

#define SYS_INIT_MAGIC_VALUE  0x5AA56996

// 0:WIFI initialization has not been completed
// 1:WIFI initialization has been completed
unsigned char wifi_system_init_complete(void)
{
	return (unsigned char)(eloop.signaled==SYS_INIT_MAGIC_VALUE);
}

void eloop_run(void)
{
	size_t next;

    // system runs to here indicate that wpa initialization is complete
	// borrow eloop.signaled for initialization flag because it never be used
	eloop.signaled = SYS_INIT_MAGIC_VALUE;
    extern void notify_wifi_ready_event();
    notify_wifi_ready_event();
	
	while (1/*!eloop.terminate && !eloop.pending_terminate*/) {
		eloop_trigger_timeout();
		next = eloop_rearrange_timeout();

		if (!dl_list_empty(&eloop.execute))
			continue;

		if (next == 0)
			next = WAIT_FOREVER;

		eloop_wait_for_signal(next);
	}
	eloop.terminate = 0;
}

void eloop_terminate(void)
{
	wpa_printf(MSG_INFO, "eloop: %s", __func__);
	eloop.terminate = 1;
	eloop_run_signal();
}

void eloop_destroy(void)
{
	struct eloop_timeout *timeout, *prev;
	struct os_reltime now;

	os_get_reltime(&now);
	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
					struct eloop_timeout, list) {
		int sec, usec;
		sec = timeout->time.sec - now.sec;
		usec = timeout->time.usec - now.usec;
		if (timeout->time.usec < now.usec) {
			sec--;
			usec += 1000000;
		}
		wpa_printf(MSG_INFO, "ELOOP: remaining timeout: %d.%06d "
			"eloop_data=%p user_data=%p handler=%p",
			sec, usec, timeout->eloop_data, timeout->user_data,
			timeout->handler);
		wpa_trace_dump_funcname("eloop unregistered timeout handler",
					timeout->handler);
		wpa_trace_dump("eloop timeout", timeout);
		eloop_remove_timeout(timeout);
	}
	os_sem_destroy(eloop.run_signal);
	os_mutex_destroy(eloop.timeout_list_lock);
}

int eloop_terminated(void)
{
	return 0;
}

void eloop_wait_for_read_sock(int sock)
{
}
