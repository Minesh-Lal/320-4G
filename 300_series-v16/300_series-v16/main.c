/*****************************************************
Project : EDAC320
Version : REV 3.15.501
Date    : 22/02/11
Author  : Sam Lea (SJL), Christopher Cook
Company : EDAC Electronics Ltd
*****************************************************/

#include <mega128.h>
#include <delay.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "drivers\uart.h"
#include "drivers\modem.h"
#include "drivers\mmc.h"
//#include "drivers\extrauart.h"
#include "buildnumber.h"

//SJL - added, initialize_media() used instead of mmc_init
#include "flash\sd_cmd.h"
#include "flash\file_sys.h"


// Standard Input/Output functions
#include <stdio.h>

#if (HW_REV == V2)
    // I2C Bus functions
    #asm
   .equ __i2c_port=0x15 ;PORTC
   .equ __sda_bit=1
   .equ __scl_bit=0
    #endasm
#elif (HW_REV == V5 || HW_REV == V6)
    #asm
   .equ __i2c_port=0x12 ;PORTD
   .equ __sda_bit=1
   .equ __scl_bit=0
    #endasm
#endif

#include <i2c.h>

#define WDCE 4
#define WDE 3

//Prototype headers moved to global.h
//void port_init(void);
//void startup_config_reset(void);
//void wakeRover(void);
//void sedateRover(void);

#include "drivers\ds1337.h"
#include "drivers\input.h"
#include "drivers\sms.h"
#include "drivers\output.h"
#include "drivers\config.h"
#include "drivers\str.h"
#include "drivers\error.h"
#include "drivers\queue.h"
#include "drivers\event.h"
#include "drivers\debug.h"
//#include "drivers\gprs.h"
#include "drivers\email.h"
#include "drivers\contact.h"

//added for EMC Testing
//#include "drivers\usb.h"

/* Globals needed in main.c */
//extern eeprom unsigned int modem_baud; 		//global.h
//extern eeprom uart_t uart1_setup; 	//uart.h
//extern eeprom error_t error; 			//error.h
//extern eeprom unsigned char startup_first; 	//global.h
//extern eeprom unsigned char build_type; 		//global.h
//extern char buffer[];                 //global.h
//extern char uart_rx_string[UART_SWBUFFER_LEN];	//uart.h and .c
//extern flash char *code_version;		//global.h
//extern unsigned char startupError; 			//global.h
//extern bool modem_TMEcho;             //global.h
//extern bool modem_TMEnabled;          //global.h
//extern bool long_time_up;
//extern queue_t q_modem;
//extern queue_t q_event;
//extern bool uart_paused;
//extern bool modem_paused; 			//global.h
//extern eeprom config_t config; 		//config.h
//extern eeprom bool config_cmgs; 		//global.h
//extern eeprom bool email_recMail; 	//global.h
//eeprom bool gprs_enabled;				//global scope removed - unused
//extern bool log_event_on_queue;		//global.h
//extern bool mmc_init_event_on_queue;	//global.h
//extern eeprom unsigned char* adc_current_offset;	//input.h
//extern eeprom unsigned char* adc_voltage_offset;	//input.h
//extern eeprom char verbose; 			//global.h
//extern bool modem_queue_stuck;		//sms.h
//extern char modem_register;           //global.h
//extern char modem_not_responding;    	//modem.h

//extern unsigned int current_duty_high[MAX_INPUTS];
//extern unsigned int current_duty_count[MAX_INPUTS];
//extern unsigned int last_duty_high[MAX_INPUTS];
//extern unsigned int last_duty_count[MAX_INPUTS]; //global.h
//extern char modem_rx_string[MODEM_SWBUFFER_LEN]; //global.h

//extern char timer; 					//mmc.h
//extern bool timer_overflow;			//config.h

//#if MODEM_TYPE == Q24NG_PLUS
//extern char gprs_state;          		//unused
//#endif

//#if TX_BUFFER_SIZE0<256				//uart.h
//extern unsigned char tx_wr_index0,tx_rd_index0,tx_counter0;
//#else
//extern unsigned int tx_wr_index0,tx_rd_index0,tx_counter0;
//#endif
//extern char tx_buffer0[TX_BUFFER_SIZE0];	//uart.h
//extern bit uart0_held;					//uart.h

//extern unsigned int rx_counter0;
//extern unsigned int rx_counter1;

//extern char click_back_in[4];			//output.h


//unsigned long log_entry_timer = 5;
//unsigned int log_update_timer = 1;
//bool mmc_fail_sent=false; //mmc.h
char minute_timer = 60;					//global scope removed
char led_state=0;

/* Timer1 Globals
 * This is used for timing in the main loop to indicate a 15min interval, when the
 * unit needs to check reception and if there are any sms's waiting in the inbox
 */
volatile unsigned int timer_count;                     //Counter
volatile bool timer_ovf;                       //Flag there is an overflow
#define FIVE_MIN_OVF 4                     //Five minute overflow
#define TEN_MIN_OVF 9

bit input_int_sms;			//SJL - toggle to send input sms messages from adc_int adc_isr
							//disabled during startup
//char net_check_timer = 92;	//SJL - check network on startup
// 32s delay required to allow for initialization first
//bool startup_send_status = false;	//SJL - extern in global.h to be accessed by uart.c - AT+CONFIG
//char startup_send_delay = 1;
//eeprom char *contact_one_eeprom;
//char char1_temp[2], char2_temp[1];

//#if PULSE_COUNTING_AVAILABLE
//unsigned long last_second; //this_second[2]
//unsigned int seconds_recorded=0; //minute_count=0, hour_count=0,
//#endif


// Timer 3 overflow interrupt service routine
interrupt [TIM3_OVF] void timer3_ovf_isr(void)
{
    unsigned char i;
    Event e;
	char hr,min,sec;

	//#if PULSE_COUNTING_AVAILABLE
	//unsigned int second_count=0;
	//#endif
	
    #ifdef INT_TIMING
    start_x_int();
    start_int();
    #endif
    // Reinitialize Timer 3 value
    #if CLOCK == SIX_MEG
    TCNT3H=0xE9;
    TCNT3L=0x1C;
    #elif CLOCK == SEVEN_SOMETHING_MEG
    TCNT3H=0xE3;
    TCNT3L=0xDF;
    #endif
	
	//sprintf(buffer,"[TIM3_OVF] Timer interrupt\r\n");print1(buffer);

    //allow this function to be interrupted
    global_interrupt_on();
	
	minute_timer--;
	//sprintf(buffer,"minute_timer = %d\r\n",minute_timer);print1(buffer);
	
	//PULSE - Instantaneous Frequency Calculation	
	//this_second[1] = pulse_aggregate[1];
	//record the number of seconds pulses have been counted
	//seconds_recorded++;
	//caluclate the number of pulses in the last second
	//second_count = pulse_aggregate[1] - last_second;		
	//debug
	//sprintf(buffer,"s=%u,#=%u,p/s=%u\r\n",seconds_recorded,pulse_aggregate[1],second_count);print1(buffer);	
	//last_second = pulse_aggregate[1];
	
	
	
	// SJL - check network every 60 seconds
    // sprintf(buffer,"net_check_timer = %d\r\n",net_check_timer);print(buffer);	//SJL - Debug
	// net_check_timer--;
	// if(net_check_timer==0)
	// {
		// e.type = sms_CHECK_NETWORK_INTERNAL;
		// queue_push(&q_modem, &e);
		// net_check_timer = 60;
	// }

    //SJL - on startup/reset send a status message to primary contact
    // startup_send_status - delay set after initialization
    // This has been changed to just queuing the event at startup and after config dl
//	if(startup_send_status == true)
//    {
//    	startup_send_delay--;
//		if(startup_send_delay == 0)
//    	{
//        	startup_send_status = false;
//        	contact_one_eeprom = contact_read(0);
//        	char1_temp[0] = contact_one_eeprom[0];
//    		char2_temp[0] = contact_one_eeprom[1];
//        	strcat(char1_temp,char2_temp);
//          sprintf(buffer,"\r\nThe first two chars are : %s\r\n\r\n",char1_temp);
//          print(buffer);
//        	if (strncmpf(char1_temp,"S<",1) == 0)
//        	{
//            	e.type = sms_RESET_STATUS;
//            	e.param = 0;
//            	queue_push(&q_modem, &e);
//            	strcpy(char1_temp,"");
//        	}
//    	}
//    }

    for(i=0;i<4;i++)
    {
		if(config.output[i].enabled)	//added 3.xx.7
		{
			if(click_back_in[i]>0)
			{
				click_back_in[i]--;
				#ifdef _SMS_DEBUG_
				sprintf(buffer,"Output %d, Countdown %d\r\n",i+1,click_back_in[i]);
				print(buffer);
				#endif
				if(click_back_in[i] == 0)
				{				
					switch(config.output[i].config.default_state)
					{
						case ON:
							e.param = i;
							e.type = event_OUT_ON;
							queue_push(&q_event,&e);
							break;
						case OFF:
							#ifdef _SMS_DEBUG_
							sprintf(buffer,"8d\r\n");print1(buffer);
							#endif
							e.param = i;
							e.type = event_OUT_OFF;
							queue_push(&q_event,&e);
							break;
						default:
							break;
					}
				}
			}
		}	
	}

    timer--;
	if(timer<=0)
    {
        timer_overflow=true;
    }

    #if (HW_REV == V5 || HW_REV == V6)
    if(csd_connected)
    {
        _status_led_toggle();
    }
    #endif

   #if EXTERNAL_UART
    if(command_ready2())
    {
        e.type = event_EXTERNAL_UART;
        queue_push(&q_event,&e);
    }
   #endif

    #if LOGGING_AVAILABLE
    //if(!log_entry_timer--)
	log_entry_timer--;
	if(log_entry_timer==0)
    {
        if(!log_event_on_queue)
        {
            if(config.data_log_enabled)
            {
                log_event_on_queue = true;
                e.type = event_LOG_ENTRY;
                queue_push(&q_event, &e);
				//sprintf(buffer,"\r\n*** ADDING LOG ENTRY EVENT TO QUEUE ***\r\n\r\n");print1(buffer);
            }
        }
        log_entry_timer = config.log_entry_period + 1;
    }
    if(log_entry_timer<0)
        log_entry_timer = -1;

    if((!log_entry_timer)&&config.data_log_enabled&&led_state!=LED_POWER_FLASH)
        power_led_off(); //give  1 second to remove the card before destroying the FAT table
    else if(!(log_entry_timer%5)||log_entry_timer==-1) //5 - cahnged to 30 for debugging events
	//divide by 5 remainder 0
	{
		//sprintf(buffer,"log_entry_timer=%X",log_entry_timer);print1(buffer);
        if(!mmc_init_event_on_queue)
        {
            mmc_init_event_on_queue=true;
            e.type = event_MMC_INIT;
            queue_push(&q_event, &e);
        }
	}
    #endif
    //every 60 seconds decrement the minute counter

   // if(!(minute_timer%10))
   // {
       // e.type = event_THRU_MODE;
       // queue_push(&q_event, &e);
   // }

    //if(!--minute_timer)
	if(minute_timer==0)
    {
        //shift the current duty rate info out
		
		//rtc_get_time(&hr,&min,&sec);
		//sprintf(buffer,"%02d:%02d:%02d\r\n",hr,min,sec);
		//print1(buffer);
		
		minute_timer = 60; //seconds in a minute - was wrongly set to 59	
		
		//SJL 120112
		//Check the network status - modem (MC8795V) sometimes don't pass CREG messges when required
		//sprintf(buffer,"60 seconds - flag = %d\r\n",check_status_event_on_queue);print1(buffer);
		if(check_status_event_on_queue==false)
		{
			//sprintf(buffer,"Queue check status\r\n");print1(buffer);
			check_status_event_on_queue=true;
			e.type = event_CHECK_NETWORK_STATUS;
			e.param = false;
			queue_push(&q_event, &e);
		}
		
        for(i=0;i<MAX_INPUTS;++i)
        {
            if(config.input[i].type == DIGITAL)
            {
                global_interrupt_off();
                last_duty_high[i] = current_duty_high[i];
                last_duty_count[i] = current_duty_count[i];
                current_duty_high[i] = current_duty_count[i] = 0;
                global_interrupt_on();
            }
        }

        if (timer_count<=0)
        {
			timer_count--;
            timer_ovf = true;
            timer_count = TEN_MIN_OVF;
            if(modem_TMEnabled)
            {
                if(long_time_up)
                    #asm ("jmp 0x00");
                long_time_up=true;
            }
            else
                long_time_up=false;
        }
		else
		{
			timer_count--;
		}	
        //if(!log_update_timer--)
		log_update_timer--;
		if(log_update_timer==0)
        {
			if(!email_log_event_on_queue)
			{
				if(config.data_log_enabled)
				{
					email_log_event_on_queue = true;
					mmc_fail_sent=false;
					e.type = sms_LOG_UPDATE;
					e.param = 0;
					queue_push(&q_modem, &e);
				}				
			}
			log_update_timer = config.log_update_period + 1;
        }		
        if(log_update_timer<0)
            log_update_timer = -1;
        
		
		//if(!email_retry_timer--)
		email_retry_timer--;
		if(email_retry_timer==0)
		{	
			if(!email_log_event_on_queue)
			{
				if(config.data_log_enabled)
				{
					email_log_event_on_queue = true;
					mmc_fail_sent=false;
					e.type = sms_LOG_UPDATE;
					e.param = 0;
					queue_push(&q_modem, &e);
				}				
			}
			email_retry_timer = -1;
		}		
		if(email_retry_timer<0)
            email_retry_timer = -1;		
    }

    #ifdef INT_TIMING
    stop_int();
    #endif
}
//extern eeprom char saved_state[MAX_INPUTS][2];
void main(void)
{
    Event q_newEvent;
    //unsigned char old_write = 0;
    char i,j,h,m,s;
    //unsigned char found_version = 0;	//SJL - CAVR2 variable was set but not used

    //discover the version of this hardware
    sbi(DDRG,0x10);
    cbi(PORTG,0x10);
    cbi(DDRG,0x10);
    delay_us(1);
    //if(PING&0x10)				//SJL - CAVR2 variable was set but not used
    //    found_version = V5;  	//SJL - CAVR2 variable was set but not used
    //else found_version = V2; 	//SJL - CAVR2 variable was set but not used


    //Init the DDR and initial state of ports
    port_init();

    _active_led_on();
    _power_led_on();
    _status_led_on();
    delay_ms(200);
    _active_led_off();
    _power_led_off();
    _status_led_off();
    delay_ms(200);


    power_led_on();
    active_led_on();
    status_led_off();

    modem_not_responding = false;

    
	//Restart the modem to a known state
	DDRD.6 = 1;
	PORTD.6 = 0;
	delay_ms(1000);
	PORTD.6 = 1;
	delay_ms(10);
	PORTD.6 = 0;
	//Delay required before modem can be initialized AND UARTS can be set up
	delay_ms(15100);
	
	//Init both uart ports
    uart1_init(&uart1_setup);
    uart0_init(9600);
	
	if(config_cmgs)
	{
		sprintf(buffer,"**********************************************************\r\n");print1(buffer);
		sprintf(buffer,"Unit Initializing...\r\n");print1(buffer);
		sprintf(buffer,"Modem Initializing, please wait\r\n");print(buffer);
	}
	
    build_type = SIERRA_MC8795V;	//SJL - debug - commented out
    modem_init(build_type); 		//SJL - debug - commented out

	if(config_cmgs)
	{
		sprintf(buffer,"                                - OK\r\n");print(buffer);
	}
	
    //force the modem into a known state
    global_interrupt_on();
	
	//modem_restart();	//SJL - debug modem start up
		
	//Reset Source checking
    if (MCUCSR & 1)
    {
        // Power-on Reset
        MCUCSR&=0xE0;
        DEBUG_printStr("Power-on Reset\r\n");
		//sprintf(buffer,"Power-on Reset\r\n");print1(buffer);
        if (error.powerOnReset != 0xFFFF)
            error.powerOnReset++;
        #if SYSTEM_LOGGING_ENABLED
        sprintf(buffer,"Unit repowered");
        log_line("system.log",buffer);
        #endif
    }
    else if (MCUCSR & 2)
    {
       // External Reset
       MCUCSR&=0xE0;
       DEBUG_printStr("External Reset\r\n");
	   //sprintf(buffer,"External Reset\r\n");print1(buffer);
       if (error.externalReset != 0xFFFF)
            error.externalReset++;
       #if SYSTEM_LOGGING_ENABLED
       sprintf(buffer,"External reset");
       log_line("system.log",buffer);
       #endif

    }
    else if (MCUCSR & 4)
    {
       // Brown-Out Reset
       MCUCSR&=0xE0;
       DEBUG_printStr("Brown-out Reset\r\n");
	   //sprintf(buffer,"Brown-out Reset\r\n");print1(buffer);
       if (error.brownOutReset != 0xFFFF)
            error.brownOutReset++;
       #if SYSTEM_LOGGING_ENABLED
       sprintf(buffer,"Unit brownout");
       log_line("system.log",buffer);
       #endif
    }
    else if (MCUCSR & 8)
    {
       // Watchdog Reset
       MCUCSR&=0xE0;
       DEBUG_printStr("Watchdog Reset\r\n");
	   //sprintf(buffer,"Watchdog Reset\r\n");print1(buffer);
       if (error.watchdogReset != 0xFFFF)
            error.watchdogReset++;
       #if SYSTEM_LOGGING_ENABLED
       sprintf(buffer,"Watchdog reset");
       log_line("system.log",buffer);
       #endif
    }
    else if (MCUCSR & 0x10)
    {
       // JTAG Reset
       MCUCSR&=0xE0;
       DEBUG_printStr("JTAG Reset\r\n");
	   //sprintf(buffer,"JTAG Reset\r\n");print1(buffer);
       if (error.jtagReset != 0xFFFF)
            error.jtagReset++;
       #if SYSTEM_LOGGING_ENABLED
       sprintf(buffer,"JTag reset");
       log_line("system.log",buffer);
       #endif
    }
    else
    {
        DEBUG_printStr("Unknown Reset\r\n");
		//sprintf(buffer,"Unknown Reset\r\n");print1(buffer);
        if (error.unknownReset != 0xFFFF)
            error.unknownReset++;
        #if SYSTEM_LOGGING_ENABLED
        sprintf(buffer,"Unknown reset (crash)");
        log_line("system.log",buffer);
        #endif

    }
	
	//SREG &= ~(0x80);	//SJL added to protect sedateRover()	
	//tickleRover();		//SJL added to protect sedateRover()
	sedateRover();
	//SREG |= 0x80;		//SJL added to protect sedateRover()

    //Only print out if the user wants start up messages to be printed.
    /* if (config_cmgs)
    {
		sprintf(buffer,"**********************************************************\r\n");print1(buffer);		
        //Make the bell sound
        //putchar1(7);
        //Startup Welcome message
        #if PRODUCT==SMS300 && MODEM_TYPE==Q24NG_PLUS
        sprintf(buffer,"EDAC 053-0028 Rev: ");
        #elif PRODUCT==SMS300 && MODEM_TYPE==Q2406B
        sprintf(buffer,"EDAC 053-0027 Rev: ");
        #elif PRODUCT==SMS300 && MODEM_TYPE==Q2438F
        sprintf(buffer,"EDAC 053-0029 Rev: ");
        #elif PRODUCT==EDAC310
        sprintf(buffer,"EDAC 053-00xx Rev: ");
        #elif PRODUCT==EDAC320 && MODEM_TYPE==Q24NG_PLUS
        sprintf(buffer,"EDAC 053-0031 Rev: ");
        #elif PRODUCT==EDAC320 && MODEM_TYPE==SIERRA_MC8775V
        sprintf(buffer,"EDAC 053-0032 Rev: ");
        #elif PRODUCT==EDAC321 && MODEM_TYPE==Q24NG_PLUS
        sprintf(buffer,"EDAC 053-0033 Rev: ");
        #elif PRODUCT==EDAC321 && MODEM_TYPE==Q2438F
        sprintf(buffer,"EDAC 053-0034 Rev: ");
        #elif PRODUCT==SMS300Lite
        sprintf(buffer,"EDAC 053-00xx Rev: ");
        #elif PRODUCT==HERACLES //315-NG
        sprintf(buffer,"EDAC 315-NG 053-0030 Rev: ");
		//#elif PRODUCT==EDAC315 && MODEM_TYPE==SIERRA_MC8795V
        //sprintf(buffer,"EDAC 315 053-0044 Rev: ");	//SJL - consolidated code for all 300 series projects - only 315 so far implemented
        #elif PRODUCT==EDAC320 && MODEM_TYPE==SIERRA_MC8795V
		sprintf(buffer,"EDAC 320-NG 053-0032 Rev: ");
		#elif PRODUCT==EDAC321 && MODEM_TYPE==SIERRA_MC8795V
		sprintf(buffer,"EDAC 321-NG 053-0041 Rev: ");
		#elif PRODUCT==DEVELOPMENT
        sprintf(buffer,"EDAC DEVELOPMENT Rev: ");
        #endif
        print1(buffer);
		
		#ifdef BETA
		sprintf(buffer,"%p.%pb\r\n",code_version,buildnumber);
		#else
		sprintf(buffer,"%p.%p\r\n",code_version,buildnumber);
		#endif
		print(buffer);


        #ifdef _DEBUG_
        sprintf(buffer,"_DEBUG_ enabled\n\r");
        print1(buffer);
        #endif
//        #ifdef _MODEM_DEBUG_
//        sprintf(buffer,"_MODEM_DEBUG_ enabled\n\r");
//        print(buffer);
//        #endif
        #ifdef _ALARM_DEBUG_
        sprintf(buffer,"_ALARM_DEBUG_ enabled\n\r");
        print1(buffer);
        #endif
        #ifdef _INPUT_DEBUG_
        sprintf(buffer,"_INPUT_DEBUG_ enabled\n\r");
        print1(buffer);
        #endif
		
		#ifdef BETA
		sprintf(buffer,"\r\nBETA Version FOR TEST PURPOSES ONLY\n\r");print1(buffer);
		#ifdef EMAIL_DEBUG
		sprintf(buffer,"Email debug   ENABLED\n\r");print1(buffer);
		#endif
		#ifdef SOCKET_DEBUG
		sprintf(buffer,"Socket debug  ENABLED\n\r");print1(buffer);
		#endif
		#ifdef DNS_DEBUG
		sprintf(buffer,"DNS debug     ENABLED\n\r");print1(buffer);
		#endif
		#ifdef SMTP_DEBUG	
		sprintf(buffer,"SMTP debug    ENABLED\n\r");print1(buffer);
        #endif
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"SD card debug ENABLED\n\r\n\r");print1(buffer);
		#endif
		#endif
		
		#if SMS_AVAILABLE
        sprintf(buffer,"Build: SMS available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"Build: SMS unavailable\n\r");
        print1(buffer);
        #endif

        //#if TCPIP_AVAILABLE
        //sprintf(buffer,"       TCP/IP available\n\r");
        //print(buffer);
        //#else
        //sprintf(buffer,"       TCP/IP unavailable\n\r");
        //print(buffer);
        //#endif

        #if EMAIL_AVAILABLE
        sprintf(buffer,"       Email available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"       Email unavailable\n\r");
        print1(buffer);
        #endif

        #if LOGGING_AVAILABLE
        sprintf(buffer,"       Logging available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"       Logging unavailable\n\r");
        print1(buffer);
        #endif

        #if SYSTEM_LOGGING_ENABLED
        sprintf(buffer,"       System logging available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"       System unavailable\n\r");
        print1(buffer);
        #endif

        #if PULSE_COUNTING_AVAILABLE
        sprintf(buffer,"       Pulse counting available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"       Pulse counting unavailable\n\r");
        print1(buffer);
        #endif
		
		#ifdef DEBUG 
			#if CONNECT_ONE_AVAILABLE
			sprintf(buffer,"       ConnectOne available\n\r");
			print1(buffer);
			#else
			sprintf(buffer,"       ConnectOne unavailable\n\r");
			print1(buffer);
			#endif
		#endif		
		
        sprintf(buffer,"       Version %d Hardware\r\n",HW_REV);
        print1(buffer);

        switch(MODEM_TYPE)
        {
            case Q2406B:
                sprintf(buffer,"       Q2406B Modem\r\n");
                break;
            case Q2438F:
                sprintf(buffer,"       Q2438F Modem\r\n");
                break;
            case Q2438_NEW:
                sprintf(buffer,"       Q2438 New Modem\r\n");
                break;
            case Q24NG_CLASSIC:
                sprintf(buffer,"       Q24NG Classic Modem\r\n");
                break;
            case Q24NG_PLUS:
                sprintf(buffer,"       Q24NG Plus Modem\r\n");
                break;
            case SIERRA_MC8775V:
                sprintf(buffer,"       Sierra MC8775 Modem\r\n");
                break;
			case SIERRA_MC8795V:
			//SJL - added MC8795V modem - only difference from MC8775 in FW is added delays
			//main.c - line 723 - delay_ms(13000);
			//modem.c - modem_wait_for - line 2086 - delay_ms(8);
                sprintf(buffer,"       Sierra MC8795V Modem\r\n");print(buffer);
				sprintf(buffer,"       GSM/GPRS/EDGE 850/900/1800/1900 MHz\r\n");print(buffer);
				sprintf(buffer,"       WCDMA/HSPA 800/850/900/1900/2100 MHz\r\n");
                break;
            default:
                sprintf(buffer,"       Unknown Modem\r\n");
                break;
        }
        print1(buffer);


        sprintf(buffer,"\r\n(c) Copyright EDAC Electronics LTD 2006\r\n\r\n");print1(buffer);
		sprintf(buffer,"**********************************************************\r\n");print1(buffer);
		
		sprintf(buffer,"Unit Initializing...\r\n");print1(buffer);
    } */

    delay_ms(200);

    wakeRover();
    tickleRover();
		
    //ensure global interrupts are disabled
    global_interrupt_off();
    #if LOGGING_AVAILABLE
	if(config_cmgs)
	{
		sprintf(buffer,"Memory Card Initializing       ");print(buffer);	//SJL - 290611
	}
	if(!initialize_media())
	{
        power_led_flashing();
		if(config_cmgs)
		{
			sprintf(buffer," - FAILED\r\n");print(buffer);
		}
	}
    else
    {
        power_led_on();
		if(config_cmgs)
		{
			sprintf(buffer," - OK\r\n");print(buffer);
		}
    }	
    #else
    power_led_on();
    #endif

    //I2C Bus initialization
	if(config_cmgs)
	{
		sprintf(buffer,"I2C Initializing               ");print(buffer);	//SJL - 290611
	}
	i2c_init();
	if(config_cmgs)
	{
		sprintf(buffer," - OK\r\n");print(buffer);	//SJL - 290611
	}

    delay_ms(100);

    // Global enable interrupts
    global_interrupt_on();
	
	if(config_cmgs)
	{
		sprintf(buffer,"Initial values                  - ");print(buffer);		
	}
    if(!(adc_voltage_offset[-1]==0xAA&&adc_voltage_offset[-2]==0x55))
    {        
        for(i=0;i<MAX_INPUTS;i++)
        {
            #if HW_REV == V5 || HW_REV == V6
            adc_voltage_offset[i] = 127;
            adc_current_offset[i] = 127;
            #else
            adc_voltage_offset[i] = 127;
            adc_current_offset[i] = 127;
            #endif
        }
        strcpyef(serial,"NOTSET\0");
        adc_voltage_offset[-1] = 0xAA;
        adc_voltage_offset[-2] = 0x55;
		if(config_cmgs)
        {
            //sprintf(buffer,"Setting initial values to stable eeprom");
			sprintf(buffer,"Set to stable eeprom\r\n");
			print(buffer);
        }
    }
    else
    {
        if(config_cmgs)
        {
            //sprintf(buffer,"Initial values already set");
			sprintf(buffer,"Already set\r\n");
            print(buffer);
        }
    }
	
    //Make sure the watchdog is cleared
    tickleRover();
    delay_ms(250);

    //Init the inputs
	if(config_cmgs)
	{
		sprintf(buffer,"Initializing Inputs            ");print(buffer);	//SJL - 290611
	}
	input_init();
	if(config_cmgs)
	{
		sprintf(buffer," - OK\r\n");print(buffer);		//SJL - 290611
	}
    

    input_int_sms = false;	//SJL - disable input sms messages from adc_isr during startup_config_reset
    //other initial ADC reads are started by the timer, input 1 is missed
    input_adcStart(0);

    tickleRover();
    pulse_restart();
    //queue_print(&q_event);	//SJL - Debug
    startup_config_reset();
    //queue_print(&q_event);	//SJL - Debug
    input_resume();

    input_int_sms = true;	//SJL - enable input sms messages from adc_isr

    modem_register=true;
  
    // printf("AT+CREG=1\r\n");
    // modem_wait_for(MSG_OK | MSG_ERROR);
    // DEBUG_printStr("Saving, calling AT&W\r\n");
    // printf("AT&W\r\n");
    // modem_wait_for(MSG_OK | MSG_ERROR);
    // modem_register=false;

    //Init the globals
    timer_count=0;
    timer_ovf=0;
    modem_TMEcho = false;
    modem_TMEnabled = false;
    long_time_up=false;
    uart_paused = false;
    modem_paused = false;    

    //start the timers at the right values
    log_entry_timer = config.log_entry_period;
	if(config.log_entry_period<0)
	{
		log_entry_timer = -1;	//config.log_entry_period;
	}
	else
	{
		log_entry_timer = config.log_entry_period+1;
	}
    
	log_update_timer = config.log_update_period;	//debug	=3
	if(config.log_update_period<0)
	{
		log_update_timer = -1;	//config.log_update_period;
	}
	else
	{
		log_update_timer = config.log_update_period+1;
	}
		
	//DS1337 Real time Clock Init
	if(config_cmgs)
	{
		sprintf(buffer,"RTC Initializing               ");print(buffer);
	}
	rtc_init();
	if(config_cmgs)
	{
		sprintf(buffer," - OK\r\n");print(buffer);
		//sprintf(buffer,"Modem Initializing, please wait\r\n");print(buffer);
	}
	
	//build_type = SIERRA_MC8795V;
    //modem_init(build_type);
		
	//just to make sure...
    printf("AT+CREG=1\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR);
	
	printf("AT+CNMI=2,1\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR);
	printf("AT+CMGF=1\r\n");
	modem_wait_for(MSG_OK | MSG_ERROR);
		
    DEBUG_printStr("Saving, calling AT&W\r\n");
    printf("AT&W\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR);
    modem_register=false;
	
	minute_timer = 60; //SJL 120112
	
	//Init complete so turn off the active led.
    active_led_off();
	
	if(config_cmgs)
	{
		sprintf(buffer,"Initialization Complete\r\n");print(buffer);
		sprintf(buffer,"\r\n**********************************************************\r\n");print(buffer);
	}
	
	if (config_cmgs)
    {
		//sprintf(buffer,"**********************************************************\r\n");print1(buffer);		
        //Make the bell sound
        //putchar1(7);
        //Startup Welcome message
        #if PRODUCT==SMS300 && MODEM_TYPE==Q24NG_PLUS
        sprintf(buffer,"EDAC 053-0028 Rev: ");
        #elif PRODUCT==SMS300 && MODEM_TYPE==Q2406B
        sprintf(buffer,"EDAC 053-0027 Rev: ");
        #elif PRODUCT==SMS300 && MODEM_TYPE==Q2438F
        sprintf(buffer,"EDAC 053-0029 Rev: ");
        #elif PRODUCT==EDAC310
        sprintf(buffer,"EDAC 053-00xx Rev: ");
        #elif PRODUCT==EDAC320 && MODEM_TYPE==Q24NG_PLUS
        sprintf(buffer,"EDAC 053-0031 Rev: ");
        #elif PRODUCT==EDAC320 && MODEM_TYPE==SIERRA_MC8775V
        sprintf(buffer,"EDAC 053-0032 Rev: ");
        #elif PRODUCT==EDAC321 && MODEM_TYPE==Q24NG_PLUS
        sprintf(buffer,"EDAC 053-0033 Rev: ");
        #elif PRODUCT==EDAC321 && MODEM_TYPE==Q2438F
        sprintf(buffer,"EDAC 053-0034 Rev: ");
        #elif PRODUCT==SMS300Lite
        sprintf(buffer,"EDAC 053-00xx Rev: ");
        #elif PRODUCT==HERACLES //315-NG
        sprintf(buffer,"EDAC 315-NG 053-0030 Rev: ");
		//#elif PRODUCT==EDAC315 && MODEM_TYPE==SIERRA_MC8795V
        //sprintf(buffer,"EDAC 315 053-0044 Rev: ");	//SJL - consolidated code for all 300 series projects - only 315 so far implemented
        #elif PRODUCT==EDAC320 && MODEM_TYPE==SIERRA_MC8795V
		sprintf(buffer,"EDAC 320-NG 053-0032 Rev: ");
		#elif PRODUCT==EDAC321 && MODEM_TYPE==SIERRA_MC8795V
		sprintf(buffer,"EDAC 321-NG 053-0041 Rev: ");
		#elif PRODUCT==DEVELOPMENT
        sprintf(buffer,"EDAC DEVELOPMENT Rev: ");
        #endif
        print1(buffer);
		
		#ifdef BETA
		sprintf(buffer,"%p.%pb\r\n",code_version,buildnumber);
		#else
		sprintf(buffer,"%p.%p\r\n",code_version,buildnumber);
		#endif
		print(buffer);


        #ifdef _DEBUG_
        sprintf(buffer,"_DEBUG_ enabled\n\r");
        print1(buffer);
        #endif
//        #ifdef _MODEM_DEBUG_
//        sprintf(buffer,"_MODEM_DEBUG_ enabled\n\r");
//        print(buffer);
//        #endif
        #ifdef _ALARM_DEBUG_
        sprintf(buffer,"_ALARM_DEBUG_ enabled\n\r");
        print1(buffer);
        #endif
        #ifdef _INPUT_DEBUG_
        sprintf(buffer,"_INPUT_DEBUG_ enabled\n\r");
        print1(buffer);
        #endif
		
		#ifdef BETA
		sprintf(buffer,"\r\nBETA Version FOR TEST PURPOSES ONLY\n\r");print1(buffer);
		#ifdef EMAIL_DEBUG
		sprintf(buffer,"Email debug   ENABLED\n\r");print1(buffer);
		#endif
		#ifdef SOCKET_DEBUG
		sprintf(buffer,"Socket debug  ENABLED\n\r");print1(buffer);
		#endif
		#ifdef DNS_DEBUG
		sprintf(buffer,"DNS debug     ENABLED\n\r");print1(buffer);
		#endif
		#ifdef SMTP_DEBUG	
		sprintf(buffer,"SMTP debug    ENABLED\n\r");print1(buffer);
        #endif
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"SD card debug ENABLED\n\r\n\r");print1(buffer);
		#endif
		#endif
		
		#if SMS_AVAILABLE
        sprintf(buffer,"Build: SMS available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"Build: SMS unavailable\n\r");
        print1(buffer);
        #endif

        //#if TCPIP_AVAILABLE
        //sprintf(buffer,"       TCP/IP available\n\r");
        //print(buffer);
        //#else
        //sprintf(buffer,"       TCP/IP unavailable\n\r");
        //print(buffer);
        //#endif

        #if EMAIL_AVAILABLE
        sprintf(buffer,"       Email available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"       Email unavailable\n\r");
        print1(buffer);
        #endif

        #if LOGGING_AVAILABLE
        sprintf(buffer,"       Logging available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"       Logging unavailable\n\r");
        print1(buffer);
        #endif

        #if SYSTEM_LOGGING_ENABLED
        sprintf(buffer,"       System logging available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"       System unavailable\n\r");
        print1(buffer);
        #endif

        #if PULSE_COUNTING_AVAILABLE
        sprintf(buffer,"       Pulse counting available\n\r");
        print1(buffer);
        #else
        sprintf(buffer,"       Pulse counting unavailable\n\r");
        print1(buffer);
        #endif
		
		#ifdef DEBUG 
			#if CONNECT_ONE_AVAILABLE
			sprintf(buffer,"       ConnectOne available\n\r");
			print1(buffer);
			#else
			sprintf(buffer,"       ConnectOne unavailable\n\r");
			print1(buffer);
			#endif
		#endif		
		
        sprintf(buffer,"       Version %d Hardware\r\n",HW_REV);
        print1(buffer);

        switch(MODEM_TYPE)
        {
            case Q2406B:
                sprintf(buffer,"       Q2406B Modem\r\n");
                break;
            case Q2438F:
                sprintf(buffer,"       Q2438F Modem\r\n");
                break;
            case Q2438_NEW:
                sprintf(buffer,"       Q2438 New Modem\r\n");
                break;
            case Q24NG_CLASSIC:
                sprintf(buffer,"       Q24NG Classic Modem\r\n");
                break;
            case Q24NG_PLUS:
                sprintf(buffer,"       Q24NG Plus Modem\r\n");
                break;
            case SIERRA_MC8775V:
                sprintf(buffer,"       Sierra MC8775 Modem\r\n");
                break;
			case SIERRA_MC8795V:
			//SJL - added MC8795V modem - only difference from MC8775 in FW is added delays
			//main.c - line 723 - delay_ms(13000);
			//modem.c - modem_wait_for - line 2086 - delay_ms(8);
                sprintf(buffer,"       Sierra MC8795V Modem\r\n");print(buffer);
				sprintf(buffer,"       GSM/GPRS/EDGE 850/900/1800/1900 MHz\r\n");print(buffer);
				sprintf(buffer,"       WCDMA/HSPA 800/850/900/1900/2100 MHz\r\n");
                break;
            default:
                sprintf(buffer,"       Unknown Modem\r\n");
                break;
        }
        print1(buffer);


        sprintf(buffer,"\r\n(c) Copyright EDAC Electronics LTD 2006\r\n\r\n");print1(buffer);
		sprintf(buffer,"**********************************************************\r\n");print1(buffer);	
		
    }

    if(config.autoreport.type != NO_ALARM)
    {
	rtc_set_alarm_1(config.autoreport.day,config.autoreport.hour,config.autoreport.minute,
		    config.autoreport.second,config.autoreport.type);
	print("RTC set alarm\r\n");
    }

    //main control loop.  this is executed constantly
    while(1)
    {
        //Clear the watchdog
        tickleRover();
        _active_led_off();


        //check if in modem through mode (csd) and transparent, so all char's
        //received down the serial port are forwarded to the modem
        if (modem_TMEcho)
        {
            if(!modem_throughMode())
            {
                //The through mode connection has been lost
                modem_TMEnabled = false;
                long_time_up = false;
                queue_resume(&q_modem);
            }
        }
        //otherwise the commands from the modem are read in and parsed
        else
        {
            //If modem is in through mode, the getchar for uart is pointed to the modem
            //getchar routine, so only characters coming from the modem are parsed.  as
            //it is in through mode they are treated like you are connected directly to
            //the external serial port.
            if (modem_TMEnabled)
            {
                if (!uart_paused)
                {
                    //The uart has some data that is ready to read in and processed
                    if (uart_readPort())
                    {
                        //A \r was found in the string, and therefore the command is complete
                        //Enumerate the string that has been received.
                        uart_paused = uart_handleNew(uart_rx_string);
                    }
                }
            }
            else
            {
                //this is normal operation, commands from the uart and modem are parsed and
                //handled.
                if (!uart_paused)
                {
                    //The uart has some data that is ready to read in and processed
                    if (uart_readPort())
                    {

                    	//sprintf(buffer,"Read UART");				//SJL - CAVR2
    					//log_line("system.log",buffer);              //SJL - CAVR2

                        tickleRover();
                        //A \r was found in the string, and therefore the command is complete
                        //Enumerate the string that has been received.
                        uart_paused = uart_handleNew(uart_rx_string);
                    }
                }
                //The modem has some data that is ready to be read in
                if (!modem_paused)
                {
                	//sprintf(buffer,"Read MODEM - %c",UDR0);print(buffer);				//SJL - CAVR2
                    //if(rx_counter0 > 0)
                    //{
                    //	sprintf(buffer,"main.c - rx_counter0 = %d\r\n",rx_counter0);print(buffer);
                    //}

                    if (modem_readPort())
                    {
                    	//print("main.c - Modem command recieved: "); //SJL - debug
                        //sprintf(buffer,"%s\r\n",modem_rx_string);print(buffer);
                        modem_paused = modem_handleNew();
                    }
                }
            }
        }

        //sprintf(buffer,"Back in main control loop");	//SJL - CAVR2
    	//log_line("system.log",buffer);                	//SJL - CAVR2

        //If there is an event waiting in the queue, pop it off
        if(queue_pop(&q_event,&q_newEvent))
        {
        	//sprintf(buffer,"EVENT Queue Pop");			//SJL - CAVR2
    		//log_line("system.log",buffer);              //SJL - CAVR2

            _active_led_off();
            #ifdef EVENT_DEBUG
            	rtc_get_time(&h,&m,&s);
           		sprintf(buffer,"%02d:%02d:%02d  EVENT_Q_POP - Event = %d  Param = %d Length = %d\r\n",h,m,s,q_newEvent.type,q_newEvent.param, q_event.length);
           		print(buffer);
                //log_line("system.log",buffer);
            #endif

            //Run the new event
            event_handleNew (&q_newEvent);
        }

        //If there is an event waiting in the queue, pop it off
        else if(!modem_TMEnabled)
        {
            _active_led_off();
            if(queue_pop(&q_modem,&q_newEvent))

            //sprintf(buffer,"MODEM Queue Pop");			//SJL - CAVR2
    		//log_line("system.log",buffer);              //SJL - CAVR2

            {
                #ifdef EVENT_DEBUG
                rtc_get_time(&h,&m,&s);
                sprintf(buffer,"%02d:%02d:%02d  SMS_Q_POP - Event = %d  Param = %d Length = %d\r\n",h,m,s,q_newEvent.type,q_newEvent.param, q_modem.length);
                print(buffer);
                #endif
                //Run the new event if the modem was able to init on startup, if it wasn't able to startup
                //there is a network connection problem, and therefore no messages can be sent via the
                //cellular network.  only allow the modem to check it's netowrk connection and signal
                //strength.
                if (!startupError || q_newEvent.type == sms_CHECK_NETWORK || q_newEvent.type == sms_GET_CSQ ||
                    q_newEvent.type == sms_CHECK_NETWORK_INTERNAL)
                {
                    sms_handleNew (&q_newEvent);
                }
                //else don't handle anything, just allow the queue to continue.
                else
                {

                    //DEBUG_printStr("Modem not up, pushing event again\r\n");
                    //push the sms event back onto the queue so it can be handled later.
                    //This has been removed, as not sure if it is a good idea?  may need to only push
                    //back sms sends...???? TODO
                    //DEBUG_printStr("Pushing an event back on the queue\r\n");
                    if(q_newEvent.type != sms_CHECK_MSG)
                        queue_push(&q_modem,&q_newEvent);
                }
            }
        }

        //10 minute timer event to control network and email checking.
        //a similar timer could be used for logging?
        if (timer_ovf)
        {

        	//sprintf(buffer,"10 minute timer overflow"); //SJL - CAVR2
    		//log_line("system.log",buffer);              //SJL - CAVR2

            if(!modem_TMEnabled)
            {
                if(modem_queue_stuck)
                    queue_resume(&q_modem);
                else
                    modem_queue_stuck = true;
            }
            DEBUG_printStr("10 min - chech msg's and network\r\n");
            timer_ovf = false;
            q_newEvent.type = sms_CHECK_NETWORK_INTERNAL;
            queue_push(&q_modem, &q_newEvent);
            
            #if SMS_AVAILABLE
            q_newEvent.type = sms_CHECK_MSG;
            queue_push(&q_modem, &q_newEvent);
            #endif

        }
    } //end of while loop
}


void port_init (void) {

    #if (HW_REV == V2)
        // Input/Output Ports initialization
        // Port A initialization
        // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In
        // State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T
        PORTA=0x00;
        DDRA=0x00;

        // Port B initialization
        // Func7=Out Func6=Out Func5=Out Func4=Out Func3=In Func2=Out Func1=Out Func0=In
        // State7=1 State6=0 State5=0 State4=0 State3=T State2=0 State1=0 State0=T
        PORTB=0x80;
        DDRB=0xF6;

        // Port C initialization
        // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=Out Func1=In Func0=Out
        // State7=T State6=T State5=T State4=T State3=T State2=1 State1=T State0=0
        PORTC=0x00;
        DDRC=0x05;

        // Port D initialization
        // Func7=Out Func6=Out Func5=Out Func4=Out Func3=In Func2=In Func1=Out Func0=Out
        // State7=0 State6=0 State5=0 State4=0 State3=T State2=T State1=0 State0=0
        PORTD=0x00;
        DDRD=0xF3;

        // Port E initialization
        // Func7=Out Func6=Out Func5=Out Func4=Out Func3=Out Func2=Out Func1=In Func0=In
        // State7=1 State6=0 State5=0 State4=0 State3=0 State2=1 State1=T State0=T
        PORTE=0x84;
        DDRE=0xFC;


        // Port F initialization
        // Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In
        // State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T
        PORTF=0x00;
        DDRF=0x00;

        // Port G initialization
        // Func4=In Func3=In Func2=In Func1=Out Func0=Out
        // State4=T State3=T State2=T State1=1 State0=0
        PORTG=0x02;
        DDRG=0x03;
    #elif ( HW_REV == V5 || HW_REV == V6)
        DDRA=0x00;
        PORTA=0x00;     //This is now used for the LCD, so change this when you use it

        DDRB=0xE0;
        PORTB=0xE0;     //Power and Active LED's

        DDRC=0x01;
        PORTC=0x00;     //This is only used for the kpd now

        DDRD=0x60;
        PORTD=0x00;     //i2c is moved here now.  RTS and CTS need to be added later

    #if MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE == SIERRA_MC8795V
       DDRE=0x10;
       PORTE=0x10;
    #else
        DDRE=0x00;
        PORTE=0x00;
    #endif
        DDRF=0x00;
        PORTF=0x00;

        DDRG=0x1F;
        PORTG=0x10; //PORTG.4 is the battery backup pin
    #else
        #error No Hardware Version was selected.
    #endif

    // Timer/Counter 0 initialization
    // Clock source: System Clock
    // Clock value: 5.859 kHz
    // Mode: Normal top=FFh
    // OC0 output: Disconnected
    #if CLOCK == SIX_MEG
    ASSR=0x00;
    TCCR0=0x07;
    TCNT0=0x00;
    OCR0=0x00;
    #elif CLOCK == SEVEN_SOMETHING_MEG
    ASSR=0x00;
    TCCR0=0x07;
    TCNT0=0x00;
    OCR0=0x00;
    #endif

    // Timer/Counter 1 initialization
    // Clock source: System Clock
#if CLOCK == SIX_MEG
    // Clock value: 5.859 kHz
#elif CLOCK == SEVEN_SOMETHING_MEG
    // Clock value: 7.200 kHz
#endif
    // Mode: Normal top=FFFFh
    // OC1A output: Discon.
    // OC1B output: Discon.
    // OC1C output: Discon.
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // Timer 1 Overflow Interrupt: Off
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: On
    // Compare B Match Interrupt: On
    // Compare C Match Interrupt: Off
    #if CLOCK == SIX_MEG
    TCCR1A=0x00;
    TCCR1B=0x05;
    TCNT1H=0x00;
    TCNT1L=0x00;
    ICR1H=0x00;
    ICR1L=0x00;
    OCR1AH=0x00;
    OCR1AL=0x00;
    OCR1BH=0x00;
    OCR1BL=0x00;
    OCR1CH=0x00;
    OCR1CL=0x00;
    #elif CLOCK == SEVEN_SOMETHING_MEG
    TCCR1A=0x00;
    TCCR1B=0x05;
    TCNT1H=0x00;
    TCNT1L=0x00;
    ICR1H=0x00;
    ICR1L=0x00;
    OCR1AH=0x00;
    OCR1AL=0x00;
    OCR1BH=0x00;
    OCR1BL=0x00;
    OCR1CH=0x00;
    OCR1CL=0x00;
    #endif

    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: Timer 2 Stopped
    // Mode: Normal top=FFh
    // OC2 output: Disconnected
    #if CLOCK == SIX_MEG
    TCCR2=0x00;
    TCNT2=0x00;
    OCR2=0x00;
    #elif CLOCK == SEVEN_SOMETHING_MEG
    TCCR2=0x00;
    TCNT2=0x00;
    OCR2=0x00;
    #endif


    // Timer/Counter 3 initialization
    // Clock source: System Clock
    // Clock value: 5.859 kHz (prescaler: 1024)
    // Mode: Normal top=FFFFh
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // OC3A output: Discon.
    // OC3B output: Discon.
    // OC3C output: Discon.
    // Timer 3 Overflow Interrupt: On
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: Off
    // Compare B Match Interrupt: Off
    // Compare C Match Interrupt: Off
    #if CLOCK == SIX_MEG
    TCCR3A=0x00;
    TCCR3B=0x05;
    TCNT3H=0xE9;
    TCNT3L=0x1C;
    ICR3H=0x00;
    ICR3L=0x00;
    OCR3AH=0x00;
    OCR3AL=0x00;
    OCR3BH=0x00;
    OCR3BL=0x00;
    OCR3CH=0x00;
    OCR3CL=0x00;
    #elif CLOCK == SEVEN_SOMETHING_MEG
    TCCR3A=0x00;
    TCCR3B=0x05;
    TCNT3H=0xE3;
    TCNT3L=0xDF;
    ICR3H=0x00;
    ICR3L=0x00;
    OCR3AH=0x00;
    OCR3AL=0x00;
    OCR3BH=0x00;
    OCR3BL=0x00;
    OCR3CH=0x00;
    OCR3CL=0x00;
    #endif


    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: Off
    // INT2: Off
    // INT3: Off
    // INT4: Off
    // INT5: Off
    // INT6: On
    // INT6 Mode: Any change
    // INT7: On
    // INT7 Mode: Any change
    EICRA=0x00;
    EICRB=0x00;
    EIMSK=0x00;
    EIFR=0x00;

    // Timer(s)/Counter(s) Interrupt(s) initialization
    TIMSK=0x1D;
    ETIMSK=0x04;

    // Analog Comparator initialization
    // Analog Comparator: Off
    // Analog Comparator Input Capture by Timer/Counter 1: Off
    ACSR=0x80;
    SFIOR=0x00;



    return;
}


/*
    After a configuartion has been loaded, this function must be called to
    setup all of the correct settings for the inputs and outputs.  it will
    first turn off the input sampling, then set the requested state for the
    outputs.  Then calculate the conversion factors for the analog inputs.
    And send alarms if necessary
*/
void startup_config_reset(void)
{
    unsigned char i;

    tickleRover();

    //Turn the inputs off.
    cbi(TIMSK,0x40);
    delay_ms(100);
    cbi(ADCSRA,0x08);

    //Reset both of the queues.
    queue_clear(&q_modem);
    queue_clear(&q_event);
    log_event_on_queue = false;
	email_log_event_on_queue = false;
    mmc_init_event_on_queue = false;

	//SJL - CAVR2 - debug
    DEBUG_printStr("Running startup config reset...\r\n");

    //handle the outputs
    for (i=0;i<MAX_OUTPUTS;i++) {
        if (config.output[i].enabled)
        {
            //Set the state of the output
            if (config.output[i].config.default_state == LAST_KNOWN)
            {
                output_switch(i,config.output[i].state);
            }
            else
            {
                output_switch(i,config.output[i].config.default_state);
                config.output[i].state=config.output[i].config.default_state;
            }
        }
        else
        {
            output_switch(i,OFF);
            config.output[i].state=OFF;
        }
        tickleRover();
    }

	//SJL - 3.20.8 modification
	//previous -3.20.7: input_setup and input_startup were called from single for loop - resulted in OOB 
	//messages being sent on after config dl even if analogue inputs where in bounds - the sensor thresholds 
	//had not been setup (e.g. input_setup(2)) for later inputs before the adc is started in input_startup
	//(e.g. input_startup(1))
	//3.20.8: input_setup() for all inputs executed before input_startup(0) is executed
	
    //setup the inputs thresholds
    for (i=0;i<MAX_INPUTS;i++)
    {
        if (config.input[i].enabled)
        {
            //Set the conversion factors and thresholds for the inputs.
            input_setup(i);
            //call the input_startup function to see if the inputs are already active and
            //set the appropriate zone.
            //input_startup(i);
        }
        tickleRover();
    }	
	for (i=0;i<MAX_INPUTS;i++)
    {
        if (config.input[i].enabled)
        {
            input_startup(i);
        }
        tickleRover();
    }
	
    //SJL - Debug
    //sprintf(buffer,"startup_config_reset - after input_startup");print(buffer);
    //queue_print(&q_event);
		
    input_resume();
    pulse_restart();
    return;
}

/*
    Start up the watchdog
*/
void wakeRover(void)
{
    // Watchdog Timer initialization
    // Watchdog Timer Prescaler: OSC/2048k
    // This will give a time out of approx 2.2 seconds
    WDTCR=0x1F;
    WDTCR=0x0F;
    return;
}

void sedateRover(void)
{
    //Watchdog Timer shutdown
    WDTCR = (1<<WDCE) | (1<<WDE);
    /* Turn off WDT */
    WDTCR = 0x00;
    return;
}


