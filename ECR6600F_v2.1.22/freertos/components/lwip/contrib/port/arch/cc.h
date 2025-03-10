/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __CC_H__
#define __CC_H__


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> /* abort */
#include "sys_arch.h"

/* Types based on stdint.h */
typedef uint8_t            u8_t;
typedef int8_t             s8_t;
typedef uint16_t           u16_t;
typedef int16_t            s16_t;
typedef uint32_t           u32_t;
typedef int32_t            s32_t;
typedef uintptr_t          mem_ptr_t;

/* Plaform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x) do{ system_printf x; } while( 0 );

#ifdef CONFIG_SYSTEM_IRQ
#define LWIP_PLATFORM_ASSERT(x) do { os_printf(LM_APP, LL_INFO, "A L:%d F:%s\n", \
                                     __LINE__, __FILE__);    vPortEnterCritical((unsigned int)__FUNCTION__, (unsigned int)__LINE__);  for( ;; ); asm("trap 0");  for( ;; );} while(0)
#else
#define LWIP_PLATFORM_ASSERT(x) do { os_printf(LM_APP, LL_INFO, "A L:%d F:%s\n", \
                                           __LINE__, __FILE__);    vPortEnterCritical(  );  for( ;; ); asm("trap 0");  for( ;; );} while(0)
#endif

#ifdef LWIP_DEBUG
#define LWIP_ERROR(message, expression, handler) do { if (!(expression)) { \
      os_printf(LM_APP, LL_INFO, message"\n"); handler;}} while(0)
#else
    // If LWIP_DEBUG is not set, return the error silently (default LWIP behaviour, also.)
#define LWIP_ERROR(message, expression, handler) do { if (!(expression)) { \
      handler;}} while(0)
#endif // LWIP_DEBUG

/* Define (sn)printf formatters for these lwIP types */
#define X8_F  "02x"
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "lu"
#define S32_F "ld"
#define X32_F "lx"
#define SZT_F "uz"

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#define ALIGNED(n)  __attribute__((aligned (n)))

#define LWIP_RAND() ((u32_t)sys_random())

#ifdef CONFIG_ECR6600
#define __SHAREDRAM __attribute__ ((section("SHAREDRAM")))
#define LWIP_DECLARE_MEMORY_ALIGNED_SHAREDRAM(variable_name, size) u8_t variable_name[size] ALIGNED(4) __SHAREDRAM
#endif

#endif /* __CC_H__ */
