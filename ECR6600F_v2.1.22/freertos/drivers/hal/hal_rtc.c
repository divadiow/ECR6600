#include "hal_rtc.h"
#include "rtc.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cli.h"
#include "oshal.h"
#ifdef CONFIG_SNTP
#include "sntp_tr.h"
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define  IS_LEAP_YEAR(year) (year%400==0 || (year%4==0&&year%100!=0))
#define  SEC_NUM_DAY		(24*ONE_HOUR2SEC)
#define  DAY_NUM_YEAR		(31+28+31+30+31+30+31+31+30+31+30+31)
#define  SEC_NUM_YEAR		(DAY_NUM_YEAR*SEC_NUM_DAY)
#define  DAY_NUM_LEAP_YEAR	(DAY_NUM_YEAR+1)
#define  SEC_NUM_LEAR_YEAR	(DAY_NUM_LEAP_YEAR*SEC_NUM_DAY)
#define	FEBRUARY_OF_LEAPEYEAR	29
const char month_days[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char *week[7] = {"Sun", "Mon", "Tues", "Wed", "Thur", "Fri", "Sat"};
DATE_RTC g_date;

/**    @brief		set hal time function .利用基姆拉尔森计算日期公式  w=(d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)
*	   @param[in]	year    current year
*	   @param[in]	month	current month
*	   @param[in]	day    	current day
*	   @return  	weekday	0-6: Sun Mon tues ... Sat
*/
unsigned char rtc_get_weekday(int year, int month, int day)
{
	int weekday = -1;
	if (1 == month || 2 == month)
	{
		month += MONTON_MAX;
		year--;
	}
	weekday = (day + 1 + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % ONE_WEEK2DAYS;
	
	return weekday;
}

/**    @brief		days interrupt callback function .
*	   @return  	
*/
void hal_rtc_callback()
{
	unsigned char daysofmonth;
	struct rtc_time tm;
	g_date.day += 1;
	if (!(IS_LEAP_YEAR(g_date.year) && (g_date.month == 2)))
	{
		daysofmonth = month_days[g_date.month];
	}
	else
	{
		daysofmonth = FEBRUARY_OF_LEAPEYEAR;
	}

	if (g_date.day > daysofmonth)
	{
		g_date.day = 1;
		g_date.month++;

		if (g_date.month > MONTON_MAX)
		{
			g_date.month = 1;
			g_date.year++;
		}
	}
	g_date.wday = rtc_get_weekday(g_date.year,g_date.month,g_date.day);
	drv_rtc_get_time(&tm);
	
	os_printf(LM_OS, LL_INFO, "YYYY-MM-DD HH:mm:ss = %4d-%2d-%2d %2d:%2d:%2d week=%s", g_date.year,g_date.month,g_date.day,tm.hour,tm.min,tm.sec,week[g_date.wday]);
}

/**    @brief		hal rtc init function .
*	   @return  	0-- hal rtc init succeed
*/
int hal_rtc_init()
{
	g_date.year = ORIGIN_YEAR;
	g_date.month= ORIGIN_MONTH;
	g_date.day 	= ORIGIN_DAY;
	g_date.hour = 0;
	g_date.min 	= 0;
	g_date.sec 	= 0;
	g_date.wday	= ORIGIN_WEEK;
	drv_rtc_init();
	drv_rtc_isr_register(DRV_RTC_DAY, hal_rtc_callback);
	return 0;
}

/**    @brief		set hal time function .
*	   @param[in]	rtc_time    Total seconds since January 1, 1970
*	   @return  	0-- set hal rtc time succeed, -1--set time failed
*/
int hal_rtc_set_time(int rtc_time) //sec
{
    struct rtc_time rt = {0};
    int res = 0;
    //unsigned int irq = 0;
    DATE_RTC date;
    res = hal_rtc_utctime_to_date(rtc_time,&date);
    if(res)
    {
        return -1;
    }
    //irq = system_irq_save();
    g_date = date;

    rt.hour = date.hour;
    rt.min  = date.min;
    rt.sec  = date.sec;
    res = drv_rtc_set_time(&rt);
    //system_irq_restore(irq);
    return res;
}

/**    @brief		get hal time function .
*	   @return  	Total seconds since January 1, 1970
*/
int hal_rtc_get_time()		//sec
{
	int total_sec = 0;
	DATE_RTC *cur_time = &g_date;
	struct rtc_time tm;
	drv_rtc_get_time(&tm);
	cur_time->hour	= tm.hour;
	cur_time->min	= tm.min;
	cur_time->sec	= tm.sec;
	
	hal_rtc_date_to_utctime(*cur_time,&total_sec);

	return total_sec;
}

/**    @brief		format convert. utctime to DATE_RTC struct convert
*	   @param[in]	utc_time    Total seconds since January 1, 1970
*	   @param[in]	date    	point of DATE_RTC struct data;
*	   @return  	0-- succeed 	-1-- failed
*/
int hal_rtc_utctime_to_date(int utc_time, DATE_RTC * date) //sec --struct
{
	if((utc_time < 0 )|| (date == NULL))
	{
		return -1;
	}
	date->wday = ORIGIN_WEEK+utc_time/SEC_NUM_DAY;
	date->wday %= ONE_WEEK2DAYS;

	for(date->year=ORIGIN_YEAR; ; date->year++)
	{
		if(IS_LEAP_YEAR(date->year))
		{
			if(utc_time < SEC_NUM_YEAR)
				break;				
			utc_time -= SEC_NUM_LEAR_YEAR;
		}
		else
		{
			if(IS_LEAP_YEAR(date->year+1) && utc_time<SEC_NUM_LEAR_YEAR)
				break;
			else if(utc_time<SEC_NUM_YEAR)
				break;	
			utc_time -= SEC_NUM_YEAR;	
		}
	}

	{
		unsigned int month_sec;
		for(date->month=1; date->month<=MONTON_MAX; date->month++)
		{
			month_sec = month_days[date->month]*SEC_NUM_DAY;
			if(date->month==LEAP_MONTH && IS_LEAP_YEAR(date->year))
				month_sec += SEC_NUM_DAY;
			if(utc_time < month_sec)
				break;
			utc_time -= month_sec;
		}
	}

	{
		unsigned char day_num;
		day_num = month_days[date->month];
		if(date->month==LEAP_MONTH && IS_LEAP_YEAR(date->year))
			day_num++;

		for(date->day=1; date->day<=day_num; date->day++)
		{
			if(utc_time < SEC_NUM_DAY)
				break;
			utc_time -= SEC_NUM_DAY;
		}
	}
	date->hour = utc_time/ONE_HOUR2SEC;
	date->min  = utc_time%ONE_HOUR2SEC/MIN_MAX;
	date->sec  = utc_time%ONE_HOUR2SEC%SEC_MAX;
	date->wday = rtc_get_weekday(date->year,date->month,date->day);
	return 0;
}

/**    @brief		format convert. utctime to DATE_RTC struct convert
*	   @param[in]	date    	DATE_RTC struct data;
*	   @param[out]	utc_time    the point of Total seconds since January 1, 1970
*	   @return  	0-- succeed 	-1-- failed
*/
int hal_rtc_date_to_utctime(DATE_RTC date, int * utc_time)	//struct --> sec
{
	int total_sec = 0;
	unsigned int num;
	DATE_RTC *cur_time =&date;
	if( cur_time->year	<ORIGIN_YEAR || \
		cur_time->month > MONTON_MAX || \
		cur_time->day	> DAYS_MAX || \
		cur_time->hour	> HOUR_MAX|| \
		cur_time->min	> MIN_MAX || \
		cur_time->sec	> SEC_MAX || \
		utc_time == NULL)
	{
		return -1;
	}
	for(num=ORIGIN_YEAR; num<cur_time->year; num++)
	{
		if(IS_LEAP_YEAR(num))
			total_sec += SEC_NUM_LEAR_YEAR;
		else
			total_sec += SEC_NUM_YEAR;
	}

	for(num=1; num<cur_time->month; num++)
	{
		total_sec += month_days[num]*SEC_NUM_DAY;
	}

	if(IS_LEAP_YEAR(cur_time->year) && cur_time->month>2)
		total_sec += SEC_NUM_DAY;

	total_sec += (cur_time->day-1)*SEC_NUM_DAY;
	total_sec += cur_time->hour*ONE_HOUR2SEC;
    total_sec += cur_time->min*MIN_TO_SEC;
    total_sec += cur_time->sec;
	*utc_time = total_sec;
	return 0;
}

#if 0
int cmd_hal_rtc_op(cmd_tbl_t *t, int argc, char *argv[])
{
	int res=0;
	int value = 0;
	struct rtc_time tm1;
	if(!strcmp(argv[1], "init"))
	{
		hal_rtc_init();
	}else if(!strcmp(argv[1], "get"))
	{
		res = hal_rtc_get_time();
		drv_rtc_get_time(&tm1);
		os_printf(LM_OS, LL_INFO, "\r\n[get] YYYY-MM-DD HH:mm:ss = %4d-%02d-%02d %02d:%02d:%02d week=%s\r\n", g_date.year,g_date.month,g_date.day,tm1.hour,tm1.min,tm1.sec,week[g_date.wday]);
		//os_printf(LM_OS, LL_INFO, "get time=%ld\r\n", res);
		
	}else if(!strcmp(argv[1], "set"))
	{
		value = atoi(argv[2]);
		hal_rtc_set_time(value);
		os_printf(LM_OS, LL_INFO, "set time=%ld\r\n", value);
	}else if(!strcmp(argv[1], "print"))
	{	
		drv_rtc_get_time(&tm1);
		os_printf(LM_OS, LL_INFO, "\r\n[get] YYYY-MM-DD HH:mm:ss = %4d-%02d-%02d %02d:%02d:%02d week=%s\r\n", g_date.year,g_date.month,g_date.day,tm1.hour,tm1.min,tm1.sec,week[g_date.wday]);
	}else if(!strcmp(argv[1], "st"))
	{	
		tm1.hour= atoi(argv[2]);							/** range 0-24 */
		tm1.min	= atoi(argv[3]);							/** range 0-59 */
		tm1.sec	= atoi(argv[4]);							/** range 0-59 */
		drv_rtc_set_time(&tm1);
		os_printf(LM_OS, LL_INFO, "\r\n[set] YYYY-MM-DD HH:mm:ss = %4d-%02d-%02d %02d:%02d:%02d week=%s\r\n", g_date.year,g_date.month,g_date.day,tm1.hour,tm1.min,tm1.sec,week[g_date.wday]);
	}else if(!strcmp(argv[1], "sty"))
	{	
		g_date.year	= atoi(argv[2]);							
		g_date.month= atoi(argv[3]);							
		g_date.day	= atoi(argv[4]);
		g_date.wday =rtc_get_weekday(g_date.year,g_date.month,g_date.day); 
		os_printf(LM_OS, LL_INFO, "\r\n[set] YYYY-MM-DD HH:mm:ss = %4d-%02d-%02d week=%s\r\n", g_date.year,g_date.month,g_date.day,week[g_date.wday]);
	}
	return CMD_RET_SUCCESS;
}

CLI_CMD(hal_rtc, cmd_hal_rtc_op, "init/get/set/print", "hal rtc");
#endif
