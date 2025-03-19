#include <string.h>
#include "reg_macro_def.h"
#include "chip_memmap.h"
#include "oshal.h"
#include "system_def.h"

#define KE_LIST_PATTERN           (0xA55A)
#define KE_ALLOCATED_PATTERN      (0x8338)
#define KE_FREE_PATTERN           (0xF00F)

#ifndef ASSERT_ERR
#define ASSERT_ERR(cond) do { if (!(cond)) { os_printf(LM_OS, LL_INFO, "[ERR]%s line:%d \""#cond"\"!!!\n", __func__, __LINE__); } } while(0)

#define ASSERT_INFO(cond, param0) do { if (!(cond)) { os_printf(LM_OS, LL_INFO, "[INFO]%s line:%d param 0x%x \""#cond"\"!!!\n", __func__, __LINE__, param0); } } while(0)
#endif

struct mblock_free
{
    uint32_t corrupt_check;
    uint32_t free_size;
    struct mblock_free* next;
    struct mblock_free* previous;
};

struct mblock_used
{
    uint32_t corrupt_check;
    uint32_t size;
};

#define CO_ALIGN4_HI(val) (((val)+3)&~3)

#define PSRAM_MEM_SIZE 0x80000
#define PSRAM_MEM_BASE 0xC00000
struct mblock_free *g_psram_mem;

void os_psram_mem_init(void)
{
    unsigned long flag = system_irq_save();
    g_psram_mem = (struct mblock_free *)PSRAM_MEM_BASE;
    WRITE_REG(MEM_BASE_SPI0 + 0x30, READ_REG(MEM_BASE_SPI0 + 0x30) | (1 << 21));
    g_psram_mem->free_size = PSRAM_MEM_SIZE;
    g_psram_mem->next = NULL;
    g_psram_mem->previous = NULL;
    g_psram_mem->corrupt_check = KE_LIST_PATTERN;
    WRITE_REG(MEM_BASE_SPI0 + 0x30, READ_REG(MEM_BASE_SPI0 + 0x30) & (~(1 << 21)));
    system_irq_restore(flag);
}

void *os_psram_mem_alloc(uint32_t size)
{
    struct mblock_free *node, *found;
    struct mblock_used *alloc = NULL;
    uint32_t totalsize;
    unsigned long flag = system_irq_save();

    /* mblock_used as head */
    totalsize = CO_ALIGN4_HI(size) + sizeof(struct mblock_used);

    /* the totalsize should be large enough to hold free block descriptor */
    if (totalsize < sizeof(struct mblock_free))
    {
        /* free may list it in free block descriptor */
        totalsize = sizeof(struct mblock_free);
    }

    found = NULL;
    node = g_psram_mem;
    while (node != NULL)
    {
        ASSERT_INFO(node->corrupt_check == KE_LIST_PATTERN, node->corrupt_check);
        if (node->free_size < totalsize)
        {
            node = node->next;
            continue;
        }

        found = node;
        if (node->previous != NULL)
        {
            /* As a mem node must bigger than free block descriptor */
            if (found->free_size < (totalsize + sizeof(struct mblock_free)))
            {
                totalsize = found->free_size;
            }
            break;
        }

        if (node->free_size >= (totalsize + sizeof(struct mblock_free)))
        {
            if (found->free_size < (totalsize + 2 * sizeof(struct mblock_free)))
            {
                totalsize = found->free_size - sizeof(struct mblock_free);
            }
            break;
        }
        found = NULL;
        node = node->next;
    }

    if (found != NULL)
    {
        if (found->free_size == totalsize)
        {
            ASSERT_ERR(found->previous != NULL);

            found->previous->next = found->next;
            if(found->next != NULL)
            {
                found->next->previous = found->previous;
            }
            //cli_printf("found->free_size = 0x%x[0x%x]\n", found->free_size, totalsize);
        }

        found->free_size -= totalsize;
        alloc = (struct mblock_used *) ((uint32_t)found + found->free_size);
        alloc->size = totalsize;

        //cli_printf("alloc = 0x%x[0x%x]\n", alloc, alloc->size);
        alloc->corrupt_check = KE_ALLOCATED_PATTERN;
        alloc++;
    }
    system_irq_restore(flag);

    return (void *)alloc;
}

void *os_psram_mem_free(void *mem_ptr)
{
    struct mblock_used *bfreed;
    struct mblock_free *node, *prev_node, *next_node, *freed;
    uint32_t size;
    unsigned long flag = system_irq_save();

    ASSERT_ERR(mem_ptr != NULL);
    node = g_psram_mem;
    prev_node = NULL;
    if (node == NULL)
    {
        system_irq_restore(flag);
        return mem_ptr;
    }

    bfreed = ((struct mblock_used *)mem_ptr) - 1;
    //cli_printf("%s[%d]free = 0x%x[0x%x]\n", __FUNCTION__, __LINE__, bfreed, bfreed->size);
    freed = ((struct mblock_free *)bfreed);
    size = bfreed->size;
    ASSERT_INFO(bfreed->corrupt_check == KE_ALLOCATED_PATTERN, bfreed->corrupt_check);

    while (node != NULL)
    {
        ASSERT_INFO(node->corrupt_check == KE_LIST_PATTERN, node);
        // check if the freed block is right after the current block
        if ((uint32_t)freed == ((uint32_t)node + node->free_size))
        {
            // append the freed block to the current one
            node->free_size += size;

            // check if this merge made the link between free blocks
            if ((uint32_t)node->next == ((uint32_t)node + node->free_size))
            {
                next_node = node->next;
                // add the size of the next node to the current node
                node->free_size += next_node->free_size;
                // update the next of the current node
                ASSERT_ERR(next_node != NULL);
                node->next = next_node->next;
                if(next_node->next != NULL)
                {
                    next_node->next->previous = node;
                }
            }
            goto free_end;
        }
        else if ((uint32_t)freed < (uint32_t)node)
        {
            // sanity check: can not happen before first node
            ASSERT_ERR(prev_node != NULL);

            // update the next pointer of the previous node
            prev_node->next = (struct mblock_free*)freed;
            freed->previous = prev_node;
            freed->corrupt_check = KE_LIST_PATTERN;
            // check if the released node is right before the free block
            if (((uint32_t)freed + size) == (uint32_t)node)
            {
                // merge the two nodes
                freed->next = node->next;
                if(node->next != NULL)
                {
                    node->next->previous = freed;
                }
                freed->free_size = node->free_size + size;
            }
            else
            {
                // insert the new node
                freed->next = node;
                node->previous = freed;
                freed->free_size = size;
            }
            goto free_end;
        }

        // move to the next free block node
        prev_node = node;
        node = node->next;
    }

    freed->corrupt_check = KE_LIST_PATTERN;
    prev_node->next = (struct mblock_free*)freed;
    freed->next = NULL;
    freed->previous = prev_node;
    freed->free_size = size;

free_end:
    system_irq_restore(flag);
    return NULL;
}

void os_psram_mem_read(char *psramptr, char *bufptr, unsigned int len)
{
    memcpy(bufptr, psramptr, len);
}

void os_psram_mem_write(char *psramptr, char *bufptr, unsigned int len)
{
    unsigned int length;
    unsigned int offset = 0;

    while (offset < len)
    {
        length = (len - offset) > 1024 ? 1024 : (len - offset);
        unsigned int flags = system_irq_save();

        WRITE_REG(MEM_BASE_SPI0 + 0x30, READ_REG(MEM_BASE_SPI0 + 0x30) | (1 << 21));
        memcpy(psramptr + offset, bufptr + offset, length);
        WRITE_REG(MEM_BASE_SPI0 + 0x30, READ_REG(MEM_BASE_SPI0 + 0x30) & (~(1 << 21)));

        system_irq_restore(flags);
        offset += length;
    }
}

#if 0
#include "cli.h"

static int psram_data_test_cmd(cmd_tbl_t *t, int argc, char *argv[])
{
    int index, j;
    unsigned int buff[1024] = {0};
    char *pbf = NULL;

    for (index = 0; index < 1024; index++)
        buff[index] = index;

    pbf = os_psram_mem_alloc(16*1024);

    for (index = 0; index < 4; index++)
        os_psram_mem_write(pbf + index * sizeof(buff), (char *)buff, sizeof(buff));

    for (index = 0; index < 4; index++)
    {
        memset(buff, 0, sizeof(buff));
        os_psram_mem_read(pbf + index * sizeof(buff), (char *)buff, sizeof(buff));

        for (j = 0; j < 1024; j++)
            os_printf(LM_OS, LL_INFO, "0x%x ", buff[j]);
        os_printf(LM_OS, LL_INFO, "\n");
    }

    os_psram_mem_free(pbf);

    return CMD_RET_SUCCESS;
}
CLI_CMD(psram_test, psram_data_test_cmd, "psram data read/write", "psram data read/write test");

#endif
