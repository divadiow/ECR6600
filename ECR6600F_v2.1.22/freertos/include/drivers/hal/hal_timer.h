#ifndef HAL_TIMER_H
#define HAL_TIMER_H





int hal_timer_create(int period_us, void (* timer_callback)(void *), void * data, int one_shot);
int hal_timer_delete(int timer_handle);
int hal_timer_start(int timer_handle);
int hal_timer_change_period(int timer_handle, int period_us);
int hal_timer_stop(int timer_handle);
int hal_timer_get_current_time(int timer_handle, unsigned int *us);



#endif /*HAL_TIMER_H*/


