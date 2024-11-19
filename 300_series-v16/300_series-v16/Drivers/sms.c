/******************************************************************
 * sms.c
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
#include <ctype.h>
#include <string.h>
#include <stdio.h>              //SJl - CAVR2 - added
#include <delay.h>              //SJl - CAVR2 - added
#include <stdlib.h>             //SJl - CAVR2 - added

#include "drivers\str.h"
#include "global.h"
#include "drivers\input.h"
#include "drivers\event.h"
#include "drivers\sms.h"
#include "drivers\config.h"
#include "drivers\debug.h"
#include "drivers\contact.h"
#include "drivers\modem.h"
#include "buildnumber.h"
#include "drivers\gps.h"
#include "drivers\uart.h"       //SJl - CAVR2 - added to access uart print functions
#include "drivers\config.h"     //SJl - CAVR2 - added to access conig struct
#include "drivers\queue.h"      //SJl - CAVR2 - added to access queue functions
#include "drivers\error.h"      //SJl - CAVR2 - added to access error struct
#include "drivers\mmc.h"        //SJl - CAVR2 - added to access logging functions
//#include "drivers\gprs.h"       //SJl - CAVR2 - added to access gprs functions
#include "drivers\ds1337.h"     //SJl - CAVR2 - added to access real time clock functions
#include "drivers\output.h"     //SJl - CAVR2 - added to access LAST_KNOWN and OFF variables

/************************************************************************
 * SMS magic match patterns
 */

static int sub_magic(char *text)
{
	int i;

	/* Find opening */
	for (i = 0; text[i]; i++) {
		if (text[i] == '*' && text[i + 1] == '<') {
			int j = i + 2;
			int len;
			int k;

			while (text[j] && text[j] != '>')
				j++;
			if (!text[j])
				return 0;

			len = j - i - 2;

			for (k = 0; k < len; k++)
				text[k] = text[k + i + 2];
			text[k] = 0;
			return 1;
		}
	}

	return 0;
}

static int rmatch_ws(const char *text, int *pio)
{
	int p = *pio;

	while (p > 0) {
		char c = text[p - 1];

		if (!(c == ' ' || c == '\r' || c == '\n' || c == '\t'))
			break;
		p--;
	}

	if (p == *pio)
		return 0;

	*pio = p;
	return 1;
}

static int rmatch_number(const char *text, int *pio)
{
	int p = *pio;

	while (p > 0) {
		char c = text[p - 1];

		if (c < '0' || c > '9')
			break;
		p--;
	}

	if (p == *pio)
		return 0;

	*pio = p;
	return 1;
}

static int rmatch_char(const char *text, int *pio, char what)
{
	int p = *pio;

	if (p > 0 && text[p - 1] == what) {
		*pio = p - 1;
		return 1;
	}

	return 0;
}

static int rmatch_nsep(const char *text, int *pio, char what, int n)
{
	if (!rmatch_number(text, pio))
		return 0;
	n--;

	while (n--) {
		if (!rmatch_char(text, pio, what))
			return 0;
		if (!rmatch_number(text, pio))
			return 0;
	}

	return 1;
}

static int has_dt_suffix(const char *text)
{
	int p = strlen(text);

	rmatch_ws(text, &p);

	return rmatch_nsep(text, &p, '/', 3) &&
	       rmatch_ws(text, &p) &&
	       rmatch_nsep(text, &p, ':', 2);
}

/************************************************************************/

//GLOBAL DEFINITIONS
bit modem_queue_stuck=0;
unsigned char msg_index; // = 255;	//SJL - CAVR2 - must be defined for linking
sms_t sms_newMsg;					//SJL - CAVR2 - must be defined for linking
unsigned char sms_charCount;		//SJL - CAVR2 - must be defined for linking
unsigned char sms_delIndex;			//SJL - CAVR2 - must be defined for linking
char sms_dates[40];					//SJL - used to store date string from sms request for log email
bit sms_header = 0;                        //flag set if you want to add sitename and time to sms
unsigned char msg_send = false;

//GLOBAL CONSTANTS - will be stored in flash memory
//const unsigned char* FACTORY_RESET = "at&f";	//SJL - CAVR2
//const unsigned char* HARDWARE_RESET = "at&r"; //SJL - CAVR2
const char *vstate_values[] = {"IDLE","CONNECTED","DIALING","AUTHENTICATING","NO SERVICE","DISCONNECTING","CHECKING","ERROR:", "ERROR"};
const unsigned int SMS_MAX_LEN = 140;

enum
{
    IDLE = 0,
    CONNECTED,
    DIALING,
    AUTHENTICATING,
    NO_SERVICE,
    DISCONNECTED,
    CHECKING,
    GENERAL_ERROR,
    VSTATE_ERROR
};

/*
    Handle the new message event that has been popped off the queue
*/
void sms_handleNew (Event *handleThis)
{	
    float inputVal;	//SJL - CAVR2 - double not supported
    signed int pos=0;
    Event e;
    char i=0, j=0, *c, *d;
    char event, param;
    unsigned char length=0;
    unsigned char sms_charCount=0;
	//char datetime[19];
	eeprom char *temp;
	
    sms_newMsg.usePhone = false;
    event = handleThis->type;
    param = handleThis->param;
    switch (event)
    {
		/* case sms_GPRSSTATE:
            #if MODEM_TYPE == Q2406B && TCPIP_AVAILABLE
            //Return the status of the network connection
            modem_clear_channel();
            printf("AT#VSTATE\r\n");
            modem_wait_for(MSG_STATE | MSG_ERROR);
            for(i=0;i<7;i++)
            {
                if(strstrf(modem_rx_string,vstate_values[i])!=0)
                    break;
            }
            switch(i)
            {
                case NO_SERVICE:
                    printStr("+GPRSSTATE=No Service\r\n");
                    modem_state = NO_CONNECTION;
                    break;
                case IDLE:
                    printStr("+GPRSSTATE=Idle\r\n");
                    modem_state = CELL_CONNECTION;
                    break;
                case CONNECTED:
                    printStr("+GPRSSTATE=Connected\r\n");
                    modem_state = GPRS_CONNECTION;
                    break;
                case DIALING:
                    printStr("+GPRSSTATE=Dialing\r\n");
                    break;
                case AUTHENTICATING:
                    printStr("+GPRSSTATE=Authenticating\r\n");
                    break;
                case CHECKING:
                    printStr("+GPRSSTATE=Checking\r\n");
                    break;
                default:
                    printStr("+GPRSSTATE=");
                    print(modem_rx_string);
                    print_char('\r');
                    print_char('\n');
                    break;
            }
            modem_wait_for(MSG_OK | MSG_ERROR); //clear following OK
            #elif MODEM_TYPE == Q24NG_PLUS
                switch(modem_state)
                {
                    case CELL_CONNECTION:
                        printStr("+GPRSSTATE=Idle\r\n");
                        break;
                    case NO_CONNECTION:
                        printStr("+GPRSSTATE=No Service\r\n");
                        break;
                    case GPRS_CONNECTION:
                        printStr("+GPRSSTATE=Connected\r\n");
                        break;
                    default:
                        sprintf(buffer,"+GPRSSTATE=%d\r\n",modem_state);
                        break;

                }
            #else
                printStr("+GPRSSTATE=UNAVAILABLE\r\n");
            #endif
        break; */

        //SMS Input Alarm A
        case sms_ALARM_A :
            sms_newMsg.contactList = config.input[param].alarm[ALARM_A].alarm_contact;
            //Need to add the analog reading for the port here too.
            strcpyre(buffer, config.site_name);
            sprintf(sms_newMsg.txt,"%s\r",buffer);
            strcpyre(buffer, config.input[param].alarm[ALARM_A].alarm_msg);
            strcat(sms_newMsg.txt,buffer);
            if (config.input[param].type != DIGITAL)
            {
                inputVal = (config.input[param].conv_grad * input[param].ADCVal) + config.input[param].conv_int;
                sprintf(buffer," %2.1f", inputVal);
                strcat(sms_newMsg.txt, buffer);
                strcpyre(buffer,config.input[param].units);
                strcat(sms_newMsg.txt, buffer);
                #ifdef PULSE_COUNTING_AVAILABLE
                if(config.input[param].type == PULSE)
                {
                    switch(config.pulse[param-6].period)
                    {
                        case SECONDS:
                            sprintf(buffer,"/s");
                            break;
                        case MINUTES:
                            sprintf(buffer,"/m");
                            break;
                        case HOURS:
                            sprintf(buffer,"/h");
                    }
                    strcat(sms_newMsg.txt, buffer);
                }
                #endif
            }
            sms_set_header(handleThis);
            #ifdef DEBUG
            //sprintf(buffer,"ALARM A msg = [%s], contact=0x%X\r\n", sms_newMsg.txt, sms_newMsg.contactList);
            //print(buffer);
            printStr("Alarm A message:");
            print(sms_newMsg.txt);
            printStr("\r\n");
            #endif
            //Pause this queue, so that nothing is popped off until the send is complete
            modem_queue_stuck=false;
            //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"3\r\n");print1(buffer);
			#endif
			
            msg_sendData();
            break;

        //SMS Input Alarm B
        case sms_ALARM_B :
            sms_newMsg.contactList = config.input[param].alarm[ALARM_B].alarm_contact;
            //Need to add the analog reading for the port here too.
            strcpyre(buffer, config.site_name);
            sprintf(sms_newMsg.txt,"%s\r",buffer);
            strcpyre(buffer, config.input[param].alarm[ALARM_B].alarm_msg);
            strcat(sms_newMsg.txt,buffer);
            if (config.input[param].type != DIGITAL)
            {
                inputVal = (config.input[param].conv_grad * input[param].ADCVal) + config.input[param].conv_int;
                sprintf(buffer," %2.1f", inputVal);
                strcat(sms_newMsg.txt, buffer);
                strcpyre(buffer,config.input[param].units);
                strcat(sms_newMsg.txt, buffer);
                #ifdef PULSE_COUNTING_AVAILABLE
                if(config.input[param].type == PULSE)
                {
                    switch(config.pulse[param-6].period)
                    {
                        case SECONDS:
                            sprintf(buffer,"/s");
                            break;
                        case MINUTES:
                            sprintf(buffer,"/m");
                            break;
                        case HOURS:
                            sprintf(buffer,"/h");
                    }
                    strcat(sms_newMsg.txt, buffer);
                }
                #endif
            }
            sms_set_header(handleThis);
            #ifdef DEBUG
            sprintf(buffer,"ALARM B msg = %s, contact=0x%X\r\n", sms_newMsg.txt, sms_newMsg.contactList);print(buffer);
            #endif
            //Pause this queue, so that nothing is popped off until the send is complete
            //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"4\r\n");print1(buffer);
			#endif
			
            msg_sendData();
            break;

        //SMS Input Reset A
        case sms_RESET_A :
            sms_newMsg.contactList = config.input[param].alarm[ALARM_A].reset_contact;
            //Need to add the analog reading for the port here too.
            strcpyre(buffer, config.site_name);
            sprintf(sms_newMsg.txt,"%s\r",buffer);
            strcpyre(buffer, config.input[param].alarm[ALARM_A].reset_msg);
            strcat(sms_newMsg.txt,buffer);
            if (config.input[param].type != DIGITAL)
            {
                inputVal = (config.input[param].conv_grad * input[param].ADCVal) + config.input[param].conv_int;
                sprintf(buffer," %2.1f", inputVal);
                strcat(sms_newMsg.txt, buffer);
                strcpyre(buffer,config.input[param].units);
                strcat(sms_newMsg.txt, buffer);
                #ifdef PULSE_COUNTING_AVAILABLE
                if(config.input[param].type == PULSE)
                {
                    switch(config.pulse[param-6].period)
                    {
                        case SECONDS:
                            sprintf(buffer,"/s");
                            break;
                        case MINUTES:
                            sprintf(buffer,"/m");
                            break;
                        case HOURS:
                            sprintf(buffer,"/h");
                    }
                    strcat(sms_newMsg.txt, buffer);
                }
                #endif
            }
            sms_set_header(handleThis);
            #ifdef DEBUG
            sprintf(buffer,"RESET A msg = %s, contact=0x%X\r\n", sms_newMsg.txt, sms_newMsg.contactList);print(buffer);
            #endif
            //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"5\r\n");print1(buffer);
			#endif
			
            msg_sendData();
             break;

        //SMS Input Reset B
        case sms_RESET_B :
            sms_newMsg.contactList = config.input[param].alarm[ALARM_B].reset_contact;
            //Need to add the analog reading for the port here too.
            strcpyre(buffer, config.site_name);
            sprintf(sms_newMsg.txt,"%s\r",buffer);
            strcpyre(buffer, config.input[param].alarm[ALARM_B].reset_msg);
            strcat(sms_newMsg.txt,buffer);
            if (config.input[param].type != DIGITAL)
            {
                inputVal = (config.input[param].conv_grad * input[param].ADCVal) + config.input[param].conv_int;
                sprintf(buffer," %2.1f", inputVal);
                strcat(sms_newMsg.txt, buffer);
                strcpyre(buffer,config.input[param].units);
                strcat(sms_newMsg.txt, buffer);
                #ifdef PULSE_COUNTING_AVAILABLE
                if(config.input[param].type == PULSE)
                {
                    switch(config.pulse[param-6].period)
                    {
                        case SECONDS:
                            sprintf(buffer,"/s");
                            break;
                        case MINUTES:
                            sprintf(buffer,"/m");
                            break;
                        case HOURS:
                            sprintf(buffer,"/h");
                    }
                    strcat(sms_newMsg.txt, buffer);
                }
                #endif
            }
            sms_set_header(handleThis);
            #ifdef DEBUG
            sprintf(buffer,"RESET B msg = %s, contact=0x%X\r\n", sms_newMsg.txt, sms_newMsg.contactList);print(buffer);
            #endif
            //Start the send operation
            #ifdef _SMS_DEBUG_
			sprintf(buffer,"6\r\n");print1(buffer);
			#endif			
			msg_sendData();			
            break;



        //SMS Input Output control On
        //param = bit masked information
        //input number = param & 0x7
        //output number = param & 0x18
        //alarm number = param & 0x60
        //action = param & 0x80
        case sms_REMOTE_OUT_ON :
            DEBUG_printStr("sms_REMOTE_OUT_ON event\r\n");
//            #ifdef _MODEM_DEBUG_
//            sprintf(buffer,"alaAct=%d, outAlamA=%d, input=%d, alarmNum=%d\r\n", ALARM_ACTION, OUTPUT_ALARM_ACTION, INPUT_NUMBER, ALARM_NUMBER);print(buffer);
//            #endif
            if (!ALARM_ACTION)
            {
                #ifdef DEBUG
                sprintf(buffer,"alarmAction.contactList=0x%x\r\n", config.input[INPUT_NUMBER].alarm[ALARM_NUMBER].alarmAction.contact_list);print(buffer);
                #endif
                sms_newMsg.contactList = config.input[INPUT_NUMBER].alarm[ALARM_NUMBER].alarmAction.contact_list;
            }
            else
            {
                #ifdef DEBUG
                sprintf(buffer,"resetAction.contactList=0x%x\r\n", config.input[INPUT_NUMBER].alarm[ALARM_NUMBER].resetAction.contact_list);print(buffer);
                #endif
                sms_newMsg.contactList = config.input[INPUT_NUMBER].alarm[ALARM_NUMBER].resetAction.contact_list;
            }



            //Build the message to send
            strcpyre(sms_newMsg.txt, config.pin_code);
            if(strlene(config.pin_code) != 0)
            {
                sprintf(buffer," ");
                strcat(sms_newMsg.txt, buffer);
            }
            sprintf(buffer,"%d on", OUTPUT_NUMBER+1);
            strcat(sms_newMsg.txt, buffer);
            #ifdef DEBUG
            sprintf(buffer,"REMOTE OUT ON msg = %s, contact=0x%X\r\n", sms_newMsg.txt, sms_newMsg.contactList);print(buffer);
            #endif
            //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"7\r\n");print1(buffer);
			#endif
			
            msg_sendData();
            break;

        //SMS Input Output control Off
        case sms_REMOTE_OUT_OFF :
            DEBUG_printStr("sms_REMOTE_OUT_OFF event\r\n");
//            #ifdef _MODEM_DEBUG_
//            sprintf(buffer,"alaAct=%d, outAlamA=%d, input=%d, alarmNum=%d\r\n", ALARM_ACTION, OUTPUT_ALARM_ACTION, INPUT_NUMBER, ALARM_NUMBER);print(buffer);
//            #endif
            if (!ALARM_ACTION)
            {
                #ifdef DEBUG
                sprintf(buffer,"alarmAction.contactList=0x%x\r\n", config.input[INPUT_NUMBER].alarm[ALARM_NUMBER].alarmAction.contact_list);print(buffer);
                #endif
                sms_newMsg.contactList = config.input[INPUT_NUMBER].alarm[ALARM_NUMBER].alarmAction.contact_list;
            }
            else
            {
                #ifdef DEBUG
                sprintf(buffer,"resetAction.contactList=0x%x\r\n", config.input[INPUT_NUMBER].alarm[ALARM_NUMBER].resetAction.contact_list);print(buffer);
                #endif
                sms_newMsg.contactList = config.input[INPUT_NUMBER].alarm[ALARM_NUMBER].resetAction.contact_list;
            }


            //Build the message to send
            strcpyre(sms_newMsg.txt, config.pin_code);
            if(strlene(config.pin_code) != 0)
            {
                sprintf(buffer," ");
                strcat(sms_newMsg.txt, buffer);
            }
            sprintf(buffer,"%d off", OUTPUT_NUMBER+1);
            strcat(sms_newMsg.txt, buffer);
            #ifdef DEBUG
            sprintf(buffer,"REMOTE OUT OFF msg = %s, contact=0x%X\r\n", sms_newMsg.txt, sms_newMsg.contactList);print(buffer);
            #endif
            //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"8\r\n");print1(buffer);
			#endif
			
            msg_sendData();
            break;

        //SMS Output On
        case sms_OUT_ON :
            sms_newMsg.contactList = config.output[param].config.on_contact;
            //set up the message
            strcpyre(buffer, config.site_name);
            sprintf(sms_newMsg.txt,"%s\r",buffer);
            strcpyre(buffer, config.output[param].config.on_msg);
            strcat(sms_newMsg.txt,buffer);
            sms_set_header(handleThis);

            #ifdef DEBUG
            sprintf(buffer,"OUT ON msg = %s, contact=0x%X\r\n", sms_newMsg.txt, sms_newMsg.contactList);print(buffer);
            #endif
            //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"9\r\n");print1(buffer);
			#endif
			
            msg_sendData();
            break;

        //SMS Output Off
        case sms_OUT_OFF :
            sms_newMsg.contactList = config.output[param].config.off_contact;

            //set up the message
            strcpyre(buffer, config.site_name);
            sprintf(sms_newMsg.txt,"%s\r",buffer);
            strcpyre(buffer, config.output[param].config.off_msg);
            strcat(sms_newMsg.txt,buffer);
            sms_set_header(handleThis);

            #ifdef DEBUG
            sprintf(buffer,"OUT OFF msg = %s, contact=0x%X\r\n", sms_newMsg.txt, sms_newMsg.contactList);
            print(buffer);
            #endif
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"10\r\n");print1(buffer);
			#endif

            msg_sendData();
            break;

        //SMS Status
        //case sms_RESET_STATUS:
        case sms_AUTO_REPORT:
        case sms_STATUS :
            if(handleThis->type==sms_AUTO_REPORT) // || handleThis->type==sms_RESET_STATUS)
            {
                strcpyre(buffer,contact_read(0));
                strcpy(sms_newMsg.phoneNumber,strchr(buffer,'<')+1);
                *strchr(sms_newMsg.phoneNumber,'>') = '\0';
            }
            sms_newMsg.usePhone = true;
            
			//removed ability to request status message via email - see earlier code versions

			//SMS_MAX_LEN = 140;
			#ifdef DEBUG
			sprintf(buffer,"param=%d\r\n",param);print(buffer);
			#endif
			//If this is the first status message sent in this query, then append the site
			//msg first
			//Check to see if this is the second message being sent.  If it is, then start the
			//messages part way through the list, if necessary.
			//Start the counter with the length of the site message and time

			//Clear the sms buffer
			if(param==0)
			{
				strcpyre(sms_newMsg.txt,config.site_name);
				if(handleThis->type==sms_AUTO_REPORT)
					sprintf(buffer,"\nReporting:\n");
				//else if(handleThis->type==sms_RESET_STATUS)
				//    sprintf(buffer,"\nUnit Restarted:\n");
				else
					sprintf(buffer,"\n");
				strcat(sms_newMsg.txt,buffer);
				sms_charCount = strlen(sms_newMsg.txt);
			}
			else
				sprintf(sms_newMsg.txt,"");

			//Decide where you are up to
			if (param < MAX_INPUTS)
			{
				//Loop through all the inputs to print there mesasges
				for (; param < MAX_INPUTS; param++)
				{
					if (config.input[param].enabled)
					{
						#ifdef DEBUG
						sprintf(buffer,"config.input[%d].msg = %d\r\n",param, config.input[param].msg);print(buffer);
						#endif
						if ((config.input[param].msg == sms_ALARM_A) || (config.input[param].msg == sms_ALARM_B))
						{
							if (config.input[param].msg == sms_ALARM_A)
								pos = ALARM_A;
							else
								pos = ALARM_B;

							length = strlene(config.input[param].alarm[pos].alarm_msg) + 1;
							if (config.input[param].type != DIGITAL)
							{
								inputVal = (config.input[param].conv_grad * input[param].ADCVal) + config.input[param].conv_int;
								sprintf(buffer," %2.1f", inputVal);
								length += strlen(buffer);
								length += strlene(config.input[param].units);
							}
							if ((length + sms_charCount) >= SMS_MAX_LEN)
							{
								//We have run out of characters for the sms, so send
								//it now
								#ifdef DEBUG
								 sprintf(buffer,"Status Response msg = ");print(buffer);
								print(sms_newMsg.txt);
								sprintf(buffer,", contact=%s\r\n", sms_newMsg.phoneNumber);print(buffer);
							   #endif
								e.type = sms_STATUS;
								e.param = param;
								queue_push(&q_modem, &e);
								sms_set_header(handleThis);
								
								#ifdef _SMS_DEBUG_
								sprintf(buffer,"11\r\n");print1(buffer);
								#endif
			
								msg_sendData();
								return;
							}
							else
							{
								strcpyre(buffer, config.input[param].alarm[pos].alarm_msg);
								strcat(sms_newMsg.txt, buffer);
								if (config.input[param].type != DIGITAL)
								{
									sprintf(buffer," %2.1f", inputVal);
									strcat(sms_newMsg.txt, buffer);
									strcpyre(buffer,config.input[param].units);
									strcat(sms_newMsg.txt, buffer);
									if(config.input[param].type == PULSE)
									{
										switch(config.pulse[param-6].period)
										{
											case SECONDS:
												sprintf(buffer,"/s");
												break;
											case MINUTES:
												sprintf(buffer,"/m");
												break;
											case HOURS:
												sprintf(buffer,"/h");
												break;
										}
										strcat(sms_newMsg.txt, buffer);
									}
								}

								sprintf(buffer,"\n");
								strcat(sms_newMsg.txt, buffer);

								sms_charCount += length;
								#ifdef DEBUG
								sprintf(buffer, "sms_charCount=%d\r\n", sms_charCount);print(buffer);
								#endif
							}
						}
						else
						{
							if (config.input[param].msg == sms_RESET_A)
								pos = ALARM_A;
							else
								pos = ALARM_B;

							length = strlene(config.input[param].alarm[pos].reset_msg) + 1;
							if (config.input[param].type != DIGITAL)
							{
								inputVal = (config.input[param].conv_grad * input[param].ADCVal) + config.input[param].conv_int;
								sprintf(buffer," %2.1f", inputVal);
								length += strlen(buffer);
								length += strlene(config.input[param].units);
								if(config.input[param].type == PULSE)
									length += 2; // added a /h or /m or /s to the unit
							}
							if ((length + sms_charCount) >= SMS_MAX_LEN)
							{
								//We have run out of characters for the sms, so send
								//it now
								#ifdef DEBUG
								 sprintf(buffer,"Status Response msg = ");print(buffer);
								print(sms_newMsg.txt);
								sprintf(buffer,", contact=%s\r\n", sms_newMsg.phoneNumber);print(buffer);
							   #endif
								//Push the next send onto the queue
								e.type = sms_STATUS;
								e.param = param;
								queue_push(&q_modem, &e);
								sms_set_header(handleThis);
								
								#ifdef _SMS_DEBUG_
								sprintf(buffer,"12\r\n");print1(buffer);
								#endif
			
								msg_sendData();
								return;
							}
							else
							{
								strcpyre(buffer, config.input[param].alarm[pos].reset_msg);
								strcat(sms_newMsg.txt, buffer);
								if (config.input[param].type != DIGITAL)
								{
									sprintf(buffer," %2.1f", inputVal);
									strcat(sms_newMsg.txt, buffer);
									strcpyre(buffer,config.input[param].units);
									strcat(sms_newMsg.txt, buffer);
									if(config.input[param].type == PULSE)
									{
										switch(config.pulse[param-6].period)
										{
											case SECONDS:
												sprintf(buffer,"/s");
												break;
											case MINUTES:
												sprintf(buffer,"/m");
												break;
											case HOURS:
												sprintf(buffer,"/h");
										}
										strcat(sms_newMsg.txt, buffer);
									}
								}
								sprintf(buffer,"\n");
								strcat(sms_newMsg.txt, buffer);

								sms_charCount += length;
								#ifdef DEBUG
								sprintf(buffer, "sms_charCount=%d\r\n", sms_charCount);print(buffer);
								#endif
							}
						}
					}
				}
				#ifdef DEBUG
				sprintf(buffer,"end of inputs > param=%d\r\n",param);print(buffer);
				#endif
			}
			if (param >= MAX_INPUTS)
			{
				#ifdef DEBUG
				sprintf(buffer,"start of output > i=%d\r\n",param);print(buffer);
				#endif
				for (param -= (MAX_INPUTS); param < MAX_OUTPUTS; param++)
				{
					if(config.output[param].enabled)
					{
						if(config.output[param].state == on)
						{
							length = strlene(config.output[param].config.on_msg) + 1;
							if ((length + sms_charCount) >= SMS_MAX_LEN)
							{
								//We have run out of characters for the sms, so send
								//it now
								#ifdef DEBUG
								sprintf(buffer,"Status Response msg = ");print(buffer);
								print(sms_newMsg.txt);
								sprintf(buffer,", contact=%s\r\n", sms_newMsg.phoneNumber);print(buffer);
								#endif
								//Push the next send onto the queue
								e.type = sms_STATUS;
								e.param = param + MAX_INPUTS;
								queue_push(&q_modem, &e);
								sms_set_header(handleThis);
								
								#ifdef _SMS_DEBUG_
								sprintf(buffer,"13\r\n");print1(buffer);
								#endif
								
								msg_sendData();
								return;
							}
							else
							{
								strcpyre(buffer, config.output[param].config.on_msg);
								strcat(sms_newMsg.txt, buffer);
								sprintf(buffer,"\n");
								strcat(sms_newMsg.txt, buffer);

								sms_charCount += length;
								#ifdef DEBUG
								sprintf(buffer, "sms_charCount=%d\r\n", sms_charCount);print(buffer);
								#endif
							}

						}
						else
						{
							length = strlene(config.output[param].config.off_msg) + 1;
							if ((length + sms_charCount) >= SMS_MAX_LEN)
							{
								//We have run out of characters for the sms, so send
								//it now
								#ifdef DEBUG
								sprintf(buffer,"Status Response msg = ");print(buffer);
								print(sms_newMsg.txt);
								sprintf(buffer,", contact=%s\r\n", sms_newMsg.phoneNumber);print(buffer);
								#endif
								//Push the next send onto the queue
								e.type = sms_STATUS;
								e.param = param + MAX_INPUTS;
								sms_set_header(handleThis);
								queue_push(&q_modem, &e);
								
								#ifdef _SMS_DEBUG_
								sprintf(buffer,"15\r\n");print1(buffer);
								#endif
								
								msg_sendData();
								return;
							}
							else
							{
								strcpyre(buffer, config.output[param].config.off_msg);
								strcat(sms_newMsg.txt, buffer);
								sprintf(buffer,"\n");
								strcat(sms_newMsg.txt, buffer);

								sms_charCount += length;
								#ifdef DEBUG
								sprintf(buffer, "sms_charCount=%d\r\n", sms_charCount);print(buffer);
								#endif
							}

						}
					}
				}
			}
			//Start the send operation
			sms_set_header(handleThis);
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"14\r\n");
			#endif
			
			msg_sendData();
			break;
        //SMS System
        case sms_SYSTEM :		
            sms_newMsg.usePhone = true;
            //Response will be in the form
            //+CSQ: <rssi>,<ber>
            strcpyre(sms_newMsg.txt,config.site_name);
            //sprintf(buffer,"\nrssi: %d\nber: %d",modem_get_rssi(), modem_get_ber());
			sprintf(buffer,"\nrssi: %d",modem_get_rssi());
            strcat(sms_newMsg.txt,buffer);
            sms_set_header(handleThis);	

			#ifdef _SMS_DEBUG_
			sprintf(buffer,"16\r\n");print1(buffer);
			#endif			
			
            msg_sendData();
            break;

        //SMS EDAC Check
        case sms_EDAC :
             sms_newMsg.usePhone = true;
             sprintf(buffer,"rev=");
             strcpy(sms_newMsg.txt, buffer);
             sprintf(buffer,"%p",code_version);
             strcat(sms_newMsg.txt, buffer);
             sprintf(buffer,".%p",buildnumber);
             strcat(sms_newMsg.txt, buffer);
             sprintf(buffer, "\n%02d%02d%02d%02d%02d%02d\n", error.startup.year,
                                                              error.startup.month,
                                                              error.startup.day,
                                                              error.startup.hour,
                                                              error.startup.min,
                                                              error.startup.sec,);
             strcat(sms_newMsg.txt, buffer);
             sprintf(buffer, "q_ev%d\nq_modem%d\npOnR%d\nexR%d\nboR%d\nwdR%d\nuR%d\n",
                        error.q_event_ovf,
                        error.q_modem_ovf,
                        error.powerOnReset,
                        error.externalReset,
                        error.brownOutReset,
                        error.watchdogReset,
                        error.unknownReset);
             strcat(sms_newMsg.txt, buffer);
             sprintf(buffer,"jtR%d\nsrC%d\nsfC%d\nmeC%d\nnfC%d\ntmC%d\nssC%d\n",
                                                                        error.jtagReset,
                                                                        error.emailRetryCounter,
                                                                        error.emailFailCounter,
                                                                        error.modemErrorCounter,
                                                                        error.networkFailureCounter,
                                                                        error.throughModeCounter,
                                                                        error.emailSendCounter);
             strcat(sms_newMsg.txt, buffer);
             //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"17\r\n");print1(buffer);
			#endif
			
             msg_sendData();

             break;

        //SMS Config Confirm
        case sms_CONFIG :
             break;

        //SMS Forward SMS
        case sms_FORWARD :
             break;

        //SMS Input Sensor Failure
        case sms_SENSOR_FAIL :
            sms_newMsg.usePhone = false;
            //Copy the message to be sent
            sprintf(buffer,"Input %d's sensor is reading out of bounds, it might be faulty", param+1);
            strcpy(sms_newMsg.txt, buffer);
            sms_set_header(handleThis);
            //Send this message to the primary user only
            sms_newMsg.contactList  = 0x01;
            #ifdef DEBUG
            sprintf(buffer,"Fail msg = %s, contact=%s\r\n", sms_newMsg.txt, sms_newMsg.contactList);print(buffer);
            #endif
            //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"18\r\n");print1(buffer);
			#endif
			
            msg_sendData();
            break;

        //SMS Check in
        case sms_CHECK_IN  :
             break;

        //SMS UART send SMS
        //unused - at+sms uses msg_sendData, doesn't get queued to here
		/*case sms_UART_SEND :
             //Send the sms that was sent down the uart port to the sms300.
             //this will be in the format at+sms="<phone number>" ,<message>\r\n
             sms_newMsg.usePhone = true;
             //Find the start of the phone number
             pos = strpos(uart_rx_string, '"');
             //Make sure that the char was in the string
             if (pos == 255)
             {
                sprintf(buffer,"ERROR\r\n");
                print(buffer);
                //Free up the uart
                uart_paused = false;
                break;
             }
             substring(uart_rx_string, uart_rx_string, pos+1);
             //find the end of the phone number
             pos = strpos(uart_rx_string, '"');
             //Make sure that the char was in the string
             if (pos == 255 || pos == 0)
             {
                sprintf(buffer,"ERROR\r\n");
                print(buffer);
                //Free up the uart
                uart_paused = false;
                break;
             }
             //copy the phone number
             substr(uart_rx_string, sms_newMsg.phoneNumber, 0, pos);
             //find the start of the message
             pos = strpos(uart_rx_string, ',');
             //Make sure that the char was in the string
             if (pos == 255)
             {
                sprintf(buffer,"ERROR\r\n");
                print(buffer);
                //Free up the uart
                uart_paused = false;
                break;
             }
             //copy the message
             substr(uart_rx_string, sms_newMsg.txt, pos+1,pos+160);
             #ifdef DEBUG
             sprintf(buffer,"UART SEND msg = %s, contact=%s\r\n", sms_newMsg.txt, sms_newMsg.phoneNumber);print(buffer);
             #endif
             //Start the send operation
             msg_sendData();
             //Free up the uart
             uart_paused = false;
             sprintf(buffer,"OK\r\n");print(buffer);
             break;
		*/
			 
        //Get CSQ
        case sms_GET_CSQ  :
            switch (param)
            {
                case CSQ:
                    //sprintf(buffer,"+CSQ=%d,%d\r\n",modem_get_rssi(),modem_get_ber());
					sprintf(buffer,"+CSQ=%d\r\n",modem_get_rssi());
                    print(buffer);
                    break;
                case RSSI:
                    sprintf(buffer,"+RSSI=%d\r\n",modem_get_rssi());
                    print(buffer);
                    break;
                case BER:
                   sprintf(buffer,"+BER=%d\r\n",modem_get_ber());
                    print(buffer);
                    break;
            }
            break;

        //Read Incoming SMS
        //param = index of the SMS to be read in
        case sms_READ_SMS  :
            //Pause this queue, so that nothing is popped off until the send is complete
            //sprintf(buffer,"sms.c - reading sms\r\n");print(buffer);
            #if SMS_AVAILABLE
            if(modem_read_sms(param))
            {
            	#ifdef _SMS_DEBUG_
				sprintf(buffer,"sms.c - sms read - call sms process\r\n");print(buffer);
				#endif
                sms_process();
                //Delete the SMS that was received
                e.type = sms_DELETE;
                e.param = sms_newMsg.index;
                queue_push (&q_modem, &e);
            }
            #endif
            break;
        //The SMS sent didn't have the correct syntax
        case sms_FAIL :
            sms_newMsg.usePhone = true;
            //Copy the message to be sent
            strncpy(buffer,sms_newMsg.txt,100);
            sprintf(sms_newMsg.txt,"The command or pin was invalid: \"%s\"",buffer);
            #ifdef DEBUG
            sprintf(buffer,"Fail msg = ");print(buffer);
            print(sms_newMsg.txt);
            sprintf(buffer,", contact=%s\r\n", sms_newMsg.phoneNumber);print(buffer);
            #endif
            //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"19\r\n");print1(buffer);
			#endif
			
            msg_sendData();
            break;
        case sms_CHECK_MSG :
            DEBUG_printStr("Clean old messages\r\n");
	    /* AT+CMGD=<index>,<delflag>.
	     *
	     * If delflag is 3, all read and stored mobile-originated
	     * messages will be deleted, and index is ignored.
	     */
            printf("AT+CMGD=1,3\r\n");
            modem_wait_for(MSG_OK | MSG_ERROR);

            DEBUG_printStr("SMS Check Message List\r\n");
            printf("AT+CMGL=\"ALL\"\r\n");
            break;
        //param = index of sms  to be deleted
        case sms_DELETE :
            printf("AT+CMGD=%d\r\n",param);
            DEBUG_printStr("SMS Delete Message off SIM\r\n");
            #ifdef DEBUG
            sprintf(buffer,"AT+CMGD=%d\r\n",param);print(buffer);
            #endif
            modem_wait_for(MSG_OK | MSG_ERROR);
            break;
        case sms_DELETE_SENT :
            printf("AT+CMGD=1,2\r\n",param);
            DEBUG_printStr("SMS Delete SENT Messages off SIM\r\n");
            #ifdef DEBUG
            sprintf(buffer,"AT+CMGD=1,2\r\n",param);print(buffer);
            #endif
            modem_wait_for(MSG_OK | MSG_ERROR);
            break;
        case sms_CHECK_NETWORK:
            modem_register=true;
            modem_clear_channel();
            DEBUG_printStr("SMS: Check the network registration - ");
CREG_AGAIN:
            DEBUG_printStr("requesting AT+CREG?\r\n");
            modem_get_rssi();
            //modem_get_ber();
            printf("AT+CREG?\r\n");
            #if MODEM_TYPE == Q24NG_PLUS
            do
            {
                modem_read();
                #ifdef DEBUG
                printStr("|");
                print(modem_rx_string);
                printStr("|\r\n");
                #endif
                if(strstrf(modem_rx_string,"+WIND"))
                {
                    printf("ATH\r\n");
                    goto CREG_AGAIN;
                }
            } while(!(strstrf(modem_rx_string,"ERROR")||
                      strstrf(modem_rx_string,"+CREG:")));

            #else
            modem_wait_for(MSG_CREG | MSG_ERROR);
            #endif
            if (strstrf(modem_rx_string, "+CREG") != 0)
            {
                //Break up the response to check the status of the registration
//                #ifdef _MODEM_DEBUG_
//        		sprintf(buffer, ">+CREG RESPONSE: (");
//        		print(buffer);
//        		print(modem_rx_string);
//        		sprintf(buffer,")\r\n");
//        		print(buffer);
//        		#endif
        		//See what the status of the CREG command.
        		if (strcmpf (modem_rx_string, "+CREG: 1") == 0 || strcmpf (modem_rx_string, "+CREG:1") == 0)
        		{
        		    DEBUG_printStr("Modem Registered again\r\n");
        		    sprintf(buffer,"\r\n+CNI=OK\r\n");print(buffer);
        		    break;
        		}
        		else if (strcmpf (modem_rx_string, "+CREG: 0") == 0 || strcmpf (modem_rx_string, "+CREG: 2") == 0 ||
        		         strcmpf (modem_rx_string, "+CREG:0") == 0 || strcmpf (modem_rx_string, "+CREG:2") == 0)
        		{
        		    modem_state = NO_CONNECTION;
        		    DEBUG_printStr("Modem Lost the network\r\n");
        		    sprintf(buffer,"\r\n+CNI=failed\r\n");print(buffer);
                    #if SYSTEM_LOGGING_ENABLED
        		    sprintf(buffer,"Network failure");log_line("system.log",buffer);
        		    #endif
        		    if (error.networkFailureCounter != 0xFFFF)
                        error.networkFailureCounter++;
        		    break;
        		}
         	    else if (strstrf (modem_rx_string, "1,1") != 0 || strstrf (modem_rx_string, "1,5") != 0)
        		{
//            		#ifdef _MODEM_DEBUG_
//            		sprintf(buffer, "Modem registered OK\r\n");
//            		print(buffer);
//         		    #endif
         		    startupError = false;
         		    sprintf(buffer,"\r\n+CNI=OK\r\n");print(buffer);
         	    }
         	    else if (strstrf (modem_rx_string, "0,1") != 0 || strstrf (modem_rx_string, "1,2") != 0 ||
         	             strstrf (modem_rx_string, "0,5") != 0)
         	    {
         	        DEBUG_printStr("Modem can now register on the network\r\n");
         	        #ifdef _MODEM_ON_
                    active_led_on();
                    //Init the Wavecom Integra M2106B Modem.
                    if (!modem_init(build_type)) {
                    	sprintf(buffer,"Modem was not able to initialise\r\n\r\n");
                        print(buffer);
                    	#ifdef _DEBUG_
                    	sprintf(buffer, ">DEBUG: Modem did not initialise properly\r\n");
                    	print(buffer);
                    	#endif
                    	active_led_on();
                    	while(startupError++ < 20)
                    	{
                    	    power_led_off();
                    	    delay_ms(200);
                    	    power_led_on();
                    	    delay_ms(200);
                    	}
                    	startupError = true;
                    	//Turn off the modem echo
                    	//printf("ATE0\r\n");
                       // modem_wait_for(MSG_OK | MSG_ERROR);
                    	sprintf(buffer,"\r\n+CNI=failed\r\n");print(buffer);
                    }
                    else {
                    	#ifdef _DEBUG_
                    	sprintf(buffer, ">DEBUG: Modem init complete\r\n");
                    	print(buffer);
                    	#endif
                    	startupError = false;
                    	sprintf(buffer,"\r\n+CNI=OK\r\n");print(buffer);
                    }
                    #endif
                    active_led_off();
                    break;
                }
         	    else
        		{
            		sprintf(buffer, "The Modem could not register on a network\r\n");
            		print(buffer);
            		startupError = true;
            		if (error.networkFailureCounter != 0xFFFF)
                        error.networkFailureCounter++;
         	        sprintf(buffer,"\r\n+CNI=OK\r\n");print(buffer);
         	    }
         	}
         	else
         	{
         	    sprintf(buffer,"\r\n+CNI=failed\r\n");print(buffer);
         	}
            modem_register=false;
            break;
        case sms_LOG_UPDATE:
            #if EMAIL_AVAILABLE && LOGGING_AVAILABLE	
			tickleRover();
			
			if(param==0) //modified in 3.xx.7 changing from email_log() to email_latest()
						 //regular log update - remove latest.csv after successful send						 
			{	
				#ifdef EMAIL_DEBUG
				rtc_get_time(&i,&j,&param);
				//sprintf(buffer,";s;%02d:%02d:%02d;",i,j,param);
				sprintf(buffer,"Log update send started @ %02d:%02d:%02d\r\n",i,j,param);				
				print1(buffer);
				#endif
			
				email_retry_timer=-1;
			
				//sprintf(sms_dates,"=latest-end");
				//if(cmdDates_to_mmcDates(sms_dates))
				//{		
					//concat_0();
					//concat_1();
					
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_instant_pause = 1;
					}
					#endif
					
					i=1;
					//if(!email_log())
					if(!email_latest(&i))
					{
						error.emailRetryCounter++;
						error.emailFailCounter++;
						//if email fails to send set the retry timer						
						email_retry_timer=email_retry_period;
						#ifdef EMAIL_DEBUG
						sprintf(buffer,"Email retry timer set to %d mins\r\n",email_retry_timer);
						print1(buffer);
						#endif
					}
					else
					{
						error.emailSendCounter++;
					}
					
					//modem_check_status();
					
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_restart();	//restart pulse inputs 7 and 8
						pulse_instant_pause = 0;
					}
					#endif
				//}			
				
				email_log_event_on_queue = false;
				#ifdef EMAIL_DEBUG
				rtc_get_time(&i,&j,&param);
				//sprintf(buffer,";e;%02d:%02d:%02d;\r\n",i,j,param);
				sprintf(buffer,"Log update send completed @ %02d:%02d:%02d\r\n",i,j,param);
				print1(buffer);
				#endif				
			}
			
			else if(param==1) //log.csv
			{
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"[sms_LOG_UPDATE] SMS request for log\r\n");
				print1(buffer);
				sprintf(buffer,"[sms_LOG_UPDATE] sms_dates: %s\r\n",sms_dates);
				print1(buffer);
				#endif	
				
				if(cmdDates_to_mmcDates(sms_dates))
				{		
					concat_0();		//file select = 0 = log.csv
					//concat_0();	//save date and time to hidden file flag - now redundant 3.xx.7
					
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_instant_pause = 1;
						//EIMSK &= ~(0xC0);	//disable pulse interrupts
						//EIFR &= ~(0xC0);	//clear pulse interrupt flags
					}
					#endif
					
					if(!email_log())
					{
						#ifdef EMAIL_DEBUG
						sprintf(buffer,"[sms_LOG_UPDATE] Email send failedr\n");
						print1(buffer);
						#endif	
						error.emailFailCounter++;
						//send sms - email send failed, retry later
						
						sms_newMsg.usePhone = true;
						sprintf(sms_newMsg.txt,"The email failed to send due to a network error\nPlease try again later");
						
						#ifdef _SMS_DEBUG_
						sprintf(buffer,"20\r\n");print1(buffer);
						#endif
						
						//modem_check_status();
						
						msg_sendData();
					}
					else
					{
						#ifdef EMAIL_DEBUG
						sprintf(buffer,"[sms_LOG_UPDATE] Email sent successfully\n");
						print1(buffer);
						#endif
						error.emailSendCounter++;
						
						//modem_check_status();
					}
					
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_restart();	//restart pulse inputs 7 and 8
						pulse_instant_pause = 0;
					}
					#endif
				}
			}
			
			else if(param==2) //system log
			{
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"[sms_LOG_UPDATE] SMS request for system log\r\n");
				print1(buffer);
				sprintf(buffer,"[sms_LOG_UPDATE] sms_dates: %s\r\n",sms_dates);
				print1(buffer);
				#endif
				
				if(cmdDates_to_mmcDates(sms_dates))
				{
					concat_1();		//file select = 1 = system.log
					
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_instant_pause = 1;
						//EIMSK &= ~(0xC0);	//disable pulse interrupts
						//EIFR &= ~(0xC0);	//clear pulse interrupt flags
					}
					#endif
					
					if(!email_log())
					{
						error.emailFailCounter++;
						
						sms_newMsg.usePhone = true;
						sprintf(sms_newMsg.txt,"The email failed to send due to a network error\nPlease try again later");
						
						#ifdef _SMS_DEBUG_
						sprintf(buffer,"21\r\n");print1(buffer);
						#endif
			
						//modem_check_status();
						
						msg_sendData();
					}
					else
					{
						error.emailSendCounter++;
						//modem_check_status();
					}
					
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_restart();	//restart pulse inputs 7 and 8
						pulse_instant_pause = 0;
					}
					#endif
				}
			}
			
			else if(param==3) //event log
			{
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"[sms_LOG_UPDATE] SMS request for event log\r\n");
				print1(buffer);
				sprintf(buffer,"[sms_LOG_UPDATE] sms_dates: %s\r\n",sms_dates);
				print1(buffer);
				#endif
				
				if(cmdDates_to_mmcDates(sms_dates))
				{	
					concat_2();		//file select = 2 = alarm.log
					
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_instant_pause = 1;
						//EIMSK &= ~(0xC0);	//disable pulse interrupts
						//EIFR &= ~(0xC0);	//clear pulse interrupt flags
					}
					#endif
					
					if(!email_log())
					{
						error.emailFailCounter++;
						
						sms_newMsg.usePhone = true;
						sprintf(sms_newMsg.txt,"The email failed to send due to a network error\nPlease try again later");
						
						#ifdef _SMS_DEBUG_
						sprintf(buffer,"22\r\n");print1(buffer);
						#endif
			
						msg_sendData();
					}
					else
					{
						error.emailSendCounter++;
					}
					
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_restart();	//restart pulse inputs 7 and 8
						pulse_instant_pause = 0;
					}
					#endif
				}
			}
			
			else if(param==4) //added 3.xx.7 - email latest.csv - request from SMS command - do not delete file
			{
				#ifdef EMAIL_DEBUG
				rtc_get_time(&i,&j,&param);
				//sprintf(buffer,";s;%02d:%02d:%02d;",i,j,param);
				sprintf(buffer,"SMS request for latest.csv send started @ %02d:%02d:%02d\r\n",i,j,param);				
				print1(buffer);
				#endif
				
				#if PULSE_COUNTING_AVAILABLE
				if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
				{
					pulse_instant_pause = 1;
					//EIMSK &= ~(0xC0);	//disable pulse interrupts
					//EIFR &= ~(0xC0);	//clear pulse interrupt flags
				}
				#endif
				
				i=0;
				if(!email_latest(&i))
				{
					error.emailFailCounter++;
					//send sms - email send failed, retry later
					
					sms_newMsg.usePhone = true;
					sprintf(sms_newMsg.txt,"The email failed to send due to a network error\nPlease try again later");
					
					#ifdef _SMS_DEBUG_
					sprintf(buffer,"23\r\n");print1(buffer);
					#endif
					
					//modem_check_status();
					
					msg_sendData();
				}
				else
				{
					error.emailSendCounter++;
					//modem_check_status();
				}				
				
				#if PULSE_COUNTING_AVAILABLE
				if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
				{
					pulse_restart();	//restart pulse inputs 7 and 8
					pulse_instant_pause = 0;
				}
				#endif
				
				#ifdef EMAIL_DEBUG
				rtc_get_time(&i,&j,&param);
				//sprintf(buffer,";e;%02d:%02d:%02d;\r\n",i,j,param);
				sprintf(buffer,"SMS request for latest.csv send completed @ %02d:%02d:%02d\r\n",i,j,param);
				print1(buffer);
				#endif
			}
			
			sprintf(buffer,"AT\r\n");
			print0(buffer);
			modem_wait_for(MSG_OK | MSG_ERROR);
			
			#ifdef EMAIL_DEBUG
			sprintf(buffer,"[sms_LOG_UPDATE] complete\n");
			print1(buffer);
			#endif
						
			#endif
            break;
        case sms_CHECK_NETWORK_INTERNAL:
            modem_register = true;
            active_led_on();
            modem_clear_channel();
            DEBUG_printStr("SMS: Check the network registration - ");
            for(j=0;j<1;j++)
            {
                tickleRover();
                #if MODEM_TYPE == Q2406B && TCPIP_AVAILABLE
                DEBUG_printStr("Sending AT#VSTATE...");
                //printf("\r\nAT#VSTATE\r\n");
                printf("AT#VSTATE\r\n");
                modem_wait_for(MSG_ERROR | MSG_STATE);

                #ifdef DEBUG
                DEBUG_printStr("\r\nComparing against <");
                print(modem_rx_string);
                DEBUG_printStr(">\r\n");
                #endif
                for(i=0;i<9;i++)
                {
                    if(strstrf(modem_rx_string,vstate_values[i])!=0)
                        break;
                }
                modem_rx_string[0] = 0;

                switch(i)
                {
                    case NO_SERVICE:
                        DEBUG_printStr("Modem disconnected\r\n");
                        if(!modem_init(build_type))
                        {
                            DEBUG_printStr("Modem failed to init correctly\r\n");
                            break;
                        }
                    case IDLE:                    
					#if TCPIP_AVAILABLE
                        DEBUG_printStr("No GPRS session\r\n");
                        //gprs_startConnection();
                        delay_ms(1000);
                    #endif					
                        startupError = false;
                        break;
                    case CONNECTED:
                        startupError = false;
                        DEBUG_printStr("Modem Connected!\r\n");
                        modem_state = GPRS_CONNECTION;
                        break;
                    case VSTATE_ERROR:
                        printStr("WARNING: Modem is not capable of a GPRS connection.\r\n");
                        printf("AT#VSTATE\r\n");
                        break;
                    case DIALING:
                    case AUTHENTICATING:
                    case CHECKING:
                    case GENERAL_ERROR:
                    default:
                        break;

                }
                if(i==CONNECTED)
                    break;
                #else
CREG_AGAIN2:
                DEBUG_printStr("requesting AT+CREG?\r\n");
                printf("AT+CREG?\r\n");
                #if MODEM_TYPE == Q24NG_PLUS
                do
                {
                    modem_read();
                    #ifdef DEBUG
                    printStr("|");
                    print(modem_rx_string);
                    printStr("|\r\n");
                    #endif
                    if(strstrf(modem_rx_string,"+WIND"))
                    {
                        printf("ATH\r\n");
                        goto CREG_AGAIN;
                    }
                } while(!(strstrf(modem_rx_string,"ERROR")||
                          strstrf(modem_rx_string,"+CREG:")));
                #else
                modem_wait_for(MSG_CREG | MSG_ERROR);
                #endif
                if(strstrf(modem_rx_string,"ERROR"))
                {
                    modem_state = NO_CONNECTION;
                    DEBUG_printStr("Error getting state\r\n");
                }
                else if(strstrf(modem_rx_string,",1") ||
                        strstrf(modem_rx_string,",5"))
                {
                    //modem_state = (CELL_CONNECTION>modem_state?CELL_CONNECTION:modem_state);
					modem_state = CELL_CONNECTION;					
                    //DEBUG_printStr("Connection established\r\n");
					//sprintf(buffer,"Network connection ok - ");print1(buffer);
					//sprintf(buffer,"RSSI = %d\r\n",modem_get_rssi());print1(buffer);
                    if(startupError)
                    {
                        if(!modem_init(build_type))
                        {
                            DEBUG_printStr("Modem failed to init correctly\r\n");
                            startupError = true;
                            break;
                        }
                    }
                    startupError = false;
                }
                else
                {
                    DEBUG_printStr("Modem disconnected\r\n");
                    if(!modem_init(build_type))
                    {
                        DEBUG_printStr("Modem failed to init correctly\r\n");
                        break;
                    }
                }
                #endif
            }
            #if MODEM_TYPE == Q24NG_PLUS && TCPIP_AVAILABLE
            if(modem_state != GPRS_CONNECTION)
                gprs_startConnection();
            #endif
            active_led_off();
            modem_register = false;
            queue_resume(&q_modem);
            break;
        case sms_OUT_FAIL:
        case sms_REMOTE_OUT_FAIL:
            DEBUG_printStr("Output switch has failed, send to primary user\r\n");
            //Send this message to the primary user only
            sms_newMsg.contactList  = 0x01;
            //Copy the message to be sent
            sprintf(buffer,"Output %d is not enabled.", param+1);
            strcpy(sms_newMsg.txt, buffer);
            sms_set_header(handleThis);
            #ifdef DEBUG
            sprintf(buffer,"Fail msg = %s, contact=%d\r\n", sms_newMsg.txt, sms_newMsg.contactList);print(buffer);
            #endif
            //Start the send operation
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"24\r\n");print1(buffer);
			#endif
			
            msg_sendData();
            break;
        case sms_GPRS_CHECK:
            //Try to re-establish a GPRS Connection
            //#if TCPIP_AVAILABLE
            //active_led_on();
            //if (!gprs_startConnection())
            //{
            //    DEBUG_printStr("GPRS wouldn't start\r\n");
            //}
            //else
            //{
            //    DEBUG_printStr("GPRS Connection OK\r\n");
            //}
            //active_led_off();
            //#endif
            break;
        case sms_GPS_REQUEST:
            //location requested
            //#if GPS_AVAILABLE
            //gps_print(sms_newMsg.txt,DEGREES);
            //*(strchr(sms_newMsg.txt,',')) = '\n'; //break the comma with a newline
            //strcatf(sms_newMsg.txt,"\n");
            //gps_velocity(buffer);
            //strcat(sms_newMsg.txt,buffer);
            //sms_newMsg.usePhone = true;
            //msg_sendData();
            //#endif
            break;
        case sms_CONTACT_CHANGE:
            #ifdef _SMS_DEBUG_
            sprintf(buffer,"contact change, working with [%s]\r\n",sms_newMsg.txt);
            print(buffer);
            #endif
            
			if (strstrf(sms_newMsg.txt,"contacts?")!=0)
			{			
				j=0;
				param=0;				
				do
				{
					i=0;
					temp=0;
					#ifdef _SMS_DEBUG_
					sprintf(buffer,"j = %d\r\n",j);print1(buffer);
					#endif
					temp = contact_read(j);
					strcpyre(buffer,temp+1);					
					i=strlen(buffer);
					if (!temp || i==0)
					{
						sprintf(buffer,"Not Set");
					}					
					i=strlen(buffer);
					#ifdef _SMS_DEBUG_
					//print1(buffer);
					#endif					
					
					if (j==0)
					{
						strcpyre(modem_rx_string, config.site_name);
						sprintf(sms_newMsg.txt,"Contact list for %s\n",modem_rx_string);
						#ifdef _SMS_DEBUG_
						//print1(sms_newMsg.txt);					
						#endif
						param=1;
					}
					
					if ( ((strlen(sms_newMsg.txt)+i+26) > SMS_MAX_LEN ) && j>0)
					{				
						sms_newMsg.usePhone = true;
						
						#ifdef _SMS_DEBUG_
						sprintf(buffer,"25\r\n");print1(buffer);
						#endif
			
						msg_sendData();
						
						sprintf(sms_newMsg.txt,"");
						j--;
						param=1;
					}
					else
					{
						if(param)
						{
							sprintf(modem_rx_string,"%d = %s",j+1,buffer);
							strcat(sms_newMsg.txt,modem_rx_string);
							param=0;
						}
						else
						{
							sprintf(modem_rx_string,"\n%d = %s",j+1,buffer);
							strcat(sms_newMsg.txt,modem_rx_string);
						}
						
						//if(j==15)
						//{							
						//	sms_newMsg.usePhone = true;
						//	msg_sendData();
						//}
						
					}
					#ifdef _SMS_DEBUG_
					print1(sms_newMsg.txt);					
					#endif
					j++;
				}
				while(j<16);
				
				sms_newMsg.usePhone = true;
				
				#ifdef _SMS_DEBUG_
				sprintf(buffer,"26\r\n");print1(buffer);
				#endif
			
				msg_sendData();		
			
			}
			
			else
			{
				c = strstrf(sms_newMsg.txt,"contact")+7;
				j = atoi(c);
			
				if(strstrf(sms_newMsg.txt,"=")!=0)
				{				
					c = strchr(sms_newMsg.txt,'=')+1;

					#ifdef _SMS_DEBUG_
					sprintf(buffer,"modifying %d to [%s]\r\n",j,c);
					print(buffer);
					#endif
					
					j--;
					if(j<0 || j>15)
						sprintf(buffer,"Contact %d is an invalid number",j+1);
					else
					{
						/*
						i = contact_modify(c,j);

						if(i==j)
						{
							sprintf(buffer,"Contact %d changed to \"%s\"",j+1,c);
						}
						else if(i == -1)
						{
							sprintf(buffer,"Contact change failed",j,c);
						} 
						else
						{
							sprintf(buffer,"Contact \"%s\" added in spot %d",c,j+1);
						}
						*/						
						
						if(j!=0)
						{
							do
							{
								temp = contact_read(j-1);
								strcpyre(buffer,temp+1);					
								i=strlen(buffer);
								if (!temp || i==0)
								{
									#ifdef _SMS_DEBUG_
									sprintf(buffer,"Contact %d is empty, decrementing contact",j-1);
									print1(buffer);
									#endif
									j--;
								}
								else
								{
									#ifdef _SMS_DEBUG_
									sprintf(buffer,"Contact %d is occupied, saving as contact %d",j-1,j);
									print1(buffer);
									#endif
									break;
								}							
							}
							while(j>0);
						}
						
						//i = contact_write(c);
						i = contact_modify(c,j);
						
						if(i==j)
						{							
							sprintf(buffer,"Contact %d changed to \"%s\"",j+1,c);
							#ifdef _SMS_DEBUG_
							print1(buffer);
							#endif
						}
						else if(i == -1)
						{
							sprintf(buffer,"Contact change failed");
							#ifdef _SMS_DEBUG_
							print1(buffer);
							#endif
						} 
						else // doesn't do anything
						{
							sprintf(buffer,"Contact \"%s\" added in position %d",c,j+1);
							#ifdef _SMS_DEBUG_
							print1(buffer);
							#endif
						}					
						
						
						
						
					}
					strcpy(sms_newMsg.txt,buffer);
					sms_newMsg.usePhone = true;
					
					#ifdef _SMS_DEBUG_
					sprintf(buffer,"27\r\n");print1(buffer);
					#endif
					
					msg_sendData();
				}
				
				else if(strstrf(sms_newMsg.txt,"?")!=0)
				{
					j--;
					if(j>15)
						sprintf(sms_newMsg.txt,"Contact %d is an invalid number",j+1);
					else
					{
						temp = contact_read(j);						
						strcpyre(buffer,temp+1);
						sprintf(sms_newMsg.txt,"Contact %d = %s",j+1,buffer);					
					}
					sms_newMsg.usePhone = true;
					
					#ifdef _SMS_DEBUG_
					sprintf(buffer,"28\r\n");print1(buffer);
					#endif
					
					msg_sendData();
				}
			}
			
            break;
			
		case sms_EMAIL_CHANGE:
			c = strchr(sms_newMsg.txt,'=')+1;
			strcpye(config.update_address,c);
			strcpyre(buffer,config.update_address);
			sprintf(sms_newMsg.txt,"Log email address set to: %s\r\n",buffer);			
			
			#ifdef EMAIL_DEBUG			
			print1(buffer);
			sprintf(buffer,"\r\n");
			print1(buffer);
			#endif
			
			sms_newMsg.usePhone = true;
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"29\r\n");print1(buffer);
			#endif
			
			msg_sendData();
			
			break;
		
        case sms_MMC_ACCESS_FAIL:
            if(mmc_fail_sent)
            {
                DEBUG_printStr("MMC fail message already sent\r\n");
                break;
            }
            else
            {
                mmc_fail_sent=true;
                DEBUG_printStr("Sending MMC fail message\r\n");
            }
            #ifdef _MODEM_ON_
            sms_newMsg.usePhone = false;
            //Copy the message to be sent
            strcpyre(sms_newMsg.txt,config.site_name);
            sprintf(buffer,"\nWARNING: Unable to access the data card.");
            strcat(sms_newMsg.txt, buffer);
            sms_set_header(handleThis);
            //Send this message to the primary user only
            sms_newMsg.contactList  = 0x01;
            #ifdef DEBUG
            sprintf(buffer,"Fail msg = ");print(buffer);
            print(sms_newMsg.txt);
            sprintf(buffer,", contact=%s\r\n", sms_newMsg.contactList);print(buffer);
            #endif
            //Start the send operation
            
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"230\r\n");print1(buffer);
			#endif
			
			msg_sendData();
			
            #endif
            break;
		
		case sms_EMAILRETRYTIMER:
			sms_newMsg.usePhone=true;
			
			if(strstrf(sms_newMsg.txt,"=")!=0)
			{
				c = strchr(sms_newMsg.txt,'=')+1;
				email_retry_period = atoi(c);
				
				if(email_retry_period<=0)
				{
					email_retry_period=-1;
					sprintf(sms_newMsg.txt,"The timer is disabled\r\n");
				}
				else
				{
					sprintf(sms_newMsg.txt,"The timer is set to %d minutes\r\n",email_retry_period);
				}
				
				#ifdef _SMS_DEBUG_
				sprintf(buffer,"31\r\n");print1(buffer);
				#endif
				
				msg_sendData();
				
												
			}
			else if(strstrf(sms_newMsg.txt,"?")!=0)
			{
				if(email_retry_period==-1)
				{
					sprintf(sms_newMsg.txt,"The timer is disabled\r\n");
				}
				else
				{
					sprintf(sms_newMsg.txt,"The timer is set to %d minutes\r\n",email_retry_period);
				}
				
				#ifdef _SMS_DEBUG_
				sprintf(buffer,"32\r\n");print1(buffer);
				#endif
			
				msg_sendData();
			}
			else
			{
				e.type = sms_FAIL;
                queue_push(&q_modem, &e);
			}
			break;
		
		case sms_UNITINFO:			
			sms_newMsg.usePhone=true;						
			strcpyre(buffer,serial);			
			if (PRODUCT==EDAC315)
			{
				sprintf(sms_newMsg.txt,"EDAC315v%d 053-0030 Rev:%p.%p\nSerial: %s\n",HW_REV,code_version,buildnumber,buffer);
			}
			else if (PRODUCT==EDAC320)
			{
				sprintf(sms_newMsg.txt,"EDAC320-NGv%d 053-0032 Rev:%p.%p\nSerial: %s\n",HW_REV,code_version,buildnumber,buffer);
			}
			else if (PRODUCT==EDAC321)
			{
				sprintf(sms_newMsg.txt,"EDAC321-NGv%d 053-0034 Rev:%p.%p\nSerial: %s\n",HW_REV,code_version,buildnumber,buffer);
			}
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"33\r\n");print1(buffer);
			#endif
			
			msg_sendData();			
			break;
		
		case sms_PIN_CHANGE:
			c = strstrf(sms_newMsg.txt,"new pin=")+8;
			strcpye(config.pin_code,c);			
			#ifdef ADMIN_DEBUG
			sprintf(buffer,"PIN changed to: ");print1(buffer);
			strcpyre(buffer,config.pin_code);print1(buffer);
			sprintf(buffer,"\r\n");print1(buffer);
			#endif			
			break;
        
		case sms_EMAIL_ERROR:
			#if EMAIL_AVAILABLE
			if(cmdDates_to_mmcDates(sms_dates))
			{
				#if PULSE_COUNTING_AVAILABLE
				if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
				{
					pulse_instant_pause = 1;
					//EIMSK &= ~(0xC0);	//disable pulse interrupts
					//EIFR &= ~(0xC0);	//clear pulse interrupt flags
				}
				#endif
				
				if(!email_error())
				{
					error.emailFailCounter++;
					
					sms_newMsg.usePhone = true;
					sprintf(sms_newMsg.txt,"The email failed to send due to a network error\nPlease try again later");
					
					#ifdef _SMS_DEBUG_
					sprintf(buffer,"34\r\n");print1(buffer);
					#endif
					
					//modem_check_status();
					
					msg_sendData();
				}
				else
				{
					error.emailSendCounter++;
					//modem_check_status();
				}
				
				#if PULSE_COUNTING_AVAILABLE
				if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
				{
					pulse_restart();	//restart pulse inputs 7 and 8
					pulse_instant_pause = 0;
				}
				#endif
			}
			#endif
			break;
			
		// case sms_CHECK_NETWORK_STATUS:
			// sprintf(buffer,"Execute check status\r\n");print1(buffer);
			// modem_check_status();
			// sprintf(buffer,"Set flag to false\r\n");print1(buffer);
			// check_status_event_on_queue=false;
			// break;
		
		default:
            DEBUG_printStr("Unknown Event\r\n");
            break;
    }

    return;
}

void msg_done()
{
    #ifdef DEBUG
    sprintf(buffer,"SMS Send Complete\r\n");print(buffer);
    sprintf(buffer,"Clear modem reset timer\r\n");print(buffer);
    #endif

   //You have succesfull send all of the messages required so return
   //control to the sms queue
    msg_index=0;
    queue_resume (&q_modem);
    active_led_off();
    msg_send = false;
}

void sms_set_header(Event *e)
{
    //print out the date and time.
    sprintf(buffer,"\n%02d:%02d ",e->hour,e->minute);
    strcat(sms_newMsg.txt,buffer);
    sprintf(buffer,"%02d/%02d/%02d",e->day,e->month,e->year);
    strcat(sms_newMsg.txt,buffer);
}

//Send message Routine
void msg_sendData()
{
    char i, int1, int2;
	bit first_email_contact = 0;
	
	tickleRover();
	#if defined _SMS_DEBUG_ || defined EMAIL_DEBUG
	rtc_get_time(&i,&int1,&int2);
	sprintf(buffer,"Send started @ %02d:%02d:%02d\r\n",i,int1,int2);
	print1(buffer);
	tickleRover();
	#endif
	
    if(sms_newMsg.usePhone)
    {
        if(strchr(sms_newMsg.phoneNumber,'@'))
        {
            #if EMAIL_AVAILABLE
			#if PULSE_COUNTING_AVAILABLE
			if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
			{
				pulse_instant_pause = 1;
				//EIMSK &= ~(0xC0);	//disable pulse interrupts
				//EIFR &= ~(0xC0);	//clear pulse interrupt flags
			}
			#endif
			if(!email_send())
			{
				error.emailFailCounter++;
			}
			else
			{
				error.emailSendCounter++;
			}
			#if PULSE_COUNTING_AVAILABLE
			if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
			{
				pulse_restart();	//restart pulse inputs 7 and 8
				pulse_instant_pause = 0;
			}
			#endif	
			sprintf(buffer,"AT\r\n");
			print0(buffer);
			modem_wait_for(MSG_OK | MSG_ERROR);	
			//modem_check_status();
            #endif				
        }
        else
        {
            #if SMS_AVAILABLE
			modem_send_sms(sms_newMsg.txt,sms_newMsg.phoneNumber);
            #endif
        }
    }
	else
    {
        for(i=0;i<MAX_CONTACTS;i++)
        {
			//sprintf(buffer,"contact=%d, ",i);print1(buffer);
            if(((unsigned int)0x01)<<i & sms_newMsg.contactList)
            {
                strcpyre(buffer,contact_read(i));
                int1 = strpos(buffer, '<');
                int2 = strpos(buffer, '>');
                if (int1 == int2)
                {
                    //There are no chevrons around the phone number, therefore continue.
                    strcpy(sms_newMsg.phoneNumber, buffer);
                }
                else
                {
                    substr(buffer, sms_newMsg.phoneNumber, int1+1, int2);
                    #ifdef DEBUG
                    sprintf(buffer, " Phone Number: \"%s\"\r\n", sms_newMsg.phoneNumber);print(buffer);
                    #endif
                }
                if(contact_getType(i)==SMS)
                {
                    DEBUG_printStr("Phone number, so sending as SMS\r\n");
					//sprintf(buffer,"SMS\r\n");print1(buffer);
                    #if SMS_AVAILABLE
                    modem_send_sms(sms_newMsg.txt,sms_newMsg.phoneNumber);
                    #endif
                }
                /* else
                {
                    #if EMAIL_AVAILABLE					
					if(!email_send())
					{
						error.emailFailCounter++;
					}
					else
					{
						error.emailSendCounter++;
					}
                    #endif
				}
				*/
				else if (first_email_contact == 0)
				{	
					//sprintf(buffer,"first email contact\r\n");print1(buffer);
					first_email_contact = 1;
					#if EMAIL_AVAILABLE
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_instant_pause = 1;
						//EIMSK &= ~(0xC0);	//disable pulse interrupts
						//EIFR &= ~(0xC0);	//clear pulse interrupt flags
					}
					#endif
					if(!email_send())	//send a single email to all applicable contacts
					{
						error.emailFailCounter++;
					}
					else
					{
						error.emailSendCounter++;
					}
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						pulse_restart();	//restart pulse inputs 7 and 8
						pulse_instant_pause = 0;
					}
					#endif	
					sprintf(buffer,"AT\r\n");
					print0(buffer);
					modem_wait_for(MSG_OK | MSG_ERROR);
					//modem_check_status();
                    #endif					
				}
            }
        }
    }
	
	tickleRover();
	#if defined _SMS_DEBUG_ || defined EMAIL_DEBUG
	rtc_get_time(&i,&int1,&int2);
	sprintf(buffer,"Send completed @ %02d:%02d:%02d\r\n",i,int1,int2);
	print1(buffer);
	tickleRover();
	#endif
	
    msg_done();

    return;
}

//read an sms from the modem routine
void sms_process()
{
    unsigned char i,j;
    Event e;
    bool last_command = false;
    char temp[32];
    char *c;	//, *cmd;
    signed char p1=0, p2=0, p3=0;
    char action[32];
    char output[32];
    char time[16];
	bit admin_pin = false;
     //make all lower case
    for(i=0;sms_newMsg.txt[i];i++) {
        if(sms_newMsg.txt[i] == '<')
            break; //message to be passed on, we don't want to lowercase it
        sms_newMsg.txt[i]=tolower(sms_newMsg.txt[i]);
    }

    #ifdef _SMS_DEBUG_
    sprintf(buffer,">SMS: tolower (");
    print(buffer);
    print(sms_newMsg.txt);
    sprintf(buffer,")\r\n");
    print(buffer);
    #endif

 	//sprintf(buffer,"sms.c -  convert to lowercase (");print(buffer);
    //print(sms_newMsg.txt);sprintf(buffer,")\r\n");print(buffer);

    /* Extract *<embedded> commands in message */
    sub_magic(sms_newMsg.txt);

    //Copy newMsg into modem_rx_string, to use it as a buffer, so you can break
    //out multiple commands.
    strcpy(modem_rx_string, sms_newMsg.txt);

	#ifdef _SMS_DEBUG_
    sprintf(buffer,"modem_rx_string = %s\r\n",modem_rx_string);print1(buffer);	//SJL - CAVR2 - Debug
	#endif

    /* Ignore anything with an EDAC 300-series time/date suffix */
    if (has_dt_suffix(modem_rx_string))
	return;

    //message recieved from remote unit saying a message was formatted wrong or an output was already turned on.
    //this breaks the chain
    if(strstrf(modem_rx_string,"command or pin was invalid")||
       strstrf(modem_rx_string,"already turned"))
        return;

    do
    {
        tickleRover();
		
		#ifdef _SMS_DEBUG_
		sprintf(buffer,"Processing command\r\n");print1(buffer);	//SJL - CAVR2 - Debug
		#endif
		
        //sprintf(buffer,"modem_rx_string = %s\r\n",modem_rx_string);print(buffer);

        //First see if there is more than one command sent by the user.
        //if (strpos(modem_rx_string, '+') != 255) 	// SJL - CAVR2 - Debug
        if (strpos(modem_rx_string, '+') != 255 && strpos(modem_rx_string, '+') !=-1)
        {
            DEBUG_printStr("More than one command in the SMS string\r\n");
            //Need to adjust the string, to only have one command at a time.
            substr(modem_rx_string, sms_newMsg.txt, 0, strpos(modem_rx_string, '+'));
            //Crop modem_rx_string,
            substring (modem_rx_string, modem_rx_string, strpos(modem_rx_string, '+') + 1);
        }
        else
        {
			if (strncmpf(modem_rx_string,"45444143",8) == 0 )	//send only one admin cmd at a time
			{			
				admin_pin = true;
				
				c = strchr(modem_rx_string,' ')+1;
				sprintf(sms_newMsg.txt,"%s",c);
				
				#ifdef ADMIN_DEBUG
				sprintf(buffer,"EDAC admin PIN detected\r\n");print1(buffer);				
				sprintf(buffer,"%s\r\n",sms_newMsg.txt);print1(buffer);
				#endif
			}
			else
			{
				//DEBUG_printStr("Only one SMS cmd\r\n");
				#ifdef _SMS_DEBUG_
				sprintf(buffer,"Only one SMS cmd\r\n");print(buffer);
				#endif
				strcpy(sms_newMsg.txt, modem_rx_string);				
			}
			last_command = true;
		}

        //Note that the proprietory SMS message about the EDAC info can be
        //accessed by any phone number, under any conditions.  The EDAC
        //code will not be distributed.
        tickleRover();
		
		if (strcmpf(sms_newMsg.txt,"**debug**") == 0)
        {
            //A SYS string was received in the message.  This request is for the
            //SMS300 to return it's current system settins to the
            //phone number that sent the message.
            e.type = sms_EDAC;
            queue_push(&q_modem, &e);
            DEBUG_printStr("SMS: EDAC query\r\n");
        }
        else if (strcmpf(sms_newMsg.txt,"**clearsms**") == 0)
        {
            //A **debug** string was received in the message.
            for(i=0;i<32;i++)
            {
                printf("AT+CMGD=%d\r\n",i);
                modem_wait_for(MSG_OK | MSG_ERROR);
            }
        }
        else if (strlen(sms_newMsg.phoneNumber) <= 4)
        {
            //The SMS is from vodafone.  If forwarding is enabled, then set task
            //to forward the SMS on the next go.
            DEBUG_printStr("SMS: From VODAFONE\r\n");
            if(config.forwarding) {
                e.type = sms_FORWARD;
                queue_push(&q_modem, &e);
            }
        }

	//Check if the phone number that is received is in the list
        else if (sms_cmpPhone (sms_newMsg.phoneNumber) || admin_pin==true)	
		//changed sms_cmpPhone so it only compares phone numbers - email addresses return false
        {
			#ifdef ADMIN_DEBUG
			if(admin_pin==true)
			{
				sprintf(buffer,"Processing admin SMS\r\n");print1(buffer);
			}
			#else
				DEBUG_printStr("SMS> Phone number in list\r\n");
			#endif

            if (strcmpf(sms_newMsg.txt,"sys") == 0)
            {
                //A SYS string was received in the message.  This request is for the
                //SMS300 to return it's current system settins to the
                //phone number that sent the message.                				
				e.type = sms_SYSTEM;
                e.param = 0;
                queue_push(&q_modem, &e);
                DEBUG_printStr("SMS: SYS query\r\n");
            }
            else if (strcmpf(sms_newMsg.txt,"status") == 0)
            {
                //A STATUS string was received in the message.  This request is for the
                //SMS300 to return it's current status of the IO to the
                //phone number that sent the message.
                e.type = sms_STATUS;
                e.param = 0;
                queue_push(&q_modem, &e);
                DEBUG_printStr("SMS: STATUS query\r\n");
            }
            /* else if (strstrf(sms_newMsg.txt,"contact")) //and else if (strstrf(sms_newMsg.txt,"log email=")) moved below
            {
                if((strlene(config.pin_code) > 0 && strncmpe(sms_newMsg.txt,config.pin_code,strlene(config.pin_code)) == 0) || admin_pin==true)
				{
                    e.type = sms_CONTACT_CHANGE;
                    e.param = 0;
                    sms_handleNew(&e);
                    DEBUG_printStr("SMS: Contact change\r\n");
                } 
				else
                {
                    e.type = sms_FAIL;
                    queue_push(&q_modem, &e);
                }
            }
			#if EMAIL_AVAILABLE
			else if (strstrf(sms_newMsg.txt,"log email="))
            {
                if((strlene(config.pin_code) > 0 && strncmpe(sms_newMsg.txt,config.pin_code,strlene(config.pin_code)) == 0) || admin_pin==true)
                {
					#ifdef EMAIL_DEBUG
                    sprintf(buffer,"SMS: Log update email address change\r\n");
					print1(buffer);
					#endif
					e.type = sms_EMAIL_CHANGE;
					e.param = 0;
                    sms_handleNew(&e);					
				}
				else
                {
                    e.type = sms_FAIL;
                    queue_push(&q_modem, &e);
                }
			} */
			
			#if EMAIL_AVAILABLE
			else if (strstrf(sms_newMsg.txt,"email log") != 0)
			{
				//doesn't require pin but number must be in list and will only send to update address
				
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"SMS request to email log.csv file received: ");print1(buffer);
				sprintf(buffer,"%s\r\n",sms_newMsg.txt);print1(buffer);
				#endif				
				if(strstrf(sms_newMsg.txt,"=")==0) //no equals - print complete log
				{
					sprintf(sms_dates,"=start-end");
				}
				else
				{
					c = strchr(sms_newMsg.txt,'=');
					sprintf(sms_dates,"%s",c);
					//sprintf(sms_dates,"=2011/12/15 10:20:00-2011/12/15 10:40:00");
				}								
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"sms_dates: %s\r\n",sms_dates);print1(buffer);
				#endif
				mmc_fail_sent=false;
				e.type = sms_LOG_UPDATE;
				e.param = 1;
				queue_push(&q_modem, &e);
			}
			
			else if (strstrf(sms_newMsg.txt,"email latest") != 0) //added 3.xx.7
			{
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"SMS request to email latest.csv file received: ");print1(buffer);
				sprintf(buffer,"%s\r\n",sms_newMsg.txt);print1(buffer);
				#endif
			
				mmc_fail_sent=false;
				e.type = sms_LOG_UPDATE;
				e.param = 4;
				queue_push(&q_modem, &e);
			}
			
			else if (strstrf(sms_newMsg.txt,"email system") != 0)
			{
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"[emailsystem] SMS request to email system log received: ");print1(buffer);
				sprintf(buffer,"%s\r\n",sms_newMsg.txt);print1(buffer);
				#endif
				if(strstrf(sms_newMsg.txt,"=")==0) //no equals - print complete log
				{
					sprintf(sms_dates,"=start-end");
				}
				else
				{
					c = strchr(sms_newMsg.txt,'=');
					sprintf(sms_dates,"%s",c);			
				}
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"[emailsystem] sms_dates: %s\r\n",sms_dates);print1(buffer);
				#endif
				mmc_fail_sent=false;
				e.type = sms_LOG_UPDATE;
				e.param = 2;
				queue_push(&q_modem, &e);		
			}
			
			else if (strstrf(sms_newMsg.txt,"email event") != 0)
			{
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"[emailevent] SMS request to email event log received: ");print1(buffer);
				sprintf(buffer,"%s\r\n",sms_newMsg.txt);print1(buffer);
				#endif
				if(strstrf(sms_newMsg.txt,"=")==0) //no equals - print complete log
				{
					//sprintf(sms_newMsg.txt,"AT+EMAILLOG=start-end");
					sprintf(sms_dates,"=start-end");
				}
				else
				{
					c = strchr(sms_newMsg.txt,'=');
					sprintf(sms_dates,"%s",c);			
				}
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"[emailevent] sms_dates: %s\r\n",sms_dates);print1(buffer);
				#endif
				mmc_fail_sent=false;
				e.type = sms_LOG_UPDATE;
				e.param = 3;
				queue_push(&q_modem, &e);		
			}
			
			else if (strstrf(sms_newMsg.txt,"email error") != 0)
			{
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"[emailevent] SMS request to email error log received: ");print1(buffer);
				sprintf(buffer,"%s\r\n",sms_newMsg.txt);print1(buffer);
				#endif
				if(strstrf(sms_newMsg.txt,"=")==0) //no equals - print complete log
				{
					sprintf(sms_dates,"=start-end");
				}
				else
				{
					c = strchr(sms_newMsg.txt,'=');
					sprintf(sms_dates,"%s",c);			
				}
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"[emailevent] sms_dates: %s\r\n",sms_dates);print1(buffer);
				#endif
				mmc_fail_sent=false;
				e.type = sms_EMAIL_ERROR;
				queue_push(&q_modem, &e);
			}
			
			else if (strstrf(sms_newMsg.txt,"email retry timer") != 0)
			{
				e.type = sms_EMAILRETRYTIMER;
				queue_push(&q_modem, &e);
			}
			
			/*else if (strncmpf(sms_newMsg.txt,"latest",6) == 0)
			{
				sms_newMsg.usePhone=true;				
				
				if (strstrf(sms_newMsg.txt,"=") != 0)
				{
					//if(strlen(sms_newMsg.txt) == 26)
					if( strlen(sms_newMsg.txt)==26 
						&& strpos(sms_newMsg.txt,'/')==11 && strrpos(sms_newMsg.txt,'/')==14 
						&& strpos(sms_newMsg.txt,' ')==17 
						&& strpos(sms_newMsg.txt,':')==20 && strrpos(sms_newMsg.txt,':')==23 )
					{	
						email_log_write_date();
					}
					else
					{
						e.type = sms_FAIL;
						queue_push(&q_modem, &e);
						break;
					}
				}
				
				if ((strstrf(sms_newMsg.txt,"?") != 0) || (strstrf(sms_newMsg.txt,"=") != 0))
				{
					c=email_log_read_date(temp);
					if(!c || c==0)
					{
						sprintf(sms_newMsg.txt,"File not found");
					}
					else
					{
						sprintf(sms_newMsg.txt,"Date and time saved in hidden file: %s",c);				
					}
					msg_sendData();			
				}
				else
				{
					e.type = sms_FAIL;
					queue_push(&q_modem, &e);
				}
				
				//message is sent straight back rather than being queued due to memory constraints
				//an extra 19 bytes are required for char datetime[19]; in sms_handleNew
				//e.type = sms_LATEST;
				//queue_push(&q_modem, &e);				
			}
			*/
			#endif
			
			else if (strncmpf(sms_newMsg.txt,"what are you?",13) == 0)
			{
				e.type = sms_UNITINFO;
				queue_push(&q_modem, &e);
			}
				
            #if GPS_AVAILABLE
            else if (strstrf(sms_newMsg.txt,"gps"))
            {
                DEBUG_printStr("SMS: GPS request received\r\n");
                e.type = sms_GPS_REQUEST;
                queue_push(&q_modem, &e);
            }
            #endif
			
			//else if (strcmp(sms_newMsg.txt,FACTORY_RESET) == 0) //(unsigned char*)
			else if (strncmpf(sms_newMsg.txt,"at&f",4) == 0)
            {
                DEBUG_printStr("Give the unit a factory reset\r\n");
                config_factoryReset();
            }
            //else if (strcmp(sms_newMsg.txt,HARDWARE_RESET) == 0) //(unsigned char*)
			else if (strncmpf(sms_newMsg.txt,"at&r",4) == 0)
            {
                DEBUG_printStr("Give the unit a Hardware reset\r\nNeed Watchdog\r\n");
                while(1);
            }		
			else if ((strncmpe(sms_newMsg.txt, config.pin_code, strlene(config.pin_code)) == 0 ||
                     strlene(config.pin_code)==0) || admin_pin==true)
            {
				#ifdef _SMS_DEBUG_
                sprintf(buffer,"SMS: PIN OK or Admin password received\r\n");print(buffer);
				#endif

                if (strchr(sms_newMsg.txt,'<') && strrchr(sms_newMsg.txt,'>') &&
                    strchr(sms_newMsg.txt,'<') < strrchr(sms_newMsg.txt,'>'))
                { //message to be passed to the UART

                	//sprintf(buffer,"< and > found\r\n");print(buffer);
                    *strrchr(sms_newMsg.txt,'>') = '\0';

                    //figure a way of filtering \r \n and \\ out of the string
                    strrep(sms_newMsg.txt,"##","#");
                    strrep(sms_newMsg.txt,"#r","\r");
                    strrep(sms_newMsg.txt,"#n","\n");
                    print(strchr(sms_newMsg.txt,'<')+1);
                }
				else if (strstrf(sms_newMsg.txt,"contact"))
				{
					e.type = sms_CONTACT_CHANGE;
					e.param = 0;
					sms_handleNew(&e);
					DEBUG_printStr("SMS: Contact change\r\n");				
				}
				
				else if (strstrf(sms_newMsg.txt,"remove data"))
				{
					e.type = mmc_CLEAR_DATA;
					queue_push(&q_event, &e);
				}
				
				#if EMAIL_AVAILABLE
				else if (strstrf(sms_newMsg.txt,"log email="))
				{
					#ifdef EMAIL_DEBUG
					sprintf(buffer,"SMS: Log update email address change\r\n");
					print1(buffer);
					#endif
					e.type = sms_EMAIL_CHANGE;
					e.param = 0;
					sms_handleNew(&e);					
				}
				#endif
				else if (strncmpf(sms_newMsg.txt,"reset pin",9) == 0 && admin_pin==true)
				{
					strcpyef(config.pin_code,"1234");
					#ifdef ADMIN_DEBUG
					sprintf(buffer,"PIN reset to: ");print1(buffer);
					strcpyre(buffer,config.pin_code);print1(buffer);
					sprintf(buffer,"\r\n");print1(buffer);
					#endif
				}
				else if ( strlene(config.pin_code) && strstrf(sms_newMsg.txt,"new pin=") )
				{
					e.type = sms_PIN_CHANGE;
                    sms_handleNew(&e);
                    #ifdef ADMIN_DEBUG
					sprintf(buffer,"SMS: PIN change\r\n");
					print1(buffer);
					#endif
				}
                //else
				else if (strstrf(sms_newMsg.txt,"outall") || strstrf(sms_newMsg.txt,"on") || strstrf(sms_newMsg.txt,"off"))
                //The PIN has been received.  This command is used to turn the outputs on and off.
                //parse the string to find out what output to action
                {

                    if((p3 = strpos(sms_newMsg.txt,';'))!=-1)
                    {
                        substr(sms_newMsg.txt,time,p3+1,0);
                        #ifdef DEBUG
                        sprintf(buffer,"Expiry time: %s\r\n",time);
                        print(buffer);
                        #endif
                        event_populate_time(&e);
                        sprintf(buffer,"%04d%02d%02d%02d%02d%02d",
                                2000+e.year,e.month,e.day,
                                e.hour,e.minute,e.second);
                        print(buffer);
                        if(strncmp(buffer,time,strlen(time))>=0)
                        {
                            #ifdef DEBUG
                            sprintf(buffer,"message expired\r\n");
                            print(buffer);
                            #endif
                            strcpyre(buffer,config.site_name);
                            sprintf(temp,"%s\n",buffer);
                            sprintf(buffer,"%sExpired message received: \"%s\"",temp,sms_newMsg.txt);
                            strcpy(sms_newMsg.txt,buffer);
                            event_populate_time(&e);
                            sms_set_header(&e);
                            sms_newMsg.usePhone = true;
							
							#ifdef _SMS_DEBUG_
							sprintf(buffer,"35\r\n");print1(buffer);
							#endif
			
                            msg_sendData();
                            return;
                        } 
						else
                        {
                            #ifdef DEBUG
                            sprintf(buffer,"message good\r\n");
                            print(buffer);
                            #endif
                        }
                        *strchr(sms_newMsg.txt,';') = '\0';

                    }

                    //break the message into output name / command.
                    //find the last space. This delimits between the ON / OFF command and the output name
                    p1 = strrpos(sms_newMsg.txt,' ');
                    //find the first space. This is either the end of the pin or the end of the output.
                    //Or the end of the message.
                    p2 = strpos(sms_newMsg.txt,' ');

                    if(strlene(config.pin_code) && admin_pin==false) //we have a PIN match, so p2 is the end of the PIN
                    {
                        substr(sms_newMsg.txt,output,p2+1,p1);
                        if(p1 == p2)
                            sprintf(action,"");
                        else
                            substring(sms_newMsg.txt,action,p1+1);
                    }
                    else //empty PIN, so p1 is the end of the output name, if a space is present
                    {
                        if(p1 < 0)
                            sprintf(action,"");
                        else
                        {
                            substring(sms_newMsg.txt,action,p1+1);
                            sms_newMsg.txt[p1] = '\0';
                        }
                        strcpy(output,sms_newMsg.txt);
                    }

                    #ifdef DEBUG
                    sprintf(buffer,"msg=[");print(buffer);
                    print(sms_newMsg.txt);
                    sprintf(buffer,"] p1=%u p2=%u output=[%s] action=[%s]\r\n",p1,p2, output, action);print(buffer);
                    #endif

                    if (strcmpf(output, "outall") == 0)
                    {

                        if (strlen(action) != MAX_OUTPUTS)
                        {
                            DEBUG_printStr("The command was too short\r\n");
                            e.type = sms_FAIL;
                            queue_push(&q_modem, &e);

                        }
                        else
                        {
                            for (i=0; i < MAX_OUTPUTS; i++)
                            {
                                if (action[i] == '1')
                                {
                                   if(config.output[i].state == on)
                                   {
                                        strcpyre(buffer,config.site_name);
                                        sprintf(sms_newMsg.txt,"%s\n",buffer);
                                        strcpyre(buffer,config.output[i].config.name);
                                        strcat(sms_newMsg.txt,buffer);
                                        sprintf(buffer," already turned on.");
                                        strcat(sms_newMsg.txt,buffer);
                                        event_populate_time(&e);
                                        sms_set_header(&e);
                                        sms_newMsg.usePhone = true;
										
										#ifdef _SMS_DEBUG_
										sprintf(buffer,"36\r\n");print1(buffer);
										#endif
										
                                        msg_sendData();
                                   }
                                   else
                                   {
                                        e.type = event_OUT_ON;
                                        e.param = i;
                                        queue_push(&q_event, &e);
                                   }
                                }
                                else if (action[i] == '0')
                                {
                                    if(config.output[i].state == off)
                                   {
                                        strcpyre(buffer,config.site_name);
                                        sprintf(sms_newMsg.txt,"%s\n",buffer);
                                        strcpyre(buffer,config.output[i].config.name);
                                        strcat(sms_newMsg.txt,buffer);
                                        sprintf(buffer," already turned off.");
                                        strcat(sms_newMsg.txt,buffer);
                                        event_populate_time(&e);
                                        sms_set_header(&e);
                                        sms_newMsg.usePhone = true;
										
										#ifdef _SMS_DEBUG_
										sprintf(buffer,"37\r\n");print1(buffer);
										#endif
			
                                        msg_sendData();				
                                   }
                                   else
                                   {
										#ifdef _SMS_DEBUG_
										sprintf(buffer,"1c\r\n");print1(buffer);
										#endif
                                        e.type = event_OUT_OFF;
                                        e.param = i;
                                        queue_push(&q_event, &e);
                                   }
                                }
                                else if (action[i] == 'x')
                                    continue;
                                else
                                {
                                    DEBUG_printStr("The command was too short\r\n");
                                    e.type = sms_FAIL;
                                    e.param = 0;
                                    queue_push(&q_modem, &e);
                                    break;
                                }

                            }
                        }
                    }
                    else
                    {
                        for (i=0;i<MAX_OUTPUTS;i++)
                        {
                            strcpyre(temp, config.output[i].config.name);
                            //make all lower case
                            for(j=0;j<strlene(config.output[i].config.name);j++) {
                                temp[j]=tolower(config.output[i].config.name[j]);
                            }

                            if (atoi(output) == i+1)
                                break;
                            else if (strcmp(output, temp) == 0)
                                break;
                        }

                        if (i < MAX_OUTPUTS)
                        {
                            #ifdef DEBUG
                            sprintf(buffer, "Output %d found\r\n", i);print(buffer);
                            #endif
                            if (strstrf(action, "on"))
                            {
                                if(config.output[i].state == on)
                                {
                                     strcpyre(buffer,config.site_name);
                                     sprintf(sms_newMsg.txt,"%s\n",buffer);
                                     strcpyre(buffer,config.output[i].config.name);
                                     strcat(sms_newMsg.txt,buffer);
                                     sprintf(buffer," already turned on.");
                                     strcat(sms_newMsg.txt,buffer);
                                     event_populate_time(&e);
                                     sms_set_header(&e);
                                     sms_newMsg.usePhone = true;
									 
									#ifdef _SMS_DEBUG_
									sprintf(buffer,"38\r\n");print1(buffer);
									#endif
			
                                     msg_sendData();
                                }
                                else
                                {
									#ifdef _SMS_DEBUG_
									sprintf(buffer,"1a\r\n");print1(buffer);
									#endif
                                    e.type = event_REMOTE_OUT_ON;
                                    e.param = i;
                                    queue_push(&q_event, &e);
                                    DEBUG_printStr("Turn output ON\r\n");
                                }
                            }
                            else if (strstrf (action, "off"))
                            {
                                if(config.output[i].state == off)
                                {
                                     strcpyre(buffer,config.site_name);
                                     sprintf(sms_newMsg.txt,"%s\n",buffer);
                                     strcpyre(buffer,config.output[i].config.name);
                                     strcat(sms_newMsg.txt,buffer);
                                     sprintf(buffer," already turned off.");
                                     strcat(sms_newMsg.txt,buffer);
                                     event_populate_time(&e);
                                     sms_set_header(&e);
                                     sms_newMsg.usePhone = true;
									 
									#ifdef _SMS_DEBUG_
									sprintf(buffer,"39\r\n");print1(buffer);
									#endif
			
                                     msg_sendData();
                                }
                                else
                                {
									#ifdef _SMS_DEBUG_
									sprintf(buffer,"1b\r\n");print1(buffer);
									#endif
									
                                    e.type = event_REMOTE_OUT_OFF;
                                    e.param = i;
                                    queue_push(&q_event, &e);
                                    DEBUG_printStr("Turn output OFF\r\n");
                                }
                            }
                            else if (strcmpf(action,"")==0 && config.output[i].config.momentaryLength != 0 &&
                                     config.output[i].config.default_state != LAST_KNOWN)
                            {
                                if(config.output[i].config.default_state == ON)
                                {
									#ifdef _SMS_DEBUG_
									sprintf(buffer,"2b\r\n");print1(buffer);
									#endif
									
                                    e.type = event_REMOTE_OUT_OFF;
                                    e.param = i;
                                    queue_push(&q_event, &e);
                                    DEBUG_printStr("Turn momentary output OFF\r\n");
                                }
                                else if(config.output[i].config.default_state == OFF)
                                {
									#ifdef _SMS_DEBUG_
									sprintf(buffer,"2a\r\n");print1(buffer);
									#endif
                                    e.type = event_REMOTE_OUT_ON;
                                    e.param = i;
                                    queue_push(&q_event, &e);
                                    DEBUG_printStr("Turn momentary output ON\r\n");
                                }
                            }
                        }
                        else
                        {
                            //Handle an unfound type
                            DEBUG_printStr("SMS> Command not valid\r\n");
                            e.type = sms_FAIL;
                            queue_push(&q_modem, &e);
                        }
                    }
                }
				else
				{
					DEBUG_printStr("SMS: CMD or PIN FAIL\r\n");
					e.type = sms_FAIL;
					queue_push(&q_modem, &e);
				}				
			}
                        
			else
			{
                DEBUG_printStr("SMS: CMD or PIN FAIL\r\n");
                e.type = sms_FAIL;
                queue_push(&q_modem, &e);
            }
        }

        else
        {
            DEBUG_printStr("SMS> Phone number not in list\r\n");
            //If the phone number is not in the list, then the only info they
            //can access is the system and status queries.  And these can
            //only be accessed if the public queries are enabled

            if (config.public_queries)
            {
                if (strcmpf(sms_newMsg.txt,"sys") == 0)
                {
                    //A SYS string was received in the message.  This request is for the
                    //SMS300 to return it's current system settins to the
                    //phone number that sent the message.
                    e.type = sms_SYSTEM;
                    e.param = 0;
                    queue_push(&q_modem, &e);
                    DEBUG_printStr("SMS: SYS query\r\n");
                }
                else if (strcmpf(sms_newMsg.txt,"status") == 0)
                {
                    //A STATUS string was received in the message.  This request is for the
                    //SMS300 to return it's current status of the IO to the
                    //phone number that sent the message.
                    e.type = sms_STATUS;
                    e.param = 0;
                    queue_push(&q_modem, &e);
                    DEBUG_printStr("SMS: STATUS query\r\n");
                }
            }

        }
    } while (!last_command);
	
	#ifdef _SMS_DEBUG_
	sprintf(buffer,"[sms_process] resumming queue\r\n");print(buffer);
	#endif

    //Pause this queue, so that nothing is popped off until the send is complete
    queue_resume(&q_modem);
	
	#ifdef _SMS_DEBUG_
	sprintf(buffer,"[sms_process] queue resumed\r\n");print(buffer);
	#endif

    active_led_off();
    return;
}

/*
    Compare the contact received, to the phone book to see if
    the user is in the contact list
*/
bool sms_cmpPhone (char *contact)
{
    unsigned char i,j;
   // int pnCount=0;
   // int clCount=0;
    eeprom char* temp;
   // bool compareFail=false;

    //if this isn't an email check the last 6 characters only. This avoids network differences
    if(strstrf(contact,"@")==0)
        contact = contact+strlen(contact)-6;
	else //contact is an email address
		return false;

    for (i=0; i<MAX_CONTACTS;i++)
    {
        temp = contact_read(i);
        if(!temp)
            break;
        j=0;
        do
        {
            buffer[j] = temp[j];
        } while(buffer[j++] != 0);

        if(strstr(buffer,contact))
            return true;
    }
    return false;
 }

bool sms_checkConnection()
{
/*
    modem_clear_channel();
    printf("AT+CNI\r\n");
    while(1)
    {
        modem_read();
        if(strstrf(modem_rx_string,"OK")!=0)
        {
            return true;
        }
        else if(strstrf(modem_rx_string,"ERROR")!=0)
        {
            return false;
        }
    }*/
    return true;
}






