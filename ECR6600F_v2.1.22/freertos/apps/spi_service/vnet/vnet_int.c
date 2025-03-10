#include "vnet_int.h"
#include "vnet_register.h"
#include "spi_slave.h"
#include "oshal.h"

int vnet_interrupt_enable(unsigned int int_num)
{
    unsigned int flag = system_irq_save();
    vnet_reg_t *vReg = vnet_reg_get_addr();

    if (int_num >= VNET_INTERRUPT_MAX) {
        system_irq_restore(flag);
        os_printf(LM_APP, LL_ERR, "Not support enable interrupt 0x%x\n", int_num);

        return -1;
    }
    vReg->intMask |= 1 << int_num;

    system_irq_restore(flag);
    return 0;
}

int vnet_interrupt_disable(unsigned int int_num)
{
    unsigned int flag = system_irq_save();
    vnet_reg_t *vReg = vnet_reg_get_addr();

    if (int_num >= VNET_INTERRUPT_MAX) {
        system_irq_restore(flag);
        os_printf(LM_APP, LL_ERR, "Not support disable interrupt 0x%x\n", int_num);
        return -1;
    }

    vReg->intMask &= ~(1 << int_num);
    vReg->intFlag &= ~(1 << int_num);

    system_irq_restore(flag);
    return 0;
}

int vnet_interrupt_handle(unsigned int int_num)
{
    unsigned int flag = system_irq_save();
    vnet_reg_t *vReg = vnet_reg_get_addr();

    if (int_num >= VNET_INTERRUPT_MAX) {
        system_irq_restore(flag);
        os_printf(LM_APP, LL_ERR, "Not support handle interrupt 0x%x\n", int_num);
        return -1;
    }

    if (vReg->intMask & (1 << int_num)) {
        vReg->intFlag |= 1 << int_num;
        spi_slave_tx_host_start();
    }

    system_irq_restore(flag);
    return 0;
}

unsigned int vnet_interrupt_get_addr(void)
{
    unsigned int flag = system_irq_save();
    vnet_reg_t *vReg = vnet_reg_get_addr();

    if (vReg->intFlag != 0) {
        return (unsigned int)&vReg->intFlag;
    }

    system_irq_restore(flag);
    return 0;
}

int vnet_interrupt_get_num(void)
{
    unsigned int flag = system_irq_save();
    vnet_reg_t *vReg = vnet_reg_get_addr();
    int num = 0;

    for (num = 0; num < VNET_INTERRUPT_MAX; num++) {
        if ((vReg->intFlag & (1 << num))) {
            return num;
        }
    }

    system_irq_restore(flag);
    return -1;
}

void vnet_interrupt_clear(int num)
{
    unsigned int flag = system_irq_save();
    vnet_reg_t *vReg = vnet_reg_get_addr();

    vReg->intFlag &= ~(1 << num);

    system_irq_restore(flag);
}

