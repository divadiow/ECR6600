/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2014-2019, Armink, <armink.ztl@gmail.com>
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
 * Function: It is an head file for this library. You can see all be called functions.
 * Created on: 2014-09-10
 */


#ifndef EASYFLASH_H_
#define EASYFLASH_H_

#include <ef_cfg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "oshal.h"

//#include "system_log.h"
#ifdef __cplusplus
extern "C" {
#endif



/* EasyFlash debug print function. Must be implement by user. */
#ifdef PRINT_DEBUG
#define EF_DEBUG system_printf
#else
#define EF_DEBUG 
#endif
/* EasyFlash routine print function. Must be implement by user. */
#define EF_INFO system_printf
/* EasyFlash assert for developer. */
#define EF_ASSERT(EXPR)                                                       \
if (!(EXPR))                                                                  \
{                                                                             \
    EF_DEBUG("(%s) has assert failed at %s.\n", #EXPR, __FUNCTION__);         \
    while (1);                                                                \
}

/**
 * ENV version number defined by user.
 * Please change it when your firmware add a new ENV to default_env_set.
 */
#ifndef EF_ENV_VER_NUM
#define EF_ENV_VER_NUM                 0
#endif

/* EasyFlash software version number */
#define EF_SW_VERSION                  "4.0.0"
#define EF_SW_VERSION_NUM              0x40000

typedef struct _ef_env {
    char *key;
    void *value;
    signed int value_len;
} ef_env, *ef_env_t;

/* EasyFlash error code */
typedef enum {
    EF_NO_ERR = 0,
    EF_ERASE_ERR = -1,
    EF_READ_ERR = -2,
    EF_WRITE_ERR = -3,
    EF_ENV_NAME_ERR = -4,
    EF_ENV_NAME_EXIST = -5,
    EF_ENV_FULL = -6,
    EF_ENV_INIT_FAILED = -7,
    EF_PARTITION_STATUS_ERR = -8,    
    EF_READ_ADDR_OVERFLOW = 9,
} EfErrCode;

/* the flash sector current status */
typedef enum {
    EF_SECTOR_EMPTY,
    EF_SECTOR_USING,
    EF_SECTOR_FULL,
} EfSecrorStatus;

/* easyflash.c */
EfErrCode easyflash_init(void);

#ifdef EF_USING_ENV
/* only supported on ef_env.c */
signed int customer_get_env_blob(const char *key, void *value_buf, signed int buf_len, signed int *value_len);
EfErrCode customer_set_env_blob(const char *key, const void *value_buf, signed int buf_len);
EfErrCode customer_del_env(const char *key);
void customer_print_env(void);

/* ef_env.c, ef_env_legacy_wl.c and ef_env_legacy.c */
EfErrCode ef_load_env(void);
void ef_print_env(void);
char *ef_get_env(const char *key);
EfErrCode ef_set_env(const char *key, const char *value);
EfErrCode ef_del_env(const char *key);
EfErrCode ef_save_env(void);
EfErrCode ef_env_set_default(void);
signed int ef_get_env_write_bytes(void);
EfErrCode ef_set_and_save_env(const char *key, const char *value);
EfErrCode ef_del_and_save_env(const char *key);

EfErrCode develop_set_env_blob(const char *key, const void *value_buf, signed int buf_len);
signed int develop_get_env_blob(const char *key, void *value_buf, signed int buf_len, signed int *value_len);
EfErrCode develop_del_env(const char *key);

EfErrCode amt_set_env_blob(const char *key, const void *value_buf, signed int buf_len);
signed int amt_get_env_blob(const char *key, void *value_buf, signed int buf_len, signed int *value_len);
EfErrCode amt_del_env(const char *key);



void backup_recovery(void);
void develop_print_env(void);

//ADD for AES mode to choose read api
#define PARTITION_END_ADDR 			0x1000

#define PARTION_NAME_BOOT			"uboot"
#define PARTION_NAME_CPU			"cpu"
#define PARTION_NAME_MCU			"MCU"
#define PARTION_NAME_OTA_STATUS		"ota_status"
#define PARTION_NAME_CA_CRT			"ca_crt"
#define PARTION_NAME_CLIENT_CRT		"client_crt"
#define PARTION_NAME_CLIENT_KEY		"client_key"
#define PARTION_NAME_NV_CUSTOMER	"nv_customer"
#define PARTION_NAME_NV_DEVELOP		"nv_develop"
#define PARTION_NAME_NV_AMT			"nv_amt"
//#define PARTION_NAME_DATA_CERT		"data_cert"
//#define PARTION_NAME_DATA_CA		"data_ca" // add by wangxia 20200226
//#define PARTION_NAME_DATA_KEY		"data_key" // add by wangxia 20200226
//#define PARTION_NAME_DATA_SSL		"data_ssl"

int partion_info_get(char *key, unsigned int *addr, unsigned int * length);
int partion_init(void);
void partion_print_all(void);
void partion_get_string_part(char *buff, unsigned int len);


EfErrCode ef_port_init(ef_env const **default_env, signed int *default_env_size);
EfErrCode ef_env_init(ef_env const *default_env, signed int default_env_size);
EfErrCode ef_iap_init(void);
EfErrCode ef_log_init(void);


#endif


/* ef_iap.c */
EfErrCode ef_erase_bak_app(signed int app_size);
EfErrCode ef_erase_user_app(uint32_t user_app_addr, signed int user_app_size);
EfErrCode ef_erase_spec_user_app(uint32_t user_app_addr, signed int app_size,
                                 EfErrCode (*app_erase)(uint32_t addr, signed int size));
EfErrCode ef_erase_bl(uint32_t bl_addr, signed int bl_size);
EfErrCode ef_write_data_to_bak(uint8_t *data, signed int size, signed int *cur_size,
                               signed int total_size);
EfErrCode ef_copy_app_from_bak(uint32_t user_app_addr, signed int app_size);
EfErrCode ef_copy_spec_app_from_bak(uint32_t user_app_addr, signed int app_size,
                                    EfErrCode (*app_write)(uint32_t addr, const uint32_t *buf, signed int size));
EfErrCode ef_copy_bl_from_bak(uint32_t bl_addr, signed int bl_size);
uint32_t ef_get_bak_app_start_addr(void);


/* ef_log.c */
EfErrCode ef_log_read(signed int index, uint32_t *log, signed int size);
EfErrCode ef_log_write(const uint32_t *log, signed int size);
EfErrCode ef_log_clean(void);
signed int ef_log_get_used_size(void);


/* ef_utils.c */
uint32_t ef_calc_crc32(uint32_t crc, const void *buf, signed int size);
unsigned char ef_calc_crc8(unsigned int crc, const void *buf, signed int size);

/* ef_port.c */
EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, signed int size);
EfErrCode ef_port_erase(uint32_t addr, signed int size);
EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, signed int size);
void ef_port_env_lock(void);
void ef_port_env_unlock(void);
void ef_log_debug(const char *file, const long line, const char *format, ...);
void ef_log_info(const char *format, ...);
void ef_print(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* EASYFLASH_H_ */
