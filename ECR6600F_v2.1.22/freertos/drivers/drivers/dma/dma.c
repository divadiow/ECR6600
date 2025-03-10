







#include <stdio.h>
#include "chip_memmap.h"
#include "chip_clk_ctrl.h"
#include "arch_irq.h"
#include "chip_irqvector.h"
#include "dma.h"
#include "i2c.h"
#include "oshal.h"



#define DRV_DMA_CH_NUM	4

typedef volatile struct
{
	volatile unsigned int ChCtrl;			//  Channel control register
	volatile unsigned int ChSrcAddr;		//  Channel source address register
	volatile unsigned int ChDstAddr;		//  Channel destination address register
	volatile unsigned int ChTranSize;	//  Channel transfer size register		
	volatile unsigned int ChLLPointer	;	//  Channel linked list pointer register	
} DMA_LLDESCRIPTOR;


typedef  volatile struct _T_DMA_REG_MAP
{
	unsigned int IdRev;		/* 0x00  -- ID and revision register */
	unsigned int Resv0[3];		/* 0x04 ~ 0x0C  -- reserved register */
	unsigned int DMACfg;		/* 0x10  -- DMAC configuration register */
	unsigned int Resv1[3];		/* 0x14 ~ 0x1C  -- reserved register */
	unsigned int DMACtrl;		/* 0x20  -- DMA control register */
	unsigned int Resv2[3];		/* 0x24 ~ 0x2C  -- reserved register */
	unsigned int IntStatus;		/* 0x30  -- interrupt status register */
	unsigned int ChEN;			/* 0x34  -- Channel enable register */
	unsigned int Resv3[2];		/* 0x38 ~ 0x3C  -- reserved register */
	unsigned int ChAbort;		/* 0x40 -- Channel abort register */

	DMA_LLDESCRIPTOR  Ch[DRV_DMA_CH_NUM];

} T_DMA_REG_MAP;


#define DMAC_INTR_TC(X)				(((X) >> 16) & 0xFF)
#define DMAC_INTR_ABORT(X)			(((X) >> 8) & 0xFF)
#define DMAC_INTR_ERROR(X)			(((X) >> 0) & 0xFF)


#define CHN_CTRL_EN(x)					(((x) & 0x1)<<0)
#define CHN_CTRL_INT_TC_MASK(x)		(((x) & 0x1)<<1)
#define CHN_CTRL_INT_ERR_MASK(x)		(((x) & 0x1)<<2)
#define CHN_CTRL_INT_ABT_MASK(x)		(((x) & 0x1)<<3)
#define CHN_CTRL_DST_REQ_SEL(x)		(((x) & 0xF)<<4)
#define CHN_CTRL_SRC_REQ_SEL(x)		(((x) & 0xF)<<8)
#define CHN_CTRL_DST_ADDR_CTRL(x)		(((x) & 0x3)<<12)
#define CHN_CTRL_SRC_ADDR_CTRL(x)		(((x) & 0x3)<<14)
#define CHN_CTRL_DST_MODE(x)			(((x) & 0x1)<<16)
#define CHN_CTRL_SRC_MODE(x)			(((x) & 0x1)<<17)
#define CHN_CTRL_DST_WIDTH(x)			(((x) & 0x3)<<18)
#define CHN_CTRL_SRC_WIDTH(x)			(((x) & 0x3)<<20)
#define CHN_CTRL_SRC_BURST_SIZE(x)		(((x) & 0x7)<<22)
#define CHN_CTRL_PRIORITY(x)			(((x) & 0x1)<<29)

#define CHN_ENABLE		1
#define CHN_DISABLE		0

#define CHN_INT_MASK		1
#define CHN_INT_UNMASK		0


#define CHN_ADDR_INC	0
#define CHN_ADDR_DEC	1
#define CHN_ADDR_FIX	2

#define CHN_SRC_MODE_NORMAL		0
#define CHN_SRC_MODE_HANDSHAKE	1

#define CHN_WIDTH_BYTE			0
#define CHN_WIDTH_HALF_WORLD	1
#define CHN_WIDTH_WORLD		2

#define CHN_BURST_SIZE_1_TRANS	0
#define CHN_BURST_SIZE_2_TRANS	1
#define CHN_BURST_SIZE_4_TRANS	2
#define CHN_BURST_SIZE_8_TRANS	3
#define CHN_BURST_SIZE_16_TRANS	4
#define CHN_BURST_SIZE_32_TRANS	5
#define CHN_BURST_SIZE_64_TRANS	6
#define CHN_BURST_SIZE_128_TRANS	7

#define CHN_PRIORITY_LOW	0
#define CHN_PRIORITY_HIGH	1


#define CHN_REQ_SEL_UART0_TX        0
#define CHN_REQ_SEL_UART0_RX        1
#define CHN_REQ_SEL_UART1_TX        2
#define CHN_REQ_SEL_UART1_RX        3
#define CHN_REQ_SEL_I2C             4
#define CHN_REQ_SEL_SPI_TX          5
#define CHN_REQ_SEL_SPI_RX          6
#define CHN_REQ_SEL_SPI_FLASH_TX    7
#define CHN_REQ_SEL_SPI_FLASH_RX    8
#define CHN_REQ_SEL_I2S_TX          9
#define CHN_REQ_SEL_I2S_RX          10
#define CHN_REQ_SEL_UART2_TX        11
#define CHN_REQ_SEL_UART2_RX        12
#define CHN_REQ_SEL_AES             13
#define CHN_REQ_SEL_MEM             14


#define DRV_DMA_WIDTH_IS_WORLD(x)   ((((x)>>18) & 0xF) == 0xA)
#define DRV_DMA_WIDTH_IS_HALF_WORLD(x)   ((((x)>>18) & 0xF) == 0x5)


/*  dma mem & mem cfg */
#define CHN_CTRL_VALUE_MEM		CHN_CTRL_EN(CHN_ENABLE)		\
									|CHN_CTRL_INT_TC_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ERR_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ABT_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_DST_ADDR_CTRL(CHN_ADDR_INC)	\
									|CHN_CTRL_SRC_ADDR_CTRL(CHN_ADDR_INC)	\
									|CHN_CTRL_DST_MODE(CHN_SRC_MODE_NORMAL) \
									|CHN_CTRL_DST_WIDTH(CHN_WIDTH_BYTE)	\
									|CHN_CTRL_SRC_WIDTH(CHN_WIDTH_BYTE)	\
									|CHN_CTRL_SRC_BURST_SIZE(CHN_BURST_SIZE_1_TRANS)	\
									|CHN_CTRL_PRIORITY(CHN_PRIORITY_LOW)

/*  dma mem & uart cfg */
#define CHN_CTRL_VALUE_TX_BYTE(X)	CHN_CTRL_EN(CHN_ENABLE)		\
									|CHN_CTRL_INT_TC_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ERR_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ABT_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_DST_REQ_SEL(X) \
									|CHN_CTRL_DST_ADDR_CTRL(CHN_ADDR_FIX)	\
									|CHN_CTRL_SRC_ADDR_CTRL(CHN_ADDR_INC)	\
									|CHN_CTRL_DST_MODE(CHN_SRC_MODE_HANDSHAKE) \
									|CHN_CTRL_DST_WIDTH(CHN_WIDTH_BYTE)	\
									|CHN_CTRL_SRC_WIDTH(CHN_WIDTH_BYTE)	\
									|CHN_CTRL_SRC_BURST_SIZE(CHN_BURST_SIZE_1_TRANS)	\
									|CHN_CTRL_PRIORITY(CHN_PRIORITY_LOW)

#define CHN_CTRL_VALUE_I2S_TX_BYTE(X)	CHN_CTRL_EN(CHN_ENABLE)		\
									|CHN_CTRL_INT_TC_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ERR_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ABT_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_DST_REQ_SEL(X) \
									|CHN_CTRL_DST_ADDR_CTRL(CHN_ADDR_FIX)	\
									|CHN_CTRL_SRC_ADDR_CTRL(CHN_ADDR_INC)	\
									|CHN_CTRL_DST_MODE(CHN_SRC_MODE_HANDSHAKE) \
									|CHN_CTRL_DST_WIDTH(CHN_WIDTH_HALF_WORLD)	\
									|CHN_CTRL_SRC_WIDTH(CHN_WIDTH_HALF_WORLD)	\
									|CHN_CTRL_SRC_BURST_SIZE(CHN_BURST_SIZE_1_TRANS)	\
									|CHN_CTRL_PRIORITY(CHN_PRIORITY_LOW)
									
#define CHN_CTRL_VALUE_I2S_RX_BYTE(X)	CHN_CTRL_EN(CHN_ENABLE)		\
									|CHN_CTRL_INT_TC_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ERR_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ABT_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_SRC_REQ_SEL(X) \
									|CHN_CTRL_DST_ADDR_CTRL(CHN_ADDR_INC)	\
									|CHN_CTRL_SRC_ADDR_CTRL(CHN_ADDR_FIX)	\
									|CHN_CTRL_SRC_MODE(CHN_SRC_MODE_HANDSHAKE) \
									|CHN_CTRL_DST_WIDTH(CHN_WIDTH_HALF_WORLD)	\
									|CHN_CTRL_SRC_WIDTH(CHN_WIDTH_HALF_WORLD)	\
									|CHN_CTRL_SRC_BURST_SIZE(CHN_BURST_SIZE_1_TRANS)	\
									|CHN_CTRL_PRIORITY(CHN_PRIORITY_LOW)

#define CHN_CTRL_VALUE_RX_BYTE(X)	CHN_CTRL_EN(CHN_ENABLE)		\
									|CHN_CTRL_INT_TC_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ERR_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ABT_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_SRC_REQ_SEL(X) \
									|CHN_CTRL_DST_ADDR_CTRL(CHN_ADDR_INC)	\
									|CHN_CTRL_SRC_ADDR_CTRL(CHN_ADDR_FIX)	\
									|CHN_CTRL_SRC_MODE(CHN_SRC_MODE_HANDSHAKE) \
									|CHN_CTRL_DST_WIDTH(CHN_WIDTH_BYTE)	\
									|CHN_CTRL_SRC_WIDTH(CHN_WIDTH_BYTE)	\
									|CHN_CTRL_SRC_BURST_SIZE(CHN_BURST_SIZE_1_TRANS)	\
									|CHN_CTRL_PRIORITY(CHN_PRIORITY_LOW)

#define CHN_CTRL_VALUE_TX_WORLD(X)	CHN_CTRL_EN(CHN_ENABLE)		\
									|CHN_CTRL_INT_TC_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ERR_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ABT_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_DST_REQ_SEL(X) \
									|CHN_CTRL_DST_ADDR_CTRL(CHN_ADDR_FIX)	\
									|CHN_CTRL_SRC_ADDR_CTRL(CHN_ADDR_INC)	\
									|CHN_CTRL_DST_MODE(CHN_SRC_MODE_HANDSHAKE) \
									|CHN_CTRL_DST_WIDTH(CHN_WIDTH_WORLD)	\
									|CHN_CTRL_SRC_WIDTH(CHN_WIDTH_WORLD)	\
									|CHN_CTRL_SRC_BURST_SIZE(CHN_BURST_SIZE_1_TRANS)	\
									|CHN_CTRL_PRIORITY(CHN_PRIORITY_LOW)


#define CHN_CTRL_VALUE_RX_WORLD(X)	CHN_CTRL_EN(CHN_ENABLE)		\
									|CHN_CTRL_INT_TC_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ERR_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_INT_ABT_MASK(CHN_INT_UNMASK)	\
									|CHN_CTRL_SRC_REQ_SEL(X) \
									|CHN_CTRL_DST_ADDR_CTRL(CHN_ADDR_INC)	\
									|CHN_CTRL_SRC_ADDR_CTRL(CHN_ADDR_FIX)	\
									|CHN_CTRL_SRC_MODE(CHN_SRC_MODE_HANDSHAKE) \
									|CHN_CTRL_DST_WIDTH(CHN_WIDTH_WORLD)	\
									|CHN_CTRL_SRC_WIDTH(CHN_WIDTH_WORLD)	\
									|CHN_CTRL_SRC_BURST_SIZE(CHN_BURST_SIZE_1_TRANS)	\
									|CHN_CTRL_PRIORITY(CHN_PRIORITY_LOW)

#define 	DRV_DMA_DESCRIPTOR_MAX		4


typedef struct
{
	unsigned int count;
	unsigned int dma_ctrl_cfg;
	DMA_LLDESCRIPTOR llDes[DRV_DMA_DESCRIPTOR_MAX];
} DMA_CHAIN;


typedef struct
{
	T_DRV_DMA_CALLBACK dma_callback;
	DMA_CHAIN dma_chn_lldes;
} T_DMA_CHN;


typedef struct
{
	char init;
	char alloc_flag;
	T_DMA_REG_MAP * reg_base;
	T_DMA_CHN  * p_chn;
} T_DRV_DMA_DEV;

static T_DMA_CHN  dma_chn[DRV_DMA_CH_NUM]  __attribute__((section(".dma.data")));
//static T_DMA_CHN  dma_chn[DRV_DMA_CH_NUM] ;
static T_DRV_DMA_DEV dma_dev;

////////////////////////////////////////////////////////////////////////
#if 0
static unsigned int dma_get_ctrl_cfg(T_DMA_CFG_INFO * cfg)
{
	unsigned int val = 0;
	E_DMA_CHN_MODE mode = cfg->mode;

	switch (mode)
	{
		case DMA_CHN_UART0_TX:
		case DMA_CHN_UART1_TX:
		case DMA_CHN_UART2_TX:
			val = CHN_CTRL_VALUE_TX_BYTE(mode);
			break;
		case DMA_MODE_I2C_TX:
            val = CHN_CTRL_VALUE_TX_BYTE(CHN_REQ_SEL_I2C);
            break;
        case DMA_MODE_I2C_RX:
            val = CHN_CTRL_VALUE_RX_BYTE(CHN_REQ_SEL_I2C);
			break;
		case DMA_CHN_I2S_TX:
			val = CHN_CTRL_VALUE_I2S_TX_BYTE(mode);
			break;
		case DMA_CHN_I2S_RX:
			val = CHN_CTRL_VALUE_I2S_RX_BYTE(mode);
			break;
		case DMA_CHN_SPI_TX:
		case DMA_CHN_SPI_FLASH_TX:
			val = CHN_CTRL_VALUE_TX_WORLD(CHN_REQ_SEL_SPI_TX);
			cfg->len = (cfg->len+3) / 4;
			break;

		case DMA_CHN_UART0_RX:
		case DMA_CHN_UART1_RX:
		case DMA_CHN_UART2_RX:
			val = CHN_CTRL_VALUE_RX_BYTE(mode);
			break;

		case DMA_CHN_SPI_RX:
		case DMA_CHN_SPI_FLASH_RX:
			val = CHN_CTRL_VALUE_RX_WORLD(mode);
			cfg->len =(cfg->len+3) / 4;
			break;

		case DMA_CHN_AES:
			while(1);
			break;

		case DMA_CHN_MEM:
			val = CHN_CTRL_VALUE_MEM;
			break;
	}

	return val;
}
#endif
static unsigned int dma_get_ctrl_cfg(T_DMA_CFG_INFO * cfg)
{
	unsigned int val = 0;
	E_DMA_CHN_MODE mode = cfg->mode;
    static unsigned int dma_cfg[DMA_CHN_MEM + 1] =
    {
        CHN_CTRL_VALUE_TX_BYTE(CHN_REQ_SEL_UART0_TX),
        CHN_CTRL_VALUE_RX_BYTE(CHN_REQ_SEL_UART0_RX),
        CHN_CTRL_VALUE_TX_BYTE(CHN_REQ_SEL_UART1_TX),
        CHN_CTRL_VALUE_RX_BYTE(CHN_REQ_SEL_UART1_RX),
        CHN_CTRL_VALUE_TX_BYTE(CHN_REQ_SEL_I2C),
        CHN_CTRL_VALUE_RX_BYTE(CHN_REQ_SEL_I2C),
        CHN_CTRL_VALUE_TX_WORLD(CHN_REQ_SEL_SPI_TX),
        CHN_CTRL_VALUE_RX_WORLD(CHN_REQ_SEL_SPI_RX),
        CHN_CTRL_VALUE_TX_WORLD(CHN_REQ_SEL_SPI_FLASH_TX),
        CHN_CTRL_VALUE_RX_WORLD(CHN_REQ_SEL_SPI_FLASH_RX),
        CHN_CTRL_VALUE_I2S_TX_BYTE(CHN_REQ_SEL_I2S_TX),
        CHN_CTRL_VALUE_I2S_RX_BYTE(CHN_REQ_SEL_I2S_RX),
        CHN_CTRL_VALUE_TX_BYTE(CHN_REQ_SEL_UART2_TX),
        CHN_CTRL_VALUE_RX_BYTE(CHN_REQ_SEL_UART2_RX),
        CHN_CTRL_VALUE_MEM
    };

    if(mode > DMA_CHN_MEM )
    {
        return -1;
    }

    val = dma_cfg[mode];

    if(DRV_DMA_WIDTH_IS_WORLD(val))
    {
        cfg->len = (cfg->len + 3)/4;
    }
    else if(DRV_DMA_WIDTH_IS_HALF_WORLD(val))
    {
        cfg->len = (cfg->len + 1)/2;
    }
        
	return val;
}

static void drv_dma_isr(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	int i;
	T_DRV_DMA_DEV * p_dma_dev = &dma_dev;

	unsigned int status = p_dma_dev->reg_base->IntStatus;

	p_dma_dev->reg_base->IntStatus = status;

	if (DMAC_INTR_ERROR(status))
	{
		while(1);
	}

	if (DMAC_INTR_ABORT(status))
	{
		for (i=0; i<DRV_DMA_CH_NUM; i++)
		{
			if (((DMAC_INTR_ABORT(status) >> i) & 0x1) && (dma_chn[i].dma_callback.callback != NULL))
			{
				//p_dma_dev->reg_base->Ch[i].ChTranSize = 0;
				//dma_chn[i].dma_callback.callback(dma_chn[i].dma_callback.data);
				dma_chn[i].dma_chn_lldes.count = 0;
			}
		}
	}

	if (DMAC_INTR_TC(status))
	{
		for (i=0; i<DRV_DMA_CH_NUM; i++)
		{
			if (((DMAC_INTR_TC(status) >> i) & 0x1) && (dma_chn[i].dma_callback.callback != NULL))
			{
				dma_chn[i].dma_callback.callback(dma_chn[i].dma_callback.data);
				dma_chn[i].dma_chn_lldes.count = 0;
			}
		}
	}

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}

void drv_dma_status_clean(int num)
{
	T_DRV_DMA_DEV * p_dma_dev = &dma_dev;

	p_dma_dev->reg_base->IntStatus  = (1<<num)<<16;
	p_dma_dev->p_chn[num].dma_chn_lldes.count = 0;
}

int drv_dma_cfg(int num, T_DMA_CFG_INFO * cfg)
{
	T_DRV_DMA_DEV * p_dma_dev = &dma_dev;
	DMA_CHAIN * pDMA_chn_chain = &p_dma_dev->p_chn[num].dma_chn_lldes;
	DMA_LLDESCRIPTOR *pDMA_chn_LLD = &p_dma_dev->reg_base->Ch[num];

	if (pDMA_chn_chain->count == 0)
	{
		pDMA_chn_chain->dma_ctrl_cfg = dma_get_ctrl_cfg(cfg);

		pDMA_chn_LLD->ChSrcAddr = cfg->src;
		pDMA_chn_LLD->ChDstAddr = cfg->dst;
		pDMA_chn_LLD->ChTranSize = cfg->len;
		pDMA_chn_LLD->ChLLPointer = 0;
	}
	else if (pDMA_chn_chain->count > DRV_DMA_DESCRIPTOR_MAX)
	{
		return -1;
	}
	else
	{
		if (pDMA_chn_chain->count == 1)
		{
			pDMA_chn_LLD->ChLLPointer = (unsigned int)(&(pDMA_chn_chain->llDes[pDMA_chn_chain->count-1]));
		}
		else
		{
			pDMA_chn_chain->llDes[pDMA_chn_chain->count-2].ChLLPointer = \
				(unsigned int)(&(pDMA_chn_chain->llDes[pDMA_chn_chain->count-1]));
		}

		pDMA_chn_chain->llDes[pDMA_chn_chain->count-1].ChCtrl = dma_get_ctrl_cfg(cfg);
		pDMA_chn_chain->llDes[pDMA_chn_chain->count-1].ChSrcAddr = cfg->src;
		pDMA_chn_chain->llDes[pDMA_chn_chain->count-1].ChDstAddr = cfg->dst;
		pDMA_chn_chain->llDes[pDMA_chn_chain->count-1].ChTranSize = cfg->len;
		pDMA_chn_chain->llDes[pDMA_chn_chain->count-1].ChLLPointer = 0;
	}

	pDMA_chn_chain->count ++;
	return 0;
}


void drv_dma_start(int num)
{
	T_DRV_DMA_DEV * p_dma_dev = &dma_dev;
	p_dma_dev->reg_base->Ch[num].ChCtrl = 
		p_dma_dev->p_chn[num].dma_chn_lldes.dma_ctrl_cfg;

}


unsigned int drv_dma_stop(int num)
{
//	unsigned int len;
	T_DRV_DMA_DEV * p_dma_dev = &dma_dev;

	p_dma_dev->reg_base->ChAbort = 1<<num;
#if 1
	return p_dma_dev->reg_base->Ch[num].ChTranSize;

#else
	len = p_dma_dev->reg_base->Ch[num].ChTranSize;
	p_dma_dev->reg_base->Ch[num].ChTranSize = 0;

	return len;
#endif
}



void drv_dma_isr_register(int num, void (* callback)(void *), void  *data)
{
	dma_chn[num].dma_callback.callback = callback;
	dma_chn[num].dma_callback.data = data;
}


int  drv_dma_ch_alloc(void)
{
	int i;
	unsigned long flags;
	T_DRV_DMA_DEV * p_dma_dev = &dma_dev;

	if (p_dma_dev->init == 0)
	{
		chip_clk_enable(CLK_DMA);
		p_dma_dev->p_chn = dma_chn;
		p_dma_dev->reg_base = (T_DMA_REG_MAP *)MEM_BASE_DMAC;
		p_dma_dev->init = 1;
		arch_irq_register(VECTOR_NUM_DMA, drv_dma_isr);
		arch_irq_unmask(VECTOR_NUM_DMA);
	}

	for (i=0; i < DRV_DMA_CH_NUM; i++)
	{
		flags = system_irq_save();
		if ((p_dma_dev->alloc_flag & (1<<i)) == 0)
		{
			p_dma_dev->alloc_flag |= 1<<i;
			p_dma_dev->p_chn[i].dma_chn_lldes.count = 0;
			system_irq_restore(flags);
			return i;
		}

		system_irq_restore(flags);
	}

	return -1;
}


