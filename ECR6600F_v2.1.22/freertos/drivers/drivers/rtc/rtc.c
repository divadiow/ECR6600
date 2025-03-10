/**
*@file	   rtc.c
*@brief    This part of program is rtc driver 
*@author   Houjie
*@data     2021.2.8
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#include "rtc.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cli.h"
#include "oshal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "arch_irq.h"
#include "efuse.h"
#include "oshal.h"

#define RTC_WRITE_DONE_NEW		1

#ifdef CONFIG_PSM_SURPORT
#include "psm_wifi.h"	
#endif
//#define CONFIG_EXT_XTAL 1		//open external xtal
#ifdef CONFIG_EXT_XTAL
#include "aon_reg.h"
#include "pmu_reg.h"
#include "chip_pinmux.h"	
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

//#ifndef REG32
//#define REG32(reg) (  *( (volatile unsigned int *) (reg) ) )
//#endif
//
//#ifndef WRITE_REG
//#define WRITE_REG(offset,value) (*(volatile unsigned int *)(offset) = (unsigned int)(value));
//#endif

#ifndef PCU_INT_ENA_CTRL_REG
#define PCU_INT_ENA_CTRL_REG	(MEM_BASE_PCU + 0x4c)
#endif

#define PCU_INT_CLEAR_REG		(MEM_BASE_PCU + 0x50)
#define PCU_INT_STATUS_REG		(MEM_BASE_PCU + 0x54)

#define PCU_RTC_INT_TO_CPU_ENA	0x18
#define PCU_RTC_INT_CLEAR				(0x1<<0)

/** RTC Counter Register  */
#define RTC_CNT_SET_SEC(x)			((x)<<0)
#define RTC_CNT_SET_MIN(x)			((x)<<6)
#define RTC_CNT_SET_HOUR(x)			((x)<<12)
#define RTC_CNT_SET_DAY(x)			((x)<<17)
	
#define RTC_CNT_GET_SEC(x)			(((x)>>0) & 0x3F)
#define RTC_CNT_GET_MIN(x)			(((x)>>6) & 0x3F)
#define RTC_CNT_GET_HOUR(x)			(((x)>>12) & 0x1F)
#define RTC_CNT_GET_DAY(x)			(((x)>>17) & 0x7FFF)

/** RTC Alarm Register   */
#define RTC_ALARM_SET_CNT32K(x)			((x)<<0)
#define RTC_ALARM_SET_SEC(x)			((x)<<15)
#define RTC_ALARM_SET_MIN(x)			((x)<<21)
#define RTC_ALARM_SET_HOUR(x)			((x)<<27)
	
#define RTC_ALARM_GET_CNT32K(x)		((x) & 0x7fff)
#define RTC_ALARM_GET_SEC(x)		(((x)& 0x1f8000) >> 15 )
#define RTC_ALARM_GET_MIN(x)		(((x) & 0x7e00000) >> 21)
#define RTC_ALARM_GET_HOUR(x)		(((x) & 0xf8000000)>> 27)

/** RTC Control Register  */
#define RTC_CTRL_EN(x)				((x)<<0)
#define RTC_CTRL_WAKEUP(x)			((x)<<1)
#define RTC_CTRL_INT(x)				((x)<<2)
#define RTC_CTRL_DAY(x)				((x)<<3)
#define RTC_CTRL_HOUR(x)			((x)<<4)
#define RTC_CTRL_MIN(x)				((x)<<5)
#define RTC_CTRL_SEC(x)				((x)<<6)
#define RTC_CTRL_HSEC(x)			((x)<<7)
/** RTC Control Register control bit  */
#define DRV_RTC_EN			(1<<0)
#define DRV_RTC_INT_ALARM	(1<<1)
//reserve
#define DRV_RTC_INT_DAY		(1<<3)
#define DRV_RTC_INT_HOUR	(1<<4)
#define DRV_RTC_INT_MIN		(1<<5)
#define DRV_RTC_INT_SEC		(1<<6)
//#define DRV_RTC_INT_HSEC	(1<<7)	

/** RTC Status Register  */
#define RTC_STATUS_GET_INT(x)		(((x)>>2)&1)
#define RTC_STATUS_GET_DAY(x)		(((x)>>3)&1)
#define RTC_STATUS_GET_HOUR(x)	(((x)>>4)&1)
#define RTC_STATUS_GET_MIN(x)		(((x)>>5)&1)
#define RTC_STATUS_GET_SEC(x)		(((x)>>6)&1)
#define RTC_STATUS_GET_HSEC(x)		(((x)>>7)&1)
#define RTC_STATUS_GET_WD(x)		(((x)>>14)&1)

//32K RTC frequence parameter rc_captrim address
#define RC_CAPTRIM_ADDR		0x201004		//todo :MODIFY   bit 0 -10
#define SYSTEM_RTC_CAL_CONF	0x202044		//base addr
#define SYSTEM_RTC_CNT		0x202048		//base addr
#define CAL_RTC_CNT			100				//when generate 100 rtc,cal rtc 
#define SYSTEM_CLOCK		40000000		//system clock frequece  40Mhz
#define ALARM_CLOCK			32768			//32k rtc cnt per sec
// 100 rtc,  SYSTEM_CLOCK couter num
#define THEORY_OSC_CNT				(SYSTEM_CLOCK*CAL_RTC_CNT/ALARM_CLOCK) // 122070

/**
 * @brief RTC register map.
 */
typedef struct _T_RTC_REG_MAP
{
	volatile unsigned int IdRev;			/* 0x00  -- ID and Revision Register */
	volatile unsigned int Cnt;				/* 0x04  -- number of cnt 32k counter .format: hour:min:sec:32k*/
	volatile unsigned int Resv0[2];			/* 0x08 ~ 0x0C  --reserved register */
	volatile unsigned int Cntr;				/* 0x10  -- Counter Register .format: day:hour:min:sec*/
	volatile unsigned int Alarm;			/* 0x14  -- Alarm Register 	 .format: hour:min:sec:32k*/
	volatile unsigned int Ctrl;				/* 0x18  -- Control Register */
	volatile unsigned int St;				/* 0x1C  -- Status Register */
	volatile unsigned int Trim;				/* 0x20  -- Digital Trimming Register */
} T_RTC_REG_MAP;

/**
 * @brief RTC device.
 */
typedef struct _DRV_RTC_DEV
{
	void_fn cb[DRV_RTC_ISR_MAX];
}DRV_RTC_DEV;

static T_RTC_REG_MAP * rtc_reg_base = (T_RTC_REG_MAP *)MEM_BASE_RTC;

static DRV_RTC_DEV rtc_dev;

/**    @brief		wait rtc ready to write data
*	   @return  	none
*/
void rtc_ready_to_write()
{
	unsigned int wd = 0 ;
	while(wd == 0)
	{
		wd = RTC_STATUS_GET_WD(rtc_reg_base->St);
	}
}

/**    @brief		Get rtc number of cnt 32k counter(rtc data format:hour|min|sec|cnt_32K)
*	   @return  	the rtc number of cnt 32k counter 
*/
unsigned int drv_rtc_get_32K_cnt(void)
{
	unsigned int rtcMsCnt = rtc_reg_base->Cnt;
	while(1)
	{
		if((rtcMsCnt & 0x7fff) != 0)
                   break;

		rtcMsCnt = rtc_reg_base->Cnt;
	}
	return rtcMsCnt;
}

/**    @brief		clear pcu interrupt of alarm 
*	   @return  	none
*/
void rtc_clear_pcu_int(void)
{
	while(1){
		unsigned int status=READ_REG(PCU_INT_STATUS_REG);
		if(status==0){
			//write 1 to set 0
			if(READ_REG(PCU_INT_CLEAR_REG))WRITE_REG(PCU_INT_CLEAR_REG,PCU_RTC_INT_CLEAR);
			break;
		}
		WRITE_REG(PCU_INT_CLEAR_REG,PCU_RTC_INT_CLEAR);//clear all interrupt
	}
}

/**    @brief		rtc interrupt except alarm.
*	   @return  	none
*/
static void drv_rtc_isr(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
	unsigned int rtcstatus = rtc_reg_base->St;
	DRV_RTC_DEV *p_rtc_dev =&rtc_dev;
	int i = 0;
	for(i=0;i<DRV_RTC_ISR_MAX;i++)
	{
		if(((rtcstatus >>i) & 0x1) && (p_rtc_dev->cb[i] != NULL))
		{
			p_rtc_dev->cb[i]();
			//os_printf(LM_OS, LL_INFO, "rtc-[%d]-isr comes\r\n",i);
		}
	}
	//rtc_ready_to_write();
	rtc_reg_base->St = rtcstatus;

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}

/**    @brief		rtc alarm interrupt.
*	   @return  	none
*/
static void drv_rtc_alarm(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
	DRV_RTC_DEV *p_rtc_dev =&rtc_dev;
	arch_irq_mask(VECTOR_NUM_PCU);
	//os_printf(LM_OS, LL_INFO, "pcu\r\n");
	if(p_rtc_dev->cb[DRV_RTC_ALARM] !=NULL)
	{
		p_rtc_dev->cb[DRV_RTC_ALARM]();
	}
	rtc_reg_base->St &= ~DRV_RTC_INT_ALARM;
	arch_irq_clean(VECTOR_NUM_PCU);
	rtc_clear_pcu_int();
	arch_irq_unmask(VECTOR_NUM_PCU);

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}

/**    @brief		initial rtc .
*	   @return  	0--initial succeed, -1--initial failed
*/
int drv_rtc_init()
{
	unsigned int flags;
	//DRV_RTC_DEV *p_rtc_dev =&rtc_dev;
	#ifdef CONFIG_EXT_XTAL
	BIT_SET(AON_PAD_MODE_REG,5);	//pad mode :io 5\6 gpio mode 
	BIT_SET(AON_PAD_MODE_REG,6);
	BIT_CLR(AON_PAD_IE_REG,5);	//input enable: io 5\6 disable
	BIT_CLR(AON_PAD_IE_REG,6);
	PIN_FUNC_SET(IO_MUX_GPIO5, FUNC_GPIO5_32K_I);
	PIN_FUNC_SET(IO_MUX_GPIO6, FUNC_GPIO5_32K_O);
	#ifdef CONFIG_CLI_UART_0
	PIN_FUNC_SET(IO_MUX_GPIO22, FUNC_GPIO22_UART0_TXD);
	PIN_FUNC_SET(IO_MUX_GPIO21, FUNC_GPIO21_UART0_RXD);
	#endif
	WRITE_REG(AON_PAD_PE_REG, (READ_REG(AON_PAD_PE_REG) &~ (0x3<<5)));//25:0 map to pullup gpio5~13 14~15 18 pullup
	WRITE_REG(AON_PAD_PS_REG, (READ_REG(AON_PAD_PS_REG) &~ (0x3<<5)));//25:0 map to pulldown gpio0~4 13 16~17 20~25 pulldown

	WRITE_REG(PCU_AO_CTRL_REG1, 0x3 << 26 | 0x1 << 21);	//enable xtal
	WRITE_REG(AON_32KCLK_SEL_REG, 0x1);//switch to  xtal
	#endif

#if RTC_WRITE_DONE_NEW
	while(1)
	{
		flags = system_irq_save();
		if (RTC_STATUS_GET_WD(rtc_reg_base->St))
		{
			rtc_reg_base->Ctrl |= DRV_RTC_EN;	
			system_irq_restore(flags);
			break;
		}
		else
		{
			system_irq_restore(flags);
		}	
	}
#else
	flags = system_irq_save();
	rtc_ready_to_write();
	rtc_reg_base->Ctrl |= DRV_RTC_EN;	
	system_irq_restore(flags);
#endif
	return 0;
}

/**    @brief		register interrupt .
*	   @param[in]	type    Rtc interrupt type 
*	   @param[in]	cb    	isr callback function 
*	   @return  	0--Register succeed, -1--Register failed
*/
int drv_rtc_isr_register(RTC_TYPE type,void_fn cb)
{
	unsigned int flags;
	DRV_RTC_DEV *p_rtc_dev =&rtc_dev;

	if((NULL == cb) || (type >= DRV_RTC_ISR_MAX))
	{
		return -1;
	}

#if RTC_WRITE_DONE_NEW
	if(DRV_RTC_ALARM == type)
	{
		flags = system_irq_save();
		p_rtc_dev->cb[type] = cb;
		arch_irq_register(VECTOR_NUM_PCU, drv_rtc_alarm);
		arch_irq_clean(VECTOR_NUM_PCU);
		arch_irq_unmask(VECTOR_NUM_PCU);
		WRITE_REG(PCU_INT_ENA_CTRL_REG, READ_REG(PCU_INT_ENA_CTRL_REG)|PCU_RTC_INT_TO_CPU_ENA);	//umask pcu isr
		system_irq_restore(flags);
	}
	else if(DRV_RTC_NORMAL != type)
	{
		while(1)
		{
			flags = system_irq_save();
			if (RTC_STATUS_GET_WD(rtc_reg_base->St))
			{
				rtc_reg_base->Ctrl |= (1<<type);		
				p_rtc_dev->cb[type] = cb;
				arch_irq_register(VECTOR_NUM_RTC, drv_rtc_isr);
				arch_irq_clean(VECTOR_NUM_RTC);
				arch_irq_unmask(VECTOR_NUM_RTC);
				system_irq_restore(flags);
				break;
			}
			else
			{
				system_irq_restore(flags);
			}	
		}
	}
	else
	{
		return -1;
	}
#else

	flags = system_irq_save();
	p_rtc_dev->cb[type] = cb;
	if(DRV_RTC_ALARM == type)
	{
		arch_irq_register(VECTOR_NUM_PCU, drv_rtc_alarm);
		arch_irq_clean(VECTOR_NUM_PCU);
		arch_irq_unmask(VECTOR_NUM_PCU);
		WRITE_REG(PCU_INT_ENA_CTRL_REG, READ_REG(PCU_INT_ENA_CTRL_REG)|PCU_RTC_INT_TO_CPU_ENA);	//umask pcu isr
	}
	else if(DRV_RTC_NORMAL != type)
	{
		arch_irq_register(VECTOR_NUM_RTC, drv_rtc_isr);
		arch_irq_clean(VECTOR_NUM_RTC);
		arch_irq_unmask(VECTOR_NUM_RTC);
		rtc_ready_to_write();
		rtc_reg_base->Ctrl |= (1<<type);		
	}
	system_irq_restore(flags);
#endif
	return 0;
}

/**    @brief		Interrupt unregister.
*	   @param[in]	type    Rtc interrupt type 
*	   @return  	0--Unregister succeed, -1--Unregister failed
*/
int drv_rtc_isr_unregister(RTC_TYPE type)
{
	unsigned int flags;
	DRV_RTC_DEV *p_rtc_dev =&rtc_dev;

	if( type >= DRV_RTC_ISR_MAX)
	{
		return -1;
	}

#if RTC_WRITE_DONE_NEW

	while(1)
	{
		flags = system_irq_save();
		if (RTC_STATUS_GET_WD(rtc_reg_base->St))
		{
			rtc_reg_base->Ctrl &= ~(1<<type);
			if (DRV_RTC_ALARM == type)
			{
				WRITE_REG(PCU_INT_ENA_CTRL_REG, READ_REG(PCU_INT_ENA_CTRL_REG)&~PCU_RTC_INT_TO_CPU_ENA);
				//mask pcu isr
				arch_irq_mask(VECTOR_NUM_PCU);
				arch_irq_clean(VECTOR_NUM_PCU); 	
			}
			p_rtc_dev->cb[type] = NULL;
			system_irq_restore(flags);
			break;
		}
		else
		{
			system_irq_restore(flags);
		}	
	}

#else
	flags = system_irq_save();
	rtc_ready_to_write();
	rtc_reg_base->Ctrl &= ~(1<<type);
	if(DRV_RTC_ALARM == type)
	{
		WRITE_REG(PCU_INT_ENA_CTRL_REG, READ_REG(PCU_INT_ENA_CTRL_REG)&~PCU_RTC_INT_TO_CPU_ENA);	//mask pcu isr
		arch_irq_mask(VECTOR_NUM_PCU);
		arch_irq_clean(VECTOR_NUM_PCU);		
	}
	p_rtc_dev->cb[type] = NULL;
	system_irq_restore(flags);
#endif

	return 0;
}

/**    @brief		Get rtc time.
*	   @param[in]	*time    rtc_time structure pointer 
*	   @return  	0--Unregister succeed, -1--Unregister failed
*/
int drv_rtc_get_time(struct rtc_time *time)
{
	unsigned int value = 0;
	//unsigned int flags;
	if(time == NULL)
	{
		return -1;
	}
	//flags = arch_irq_save();
	value = rtc_reg_base->Cntr;
	time->cnt_32k = RTC_ALARM_GET_CNT32K(drv_rtc_get_32K_cnt());
	//arch_irq_restore(flags);
	time->sec  = RTC_CNT_GET_SEC(value);
	time->min  = RTC_CNT_GET_MIN(value);
	time->hour = RTC_CNT_GET_HOUR(value);
	time->day 	= RTC_CNT_GET_DAY(value);
	
	return 0;
}

/**    @brief		Set rtc time.note:cnt_32k is readonly.when time set,cnt_32k is automatically set to zero
*	   @param[in]	*time    rtc_time structure pointer 
*	   @return  	0--Set time succeed, -1--Set time failed
*/
int drv_rtc_set_time(struct rtc_time *time)
{
	unsigned int set_time;
	
	if (!time)
	{
		return -1;
	}

	set_time =    RTC_CNT_SET_SEC(time->sec)
				| RTC_CNT_SET_MIN(time->min)
				| RTC_CNT_SET_HOUR(time->hour)
				| RTC_CNT_SET_DAY(time->day);

#if RTC_WRITE_DONE_NEW
	while(1)
	{
		unsigned int flags = system_irq_save();
		if (RTC_STATUS_GET_WD(rtc_reg_base->St))
		{
			rtc_reg_base->Cntr = set_time;
			system_irq_restore(flags);
			break;
		}
		else
		{
			system_irq_restore(flags);
		}	
	}
#else
	rtc_reg_base->Cntr = set_time;
    rtc_ready_to_write();
#endif

	#ifdef CONFIG_PSM_SURPORT
	unsigned int old_rtc = drv_rtc_get_32K_cnt();
    /*if(g_sntp_update_flag && (psm_check_single_device_idle(PSM_DEVICE_WIFI_STA)))
    {
        psm_rtc_bias_timer_init(1);
    }*/
    if(psm_sleep_next_time_point_op(false,0))
    {
		rtc_ready_to_write();
    
		if(old_rtc > drv_rtc_get_32K_cnt())
		{
			psm_sleep_next_time_point_op(true,drv_rtc_32k_format_sub(psm_sleep_next_time_point_op(false,0),drv_rtc_get_interval_cnt(drv_rtc_get_32K_cnt(),old_rtc)));
		}
		else
		{
			psm_sleep_next_time_point_op(true,drv_rtc_32k_format_add(psm_sleep_next_time_point_op(false,0),drv_rtc_get_interval_cnt(drv_rtc_get_32K_cnt(),old_rtc)));
		}
	}
	psm_is_rtc_update_op(true, true);
	#endif
	return 0;
}

/**    @brief		Set rtc alarm.note:Only hour,minutes,seconds and cnt_32K are supported for configuring alarm clocks,day is unsupported
*	   @param[in]	*time    rtc_time structure pointer 
*	   @return  	0--Set alarm succeed, -1--Set alarm failed
*/
int drv_rtc_set_alarm(struct rtc_time *time)
{
	unsigned int wakeup_time;

	if (!time)
	{
		return -1;
	}

	wakeup_time = RTC_ALARM_SET_SEC(time->sec)
				| RTC_ALARM_SET_MIN(time->min)
				| RTC_ALARM_SET_HOUR(time->hour)
				| RTC_ALARM_SET_CNT32K(time->cnt_32k);

#if RTC_WRITE_DONE_NEW
	while(1)
	{
		unsigned int flags = system_irq_save();
		if (RTC_STATUS_GET_WD(rtc_reg_base->St))
		{
			rtc_reg_base->Alarm = wakeup_time;
			rtc_reg_base->Ctrl |= RTC_CTRL_WAKEUP(1);
			system_irq_restore(flags);
			break;
		}
		else
		{
			system_irq_restore(flags);
		}	
	}
#else
	rtc_reg_base->Alarm = wakeup_time;
	rtc_reg_base->Ctrl |= RTC_CTRL_WAKEUP(1);
#endif

	return 0;
}

/**    @brief		Get rtc alarm time. note:get alarm clock about hour,minutes,seconds and cnt_32K.
*	   @param[in]	*time    rtc_time structure pointer 
*	   @return  	0--Get alarm time succeed, -1--Get alarm time failed
*/
int drv_rtc_get_alarm(struct rtc_time *time)
{
	unsigned int value = 0;
	if(time == NULL)
	{
		return -1;
	}
	value = rtc_reg_base->Alarm;
	time->sec  = RTC_ALARM_GET_SEC(value);
	time->min  = RTC_ALARM_GET_MIN(value);
	time->hour = RTC_ALARM_GET_HOUR(value);
	time->cnt_32k = RTC_ALARM_GET_CNT32K(value);
	return 0;
}

/**    @brief		Get rtc interval in cnt, now - prev
*	   @param[in]	pre_rtc    previous data in RTC format
*	   @param[in]	now_rtc    now data in RTC format
*	   @return  	toltal cnt of interval (unit: 1cnt) note:1cnt = 1/32768s.
*/
unsigned int drv_rtc_get_interval_cnt(unsigned       int pre_rtc,unsigned int now_rtc)
{
	struct rtc_time tm;
	if (RTC_ALARM_GET_HOUR(pre_rtc) > RTC_ALARM_GET_HOUR(now_rtc))
	{
		tm.hour = DAY_TO_HOUR - RTC_ALARM_GET_HOUR(pre_rtc) + RTC_ALARM_GET_HOUR(now_rtc);
	}	
	else
	{
		tm.hour = RTC_ALARM_GET_HOUR(now_rtc) - RTC_ALARM_GET_HOUR(pre_rtc);
	}	
	
	tm.min = tm.hour * HOUR_TO_MIN + RTC_ALARM_GET_MIN(now_rtc) - RTC_ALARM_GET_MIN(pre_rtc);
	tm.sec = tm.min * MIN_TO_SEC + RTC_ALARM_GET_SEC(now_rtc) - RTC_ALARM_GET_SEC(pre_rtc);
	tm.cnt_32k = tm.sec * SEC_TO_32K + RTC_ALARM_GET_CNT32K(now_rtc) - RTC_ALARM_GET_CNT32K(pre_rtc);
	return tm.cnt_32k;
}

/**    @brief		Stop rtc 
*	   @return  	0--succeed
*/
int drv_rtc_stop(void)
{
#if RTC_WRITE_DONE_NEW
	while(1)
	{
		unsigned int flags = system_irq_save();
		if (RTC_STATUS_GET_WD(rtc_reg_base->St))
		{
			rtc_reg_base->Ctrl = 0;
			system_irq_restore(flags);
			break;
		}
		else
		{
			system_irq_restore(flags);
		}	
	}
#else
	unsigned int flags;
	flags = arch_irq_save();
	rtc_ready_to_write();
	rtc_reg_base->Ctrl = 0;
	arch_irq_restore(flags);
#endif
	return 0;
}

/**    @brief		Set the relative time of the alarm
*	   @param[in]	value    relative value of alarm time
*	   @return  	the alarm time in cnt (unit: 1cnt) note:1cnt = 1/32768s.
*/
unsigned int drv_rtc_set_alarm_relative(unsigned int value)
{
	struct rtc_time wakeup_time={0};
	unsigned int rtc_cnt=0,rtc_config=0;
	int rtc_32k_cnt=0;
	value %= ((unsigned int)DAY_TO_HOUR * HOUR_TO_MIN * MIN_TO_SEC * SEC_TO_32K); // value should less than one day;
	//struct rtc_time tm = drv_rtc_read();
	//os_printf(LM_OS, LL_INFO, "read time day:hour:min:sec:cnt %d:%d:%d:%d:%d\n",tm.day,tm.hour,tm.min,tm.sec,tm.cnt_32k);
	//read hour min sec from rtc_cnt_reg  and read ms from rtc_32k_cnt
	rtc_32k_cnt = drv_rtc_get_32K_cnt();
	//set 32k alarm count
	wakeup_time.cnt_32k = value + RTC_ALARM_GET_CNT32K(rtc_32k_cnt);
	wakeup_time.sec 	= wakeup_time.cnt_32k / SEC_TO_32K;
	wakeup_time.cnt_32k = wakeup_time.cnt_32k % SEC_TO_32K;

	//alarm  sec
	wakeup_time.sec += RTC_ALARM_GET_SEC(rtc_32k_cnt);
	wakeup_time.min = wakeup_time.sec / MIN_TO_SEC;
	wakeup_time.sec = wakeup_time.sec % MIN_TO_SEC;

	//alarm min
	wakeup_time.min += RTC_ALARM_GET_MIN(rtc_32k_cnt);
	wakeup_time.hour = wakeup_time.min / HOUR_TO_MIN;
	wakeup_time.min	 = wakeup_time.min % HOUR_TO_MIN;

	//alarm hour
	wakeup_time.hour = (RTC_ALARM_GET_HOUR(rtc_32k_cnt) + wakeup_time.hour) % DAY_TO_HOUR;

	rtc_cnt = RTC_ALARM_SET_HOUR(wakeup_time.hour) |
			  RTC_ALARM_SET_MIN(wakeup_time.min) |
			  RTC_ALARM_SET_SEC(wakeup_time.sec) |
			  RTC_ALARM_SET_CNT32K(wakeup_time.cnt_32k);


#if RTC_WRITE_DONE_NEW
	  while(1)
	  {
		  unsigned int flags = system_irq_save();
		  if (RTC_STATUS_GET_WD(rtc_reg_base->St))
		  {
			  rtc_reg_base->Alarm = rtc_cnt;
			  rtc_config = rtc_reg_base->Ctrl;
			  rtc_reg_base->Ctrl = rtc_config |DRV_RTC_INT_ALARM;
			  system_irq_restore(flags);
			  break;
		  }
		  else
		  {
			  system_irq_restore(flags);
		  }   
	  }
#else
	//os_printf(LM_OS, LL_INFO, "set wakeup -h:m:s:ms %d:%d:%d:%d\n",wakeup_time.hour,wakeup_time.min,wakeup_time.sec,wakeup_time.cnt_32k);

	//system_printf("%d:%d:%d.%d, rtc_cnt:%x ,values:%d\n", alarm_hour, alarm_min, alarm_sec, alarm_32k, rtc_cnt, value);
	//if((rtc_cnt & 0x7fff) == 0x0) //should be used in get alarm
	//{
	//	rtc_cnt += 1;
	//}
	//Wait until the WriteDone field of the Status Register equals 1.
	//rtc_ready_to_write();
	//Set the Alarm Register to the time you want to wake up the system.
	rtc_reg_base->Alarm = rtc_cnt;
	rtc_config = rtc_reg_base->Ctrl;
	//rtc_ready_to_write();
	//Enable RTC and alarm wakeup: set the Control Register to 0x3 or 0x5.
	rtc_reg_base->Ctrl = rtc_config |DRV_RTC_INT_ALARM;
#endif
	return rtc_cnt;
}


/**    @brief		cal the 32k format data add normal cnt;
*	   @param[in]	cnt		 cnt 32k .format: hour:min:sec:32k
*	   @param[in]	value    total cnt ;      note:1cnt = 1/32768s.
*	   @return  	cal the new cnt format in cnt (format: hour:min:sec:32k)
*/
unsigned int drv_rtc_32k_format_add(unsigned int cnt,unsigned int value)
{
	struct rtc_time rt_time={0};
	unsigned int rtc_cnt=0;
	//32k count
	value %= ((unsigned int)DAY_TO_HOUR * HOUR_TO_MIN * MIN_TO_SEC * SEC_TO_32K); // value should less than one day;
	rt_time.cnt_32k = value + RTC_ALARM_GET_CNT32K(cnt);
	rt_time.sec 	= rt_time.cnt_32k / SEC_TO_32K;
	rt_time.cnt_32k = rt_time.cnt_32k % SEC_TO_32K;

	// sec
	rt_time.sec += RTC_ALARM_GET_SEC(cnt);
	rt_time.min = rt_time.sec / MIN_TO_SEC;
	rt_time.sec = rt_time.sec % MIN_TO_SEC;

	//min
	rt_time.min += RTC_ALARM_GET_MIN(cnt);
	rt_time.hour = rt_time.min / HOUR_TO_MIN;
	rt_time.min	 = rt_time.min % HOUR_TO_MIN;

	//hour
	rt_time.hour = (RTC_ALARM_GET_HOUR(cnt) + rt_time.hour) % DAY_TO_HOUR;

	rtc_cnt = RTC_ALARM_SET_HOUR(rt_time.hour) |
			  RTC_ALARM_SET_MIN(rt_time.min) |
			  RTC_ALARM_SET_SEC(rt_time.sec) |
			  RTC_ALARM_SET_CNT32K(rt_time.cnt_32k);
	//os_printf(LM_OS, LL_INFO, "add hh:mm:ss:cnt = %d:%d:%d:%d\n", rt_time.hour,rt_time.min,rt_time.sec,rt_time.cnt_32k);
	return rtc_cnt;
}

/**    @brief		cal the 32k format data sub normal cnt;
*	   @param[in]	cnt		 cnt 32k .format: hour:min:sec:32k
*	   @param[in]	value    total cnt ;      note:1cnt = 1/32768s.
*	   @return  	cal the new cnt format in cnt (format: hour:min:sec:32k) 
*/
unsigned int drv_rtc_32k_format_sub(unsigned int cnt,unsigned int value)
{
	struct rtc_time rt_time={0};
	unsigned int rtc_cnt=0;
	
	rt_time.sec 	= value / SEC_TO_32K  ;
	rt_time.cnt_32k = value % SEC_TO_32K ;
	
	rt_time.hour	= rt_time.sec / (MIN_TO_SEC * HOUR_TO_MIN) % DAY_TO_HOUR;
	rt_time.min		= rt_time.sec % (MIN_TO_SEC * HOUR_TO_MIN) / MIN_TO_SEC;
	rt_time.sec 	= rt_time.sec % MIN_TO_SEC;
	//os_printf(LM_OS, LL_INFO, "%d %d %d %d \n",rt_time.hour,rt_time.min,rt_time.sec,rt_time.cnt_32k);
	
	//os_printf(LM_OS, LL_INFO, "org hh:mm:ss:cnt = %d:%d:%d:%d\n", rt_time.hour,rt_time.min,rt_time.sec,rt_time.cnt_32k);
	if(rt_time.cnt_32k > RTC_ALARM_GET_CNT32K(cnt))
	{
		rt_time.cnt_32k = SEC_TO_32K + RTC_ALARM_GET_CNT32K(cnt) - rt_time.cnt_32k ;
		if(rt_time.sec == 59)
		{
			rt_time.sec = 0;
			if(rt_time.min == 59)
			{
				rt_time.min = 0;
				if(rt_time.hour == 23)
				{
					rt_time.hour = 0;
				}
				else
				{
					rt_time.hour += 1;
				}
			}
			else 
			{
				rt_time.min += 1;
			}
		}
		else 
		{
			rt_time.sec += 1;
		}
	}
	else
	{
		rt_time.cnt_32k = RTC_ALARM_GET_CNT32K(cnt) - rt_time.cnt_32k;
	}
	if(RTC_ALARM_GET_SEC(cnt) >= rt_time.sec)
	{
		rt_time.sec = RTC_ALARM_GET_SEC(cnt) - rt_time.sec;
	}
	else
	{
		rt_time.sec = MIN_TO_SEC + RTC_ALARM_GET_SEC(cnt) - rt_time.sec;
		if(rt_time.min == 59)
		{
			rt_time.min = 0;
			if(rt_time.hour == 23)
			{
				rt_time.hour = 0;
			}
			else
			{
				rt_time.hour += 1;
			}
		}
		else
		{
			rt_time.min += 1;
		}

		
	}

	if(RTC_ALARM_GET_MIN(cnt) >= rt_time.min )
	{
		rt_time.min = RTC_ALARM_GET_MIN(cnt) - rt_time.min;
	}
	else
	{
		rt_time.min = HOUR_TO_MIN - rt_time.min + RTC_ALARM_GET_MIN(cnt);
		if(rt_time.hour == 23)
		{
			rt_time.hour = 0;
		}
		else
		{
			rt_time.hour += 1;
		}
	}

	if(RTC_ALARM_GET_HOUR(cnt) >= rt_time.hour)
	{
		rt_time.hour = RTC_ALARM_GET_HOUR(cnt) - rt_time.hour;
	}
	else
	{
		rt_time.hour = DAY_TO_HOUR + RTC_ALARM_GET_HOUR(cnt) - rt_time.hour;
	}
	rtc_cnt = RTC_ALARM_SET_HOUR(rt_time.hour) |
			  RTC_ALARM_SET_MIN(rt_time.min) |
			  RTC_ALARM_SET_SEC(rt_time.sec) |
			  RTC_ALARM_SET_CNT32K(rt_time.cnt_32k);
	//os_printf(LM_OS, LL_INFO, "sub hh:mm:ss:cnt = %d:%d:%d:%d\n", rt_time.hour,rt_time.min,rt_time.sec,rt_time.cnt_32k);
	return rtc_cnt;
}

/**    @brief		Get the interval time of the alarm in cnt
*	   @return  	the interval of alarm time in cnt (unit: 1cnt) note:1cnt = 1/32768s.
*/
int drv_rtc_get_alarm_cnt(void)
{
	unsigned int alarmReg = 0, currReg = 0;
	int   cnt = 0;
	//rtc_ready_to_write();
	alarmReg= rtc_reg_base->Alarm;
	currReg	= drv_rtc_get_32K_cnt();
	cnt 	= drv_rtc_get_interval_cnt(currReg, alarmReg);
	return   cnt;
}


/**    @brief		clean the interrupt of rtc 
*	   @return  	none
*/
void rtc_int_status_clean(void)
{
	//OUT32(RTC_STATUS_REG, IN32(RTC_STATUS_REG));	
	unsigned int rtcstatus = rtc_reg_base->St;
	//rtc_ready_to_write();
	rtc_reg_base->St = rtcstatus;
}

#define CNT_40M_FRO_100_32K_CYCLE  122070
#define CNT_26M_FRO_100_32K_CYCLE  79346

/**
 ****************************************************************************************
 * @brief Cal the diff of 100 rtc  clock and osc cnt. if rtc freq is fast ,the return value is <0
 *
 * @return the diff cnt
 ****************************************************************************************
 */
int cal_rtc_diff_cnt(void)
{
	unsigned int efuse_ctrl_value;
	int diff = 0;
	//utils_Printf("%x\r\n",IN32(RC_CAPTRIM_ADDR)&0x7ff);
	//configure :after 100 rtc clock ,rtc_cnt end_flag will be set 1;
	WRITE_REG(SYSTEM_RTC_CAL_CONF, CAL_RTC_CNT);
	// enable calculation  rtc 
	WRITE_REG(SYSTEM_RTC_CAL_CONF, READ_REG(SYSTEM_RTC_CAL_CONF) | (1 << 15));	
	//wait until end_flag equal 1
	//while(!(IN32(SYSTEM_RTC_CNT) & 0x1));
	while (!(READ_REG(SYSTEM_RTC_CNT) & 0x1))
	{
		/* code */
		vTaskDelay(3 / portTICK_PERIOD_MS);
	}
	
	//get systerm couter num
	int sys_couter_num = (int)(READ_REG(SYSTEM_RTC_CNT) >> 1);
	//utils_Printf("sys_couter_num=%d\n",sys_couter_num);
	drv_efuse_read(EFUSE_CTRL_OFFSET, &efuse_ctrl_value, 1);
	if((efuse_ctrl_value&EFUSE_CRYSTAL_40M))  // 1--40M, 0--26M :default 26M
	{
		diff =sys_couter_num - (int)CNT_40M_FRO_100_32K_CYCLE;
	}
	else{
		diff =sys_couter_num - (int)CNT_26M_FRO_100_32K_CYCLE;
	}
	//utils_Printf("diff=%d\n",diff);
	// disable calculation  rtc 
	WRITE_REG(SYSTEM_RTC_CAL_CONF, READ_REG(SYSTEM_RTC_CAL_CONF) & ~(1<<15));	
	return diff;
}


int calibration = 0;
extern void psm_phy_hw_set_channel_interface();
unsigned int sec_step_add_num = 0;
unsigned int sec_step_sub_num = 0;
void rtc_sec_isr(void)
{	
	unsigned int efuse_ctrl_value,cnt_for_100_32k_cycle;
	static int sec_cnt = 0;
	struct rtc_time time = {0};
	
	drv_efuse_read(EFUSE_CTRL_OFFSET, &efuse_ctrl_value, 1);
	if((efuse_ctrl_value&EFUSE_CRYSTAL_40M))  // 1--40M, 0--26M :default 26M
	{
		cnt_for_100_32k_cycle = CNT_40M_FRO_100_32K_CYCLE;
	}
	else
	{
		cnt_for_100_32k_cycle = CNT_26M_FRO_100_32K_CYCLE;
	}
	sec_cnt++;
#if defined CONFIG_PSM_SURPORT_close && !defined CONFIG_PSM_SUPER_LOWPOWER
	static unsigned int beacon_cnt_old = 0;
    extern unsigned char psm_get_wifi_next_status(void);
	if(((sec_cnt % 2) == 0)
		&&(psm_check_single_device_idle(PSM_DEVICE_WIFI_STA)
		&& psm_is_stat_sleep()
        && beacon_cnt_old != 0 
        && beacon_cnt_old == psm_cnt_rec_beacon_op(false, 0))
        && psm_get_wifi_next_status() == PSM_COMMON_ACTIVE)
	{
		psm_phy_hw_set_channel_interface();
		os_printf(LM_OS, LL_INFO, "%s ch_restore\n",__func__);
	}

	beacon_cnt_old = psm_cnt_rec_beacon_op(false, 0);
#elif defined CONFIG_PSM_SUPER_LOWPOWER
    unsigned int dhcp_wait_time = psm_dhcp_wait_time_op(false, 0);
    if(dhcp_wait_time > 0)
    {
    	os_printf(LM_OS, LL_INFO, "%s wait_time:%d\n",__func__,dhcp_wait_time);
        psm_dhcp_wait_time_op(true, dhcp_wait_time - 1);
    }
#endif
	int sec_step = (sec_cnt * calibration) / cnt_for_100_32k_cycle;
	if(ABS(sec_step) == 1){
        if (sec_step == 1) {
            sec_step_add_num++;
        }
        else {
            sec_step_sub_num++;
        }
		sec_cnt = 0;

		drv_rtc_get_time(&time);
		//os_printf(LM_OS, LL_INFO, "%s %d, step:%d\n", __func__, __LINE__, sec_step);
		time.sec += sec_step;

		if(time.sec < 0)
		{
			time.sec = 59;
			time.min -= 1;
			if(time.min < 0)
			{
				time.min = 59;
				time.hour -= 1;

				if(time.hour < 0){
					time.hour = 23;
				}
			}
		}else if(time.sec >= 60){
			time.sec = 0;
			time.min += 1;

			if(time.min >= 60)
			{
				time.min = 0;
				time.hour += 1;
				if(time.hour == 24)
					time.hour = 0;
			}
		}

		drv_rtc_set_time(&time);
		//os_printf(LM_OS, LL_INFO, "%s %d, calibration:%d\n", __func__, __LINE__, calibration);
	}

	if(READ_REG(SYSTEM_RTC_CAL_CONF) & (1 << 15)){
		if(!(READ_REG(SYSTEM_RTC_CNT) & 0x1))
			return;

		int sys_couter_num = (int)(READ_REG(SYSTEM_RTC_CNT) >> 1);
		int diff = sys_couter_num - (int)cnt_for_100_32k_cycle;
		calibration = (calibration + diff) / 2;
		WRITE_REG(SYSTEM_RTC_CAL_CONF, READ_REG(SYSTEM_RTC_CAL_CONF) & ~(1<<15));
	}else{
		WRITE_REG(SYSTEM_RTC_CAL_CONF, CAL_RTC_CNT);
		WRITE_REG(SYSTEM_RTC_CAL_CONF, READ_REG(SYSTEM_RTC_CAL_CONF) | (1 << 15));
	}
}

void rtc_hour_isr(void)
{
	os_printf(LM_OS, LL_INFO, "%d, %s\n", __LINE__, __func__);
}
volatile int rtc_task_handle;
void calculate_rtc_task()
{
	volatile unsigned int rc_captrim=0,reg_high=0,reg_final=0;	//rc_captrim value
	volatile int i = 9, diff_last = 0;
	//volatile int j=0;
	reg_high = READ_REG(RC_CAPTRIM_ADDR) & ~0x7ff;	//record other bit
	WRITE_REG(RC_CAPTRIM_ADDR,reg_high | 0x7ff);		// check bit 10 start with 0 or 1.
	//for(j=0;j<1000;j++);
	int direction = cal_rtc_diff_cnt();	//first check calibration start with 0 or 0x7ff
	diff_last = direction;

	if(direction > 0){	//is start with 0x7ff?
		rc_captrim = 0x7ff;
	}else{
		rc_captrim = 0x3ff;
	}

	//os_printf(LM_OS, LL_INFO, "diff=%x\t captrim=%d\t captrim=%x\n",direction,rc_captrim,rc_captrim);
	while(i >= 0)
	{	
		if(direction > 0){	//is start with 0x7ff?
			rc_captrim = rc_captrim & ~(1 << i);
		}else{
			rc_captrim = (rc_captrim | (1 << (i + 1))) & ~(1 << i);
		}
		WRITE_REG(RC_CAPTRIM_ADDR, reg_high|rc_captrim);//update captrim
		//for(j=0;j<1000;j++);
		//utils_Printf("[1]\t%d\t%x\r\n",rc_captrim,rc_captrim);
		direction = cal_rtc_diff_cnt();				//after update,get new diff value
		if(ABS(direction) < ABS(diff_last))
		{
			//os_printf(LM_OS, LL_INFO, "i=%d,diff=%d,diff_last=%d\n", i,diff,diff_last);
			reg_final = rc_captrim;
			diff_last = direction;
		}
		
		if(direction == 0) break;		
		//for(j=0;j<1000;j++);
		i--;
		//os_printf(LM_OS, LL_INFO, "direction=%d captrim=0x%x\n", direction, rc_captrim);
	}
	WRITE_REG(RC_CAPTRIM_ADDR,reg_high|reg_final);
	//diff_last =cal_rtc_diff_cnt();
	calibration = diff_last;
	drv_rtc_isr_register(DRV_RTC_SEC, rtc_sec_isr);
	//drv_rtc_isr_register(DRV_RTC_HOUR, rtc_hour_isr);
	//os_printf(LM_OS, LL_INFO, "rtc calculate ok!,captrim=%x\tdiff=%d\r",reg_final, diff_last);
	//os_task_delete(rtc_task_handle);
}
#if 0
int cmd_cal_op(cmd_tbl_t *t, int argc, char *argv[])
{
	volatile int res,value = 0,j;
	if(argc>1)
	{
		value = strtoull(argv[1], NULL, 0);
		OUT32(RC_CAPTRIM_ADDR,IN32(RC_CAPTRIM_ADDR)&~0x7ff | value);
		for(j=0;j<2000;j++);
	}
	os_printf(LM_OS, LL_INFO, "argc=%d\n", argc);
	res =cal_rtc_diff_cnt();	
	os_printf(LM_OS, LL_INFO, "rtc calculate ok!,value = %d\t0x%d\r\n",value,res);
	return CMD_RET_SUCCESS;
}
CLI_CMD(cal, cmd_cal_op, "calculate", "rtc calculate");
int cmd_rtc_add_sub(cmd_tbl_t *t, int argc, char *argv[])
{
	if(!strcmp(argv[1], "rtc"))
	{
		for(unsigned int i =0xffff9d29 ;i<0xffffffff ;i++)
		{	
			volatile unsigned int cnt	= drv_rtc_get_32K_cnt();
			//os_printf(LM_OS, LL_INFO, "get hh:mm:ss:cnt = %d:%d:%d:%d\n", RTC_ALARM_GET_HOUR(cnt),RTC_ALARM_GET_MIN(cnt) 
			//										 ,RTC_ALARM_GET_SEC(cnt),RTC_ALARM_GET_CNT32K(cnt));
			volatile unsigned int temp1 = drv_rtc_32k_format_add(cnt,i);
			volatile unsigned int temp2 = drv_rtc_32k_format_sub(temp1,i);
			if(temp2 != cnt)
			{
				os_printf(LM_OS, LL_INFO, "[%d] %d %d %d error \n",i,cnt,temp1,temp2);
			}
			if(i == 0xff || i == 0xffff || i == 0xffffff || i == 0xfffffff){os_printf(LM_OS, LL_INFO, "%x\n",i);}
		}
	}

}
CLI_CMD(rtc, cmd_rtc_add_sub, "test", "rtc test");
#endif
