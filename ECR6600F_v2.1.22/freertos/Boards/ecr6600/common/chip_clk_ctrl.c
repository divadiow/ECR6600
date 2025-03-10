





#include "chip_smupd.h"
#include "chip_clk_ctrl.h"
#include "arch_irq.h"
#include "pit.h"
#include "efuse.h"
#include "reg_macro_def.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif

//#define CHIP_GET_REG(reg)                (*((volatile unsigned int *) (reg)))
//#define CHIP_SET_REG(reg, data)         ((*((volatile unsigned int *)(reg)))=(unsigned int)(data))



#define CLK_MASK_TYPE		0xFF00
#define CLK_MASK_ID			0x00FF

void chip_clk_enable(unsigned int clk_id)
{
	unsigned long flags = arch_irq_save();

	if (clk_id & CLK_MASK_TYPE)
	{
		WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_1,   \
			READ_REG(CHIP_SMU_PD_CLK_ENABLE_1) | (1 <<(clk_id & CLK_MASK_ID)));
		//((*((volatile unsigned int *)CHIP_SMU_PD_CLK_ENABLE_1)) =
		//	(unsigned int)((*((volatile unsigned int *)CHIP_SMU_PD_CLK_ENABLE_1))|(1 <<(clk_id & CLK_MASK_ID))));
	}
	else
	{
		WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0,   \
			READ_REG(CHIP_SMU_PD_CLK_ENABLE_0) | (1 <<(clk_id & CLK_MASK_ID)));
		// ((*((volatile unsigned int *)CHIP_SMU_PD_CLK_ENABLE_0)) =
		//	(unsigned int)((*((volatile unsigned int *)CHIP_SMU_PD_CLK_ENABLE_0))|(1 <<(clk_id & CLK_MASK_ID))));
	}

	arch_irq_restore(flags);
}

void chip_clk_disable(unsigned int clk_id)
{
	unsigned long flags = arch_irq_save();

	if (clk_id & CLK_MASK_TYPE)
	{
		*((volatile unsigned int *)CHIP_SMU_PD_CLK_ENABLE_1) =
			(*((volatile unsigned int *)CHIP_SMU_PD_CLK_ENABLE_1)) & (~ (1 <<( clk_id & CLK_MASK_ID )));
	}
	else
	{
		*((volatile unsigned int *)CHIP_SMU_PD_CLK_ENABLE_0) =
			(*((volatile unsigned int *)CHIP_SMU_PD_CLK_ENABLE_0)) & (~ (1 <<( clk_id & CLK_MASK_ID )));
	}

	arch_irq_restore(flags);
}


#define CHIP_RF_DIG_PLL1				(MEM_BASE_RFC_REG + 0x84)
#define CHIP_RF_DIG_PLL2				(MEM_BASE_RFC_REG + 0x88)
#define CHIP_RF_DIG_PLL3				(MEM_BASE_RFC_REG + 0x8C)

#define CHIP_RF_PLL_REG12				(MEM_BASE_RFC_REG + 0x130)
#define CHIP_RF_PLL_REG13				(MEM_BASE_RFC_REG + 0x134)
#define CHIP_RF_PLL_REG14				(MEM_BASE_RFC_REG + 0x138)
#define CHIP_RF_PLL_REG18				(MEM_BASE_RFC_REG + 0x148)
#define CHIP_RF_PLL_REG20				(MEM_BASE_RFC_REG + 0x150)

#define RF_CTRL_DCXO_REG6				(MEM_BASE_PCU + 0x18)

//static 
#if 0
void chip_clk_bbpll_enable(void)
{
#if 0
#if 0 /* modified by baoyong, eco chip do not need it */
	CHIP_SET_REG(0x203094 , 0xc10001);

	//CHIP_SET_REG(0x201218 , 0x41f80);

	CHIP_SET_REG(0x203098, 0x5000000);	//AO gating 1     
	CHIP_SET_REG(0x201008, 0x180);		//AO gating 2    
	CHIP_SET_REG(0x201000, 0x8001);		//LPBG2MBG main gating
#endif

	CHIP_SET_REG(CHIP_RF_DIG_PLL3, 0x2e43c06c);
	CHIP_SET_REG(CHIP_RF_DIG_PLL1, 0x2B00);
	drv_pit_delay(400);

	CHIP_SET_REG(CHIP_RF_PLL_REG18,0x0);
	CHIP_SET_REG(CHIP_RF_PLL_REG12,0x0);
	CHIP_SET_REG(0x20314C,0x1102);
	CHIP_SET_REG(CHIP_RF_PLL_REG20,0x300);

	CHIP_SET_REG(CHIP_RF_PLL_REG18,0x1);
	CHIP_SET_REG(CHIP_RF_PLL_REG18,0x0);

	drv_pit_delay(160);

	CHIP_SET_REG(CHIP_RF_PLL_REG13,0x30);

	drv_pit_delay(40);

	CHIP_SET_REG(CHIP_RF_DIG_PLL3,0x2e438260);
	CHIP_SET_REG(CHIP_RF_DIG_PLL2, 0x1a687f74);

	CHIP_SET_REG(CHIP_RF_PLL_REG12,0x3);
	CHIP_SET_REG(CHIP_RF_DIG_PLL3,0x2ec38220);

#else

	unsigned int value;
	unsigned int val;//EFUSE_CRYSTAL_OFFSET;
	
	drv_efuse_read(EFUSE_CTRL_OFFSET, (unsigned int *)&val, 4);
	if((val & EFUSE_CRYSTAL_26M) == EFUSE_CRYSTAL_40M)
	{
		/**            40mhz             **/
		// cfg  dig_ari_wifi_bbpll_cp_ilk_sel, 0x0020308C:::bit7:4 = 6
		
		value = CHIP_GET_REG(CHIP_RF_DIG_PLL3);
		value = (value & 0xFFFFFF0F) | 0x60 ;
		CHIP_SET_REG(CHIP_RF_DIG_PLL3, value);
	}
	else
	{
		/**            26mhz             **/
		// cfg  dig_ari_wifi_bbpll_cp_ilk_sel, 0x0020308C:::bit7:4 = 6
		// cfg  dig_ari_wifi_bbpll_prse_div_shift, 0x0020308C:::bit15 = 0
		value = CHIP_GET_REG(CHIP_RF_DIG_PLL3);
		value = (value & 0xFFFF7F0F) | 0x60 ;
		CHIP_SET_REG(CHIP_RF_DIG_PLL3, value);
		
		// cfg  dig_ari_wifi_bbpll_lpf_rs_sel, 0x00203088:::bit20:14 = 0x25
		value = CHIP_GET_REG(CHIP_RF_DIG_PLL2);
		value = (value & (~(0x7F<<14))) | (0x25<<14);
		CHIP_SET_REG(CHIP_RF_DIG_PLL2, value);
		
		// cfg  rfc_reg_dpll_div_int, 0x00203134:::bit7:0 = 0x49
		CHIP_SET_REG(CHIP_RF_PLL_REG13, 0x49);
	
		// cfg  rfc_reg_dpll_div_frac, 0x00203138:::bit23:0 = 0x6C4EC5
		CHIP_SET_REG(CHIP_RF_PLL_REG14, 0x6C4EC5);

		// cfg  rfc_reg_dpll_freq_in, 0x00203150:::bit13:0 = 0x49E
		CHIP_SET_REG(CHIP_RF_PLL_REG20, 0x49E);	
	}

	//enable  dig_ari_wifi_bbpll_ldo_pu, 0x00203084:::bit13 = 1
	value = CHIP_GET_REG(CHIP_RF_DIG_PLL1);
	value |= 1<<13;
	CHIP_SET_REG(CHIP_RF_DIG_PLL1, value);

	// delay 10us for stability
	drv_pit_delay(100);

	// enable rfc_reg_dpll_ fcal_en,  0x00203148:::bit0 = 1
	CHIP_SET_REG(CHIP_RF_PLL_REG18, 1);

	// delay 10us for stability
	drv_pit_delay(100);

	//cfg  dig_ari_wifi_bbpll_prsc_pdiv2_pu  0x0020308C:::bit14 = 0
	//cfg  dig_ari_wifi_bbpll_vco_cal_pu  0x0020308C:::bit3 = 0
	//cfg  dig_ari_wifi_bbpll_vco_calrefl_en  0x0020308C:::bit2:0 = 0
	value = CHIP_GET_REG(CHIP_RF_DIG_PLL3);
	value &= ~((1<<14) |0xF);
	CHIP_SET_REG(CHIP_RF_DIG_PLL3, value);

	//cfg  rfc_reg_bbpll_fcal_en  0x00203148:::bit0 = 0
	CHIP_SET_REG(CHIP_RF_PLL_REG18, 0);

	// delay 5us for stability
	drv_pit_delay(50);

	// cfg dig_ari_wifi_bbpll_pfd_pu  0x00203088:::bit27 = 1
	value = CHIP_GET_REG(CHIP_RF_DIG_PLL2);
	value |= 1<<27;
	CHIP_SET_REG(CHIP_RF_DIG_PLL2, value);

	// cfg rfc_reg_dpll_sdm_en  0x00203088:::bit0 = 1
	value = CHIP_GET_REG(CHIP_RF_PLL_REG12);
	value |= 1;
	CHIP_SET_REG(CHIP_RF_PLL_REG12, value);

	// delay 20us for stability
	drv_pit_delay(200);

	// cfg dig_ari_wifi_bbpll_fout_postdiv_pu  0x0020308C:::bit23 = 1
	value = CHIP_GET_REG(CHIP_RF_DIG_PLL3);
	value |= 1<<23;
	CHIP_SET_REG(CHIP_RF_DIG_PLL3, value);	
#endif
}
#else
void chip_clk_bbpll_enable(void)
{
	volatile unsigned int value;
	WRITE_REG(RF_CTRL_DCXO_REG6,0x01f03201);	//note:eco3 isn't need 
	while(1)
	{
		// 0x00203084	
		WRITE_REG(CHIP_RF_DIG_PLL1, 0x1300);
		// 0x00203130	
		WRITE_REG(CHIP_RF_PLL_REG12, 0x6);
		// 0x00203130	
		WRITE_REG(CHIP_RF_PLL_REG12, 0x2);
		// 0x0020308c
		WRITE_REG(CHIP_RF_DIG_PLL3, 0x0e4345ac);
		// 0x00203088
		WRITE_REG(CHIP_RF_DIG_PLL2, 0x12697f74);
		// 0x00203134
		WRITE_REG(CHIP_RF_PLL_REG13, 0x49);
		// 0x00203138
		WRITE_REG(CHIP_RF_PLL_REG14, 0x006c4ec5);
		// 0x00203150
		WRITE_REG(CHIP_RF_PLL_REG20, 0x49e);
		// 0x00203084
		WRITE_REG(CHIP_RF_DIG_PLL1, 0x3300);
		drv_pit_delay(10);
		// 0x00203148
		WRITE_REG(CHIP_RF_PLL_REG18, 0x1);
		drv_pit_delay(40);
		WRITE_REG(CHIP_RF_PLL_REG18, 0x0);
		value = (READ_REG(0x203158) >> 16) & 0xffff;
		if(value <= 0x4AE && value >= 0x48E)
		{
			break;
		}
	}
	// 0x0020308c
	WRITE_REG(CHIP_RF_DIG_PLL3, 0x2e4305a0);
	// 0x00203088
	WRITE_REG(CHIP_RF_DIG_PLL2, 0x1a697f74);
	// 0x00203084	
	WRITE_REG(CHIP_RF_DIG_PLL1, 0x2300);
	// 0x00203130	
	WRITE_REG(CHIP_RF_PLL_REG12, 0x3);
	drv_pit_delay(10);
	// 0x0020308c
	WRITE_REG(CHIP_RF_DIG_PLL3, 0x2ec305a0);
}
#endif




#define CHIP_SMU_PD_CLK_DIV5_EN					(1<<4)
#define CHIP_SMU_PD_CLK_SDIOH_48M_EN				(1<<3)
#define CHIP_SMU_PD_CLK_UART_TRNG_80M_EN		(1<<2)
#define CHIP_SMU_PD_CLK_DIV3_EN					(1<<1)
#define CHIP_SMU_PD_CLK_DIV2_EN					(1<<0)


#define CHIP_SMU_PD_CORE_CLK_SRC_40M_26M		(0)
#define CHIP_SMU_PD_CORE_CLK_SRC_240M			(1)
#define CHIP_SMU_PD_CORE_CLK_SRC_160M			(2)
#define CHIP_SMU_PD_CORE_CLK_SRC_96M				(3)

#define CHIP_SMU_PD_CORE_CLK_DIV2					(0<<2)
#define CHIP_SMU_PD_CORE_CLK_DIV0					(1<<2)
#define CHIP_SMU_PD_CORE_CLK_DIV4					(2<<2)
#define CHIP_SMU_PD_CORE_CLK_DIV8					(3<<2)

#define CHIP_SMU_PD_UART_CLK_SEL_40M_26M		(0)
#define CHIP_SMU_PD_UART_CLK_SEL_80M				(1)


void chip_clk_init(void)
{
	unsigned int value = READ_REG(CHIP_SMU_PD_CLK_DIV_EN);

	WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0, READ_REG(CHIP_SMU_PD_CLK_ENABLE_0) \
		| (1<<CLK_DIG_PLL_FREF) | (1<<CLK_RF_FREF) | (1<<CLK_EFUSE) |(1<<CLK_RF_APB)  |(1<<CLK_IRAM0) |(1<<CLK_IRAM1) );

	if (value == 0)
	{
		chip_clk_bbpll_enable();
	}

#ifdef CONFIG_PSM_SURPORT
    unsigned int pit_when_sleep;
    drv_pit_ioctrl(DRV_PIT_CHN_6, DRV_PIT_CTRL_GET_COUNT, (unsigned int)&pit_when_sleep);
    psm_pit_when_sleep_op(true, pit_when_sleep);
#endif

#if defined (CONFIG_CPU_CLK_SRC_96M)

	WRITE_REG(CHIP_SMU_PD_CLK_DIV_EN , \
		value |CHIP_SMU_PD_CLK_DIV5_EN |CHIP_SMU_PD_CLK_DIV3_EN |CHIP_SMU_PD_CLK_UART_TRNG_80M_EN);
	WRITE_REG(CHIP_SMU_PD_UART_CLK_SEL , CHIP_SMU_PD_UART_CLK_SEL_80M);
	drv_pit_delay(160);
	value = CHIP_SMU_PD_CORE_CLK_SRC_96M ;

#elif defined (CONFIG_CPU_CLK_SRC_160M)

	WRITE_REG(CHIP_SMU_PD_CLK_DIV_EN , \
		value |CHIP_SMU_PD_CLK_DIV2_EN |CHIP_SMU_PD_CLK_DIV3_EN |CHIP_SMU_PD_CLK_UART_TRNG_80M_EN );
	WRITE_REG(CHIP_SMU_PD_UART_CLK_SEL , CHIP_SMU_PD_UART_CLK_SEL_80M);
	drv_pit_delay(10);
	value = CHIP_SMU_PD_CORE_CLK_SRC_160M;

#elif defined (CONFIG_CPU_CLK_SRC_240M)

	WRITE_REG(CHIP_SMU_PD_CLK_DIV_EN , \
		value |CHIP_SMU_PD_CLK_DIV2_EN |CHIP_SMU_PD_CLK_DIV3_EN |CHIP_SMU_PD_CLK_UART_TRNG_80M_EN );
	WRITE_REG(CHIP_SMU_PD_UART_CLK_SEL , CHIP_SMU_PD_UART_CLK_SEL_80M);
	drv_pit_delay(10);
	value = CHIP_SMU_PD_CORE_CLK_SRC_240M ;

#elif defined (CONFIG_CPU_CLK_SRC_40m)

	WRITE_REG(CHIP_SMU_PD_CLK_DIV_EN , value |CHIP_SMU_PD_CLK_DIV2_EN |CHIP_SMU_PD_CLK_DIV3_EN );
	WRITE_REG(CHIP_SMU_PD_UART_CLK_SEL , CHIP_SMU_PD_UART_CLK_SEL_40M_26M);
	drv_pit_delay(10);
	value = CHIP_SMU_PD_CORE_CLK_SRC_40M_26M ;

#endif

#if defined (CONFIG_CPU_CLK_FREQ_DIV_0)

	value |= CHIP_SMU_PD_CORE_CLK_DIV0;

#elif defined (CONFIG_CPU_CLK_FREQ_DIV_2)

	value |= CHIP_SMU_PD_CORE_CLK_DIV2;

#elif defined (CONFIG_CPU_CLK_FREQ_DIV_4)

	value |= CHIP_SMU_PD_CORE_CLK_DIV4;

#elif defined (CONFIG_CPU_CLK_FREQ_DIV_8)

	value |= CHIP_SMU_PD_CORE_CLK_DIV8;

#endif

	WRITE_REG(CHIP_SMU_PD_CORE_CTRL, value);
}

void chip_clk_config_40M_26M(void)
{
    unsigned int value = READ_REG(CHIP_SMU_PD_CLK_DIV_EN);

    WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0, READ_REG(CHIP_SMU_PD_CLK_ENABLE_0) \
        | (1<<CLK_DIG_PLL_FREF) | (1<<CLK_RF_FREF) | (1<<CLK_EFUSE) |(1<<CLK_RF_APB)  |(1<<CLK_IRAM0) |(1<<CLK_IRAM1) );

    if (value == 0)
    {
        chip_clk_bbpll_enable();
    }

    //CHIP_SET_REG(CHIP_SMU_PD_SW_RESET, CHIP_GET_REG(CHIP_SMU_PD_SW_RESET) & ~(0x1<<15));
    //CHIP_SET_REG(CHIP_SMU_PD_UART_CLK_SEL , CHIP_SMU_PD_UART_CLK_SEL_40M_26M);
    //CHIP_SET_REG(CHIP_SMU_PD_SW_RESET, CHIP_GET_REG(CHIP_SMU_PD_SW_RESET) | (0x1<<15));

    value = CHIP_SMU_PD_CORE_CLK_SRC_40M_26M;

#if 0
#if defined (CONFIG_CPU_CLK_FREQ_DIV_0)
    value |= CHIP_SMU_PD_CORE_CLK_DIV0;
#elif defined (CONFIG_CPU_CLK_FREQ_DIV_2)
    value |= CHIP_SMU_PD_CORE_CLK_DIV2;
#elif defined (CONFIG_CPU_CLK_FREQ_DIV_4)
    value |= CHIP_SMU_PD_CORE_CLK_DIV4;
#elif defined (CONFIG_CPU_CLK_FREQ_DIV_8)
    value |= CHIP_SMU_PD_CORE_CLK_DIV8;
#endif
#endif

#ifdef CONFIG_PSM_SURPORT
    unsigned int pit_when_sleep;
    drv_pit_ioctrl(DRV_PIT_CHN_6, DRV_PIT_CTRL_GET_COUNT, (unsigned int)&pit_when_sleep);
    psm_pit_when_sleep_op(true, pit_when_sleep);
#endif

    value |= CHIP_SMU_PD_CORE_CLK_DIV0;

    WRITE_REG(CHIP_SMU_PD_CORE_CTRL, value);
    drv_pit_delay(10);
    WRITE_REG(CHIP_SMU_PD_CLK_DIV_EN , CHIP_SMU_PD_CLK_DIV3_EN |CHIP_SMU_PD_CLK_UART_TRNG_80M_EN);
    //CHIP_SET_REG(CHIP_SMU_PD_CLK_DIV_EN , 0);
}

void chip_clk_config_40M_26M_efuse(void)
{
    unsigned int value = READ_REG(CHIP_SMU_PD_CLK_DIV_EN);

    WRITE_REG(CHIP_SMU_PD_CLK_ENABLE_0, READ_REG(CHIP_SMU_PD_CLK_ENABLE_0) \
        | (1<<CLK_DIG_PLL_FREF) | (1<<CLK_RF_FREF) | (1<<CLK_EFUSE) |(1<<CLK_RF_APB)  |(1<<CLK_IRAM0) |(1<<CLK_IRAM1) );

    if (value == 0)
    {
        chip_clk_bbpll_enable();
    }

    value = CHIP_SMU_PD_CORE_CLK_SRC_40M_26M;

#if defined (CONFIG_CPU_CLK_FREQ_DIV_0)
    value |= CHIP_SMU_PD_CORE_CLK_DIV0;
#elif defined (CONFIG_CPU_CLK_FREQ_DIV_2)
    value |= CHIP_SMU_PD_CORE_CLK_DIV2;
#elif defined (CONFIG_CPU_CLK_FREQ_DIV_4)
    value |= CHIP_SMU_PD_CORE_CLK_DIV4;
#elif defined (CONFIG_CPU_CLK_FREQ_DIV_8)
    value |= CHIP_SMU_PD_CORE_CLK_DIV8;
#endif

    WRITE_REG(CHIP_SMU_PD_CORE_CTRL, value);
    drv_pit_delay(100);
    WRITE_REG(CHIP_SMU_PD_CLK_DIV_EN , 0xFFFFFFFF);
}


#if 0
void chip_clk_config_retore(void)
{
    unsigned int value = CHIP_GET_REG(CHIP_SMU_PD_CLK_DIV_EN);

    CHIP_SET_REG(CHIP_SMU_PD_CLK_ENABLE_0, CHIP_GET_REG(CHIP_SMU_PD_CLK_ENABLE_0) \
        | (1<<CLK_DIG_PLL_FREF) | (1<<CLK_RF_FREF) | (1<<CLK_EFUSE) |(1<<CLK_RF_APB)  |(1<<CLK_IRAM0) |(1<<CLK_IRAM1) );

    if (value == 0)
    {
        chip_clk_bbpll_enable();
    }

#if defined (CONFIG_CPU_CLK_SRC_96M)
    
        CHIP_SET_REG(CHIP_SMU_PD_CLK_DIV_EN , \
            value |CHIP_SMU_PD_CLK_DIV5_EN |CHIP_SMU_PD_CLK_DIV3_EN |CHIP_SMU_PD_CLK_UART_TRNG_80M_EN);
        CHIP_SET_REG(CHIP_SMU_PD_UART_CLK_SEL , CHIP_SMU_PD_UART_CLK_SEL_80M);
        drv_pit_delay(160);
        value = CHIP_SMU_PD_CORE_CLK_SRC_96M ;
    
#elif defined (CONFIG_CPU_CLK_SRC_160M)
    
        CHIP_SET_REG(CHIP_SMU_PD_CLK_DIV_EN , \
            value |CHIP_SMU_PD_CLK_DIV2_EN |CHIP_SMU_PD_CLK_DIV3_EN |CHIP_SMU_PD_CLK_UART_TRNG_80M_EN );
        CHIP_SET_REG(CHIP_SMU_PD_UART_CLK_SEL , CHIP_SMU_PD_UART_CLK_SEL_80M);
        drv_pit_delay(10);
        value = CHIP_SMU_PD_CORE_CLK_SRC_160M;
    
#elif defined (CONFIG_CPU_CLK_SRC_240M)
    
        CHIP_SET_REG(CHIP_SMU_PD_CLK_DIV_EN , \
            value |CHIP_SMU_PD_CLK_DIV2_EN |CHIP_SMU_PD_CLK_DIV3_EN |CHIP_SMU_PD_CLK_UART_TRNG_80M_EN );
        CHIP_SET_REG(CHIP_SMU_PD_UART_CLK_SEL , CHIP_SMU_PD_UART_CLK_SEL_80M);
        drv_pit_delay(10);
        value = CHIP_SMU_PD_CORE_CLK_SRC_240M ;
    
#elif defined (CONFIG_CPU_CLK_SRC_40m)
    
        CHIP_SET_REG(CHIP_SMU_PD_CLK_DIV_EN , value |CHIP_SMU_PD_CLK_DIV2_EN |CHIP_SMU_PD_CLK_DIV3_EN );
        CHIP_SET_REG(CHIP_SMU_PD_UART_CLK_SEL , CHIP_SMU_PD_UART_CLK_SEL_40M_26M);
        drv_pit_delay(10);
        value = CHIP_SMU_PD_CORE_CLK_SRC_40M_26M ;
    
#endif

#if defined (CONFIG_CPU_CLK_FREQ_DIV_0)
    value |= CHIP_SMU_PD_CORE_CLK_DIV0;
#elif defined (CONFIG_CPU_CLK_FREQ_DIV_2)
    value |= CHIP_SMU_PD_CORE_CLK_DIV2;
#elif defined (CONFIG_CPU_CLK_FREQ_DIV_4)
    value |= CHIP_SMU_PD_CORE_CLK_DIV4;
#elif defined (CONFIG_CPU_CLK_FREQ_DIV_8)
    value |= CHIP_SMU_PD_CORE_CLK_DIV8;
#endif

    CHIP_SET_REG(CHIP_SMU_PD_CORE_CTRL, value);
    //CHIP_SET_REG(CHIP_SMU_PD_SW_RESET, CHIP_GET_REG(CHIP_SMU_PD_SW_RESET) & ~(0x1<<15));
    //CHIP_SET_REG(CHIP_SMU_PD_UART_CLK_SEL, CHIP_SMU_PD_UART_CLK_SEL_80M);
    //CHIP_SET_REG(CHIP_SMU_PD_SW_RESET, CHIP_GET_REG(CHIP_SMU_PD_SW_RESET) | (0x1<<15));
}
#endif
