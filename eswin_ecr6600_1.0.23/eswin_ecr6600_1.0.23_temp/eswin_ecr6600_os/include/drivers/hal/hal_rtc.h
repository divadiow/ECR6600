#ifndef HAL_RTC_H
#define HAL_RTC_H


#define ORIGIN_YEAR		1970
#define ORIGIN_MONTH	1
#define ORIGIN_DAY		1
#define ORIGIN_WEEK		4

#define MONTON_MAX		12
#define DAYS_MAX		31
#define HOUR_MAX		24
#define MIN_MAX			60
#define SEC_MAX			60
#define ONE_WEEK2DAYS	7
#define ONE_HOUR2SEC	3600
#define LEAP_MONTH		2


/**
 * @brief DATE_RTC struct.
 */
typedef struct{
	unsigned short 	year;
	unsigned char 	month;
	unsigned char  	day;
	unsigned char  	hour;
	unsigned char  	min;
	unsigned char  	sec;
	unsigned char 	wday;
}DATE_RTC;

/**    @brief		set hal time function .
*	   @param[in]	rtc_time    Total seconds since January 1, 1970
*	   @return  	0-- set hal rtc time succeed, -1--set time failed
*/
int hal_rtc_set_time(int rtc_time);

/**    @brief		get hal time function .
*	   @return  	Total seconds since January 1, 1970
*/
int hal_rtc_get_time();

/**    @brief		format convert. utctime to DATE_RTC struct convert
*	   @param[in]	utc_time    Total seconds since January 1, 1970
*	   @param[in]	date    	point of DATE_RTC struct data;
*	   @return  	0-- succeed 	-1-- failed
*/
int hal_rtc_utctime_to_date(int utc_time, DATE_RTC * date);

/**    @brief		format convert. utctime to DATE_RTC struct convert
*	   @param[in]	date    	DATE_RTC struct data;
*	   @param[out]	utc_time    the point of Total seconds since January 1, 1970
*	   @return  	0-- succeed 	-1-- failed
*/
int hal_rtc_date_to_utctime(DATE_RTC date, int * utc_time);

int hal_rtc_init();

#endif /*HAL_RTC_H*/




