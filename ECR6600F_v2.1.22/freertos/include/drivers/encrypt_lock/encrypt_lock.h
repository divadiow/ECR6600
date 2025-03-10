
#ifndef DRV_ENCRYPT_LOCK_H
#define DRV_ENCRYPT_LOCK_H


#define ENCRYPT_LOCK_RET_SUCCESS			0
#define ENCRYPT_LOCK_RET_ERROR				-1
#define ENCRYPT_LOCK_RET_ENOMEM				-2


int drv_encrypt_lock(void);
int drv_encrypt_unlock(void);

/**
@brief    drv_encrypt_lock_init  init
*/
int drv_encrypt_lock_init(void);

#endif /* DRV_ENCRYPT_LOCK_H */


