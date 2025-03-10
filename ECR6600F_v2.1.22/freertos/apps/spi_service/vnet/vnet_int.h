#ifndef __VNET_INTERRUPT_H__
#define __VNET_INTERRUPT_H__

#define INTERRUPT_MAX_NUM 30

typedef enum
{
    VNET_INTERRUPT_LINK_CS,
    // 添加中断号
    // 中断号越小，中断优先级越高，优先响应，但不会中断已处理的中断
    VNET_INTERRUPT_MAX = INTERRUPT_MAX_NUM,
} VNET_INTERRUPT_E;

void vnet_interrupt_clear(int num);
unsigned int vnet_interrupt_get_addr(void);
int vnet_interrupt_get_num(void);
int vnet_interrupt_enable(unsigned int int_num);
int vnet_interrupt_disable(unsigned int int_num);
int vnet_interrupt_handle(unsigned int int_num);

#endif
