! -----------------------------------------------------------------------------
! irq.S
!
! Copyright (c) 2005-2018, Andes Techonology Corporation
! All rights reserved.
! -----------------------------------------------------------------------------

#include "chip_irqvector.h"
#include "ISR_Support.h"

!********************************************************************************************************
!	Setup HW Interrupt vector handler
!********************************************************************************************************

	.macro  SET_HWISR num, handler
	.global interrupt_HW\num
	.set    interrupt_HW\num, \handler
	.endm

	! Setup the ecr6600 HW interrupt vector handler

	SET_HWISR VECTOR_NUM_WIFI_CPU_SINGLE,	interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_WIFI_TICK_TIMER,	interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_WIFI_SOFT,			interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_WIFI_HOST,			interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_PIT0,      		interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_PIT1,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_SDIO_SLAVE,		interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_WDT, 			    interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_GPIO,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_I2C,       		interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_SPI2,       		interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_SPI_FLASH,			interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_PCU,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_DMA,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_RTC,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_UART0,      		interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_UART1,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_UART2,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_I2S, 			    interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_HASH,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_ECC,       		interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_AES,       		interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_IR,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_SDIO_HOST,			interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_BLE,				interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_BLE_PHY,			interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_BLE_ERROR,      	interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_BMC_PM,			interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_BLE_HOPPING,		interrupt_hipri_vsr
	SET_HWISR VECTOR_NUM_AUX_ADC,		interrupt_hipri_vsr
	
	
!********************************************************************************************************
!       Highest priority HW Interrupt vector service routine (Non-Nested interrupt)
!********************************************************************************************************

interrupt_hipri_vsr:
	! We enter here with the orginal r0~r5 is saved and
	! r0 is HW interrup vector number.

	portSAVE_CALLER

#if ( configSUPPORT_IFC == 1 )
	mfusr   $r2, $IFC_LP
	mfsr	$r1, $ipc
	pushm   $r1, $r2                /* $0 is dummy for 8-byte alignment stack pointer */
#endif

	move    $fp, $sp                        ! save old stack pointer
	la      $sp, xISRStack                  ! switch to interrupt stack

	move    $r1,  $fp
	bal irq_in				! enter irq

	! The entire CPU state is now stashed on the stack,
	! and the stack is also 8-byte alignment.
	! We can call C program based interrupt handler now.
	la      $r1, ISR_TABLE
	lw      $r1, [$r1+($r0<<2)]             ! ISR function pointer
	jral    $r1


	move   $r0,  $fp
	bal irq_out				! enter irq

	move	$sp, $fp						! Move back to the thread stack

#if ( configSUPPORT_IFC == 1 )
	popm    $r1, $r2
	mtusr   $r2, $IFC_LP
#endif

	portRESTORE_CALLER

	iret


!********************************************************************************************************
!       HW Interrupt vector service routine (Nested interrupt)
!********************************************************************************************************

interrupt_comm_vsr:
	! We enter here with the orginal r0~r5 is saved and
	! r0 is HW interrup vector number.

	portSAVE_CALLER

	/* Save system registers */
	mfsr    $r1, $IPC
	mfsr    $r2, $IPSW

#if ( configSUPPORT_IFC == 1 ) && ( configSUPPORT_ZOL == 1)
	pushm	$r1, $r2

	mfusr   $r1, $LB
	mfusr   $r2, $LE
	mfusr   $r3, $LC
	mfusr   $r4, $IFC_LP
	pushm   $r1, $r4
#else
#if ( configSUPPORT_IFC == 1 )
	mfusr   $r3, $IFC_LP
	pushm   $r0, $r3                 /* $0 is dummy for 8-byte alignment stack pointer */
#elif ( configSUPPORT_ZOL == 1 )
	mfusr   $r3, $LB
        mfusr   $r4, $LE
        mfusr   $r5, $LC
	pushm   $r0, $r5                 /* $0 is dummy for 8-byte alignment stack pointer */
#else
	pushm   $r1, $r2
#endif
#endif

	/* Descend interrupt level for nested */
	mfsr    $r1, $PSW
#if ( configSUPPORT_ZOL == 1 )
	/* Also enable ZOL (PSW.AEN) */
	xori    $r1, $r1, #((1 << 13) | (1 << 1))
#else
	addi    $r1, $r1, #-2
#endif
	mtsr    $r1, $PSW

	/* Check reentrant interrupt */
	srli45  $r2, #16
	fexti33 $r2, #0x2                       ! IPSW.CPL
	bnec    $r2, #0x7, 2f                   ! IPSW.CPL != 7, come form ISR, reentrant

	move    $fp, $sp                        ! save old stack pointer
	la      $sp, xISRStack                  ! switch to interrupt stack
2:
	setgie.e                                ! allow nested now

	! The entire CPU state is now stashed on the stack,
	! and the stack is also 8-byte alignment.
	! We can call C program based interrupt handler now.
	la      $r1, ISR_TABLE
	lw      $r1, [$r1+($r0<<2)]             ! ISR function pointer
	jral    $r1                             ! Call ISR

	la      $r1, xISRStack                  ! Check for nested interruption return
	bne     $r1, $sp, 3f                    ! $sp != inetrrupt satck ?

	move    $sp, $fp                        ! Move back to the thread stack
3:
	setgie.d
	dsb

	/* Restore system registers */
#if ( configSUPPORT_IFC == 1 ) && ( configSUPPORT_ZOL == 1)
	popm	$r1, $r4
	mtusr   $r1, $LB
	mtusr   $r2, $LE
	mtusr   $r3, $LC
	mtusr   $r4, $IFC_LP

	popm    $r1, $r2
#else
#if ( configSUPPORT_IFC == 1 )
	popm    $r0, $r3
	mtusr   $r3, $IFC_LP
#elif ( configSUPPORT_ZOL == 1 )
	popm    $r0, $r5
	mtusr   $r3, $LB
    mtusr   $r4, $LE
    mtusr   $r5, $LC
#else
	popm    $r1, $r2
#endif
#endif

	mtsr    $r1, $IPC
	mtsr    $r2, $IPSW

	portRESTORE_CALLER

	iret


!********************************************************************************************************
!       Interrupt default handler
!********************************************************************************************************

	! Default irq handler
default_irq_handler:
	b       default_irq_handler


!********************************************************************************************************
!       Interrupt handler Table
!********************************************************************************************************

	.section .data
	.align  2

	! These tables contain the isr pointers used to deliver interrupts
	.global ISR_TABLE
ISR_TABLE:
	.rept   32
	.long	default_irq_handler
	.endr

	! Interrupt stack
	.section .bss
	.global ISR_STACK_BASE
	.global xISRStack
ISR_STACK_BASE:

	.skip configIRQ_STACK_SIZE
	.align 3
xISRStack:
	.end
