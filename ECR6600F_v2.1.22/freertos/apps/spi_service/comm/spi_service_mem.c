#include "spi_service_mem.h"
#include "spi_service_main.h"
#include "vnet_register.h"
#include "lwip/netif.h"

#ifdef SPI_SERVICE_MEM_DEBUG_SUPPORT
#include "cli.h"
#endif

#define SPI_SERVICE_RX_MEM_SIZE (16*1024)
#define SPI_SERVICE_TX_MEM_SIZE (20*1024)

uint8_t g_service_rxmem[SPI_SERVICE_RX_MEM_SIZE] __SHAREDRAM;
uint8_t g_service_txmem[SPI_SERVICE_TX_MEM_SIZE] __SHAREDRAM;
struct mblock_free *g_service_fblk[SPI_SERVICE_MEM_MAX];

void spi_service_mem_init(void)
{
    g_service_fblk[SPI_SERVICE_MEM_RX] = (struct mblock_free *)CO_ALIGN4_HI((uint32_t)g_service_rxmem);
    g_service_fblk[SPI_SERVICE_MEM_RX]->free_size = ((uint32_t)&g_service_rxmem[SPI_SERVICE_RX_MEM_SIZE] & (~3)) - (uint32_t)g_service_fblk[SPI_SERVICE_MEM_RX];
    g_service_fblk[SPI_SERVICE_MEM_RX]->next = NULL;
    g_service_fblk[SPI_SERVICE_MEM_RX]->previous = NULL;
    g_service_fblk[SPI_SERVICE_MEM_RX]->corrupt_check = KE_LIST_PATTERN;

    g_service_fblk[SPI_SERVICE_MEM_TX] = (struct mblock_free *)CO_ALIGN4_HI((uint32_t)g_service_txmem);
    g_service_fblk[SPI_SERVICE_MEM_TX]->free_size = ((uint32_t)&g_service_txmem[SPI_SERVICE_TX_MEM_SIZE] & (~3)) - (uint32_t)g_service_fblk[SPI_SERVICE_MEM_TX];
    g_service_fblk[SPI_SERVICE_MEM_TX]->next = NULL;
    g_service_fblk[SPI_SERVICE_MEM_TX]->previous = NULL;
    g_service_fblk[SPI_SERVICE_MEM_TX]->corrupt_check = KE_LIST_PATTERN;
#ifdef CONFIG_VNET_SERVICE
    typedef void *(*macif_spi_buff_alloc)(int memtype, uint32_t size);
    void macif_spi_buff_call(macif_spi_buff_alloc cb);
    macif_spi_buff_call((macif_spi_buff_alloc)spi_service_mem_pool_alloc);

    typedef void *(*rxl_vnet_packet_free)(void *rx_buff);
    void fhost_rx_buf_push_call(rxl_vnet_packet_free call);
    fhost_rx_buf_push_call(spi_service_mem_pool_free);
#endif
}

struct mblock_free *spi_service_mem_range_check(void *memPtr)
{
    ASSERT_ERR(memPtr != NULL);
    if ((uint32_t)g_service_fblk[SPI_SERVICE_MEM_RX] <= (uint32_t)memPtr &&
        (uint32_t)memPtr < (uint32_t)g_service_fblk[SPI_SERVICE_MEM_RX] + SPI_SERVICE_RX_MEM_SIZE) {
        return g_service_fblk[SPI_SERVICE_MEM_RX];
    }

    if ((uint32_t)g_service_fblk[SPI_SERVICE_MEM_TX] <= (uint32_t)memPtr &&
        (uint32_t)memPtr < (uint32_t)g_service_fblk[SPI_SERVICE_MEM_TX] + SPI_SERVICE_TX_MEM_SIZE) {
        return g_service_fblk[SPI_SERVICE_MEM_TX];
    }

    return NULL;
}

void *spi_service_mem_pool_alloc(enum SPI_SERVICE_MEM memtype, uint32_t size)
{
    struct mblock_free *node = NULL;
    struct mblock_free *found = NULL;
    struct mblock_used *alloc = NULL;
    unsigned int totalsize;
    unsigned int flag;

    totalsize = CO_ALIGN4_HI(size) + sizeof(struct mblock_used);
    if (totalsize < sizeof(struct mblock_free)) {
        totalsize = sizeof(struct mblock_free);
    }

    flag = system_irq_save();
    node = g_service_fblk[memtype];
    while (node != NULL) {
        ASSERT_INFO(node->corrupt_check == KE_LIST_PATTERN, node->corrupt_check);
        if (node->free_size < totalsize) {
            node = node->next;
            continue;
        }

        found = node;
        if (node->previous != NULL) {
            if (found->free_size < totalsize + sizeof(struct mblock_free)) {
                totalsize = found->free_size;
            }
            break;
        }

        if (node->free_size >= totalsize + sizeof(struct mblock_free)) {
            break;
        }
        found = NULL;
        node = node->next;
    }

    if (found != NULL) {
        if (found->free_size == totalsize) {
            ASSERT_ERR(found->previous != NULL);
            found->previous->next = found->next;
            if(found->next != NULL) {
                found->next->previous = found->previous;
            }
        }

        found->free_size -= totalsize;
        alloc = (struct mblock_used *) ((uint32_t)found + found->free_size);
        alloc->size = totalsize;
        alloc->corrupt_check = KE_ALLOCATED_PATTERN;
        alloc++;
    }
    system_irq_restore(flag);

    return (void *)alloc;
}

void *spi_service_mem_pool_free(void *memPtr)
{
    struct mblock_free *node = NULL;
    struct mblock_free *preNode = NULL;
    struct mblock_free *nxtNode = NULL;
    struct mblock_free *freed = NULL;
    unsigned int size;
    unsigned int flag;

    ASSERT_ERR(memPtr != NULL);
    node = spi_service_mem_range_check(memPtr);
    if (node == NULL) {
        return memPtr;
    }

    freed = (struct mblock_free *)((unsigned int)memPtr - sizeof(struct mblock_used));
    size = freed->free_size;
    ASSERT_INFO(freed->corrupt_check == KE_ALLOCATED_PATTERN, freed->corrupt_check);

    flag = system_irq_save();
    while (node != NULL) {
        ASSERT_INFO(node->corrupt_check == KE_LIST_PATTERN, node);
        if ((uint32_t)freed == ((uint32_t)node + node->free_size)) {
            node->free_size += size;
            if ((uint32_t)node->next == ((uint32_t)node + node->free_size)) {
                nxtNode = node->next;
                node->free_size += nxtNode->free_size;
                ASSERT_ERR(nxtNode != NULL);
                node->next = nxtNode->next;
                if(nxtNode->next != NULL) {
                    nxtNode->next->previous = node;
                }
            }
            system_irq_restore(flag);
            return NULL;
        } else if ((uint32_t)freed < (uint32_t)node) {
            ASSERT_ERR(preNode != NULL);
            preNode->next = (struct mblock_free*)freed;
            freed->previous = preNode;
            freed->corrupt_check = KE_LIST_PATTERN;
            if (((uint32_t)freed + size) == (uint32_t)node) {
                freed->next = node->next;
                if(node->next != NULL) {
                    node->next->previous = freed;
                }
                freed->free_size = node->free_size + size;
            } else {
                freed->next = node;
                node->previous = freed;
                freed->free_size = size;
            }
            system_irq_restore(flag);
            return NULL;
        }

        preNode = node;
        node = node->next;
    }

    freed->corrupt_check = KE_LIST_PATTERN;
    preNode->next = freed;
    freed->next = NULL;
    freed->previous = preNode;
    freed->free_size = size;

    system_irq_restore(flag);
    return NULL;
}

#if defined CONFIG_VNET_SERVICE || (CONFIG_SPI_MASTER && CONFIG_SPI_REPEATER)
void *spi_service_mem_pbuf_alloc(enum SPI_SERVICE_MEM memtype, uint32_t size)
{
    struct pbuf *p = NULL;
    u16_t payloadLen = (u16_t)(LWIP_MEM_ALIGN_SIZE(PBUF_LINK) + LWIP_MEM_ALIGN_SIZE(size));
    mem_size_t allocLen = (mem_size_t)(LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf)) + payloadLen);

    if ((payloadLen < LWIP_MEM_ALIGN_SIZE(size)) || (allocLen < LWIP_MEM_ALIGN_SIZE(size))) {
        os_printf(LM_APP, LL_ERR, "%s[%d] (0x%x-0x%x-0x%x)Error\n", __FUNCTION__, __LINE__, payloadLen, allocLen, LWIP_MEM_ALIGN_SIZE(size));
        return NULL;
    }

    p = (struct pbuf *)spi_service_mem_pool_alloc(memtype, allocLen);
    if (p == NULL) {
        return NULL;
    }
    p->next = NULL;
    p->payload = LWIP_MEM_ALIGN((void *)((unsigned char *)p + LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf)) + PBUF_LINK));
    p->tot_len = size;
    p->len = size;
    p->type_internal = (unsigned char)PBUF_RAM;
#if defined CONFIG_SPI_MASTER && CONFIG_SPI_REPEATER
    p->flags = PBUF_FLAG_IS_CUSTOM;
#else
    p->flags = 0;
#endif
    p->ref = 1;
    p->if_idx = NETIF_NO_INDEX;
    ASSERT_INFO(((mem_ptr_t)p->payload % MEM_ALIGNMENT) == 0, p->payload);

    return p;
}

void *spi_service_mem_pbuf_free(struct pbuf *p)
{
    if (spi_service_mem_range_check((void *)p) == NULL) {
        return p;
    }

    p->ref -= 1;

    if (p->ref == 0) {
        return spi_service_mem_pool_free(p);
    }

    return NULL;
}
#endif

void *spi_service_smem_rx_alloc(spi_service_ctrl_t *mcfg, spi_service_mem_t *smem)
{
    void *sp = NULL;

    smem->memSlen = smem->memLen = CO_ALIGN2_HI(mcfg->len);
    smem->memType = mcfg->evt;

    switch (mcfg->evt) {
        case SPI_SERVICE_TYPE_STOH:
#ifdef CONFIG_SPI_MASTER
#ifdef CONFIG_SPI_REPEATER
            /* spi repeater alloc mem send to lwip stack */
            sp = spi_service_mem_pbuf_alloc(SPI_SERVICE_MEM_RX, mcfg->len);
            if (sp != NULL) {
                smem->memAddr = (unsigned int)sp;
                smem->memOffset = (unsigned int)(((struct pbuf *)sp)->payload) - (unsigned int)sp;
                break;
            }
#endif /* CONFIG_SPI_REPEATER */
#endif /* CONFIG_SPI_MASTER */
            return NULL;
        case SPI_SERVICE_TYPE_HTOS:
#ifdef CONFIG_SPI_SLAVE
#ifdef CONFIG_VNET_SERVICE
            /* vnet service alloc mem send to wifi */
            sp = (struct pbuf *)spi_service_mem_pbuf_alloc(SPI_SERVICE_MEM_RX, mcfg->len);
            if (sp != NULL) {
                smem->memAddr = (unsigned int)sp;
                smem->memOffset = (unsigned int)(((struct pbuf *)sp)->payload) - (unsigned int)sp;
                break;
            }
#endif
#endif
            return NULL;
        case SPI_SERVICE_TYPE_OTA:
        case SPI_SERVICE_TYPE_MSG:
            sp = spi_service_mem_pool_alloc(SPI_SERVICE_MEM_RX, mcfg->len);
            if (sp != NULL) {
                smem->memAddr = (unsigned int)sp;
                break;
            }
            return NULL;
        case SPI_SERVICE_TYPE_INFO:
        default:
#ifdef CONFIG_SPI_MASTER
#ifdef CONFIG_SPI_REPEATER
            if (mcfg->evt < SPI_SERVICE_TYPE_HTOS) {
                vnet_reg_op_t *vReg = vnet_reg_get_table(mcfg->evt);
                if (vReg != NULL) {
                    smem->memAddr = vReg->regAddr;
                    smem->memType = SPI_SERVICE_TYPE_INFO;
                    return smem;
                }
            }
#endif
#endif
            os_printf(LM_APP, LL_ERR, "%s[%d] Unkown evt 0x%x\n", __FUNCTION__, __LINE__, mcfg->evt);
            return NULL;
    }

    return smem;
}

void *spi_service_smem_rx_free(spi_service_mem_t *smem)
{
    switch (smem->memType) {
        case SPI_SERVICE_TYPE_STOH:
#ifndef SPI_SERVICE_LOOP_TEST
#ifdef CONFIG_SPI_MASTER
#ifdef CONFIG_SPI_REPEATER
            /* lwip stack free the mem */
            break;
#endif
#endif
#endif
#ifdef CONFIG_VNET_SERVICE
{
            struct pbuf *p = (struct pbuf *)smem->memAddr;

            if ((p->flags & PBUF_FLAG_IS_CUSTOM) != 0) {
                struct pbuf_custom *pc = (struct pbuf_custom *)p;
                ASSERT_ERR(pc->custom_free_function != NULL);
                pc->custom_free_function(p);
                break;
            }
}
#endif
            spi_service_mem_pool_free((void *)smem->memAddr);
            break;
        case SPI_SERVICE_TYPE_HTOS:
#ifndef SPI_SERVICE_LOOP_TEST
#ifdef CONFIG_VNET_SERVICE
            /* wifi free the mem */
            break;
#endif
#endif
        case SPI_SERVICE_TYPE_OTA:
        case SPI_SERVICE_TYPE_MSG:
            spi_service_mem_pool_free((void *)smem->memAddr);
            break;
        default:
            break;
    }

    return 0;
}

void *spi_service_smem_tx_alloc(spi_service_ctrl_t *mcfg, spi_service_mem_t *smem)
{
    void *sp = NULL;

    smem->memSlen = smem->memLen = CO_ALIGN2_HI(mcfg->len);
    smem->memType = mcfg->evt;

    switch (mcfg->evt) {
        case SPI_SERVICE_TYPE_STOH:
#ifndef SPI_SERVICE_LOOP_TEST
#ifdef CONFIG_VNET_SERVICE
            break;
#endif
#endif
        case SPI_SERVICE_TYPE_HTOS:
RETRY:
            sp = spi_service_mem_pool_alloc(SPI_SERVICE_MEM_TX, mcfg->len);
            if (sp != NULL) {
                smem->memAddr = (unsigned int)sp;
                return smem;
            } else {
                os_msleep(1);
                goto RETRY;
            }
            break;
        case SPI_SERVICE_TYPE_MSG:
            sp = spi_service_mem_pool_alloc(SPI_SERVICE_MEM_TX, mcfg->len);
            if (sp != NULL) {
                smem->memAddr = (unsigned int)sp;
                return smem;
            }
            break;
        case SPI_SERVICE_TYPE_INFO:
        default:
            if (mcfg->evt < SPI_SERVICE_TYPE_HTOS) {
#ifdef CONFIG_VNET_SERVICE
                vnet_reg_op_t *vReg = vnet_reg_get_table(mcfg->evt);
                if (vReg != NULL) {
                    smem->memAddr = vReg->regAddr;
                    smem->memType = SPI_SERVICE_TYPE_INFO;
                    return smem;
                }
#endif
            }
            break;
    }

    return NULL;
}

void *spi_service_smem_tx_free(spi_service_mem_t *smem)
{
    return spi_service_smem_rx_free(smem);
}

#ifdef SPI_SERVICE_MEM_DEBUG_SUPPORT
static void spi_service_mem_dump(enum SPI_SERVICE_MEM type)
{
    unsigned int flag = system_irq_save();
    struct mblock_free *node = g_service_fblk[type];

    os_printf(LM_APP, LL_INFO, "###########################################\n");
    while (node != NULL)
    {
        os_printf(LM_APP, LL_INFO, "corrupt_check: 0x%x\n", node->corrupt_check);
        os_printf(LM_APP, LL_INFO, "addr: 0x%x size 0x%x\n", node, node->free_size);
        os_printf(LM_APP, LL_INFO, "pre: 0x%x nxt 0x%x\n", node->previous, node->next);
        node = node->next;
    }
    os_printf(LM_APP, LL_INFO, "###########################################\n");
    system_irq_restore(flag);
}

static int spi_service_cmd_dump(cmd_tbl_t *t, int argc, char *argv[])
{
    spi_service_mem_dump(SPI_SERVICE_MEM_RX);
    spi_service_mem_dump(SPI_SERVICE_MEM_TX);

    return CMD_RET_SUCCESS;
}

CLI_CMD(spi_mem_dump, spi_service_cmd_dump, "spi_service_mem_dump", "spi_service_mem_dump");
#endif
