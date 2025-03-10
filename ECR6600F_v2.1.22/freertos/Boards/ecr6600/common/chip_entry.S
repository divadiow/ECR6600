

!****************************************************************
! INCLUDE ASSEMBLY CONSTANTS
!****************************************************************

#include <nds32_init.inc>
#include "arch_defs.h"
#include "chip_irqvector.h"

!****************************************************************
! Macro for exception/interruption vectors
!****************************************************************

	.macro VECTOR name
	.weak   trap_\name
	.set    trap_\name, default_exception_vsr
	.align 4
__vector_\name:
	pushm    $r0, $r31
	la      $r0, trap_\name
	jr5     $r0
	.endm

	.macro INTERRUPT num
	.align 4
__vector_HW\num:
	.if     \num == VECTOR_NUM_SW
	pushm   $r28,$r30
	la      $fp, FreeRTOS_SWI_Handler
	jr5     $fp
	.else
	.weak   interrupt_HW\num
	.set    interrupt_HW\num, default_interrupt_vsr
	pushm   $r0, $r5
	li      $r0, \num
	la      $r1, interrupt_HW\num
	jr5     $r1
	.endif
	.endm

!****************************************************************
! 	Vector Entry Table
!****************************************************************

	.section .vector_init, "ax"

	.global interrupt_vectors
	.global interrupt_vectors_end

interrupt_vectors:
	! Exception vectors
	b       _start              !  (0) Trap Reset/NMI
	VECTOR  TLB_Fill            !  (1) Trap TLB fill
	VECTOR  PTE_Not_Present     !  (2) Trap PTE not present
	VECTOR  TLB_Misc            !  (3) Trap TLB misc
	VECTOR  TLB_VLPT_Miss       !  (4) Trap TLB VLPT miss
	VECTOR  Machine_Error       !  (5) Trap Machine error
	VECTOR  Debug_Related       !  (6) Trap Debug related
	VECTOR  General_Exception   !  (7) Trap General exception
	VECTOR  Syscall             !  (8) Syscall

	! Interrupt vectors
	.altmacro
	.set    irqno, 0
	.rept   VECTOR_NUMINTRS
	INTERRUPT %irqno
	.set    irqno, irqno+1
	.endr

	.align 4
interrupt_vectors_end:

!******************************************************************
!	Start Entry
!******************************************************************

	.section .text
	.global _start
	.type   _start, @function

_start:
	! Initialize the registers used by the compiler
	nds32_init	! NDS32 startup initial macro in <nds32_init.inc>

	! disable cache
	mfsr    $r0, $CACHE_CTL
	li      $r1, ~(CACHE_CTL_mskIC_EN | CACHE_CTL_mskDC_EN)
	and     $r0, $r0, $r1
	mtsr    $r0, $CACHE_CTL

	! Do system low level setup. It must be a leaf function.
	! bal     _nds32_init_mem

	! System reset handler
	bal     chip_startup

	! Infinite loop, if returned accidently
1:
	b       1b

	/* Default exceptions / interrupts handler */
default_exception_vsr:
default_interrupt_vsr:
die:	b       die