/**
 * @file tuya_os_adapt_memory.c
 * @brief 内存操作接口封装
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */

#include <string.h>
#include "tkl_memory.h"
#include "tkl_output.h"
#include "tkl_system.h"
//#include <FreeRTOS.h>
#include "oshal.h"


/***********************************************************
*************************micro define***********************
***********************************************************/

/***********************************************************
*************************variable define********************
***********************************************************/


/***********************************************************
*************************function define********************
***********************************************************/
/**
 * @brief tuya_os_adapt_system_malloc用于分配内存
 * 
 * @param[in]       size        需要分配的内存大小
 * @return  分配得到的内存指针
 */
#if HAL_MEM_DEBUG
static int start_record_os_adapt_mem = 0;

void tuya_os_adapt_system_mem_start(void)
{
    start_record_os_adapt_mem = 1;
}
void tuya_os_adapt_system_mem_stop(void)
{
    start_record_os_adapt_mem = 0;
}
int tuya_os_adapt_system_mem_get(void)
{
    return start_record_os_adapt_mem;
}

static int malloc_cnt = 0;
void *__tuya_os_adapt_system_malloc(size_t size,char *file,int line)
#else
void *tkl_system_malloc(const size_t size)
#endif
{
    void *pMalloc;

    //pMalloc = pvPortMalloc(size);
    pMalloc=os_malloc(size);
#if HAL_MEM_DEBUG
    if(start_record_os_adapt_mem) {
        malloc_cnt++;
        if (file) {
            if(strstr(file,"print_") || strstr(file,"parse_") || \
               strstr(file,"cJSON_")) {
                return pMalloc;
            }
        }

    LOG_DEBUG("%s:%d cnt:%d malloc mp:%p reqSize:%d \r\n",file?file:"UNKNOWN",line,malloc_cnt,pMalloc,size);
    }
#endif
    if(pMalloc == NULL) {
        //LOG_ERR("malloc fail, heap left size %d\r\n", tuya_os_adapt_system_getheapsize());
    }
    return pMalloc;
}

/**
 * @brief tuya_os_adapt_system_free用于释放内存
 * 
 * @param[in]       ptr         需要释放的内存指针
 */
#if HAL_MEM_DEBUG
static int free_cnt = 0;
void __tuya_os_adapt_system_free(void      * ptr,char *file,int line)
#else
void tkl_system_free(void* ptr)
#endif
{
    if(ptr == NULL) {
        return;
    }

    //vPortFree(ptr);
    os_free(ptr);

#if HAL_MEM_DEBUG
    if(start_record_os_adapt_mem) {
        free_cnt++;

        // delete cjson print info
        if (file) {
            if(strstr(file,"print_") || strstr(file,"parse_") || \
               strstr(file,"cJSON_")) {
                return;
            }
        }       

        LOG_DEBUG("%s:%d sub_cnt:%d free mp:%p\r\n",file?file:"UNKNOWN",line,(malloc_cnt-free_cnt),ptr);
    }
#endif
}


VOID_T *tkl_system_calloc(size_t nitems, size_t size)
{
    void *addr;
    addr = tkl_system_malloc(nitems * size);
    
    if (addr == NULL) {
        return addr;
    }

    memset(addr, 0, nitems * size);

    return addr;
}


/**
* @brief set memory
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
VOID_T *tkl_system_memset(VOID_T* src, INT_T ch, SIZE_T n)
{
    return memset(src, ch, n);
}

/**
* @brief Alloc memory of system
*
* @param[in] size: memory size
*
* @note This API is used to alloc memory of system.
*
* @return the memory address malloced
*/
VOID_T *tkl_system_memcpy(VOID_T* dst, CONST VOID_T* src, SIZE_T n)
{
    return memcpy(dst, src, n);
}

/**
* @brief Compare the first n bytes of store str1 and store str2
*
* @param[in] str1: point to string1
* @param[in] str2: point to string2
* @param[in] n:    first n bytes
*
* @note This API is used to compare two strings
*
* @return value < 0: str1 is less than str2;
          value > 0: str1 is greater than str2;
          value = 0: str1 is equal to str2;
*/
INT_T tkl_system_memcmp(CONST VOID_T *str1, CONST VOID_T *str2, SIZE_T n)
{
    return memcmp(str1, str2, n);
}

/**
 * @brief Re-allocate the memory
 * 
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 *
 * @return VOID_T
 */
VOID_T *tkl_system_realloc(VOID_T* ptr, size_t size)
{
    if (size == 0) {
        tkl_system_free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return tkl_system_malloc(size);
    }

    VOID_T * new = tkl_system_malloc(size);
    if (new == NULL) {
        return NULL;
    }

    tkl_system_memcpy(new, ptr, size);
    tkl_system_free(ptr);

    return new;
}




