/**
 * @file filename
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-8-6
 * @version V0.1
 * @par Copyright by http://eswin.com/.
 * @par History 1:
 *      Date:
 *      Version:
 *      Author:
 *      Modification:
 *
 * @par History 2:
 */


#ifndef _COMMON_MACRO_DEF_H
#define _COMMON_MACRO_DEF_H


/*--------------------------------------------------------------------------
* 	                                        	Include files
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                        	Macros
--------------------------------------------------------------------------*/
/** Description of the macro */

#define ABS(x)      	(((x) > 0) ? (x) : (0 - (x)))
#ifndef MIN
#define MIN(x,y)		((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y)		((x)>(y)?(x):(y))
#endif

#define US_TO_32K_CNT(X)	(uint32_t)(((uint64_t)(X) << 15) / 1000000) 
#define CNT_32K_TO_US(X)	(uint32_t)(((uint64_t)(X) * 1000000) >> 15) 

/*--------------------------------------------------------------------------
* 	                                        	Types
--------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */

/*--------------------------------------------------------------------------
* 	                                        	Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                        	Global  Variables
--------------------------------------------------------------------------*/
/**  Description of global variable  */

/*--------------------------------------------------------------------------
* 	                                        	Function Prototypes
--------------------------------------------------------------------------*/



#endif/*_COMMON_MACRO_DEF_H*/

