!********************************************************************************************************
!                                               FreeRTOS
!
!                               (c) Copyright 2018, Andes Techonology
!                                          All Rights Reserved
!
!                                           NDS32 Generic Port
!                                            GNU C Compiler
!
!********************************************************************************************************
!********************************************************************************************************
! INCLUDE ASSEMBLY CONSTANTS
!********************************************************************************************************

#include "FreeRTOSConfig.h"
#include "ISR_Support.h"

!********************************************************************************************************
! IMPORT and EXPORT
!********************************************************************************************************

	.global vPortStartFirstTask
	.global FreeRTOS_SWI_Handler

!*************************************************************************
!
!	PortMacro Function, Call by C function
!	Used by First start Scheduler
!	Warm up a Initalized Task
!
!*************************************************************************
vPortStartFirstTask:
	/* Use new pxCurrentTCB to get new stack pointer */
	la	$r0, pxCurrentTCB
	lwi	$r1, [$r0]
	lwi	$sp, [$r1]		! $SP = pxCurrentTCB->pxTopOfStack

	portRESTORE_CONTEXT

	/* Return into the first task */
	iret

/******************************************************************************
 * Software interrupt is used to start the scheduler.
 *****************************************************************************/

FreeRTOS_SWI_Handler:

	portSAVE_CONTEXT

	/* Save current task's $SP into TCB */
	la	$r0, pxCurrentTCB       ! pxCurrentTCB->pxTopOfStack = $SP
	lwi	$r1, [$r0]
	swi	$sp, [$r1]

	/* Clear SWI pending */
	movi	$r0, 0x0
	mtsr	$r0, $INT_PEND                  ! clean SWI pending

	bal	vTaskSwitchContext              ! get the highest priority of Task

	/* Use new pxCurrentTCB to get new stack pointer */
	la	$r0, pxCurrentTCB
	lwi	$r1, [$r0]
	lwi	$sp, [$r1]		! $SP = pxCurrentTCB->pxTopOfStack

	portRESTORE_CONTEXT

	iret
