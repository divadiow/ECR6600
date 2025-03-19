/**
 * @file tuya_os_adapt_interrupt.c
 * @brief 系统中断操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#if 0
#include "tuya_os_adapt_interrupt.h"
#include "oshal.h"
typedef struct {
	  unsigned int nesting ;
	  unsigned int psw ;
}irq_manage_t;

static const TUYA_OS_INTERRUPT_INTF m_tuya_os_interrupt_intfs = {
	.init = tuya_os_adapt_interrupt_init,
	.enable = tuya_os_adapt_interrupt_enable,
	.disable = tuya_os_adapt_interrupt_disable,
	.release = tuya_os_adapt_interrupt_release,
};

/***********************************************************
*  Function: tuya_os_adapt_interrupt_init
*  Input: irq 	handle of irq
*  Output: irq 	handle of irq
*  Return: 0: success  Other: fail
***********************************************************/
int tuya_os_adapt_interrupt_init(INTERRUPT_HANDLE *irq)
{
	irq_manage_t *ptr = (irq_manage_t *)os_malloc(sizeof(irq_manage_t));
	if (NULL == ptr )
	{
		return -1;
	}
	memset(ptr, 0x00, sizeof(irq_manage_t));

	*irq=(INTERRUPT_HANDLE)ptr;

	return 0;
//	TUYA_OS_IRQ_S *ptr = NULL;
//
//	ptr = (TUYA_OS_IRQ_S *)tuya_os_adapt_system_malloc(sizeof(TUYA_OS_IRQ_S));
//	if (NULL == ptr)
//		return -1;
//	
//	memset(ptr, 0, sizeof(TUYA_OS_IRQ_S));
//	
//	*irq = (void *)ptr;
//	return 0;
}

/***********************************************************
*  Function: tuya_os_adapt_interrupt_enable
*  Input: irq 	handle of irq
*  Output: None
*  Return: 0: success  Other: fail
***********************************************************/
int tuya_os_adapt_interrupt_enable(INTERRUPT_HANDLE irq)
{
//	TUYA_OS_IRQ_S *ptr;

	if (NULL == irq)
		return -1;
	irq_manage_t *ptr =  (irq_manage_t *)irq;

	ptr->nesting--;
	if( ptr->nesting == 0 )
	{
		system_irq_restore(ptr->psw);
	}
	return 0;
//	ptr = (TUYA_OS_IRQ_S *)irq;
//	if (ptr->fiq)
//		portENABLE_FIQ();
//	
//	if (ptr->irq)
//		portENABLE_IRQ();
//
//	return 0;
}

/***********************************************************
*  Function: tuya_os_adapt_interrupt_disable
*  Input: irq 	handle of irq
*  Output: None
*  Return: 0: success  Other: fail
***********************************************************/
int tuya_os_adapt_interrupt_disable(INTERRUPT_HANDLE irq)
{
//	TUYA_OS_IRQ_S *ptr;

	if (NULL == irq)
		return -1;

	irq_manage_t *ptr =  (irq_manage_t *)irq;

	unsigned int psw = system_irq_save();

	if(0 == ptr->nesting)
	{
		ptr->psw=psw;
	}
	ptr->nesting++;

	return 0;

//	ptr = (TUYA_OS_IRQ_S *)irq;
//	ptr->fiq = portDISABLE_FIQ();
//	ptr->irq = portDISABLE_IRQ();
//
//	return 0;
}
/***********************************************************
*  Function: tuya_os_adapt_interrupt_release
*  Input: irq 	handle of irq
*  Output: None
*  Return: 0: success  Other: fail
***********************************************************/
int tuya_os_adapt_interrupt_release(INTERRUPT_HANDLE irq)
{
	if (NULL == irq)
		return 0;

	os_free((irq_manage_t *)irq);
	//tuya_os_adapt_system_free(irq);
	return 0;
}

int tuya_os_adapt_reg_interrupt_intf(void)
{
    return tuya_os_adapt_reg_intf(INTF_INTERRUPT, &m_tuya_os_interrupt_intfs);
}

#endif
