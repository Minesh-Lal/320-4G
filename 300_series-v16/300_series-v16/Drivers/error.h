#ifndef _ERROR_H_
#define _ERROR_H_

typedef eeprom struct dateStruct
{ 
    unsigned char day;
    unsigned char month;
    unsigned char year;
    unsigned char min;
    unsigned char hour;
    unsigned char sec;
} date_t;

typedef eeprom struct errorStruct
{ 
    unsigned int q_event_ovf;
    unsigned int q_modem_ovf; 
    date_t startup;
    unsigned int powerOnReset;
    unsigned int externalReset;
    unsigned int brownOutReset;
    unsigned int watchdogReset;
    unsigned int jtagReset;  
    unsigned int unknownReset;
    unsigned int emailRetryCounter;
    unsigned int emailFailCounter;
    unsigned int modemErrorCounter;
    unsigned int networkFailureCounter;
    unsigned int throughModeCounter;
    unsigned int framingErrorCounter;
    unsigned long emailSendCounter;  
    
} error_t;



/* GLOBALS */
/**********************************************************************************************************/
/* PROTOTYPE HEADRERS */

/**********************************************************************************************************/
/* STRUCTS */
extern eeprom error_t error;    //SJL - CAVR2 - from main.c

/**********************************************************************************************************/
/* VARIABLES */

/**********************************************************************************************************/


#endif