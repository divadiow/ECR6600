#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "arch_irq.h"
#include "chip_clk_ctrl.h"
#include "cli.h"
#include "chip_irqvector.h"

#include "sdio_slave_interface.h"

#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif

//#define BIT(x) (1 << (x))

//#define BIT0    (0x00000001L << 0)
//#define BIT1    (0x00000001L << 1)
//#define BIT2    (0x00000001L << 2)
//#define BIT3    (0x00000001L << 3)
//#define BIT4    (0x00000001L << 4)
//#define BIT5    (0x00000001L << 5)
//#define BIT6    (0x00000001L << 6)
//#define BIT7    (0x00000001L << 7)
//#define BIT8    (0x00000001L << 8)
//#define BIT9    (0x00000001L << 9)
//#define BIT10   (0x00000001L << 10)
//#define BIT11   (0x00000001L << 11)
//#define BIT12   (0x00000001L << 12)
//#define BIT13   (0x00000001L << 13)
//#define BIT14   (0x00000001L << 14)
//#define BIT15   (0x00000001L << 15)
//#define BIT16   (0x00000001L << 16)
//#define BIT17   (0x00000001L << 17)
//#define BIT18   (0x00000001L << 18)
//#define BIT19   (0x00000001L << 19)
//#define BIT20   (0x00000001L << 20)
//#define BIT21   (0x00000001L << 21)
//#define BIT22   (0x00000001L << 22)
//#define BIT23   (0x00000001L << 23)
//#define BIT24   (0x00000001L << 24)
//#define BIT25   (0x00000001L << 25)
//#define BIT26   (0x00000001L << 26)
//#define BIT27   (0x00000001L << 27)
//#define BIT28   (0x00000001L << 28)
//#define BIT29   (0x00000001L << 29)
//#define BIT30   (0x00000001L << 30)
//#define BIT31   (0x00000001L << 31)


/* SDIO device controller */
#define SDIO_BASE_ADDR       (0x00440000)

#define SDIO_PAD_SR          (*((volatile unsigned int *)(0x00201228)))

#define SDIO_CONTROL         (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x00)))
#define SDIO_COMMAND         (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x04)))
#define SDIO_ARGUMENT1       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x08)))
#define SDIO_BLOCK           (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x0C)))
#define SDIO_DMA1_ADDR       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x10)))
#define SDIO_DMA1_CTL        (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x14)))
#define SDIO_DMA2_ADDR       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x18)))
#define SDIO_DMA2_CTL        (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x1C)))
#define SDIO_ERASE_START     (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x20)))
#define SDIO_ERASE_END       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x24)))
#define SDIO_PASSWD_LEN      (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x28)))
#define SDIO_SECURE_BC       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x2C)))
#define SDIO_RESERVED0       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x30)))
#define SDIO_RESERVED1       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x34)))
#define SDIO_RESERVED2       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x38)))
#define SDIO_INT1_STATUS     (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x3C)))
#define SDIO_INT1_STATUS_EN  (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x40)))
#define SDIO_INT1_EN         (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x44)))
#define SDIO_CARD_ADDR       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x48)))
#define SDIO_CARD_DATA       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x4C)))
#define SDIO_IOREADY         (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x50)))
#define SDIO_FUN1_CTL        (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x54)))
#define SDIO_FUN2_CTL        (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x58)))
#define SDIO_CCCR            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x5C)))
#define SDIO_FBR1            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x60)))
#define SDIO_FBR2            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x64)))
#define SDIO_FBR3            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x68)))
#define SDIO_FBR4            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x6C)))
#define SDIO_FBR5            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x70)))
#define SDIO_FBR6            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x74)))
#define SDIO_FBR7            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x78)))
#define SDIO_FBR8            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x7C)))
#define SDIO_CARD_SIZE       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x80)))
#define SDIO_CARD_OCR        (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x84)))
#define SDIO_CONTROL2        (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x88)))
#define SDIO_FUN3            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x90)))
#define SDIO_FUN4            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x94)))
#define SDIO_FUN5            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x98)))
#define SDIO_INT2_STATUS     (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x9C)))
#define SDIO_INT2_STATUS_EN  (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xA0)))
#define SDIO_INT2_EN         (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xA4)))
#define SDIO_PASSWD_127_96   (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xA8)))
#define SDIO_PASSWD_95_64    (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xAC)))
#define SDIO_PASSWD_63_32    (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xB0)))
#define SDIO_PASSWD_31_0     (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xB4)))
#define SDIO_ADMA_ERR        (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xB8)))
#define SDIO_RCA             (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xBC)))
#define SDIO_DBG0            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xC0)))
#define SDIO_DBG1            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xC4)))
#define SDIO_DBG2            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xC8)))
#define SDIO_DBG3            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xCC)))
#define SDIO_DBG4            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xD0)))
#define SDIO_DBG5            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xD4)))
#define SDIO_DBG6            (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xD8)))
#define SDIO_AHB_BURST       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xDC)))
#define SDIO_ARGUMENT2       (*((volatile unsigned int *)(SDIO_BASE_ADDR + 0xE0)))


#define SDIO_INFO_REG		(*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x1C0)))
#define SDIO_INFO_EN		(*((volatile unsigned int *)(SDIO_BASE_ADDR + 0x1C4)))


//interrupt
#define SDIO_INTR_TRANSFER_CMPL			BIT(0)
#define SDIO_INTR_WRITE_START			BIT(3)
#define SDIO_INTR_READ_START			BIT(4)
#define SDIO_INTR_SOFT_RESET			BIT(12)
#define SDIO_INTR_FUNC1_RESET			BIT(21)
#define SDIO_INTR_PROGRAM_START			BIT(25)
#define SDIO_INTR_FUNCX_ERROR			BIT(28) 
#define SDIO_INTR_FUNCX_ABORT			BIT(29) 

#define IS_ADMA_ERROR()					((SDIO_INT2_STATUS) & BIT6)


//cmd
#define SDIO_BLOCK_SIZE			(SDIO_ARGUMENT1 & 0x1FF)
#define SDIO_BLOCK_COUNT		(SDIO_BLOCK)

#define SDIO_TRANSFER_SIZE		(SDIO_BLOCK_SIZE * SDIO_BLOCK_COUNT)
#define SDIO_TRANSFER_ADDR		((SDIO_ARGUMENT1>>9) & 0x1FFFF)


#define SDIO_INT_CLR(X)			(SDIO_INT1_STATUS = X)  
#define SDIO_FUN1_IND(X)		(SDIO_FUN1_CTL = X)
#define SDIO_FUN2_IND(X)		(SDIO_FUN2_CTL = X)

#define SDIO_ADMA_EN()			(SDIO_CONTROL2	   |= BIT2)


#define SDIO_ADMA_ENABLE		(1)
#define SDIO_ADMA_BLK			(0)
#define SINGLE_MASK_MAP			(1)

#define sdio_requested_flag(x)		((x >> 9) & 0xFF)
#define sdio_requested_next(x)		((x >> 17) & 0x1FF)

#define sdio_requested_type(x)		((x >> 9) & 0x1FFFF)


//adma attribute
#define SDIO_ADMA_ATTR_VALID    (BIT0)
#define SDIO_ADMA_ATTR_END      (BIT1)
#define SDIO_ADMA_ATTR_INT      (BIT2)
#define SDIO_ADMA_ATTR_ACT_NOP  (0 << 4)
#define SDIO_ADMA_ATTR_ACT_TRS  (2 << 4)
#define SDIO_ADMA_ATTR_ACT_LINK (3 << 4)


//queue
#define HIF_SDIO_TX        0
#define HIF_SDIO_RX        1

//state
#define STATE_IDLE 	  	  (0)
#define STATE_TX 	      (1)
#define STATE_RX 	      (2)
#define STATE_INFO 	      (3)
#define STATE_INFO_ASYNC  (4)
#define STATE_MAX 	      (5)

//recv address for data type
#define ADDR_DATA			0x80
#define ADDR_INFO			0x81
#define ADDR_INFO_ASYNC		0x82

//message and descriptor address
#define SDIO_MSG_BASE_INFO		(0x000AFB00)
#define SDIO_MSG_BASE			(0x000AFC00)
#define SDIO_DES_BASE_INFO		(0x000AFDE0)
#define SDIO_DES_BASE_RX		(0x000AFE00)
#define SDIO_DES_BASE_TX		(0x000AFF00)

//config
#define SDIO_MAX_BLOCK_SIZE		(512)
#define NEXT_BUF_SZ_OFFSET		(0)
#define SLAVE_BUF_SZ_OFFSET		(4)
#define SDIO_MEM_MAX_NUM		(64)


//
#define  sdio_assert(x)				\
    if (!(x))								\
{		__nds32__gie_dis();			\
    os_printf(LM_OS, LL_ERR, ">> sdio error %s, %d \r\n", __func__, __LINE__);		\
    while(1);						\
}

/////////////////////////////////////////////////////////////////////////////////////////////////





//struct 

struct adma_descriptor
{
    unsigned short attri;
    unsigned short len;
    unsigned int addr;
};

struct sdio_queue
{
    unsigned int count;
    sdio_bufs_t  *head;
    sdio_bufs_t  *tail;
} ;

typedef enum en_sdio_tx_state
{
    SDIO_TX_IDLE,
    SDIO_TX_WAIT_2_INFORM,
    SDIO_TX_WAIT_2_READ,
    SDIO_TX_RAEDING,
    SDIO_TX_LAST_READ,

    SDIO_TX_MAX,
}EN_SDIO_TX_STATE;

struct sdio_priv
{
    //	int continue_transfer;
    //	int report_threshold;

    int state;

    unsigned char restart_rx;
    unsigned short flag_rx;

    unsigned int sdio_argument;
    unsigned int sdio_command;
    struct sdio_queue hwque[2];

    sdio_tx_bufs_t *tx_curr_frag;
    sdio_tx_bufs_t *tx_curr_buff;
    unsigned int offset;
    volatile struct adma_descriptor *adma_curr;
    //struct sdio_queue to_host_que;
    sdio_tx_bufs_t *tx_head;
    sdio_tx_bufs_t *tx_free_head;
    EN_SDIO_TX_STATE tx_state;
    EN_SDIO_TX_STATE tx_next_state;
};

typedef struct 
{

    //sdio_bufs_t * tx_entry;
    sdio_bufs_t * rx_entry;
    //unsigned int tx_avail;
    unsigned int rx_avail;

}sdio_mem_t;


//global
static struct sdio_priv 	m_priv;
sdio_mem_t  				sdio_mem;
sdio_tx_param_t 			sdio_tx_mem[SDIO_MEM_MAX_NUM];
sdio_bufs_t 				sdio_rx_men[SDIO_MEM_MAX_NUM];
unsigned int 				isr_tx_count = 0;
unsigned int 				complete_cnt_tx =0;
unsigned int			    complete_cnt_rx =0;
unsigned int 				complete_cnt_total =0;

unsigned int sdio_buff[128] __attribute__((section(".dma.data"),aligned(4)));


static void sdio_tx_free_queue_push(sdio_tx_bufs_t *sysbuf);
extern void sdio_tx_complete(void);
extern int is_wifi_ready(void);
extern uint32_t get_wifi_remain_buffer(uint8_t ac);

#if 0

#define SDIO_DEBUG_RX_ADDR	0x000AE000	// 2k
#define SDIO_DEBUG_TX_ADDR	0x000AE800	// 4k

unsigned int get_rx_buff_free_size()
{
    return 8192;
}

void sdio_tx_callback(void *para)
{


}


int sdio_rx_cb(sdio_bufs_t *bufs, unsigned int flag)
{
    return 0;
}

int alloc_buffer_for_sdio(sdio_bufs_t *bufs, uint32_t flag, uint32_t need_len)
{
    bufs->bufs[0] = SDIO_DEBUG_RX_ADDR;
    bufs->lens[0] = need_len;
    bufs->nb = 1;
    return 0;
}

#else
unsigned int get_rx_buff_free_size()
{
#ifdef NX_ESWIN_SDIO
    return get_wifi_remain_buffer(0);
#else
    return 0;
#endif
}
#endif


static void sdio_hal_reset()
{
    SDIO_CARD_OCR		= 0x10FF8000;//0x20FF8000;
    SDIO_CCCR			= 0x23FFE343;//0x23FFF343;

    SDIO_INT1_STATUS_EN = 0xFFFFFFFF;
    SDIO_INT1_STATUS	= 0xFFFFFFFF;
    SDIO_INT1_EN		= 0xFFFFFFFF;

    SDIO_INT2_STATUS_EN = 0xFFFFFFFF;
    SDIO_INT2_STATUS	= 0xFFFFFFFF;
    SDIO_INT2_EN		= 0xFFFFFFFF;

    //SDIO_IOREADY		= BIT1 | BIT2;  // function 1 & 2  ready
    SDIO_IOREADY		= BIT1;  // function 1 ready
    SDIO_CONTROL		&= 0xFF1FFFFF;
    SDIO_CONTROL	    |= BIT2;  // card is ready to operate

    return;
}

static void sdio_mem_init(void)
{
    int i;

    for (i=1; i<SDIO_MEM_MAX_NUM; i++)
    {
        sdio_rx_men[i-1].next = &sdio_rx_men[i];
    }

    sdio_rx_men[i-1].next = NULL;

    sdio_mem.rx_entry = &sdio_rx_men[0];

    sdio_mem.rx_avail = SDIO_MEM_MAX_NUM;
}

/* inner func, must be called with irq lock */
static sdio_bufs_t * sdio_mem_rx_alloc(void)
{
    sdio_bufs_t * ret;

    ret = sdio_mem.rx_entry;
    sdio_assert(ret);

    sdio_mem.rx_entry = sdio_mem.rx_entry->next;
    ret->next = NULL;
    sdio_mem.rx_avail--;
    return ret;
}

/* inner func, must be called with irq lock */
static void sdio_mem_rx_free(sdio_bufs_t * bufs)
{
    sdio_assert(bufs);
    bufs->next = sdio_mem.rx_entry;
    sdio_mem.rx_entry = bufs;
    sdio_mem.rx_avail++;
}

#if 0
void sdio_mem_tx_free(sdio_tx_bufs_t * bufs)
{
    unsigned long flags;
    if(!bufs)
    {
        return;
    }
    sdio_assert(bufs);
    flags = arch_irq_save();
    ke_free(bufs);
    arch_irq_restore(flags);
}

sdio_tx_param_t * sdio_mem_tx_alloc(void)
{
    sdio_bufs_t * ret;
    unsigned long flags;
    flags = arch_irq_save();

    if (sdio_mem.tx_avail == 0)
    {
        sdio_assert(0);
    }

    ret = sdio_mem.tx_entry;
    sdio_assert(ret);

    sdio_mem.tx_entry = sdio_mem.tx_entry->next;
    ret->next = NULL;
    sdio_mem.tx_avail--;
    //W_DEBUG_I("tx avail: %d \n", sdio_mem.tx_avail);
    arch_irq_restore(flags);
    return (sdio_tx_param_t *)ret;
}
#endif

static void sdio_queue_init(struct sdio_queue *hque)
{
    hque->count = 0;
    hque->head = NULL;
    hque->tail = NULL;
}

/* inner func, must be called with irq lock */
static void sdio_queue_push(struct sdio_queue *hque, sdio_bufs_t *sysbuf)
{
    sdio_assert(hque);
    sdio_assert(sysbuf);

    if (hque->head)
    {
        hque->tail->next = sysbuf;
    }
    else
    {
        hque->head = sysbuf;
    }

    hque->tail = sysbuf;
    hque->tail->next = NULL;

    hque->count++;

    return;
}

/* inner func, must be called with irq lock */
static sdio_bufs_t *sdio_queue_pop(struct sdio_queue *hque)
{
    sdio_bufs_t *res = NULL;
    sdio_assert(hque);

    if (hque->count)
    {
        res = hque->head;
        sdio_assert(res);
        hque->head = res->next;
        res->next = NULL;
        hque->count--;
    }

    return res;
}

/* inner func, must be called with irq lock */
static unsigned int sdio_queue_count(struct sdio_queue *hque)
{
    sdio_assert(hque);

    return hque->count;
}

unsigned int sdio_tx_queue_count(void)
{
    struct sdio_priv *priv = &m_priv;
    return priv->tx_head != NULL;
}

static void sdio_tx_queue_push(sdio_tx_bufs_t *sysbuf)
{
    struct sdio_priv *priv = &m_priv;
    sdio_tx_bufs_t *head = NULL;
    sdio_tx_bufs_t *temp = NULL;

    sdio_assert(sysbuf);
    sysbuf->next_buf = NULL;


    head = priv->tx_head;
    temp = head;
    if(head == NULL)
    {
        priv->tx_head = sysbuf;
    }
    else
    {
        while(temp->next_buf)
        {
            temp = temp->next_buf;
        }
        temp->next_buf = sysbuf;
    }

    return;
}

/* inner func, must be called with irq lock */
static sdio_tx_bufs_t *sdio_tx_queue_pop(struct sdio_priv *priv)
{
    sdio_tx_bufs_t *res = NULL;

    if(priv->tx_head)
    {
        res = priv->tx_head;
        priv->tx_head = priv->tx_head->next_buf;

        res->next_buf = NULL;
    }

    return res;
}

#if 0
/* inner func, must be called with irq lock */
static unsigned int sdio_tx_buf_count(sdio_tx_bufs_t *buf)
{
    unsigned int count = 0;
    sdio_tx_bufs_t *res_frag = buf;
    //sdio_tx_bufs_t *res_frag = head;
    sdio_assert(buf);

    while(res_frag)
    {
        count += res_frag->lens;
        res_frag = res_frag->next_frag;
    }
    //count = hque->count;
    return count;
}
#endif

/* must be called with irq lock */
static sdio_bufs_t *sdio_queue_peek(struct sdio_queue *hque)
{
    sdio_assert(hque);

    return hque->head;
}

/* must be called with irq lock */
static sdio_tx_bufs_t *sdio_tx_queue_peek(sdio_tx_bufs_t *head)
{
    return head;
}

static unsigned int sdio_requested_length(unsigned int arg)
{
    unsigned int len;
    if (arg & 0x08000000)
    {
        len = SDIO_MAX_BLOCK_SIZE * (arg & 0x1FF);
    }
    else
    {
        len = arg & 0x1FF;
        if (!len) len = 512;
    }

    return len;
}

/* must be called with irq lock */
static void sdio_isr_rx(struct sdio_priv *priv)
{
    unsigned int len, ret;
    sdio_bufs_t *bufs;

    priv->sdio_command 	= SDIO_COMMAND;
    priv->sdio_argument = SDIO_ARGUMENT1;

    len = sdio_requested_length(priv->sdio_argument);

    if (priv->state == STATE_IDLE)
    {
        priv->flag_rx = sdio_requested_flag(priv->sdio_argument);


        bufs = sdio_mem_rx_alloc();

        if (sdio_requested_next(priv->sdio_argument) == 0)
        {
            ret = alloc_buffer_for_sdio(bufs, priv->flag_rx, len);
            priv->restart_rx = 0;
        }
        else
        {
            ret = alloc_buffer_for_sdio(bufs, priv->flag_rx, len + sdio_requested_next(priv->sdio_argument));
            priv->restart_rx = 1;
        }

        if (ret)
        {
            sdio_assert(0);
        }

        sdio_queue_push(&priv->hwque[HIF_SDIO_RX], bufs);
        priv->offset = 0;

    }
    else if (priv->state == STATE_RX)
    {
        //priv->rx_remain_bytes += len;
        bufs = sdio_queue_peek(&priv->hwque[HIF_SDIO_RX]);
    }
    else
    {
        sdio_assert(0);
    }

    priv->adma_curr = (volatile struct adma_descriptor *)( SDIO_DES_BASE_RX );
    priv->adma_curr->addr = bufs->bufs[0] + priv->offset;
    priv->adma_curr->len = len;
    priv->adma_curr->attri = 0x23;

    priv->offset += len;

    SDIO_DMA1_ADDR = SDIO_DES_BASE_RX;
    SDIO_DMA1_CTL  = 0xF;

    priv->state = STATE_RX;
#ifdef CONFIG_PSM_SURPORT
    //psm_set_device_status(PSM_DEVICE_SDIO_SLAVE, PSM_DEVICE_STATUS_ACTIVE);
#endif
}

/* must be called with irq lock */

#define TX_OPT  1

static void sdio_isr_tx(struct sdio_priv *priv)
{
    //unsigned char type;
    unsigned int len;
    sdio_tx_bufs_t * bufs = NULL;
    //int * p_avlbuf = NULL;

    priv->sdio_command 	= SDIO_COMMAND;
    priv->sdio_argument = SDIO_ARGUMENT1;

    // type =  sdio_requested_type(priv->sdio_argument);
    //isr_tx_count++;

    switch (sdio_requested_type(priv->sdio_argument))
    //switch (type)
    {
        case ADDR_DATA:
            {
                len = sdio_requested_length(priv->sdio_argument);

                if (priv->state == STATE_IDLE )
                {
                    unsigned int * p_next_buf_size = NULL;
                    /* get a new buf, and prepare the transmition */
                    bufs = sdio_tx_queue_pop(priv);
                    sdio_assert(bufs);
                    //priv->tx_curr_frag = bufs;
                    priv->tx_curr_buff = bufs;
                    //priv->offset = 0;
                    priv->tx_state = SDIO_TX_RAEDING;

                    /* add next buffer leninfo */
                    p_next_buf_size = (unsigned int *)(bufs->bufs + NEXT_BUF_SZ_OFFSET);
                    /* update the slave_avl_buf now */
                    //p_avlbuf = (unsigned int *)(bufs->bufs + SLAVE_BUF_SZ_OFFSET);

                    //*p_avlbuf = get_rx_buff_free_size();

                    *(unsigned int *)(bufs->bufs + SLAVE_BUF_SZ_OFFSET) = get_rx_buff_free_size() | (bufs->total_len & 0xFFFF0000);

                    if(sdio_tx_queue_peek(priv->tx_head))
                    {
                        //*p_next_buf_size = sdio_tx_buf_count(sdio_tx_queue_peek(priv->tx_head));
                        *p_next_buf_size = (priv->tx_head->total_len) & 0xFFFF;
                        priv->tx_next_state = SDIO_TX_WAIT_2_READ;
                    }
                    else
                    {
                        *p_next_buf_size = 0;
                        sdio_assert(priv->tx_next_state == SDIO_TX_IDLE);
                    }

                }

#if 0
                else if (priv->state == STATE_TX)
                {
                    /* the buf transmition has started, do nothing */
                    //bufs = priv->tx_curr_frag;
                }
                else
                {
                    sdio_assert(0);
                }

#endif
                priv->adma_curr = (volatile struct adma_descriptor *)(SDIO_DES_BASE_TX);


#if 1

                sdio_assert(len == ((bufs->total_len) & 0xFFFF) );

                if (len > 512)
                {
                    unsigned int len_temp;
                    while ( bufs )
                    {
                        len_temp = (bufs->lens + 3) & 0xFFFFFFFC;
                        priv->adma_curr->len = len_temp;
                        priv->adma_curr->attri = 0x21;
                        priv->adma_curr->addr = bufs->bufs;
                        len -= len_temp;
                        priv->adma_curr++;

                        bufs = bufs->next_frag;
                    }

                    if (len)
                    {
                        priv->adma_curr->len = len;
                        priv->adma_curr->attri = 0x23;
                        priv->adma_curr->addr = (unsigned int)sdio_buff;
                    }
                    else
                    {
                        priv->adma_curr--;
                        priv->adma_curr->attri = 0x23;
                    }
                }
                else
                {
                    while ( bufs )
                    {
                        priv->adma_curr->len = bufs->lens;
                        priv->adma_curr->attri = 0x21;
                        priv->adma_curr->addr = bufs->bufs;
                        len -= bufs->lens;
                        priv->adma_curr++;

                        bufs = bufs->next_frag;
                    }

                    priv->adma_curr--;
                    priv->adma_curr->attri = 0x23;
                }

                priv->tx_state = SDIO_TX_LAST_READ;

#else

                /* prepare the adma_curr */
                while (len > (priv->tx_curr_frag->lens - priv->offset))
                {
                    priv->adma_curr->len = priv->tx_curr_frag->lens - priv->offset;
                    priv->adma_curr->attri = 0x21;
                    priv->adma_curr->addr = priv->tx_curr_frag->bufs + priv->offset;
                    len -= priv->adma_curr->len;
                    priv->adma_curr++;

                    priv->offset = 0;
                    priv->tx_curr_frag = priv->tx_curr_frag->next_frag;
                    if(priv->tx_curr_frag == NULL)
                    {
                        sdio_assert(0);
                    }
                }
                /* last adma */
                priv->adma_curr->len = len;
                priv->adma_curr->attri = 0x23;
                priv->adma_curr->addr = priv->tx_curr_frag->bufs + priv->offset;
                priv->offset += len;
                if (priv->offset == priv->tx_curr_frag->lens)
                {
                    /* the transmition of this frag will be finished  */
                    priv->offset = 0;
                    priv->tx_curr_frag = priv->tx_curr_frag->next_frag;
                    if (priv->tx_curr_frag == NULL)
                    {
                        /* the transmition of this buf will be finished  */
                        priv->tx_state = SDIO_TX_LAST_READ;
                    }

                }

#endif

                SDIO_DMA1_ADDR = SDIO_DES_BASE_TX;
                SDIO_DMA1_CTL  = 0xF;

                priv->state = STATE_TX;
#ifdef CONFIG_PSM_SURPORT
                //psm_set_device_status(PSM_DEVICE_SDIO_SLAVE, PSM_DEVICE_STATUS_ACTIVE);
#endif
                break;
            }

        case ADDR_INFO:
            //sdio_assert(0);
#if 0
            len = sdio_requested_length(priv->sdio_argument);
            *p_available_buf_size = get_rx_buff_free_size();
            priv->adma_curr = (volatile struct adma_descriptor *)(SDIO_DES_BASE_INFO);
            priv->adma_curr->len = len;
            priv->adma_curr->attri = 0x23;
            priv->adma_curr->addr = SDIO_MSG_BASE_INFO;

            SDIO_DMA1_ADDR = SDIO_DES_BASE_INFO;
            SDIO_DMA1_CTL  = 0xF;
#endif
            priv->state = STATE_INFO;
#ifdef CONFIG_PSM_SURPORT
            //psm_set_device_status(PSM_DEVICE_SDIO_SLAVE, PSM_DEVICE_STATUS_ACTIVE);
#endif

            break;

        case ADDR_INFO_ASYNC:
            if(priv->state == STATE_IDLE)
            {
                unsigned int * p_available_buf_size = (unsigned int *)SDIO_MSG_BASE_INFO;

                len = sdio_requested_length(priv->sdio_argument);
                *p_available_buf_size = get_rx_buff_free_size();//func_get_from_ps
                priv->adma_curr = (volatile struct adma_descriptor *)(SDIO_DES_BASE_INFO);
                priv->adma_curr->len = len;
                priv->adma_curr->attri = 0x23;
                priv->adma_curr->addr = SDIO_MSG_BASE_INFO;

                SDIO_DMA1_ADDR = SDIO_DES_BASE_INFO;
                SDIO_DMA1_CTL  = 0xF;

                priv->state = STATE_INFO_ASYNC;
#ifdef CONFIG_PSM_SURPORT
                //psm_set_device_status(PSM_DEVICE_SDIO_SLAVE, PSM_DEVICE_STATUS_ACTIVE);
#endif
            }
            else
            {
                sdio_assert(0);
            }
            break;

        default:
            sdio_assert(0);
            break;
    }

    return;
}

/* must be called with irq lock */
static void sdio_check_tx_buff(struct sdio_priv *priv)
{
    sdio_tx_bufs_t * tx_bufs;
    int len=0;

    /* baoyong:todo  */
    if (priv->tx_state == SDIO_TX_IDLE)
    {
        /* nothing to send */
        if (priv->tx_next_state == SDIO_TX_IDLE)
        {
            return;
        }

        tx_bufs = sdio_tx_queue_peek(priv->tx_head);
        sdio_assert(tx_bufs);
        if (priv->tx_next_state == SDIO_TX_WAIT_2_INFORM) 
        {
            //len = sdio_tx_buf_count(tx_bufs);
			len = tx_bufs->total_len;
            while(SDIO_FUN1_CTL);
            SDIO_FUN1_IND(len & 0xFFFF);
        }

        priv->tx_state = SDIO_TX_WAIT_2_READ;

        if (tx_bufs->next_buf)
        {
            /* will indicate the host during the currant tx */
            priv->tx_next_state = SDIO_TX_WAIT_2_READ;
        }
        else
        {
            priv->tx_next_state = SDIO_TX_IDLE;
        }
    }

    return;
}

/* must be called with irq lock */
static void sdio_isr_cmpl(struct sdio_priv *priv)
{
    sdio_bufs_t * bufs;

    complete_cnt_total++;
    if (priv->state == STATE_RX)
    {
        if (sdio_queue_count(&priv->hwque[HIF_SDIO_RX]))
        {
            SDIO_CONTROL |= 1;    // write done
            SDIO_DMA1_CTL  = 0x0;
            complete_cnt_rx++;

            if (priv->restart_rx == 0)
            {
                bufs = sdio_queue_pop(&priv->hwque[HIF_SDIO_RX]);

                sdio_rx_cb(bufs, priv->flag_rx);
                sdio_mem_rx_free(bufs);

                priv->state = STATE_IDLE;
#ifdef CONFIG_PSM_SURPORT
                //psm_set_device_status(PSM_DEVICE_SDIO_SLAVE, PSM_DEVICE_STATUS_IDLE);
#endif
            }
            else
            {
                priv->restart_rx = 0;
            }
        }
        else
        {
            sdio_assert(0);
        }
    }
    else if (priv->state == STATE_TX)
    {
        complete_cnt_tx++;
        if (priv->tx_state == SDIO_TX_LAST_READ)
        {
            priv->tx_state = SDIO_TX_IDLE;

#if 0
            //os_printf(LM_OS, LL_INFO, "%s %d\n", __func__, __LINE__);
            sdio_tx_callback((void*)priv->tx_curr_buff);
            //os_printf(LM_OS, LL_INFO, "%s, 0x%08x \n", __func__, priv->tx_curr_buff);
            sdio_mem_tx_free(priv->tx_curr_buff);
#else
            //sdio_data_tx_cfm(priv->tx_curr_buff);
            sdio_tx_free_queue_push(priv->tx_curr_buff);
            sdio_tx_complete();
#endif
            priv->state = STATE_IDLE;
#ifdef CONFIG_PSM_SURPORT
            //psm_set_device_status(PSM_DEVICE_SDIO_SLAVE, PSM_DEVICE_STATUS_IDLE);
#endif
        }
    }
    else if (priv->state == STATE_INFO)
    {
        //unused state
        priv->state = STATE_IDLE;
#ifdef CONFIG_PSM_SURPORT
        //psm_set_device_status(PSM_DEVICE_SDIO_SLAVE, PSM_DEVICE_STATUS_IDLE);
#endif
    }
    else if(priv->state == STATE_INFO_ASYNC)
    {
        priv->state = STATE_IDLE;
#ifdef CONFIG_PSM_SURPORT
        //psm_set_device_status(PSM_DEVICE_SDIO_SLAVE, PSM_DEVICE_STATUS_IDLE);
#endif
    }
    else
    {
        sdio_assert(0);
    }

    if (priv->state == STATE_IDLE)
    {
        sdio_check_tx_buff(priv);
    }

    return;
}

/* isr callback*/
static void sdio_isr(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
    struct sdio_priv *priv = &m_priv;

    unsigned int status = SDIO_INT1_STATUS;

restart:

    if (status)
    {
        if (SDIO_INTR_PROGRAM_START & status)
        {
            SDIO_INT1_STATUS	= SDIO_INTR_PROGRAM_START;
            SDIO_ADMA_EN();
            SDIO_CONTROL		|= 0x1<<2;
            os_printf(LM_OS, LL_INFO, ">> SDIO Program start !!!\r\n");
        }
        else if (SDIO_INTR_WRITE_START & status)
        {
            sdio_isr_rx(priv);
            SDIO_INT1_STATUS = SDIO_INTR_WRITE_START;	
        }
        else if (SDIO_INTR_READ_START & status)
        {
            sdio_isr_tx(priv);
            SDIO_INT1_STATUS = SDIO_INTR_READ_START;	
        }
        else if (SDIO_INTR_TRANSFER_CMPL & status)
        {
            SDIO_INT1_STATUS = SDIO_INTR_TRANSFER_CMPL;
            sdio_isr_cmpl(priv);
        }
        else if (SDIO_INTR_SOFT_RESET & status)
        {
            SDIO_INT1_STATUS = SDIO_INTR_SOFT_RESET;
            sdio_hal_reset();
            os_printf(LM_OS, LL_INFO, ">> SDIO Soft reset\r\n");
        }
        else if (SDIO_INTR_FUNCX_ABORT & status)
        {
            SDIO_INT1_STATUS = SDIO_INTR_FUNCX_ABORT;
            sdio_hal_reset();
            os_printf(LM_OS, LL_INFO, ">> SDIO Abort\r\n");
        }
        else if (SDIO_INTR_FUNC1_RESET & status)
        {
            SDIO_INT1_STATUS = SDIO_INTR_FUNC1_RESET;
            os_printf(LM_OS, LL_INFO, ">> SDIO Func1 reset\r\n");
        }
        else if (SDIO_INTR_FUNCX_ERROR & status)
        {
            SDIO_INT1_STATUS = SDIO_INTR_FUNCX_ERROR;
            os_printf(LM_OS, LL_INFO, ">> SDIO CRC error\r\n");
        }
        else
        {
            os_printf(LM_OS, LL_INFO, ">> SDIO unexpected interrupt, 0x%08x\r\n", status);
            SDIO_INT1_STATUS = status;
            //while(1);
        }
    }
    else
    {
        os_printf(LM_OS, LL_INFO, ">> SDIO ADMA Interrupt error !!!\r\n");
        while(1);
    }

    status = SDIO_INT1_STATUS;
    if (status )
    {
        goto restart;
    }

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
	
    return;
}

#if 0
static int cmd_sdio_test(cmd_tbl_t *t, int argc, char *argv[])
{
    unsigned int len, total_len = 0, i;
    unsigned char *p;
    sdio_tx_param_t param;

    param.bufs.nb = 3;

    param.bufs.lens[0] = 64;
    param.bufs.bufs[0] = SDIO_DEBUG_TX_ADDR;

    total_len += param.bufs.lens[0];			

    param.bufs.lens[1] = 96;
    param.bufs.bufs[1] = SDIO_DEBUG_TX_ADDR + total_len;

    total_len += param.bufs.lens[1];			

    param.bufs.lens[2] = 24;
    param.bufs.bufs[2] = SDIO_DEBUG_TX_ADDR + total_len;

    total_len += param.bufs.lens[2];	

    os_printf(LM_OS, LL_INFO, "cmd_sdio_test, len: %d\r\n", total_len);

    p = (unsigned char *)SDIO_DEBUG_TX_ADDR;
    for (i=0; i<total_len; i++)
    {
        *p++ = (unsigned char)i;
    }

    sdio_send_to_host(&param);

    return 0;
}

CMD(sdiot,   cmd_sdio_test);


static int cmd_sdio_alt(cmd_tbl_t *t, int argc, char *argv[])
{
    struct sdio_priv *priv = &m_priv;

    if (priv->use_fun_num1)
    {
        priv->use_fun_num1 = 0;
    }
    else
    {
        priv->use_fun_num1 = 1;
    }

    os_printf(LM_OS, LL_INFO, "cmd_sdio_alt, func num: %d\r\n", (priv->use_fun_num1) ? (1) : (2));

    return 0;
}

CMD(sdioa,   cmd_sdio_alt);
#endif

/* must be called with irq lock */
static void sdio_tx_free_queue_push(sdio_tx_bufs_t *sysbuf)
{
    struct sdio_priv *priv = &m_priv;
    sdio_tx_bufs_t *head = priv->tx_free_head;
    sdio_tx_bufs_t *temp = head;

    sdio_assert(sysbuf);

    sysbuf->next_buf = NULL;

    if(head == NULL)
    {
        priv->tx_free_head = sysbuf;
    }
    else
    {
        while(temp->next_buf)
        {
            temp = temp->next_buf;
        }
        temp->next_buf = sysbuf;
    }
    //os_printf(LM_OS, LL_INFO, "push_tx_free_head:0x%08x, sysbuf: 0x%08x, count: %d \n", priv->tx_free_head, sysbuf, sdio_tx_free_queue_count(priv->tx_free_head));
    return;
}

/* for other modules start */
sdio_tx_bufs_t *sdio_tx_free_queue_pop()
{
    sdio_tx_bufs_t *res = NULL;
    struct sdio_priv *priv = &m_priv;
    unsigned long flags;

    //flags = arch_irq_save();

    flags = portSET_INTERRUPT_MASK_FROM_ISR();
    if(priv->tx_free_head)
    {
        res = priv->tx_free_head;
        priv->tx_free_head = priv->tx_free_head->next_buf;
    }
    //arch_irq_restore(flags);

    vPortClearInterruptMask(flags);
    res->next_buf = NULL;
    //os_printf(LM_OS, LL_INFO, "pop_tx_free_head:0x%08x, res: 0x%08x, count: %d \n", priv->tx_free_head, res, sdio_tx_free_queue_count(priv->tx_free_head));

    return res;
}

unsigned int sdio_send_to_host(sdio_tx_param_t *param)
{
    struct sdio_priv *priv = &m_priv;
    unsigned long flags;

    /* baoyong:todo: can be removed ? */
    if(!is_wifi_ready()){
        os_printf(LM_OS, LL_INFO, "wifi not ready\r\n");
        if (param->cb) {
            param->cb(param);
        }
        return 0;
    }

    //dump_sdio_bufs(&param->bufs);
    {
        unsigned int count = 0;
        unsigned int total_len = 0;
        sdio_tx_bufs_t *res_frag = &param->bufs;

        while(res_frag)
        {
            count += res_frag->lens;
            res_frag = res_frag->next_frag;
        }

        total_len = (count > 512) ? ((count + 511) & 0xFFFFFFE00):( count);
        param->bufs.total_len = (total_len) | (count<<16);
    }

    //flags = arch_irq_save();
    flags = portSET_INTERRUPT_MASK_FROM_ISR();
    sdio_tx_queue_push(&param->bufs);
    //os_printf(LM_OS, LL_INFO,"%s: type=0x%08x buf_cnt=%d\n", __func__, *((uint32_t *)(param->bufs.bufs) + 2), get_rx_buff_free_size());

    /* empty tx queue, send indicate when state is idle */
    if (priv->tx_next_state == SDIO_TX_IDLE) 
    {
        priv->tx_next_state = SDIO_TX_WAIT_2_INFORM;
    }

    if (priv->state == STATE_IDLE)
    {
        sdio_check_tx_buff(priv);
    }
    //arch_irq_restore(flags);
    vPortClearInterruptMask(flags);
    return 0;
}

/* init entry */
void sdio_init(void)
{
    struct sdio_priv *priv = &m_priv;

    os_printf(LM_OS, LL_INFO, "sdio_init\r\n");


    SDIO_PAD_SR = SDIO_PAD_SR | 0x1F80;

    chip_clk_enable(CLK_SDIO_SLAVE);
    sdio_hal_reset();

    memset(&m_priv, 0, sizeof(m_priv));
    memset((void*)SDIO_DES_BASE_INFO, 0, 16);
    memset((void*)SDIO_DES_BASE_RX, 0, 48);
    memset((void*)SDIO_DES_BASE_TX, 0, 48);

    arch_irq_clean(VECTOR_NUM_SDIO_SLAVE);
    arch_irq_unmask(VECTOR_NUM_SDIO_SLAVE);
    arch_irq_register(VECTOR_NUM_SDIO_SLAVE, sdio_isr);

    SDIO_ADMA_EN();

    sdio_queue_init(&priv->hwque[HIF_SDIO_RX]);
    //sdio_queue_init(&priv->hwque[HIF_SDIO_TX]);
    //sdio_queue_init(&priv->to_host_que);
    sdio_mem_init();

    priv->adma_curr = (volatile struct adma_descriptor*)(SDIO_DES_BASE_INFO);
    priv->adma_curr->addr =SDIO_MSG_BASE;
    priv->adma_curr->len = 16;
    priv->adma_curr->attri = SDIO_ADMA_ATTR_ACT_TRS | SDIO_ADMA_ATTR_VALID | SDIO_ADMA_ATTR_END;

    priv->state = STATE_IDLE;
#ifdef CONFIG_PSM_SURPORT
    //psm_set_device_status(PSM_DEVICE_SDIO_SLAVE, PSM_DEVICE_STATUS_IDLE);
#endif
    priv->tx_state = SDIO_TX_IDLE;
    priv->tx_next_state = SDIO_TX_IDLE;

	memset(sdio_buff, 0, 512);

    os_printf(LM_OS, LL_INFO, "sdio_start\r\n");
}

/* for other modules end */
