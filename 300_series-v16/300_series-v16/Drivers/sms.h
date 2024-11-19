#ifndef _SMS_H_
#define _SMS_H_
/******************************************************************
 * sms.h
 *
 * Send and Receive command set for SMS Messages
 *
 * This file contains the commands for sending and receiving SMS
 * messages using the GSM 07.07 / 07.05 AT Command Set.
 *
 * Author: Chris Cook
 * Project: SMS300
 * Dec 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/

#include "drivers/event.h"

//#define SMS_START_LEN 0

//Defines for the queue.  Defines the type of SMS in the queue
//#define SMS_NONE 0
//#define SMS_SEND 1
//#define SMS_REC 2

//Defines for the task handler.
// #define sms_set_task(task) sms_current_task=task


//#define SMS_NO_MSG 0
//#define SMS_ALARM_MSG 1
//#define SMS_RESET_MSG 2
//#define SMS_ACTIVATE_MSG 3
//#define SMS_DEACTIVATE_MSG 4
//#define SMS_SYS_QUERY 5
//#define SMS_SYS_QUERY_SEND 11
//#define SMS_STATUS_QUERY 6
//#define SMS_VODAFONE 7
//#define SMS_TELECOM 8
//#define SMS_WAIT_SEND 9
//#define SMS_RESPONSE_REC 10
//#define SMS_UART_SEND 12
//#define SMS_RETRY_WAIT 13
//#define SMS_SENSOR_FAIL 14	//defined in event.h

//#define SMS_QUEUE_SIZE 16


//#define sms_rec_set_task(task) sms_rec_current_task=task
// #define SMS_REC_NONE 0
// #define SMS_REC_NEW 1
// #define SMS_REC_CHECK_MSG 2
// #define SMS_REC_FWD 3
// #define SMS_REC_STATUS 4
// #define SMS_REC_SYS 5
// #define SMS_REC_OUTPUT 6


// #define USE_SAVED_PHONE_NUMBER 0
// #define NO_IO -1
// #define MASTER_USER 0x01


typedef enum
    {
        EMPTY_CONTACT=0,
        EMAIL,
        SMS,
        FTP,
        TCP,
        VOICE,
        DATA,
        HTTP
    } msg_type;

#define MSG_REC_MAXLEN 50
typedef struct smsSendConstruct
{
    unsigned int contactList;
    char txt[170];
    char phoneNumber [MSG_REC_MAXLEN];
    bool usePhone;
    char index;
} sms_t;


//#define FIRST 0
//#define FAIL 1
//#define RETRY 2
//#define OK 3

void sms_handleNew (Event *handleThis);
void msg_sendData();
void sms_process();
bool sms_cmpPhone (char *contact);
bool sms_checkConnection();
void sms_set_header(Event *e);



/* GLOBALS */
/**********************************************************************************************************/
/* PROTOTYPE HEADRERS */

/**********************************************************************************************************/
/* STRUCTS */
extern sms_t sms_newMsg;        //structure storing the message to be sent or rec

/**********************************************************************************************************/
/* VARIABLES */

//SJL - CAVR2 - moved from global.c
extern unsigned char msg_index;         //index the contact list is up to
extern unsigned char sms_charCount;
extern unsigned char sms_delIndex;      //sms that is to be deleted
extern bit sms_header;         //flag set if you want to add sitename and time to sms
//bool sms_send_active;         //the modem is currently in the middle of a send
extern unsigned char msg_send;
extern bit modem_queue_stuck;

extern char sms_dates[40];
/**********************************************************************************************************/


#endif