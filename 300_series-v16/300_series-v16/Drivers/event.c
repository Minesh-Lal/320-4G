/*      event.c
*
*       Handling event popped off a queue
*
*/

#include <string.h>
#include <stdio.h>
#include <delay.h>
#include <stdlib.h>
#include <ctype.h>

#include "global.h"
#include "drivers\input.h"
#include "drivers\event.h"
#include "drivers\queue.h"
#include "drivers\email.h"
#include "drivers\mmc.h"
#include "buildnumber.h"
#include "drivers\modem.h"
#include "drivers\uart.h"
#include "drivers\config.h"
#include "drivers\debug.h"      //SJL - CAVR2 - added to access debug functions
#include "drivers\output.h"     //SJL - CAVR2 - added to access output functions
#include "drivers\ds1337.h"     //SJL - CAVR2 - added to access real time clock functions
#include "drivers\error.h"      //SJl - CAVR2 - added to access error struct
//#include "drivers\gprs.h"       //SJl - CAVR2 - added to access gprs functions
#include "drivers\str.h"        //SJl - CAVR2 - added to access string functions


void event_populate_time(Event *e)
{
    rtc_get_date(&e->day,&e->month,&e->year);
    rtc_get_time(&e->hour,&e->minute,&e->second);
}

/*
    Handle an event popped off the event queue.
*/
void event_handleNew (Event *incomingEvent)
{

    unsigned char charPos=0, charPos2=0, temp=0;
    char tempStr[10];
    char param, event;
    float val;	//SJL - CAVR2 - double not supported
    char i;
    unsigned long baudTemp;
    Event e;

    event = incomingEvent->type;
    param = incomingEvent->param;

    switch (event)
    {
        //Input channel rising event on alarm A
        //Param = input number
        case event_CHAR :
            DEBUG_printStr(">event_CHAR ");
            if (config.input[param].alarm[ALARM_A].type == ALARM_ABOVE)
            {
                #if LOGGING_AVAILABLE
                log_alarm(ALARM_A,param);
                #endif
                DEBUG_printStr("push Alarm A SMS\r\n");
                //Turn on any outputs that are linked to this event
                output_toSwitch(param, sms_ALARM_A);
                //The alarm type is above, therefore we have receieved
                //an alarm event, push this onto the sms queue, so that
                //the messages will be sent
                if(config.input[param].alarm[ALARM_A].alarm_contact)
                {
                    e.type = sms_ALARM_A;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
                //Remember the last alarm to be sent
                config.input[param].msg = sms_ALARM_A;
            }
            else if (config.input[param].alarm[ALARM_A].type == ALARM_BELOW)
            {
                #if LOGGING_AVAILABLE
                log_alarm(RESET_A,param);
                #endif
                DEBUG_printStr("push Reset A SMS\r\n");
                //Turn on any outputs that are linked to this event
                output_toSwitch(param, sms_RESET_A);
                //The alarm type is below, therefore we have receieved
                //an reset event, push this onto the sms queue, so that
                //the messages will be sent
                if(config.input[param].alarm[ALARM_A].reset_contact)
                {
                    e.type = sms_RESET_A;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
                //Remember the last alarm to be sent
                config.input[param].msg = sms_RESET_A;
            }
            //remember what your current alarm state is

            config.input[param].alarm[ALARM_A].lastZone = ZONE2;
            saved_state[param][ALARM_A] = ZONE2;
          //  sprintf(buffer,"input %d:%d last zone = %d\r\n",param,ALARM_A,config.input[param].alarm[ALARM_A].lastZone);
          //  print(buffer);
            break;

        //Input channel rising event on alarm b
        //Param = input number
        case event_CHBR :
            DEBUG_printStr(">event_CHBR ");
            if (config.input[param].alarm[ALARM_B].type == ALARM_ABOVE)
            {
                #if LOGGING_AVAILABLE
                log_alarm(ALARM_B,param);
                #endif
                DEBUG_printStr("push Alarm B SMS\r\n");
                 //Turn on any outputs that are linked to this event
                output_toSwitch(param, sms_ALARM_B);
                //The alarm type is above, therefore we have receieved
                //an alarm event, push this onto the sms queue, so that
                //the messages will be sent
                if(config.input[param].alarm[ALARM_B].alarm_contact)
                {
                    e.type = sms_ALARM_B;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
                //Remember the last alarm to be sent
                config.input[param].msg = sms_ALARM_B;
            }
            else if (config.input[param].alarm[ALARM_B].type == ALARM_BELOW)
            {
                #if LOGGING_AVAILABLE
                log_alarm(RESET_B,param);
                #endif
                DEBUG_printStr("push Reset B SMS\r\n");
                //Turn on any outputs that are linked to this event
                output_toSwitch(param, sms_RESET_B);
                //The alarm type is below, therefore we have receieved
                //an reset event, push this onto the sms queue, so that
                //the messages will be sent
                if(config.input[param].alarm[ALARM_B].reset_contact)
                {
                    e.type = sms_RESET_B;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
                //Remember the last alarm to be sent
                config.input[param].msg = sms_RESET_B;
            }
            //remember what your current alarm state is

            config.input[param].alarm[ALARM_B].lastZone = ZONE2;
            saved_state[param][ALARM_B] = ZONE2;
         ////   sprintf(buffer,"input %d:%d last zone = %d\r\n",param,ALARM_B,config.input[param].alarm[ALARM_B].lastZone);
          //  print(buffer);
            break;

        //Input channel falling event on alarm A
        //Param = input number
        case event_CHAF :
            DEBUG_printStr(">event_CHAF ");
            if (config.input[param].alarm[ALARM_A].type == ALARM_ABOVE)
            {
                #if LOGGING_AVAILABLE
                log_alarm(RESET_A,param);
                #endif
                DEBUG_printStr("push Reset A SMS\r\n");
                //Turn on any outputs that are linked to this event
                output_toSwitch(param, sms_RESET_A);
                //The alarm type is above, therefore we have receieved
                //an reset event, push this onto the sms queue, so that
                //the messages will be sent
                if(config.input[param].alarm[ALARM_A].reset_contact)
                {
                    e.type = sms_RESET_A;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
                //Remember the last alarm to be sent
                config.input[param].msg = sms_RESET_A;
            }
            else if (config.input[param].alarm[ALARM_A].type == ALARM_BELOW)
            {
                #if LOGGING_AVAILABLE
                log_alarm(ALARM_A,param);
                #endif
                DEBUG_printStr("push Alarm A SMS\r\n");
                //Turn on any outputs that are linked to this event
                output_toSwitch(param, sms_ALARM_A);
                //The alarm type is below, therefore we have receieved
                //an alarm event, push this onto the sms queue, so that
                //the messages will be sent
                if(config.input[param].alarm[ALARM_A].alarm_contact)
                {
                    e.type = sms_ALARM_A;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
                //Remember the last alarm to be sent
                config.input[param].msg = sms_ALARM_A;
            }
            //remember what your current alarm state is

            config.input[param].alarm[ALARM_A].lastZone = ZONE0;
            saved_state[param][ALARM_A] = ZONE0;
         //   sprintf(buffer,"input %d:%d last zone = %d\r\n",param,ALARM_A,config.input[param].alarm[ALARM_A].lastZone);
         //   print(buffer);
            break;

        //Input channel falling event on alarm b
        //Param = input number
        case event_CHBF :
            DEBUG_printStr(">event_CHBF ");
            if (config.input[param].alarm[ALARM_B].type == ALARM_ABOVE)
            {
                #if LOGGING_AVAILABLE
                log_alarm(RESET_B,param);
                #endif
                DEBUG_printStr("push Reset B SMS\r\n");
                //Turn on any outputs that are linked to this event
                output_toSwitch(param, sms_RESET_B);
                //The alarm type is above, therefore we have receieved
                //an reset event, push this onto the sms queue, so that
                //the messages will be sent
                if(config.input[param].alarm[ALARM_B].reset_contact)
                {
                    e.type = sms_RESET_B;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
                //Remember the last alarm to be sent
                config.input[param].msg = sms_RESET_B;
            }
            else if (config.input[param].alarm[ALARM_B].type == ALARM_BELOW)
            {
                #if LOGGING_AVAILABLE
                log_alarm(ALARM_B,param);
                #endif
                DEBUG_printStr("push Alarm B SMS\r\n");
                //Turn on any outputs that are linked to this event
                output_toSwitch(param, sms_ALARM_B);
                //The alarm type is below, therefore we have receieved
                //an alarm event, push this onto the sms queue, so that
                //the messages will be sent
                if(config.input[param].alarm[ALARM_B].alarm_contact)
                {
                    e.type = sms_ALARM_B;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
                //Remember the last alarm to be sent
                config.input[param].msg = sms_ALARM_B;
            }
            //remember what your current alarm state is
            config.input[param].alarm[ALARM_B].lastZone = ZONE0;
            saved_state[param][ALARM_B] = ZONE0;
        //    sprintf(buffer,"input %d:%d last zone = %d\r\n",param,ALARM_B,config.input[param].alarm[ALARM_B].lastZone);
         //   print(buffer);
            break;

        //The sensor on the input is reading out of bounds
        //Param = input number
        case event_SENSOR_FAIL :
            e.type = sms_SENSOR_FAIL;
            e.param = param;
            queue_push(&q_modem, &e);
            break;
        //Param = enumerated uart string rec
        //All commands received on the uart and parsed, then sent here for handling.
        case event_UART_REC :          //Uart Receive event
            switch (param)
            {
                case UART_SET_BAUD :
                    //Received in the format at+baud=<baud>
                    charPos = strpos(command_string, '=');
                    substring(command_string, command_string, charPos+1);

        		    baudTemp = uart1_setup.baud;
        		    uart1_setup.baud = atoi(command_string);

        		    if ((uart1_setup.baud == 1200) || (uart1_setup.baud == 2400) || (uart1_setup.baud == 4800) ||
        		        (uart1_setup.baud == 9600) || (uart1_setup.baud == 14400) || (uart1_setup.baud == 19200) ||
        		        (uart1_setup.baud == 38400) || (uart1_setup.baud == 57600)) 	//tried to add 115200 but error too high (7%) for osc=6MHz
					{
        		        sprintf(buffer,"OK\r\n");
        		        print(buffer);
        		        delay_ms(50);
        		        uart1_init(&uart1_setup);
        		    }
        		    else {
        		        uart1_setup.baud = baudTemp;
        		        sprintf(buffer,"ERROR\r\n");
        		        print(buffer);
        		    }

                    break;
                //Setup for comm port
                case UART_SETUP :
                    //The uart setup comes in the form : AT+COMM=<data bits>,<parity>,<stop bits>
                    //Find the number of data bits requested
                    if (strlen(command_string) != 13)
                    {
                        sprintf(buffer,"ERROR: command too short\r\n");print(buffer);
        		        break;
        		    }
                    charPos = strpos(command_string, '=');
                    charPos2 = strpos(command_string, ',');
                    substr(command_string, tempStr, charPos+1, charPos2);
                    //Can only handle 7 or 8 data bits.
        		    if ((atoi(tempStr) == 8) || (atoi(tempStr) == 7))
        		    {
        		        uart1_setup.data_bits = atoi(tempStr);
        		    }
        		    else
        		    {
        		        sprintf(buffer,"ERROR: 7 or 8 expected\r\n");
        		        print(buffer);
        		        break;
        		    }
        		    //Nibble off the first part of the string that is used
        		    substring(command_string, command_string, charPos2+1);
        		    //Find the parity requested
                    charPos2 = strpos(command_string, ',');
                    substr(command_string, tempStr, 0, charPos2);
                    *tempStr = toupper(*tempStr);
                    //Make sure it is a vaild type
        		    if ((*tempStr == 'N') || (*tempStr == 'E') || (*tempStr == 'O'))
        		    {
        		        uart1_setup.parity = *tempStr;
        		    }
        		    else
        		    {
        		        sprintf(buffer,"ERROR: 'N','O' or 'E' expected\r\n");
        		        print(buffer);
        		        break;
        		    }
        		    //Nibble off the first part of the string that is used
        		    substring(command_string, command_string, charPos2+1);
        		    if ((atoi(command_string) == 1) || (atoi(command_string) == 2))
        		    {
        		        uart1_setup.stop_bits = atoi(command_string);
        		    }
        		    else
        		    {
        		        sprintf(buffer,"received %s\r\n",command_string);
        		        print(buffer);
        		        sprintf(buffer,"ERROR: 1 or 2 expected\r\n");
        		        print(buffer);
        		        break;
        		    }

        		    sprintf(buffer,"OK\r\n");print(buffer);
        		    delay_ms(50);
        		    uart1_init(&uart1_setup);

                    break;

                case UART_READ_INPUT :
                    //Work out the index of the array that the AT+IN cmd was called with.
                    charPos = (int)command_string[5] - '1';
                    if (charPos < 0 || charPos >= MAX_INPUTS)
                    {
                        sprintf(buffer,"ERROR\r\n");print(buffer);
                        break;
                    }
                    //Make sure that the input is enabled
                    if (!config.input[charPos].enabled)
                    {
                        sprintf(buffer,"+IN%d=disabled\r\n",charPos+1);
                        print(buffer);
                        break;
                    }
                    //Print out the return command
                    sprintf(buffer, "+IN%d=",charPos+1);print(buffer);
                    //Decide what message needs to be attached                
					
					//SJL - DEBUG
					//val = input[7].ADCVal;
                    //sprintf(buffer,"%2.3f", val);print(buffer);
					//val = 5859/pulse_count;
					//sprintf(buffer,"%2.3f, [%u]", val, pulse_count);print(buffer);
					//SJL - END OF DEBUG				
					
					if ((config.input[charPos].msg == sms_ALARM_A) || (config.input[charPos].msg == sms_ALARM_B))
                    {
                        if (config.input[charPos].type == DIGITAL || config.input[charPos].msg == sms_ALARM_A)
                            charPos2 = ALARM_A;
                        else
                            charPos2 = ALARM_B;
                        //Print the associated sms message that would be sent
                        if(config.input[charPos].alarm[charPos2].type != ALARM_NONE)
                        {
                            print1_eeprom(config.input[charPos].alarm[charPos2].alarm_msg);
                            printStr(": ");
                        }
                        //Decide and print the state of the input
                        if (config.input[charPos].type != DIGITAL)
                        {
                            val = input_getVal(charPos);
                            sprintf(buffer,"%2.1f", val);print(buffer);
                            print1_eeprom(config.input[charPos].units);
                            if(config.input[charPos].type == PULSE)
                            {
                                switch(config.pulse[charPos-6].period)
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
                                print(buffer);
                            }
                            sprintf(buffer,"\r\n");print(buffer);
                        }
                        else
                        {
                            if ((input[charPos].alarm[ALARM_A].buffer & LATEST_SAMPLE) == ZONE2)
                            {
                                sprintf(buffer,"open\r\n");print(buffer);
                            }
                            else
                            {
                                sprintf(buffer, "closed\r\n");print(buffer);
                            }
                        }
                    }
                    else
                    {
                        if (config.input[charPos].type == DIGITAL || config.input[charPos].msg == sms_RESET_A)
                            charPos2 = ALARM_A;
                        else
                            charPos2 = ALARM_B;
                        //Print the associated sms message that would be sent
                        if(config.input[charPos].alarm[charPos2].type != ALARM_NONE)
                        {
                            print1_eeprom(config.input[charPos].alarm[charPos2].reset_msg);
                            printStr(": ");
                        }
                        //Decide and print the state of the input
                        if (config.input[charPos].type != DIGITAL)
                        {
                            val = input_getVal(charPos);//(config.input[charPos].conv_grad * input[charPos].ADCVal) + config.input[charPos].conv_int;
                            sprintf(buffer,"%2.1f", val);print(buffer);
                            print1_eeprom(config.input[charPos].units);
                            if(config.input[charPos].type == PULSE)
                            {
                                switch(config.pulse[charPos-6].period)
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
                                print(buffer);
                            }						
                            sprintf(buffer,"\r\n");print(buffer);
                        }
                        else
                        {
                            if ((input[charPos].alarm[ALARM_A].buffer & LATEST_SAMPLE) == ZONE2)
                            {
                                sprintf(buffer,"open\r\n");print(buffer);
                            }
                            else
                            {
                                sprintf(buffer, "closed\r\n");print(buffer);
                            }
                        }
                    }
                    
					break;
                case UART_READ_OUTPUT :
                    //Find the index from the AT cmd for the output array.
                    charPos=(int)command_string[6] - '1';
                    if (charPos < 0 || charPos >= MAX_OUTPUTS)
                    {
                        sprintf(buffer,"ERROR\r\n");print(buffer);
                    }
                    else if (config.output[charPos].enabled)
                    {
                        if (config.output[charPos].state)
                        {
                            sprintf(buffer, "+OUT%d=",charPos+1);
                            print(buffer);
                            print1_eeprom(config.output[charPos].config.on_msg);
                            sprintf(buffer,": closed\r\n");
                            print(buffer);
                        }
                        else
                        {
                            sprintf(buffer, "+OUT%d=",charPos+1);
                            print(buffer);
                            print1_eeprom(config.output[charPos].config.off_msg);
                            sprintf(buffer,": open\r\n");
                            print(buffer);
                        }
                    }
                    else
                    {
                        sprintf(buffer,"+OUT%d=disabled\r\n",charPos+1);
                        print(buffer);
                    }
                    break;
                case UART_SWITCH_OUTPUT:
                    //Find the index from the AT cmd for the output array.
                    for(charPos=0;command_string[charPos];charPos++)
                        command_string[charPos]=toupper(command_string[charPos]);
                    charPos=(int)command_string[6] - '1';

                    if (charPos < 0 || charPos >= MAX_OUTPUTS)
                    {
                        sprintf(buffer,"ERROR\r\n");print(buffer);
                    }
                    else if (config.output[charPos].enabled)
                    {
                        e.param = charPos;
                        if (strstrf(command_string, "ON"))
                        {
                            e.type = event_OUT_ON;
                            queue_push(&q_event,&e);
                            sprintf(buffer,"OK\r\n");print(buffer);
                        }
                        else if(strstrf(command_string,"OFF"))
                        {
							#ifdef _SMS_DEBUG_
							sprintf(buffer,"2d\r\n");print1(buffer);
							#endif
                            e.type =  event_OUT_OFF;
                            queue_push(&q_event,&e);
                            sprintf(buffer,"OK\r\n");print(buffer);
                        }
                        else
                        {
                            sprintf(buffer,"ERROR\r\n");
                            print(buffer);
                        }
                    }
                    else
                    {
                        sprintf(buffer,"ERROR\r\n");
                        print(buffer);
                    }
                    break;
                case UART_SWITCH_OUTPUTALL :
                    //Switch all outputs at once
                    if (strlen(command_string) != 10+MAX_OUTPUTS)
                    {
                        sprintf(buffer, "ERROR\r\n");
                        print(buffer);
                        break;
                    }
                    charPos = strpos(command_string, '=')+1;
                    for (temp=0; temp < MAX_OUTPUTS; temp++)
                    {
                        if (command_string[charPos+temp] == '1')
                        {
                            e.type = event_OUT_ON;
                            e.param = temp;
                            queue_push(&q_event, &e);
                        }
                        else if (command_string[charPos+temp] == '0')
                        {
							#ifdef _SMS_DEBUG_
							sprintf(buffer,"3d\r\n");print1(buffer);
							#endif
                            e.type = event_OUT_OFF;
                            e.param = temp;
                            queue_push(&q_event, &e);
                        }
                    }
                    sprintf(buffer, "OK\r\n");print(buffer);
                    break;
                case UART_SERIAL :
                    sprintf(buffer, "+SERIAL=");print(buffer);
                    print1_eeprom(serial);
                    sprintf(buffer, "\r\n");print(buffer);
                    break;
                case UART_SERIAL_LOAD:
                    charPos=strpos(command_string, '=');
                    substring(command_string, command_string, charPos+1);

                    if (strlen(command_string) > 8) {
                        sprintf(buffer, "ERROR\r\n");
                        print(buffer);
                    }
                    else {
                        strcpye(serial, command_string);
                        sprintf(buffer, "OK\r\n");print(buffer);
                    }
                    break;
                case UART_SET_TIME:
                    //The time is entered in the following format
                    //at+time="dd/mm/yy,hh:mm:ss"

                    //Rip off the date first
                    charPos = strpos (command_string, '"');
                    charPos2= strpos (command_string, '/');
                    substr(command_string, tempStr, charPos+1, charPos2);
                    //This is the date requested
                    temp = atoi(tempStr);
                    substring(command_string, command_string, charPos2+1);

                    charPos2= strpos (command_string, '/');
                    substr(command_string, tempStr, 0, charPos2);
                    //This is the month requested
                    charPos = atoi(tempStr);
                    //sprintf(buffer,"month=%d\r\n", charPos);print(buffer);
                    substring(command_string, command_string, charPos2+1);
                    //sprintf(buffer,"%s\r\n", command_string);print(buffer);

                    charPos2= strpos (command_string, ',');
                    substr(command_string, tempStr, 0, charPos2);
                    //This is the year
                    charPos2 = atoi(tempStr);
                    //This is the year requested
                    //sprintf(buffer,"year=%d\r\n", charPos2);print(buffer);
                    rtc_set_date (temp, charPos, charPos2);

                    charPos2= strpos (command_string, ',');
                    substring(command_string, command_string, charPos2+1);
                    //sprintf(buffer,"String finished with %s\r\n", command_string);print(buffer);

                    //Rip off the time next
                    charPos2= strpos (command_string, ':');
                    substr(command_string, tempStr, 0, charPos2);
                    //This is the date requested
                    temp = atoi(tempStr);
                    //sprintf(buffer,"hour=%d\r\n", temp);print(buffer);
                    substring(command_string, command_string, charPos2+1);
                    //sprintf(buffer,"%s\r\n", command_string);print(buffer);

                    charPos2= strpos (command_string, ':');
                    substr(command_string, tempStr, 0, charPos2);
                    //This is the month requested
                    charPos = atoi(tempStr);
                    //sprintf(buffer,"sec=%d\r\n", charPos);print(buffer);
                    substring(command_string, command_string, charPos2+1);
                    //sprintf(buffer,"%s\r\n", command_string);print(buffer);

                    charPos2= strpos (command_string, '"');
                    substr(command_string, tempStr, 0, charPos2);

                    //This is the year
                    charPos2 = atoi(tempStr);
                    //This is the year requested
                    //sprintf(buffer,"hour=%d\r\n", charPos2);print(buffer);
                    //Write the time to the rtc
                    rtc_set_time (temp, charPos, charPos2);

                    sprintf(buffer, "OK\r\n");print(buffer);

                    break;
                case UART_GET_TIME:
                    rtc_get_date(&charPos, &charPos2, &temp);
                    sprintf(buffer,"+TIME: %02d/%02d/%02d,",charPos, charPos2, temp);print(buffer);
                    rtc_get_time(&charPos, &charPos2, &temp);
                    sprintf(buffer,"%02d:%02d:%02d\r\n",charPos, charPos2, temp);print(buffer);
                    break;
                case EDAC_QUEREY:
                    sprintf(buffer,"rev=");print(buffer);
                    sprintf(buffer, "%p.%p\r\nstart=%02d:%02d:%02d %02d/%02d/%02d\r\n", code_version,buildnumber,error.startup.hour,
                                                                        error.startup.min,
                                                                        error.startup.sec,
                                                                        error.startup.day,
                                                                        error.startup.month,
                                                                        error.startup.year);
                    print(buffer);
                    sprintf(buffer, "q_ev=%d\r\nq_modem=%d\r\npOnR=%d\r\nexR=%d\r\nboR=%d\r\nwdR=%d\r\nuR=%d\r\n",
                                                                        error.q_event_ovf,
                                                                        error.q_modem_ovf,
                                                                        error.powerOnReset,
                                                                        error.externalReset,
                                                                        error.brownOutReset,
                                                                        error.watchdogReset,
                                                                        error.unknownReset);
                    print(buffer);
                    sprintf(buffer,"jtR=%d\r\nsrC=%d\r\nsfC=%d\r\nmeC=%d\r\nnfC=%d\r\ntmC=%d\r\nssC=%d\r\nfeC=%d\r\n",
                                                                        error.jtagReset,
                                                                        error.emailRetryCounter,
                                                                        error.emailFailCounter,
                                                                        error.modemErrorCounter,
                                                                        error.networkFailureCounter,
                                                                        error.throughModeCounter,
                                                                        error.emailSendCounter,
                                                                        error.framingErrorCounter);
                    print(buffer);
                    break;
                case UART_CNI:
                    //Return the status of the network connection
                    if (startupError)
                    {
                        sprintf(buffer,"+CNI=failed\r\n");print(buffer);
                    }
                    else
                    {
                        sprintf(buffer,"+CNI=OK\r\n");print(buffer);
                    }
                    break;
                case UART_RN:
                    //Check the network connection and try to connect again if needed
                    e.type = sms_CHECK_NETWORK;
                    queue_push(&q_modem, &e);
                    break;
                case UART_SET_CMGS:
                    //Received in the format at+baud=<baud>
                    charPos = strpos(command_string, '=');
                    substring(command_string, command_string, charPos+1);

        		    if (strcmpf(command_string, "0") == 0)
        		    {
        		        config_cmgs = false;
        		        sprintf(buffer,"OK\r\n");
        		        print(buffer);

        		    }
        		    else if (strcmpf(command_string, "1") == 0)
        		    {
        		        config_cmgs = true;
        		        sprintf(buffer,"OK\r\n");
        		        print(buffer);

        		    }
        		    else
        		    {
        		        sprintf(buffer,"ERROR\r\n");
        		        print(buffer);
        		    }

                    break;
                case UART_RECMAIL:
                    charPos = strpos (command_string, '=');
                    if (charPos == 255)
                    {
                        sprintf(buffer, "ERROR\r\n");print(buffer);
                    }
                    else
                    {
                        if (command_string[charPos+1] == '1')
                        {
                            email_recMail = true;
                            sprintf(buffer, "OK\r\n");print(buffer);
                        }
                        else if (command_string[charPos+1] == '0')
                        {
                            email_recMail = false;
                            sprintf(buffer, "OK\r\n");print(buffer);
                        }
                        else
                        {
                            sprintf(buffer, "ERROR\r\n");print(buffer);
                        }
                    }
                    break;
                default:
                    DEBUG_printStr("EVENT: Uart Task not Handled\r\n");
                    break;
            }
            uart_paused = false;
            break;
        //Param = enumerated modem string rec
        case event_MODEM_REC :         //Modem Receive event
//            #ifdef _MODEM_DEBUG_
//            sprintf(buffer,">MODEM: New SMS received (%s)\r\n", modem_rx_string);print(buffer);
//            #endif
            //sprintf(buffer,"event.c - New SMS received (%s)\r\n", modem_rx_string);print(buffer);
            //Parse the string to find out where the index of the sms receieved is.
            charPos = strpos(modem_rx_string,',');
            if (charPos != 0)
            {
                charPos=atoi(strchr(modem_rx_string,',')+1);

                e.type = sms_READ_SMS;
                e.param = charPos;
                queue_push(&q_modem, &e);
                #ifdef DEBUG
                sprintf(buffer,"Message is in slot %d\r\n",charPos);
                print(buffer);
                #endif
            }
            #ifdef DEBUG
            else DEBUG_printStr("the colon wasn't found\r\n");
            #endif

            break;
        case event_MODEM_REC_OLD: //a previously saved message
//            #ifdef _MODEM_DEBUG_
//            sprintf(buffer,">MODEM: SMS found on sim (");print(buffer);
//            print(modem_rx_string);
//            sprintf(buffer,")\r\n");print(buffer);
//            #endif
            //Parse the string to find out where the index of the sms receieved is.
            charPos = strpos(modem_rx_string,':');
            if (charPos != 0)
            {
                charPos=atoi(strchr(modem_rx_string,':')+1);

                e.type = sms_READ_SMS;
                e.param = charPos;
                queue_push(&q_modem, &e);
                #ifdef DEBUG
                sprintf(buffer,"Message is in slot %d\r\n",charPos);
                print(buffer);
                #endif
            }
            #ifdef DEBUG
            else DEBUG_printStr("the colon wasn't found\r\n");
            #endif
            break;
        //Turn Output On
        //Param = output number
        case event_OUT_ON :
            if (config.output[param].enabled)
            {
                #ifdef _OUTPUT_DEBUG_
                sprintf(buffer,">OUTPUT: Turn on output %d\r\n", param); print(buffer);
                #endif
                //config.output[param].set(on);
                output_switch(param,on);
                config.output[param].state = on;
                //Send message to the user to tell them the output has been turned on
                if(config.output[param].config.on_contact)
                {
                    e.type = sms_OUT_ON;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
            }
            else
            {
		#ifdef DEBUG
		sprintf(buffer,"Output %d not enabled\r\n",param);
		print(buffer);
		#endif
                if(config.output[param].config.on_contact)
                {
		    e.type = sms_OUT_FAIL;
		    e.param = param;
		    queue_push(&q_modem, &e);
		}
            }
            break;
        //Turn Output Off
        //Param = output number
        case event_OUT_OFF :
            if (config.output[param].enabled)
            {
                #ifdef _OUTPUT_DEBUG_
                sprintf(buffer,">OUTPUT: Turn off output %d\r\n", param); print(buffer);
                #endif
                output_switch(param,off);
                config.output[param].state = off;
                //Send message to the user to tell them the output has been turned on
                if(config.output[param].config.off_contact)
                {
                    e.type = sms_OUT_OFF;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
            }
            else
            {
                #ifdef DEBUG
                sprintf(buffer,"Output %d not enabled\r\n",param);
                print(buffer);
                #endif
				#ifdef _SMS_DEBUG_
				sprintf(buffer,"1c\r\n");print1(buffer);
				#endif
                if(config.output[param].config.on_contact)
                {
		    e.type = sms_OUT_FAIL;
		    e.param = param;
		    queue_push(&q_modem, &e);
		}
            }
            break;
        //Turn Output On
        //Param = output number
        case event_REMOTE_OUT_ON :
            if (config.output[param].enabled)
            {
                #ifdef _OUTPUT_DEBUG_
                sprintf(buffer,">OUTPUT: Turn on output %d\r\n", param); print(buffer);
                #endif
                output_switch(param,on);
                config.output[param].state = on;
                //Send message to the user to tell them the output has been turned on
                if(config.output[param].config.on_contact)
                {
                    e.type = sms_OUT_ON;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
            }
            else
            {
                #ifdef DEBUG
                sprintf(buffer,"Output %d not enabled\r\n",param);
                print(buffer);
                #endif		
				#ifdef _SMS_DEBUG_
				sprintf(buffer,"2c\r\n");print1(buffer);
				#endif
                if(config.output[param].config.on_contact)
                {
		    e.type = sms_REMOTE_OUT_FAIL;
		    e.param = param;
		    queue_push(&q_modem, &e);
		}
            }
            break;
        //Turn Output Off
        //Param = output number
        case event_REMOTE_OUT_OFF :
            if (config.output[param].enabled)
            {
                #ifdef _OUTPUT_DEBUG_
                sprintf(buffer,">OUTPUT: Turn off output %d\r\n", param); print(buffer);
                #endif
                output_switch(param,off);
                config.output[param].state = off;
                //Send message to the user to tell them the output has been turned on
                if(config.output[param].config.off_contact)
                {
                    e.type = sms_OUT_OFF;
                    e.param = param;
                    queue_push(&q_modem, &e);
                }
            }
            else
            {
                #ifdef DEBUG
                sprintf(buffer,"Output %d not enabled\r\n",param);
                print(buffer);
                #endif
                if(config.output[param].config.on_contact)
                {
		    e.type = sms_REMOTE_OUT_FAIL;
		    e.param = param;
		    queue_push(&q_modem, &e);
		}
            }
            break;
        case event_THRU_MODE:
            if(modem_TMEnabled)
                break;
            //#if TCPIP_AVAILABLE
            //gprs_stopConnection();
            //#endif

            #if MODEM_TYPE==Q24NG_PLUS
            printf("AT+WMUX=1\r\n");
            modem_wait_for(MSG_OK | MSG_ERROR);
            #endif

            #if SYSTEM_LOGGING_ENABLED && HW_REV != 6
            sprintf(buffer,"Answering CSD call");log_line("system.log",buffer);
            #endif
            #if MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE == SIERRA_MC8795V
           /* modem_read();
            #ifdef DEBUG
            putchar1('[');
            print(modem_rx_string);
            sprintf(buffer,"]\r\n");print(buffer);
            #endif */
            delay_ms(10);
            putchar('A');
            delay_ms(200);
            putchar('T');
            delay_ms(200);
            putchar('A');
            delay_ms(200);
            putchar('\r');
            delay_ms(200);
           /* DEBUG_printStr("TM to modem\r\n");
            while(1)
            {
                if(rx_counter0>0)
                    putchar1(getchar());
                if(rx_counter1>0)
                {
                    i = getchar1();
                    switch(i)
                    {
                        case '`':
                            delay_ms(10);
                            putchar('A');
                            delay_ms(200);
                            putchar('T');
                            delay_ms(200);
                            putchar('A');
                            delay_ms(200);
                            putchar('\r');
                            delay_ms(200);
                            break;
                        default:
                            putchar(i);
                            break;
                    }
                }
                tickleRover();
            }          */
            #else
            printf("ATA\r\n");
            #endif
            DEBUG_printStr("Answering call...\r\n");
            //wait for the connect command

                do
                {
                    modem_read();
                    #ifdef DEBUG
                    putchar1('<');
                    print(modem_rx_string);
                    sprintf(buffer,">\r\n");print(buffer);
                    #endif

                } while(!(strstrf(modem_rx_string,"CONNECT") ||
                          strstrf(modem_rx_string,"NO CARRIER") ||
                          strstrf(modem_rx_string,"+WCNT:") ||
                          strstrf(modem_rx_string,"+WEND:") ||
                          strstrf(modem_rx_string,"ERROR") ||
                          strstrf(modem_rx_string,"OK")));


            if (strstrf(modem_rx_string, "CONNECT")||
                strstrf(modem_rx_string, "+WCNT"))
            {
                #if MODEM_TYPE==Q24NG_PLUS
                while(1)
                {
                    temp = getchar();
                    #ifdef DEBUG
                    sprintf(buffer,"Cleared 0x%02X\r\n",temp);
                    print(buffer);
                    #endif
                    if(temp=='\r'||temp=='\n')
                        continue;
                    else
                        break;
                }
                printf("AT+WMUX=0\r\n");
                #endif
                if(verbose)
                {
                    sprintf(buffer,"Connect Sucessful\r\n");
                    print(buffer);
                }
                #if SYSTEM_LOGGING_ENABLED
                sprintf(buffer,"Call answered");log_line("system.log",buffer);
                #endif
                modem_TMEnabled = true;
                modem_TMEcho = true;
                uart_paused = false;
                queue_pause(&q_modem);
                if (error.throughModeCounter != 0xFFFF)
                    error.throughModeCounter++;
            }
            else
            {
                //Hang up.
                modem_disconnect_csd();
                if(verbose)
                {
                    sprintf(buffer,"Failed to Enter Through Mode\r\n");
                    print(buffer);
                }
                #if SYSTEM_LOGGING_ENABLED
                sprintf(buffer,"CSD failed");log_line("system.log",buffer);
                #endif
                //Make sure that normal operation is resumed
                modem_TMEnabled = false;
                modem_TMEcho = false;
                modem_paused = false;
                uart_paused = false;
                queue_resume(&q_modem);
                //modem_restart();
            }
            break;
        case event_LOG_ENTRY:
            #if LOGGING_AVAILABLE
            log_event_on_queue = false;
            //log_entry(); 
			//#if EMAIL_AVAILABLE
			latest_log_entry();
			//#endif
            mmc_init();
            #endif
            break;
        case event_EXTERNAL_UART:

        #if EXTERNAL_UART
            if(command_ready2())
            {
                DEBUG_printStr("Command available on alternate uart: ");
                for(i=rx_counter2(),temp=0;i>0;i--)
                {
                    uart_rx_string[temp] = toupper(getchar2());
                    putchar1(uart_rx_string[temp]);
                    if(buffer[temp++] =='\r')
                    {
                        uart_handleNew(uart_rx_string);
                        break;
                    }
                }
                DEBUG_printStr("\r\n");
            }
        #endif
            break;
        case event_MMC_INIT:
            mmc_init_event_on_queue=false;
            #if LOGGING_AVAILABLE
            mmc_init();
            #endif
            break;
        case mmc_CLEAR_DATA:
            #if LOGGING_AVAILABLE
            if(!mmc_clear_data())
                printStr("OK\r\n");
            else if(verbose)
                    printStr("ERROR\r\n");
            #else
            printStr("ERROR\r\n");
            #endif
            break;
			
		case event_CHECK_NETWORK_STATUS:
			//sprintf(buffer,"Execute check status\r\n");print1(buffer);
			modem_check_status(&param);
			//sprintf(buffer,"Set flag to false\r\n");print1(buffer);
			check_status_event_on_queue=false;
			break;
			
        default:
            break;
    }

    return;
}
