



#include "oshal.h"
#include "encrypt_lock.h"









static os_mutex_handle_t drv_encrypt_lock_mutex;



int drv_encrypt_lock(void)
{
	return os_mutex_lock(drv_encrypt_lock_mutex, WAIT_FOREVER);	
}


int drv_encrypt_unlock(void)
{
	return os_mutex_unlock(drv_encrypt_lock_mutex);
}



/**
@brief    hash  init
*/
int drv_encrypt_lock_init(void)
{
	if (!drv_encrypt_lock_mutex)
	{
		drv_encrypt_lock_mutex = os_mutex_create();
		if (!drv_encrypt_lock_mutex)
		{
			return ENCRYPT_LOCK_RET_ENOMEM;
		}
	}

	return ENCRYPT_LOCK_RET_SUCCESS;
}




