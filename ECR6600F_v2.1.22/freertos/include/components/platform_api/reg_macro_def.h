/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name:reg_macro_def.h
 * File Mark:
 * Description:the file mainly contains the macro definitions, variables and operation functions related to the public registers.
 * Others:
 * Version:v1.0
 * Author:taodong
 * Date:2021-05-21
 * History 1:
 *     Date: 20210807
 *     Version: V1.1
 *     Author: liuyong
 *     Modification: add write reg lock interrupt
 * History 2:
  ********************************************************************************/

#ifndef __REG_MACRO_DEF_H__
#define __REG_MACRO_DEF_H__


#include <stdint.h>
#include "portmacro.h"



#define BIT0    	(0x00000001L << 0)
#define BIT1    	(0x00000001L << 1)
#define BIT2    	(0x00000001L << 2)
#define BIT3    	(0x00000001L << 3)
#define BIT4    	(0x00000001L << 4)
#define BIT5    	(0x00000001L << 5)
#define BIT6    	(0x00000001L << 6)
#define BIT7    	(0x00000001L << 7)
#define BIT8    	(0x00000001L << 8)
#define BIT9    	(0x00000001L << 9)
#define BIT10   	(0x00000001L << 10)
#define BIT11   	(0x00000001L << 11)
#define BIT12   	(0x00000001L << 12)
#define BIT13   	(0x00000001L << 13)
#define BIT14   	(0x00000001L << 14)
#define BIT15   	(0x00000001L << 15)
#define BIT16   	(0x00000001L << 16)
#define BIT17   	(0x00000001L << 17)
#define BIT18   	(0x00000001L << 18)
#define BIT19   	(0x00000001L << 19)
#define BIT20   	(0x00000001L << 20)
#define BIT21   	(0x00000001L << 21)
#define BIT22   	(0x00000001L << 22)
#define BIT23   	(0x00000001L << 23)
#define BIT24   	(0x00000001L << 24)
#define BIT25   	(0x00000001L << 25)
#define BIT26   	(0x00000001L << 26)
#define BIT27   	(0x00000001L << 27)
#define BIT28   	(0x00000001L << 28)
#define BIT29   	(0x00000001L << 29)
#define BIT30   	(0x00000001L << 30)
#define BIT31   	(0x00000001L << 31)
#define BITZero    	(0x00000000L)

#ifndef BIT
#define BIT(x) 						(1U << (x))
#endif

#ifndef BIT_CLR
#define BIT_CLR(reg,bit)      		(*(volatile unsigned int*)(reg) = (*(volatile unsigned int*)(reg))&(~(1<<(bit))))
#endif

#ifndef BIT_SET
#define BIT_SET(reg,bit)      		(*(volatile unsigned int*)(reg) = (*(volatile unsigned int*)(reg))|(1<<(bit)))
#endif

#ifndef WRITE_REG
#define WRITE_REG(offset,value) 	(*(volatile unsigned int *)(offset) = (unsigned int)(value))
#endif

#ifndef READ_REG
#define READ_REG(reg)				( *( (volatile unsigned int *) (reg) ) )
#endif

#ifndef WRITE_REG_MASK
#define WRITE_REG_MASK(reg, msk, shift, value) 	(WRITE_REG(reg, (READ_REG(reg) & ~((uint32_t)msk)) | ((uint32_t)value << shift)))
#endif

#ifndef READ_REG_MASK
#define READ_REG_MASK(reg, msk, shift) 			((READ_REG(reg) & ((uint32_t)msk)) >> ((uint32_t)shift))
#endif


#endif
