#ifndef _EVENT_H_
#define _EVENT_H_

#include "global.h"

typedef struct event_t
{
    char type;
    char param;
    char year;
    char month;
    char day;
    char hour;
    char minute;
    char second;
} Event;

//Define the events that are queued in the event queue
#define event_CHXR         1      //Generic input channel rising edge event.
#define event_CHAR         1      //Input channel rising event on alarm a
#define event_CHBR         2      //Input channel rising event on alarm b

#define event_CHXF         3      //Generic input channel falling edge event.
#define event_CHAF         3      //Input channel falling event on alarm a
#define event_CHBF         4      //Input channel falling event on alarm b
#define event_SENSOR_FAIL  5

#define event_UART_REC     6      //Uart Receive event
#define event_MODEM_REC    7      //Modem Receive event

#define event_OUT_ON       8      //Output On
#define event_OUT_OFF      9      //Output Off
#define event_REMOTE_OUT_ON 10
#define event_REMOTE_OUT_OFF 11
#define event_THRU_MODE    12     //The modem has received a data call, enable through mode

#define event_LOG_ENTRY    13   //write log entry to MMC (most likely)

#define event_MMC_INIT     15
#define mmc_CLEAR_DATA     16
#define event_MODEM_REC_OLD    	17

#define event_EXTERNAL_UART		18

#define event_CHECK_NETWORK_STATUS	19

//Define the events that are queue in the SMS queue
#define sms_ALARM_A        1       //SMS Input Alarm A
#define sms_ALARM_B        2       //SMS Input Alarm B
#define sms_RESET_A        3       //SMS Input Reset A
#define sms_RESET_B        4       //SMS Input Reset B
#define sms_REMOTE_OUT_ON  5       //SMS Input Output control On
#define sms_REMOTE_OUT_OFF 6       //SMS Input Output control Off
#define sms_OUT_ON         7       //SMS Output On
#define sms_OUT_OFF        8       //SMS Output Off
#define sms_STATUS         9       //SMS Status
#define sms_SYSTEM         10      //SMS System
#define sms_EDAC           11      //SMS EDAC Check
#define sms_CONFIG         12      //SMS Config Confirm
#define sms_FORWARD        13      //SMS Forward SMS
#define sms_SENSOR_FAIL    14      //SMS Input Sensor Failure
#define sms_CHECK_IN       15      //SMS Check in
//#define sms_UART_SEND      16      //SMS UART send SMS
#define sms_GET_CSQ        17      //Get CSQ
#define sms_READ_SMS       18      //Read Incoming SMS
#define sms_FAIL           19      //The SMS sent didn't have the correct syntax
#define sms_CHECK_MSG      20      //Check to see if there are any unread messges
#define sms_DELETE         21      //Delete an SMS once it has been read.
#define sms_CHECK_NETWORK  22
#define sms_OUT_FAIL       23
#define sms_REMOTE_OUT_FAIL 24
#define sms_CHECK_NETWORK_INTERNAL 25
#define sms_DELETE_SENT     26
//#define sms_GETEMAIL        27
#define sms_GPRS_CHECK      28
#define sms_MMC_ACCESS_FAIL 29
#define sms_LOG_UPDATE      30   //send log file to email address given
//#define sms_GPRSSTATE       31
#define sms_GPS_REQUEST     32
#define sms_CONTACT_CHANGE  33
#define sms_AUTO_REPORT     34
//#define sms_RESET_STATUS  35
//#define sms_LATEST    	35
#define sms_EMAILRETRYTIMER	35
#define sms_UNITINFO		36
#define sms_PIN_CHANGE  	37
#define	sms_EMAIL_CHANGE	38
#define sms_EMAIL_ERROR		39
#define sms_CHECK_NETWORK_STATUS	40

//Masking for the remote output switching
#define INPUT_NUMBER_BITS 0x07
#define OUTPUT_NUMBER_BITS 0x18
#define ALARM_NUMBER_BITS 0x60
#define ALARM_ACTION_BITS 0x80

#define INPUT_NUMBER (param & INPUT_NUMBER_BITS)
#define OUTPUT_NUMBER ((param & OUTPUT_NUMBER_BITS) >> 3)
#define ALARM_NUMBER ((param & ALARM_NUMBER_BITS) >> 5)
#define ALARM_ACTION ((param & ALARM_ACTION_BITS) >> 7)
#define OUTPUT_ALARM_ACTION 1

//Types for the CSQ response
#define CSQ 1
#define RSSI 2
#define BER 3

void event_handleNew (Event *incomingEvent);
void event_populate_time(Event *e);

//extern const char *email_cmdList[];     //SJL - CAVR2 - from event.c

#endif
