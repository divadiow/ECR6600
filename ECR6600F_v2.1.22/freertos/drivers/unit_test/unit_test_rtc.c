#include "cli.h"
#include "hal_rtc.h"
#include "stdlib.h"
#include "oshal.h"
#define  IS_LEAP_YEAR(year) (year%400==0 || (year%4==0&&year%100!=0))
#define	FEBRUARY_OF_LEAPEYEAR	29
const char month_days2[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
extern unsigned char rtc_get_weekday(int year, int month, int day);
#define DIFF_SEC_1970_2036          ((unsigned int)2085978496L)
const char *weeken[7] = {"Sun", "Mon", "Tues", "Wed", "Thur", "Fri", "Sat"};
#ifdef CONFIG_SNTP
#include "sntp_tr.h"
#endif
static int utest_rtc_set_time(cmd_tbl_t *t, int argc, char *argv[])
{
	int time;

	if (argc >= 2)
	{
		time = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, err: no enough argt!\r\n");
		return 0;
	}
#ifdef CONFIG_SNTP
	float tz = get_timezone();
	int local_time = time + (int)(tz * ONE_HOUR2SEC);
	if (hal_rtc_set_time(local_time) == 0)
#else
	if (hal_rtc_set_time(time) == 0)
#endif
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, set rtc time = %d(s)!\r\n", time);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, set rtc failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_rtc, set, utest_rtc_set_time, "unit test set time", "ut_rtc set [time-seconds]");


static int utest_rtc_get_time(cmd_tbl_t *t, int argc, char *argv[])
{
	int res = 0;

	res = hal_rtc_get_time();
	#ifdef CONFIG_SNTP
	float tz = get_timezone();
	res = res - (int)(tz * ONE_HOUR2SEC);
	#endif
	os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, get rtc time = %d(s)!\r\n", res);

	return 0;
}

CLI_SUBCMD(ut_rtc, get, utest_rtc_get_time, "unit test get time", "ut_rtc get");


static int utest_rtc_utctime_to_date(cmd_tbl_t *t, int argc, char *argv[])
{
	int time;
	DATE_RTC dr;
	if (argc >= 2)
	{
		time = (int)strtoul(argv[1], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, err: no enough argt!\r\n");
		return 0;
	}

	if (hal_rtc_utctime_to_date(time,&dr) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc,[%d] utctime to date: YYYY-MM-DD HH:mm:ss = %4d-%02d-%02d %02d:%02d:%02d week=%s!\r\n", time, dr.year,dr.month,dr.day,dr.hour,dr.min,dr.sec,weeken[dr.wday]);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, set rtc failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_rtc, utctime_to_date, utest_rtc_utctime_to_date, "unit test utctime to date format", "ut_rtc utctime_to_date [time-seconds]");	

static int utest_rtc_date_to_utctime(cmd_tbl_t *t, int argc, char *argv[])
{
	int time;
	unsigned char daysofmonth=0;
	unsigned char wk = 0;
	DATE_RTC dr;
	if (argc >= 8)
	{
		//dr.year = (unsigned int)strtoul(argv[1], NULL, 0);
		dr.year = (unsigned short)atoi(argv[1]);
		dr.month = (unsigned char)atoi(argv[2]);
		dr.day = (unsigned char)atoi(argv[3]);
		dr.hour = (unsigned char)atoi(argv[4]);
		dr.min = (unsigned char)atoi(argv[5]);
		dr.sec = (unsigned char)atoi(argv[6]);
		dr.wday = (unsigned char)atoi(argv[7]);

		if (!(IS_LEAP_YEAR(dr.year) && (dr.month == 2)))
		{
			daysofmonth = month_days2[dr.month];
		}
		else
		{
			daysofmonth = 29;
		}
		wk = rtc_get_weekday(dr.year,dr.month,dr.day);
		if(	dr.year < 1970	|| dr.month > 12|| dr.hour >=24 	||\
			dr.min >=60		|| dr.sec >=60 	|| dr.wday != wk 	||\
			dr.day > daysofmonth)
		{
			os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, err: please input correct args!\r\n");
			return 0;
		}

	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, err: no enough arg!\r\n");
		return 0;
	}

	if (hal_rtc_date_to_utctime(dr,&time) == 0)
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, YYYY-MM-DD HH:mm:ss = %4d-%02d-%02d %02d:%02d:%02d week=%s!date to utctime =[%d]\r\n", dr.year,dr.month,dr.day,dr.hour,dr.min,dr.sec,weeken[dr.wday], time);
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,"\r\nunit test rtc, set rtc failed!\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_rtc, date_to_utctime, utest_rtc_date_to_utctime, "unit test date to utctime format", "ut_rtc date_to_utctime [year] [month] [day] [hour] [min] [sec] [week]");	


CLI_CMD(ut_rtc, NULL, "unit test hal rtc", "test_hal_rtc");


