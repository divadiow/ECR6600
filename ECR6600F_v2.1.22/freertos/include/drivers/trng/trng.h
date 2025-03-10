/**
*@file	   trng.h
*@brief    Hardware generates true random numbers and pseudo random numbers
*@author   wang chao
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#ifndef __DRV_TRNG_H__
#define __DRV_TRNG_H__

/** @brief    Get a true random number
*   @details  After reading the random number for the first time, and abandon
*   @return   The true random number is 32 bits
*/
unsigned int drv_trng_get(void);


/** @brief    Get a pseudo random number
*   @details  After reading the random number for the first time, and abandon
*   @return   The pseudo random number is 32 bits
*/
unsigned int drv_prng_get(void);

#endif /* __DRV_TRNG_H__ */

