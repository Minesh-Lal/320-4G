/******************************************************************
 * DS1337 Driver
 *
 * This file contains the protocol to control the DS1337 real time
 * Clock.
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/

#ifndef _DS1337_
#define _DS1337_

void rtc_init(void);
//void rtc_off(void);
void rtc_get_time(unsigned char *hour,unsigned char *min,unsigned char *sec);
void rtc_set_time(unsigned char hour,unsigned char min,unsigned char sec);
void rtc_get_date(unsigned char *date,unsigned char *month,unsigned char *year);
void rtc_set_date(unsigned char date,unsigned char month,unsigned char year);
void rtc_set_alarm_1(char day, char hour, char min, char sec, unsigned char type);
void rtc_clear_1 (void);
//void rtc_get_day(unsigned char *day);	//SJL - will be required for days of the week
//void rtc_set_alarm_2(unsigned char hour,unsigned char min, unsigned char type);
//void rtc_clear_2 (void);
//unsigned char rtc_check_alarm();

interrupt [EXT_INT5] void ext_int5_isr(void);

enum
{
    //ALARM_SECOND,
    //ALARM_MINUTE,
    //ALARM_HOUR,
    ALARM_DAY, 		//0
    ALARM_WEEK,		//1
    ALARM_MONTH,	//2
    NO_ALARM		//3
};

static const char month_length[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

#endif