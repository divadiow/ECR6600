


#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H







#define HM_RET_SUCCESS			0
#define HM_RET_ERROR			-1
#define HM_RET_ERROR_ID		-2
#define HM_RET_ERROR_FULL		-3
#define HM_RET_ERROR_HANDLE		-4


int health_mon_stop(void);
int health_mon_start(void);
int health_mon_update(int task_id);
int health_mon_del(int task_id);
int health_mon_add(int handle);
int health_mon_init(void);
int health_mon_debug(int reset);
int health_monitor_create_by_nv(void);


#endif	/* HEALTH_MONITOR_H */
