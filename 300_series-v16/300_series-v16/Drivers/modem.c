#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <delay.h>
#include <ctype.h>

#include "global.h"
#include "drivers\modem.h"
#include "drivers\event.h"
#include "drivers\sms.h"
#include "drivers\uart.h"
#include "drivers\input.h"
#include "drivers\sms.h"
#include "drivers\debug.h"      //SJL - CAVR2 - added
#include "drivers\ds1337.h"     //SJL - CAVR2 - added
#include "drivers\str.h"        //SJL - CAVR2 - added
#include "drivers\mmc.h"        //SJL - CAVR2 - added
#include "drivers\config.h"     //SJL - CAVR2 - added
#include "drivers\queue.h"      //SJL - CAVR2 - added
//#include "drivers\gprs.h"       //SJL - CAVR2 - added
#include "drivers\error.h"      //SJL - CAVR2 - added


#define FIVE_SECOND_DELAY 305
#define MODEM_READ_MODEM 1
#define MODEM_READ_UART 2
#define MAX_ERROR_RETRY 5
#define MODEM_RSSI_LIMIT 6
#define MAX_REGISTER_ATTEMPTS 50
#define MODEM_TIMEOUT 1000	//3000

char resetting_modem = false;
char modem_not_responding;
char no_carrier_count = 0;
char packeted_data = false;

// Timer 0 overflow interrupt service routine
//Overflows every 0.04369 seconds
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
    #ifdef INT_TIMING
    start_int();
    #endif
    //Sets the overflow every 1 second.
	
	
    if (++modem_timerCount >= MODEM_TIMEOUT) //MODEM_TIMEOUT
    {
        modem_timerCount = 0;
        modem_timeOut = true;
    }
    //restart UART0 TX if it has been stopped by CTS
    if(tx_counter0>0&&uart0_held&&UART0_CTS==0)
    {
        if (tx_counter0) {
            --tx_counter0;
       	    UDR0=tx_buffer0[tx_rd_index0];
           	uart0_held = 0;
            if (++tx_rd_index0 == TX_BUFFER_SIZE0)
                tx_rd_index0=0;
       	}
    }
    if(UART0_CTS)
    {
        _active_led_toggle();
      //  _status_led_toggle();
      //  _power_led_toggle();
    }
    else
    {
        _active_led_on();
    }

 //restart the external flow control
 #if HW_REV == V5 || HW_REV == V6
    if(UART1_CTS == 0)
        UCSR1B |= 0x20;
 #endif
    #ifdef INT_TIMING
    stop_int();
    #endif


}

void modem_read(void) {
	char input;

	unsigned char endLoop = false;
	unsigned char strPos = 0;

	//if(modem_not_responding)
    //{
    //    strcpyf(modem_rx_string, "ERROR");
    //    return;
    //}

	//Reset the counter
	global_interrupt_off();
    modem_timerCount = 0;
    modem_timeOut = false;
    global_interrupt_on();

	do
	{
		//sprintf(buffer,"%d,",modem_timerCount);print1(buffer);
        while (rx_counter0 <= 0 && !modem_timeOut)
        {
            tickleRover();
            continue;
        }
        if (modem_timeOut)
        {
            DEBUG_printStr("Modem has timed out...\r\n");
            if(!resetting_modem)
            {
                modem_restart();
                modem_init(build_type);
            }
            strcpyf(modem_rx_string, "ERROR");
            return;
        }
	    input = getchar();
	    //putchar1(input);
		tickleRover();
	    switch (input) {
	       case '\0':
		       	break;                           //nothing
		   case '\n':
		   	   	break;                           // we do not need this one
		   case '\r' :
		   	   	if (strPos > 0) {		//if we have received something
			   	   	endLoop = true;
			   	   	modem_rx_string[strPos] = '\0';
			   	}
			   	break;
		   default:
		   	   	modem_rx_string[strPos++] = input;			// build string
		   	   	if(strPos>=MODEM_SWBUFFER_LEN-1)
		   	   	{
                    modem_rx_string[strPos]='\0';
                    return;
		   	   	}
	    }
	    if(strstrf(modem_rx_string,"NO CARRIER"))
            break;
	}
    while (!endLoop);


    return;
}

    /* test fixture for the soft uart module */
#define UART0_RX      (PINE.0)
#define UART0_TX      (PORTE.1)
#define UART1_RX      (PIND.2)
#define UART1_TX      (PORTD.3)

void set_baud()
{
    #ifdef DEBUG
    sprintf(buffer,"Waiting for modem to be ready before we send AT+IPR=9600\r\n");print(buffer);
	#endif
   #if CLOCK == SIX_MEG
  //  disable_uart0();
  //  sprintf(buffer,"AT+IPR=9600\r\n");
  //  soft_eleven_k(buffer);
  //  delay_ms(200);
  //  reenable_uart0();
   #elif CLOCK == SEVEN_SOMETHING_MEG
    uart0_init(11520);
    printf("AT+IPR=9600\r\n");
    delay_ms(200);
    uart0_init(9600);
   #endif
}

void modem_restart()
{
   // int i = 10000;
    if(modem_not_responding)
    {
        DEBUG_printStr("Modem not responding\r\n");
        return;
    }
    else
        DEBUG_printStr("Modem not known to be locked\r\n");
    resetting_modem = true;

	//as it turns out the output of M_RST is negated before it hits the modem, so this is the right way up
	#if MODEM_TYPE == SIERRA_MC8775V && HW_REV == V5
    DEBUG_printStr("Resetting the modem\r\n");
	M_RST = 0;
	delay_ms(10);
	M_RST = 1;
	delay_ms(6000);
	#elif (MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE == SIERRA_MC8795V) && HW_REV == V6
		
	//hardware reset
	DEBUG_printStr("Resetting the modem\r\n");
	
	cbi(UCSR0B,0x80);
	cbi(UCSR1B,0x80);
	
	DDRD.6 = 1;
	PORTD.6 = 0;
	delay_ms(1000);
	PORTD.6 = 1;
	delay_ms(10);
	PORTD.6 = 0;
    	
	#if CONNECT_ONE_AVAILABLE
	DEBUG_printStr("Resetting the ConnectOne chip\r\n");
	DDRE.4 = 1;     //reset the ConnectOne on
	PORTE.4 = 0;
	DEBUG_printStr("1 second delay\r\n");
	delay_ms(1000);
	PORTE.4 = 1;
	#endif
	 
	DEBUG_printStr("Delay required for modem to start up before AT commands can be sent...\r\n");
	delay_ms(15000);	//8000 - increased from 8s to 9s due to loss of 1s delay above when Connect One not loaded

	reset_uarts();
	sbi(UCSR0B,0x80);
	sbi(UCSR1B,0x80);
	
	#else
	DEBUG_printStr("Resetting the modem\r\n");
    M_RST = 1;
   	delay_ms(1000);
    M_RST = 0;
    delay_ms(2000);
    //#else
    //#error "Modem type / hardware combination not right"
    #endif

    tickleRover();

    modem_state = NO_CONNECTION;

    #if MODEM_TYPE == Q2438F
	modem_wait_for(MSG_WIND | MSG_ERROR);
	if(strstrf(modem_rx_string,"ERROR"))
	    modem_not_responding = true;
	#endif

   /* ADMUX=ADC_VREF_TYPE;
ADCSRA=0x83;
    DEBUG_printStr("In through mode, CTRL-c to exit\r\n");
    while(1)
    {
        char c;
        tickleRover();
        if(rx_counter1)
        {
            c = getchar1();
            if(c==3)
                break;
            if(c==1)
            {
                //  port_init();
                  DDRD.6 = 1;
                  DEBUG_printStr("Resetting the modem\r\n");
                  PORTD.6 = 0;
                  delay_ms(1);
                  sprintf(buffer,"reset pin val=%d\r\n",read_adc(0));
                  print(buffer);
                  delay_ms(1000);
                  PORTD.6 = 1;
                  delay_ms(1);
                  sprintf(buffer,"reset pin val=%d\r\n",read_adc(0));
                  print(buffer);
                  delay_ms(1000);
                  PORTD.6 = 0;
                  delay_ms(1);
                  sprintf(buffer,"reset pin val=%d\r\n",read_adc(0));
                  print(buffer);
                  DEBUG_printStr("Resetting the ConnectOne chip\r\n");
                  DDRE.4 = 1;     //reset the ConnectOne on
                  PORTE.4 = 0;
                  delay_ms(1000);
                  PORTE.4 = 1;
            }
            putchar(c);
        }
        if(rx_counter0)
            putchar1(getchar());
    }

    */
	
	#ifdef DEBUG
    sprintf(buffer,"sending AT\r\n");
    print(buffer);
	#endif
	
	tickleRover();
    //printf("\r\nAT\r\n");
    printf("AT\r\n");		//SJL - debug
    tickleRover();
    //print("after reset wdt\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR);
    //print("after wait for modem msg\r\n");


	if(strstrf(modem_rx_string,"ERROR"))
	{
	    modem_not_responding = true;
	}		

	/* 	SJL - Commented out: iCPF AT command not supported - always returns error on
    	both MC8775V and MC8795V on 315 hardware - no ConnectOne chip!
		Re-inserted for v6 hardware for 320NG board with ConnectOne chip
    *********************************************************************************/
    //#if (MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE == SIERRA_MC8795V)  && HW_REV==6
	#if HW_REV==6 && CONNECT_ONE_AVAILABLE

	DEBUG_printStr("Setting iCPF to 1 and back to 0 to fix stupidness\r\n");
    printf("AT+iCPF=1\r\n");
    do
    {
        modem_read();
        #ifdef DEBUG
        print(modem_rx_string);
        #endif
    } while(!(strstrf(modem_rx_string,"ERROR")||
              strstrf(modem_rx_string,"I/ONLINE")||
              strstrf(modem_rx_string,"I/DONE")));
	
    printf("AT+iCPF=0\r\n");
    do
    {

        modem_read();
        #ifdef DEBUG
        print(modem_rx_string);
        #endif
    } while(!(strstrf(modem_rx_string,"ERROR")||
              strstrf(modem_rx_string,"I/ONLINE")||
              strstrf(modem_rx_string,"I/DONE")));
	
    #endif
    /*********************************************************************************/

    modem_state = NO_CONNECTION;
	resetting_modem = false;
}

int modem_init(unsigned char modem_type) {
    //note there is a global called timer.... unsigned int timer;
    #if MODEM_TYPE == Q24NG_PLUS
    Event e;
    #endif
    unsigned char i=0;
    char *str_rec[10];
	
	char print_flag=1;
	
    modem_not_responding = false;
	
	SREG &= ~(0x80);
	tickleRover();	//SJL added to protect sedateRover()
    sedateRover();
	SREG |= 0x80;
	
    while(rx_counter0) getchar();
	//Turn on the modem
	//RESET and boot must be low
	#if (HW_REV == V2)
	DDRE.3=1;
	DDRE.4=1;
	PORTE.3 = 0;
	PORTE.4 = 0;
	delay_ms(500);
	#endif

	#if HW_REV == V6 && CONNECT_ONE_AVAILABLE
	DDRE.4 = 1;
	PORTE.4 = 1;
	#endif

//	#ifdef _MODEM_DEBUG_
//    sprintf(buffer, ">MODEM: Modem Init()\r\n");
//    print(buffer);
//    #endif
	
           modem_clear_channel();
		   
		   delay_ms(2000);

	printf("AT+KSLEEP=2\r\n");
	modem_wait_for(MSG_OK | MSG_ERROR);

        #if MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE == SIERRA_MC8795V
        //need to add +CDS command here if I can find it. AT+CNMI doesn't like 3+ arguments
//            #ifdef _MODEM_DEBUG_
//            	sprintf(buffer, ">MODEM: Sent +CNMI=2,1\r\n");
//               	print(buffer);
//           	#endif
            printf("AT+CNMI=2,1\r\n");
            modem_wait_for(MSG_OK | MSG_ERROR);
//            #ifdef _MODEM_DEBUG_
//               	sprintf(buffer, ">MODEM: +CNMI response\r\n");
//               	print(buffer);
//        	#endif

            printf("AT+CMGF=1\r\n");
            modem_wait_for(MSG_OK | MSG_ERROR);
//            #ifdef _MODEM_DEBUG_
//               	sprintf(buffer, ">MODEM: +CMGF response\r\n");
//               	print(buffer);
//        	#endif
        #elif modem_is_wavecom_gsm()

        printf("AT+WOPEN=1\r\n");
        modem_wait_for(MSG_OK | MSG_ERROR);

       	#elif MODEM_TYPE == Q2438F //CDMA modem
TRY_CFUN_1_AGAIN:
			modem_clear_channel();
       	    printf("AT+CFUN=1\r\n");

       	    modem_register = true;
//       	    #ifdef _MODEM_DEBUG_
//       	    sprintf(buffer, ">MODEM: Waiting for +WIND:8\r\n");
//       	    print(buffer);
//       	    #endif
      	    //Wait for the modem to output +WIND, this tells us the modem
       	    //is ready to start the init sequence.
       	    modem_wait_for(MSG_WIND | MSG_ERROR);

//       	    #ifdef _MODEM_DEBUG_
//       	    sprintf(buffer,"+WIND Response - %s\r\n",modem_rx_string);print(buffer);
//       	    #endif
       	    if(strstrf(modem_rx_string,"CME ERROR: 3"))
       	    {
       	        printStr("Not quite ready yet\r\n");
       	        goto TRY_CFUN_1_AGAIN;
       	    }
       	    else if (strstrf(modem_rx_string, "ERROR"))
       	    {
       	        sprintf(buffer,"Modem hasn't woken up\r\n");print(buffer);
       	        return ERROR;
       	    }
       	    //wait for the WROM response
       	    do
       	    {
           	    modem_read();
//           	    #ifdef _MODEM_DEBUG_
//           	    sprintf(buffer,"+WROM Response - %s\r\n",modem_rx_string);print(buffer);
//           	    #endif
           	} while (!(strstrf(modem_rx_string,"WROM")||
           	           strstrf(modem_rx_string,"ERROR")));
       	    //make sure that this was a WROM command, else wait for it again.

       	    //check that the modem is alive.
       	    modem_register = false;
       	    printf("AT\r\n");
       	    modem_wait_for(MSG_OK | MSG_ERROR);
            if(strstrf(modem_rx_string,"OK"))
       	    {
//       	        #ifdef _MODEM_DEBUG_
//           	    sprintf(buffer, ">MODEM: awake and first AT responded too\r\n");
//           	    print(buffer);
//           	    #endif
       	    }
       	    else
       	    {
//       	        #ifdef _MODEM_DEBUG_
//           	    sprintf(buffer, ">MODEM: AT failed.... baud rate issue???\r\n");
//           	    print(buffer);
//           	    #endif
       	    }
        #endif

        //Wake up the modem
        printf("AT\r\n");
        //Wait to give the modem a chance to respond
        delay_ms(100);
       	modem_wait_for(MSG_OK | MSG_ERROR);

        //Check to see if the modem is already awake
        //If so, the echo will be off, turn on the echo
        if (strstrf(modem_rx_string, "OK"))
        {
            DEBUG_printStr("OK found, print ATE1\r\n");
            printf("ATE1\r\n");
            modem_wait_for(MSG_OK | MSG_ERROR);
            if (strncmpf (modem_rx_string, "OK",2)==0)
            {
                DEBUG_printStr("MODEM >Echo now on\r\n");
            }
            else if (strncmpf (modem_rx_string, "ATE1",4)==0)
            {
                modem_wait_for(MSG_OK | MSG_ERROR);
                if (strncmpf (modem_rx_string, "OK",2)==0)
                {
                    DEBUG_printStr("MODEM >Echo now on\r\n");
                }
                else
                {
                    DEBUG_printStr("Modem > Echo not turned back on\r\n");
                }
            }
        }

        printf("AT\r\n");
        modem_wait_for(MSG_OK | MSG_ERROR);

       	if (strstrf(modem_rx_string,"OK"))
       	{
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: awake and 2nd AT responded to\r\n");
//           print(buffer);
//           #endif
        }
        else
        {
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: 2nd AT failed.... baud rate issue???\r\n");
//           print(buffer);
//           #endif
           #if SYSTEM_LOGGING_ENABLED
           sprintf(buffer,"Modem init failed on second AT command");
           log_line("system.log",buffer);
           #endif
           return ERROR;
        }

        //Enable full functionality of the modem
        printf("AT+CFUN=1\r\n");
//        #ifdef _MODEM_DEBUG_
//        sprintf(buffer, ">MODEM: CFUN=1\r\n");
//        print(buffer);
//        #endif

        #if MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE == SIERRA_MC8795V
            modem_wait_for(MSG_OK | MSG_ERROR);
        #elif modem_is_wavecom_gsm()
            delay_ms(2000);
            modem_wait_for(MSG_OK | MSG_ERROR);
        #else
            modem_wait_for(MSG_WIND | MSG_ERROR);
        #endif

        printf("ATE1\r\n");
        modem_wait_for(MSG_OK | MSG_ERROR);

        //Enable full error reporting
        printf("AT+CMEE=1\r\n");
        modem_wait_for(MSG_OK | MSG_ERROR);

       	if (strstrf(modem_rx_string,"OK"))
       	{
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: CMEE=1 OK\r\n");
//           print(buffer);
//           #endif
        }
        else
        {
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: +CMEE Failed\r\n");
//           print(buffer);
//           #endif
           return ERROR;
        }
        delay_ms(1000);

        printf("AT\r\n");
        modem_wait_for(MSG_OK | MSG_ERROR);

       	if (strstrf(modem_rx_string,"OK"))
       	{
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: 'nother AT\r\n");
//           print(buffer);
//           #endif
        }
        else
        {
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: 'nother AT failed\r\n");
//           print(buffer);
//           #endif
           return ERROR;
        }
        delay_ms(2000);

        //Only check the SIM on GSM units
    	#if modem_is_wavecom_gsm() || MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE == SIERRA_MC8795V
    	    i=50;
    	    printf("AT+CPIN?\r\n");
//    	    #ifdef _MODEM_DEBUG_
//    	    DEBUG_printStr("AT+CPIN=");
//    	    #endif
    	    while(i--)
    	    {
      	        modem_wait_for(MSG_OK | MSG_ERROR | MSG_CPIN);
//      	        #ifdef _MODEM_DEBUG_
//      	        print(modem_rx_string);
//      	        putchar1('\r');putchar1('\n');
//      	        #endif
      	        if(strstrf(modem_rx_string,"ERROR: 515")!=0)
      	        {
      	            delay_ms(1000);
      	            printf("AT+CPIN?\r\n");
      	        }
      	        else if(strstrf(modem_rx_string,"+CPIN: SIM PIN")!=0)
      	        {
//          	        #ifdef _MODEM_DEBUG_
//        			sprintf(buffer, ">MODEM: Need SIM PIN\r\n");
//        			print(buffer);
//            		#endif
            		printf("AT+CPIN=");
        	    	print0_eeprom (config.pin_code);
        		    printf("\r\n");
                    modem_wait_for(MSG_OK | MSG_ERROR);
            		//There was no OK found, therefore the PIN Failed
            		if (strstrf (modem_rx_string, "OK") == 0)
        	    	{
        		        sprintf(buffer,"The PIN you entered was incorrect\r\n");print(buffer);
        		        return ERROR;
            		}
      	        } else if (strstrf(modem_rx_string, "+CPIN: READY") != '\0') {
//            		#ifdef _MODEM_DEBUG_
//        			sprintf(buffer, ">MODEM: SIM not needed\r\n");
//        			print(buffer);
//            		#endif
            		break;
            	} else if (strstrf(modem_rx_string, "+CME ERROR: 10") != '\0')  {
            		sprintf(buffer, "SIM Card not inserted\r\nTurn off the power, replace SIM card and restart the unit\r\n");print(buffer);
            		#if SYSTEM_LOGGING_ENABLED
            		sprintf(buffer,"No SIM inserted");log_line("system.log",buffer);
            		#endif
            		return ERROR;
            	}
    	    }
        delay_ms(1000);
        #endif

        //Check the RSSI to make sure the signal is strong enough
        i = modem_get_rssi();
        if (i < MODEM_RSSI_LIMIT)
	    {
		   //sprintf(buffer,"Signal Strength is too LOW (rssi=%d).\r\nThe Modem can't be used\r\n",modem_get_rssi()); //%s
		   sprintf(buffer,"Signal Strength is LOW (RSSI=%d).\r\n",modem_get_rssi());
		   print(buffer);
	    }
	    else if (i == 99)
	    {
//		   #ifdef _MODEM_DEBUG_
//           sprintf(buffer,"MODEM: RSSI Unknown (rssi=%d)\r\n",i);
//		   print(buffer);
//		   #endif
	    }
		else
		{
//            #ifdef _MODEM_DEBUG_
//            sprintf(buffer,"MODEM: RSSI OK (rssi=%d)\r\n",i);
//		    print(buffer);
//		    #endif
		}


        modem_register = true;
SEND_COPS_0:
#if 0
        printf("AT+COPS=0\r\n");
//        #ifdef _MODEM_DEBUG_
//        DEBUG_printStr("AT+COPS=0 : ");
//        #endif
        modem_wait_for(MSG_OK | MSG_ERROR);
//        #ifdef _MODEM_DEBUG_
//        sprintf(buffer,"%s\r\n",modem_rx_string);print(buffer);
//        #endif
        if (strstrf(modem_rx_string,"OK")!=0)
        {
//            #ifdef _MODEM_DEBUG_
//            DEBUG_printStr("Modem connected\r\n");
//            #endif
        }
        else if (strstrf(modem_rx_string,"ERROR: 30")!=0)
        {
            sprintf(buffer,"No service. Initialization aborted.\r\n");print(buffer);
            return ERROR;
        }
        else if (strstrf(modem_rx_string,"ERROR: 515")!=0)
        {
//            #ifdef _MODEM_DEBUG_
//            DEBUG_printStr("Modem is busy, trying again in 500 ms\r\n");
//            #endif
            delay_ms(500);
            goto SEND_COPS_0;
        }
        else if (strstrf(modem_rx_string,"ERROR")!=0)
        {
            sprintf(buffer,"Not able to connect to network. Trying again later.\r\n");print(buffer);
            return ERROR;
        }
#endif
        modem_requestUnsolicited();

        modem_register = false;
        delay_ms(500);


        //Setup SMS parameters for delivering and receiving SMS
	    //Set SMS to be sent in Text mode
	    #if MODEM_TYPE != Q2438F
    	    DEBUG_printStr("Printing AT+CMGF=1\r\n");
    	    printf("AT+CMGF=1\r\n");
    	    modem_wait_for(MSG_OK | MSG_ERROR);

            if(strstrf(modem_rx_string,"OK"))
           	{
//               #ifdef _MODEM_DEBUG_
//               sprintf(buffer, ">MODEM: CMGF=1\r\n");
//               print(buffer);
//               #endif
            }
            else
            {
//               #ifdef _MODEM_DEBUG_
//               sprintf(buffer, ">MODEM: +CMGF Failed\r\n");
//               print(buffer);
//               #endif
               return ERROR;
            }
            //delay_ms(1500);
        #endif

        //Turn the caller ID block off.
        #if MODEM_TYPE != SIERRA_MC8775V && MODEM_TYPE != SIERRA_MC8795V
        printf("AT+CLIR=0\r\n");
        modem_wait_for(MSG_OK | MSG_ERROR);
        if (strstrf(modem_rx_string,"OK"))
           {
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: AT+CLIR=0\r\n");
//           print(buffer);
//           #endif
        }
        else
        {
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: +CLIR Failed\r\n");
//           print(buffer);
//           #endif
           return ERROR;
        }
        #endif
        //delay_ms(1500);



        //Set the incoming call indication
        printf("AT+CRC=1\r\n");
        modem_wait_for(MSG_OK | MSG_ERROR);
        if (strstrf(modem_rx_string,"OK"))
        {
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: AT+CRC=1\r\n");
//           print(buffer);
//           #endif
        }
        else
        {
//           #ifdef _MODEM_DEBUG_
//           sprintf(buffer, ">MODEM: +CRC Failed\r\n");
//           print(buffer);
//           #endif
           return ERROR;
        }




		/* Repeat from line 643
        //	Check the RSSI to make sure the signal is strong enough
		i = modem_get_rssi();
		if (i < MODEM_RSSI_LIMIT)
		{
			sprintf(buffer,"Signal Strength is too LOW (rssi=%d).\r\nThe Modem can't be used\r\n",modem_get_rssi());
			print(buffer);
		}
		else if (i == 99)
		{
		   #ifdef _MODEM_DEBUG_
		  sprintf(buffer,"MODEM: RSSI Unknown (rssi=%d)\r\n",i);
		   print(buffer);
		   #endif
		}
		else
		{
            #ifdef _MODEM_DEBUG_
            sprintf(buffer,"MODEM: RSSI OK (rssi=%d)\r\n",i);
		    print(buffer);
		    #endif
		}
		*/

        //get the time from the network if cmda
        #if MODEM_TYPE == Q2438F
            printf("AT+CCLK?\r\n");
            do
            {
                modem_read();

            } while(!(strstrf(modem_rx_string,"+CCLK:")||
                      strstrf(modem_rx_string,"ERROR")));
            //check the correct cmd was echoed
            if (strstrf(modem_rx_string,"+CCLK:") != 0)
            {
                //Parse the string to read the time.
                i=0;
                str_rec[i]=strtok(modem_rx_string,":");
                i++;
                while ((str_rec[i] = strtok(NULL,"\"/,: ")) != NULL) {
            	    i++;
            	}
        	    rtc_set_date(atoi(str_rec[3]),atoi(str_rec[2]),atoi(str_rec[1]));
        	    rtc_set_time(atoi(str_rec[4]),atoi(str_rec[5]),atoi(str_rec[6]));

        	    //get the OK
                modem_wait_for(MSG_OK | MSG_ERROR);
             }
             else if (strstrf(modem_rx_string,"ERROR") != 0)
             {
                sprintf(buffer,"MODEM: Could not update the time\r\n");
            	print(buffer);
             }



            //set the modem to be expecting answer data calls
            //if you start using voice calls in the future change this, to either =0 or =3
            //this answers every call as a data call.
            printf("AT$QCVAD=4\r\n");
            modem_wait_for(MSG_OK | MSG_ERROR);
            if (strstrf(modem_rx_string,"OK"))
            {
//               #ifdef _MODEM_DEBUG_
//               sprintf(buffer, ">MODEM: AT$QCVAD=4\r\n");
//               print(buffer);
//               #endif
            }
            else
            {
//               #ifdef _MODEM_DEBUG_
//               sprintf(buffer, ">MODEM: AT$QCVAD=4 Failed\r\n");
//               print(buffer);
//               #endif
               return ERROR;
            }
        #endif

        //Last thing, turn off the modem echo
        //printf("ATE0\r\n");
        //modem_wait_for(MSG_OK | MSG_ERROR);
        /*if (strstrf(modem_rx_string,"OK"))
        {
           #ifdef _MODEM_DEBUG_
           sprintf(buffer, ">MODEM: ATE0\r\n");
           print(buffer);
           #endif
        }
        else
        {
           #ifdef _MODEM_DEBUG_
           sprintf(buffer, ">MODEM: ATE0 Failed\r\n");
           print(buffer);
           #endif
           return ERROR;
        } */

    wakeRover();
    
	//modem_state = CELL_CONNECTION;
	//SJL - 290611 - replaced modem_state = CELL_CONNECTION; with modem_check_status()
	//as cell connection is not necesserily established
	
	modem_check_status(&print_flag);
	
    #if TCPIP_AVAILABLE && (modem_is_wavecom_gsm() || MODEM_TYPE == Q2438F)

    printf("AT+WOPEN=1\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR);

    #if MODEM_TYPE == Q24NG_PLUS
    printf("AT+WIPCFG=1\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR);
    DEBUG_printStr("Pushing sms_GPRS_CHECK (1)\r\n");
    e.type = sms_GPRS_CHECK;
    queue_push(&q_modem,&e);
    #endif

    if (strcmpf(modem_rx_string, "OK") != 0)
    {
        DEBUG_printStr("GPRS Connection start Failed\r\n");
        return false;
    }
    #endif


    return true;

}

/******************************************************************************
 * bool modem_throughMode(void)
 *
 * This function controls the modem when it is put into through mode.  Through
 * mode means that the modem has made an asynchronous connection with an
 * external analog modem.  The modem then creates an wireless serial link, and
 * all of data received is echoed out the serial port.
 *
 *****************************************************************************/

bool modem_throughMode(void)
{
    char ch;
    Event e;
    static unsigned char uart_plus_count0 = 0;
    static unsigned char uart_plus_count1 = 0;

    //If there is new data from the modem get it
    if (rx_counter0 > 0)
    {
        //Read the character
        ch = getchar();
        //Put the char down the external serial port
        putchar1(ch);
        //check to see that we haven't received a NO CARRIER command
        if (ch == NO_CARRIER[no_carrier_count])
        {
            if (++no_carrier_count >= 10)
            {
                //We have received a NO CARRIER command, therefore drop out of
                //through mode

                no_carrier_count=0;
                modem_disconnect_csd();
                delay_ms(3000);
                return false;
            }

        }
        else
        {
            //A normal command was received
            no_carrier_count=0;
        }
        if (ch == '#')
        {
            if (++uart_plus_count0 >= 3)
            {
                //stop through mode
                modem_TMEcho = false;
                sprintf(buffer, "320 Command Mode>\r\n");print(buffer);
                return true;
            }
        }
        else
        {
            uart_plus_count0=0;
        }

    }
    //Check if there is new data on the external serial port
    if (rx_counter1 > 0)
    {
        //Receive the character
        ch = getchar1();
        //Send it down the modem
        putchar(ch);
        //Check to see if the user wants to drop the serial link
        //this is done by sending ### to the SMS300
        if (ch == '#')
        {
            if (++uart_plus_count1 >= 3)
            {
                //Hang up the modem
                modem_TMEcho = false;
                sprintf(buffer, "320 Command Mode>\r\n");
                print(buffer);
                return true;
            }
        }
        else
        {
            uart_plus_count1=0;
        }
    }
    return true;
}

void reset_uarts()
{
    tx_wr_index0 = tx_rd_index0 = tx_counter0 = 0;
    tx_wr_index1 = tx_rd_index1 = tx_counter1 = 0;
    rx_wr_index0 = rx_rd_index0 = rx_counter0 = 0;
    rx_wr_index1 = rx_rd_index1 = rx_counter1 = 0;
}

void modem_disconnect_csd()
{
    Event e;
    modem_TMEcho = false;
    modem_TMEnabled = false;
    long_time_up = false;
    #if SYSTEM_LOGGING_ENABLED
    sprintf(buffer,"CSD call dropped");
    log_line("system.log",buffer);
    #endif
    #if MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE == SIERRA_MC8795V
    modem_restart();
    modem_init(build_type);
    /*putchar('A');
    delay_ms(200);
    putchar('T');
    delay_ms(200);
    putchar('H');
    delay_ms(200);
    putchar('\r');*/
    #else
    printf("ATH\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR);
    #endif
    reset_uarts();
    e.type = sms_CHECK_NETWORK_INTERNAL;
    queue_push(&q_modem, &e);
    queue_resume(&q_modem);
    DEBUG_printStr("MODEM: NO CARRIER found, through mode off\r\n");
}

bool modem_readPort(void)
{
    char ch;
    //Is there data available?
    //sprintf(buffer,"modem_readPort - rx_counter0 = %d\r\n",rx_counter0);print(buffer);
    if (rx_counter0 <= 0)
        return false;
    //else{
    //sprintf(buffer,"modem_readPort - rx_counter0 = %d\r\n",rx_counter0);print(buffer);}
    //Read in the new char
    ch = getchar();
    //Delay
    delay_us(1);	//delay_us(1);

    //sprintf(buffer,"Reading modem port... character received: %c\r\n",ch);print(buffer); 	//SJL - debug

    //See what the char is and handle accordingly
    switch (ch)
    {
        case '\0' :
            //Do nothing, Don't want null characters
            break;
        case '\n' :
            //Do nothing, Don't want null characters
            break;
        case '\r' :
            //This is the EOL char. Set the uart_line_received flag, and add a
            //NULL char to end of string
            modem_rx_string[m_rx_write] = '\0';
            if(m_rx_write > 0)
            {
                m_rx_write=0;
                return true;
            }
            break;
        default :
            //Otherwise just append the character to the string
            modem_rx_string[m_rx_write++] = ch;
            //catch a software buffer overflow
            if (m_rx_write > MODEM_SWBUFFER_LEN)
            {
                DEBUG_printStr("MODEM SW buffer overflowed...\r\n");
                m_rx_write--;
                //exit if there is an overflow
                return true;
            }
            break;
    }

    return false;

}

//extern char month_length[12];
//flash char month_length[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
void time_adjust_minute(Event *e, signed int minute_shift)	//what was the time minute_shift minutes ago
{
    e->minute += minute_shift;

    if((signed char)e->minute >= 0)
        return;

    e->minute += 60;
    if(e->hour>0)
    {
        e->hour--;
        return;
    }

    e->hour = 23;
    if(e->day > 1)
    {
        e->day--;
        return;
    }

    if(e->month > 1)
    {
        e->month--;
    }

    e->month = 12;
    e->year--;
    e->day = month_length[e->month-1];

    if(e->month==2 && (e->year % 4)==0)
        e->day++;
}

bool modem_handleNew(void)
{
    unsigned char count, pos, index;
    char temp[16];
    unsigned long cdsResponse=0;
    Event e;

    //A command has been receieved from the modem, enumerate
    //this into a known event
    #ifdef DEBUG
    DEBUG_printStr("Unsolicited modem string: ");
    print(modem_rx_string);
    DEBUG_printStr("\r\n");
    #endif
		
    //See if the GPRS has failed
    if (strncmpf(modem_rx_string, "#CME ERROR: 35",14) == 0)
    {
        DEBUG_printStr("GPRS Physical Layer ERROR\r\n");
        if (strcmpf(modem_rx_string, "#CME ERROR: 35840") == 0)
        {
            DEBUG_printStr("Modem trying to be acessed twice...\r\n");
        }
        else
        {
            DEBUG_printStr("Pushing sms_GPRS_CHECK (2)\r\n");
            e.type = sms_GPRS_CHECK;
            queue_push (&q_modem, &e);
        }
    }
    else if(strstrf(modem_rx_string,"+WIND: 6,1"))
    {
        printf("ATH\r\n");
        e.type = sms_CHECK_NETWORK_INTERNAL;
        queue_push(&q_modem, &e);
    }

    else if (strncmpf(modem_rx_string,"+CREG",5) == 0)
    {
        if (strstrf(modem_rx_string, ",1")|| strstrf(modem_rx_string, ",5") ||
            strcmpf(modem_rx_string,"+CREG: 1")==0 || strcmpf(modem_rx_string,"+CREG: 5")==0)
        {
            //The modem is now registered on the network
            if (config_cmgs)
            {
                if(strstrf(modem_rx_string, "1") != 0)
                {
					//DEBUG_printStr("Cellular Connection established\r\n");
					if(modem_state != CELL_CONNECTION)
					{
						sprintf(buffer,"Network connection established - ");print1(buffer);
						sprintf(buffer,"RSSI = %d\r\n",modem_get_rssi());print1(buffer);
					}
				}
                else
				{
					sprintf(buffer,"Network connection established (roaming) - ");print1(buffer);
					sprintf(buffer,"RSSI = %d\r\n",modem_get_rssi());print1(buffer);
                    //DEBUG_printStr("Cellular Connection established and roaming\r\n");
				}
			}			
			startupError = false;					//SJL 290611 - added
			modem_state = CELL_CONNECTION;			//SJL 290611 - added
            queue_resume(&q_modem);
            //e.type = sms_CHECK_NETWORK_INTERNAL;	//SJL 290611 - commented out - not required if connected
            //queue_push(&q_modem, &e);				//SJL 290611 - commented out - not required if connected
        }
        if (strstrf(modem_rx_string, ",2") != 0 || strcmpf(modem_rx_string,"+CREG: 2")==0 ||
            strstrf(modem_rx_string, ",0") != 0 || strcmpf(modem_rx_string,"+CREG: 0")==0 )
        {
			//The modem is no longer registered on the network
            if (config_cmgs)
            {
				if(strstrf(modem_rx_string, "2") != 0)
				{
					sprintf(buffer,"Network Connection currently unavailable\r\n");print(buffer);
					//sprintf(buffer,"RSSI = %d\r\n",modem_get_rssi());print1(buffer);
					sprintf(buffer,"Attempting to connect...\r\n",modem_get_rssi());print1(buffer);
				}
				else
				{
					sprintf(buffer,"Network Connection currently unavailable\r\n");print(buffer);
					//sprintf(buffer,"RSSI = %d\r\n",modem_get_rssi());print1(buffer);
					
				}				
            }
            //#if TCPIP_AVAILABLE
            //gprs_stopConnection();
            //#endif
            modem_state = NO_CONNECTION;
            startupError=true;
            if (error.networkFailureCounter != 0xFFFF)
                error.networkFailureCounter++;
        }
    }
    //data call incoming
    else if (strstrf(modem_rx_string,"RING"))
    {
        #ifdef DEBUG
        sprintf(buffer,"[%s]\r\n",modem_rx_string);
        print(buffer);
        #endif
        e.type = event_THRU_MODE;
        queue_push(&q_event, &e);
    }
    else if (strstrf(modem_rx_string,"NO CARRIER"))
    {
        DEBUG_printStr("NO CARRIER found\r\n");
        if (modem_TMEnabled)
        {
            //Hang up the modem
            modem_disconnect_csd();
            queue_resume(&q_modem);
        }
        e.type = sms_CHECK_NETWORK_INTERNAL;
       // queue_push(&q_modem, &e);
    }
    else if (strstrf(modem_rx_string,"+CMTI"))
    {
        //handle new SMS
        //sprintf(buffer,"modem.c - +CMTI received, adding SMS rx event\r\n");print(buffer);
        e.type = event_MODEM_REC;
        queue_push(&q_event, &e);
    }
    else if (strstrf(modem_rx_string,"+CMGL:"))
    {
        #ifdef MESSAGE_EXPIRY
        //need to get the data after the fourth ','
        strtok(modem_rx_string,",");
        strtok(NULL,",");
        strtok(NULL,",");
        strcpy(buffer,strtok(NULL,","));
        strcat(buffer,strtok(NULL,","));
        strrep(buffer,"/","");
        strrep(buffer,"\"","");
        strrep(buffer," ","");
        strrep(buffer,":","");
        strcpy(temp,buffer);
        event_populate_time(&e);
        #ifdef DEBUG
        //print(buffer);
        //putchar1('\r');putchar1('\n');
        #endif

        //figure what the time was 30 minutes ago
        time_adjust_minute(&e,-30);

        sprintf(buffer,"%02d%02d%02d%02d%02d%02d",e.year,e.month,e.day,e.hour,e.minute,e.second);
        #ifdef DEBUG
       // print(buffer);
       // putchar1('\r');putchar1('\n');
        #endif

        if(strcmp(temp,buffer)<0)
        {
            #ifdef DEBUG
            print(buffer);
            putchar1('#');
            print(temp);
            sprintf(buffer,"Message too late arriving (%d), ignoring!\r\n",index);
            print(buffer);
            #endif
          //  CMGL_pause = true;
          //  queue_pause(&q_modem);
          //  e.type = sms_DELETE_SENT;   //deletes all read messages
          //  e.param = atoi(modem_rx_string+7);
          //  queue_push (&q_modem, &e);
        }
        else
        {

        #endif
            //handle SMS found on SIM
            DEBUG_printStr("Found an old message\r\n");
            #ifdef DEBUG
            print(modem_rx_string);
            #endif
            e.type = event_MODEM_REC_OLD;
            queue_push(&q_event,&e);
            #ifdef MESSAGE_EXPIRY
        }
        #endif
    }
    else if (strstrf(modem_rx_string,"OK"))
    {
      //  if(CMGL_pause)
      //      queue_resume(&q_modem);
        #ifdef _MODEM_DEBUG_
        DEBUG_printStr("[");
        print(modem_rx_string);
        DEBUG_printStr("]\r\n");
        #endif
    }
    else
    {
        #ifdef _MODEM_DEBUG_
        DEBUG_printStr("[");
        print(modem_rx_string);
        DEBUG_printStr("]\r\n");
        #endif
    }
    return false;
}

bool modem_clear_channel()
{

    #ifdef _MODEM_DEBUG_
    printStr("<<Clearing modem channel>>\r\n");
    #endif
    while(rx_counter0>0) getchar();
    
	DEBUG_printStr("Channel cleared - sending AT\r\n");
	printf("AT\r\n");
    delay_ms(50);
    modem_wait_for(MSG_OK | MSG_ERROR);
    //printf("ATNONSENSE\r\n");
    //modem_wait_for(MSG_ERROR);
    
	#if HW_REV == V6 && CONNECT_ONE_AVAILABLE
    printf("AT+I\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR);
    #endif
    
	return true;
}

void modem_check_status(char *print_flag)
{
    DEBUG_printStr("Checking registration state:\r\n");
    modem_clear_channel();
	sprintf(buffer,"AT+CREG?\r\n");
	print0(buffer);
    modem_wait_for(MSG_CREG | MSG_ERROR);
	
	//print1(modem_rx_string);
	//sprintf(buffer,"\r\n");print1(buffer);
	
	//sprintf(buffer,"print_flag = %d\r\n",*print_flag);print1(buffer);
	
	/*
	if (strstrf(modem_rx_string, ",1") || strstrf(modem_rx_string, ",5") )	//||
            //strcmpf(modem_rx_string,"+CREG: 1")==0 || strcmpf(modem_rx_string,"+CREG: 5")==0)  
	{
		sprintf(buffer,"Network connection established, ");print1(buffer);
		sprintf(buffer,"RSSI = %d\r\n",modem_get_rssi());print1(buffer);
        startupError = false;
		modem_state = CELL_CONNECTION;
	}
    else 
	{
		//sprintf(buffer,"Network connection unavailable, ");print1(buffer);
		sprintf(buffer,"NETWORK CONNECTION UNAVAILABLE, ");print1(buffer);
		sprintf(buffer,"RSSI = %d\r\n",modem_get_rssi());print1(buffer);
		startupError = true;
		modem_state = NO_CONNECTION;
	}
	*/
	
	if (strstrf(modem_rx_string, ",1"))
	{
		if(*print_flag)
		{
			sprintf(buffer,"Network connection established (home network)\r\n");print1(buffer);
		}
        startupError = false;
		modem_state = CELL_CONNECTION;
	}
	else if (strstrf(modem_rx_string, ",5"))
	{
		if(*print_flag)
		{
			sprintf(buffer,"Network connection established (roaming)\r\n");print1(buffer);
		}
        startupError = false;
		modem_state = CELL_CONNECTION;
	}
	else if (strstrf(modem_rx_string, ",2"))
	{
		if(*print_flag)
		{
			sprintf(buffer,"NETWORK CONNECTION UNAVAILABLE (searching)\r\n");print1(buffer);		
		}
		startupError = true;
		modem_state = NO_CONNECTION;
	}
	else if (strstrf(modem_rx_string, ",3"))
	{
		if(*print_flag)
		{
			sprintf(buffer,"NETWORK CONNECTION UNAVAILABLE (registration denied)\r\n");print1(buffer);		
		}
		startupError = true;
		modem_state = NO_CONNECTION;
	}
	else if (strstrf(modem_rx_string, ",4"))
	{
		if(*print_flag)
		{
			sprintf(buffer,"NETWORK CONNECTION UNAVAILABLE (unknown)\r\n");print1(buffer);		
		}
		startupError = true;
		modem_state = NO_CONNECTION;
	}
	else if (strstrf(modem_rx_string, ",0"))
	{
		if(*print_flag)
		{
			sprintf(buffer,"NETWORK CONNECTION UNAVAILABLE (not searching)\r\n");print1(buffer);		
		}
		startupError = true;
		modem_state = NO_CONNECTION;
	}
	else
	{
		if(*print_flag)
		{
			sprintf(buffer,"NETWORK CONNECTION UNAVAILABLE\r\n");print1(buffer);
		}
		startupError = true;
		modem_state = NO_CONNECTION;
	}
	//sprintf(buffer,"RSSI = %d\r\n",modem_get_rssi());print1(buffer);
	
}

void modem_requestUnsolicited()
{

SEND_CREG_1:
    DEBUG_printStr("Setting +CREG=1\r\n");
    printf("AT+CREG=1\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR | MSG_WIND);
    if(strstrf(modem_rx_string,"+WIND: 4")) goto SEND_CREG_1;
#if modem_is_wavecom_gsm()
SEND_CGREG_1:
    DEBUG_printStr("Setting +CGREG=1\r\n");
    printf("AT+CGREG=1\r\n");
    modem_wait_for(MSG_OK | MSG_ERROR | MSG_WIND);
    if(strstrf(modem_rx_string,"+WIND: 4")) goto SEND_CGREG_1;
#endif
}

#if SMS_AVAILABLE
char modem_send_sms(char *text, char *number)
{
    char c;
	
   #ifdef _SMS_DEBUG_
   sprintf(buffer,"Sending SMS...\r\nText:");
   print1(buffer);
   print(text);
   sprintf(buffer,"\r\nNumber:");
   print1(buffer);
   print(number);
   sprintf(buffer,"\r\n");
   print1(buffer);
   #endif
   strrep(text,"\r","\n");
   DEBUG_printStr("Carriage returns replaced with line feeds\r\n");
   if(!strlen(text)||!strlen(number))
        return false;

   printf("ATE1\r\n");
   modem_wait_for(MSG_OK | MSG_ERROR);

RESEND:
   DEBUG_printStr("Sending message now...\r\n");
   delay_ms(100);
   printf("AT+CMGS=\"%s\"\r\n",number);
   modem_timerCount = 0;
   modem_timeOut = false;
   while(!modem_timeOut)
   {
        tickleRover();
        if(rx_counter0)
        {
            if((c=getchar())=='>')
                break;
#ifdef _SMS_DEBUG_
            else
                putchar1(c);
#endif
        }
   }
   if(!modem_timeOut)
   {
       printf("%s%c",text,26);
       do
       {
           modem_wait_for(MSG_OK | MSG_ERROR);
       } while(strstrf(modem_rx_string,"+CME ERROR: 515"));
       if(strstrf(modem_rx_string,"+CMS ERROR: 536"))
       {
        DEBUG_printStr("That's really, really odd\r\n");
        delay_ms(1000);
        goto RESEND;
       }
   }
   else
       sprintf(modem_rx_string,"ERROR: Timeout");

   if(strstrf(modem_rx_string,"ERROR"))
   {
   #ifndef _SMS_DEBUG_
        return false;
   #else
       sprintf(buffer,"Modem rejected SMS message \"");print(buffer);
       print(text);
       sprintf(buffer,"\" to %s\r\n",number);
       print(buffer);
       return false;
   } else {
       sprintf(buffer,"Modem accepted SMS message \"");print(buffer);
       print(text);
       sprintf(buffer,"\" to %s\r\n",number);
       print(buffer);
   #endif
   }

   #if modem_is_wavecom_gsm() || MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE == SIERRA_MC8795V
   if(strstrf(modem_rx_string,"OK"))
       return true;
   else return false;
   #else
   modem_wait_for(MSG_CDS | MSG_ERROR);
   return true;
   #endif
}
#endif

#if SMS_AVAILABLE
char modem_read_sms(char index)
{
    char *temp = " ";
    #ifdef DEBUG
    sprintf(buffer,"AT+CMGR=%d\r\n",index);
    print(buffer);
    #endif
    printf("AT+CMGR=%d\r\n",(int)index);
    modem_wait_for(MSG_ERROR | MSG_CMGR);
    sms_newMsg.index = index;
    if(strstrf(modem_rx_string,"ERROR"))
        return false;
    else
    {
        #ifdef _MODEM_DEBUG_
        sprintf(buffer,"Message string: ");
        print(buffer);
        print(modem_rx_string);
        #endif
        temp = strchr(modem_rx_string,'"')+1; //front of "REC UNREAD"
        temp = strchr(temp,'"')+1; //back of the "REC UNREAD"
        temp = strchr(temp,'"')+1; //front of the phone number
        *strchr(temp,'"') = '\0';  //end of the phone number
        strcpy(sms_newMsg.phoneNumber,temp);
        #ifdef _MODEM_DEBUG_
        sprintf(buffer,"Message coming from %s\r\n",temp);
        print(buffer);
        #endif
        modem_read();              //content line
        strcpy(sms_newMsg.txt,modem_rx_string);
        #ifdef _MODEM_DEBUG_
        sprintf(buffer,"Message content is");
        print(buffer);
        print(sms_newMsg.txt);
        sprintf(buffer,"\r\n");
        print(buffer);
        #endif
        modem_wait_for(MSG_OK | MSG_ERROR); //clear the following OK
    }
    printf("AT+CMGD=%d\r\n",(int)index);  //deleting message
    modem_wait_for(MSG_ERROR | MSG_OK);
    return true;
}
#endif

unsigned int modem_wait_for(unsigned int index)
{
    char i;
    char* x;
    char y;

    if(!index) return false;

    while(1)
    {
        if(!packeted_data)
        {
            modem_read();
        }
        //else		//SJL - commented out
        //{
        //    email_get_line(buffer);
        //    strcpy(modem_rx_string,buffer);
        //}
        #ifdef _MODEM_DEBUG_
        putchar1('-');
        putchar1('[');
        print(modem_rx_string);
        sprintf(buffer,"]\r\n");print(buffer);
        #endif

		//below delay required for receiving sms for MC8795V modem only
		//minimum delay that works correctly (sys sms) is 4ms - using 8ms for safety margin
        delay_ms(8);	//SJL - CAVR2 - delay required to substitute for time taken by _MODEM_DEBUG_ above

        for(i=0;i<MSG_COUNT;i++)
        {
            tickleRover();
            if(index&((unsigned int)0x0001)<<i)
            {
            	/* 	SJL - CAVR2
                	strstr changed to strstrf because constants have been moved to
                    flash memory (Store Global Constants in FLASH Memory)			*/
                if(strstrf(modem_rx_string,RETURNED_MESSAGES[i]))
                //if(strstr(modem_rx_string,buffer))
                {
                    //print("\r\n** OK RECEIVED - RETURNING **\r\n");
                    return ((unsigned int)0x0001)<<i;
                }
            }
        }
    }
}


char modem_get_rssi()
{
    char rssi;
    if(q_modem.paused||modem_TMEnabled)
        return modem_rssi;
    printf("AT+CSQ\r\n");
    modem_wait_for(MSG_ERROR | MSG_CSQ);
    if(strstrf(modem_rx_string,"ERROR"))
        return 99;
    if(!strchr(modem_rx_string,':'))
        rssi = 99;
    else rssi = atoi(strchr(modem_rx_string,':')+1);
    modem_wait_for(MSG_OK | MSG_ERROR);
    modem_rssi = rssi;
    return rssi;
}

char modem_get_ber()
{
    char ber;
    if(q_modem.paused||modem_TMEnabled)
        return modem_ber;
    printf("AT+CSQ\r\n");
    modem_wait_for(MSG_ERROR | MSG_CSQ);
    if(strstrf(modem_rx_string,"ERROR"))
        return 99;
    if(!strchr(modem_rx_string,','))
        ber = 99;
    else ber = atoi(strchr(modem_rx_string,',')+1);
    modem_wait_for(MSG_OK | MSG_ERROR);
    modem_ber = ber;
    return ber;
}

#if EMAIL_AVAILABLE
void udp_putchar(char c)
{
    #ifdef DEBUG
    sprintf(buffer,"%02X ",c);
    print(buffer);
    #endif
    putchar(c);
}
#endif

void enter_command_mode()
{
    delay_ms(1200);
    sequential_plus = 0;
    putchar('+');
    putchar('+');
    sequential_plus = 0;
    putchar('+');
    delay_ms(1200);
}

flash char base64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64encode_chunk(char *c, eeprom char *src)
{
    char temp[3];
    char i;
    for(i=0;i<3;i++)
        temp[i] = '\0';
    for(i=0;i<3;i++)
    {
        temp[i] = src[i];
        if(temp[i]=='\0')
            break;
    }

    c[0] = base64[(temp[0] & 0xFC)>>2];	//if temp[0]==252 c[0]=base64[63] = '/'
    c[1] = base64[((temp[0] & 0x03) << 4) | ((temp[1] & 0xF0) >> 4)];
    c[2] = base64[((temp[1] & 0x0F) << 2) | ((temp[2] & 0xC0) >> 6)];
    c[3] = base64[temp[2] & 0x3F];
}

char *base64encode(char *c, eeprom char *source)
{
    char i;
    #pragma warn-
    for(i=0; (i*3) < strlene(source); i++)
        base64encode_chunk(c+(i*4),source+(i*3));
    c[i*4] = '\0';
    switch(strlene(source)%3)
    {
        case 1:
            if(strchr(c,'A'))
                *strrchr(c,'A') = '=';
        case 2:
            if(strchr(c,'A'))
                *strrchr(c,'A') = '=';
        case 0:
        default:
            break;
    }
    //print(c);
    return c;
    #pragma warn+
}
