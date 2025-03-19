/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-01-16
 */
#include <string.h>
#include <easyflash.h>
#include <stdarg.h>
#include "flash.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "chip_memmap.h"


/* default environment variables set for user */
static const ef_env default_env_set[] = {
    {"nv_version","0.01"}
};

QueueHandle_t ef_mutex;


/**
 * Flash port for hardware initialize.
 *
 * @param default_env default ENV set for user
 * @param default_env_size default ENV size
 *
 * @return result
 */
EfErrCode ef_port_init(ef_env const **default_env, signed int *default_env_size) {
    EfErrCode result = EF_NO_ERR;

    *default_env = default_env_set;
    *default_env_size = sizeof(default_env_set) / sizeof(default_env_set[0]);

    ef_mutex = xSemaphoreCreateMutex();

    return result;
}

/**
 * Read data from flash.
 * @note This operation's units is word.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, signed int size) {
    EfErrCode result = EF_NO_ERR;
	
	unsigned int backup_ret = READ_REG(0x202078);

    //EF_ASSERT(size % 4 == 0);

    /* You can add your code under here. */
	
	if((addr > PARTITION_END_ADDR) && (backup_ret == 0x01))
	{
		result = drv_spiflash_read_apb(addr, (unsigned char *)buf, size);
	}
	else
	{
	
		result = drv_spiflash_read(addr, (unsigned char *)buf, size);
	}
	return result;
}

/**
 * Erase data on flash.
 * @note This operation is irreversible.
 * @note This operation's units is different which on many chips.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
EfErrCode ef_port_erase(uint32_t addr, signed int size) {
    EfErrCode result = EF_NO_ERR;

    /* make sure the start address is a multiple of EF_ERASE_MIN_SIZE */
    EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);

    /* You can add your code under here. */
    result = drv_spiflash_erase(addr,size);
    if(result != EF_NO_ERR){
        result = EF_ERASE_ERR;
    }
    return result;
}
/**
 * Write data to flash.
 * @note This operation's units is word.
 * @note This operation must after erase. @see flash_erase.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */
EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, signed int size) {
    EfErrCode result = EF_NO_ERR;

    //EF_ASSERT(size % 4 == 0);
    
    /* You can add your code under here. */
    uint32_t *bufTemp;
	unsigned int backup_ret = READ_REG(0x202078);

    if(size == 0)
        return result;

    if((int)buf >= MEM_BASE_XIP) {
        bufTemp = pvPortMalloc(size);
        if(bufTemp == NULL) {
            return EF_WRITE_ERR;
        }
        memcpy(bufTemp, buf, size);
		
		if(backup_ret == 1)
		{
			WRITE_REG(0x202078, 0x00);
		}
		
        result = drv_spiflash_write(addr, (unsigned char *)bufTemp, size);
		
		if(backup_ret == 1)
		{
			WRITE_REG(0x202078, backup_ret);
		}
		
        vPortFree(bufTemp);
    } 
	else {
		
		if(backup_ret == 1)
		{
			WRITE_REG(0x202078, 0x00);
		}
		
        result = drv_spiflash_write(addr, (unsigned char *)buf, size);
		
		if(backup_ret == 1)
		{
			WRITE_REG(0x202078, backup_ret);
		}
    }
	
    if(result != EF_NO_ERR){
        result = EF_WRITE_ERR;
    }

    return result;
}

/**
 * lock the ENV ram cache
 */
void ef_port_env_lock(void) {
    
    /* You can add your code under here. */
    xSemaphoreTake(ef_mutex, 0xFFFFFFFF);    
}

/**
 * unlock the ENV ram cache
 */
void ef_port_env_unlock(void) {
    
    /* You can add your code under here. */
    xSemaphoreGive(ef_mutex);   
}


/**
 * This function is print flash debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 *
 */
void ef_log_debug(const char *file, const long line, const char *format, ...) {

#ifdef PRINT_DEBUG
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);

    /* You can add your code under here. */
    va_end(args);

#endif

}

/**
 * This function is print flash routine info.
 *
 * @param format output format
 * @param ... args
 */
void ef_log_info(const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);

    /* You can add your code under here. */
    
    va_end(args);
}
/**
 * This function is print flash non-package info.
 *
 * @param format output format
 * @param ... args
 */

void ef_print(const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);

    /* You can add your code under here. */
    system_vprintf(format, args);
    
    va_end(args);
}

