/*
 * FreeRTOS Kernel V10.1.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef ISR_SUPPORT_H
#define ISR_SUPPORT_H

#include "FreeRTOSConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef configSUPPORT_FPU
	#if defined(__NDS32_EXT_FPU_DP__) || defined(__NDS32_EXT_FPU_SP__)
		#define configSUPPORT_FPU 1
	#else
		#define configSUPPORT_FPU 0
	#endif
#endif

#ifndef configSUPPORT_IFC
	#ifdef __NDS32_EXT_IFC__
		#define configSUPPORT_IFC 1
	#else
		#define configSUPPORT_IFC 0
	#endif
#endif

#ifndef configSUPPORT_ZOL
	#define configSUPPORT_ZOL 0
#endif

/*When the kernel update, TCB structure may be changed. So the offset need to be modified*/
#if (configHSP_ENABLE==1)
/*Set the bytes of stack's offset in TCB. Unit: 1 byte*/
#define StackOffset_TCB			48 /*The offset of pxCurrentTCB->pxStack in TCB structure*/

/*Set the offset of top address of stack in TCB. Unit: 1 byte*/
#define EndStackOffset_TCB		52 + configMAX_TASK_NAME_LEN /*The offset of pxCurrentTCB->pxEndOfStack in TCB structure*/
#endif

/* The used register numbers of FPU context */
#if ( configSUPPORT_FPU == 1 )
	#if defined(__NDS32_EXT_FPU_CONFIG_0__)
		#define portFPU_REGS    8
	#elif defined(__NDS32_EXT_FPU_CONFIG_1__)
		#define portFPU_REGS    16
	#elif defined(__NDS32_EXT_FPU_CONFIG_2__)
		#define portFPU_REGS    32
	#elif defined(__NDS32_EXT_FPU_CONFIG_3__)
		#define portFPU_REGS    64
	#endif
#endif

/*-----------------------------------------------------------*/

#ifdef __ASSEMBLER__

	.macro portSAVE_FPU
#if ( configSUPPORT_FPU == 1 )
		addi    $sp, $sp, -8
	#if ( portFPU_REGS > 32 )
		fsdi.bi $fd31, [$sp], -8
		fsdi.bi $fd30, [$sp], -8
		fsdi.bi $fd29, [$sp], -8
		fsdi.bi $fd28, [$sp], -8
		fsdi.bi $fd27, [$sp], -8
		fsdi.bi $fd26, [$sp], -8
		fsdi.bi $fd25, [$sp], -8
		fsdi.bi $fd24, [$sp], -8
		fsdi.bi $fd23, [$sp], -8
		fsdi.bi $fd22, [$sp], -8
		fsdi.bi $fd21, [$sp], -8
		fsdi.bi $fd20, [$sp], -8
		fsdi.bi $fd19, [$sp], -8
		fsdi.bi $fd18, [$sp], -8
		fsdi.bi $fd17, [$sp], -8
		fsdi.bi $fd16, [$sp], -8
	#endif
	#if ( portFPU_REGS > 16 )
		fsdi.bi $fd15, [$sp], -8
		fsdi.bi $fd14, [$sp], -8
		fsdi.bi $fd13, [$sp], -8
		fsdi.bi $fd12, [$sp], -8
		fsdi.bi $fd11, [$sp], -8
		fsdi.bi $fd10, [$sp], -8
		fsdi.bi $fd9, [$sp], -8
		fsdi.bi $fd8, [$sp], -8
	#endif
	#if ( portFPU_REGS > 8 )
		fsdi.bi $fd7, [$sp], -8
		fsdi.bi $fd6, [$sp], -8
		fsdi.bi $fd5, [$sp], -8
		fsdi.bi $fd4, [$sp], -8
	#endif
		fsdi.bi $fd3, [$sp], -8
		fsdi.bi $fd2, [$sp], -8
		fsdi.bi $fd1, [$sp], -8
		fsdi    $fd0, [$sp+0]
#endif
	.endm

	.macro portRESTORE_FPU
#if ( configSUPPORT_FPU == 1 )
		fldi.bi $fd0, [$sp], 8
		fldi.bi $fd1, [$sp], 8
		fldi.bi $fd2, [$sp], 8
		fldi.bi $fd3, [$sp], 8
	#if ( portFPU_REGS > 8 )
		fldi.bi $fd4, [$sp], 8
		fldi.bi $fd5, [$sp], 8
		fldi.bi $fd6, [$sp], 8
		fldi.bi $fd7, [$sp], 8
	#endif
	#if ( portFPU_REGS > 16 )
		fldi.bi $fd8, [$sp], 8
		fldi.bi $fd9, [$sp], 8
		fldi.bi $fd10, [$sp], 8
		fldi.bi $fd11, [$sp], 8
		fldi.bi $fd12, [$sp], 8
		fldi.bi $fd13, [$sp], 8
		fldi.bi $fd14, [$sp], 8
		fldi.bi $fd15, [$sp], 8
	#endif
	#if ( portFPU_REGS > 32 )
		fldi.bi $fd16, [$sp], 8
		fldi.bi $fd17, [$sp], 8
		fldi.bi $fd18, [$sp], 8
		fldi.bi $fd19, [$sp], 8
		fldi.bi $fd20, [$sp], 8
		fldi.bi $fd21, [$sp], 8
		fldi.bi $fd22, [$sp], 8
		fldi.bi $fd23, [$sp], 8
		fldi.bi $fd24, [$sp], 8
		fldi.bi $fd25, [$sp], 8
		fldi.bi $fd26, [$sp], 8
		fldi.bi $fd27, [$sp], 8
		fldi.bi $fd28, [$sp], 8
		fldi.bi $fd29, [$sp], 8
		fldi.bi $fd30, [$sp], 8
		fldi.bi $fd31, [$sp], 8
	#endif
#endif
	.endm
/*-----------------------------------------------------------*/

	.macro portSAVE_FPU_CALLER
#if ( configSUPPORT_FPU == 1 )
		addi    $sp, $sp, -8
	#if ( portFPU_REGS > 32 )
		fsdi.bi $fd23, [$sp], -8
		fsdi.bi $fd22, [$sp], -8
		fsdi.bi $fd21, [$sp], -8
		fsdi.bi $fd20, [$sp], -8
		fsdi.bi $fd19, [$sp], -8
		fsdi.bi $fd18, [$sp], -8
		fsdi.bi $fd17, [$sp], -8
		fsdi.bi $fd16, [$sp], -8
	#endif
	#if ( portFPU_REGS > 16 )
		fsdi.bi $fd15, [$sp], -8
		fsdi.bi $fd14, [$sp], -8
		fsdi.bi $fd13, [$sp], -8
		fsdi.bi $fd12, [$sp], -8
		fsdi.bi $fd11, [$sp], -8
	#endif
		fsdi.bi $fd2, [$sp], -8
		fsdi.bi $fd1, [$sp], -8
		fsdi    $fd0, [$sp+0]
#endif
	.endm

	.macro portRESTORE_FPU_CALLER
#if ( configSUPPORT_FPU == 1 )
		fldi.bi $fd0, [$sp], 8
		fldi.bi $fd1, [$sp], 8
		fldi.bi $fd2, [$sp], 8
	#if ( portFPU_REGS > 16 )
		fldi.bi $fd11, [$sp], 8
		fldi.bi $fd12, [$sp], 8
		fldi.bi $fd13, [$sp], 8
		fldi.bi $fd14, [$sp], 8
		fldi.bi $fd15, [$sp], 8
	#endif
	#if ( portFPU_REGS > 32 )
		fldi.bi $fd16, [$sp], 8
		fldi.bi $fd17, [$sp], 8
		fldi.bi $fd18, [$sp], 8
		fldi.bi $fd19, [$sp], 8
		fldi.bi $fd20, [$sp], 8
		fldi.bi $fd21, [$sp], 8
		fldi.bi $fd22, [$sp], 8
		fldi.bi $fd23, [$sp], 8
	#endif
#endif
	.endm
/*-----------------------------------------------------------*/

	.macro portSAVE_CONTEXT
		/* We enter here with the orginal $r28~$r30 (fp/gp/lp) is saved */
		pushm	$r0, $r25
/*
   If HSP mechanism is on. At the begining of context switch, the mhsp_ctl should be turn off.
   Because the sp will change to another task's stack.
   Also, before setting the sp_bound & sp_base, mhsp_ctl need to be turn off
*/
#if ( configHSP_ENABLE ==1 )
		movi    $r0, 0x0
		mtsr    $r0, $HSP_CTL
#endif
		mfsr	$r1, $IPC
		mfsr	$r2, $IPSW

#if ( configSUPPORT_IFC ==1 ) && ( configSUPPORT_ZOL == 1)
		mfusr	$r3, $LB
		mfusr	$r4, $LE
		mfusr	$r5, $LC
		mfusr	$r6, $IFC_LP
		pushm	$r0, $r6		/* $0 is dummy */
#elif ( configSUPPORT_ZOL == 1 )
		mfusr	$r3, $LB
		mfusr	$r4, $LE
		mfusr	$r5, $LC
		pushm	$r1, $r5
#elif ( configSUPPORT_IFC ==1 )
		mfusr   $r3, $IFC_LP
		pushm	$r1, $r3
#else
		pushm	$r0, $r2		/* $0 is dummy */
#endif

		portSAVE_FPU
	.endm

	.macro portRESTORE_CONTEXT
		portRESTORE_FPU

		setgie.d
		dsb

#if ( configSUPPORT_IFC ==1 ) && ( configSUPPORT_ZOL == 1)
		popm	$r0, $r6		/* $0 is dummy */
		mtusr	$r3, $LB
		mtusr	$r4, $LE
		mtusr	$r5, $LC
		mtusr	$r6, $IFC_LP
#elif ( configSUPPORT_ZOL == 1 )
		popm	$r1, $r5
		mtusr	$r3, $LB
		mtusr	$r4, $LE
		mtusr	$r5, $LC
#elif ( configSUPPORT_IFC ==1 )
		popm	$r1, $r3
		mtusr   $r3, $IFC_LP
#else
		popm	$r0, $r2		/* $0 is dummy */
#endif

		mtsr	$r1, $IPC
		mtsr	$r2, $IPSW

#if ( configHSP_ENABLE ==1 )
                la	$r0, pxCurrentTCB
                lwi     $r1, [$r0]
                lwi     $r0, [$r1 + StackOffset_TCB]
		mtsr    $r0, $SP_BOUND
#if( configRECORD_STACK_HIGH_ADDRESS == 1 )
		la      $r0, pxCurrentTCB
		lwi     $r1, [$r0]
		lwi     $r0, [$r1 + EndStackOffset_TCB]
		mtsr    $r0, $SP_BASE
		movi    $r0, 0x85
#else
		movi    $r0, 0x05
#endif
		mtsr    $r0, $HSP_CTL
#endif
		popm	$r0, $r25
		popm	$r28,$r30
	.endm
/*-----------------------------------------------------------*/

	.macro portSAVE_CALLER
		pushm   $r15,$r30               /* full: 16 gpr, reduce: 4 gpr */

		/*Can't use r0 in this function, because the information in it will be used*/
#if ( configHSP_ENABLE ==1 )
		movi	$r1, 0x0
		mtsr	$r1, $HSP_CTL
		la      $r1, xISRStack
#if( configRECORD_STACK_HIGH_ADDRESS == 1 )
		mtsr    $r1, $SP_Base
#endif
		addi	$r1, $r1, -1024
		mtsr    $r1, $SP_BOUND
		movi    $r1, 0x05
		mtsr    $r1, $HSP_CTL
#endif
		portSAVE_FPU_CALLER
	.endm

	.macro portRESTORE_CALLER
		portRESTORE_FPU_CALLER

#if ( configHSP_ENABLE ==1 )
		movi    $r0, 0x0
		mtsr    $r0, $HSP_CTL
		la      $r0, pxCurrentTCB
		lwi     $r1, [$r0]
		lwi     $r0, [$r1 + StackOffset_TCB]
		mtsr    $r0, $SP_BOUND
#if( configRECORD_STACK_HIGH_ADDRESS == 1 )
		la      $r0, pxCurrentTCB
		lwi     $r1, [$r0]
		lwi     $r0, [$r1 + EndStackOffset_TCB]
		mtsr    $r0, $SP_BASE
		movi	$r0, 0x85
#else
		movi    $r0, 0x05
#endif
		mtsr	$r0, $HSP_CTL
#endif
		popm    $r15,$r30               /* full: 16 gpr, reduce: 4 gpr*/
		popm    $r0, $r5
	.endm

#endif

#ifdef __cplusplus
}
#endif

#endif /* ISR_SUPPORT_H */
