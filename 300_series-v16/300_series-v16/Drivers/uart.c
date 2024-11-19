/*
    Functions relating to reading and writing on UART1
*/
#include <stdlib.h>
#include <stdio.h>              //SJL - CAVR2 - added
#include <string.h>
#include <ctype.h>
#include <delay.h>              //SJL - CAVR2 - added
#include <mega128.h>            //SJL - CAVR2 - added

#include "global.h"             //SJL - CAVR2 - added
#include "buildnumber.h"
#include "options.h"			//SJL - CAVR2 - added for AT+TEST

#include "flash\sd_cmd.h"		//SJL - CAVR2 - added for AT+TEST
#include "flash\file_sys.h"		//SJL - CAVR2 - added for AT+TEST

#include "drivers\uart.h"
#include "drivers\sms.h"
#include "drivers\config.h"
#include "drivers\input.h"
#include "drivers\event.h"
#include "drivers\gps.h"
#include "drivers\debug.h"      //SJL - CAVR2 - added
#include "drivers\queue.h"      //SJL - CAVR2 - added
#include "drivers\mmc.h"        //SJL - CAVR2 - added
#include "drivers\modem.h"      //SJL - CAVR2 - added
#include "drivers\error.h"      //SJL - CAVR2 - added
#include "drivers\contact.h"    //SJL - CAVR2 - added
#include "drivers\ds1337.h"     //SJL - CAVR2 - added for AT+TEST
#include "drivers\str.h"     	//SJL - CAVR2 - added for AT+TEST strcpye

#define TOKEN ","
#define flush_msg_buffer() received_msg.phone_number[0] = '\0';received_msg.text[0] = '\0'

/**********************************************************************************************************/
/* UART Global Variable Initialization */
// SJL - CAVR2

volatile unsigned char modem_read_ready;
volatile unsigned char uart_read_ready;
static volatile bool uart_send_paused=false;
unsigned char rx_write=0;
char uart_rx_string[UART_SWBUFFER_LEN];
char *command_string;
eeprom uart_t uart1_setup={9600,8,'n',1}; //Default setup for the external comm port //from global.c

char tx_buffer0[TX_BUFFER_SIZE0];       // USART0 Transmitter buffer
#if TX_BUFFER_SIZE0<256
unsigned char tx_wr_index0=0,tx_rd_index0=0,tx_counter0=0;
#else
unsigned int tx_wr_index0=0,tx_rd_index0=0,tx_counter0=0;
#endif

char tx_buffer1[TX_BUFFER_SIZE1];
#if TX_BUFFER_SIZE1<256
volatile unsigned char tx_wr_index1=0,tx_rd_index1=0,tx_counter1=0;
#else
volatile unsigned int tx_wr_index1=0,tx_rd_index1=0,tx_counter1=0;
#endif

bit uart0_held=0;
volatile unsigned int rx_wr_index0=0,rx_rd_index0=0,rx_counter0=0;
volatile unsigned int rx_wr_index1=0,rx_rd_index1=0,rx_counter1=0;
eeprom bool uart_echo = true;
bit uart_caps = true;
bit uart0_flowPaused = false;
bit uart1_flowPaused = false;
char modem_state = NO_CONNECTION;
char modem_rssi;
char modem_ber;
bit uart_paused;
char rx_buffer0[RX_BUFFER_SIZE0];
char rx_buffer1[RX_BUFFER_SIZE1];
char sequential_plus = 0;
/**********************************************************************************************************/

bool uart0_init(unsigned long baud) {

	// USART0 initialization
    // Communication Parameters: 8 Data, 1 Stop, No Parity
    // USART0 Receiver: On
    // USART0 Transmitter: On
    // USART0 Mode: Asynchronous
    // USART0 Baud rate: 9600 (Double Speed Mode)
#if CLOCK == SIX_MEG
    UCSR0A=0x02;
#elif CLOCK == SEVEN_SOMETHING_MEG
    UCSR0A=0x00;
#endif
    UCSR0B=0xD8;
    UCSR0C=0x06;
#if CLOCK == SIX_MEG
    #ifdef DEBUG
    sprintf(buffer,"Setting UART0 up at %d\r\n",baud);
    print(buffer);
    #endif
    switch(baud)
    {
        case 9600:
            UBRR0H=0x00;
            UBRR0L=0x4D;
            break;
        case 57600:
            UBRR0H=0x00;
            UBRR0L=0x0C;
            break;
        default:
            UBRR0H=0x00;
            UBRR0L=0x4D;
            break;
    }
#elif CLOCK == SEVEN_SOMETHING_MEG
    switch(baud)
    {
    case 1200:
        UBRR1H=0x01;
        UBRR1L=0x7F;
		break;
	case 2400:
        UBRR1H=0x00;
        UBRR1L=0xBF;
		break;
	case 4800:
    	UBRR1H=0x00;
        UBRR1L=0x5F;
		break;
	case 9600:
        UBRR1H=0x00;
        UBRR1L=0x2F;
		break;
	case 14400:
        UBRR1H=0x00;
        UBRR1L=0x1F;
		break;
    case 19200:
        UBRR1H=0x00;
        UBRR1L=0x17;
		break;
    case 38400:
        UBRR1H=0x00;
        UBRR1L=0x0B;
		break;
    case 57600:
        UBRR0H=0x00;
        UBRR0L=0x07;
		break;
    case 11520:
        UBRR0H=0x00;
        UBRR0L=0x03;
		break;
	default:
	    return 0;
    }
#endif

    //Setup for flow control.
    UART0_RTS = 0;
    UART0_RTS_DDR = 1;       //Ouput
    UART0_RTS = 0;
    UART0_CTS = 0;
    UART0_CTS_DDR = 0;       //Input
    //ref baud to stop compiler warnings....
    baud=1;
    return true;
}

bool uart1_init(eeprom uart_t *setup) {

    // USART1 initialization
    // Communication Parameters: 8 Data, 1 Stop, No Parity
    // USART1 Receiver: On
    // USART1 Transmitter: On
    // USART1 Mode: Asynchronous
    // USART1 Baud rate: selectable (Double Speed Mode)

#if CLOCK == SIX_MEG
    UCSR1A=0x02;
#elif CLOCK == SEVEN_SOMETHING_MEG
    UCSR1A=0x00;
#endif
    UCSR1B=0xB8;
    UCSR1C=0x06;

	//Setup for flow control
#if HW_REV == V5 || HW_REV == V6
    UART1_RTS_DDR = 1;       //Ouput
    UART1_RTS = 0;
    UART1_CTS_DDR = 0;       //Input
    UART1_CTS = 0;
#endif
	
    //Set the baud rate to the values allowed.
	switch (setup->baud) {
	#if CLOCK == SIX_MEG
	case 1200:
        UBRR1H=0x02;
        UBRR1L=0x70;
		break;
	case 2400:
        UBRR1H=0x01;
        UBRR1L=0x37;
		break;
	case 4800:
    	UBRR1H=0x00;
        UBRR1L=0x9B;
		break;
	case 9600:
        UBRR1H=0x00;
        UBRR1L=0x4D;
		break;	
	case 14400:
        UBRR1H=0x00;
        UBRR1L=0x33;
		break;
    case 19200:
        UBRR1H=0x00;
        UBRR1L=0x26;
		break;
    case 38400:
        UBRR1H=0x00;
        UBRR1L=0x13;
		break;
    case 57600:
        UBRR1H=0x00;
        UBRR1L=0x0C;
		break;
	//case 11520:		//too many errors at this osc frequency: Error = -6.99%	
    //	UBRR1H=0x00;
    //	UBRR1L=0x06;
	//	break;
	default:
		//sprintf(buffer,"default");print1(buffer);
        UBRR1H=0x00;
        UBRR1L=0x4D;
	    break;
	#elif CLOCK == SEVEN_SOMETHING_MEG
	case 1200:
        UBRR1H=0x01;
        UBRR1L=0x7F;
		break;
	case 2400:
        UBRR1H=0x00;
        UBRR1L=0xBF;
		break;
	case 4800:
    	UBRR1H=0x00;
        UBRR1L=0x5F;
		break;
	case 9600:
        UBRR1H=0x00;
        UBRR1L=0x2F;
		break;
	case 14400:
        UBRR1H=0x00;
        UBRR1L=0x1F;
		break;
    case 19200:
        UBRR1H=0x00;
        UBRR1L=0x17;
		break;
    case 38400:
        UBRR1H=0x00;
        UBRR1L=0x0B;
		break;
    case 57600:
        UBRR0H=0x00;
        UBRR0L=0x07;
		break;
    case 11520:
        UBRR0H=0x00;
        UBRR0L=0x03;
		break;
	default:
        UBRR1H=0x00;
        UBRR1L=0x2F;
	    break;
	#endif
	}
    switch (setup->data_bits) {
        case 5:
            cbi(UCSR1B,0x04);
            cbi(UCSR1C,0x04);
            cbi(UCSR1C,0x02);
            break;
        case 6:
            cbi(UCSR1B,0x04);
            cbi(UCSR1C,0x04);
            sbi(UCSR1C,0x02);
            break;
        case 7:
            cbi(UCSR1B,0x04);
            sbi(UCSR1C,0x04);
            cbi(UCSR1C,0x02);
            break;
        case 8:
            cbi(UCSR1B,0x04);
            sbi(UCSR1C,0x04);
            sbi(UCSR1C,0x02);
            break;
        default :
            return false;
            break;
    }
    switch (setup->parity) {
        case 'n':
        case 'N':
            cbi(UCSR1C,0x20);
            cbi(UCSR1C,0x10);
            break;
        case 'o':
        case 'O':
            sbi(UCSR1C,0x20);
            sbi(UCSR1C,0x10);
            break;
        case 'e':
        case 'E':
            sbi(UCSR1C,0x20);
            cbi(UCSR1C,0x10);
            break;
        default :
            return false;
            break;
    }
    switch (setup->stop_bits) {
        case 1:
            cbi(UCSR1C,0x08);
            break;
        case 2:
            sbi(UCSR1C,0x08);
            break;
        default:
            return false;
            break;
    }
    return true;
}

// USART0 Receiver interrupt service routine
interrupt [USART0_RXC] void usart0_rx_isr(void) {
	char status,data;
	char temp[4];
    #ifdef INT_TIMING
    start_int();
    #endif

	status=UCSR0A;
	data=UDR0;
	if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0) {
   		rx_buffer0[rx_wr_index0]=data;

   		if (++rx_wr_index0 == RX_BUFFER_SIZE0)
    		rx_wr_index0=0;

   		if (++rx_counter0 == RX_BUFFER_SIZE0) {
      		rx_counter0=0;
      		//rx_buffer_overflow0=1;
    //  		putchar1('!');
      	}
   	}

   	//hardware flow control
   	if (rx_counter0 > (RX_BUFFER_SIZE0>>1)) {
 	    //putchar1('X');	//SJL - 3.xx.9 commented out
   	    uart0_flowPaused = true;
   	    //stop transfer
   	    UART0_RTS = 1;

   	}

  //  putchar1(data);
   	long_time_up = false;
   	if((status & FRAMING_ERROR) && error.framingErrorCounter != 0xFFFF)
   	    error.framingErrorCounter++;
   	#ifdef INT_TIMING
    stop_int();
    #endif
}

#ifndef _DEBUG_TERMINAL_IO_
// Get a character from the USART0 Receiver buffer
#define _ALTERNATE_GETCHAR_
#pragma used+
char getchar(void) {
	char data;

	char temp_buffer[50];
    //sprintf(temp_buffer,"getchar - entered\r\n");print(temp_buffer);

	while (rx_counter0==0)
		continue;
	data=rx_buffer0[rx_rd_index0];
	if (++rx_rd_index0 == RX_BUFFER_SIZE0) {
		rx_rd_index0=0;
	}
	//#asm("cli")
	SREG &= ~(0x80);
	--rx_counter0;
	SREG |= 0x80;
	//#asm("sei")
	
	//hardware flow control
	if (rx_counter0 < (RX_BUFFER_SIZE0>>2)) {
        if(uart0_flowPaused)
        //putchar1('M');	//SJL - 3.xx.9 commented out
        uart0_flowPaused = false;
        //continue transfer
        UART0_RTS = 0;
    }
	
    //sprintf(temp_buffer,"getchar - data: %c\r\n",data);print(temp_buffer);

	return data;
}
#pragma used-
#endif

// USART0 Transmitter interrupt service routine
interrupt [USART0_TXC] void usart0_tx_isr(void) {
	//Hardware flow control.  Only send if the modem is ready
	//WE DON'T RESTART!
	#ifdef INT_TIMING
    start_int();
    #endif
	if (UART0_CTS == 0)
	{
    	if (tx_counter0) {
       		--tx_counter0;
       		UDR0=tx_buffer0[tx_rd_index0];
       		if (++tx_rd_index0 == TX_BUFFER_SIZE0)
    			tx_rd_index0=0;
       	}
       	uart0_held = 0;
    } else {
        uart0_held = 1;
    }
    #ifdef INT_TIMING
    stop_int();
    #endif
}

#ifndef _DEBUG_TERMINAL_IO_
// Write a character to the USART0 Transmitter buffer
#define _ALTERNATE_PUTCHAR_
#pragma used+

void putchar(char c) {
    if(sequential_plus >= 2 && c == '+') return;                //these two lines stop us entering command mode
    if(c == '+') sequential_plus++; else sequential_plus = 0;
	while (tx_counter0 == TX_BUFFER_SIZE0);
	#ifdef HW_REV == 6
	while (UART0_CTS) putchar1('*');//waiting for CTS to be high (good to send)
	#endif
	#asm ("cli")
	if (tx_counter0 || ((UCSR0A & DATA_REGISTER_EMPTY)==0)) {
   		tx_buffer0[tx_wr_index0]=c;
   		if (++tx_wr_index0 == TX_BUFFER_SIZE0)
			tx_wr_index0=0;
   		++tx_counter0;
   	}
	else
   		UDR0=c;
	#asm("sei")
}

#pragma used-
#endif


// USART1 Receiver interrupt service routine
interrupt [USART1_RXC] void usart1_rx_isr(void) {
	char status,data;
	#ifdef INT_TIMING
    start_int();
    #endif
	status=UCSR1A;
	data=UDR1;
    #asm("cli")
	if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0) {
   		rx_buffer1[rx_wr_index1]=data;
   		if (++rx_wr_index1 == RX_BUFFER_SIZE1)
			rx_wr_index1=0;


   		if (++rx_counter1 == RX_BUFFER_SIZE1) {
      		rx_counter1=0;
      		//rx_buffer_overflow1=1;
      	}

      	//Xon/Xoff flow control
   		if (rx_counter1 > (RX_BUFFER_SIZE1-64)) {
   		    uart1_flowPaused = true;
#if HW_REV == V5 || HW_REV == V6
   		    //stop transfer
   		    UART1_RTS = 1;
#else
            putchar1(19);
#endif
   		}
   	}
   	#asm("sei")
   	#ifdef INT_TIMING
    stop_int();
    #endif
}

// Get a character from the USART1 Receiver buffer
#pragma used+
char getchar1(void) {
	char data;
	while (rx_counter1==0);
	data=rx_buffer1[rx_rd_index1];
	if (++rx_rd_index1 == RX_BUFFER_SIZE1)
		rx_rd_index1=0;
	#asm("cli")
	--rx_counter1;
	#asm("sei")

	//Xon/xoff flow control
	if (uart1_flowPaused && rx_counter1 < 20) {
        uart1_flowPaused = false;
        //continue transfer
    #if HW_REV == V5 || HW_REV == V6
        UART1_RTS = 0;
    #else
        putchar1(17);
    #endif
    }

	return data;
}
#pragma used-
// USART1 Transmitter buffer
//#if  (PRODUCT == EDAC320 || PRODUCT == EDAC321) && MODEM_TYPE == Q24NG_PLUS
//#define TX_BUFFER_SIZE1 16
//#else
//#define TX_BUFFER_SIZE1 128
//#endif
//char tx_buffer1[TX_BUFFER_SIZE1];

//#if TX_BUFFER_SIZE1<256
//volatile unsigned char tx_wr_index1,tx_rd_index1,tx_counter1;
//#else
//volatile unsigned int tx_wr_index1,tx_rd_index1,tx_counter1;
//#endif

// USART1 Transmitter interrupt service routine
interrupt [USART1_DRE] void usart1_tx_isr(void) {
    #ifdef INT_TIMING
    start_int();
    #endif
    if(
    #if HW_REV == V6 || HW_REV == 5
        UART1_CTS == 1 ||
    #endif
        tx_counter1 == 0)   //flow control enabled or nothing left to send
    {
        UCSR1B &=~ 0x20; //disable DRE interrupt
    }
    else
    {
        tx_counter1--;
       	UDR1=tx_buffer1[tx_rd_index1];
   	    if (++tx_rd_index1 == TX_BUFFER_SIZE1)
		   	tx_rd_index1=0;
    }
    #ifdef INT_TIMING
    stop_int();
    #endif
}

// Write a character to the USART1 Transmitter buffer
#pragma used+
void putchar1(char c) {
    int i;

    //check to see where flow control for UART1 is at
    //if enabled then enable interrupts on DATA
    //DISCARD DATA IF THE BUFFER IS FULL. NEEDS CHANGING!!
    i=30001;	//30000
	while (tx_counter1 >= TX_BUFFER_SIZE1-2)
	{
	    tickleRover();
	    #if HW_REV == V6 || HW_REV == 5
        if(UART1_CTS == 0) UCSR1B |= 0x20;
        #endif
		i--;
        if(i<=0)	//if(!i--)
            return;
    }
	#asm("cli")
	if (tx_counter1 > 0 || ((UCSR1A & DATA_REGISTER_EMPTY)==0)) {
    	tx_counter1++;
   		tx_buffer1[tx_wr_index1]=c;
   		if (++tx_wr_index1 == TX_BUFFER_SIZE1)
			tx_wr_index1=0;
   	}
	else
	{
        UDR1=c;
	    UCSR1B |= 0x20;
    }
	#asm("sei")
}
#pragma used-

//print to uart1
void print(char *str) {

    char p;

    while (uart_send_paused); //this will be changed on an interrupt when the XON is received

    if (modem_TMEnabled)
    {
        #pragma warn-
        while ((p=*str++)) {
            putchar(p);
        }
    }
    else
    {
        while ((p=*str++)) {
            putchar1(p);
        }
        #pragma warn+
    }
	return;
}

void print0(char *str) {

    char p;
    #pragma warn-

        while (p=*str++) {
            putchar(p);
        }

    #pragma warn+
	return;
}

void print1(char *str) {

    char p;
    #pragma warn-

        while (p=*str++) {
            putchar1(p);
        }

    #pragma warn+
	return;
}

void print0_eeprom(eeprom char *p) {
    char k;
    #pragma warn-
    while (k=*p++) putchar(k);
    #pragma warn+
    return;
}

void print_eeprom(eeprom char *p) {
    while(*p != '\0')
    {
        if(modem_TMEnabled)
            putchar(*(p++));
        else
            putchar1(*(p++));
    }
}

void print_char(char c)
{
    if(modem_TMEnabled)
        putchar(c);
    else
        putchar1(c);
}

void print1_eeprom(eeprom char *p) {
    char k;
    #pragma warn-
    if (modem_TMEnabled)
        while (k=*p++) putchar(k);
    else
        while (k=*p++) putchar1(k);
    #pragma warn+
    return;
}

/*
    User level function to call to read any new data from uart 1.
    This returns false if there is not a full command ready to read in
    (ie a \n hasn't been received yet.  When there is a new complete
    command written into the uart_rx_string buffer, the function
    will return true.  The user can then read the buffer.
    At this point, do not call this function again untill you have
    finished with the uart_rx_string buffer, otherwise it will
    be over written.
*/

 bool uart_readPort(void)
 {

    char c;
    //If in modem through mode, read from uart0 (the modem)
    if (modem_TMEnabled)
    {
        //Is there data available?
        if (rx_counter0 <= 0)
            return false;
        //Read in the new char
        c = getchar();
        //putchar1(c);

        if(c == NO_CARRIER[no_carrier_count])
        {
            no_carrier_count++;
           // sprintf(buffer,"\r\n%c is %d in NO CARRIER\r\n",c,no_carrier_count);
           // print1(buffer);
        }
        else
            no_carrier_count = 0;
        if(no_carrier_count>=10)
        {
            enter_command_mode();
            modem_disconnect_csd();
            return false;
        }

        //Echo
        if (uart_echo)
            putchar(c);
    }
    //Normally read from uart1
    else
    {
        //Is there data available?
        if (rx_counter1 <= 0)
            return false;
        //Read in the new char
        c = getchar1();
        if (uart_echo)
            putchar1(c);
    }

    //Remove the case sensitivity
    if (uart_caps)
        c = toupper(c);
    //Delay
    delay_us(1);

    //See what the char is and handle accordingly
    switch (c)

    {
        case '\0' :
        //Do nothing, Don't want null characters
            break;
        case '\n' :
        //Do nothing, Don't want \n characters
            break;
        case 0x8:   //delete character
            if(rx_write>0)
            {
                uart_rx_string[rx_write] = '\0';
                //remove the char from the terminal
                if (modem_TMEnabled)
                {
                    putchar(0x20);
                    putchar(0x8);
                }
                else
                {
                    putchar1(0x20);
                    putchar1(0x8);
                }
                //allow the char to be overwritten
                rx_write--;
            }
            break;
        case '\r' :
            //This is the EOL char. Set the uart_line_received flag, and add a
            //NULL char to end of string
            if (rx_write > 0)    //Make sure that there is something in the string
            {
                uart_rx_string[rx_write] = '\0';
                rx_write=0;
                if(uart_echo)
                {
                    if (modem_TMEnabled)
                        putchar('\n');
                    else
                        putchar1('\n');
                }
                uart_caps=true;

                sprintf(buffer,"[%s]",uart_rx_string);	//SJL - CARV2 - debug
            	log_line("system.log",buffer);          //SJL - CARV2 - debug

                //delay_ms(2);	//SJL - CAVR2 - delay required to load config successfully

                return true;
            }
            break;
        default :
            //Otherwise just append the character to the string
            if (c == '=')
                uart_caps=false;
            uart_rx_string[rx_write++] = c;
            //Catch a software overflow
            if (rx_write >= UART_SWBUFFER_LEN-1)
            {
                DEBUG_printStr("UART SW buffer overflowed...\r\n");
                rx_write--;
                //exit if there is an overflow...
                uart_rx_string[rx_write] = '\0';
                rx_write=0;
                if (modem_TMEnabled)
                    putchar('\n');
                else
                    putchar1('\n');
                return true;
            }
            break;
    }

    return false;

}

/*
    uart read use for config only
*/
bool uart_readPortNoEcho(void)
 {

    char c;

    //Read in the new char
    if (modem_TMEnabled)
    {
        if (rx_counter0 <= 0)
            return false;
        c = getchar();
    }
    else
    {
        if (rx_counter1 <= 0)
            return false;
        c = getchar1();
    }

    //we do not want to echo or uppercase these characters

    //Delay
    delay_us(1);

    //See what the char is and handle accordingly
    switch (c)

    {
        case '\0' :
        //Do nothing, Don't want null characters
            break;
        case '\n' :
        //Do nothing, Don't want null characters
            break;
        case 0x8:   //delete character
            uart_rx_string[rx_write] = '\0';
            rx_write--;
            break;
        case '\r' :
            //This is the EOL char. Set the uart_line_received flag, and add a
            //NULL char to end of string
            uart_rx_string[rx_write] = '\0';
            rx_write=0;

            //sprintf(buffer,"uart.c received - [%s]\r\n",uart_rx_string);	//SJL - CARV2 - debug
            //log_line("system.log",buffer);              					//SJL - CARV2 - debug

            return true;
            break;
        default :
            //Otherwise just append the character to the string
            uart_rx_string[rx_write++] = c;
            break;
    }
    if(strstrf(uart_rx_string,"NO CARRIER"))
    {
    	//sprintf(buffer,"[%s]\r\n",uart_rx_string);	//SJL - CARV2 - debug
        //log_line("system.log",buffer);              //SJL - CARV2 - debug
        return true;
    }
    return false;

}

/*
    uart read for config strings.  the delimters are changed.
*/
bool uart_readConfigSegment(void) {
	char c;

	//Read in the new char
    if (modem_TMEnabled)
    {
        if (rx_counter0 <= 0)
            return false;
        c = getchar();
    }
    else
    {
        if (rx_counter1 <= 0)
            return false;
        c = getchar1();
    }
    delay_us(1);

    switch (c)
    {
        case '~' :
            config_continue=false;
            //Set count back to 0 for next time through
        	rx_write=0;
        	return true;
            break;
        case CONFIG_DELIMITER :
        case CONFIG_BREAK:
        case CONFIG_ALARM:
            config_break=true;
            uart_rx_string[rx_write] = '\0';
        	//Set count back to 0 for next time through
        	rx_write=0;
       	   // sprintf(buffer, "CONFIG_BREAK met [%s]\r\n", uart_rx_string);print(buffer);
        	return true;
            break;
        default :
            uart_rx_string[rx_write++] = c;
            break;
    }
    return false;
}

/*
    parser for new strings received by the uart.
    This works out what the command was, and handles it appropriately.
*/
bool uart_handleNew(char *cmd)
{
    char i,c;
    char *j,*k;
	//char exit_TM;
	char last=0;
	Event e;
	char temp[19];
	eeprom char *contact_temp;
		
    command_string = cmd;
    DEBUG_printStr("Processing: ");
    #ifdef DEBUG
    print(cmd);
    #endif
    DEBUG_printStr("\r\n");
    if (strcmpf(command_string,"AT") == 0)
    {
       sprintf(buffer,"OK\r\n");
       print(buffer);
    }
    /* AT+DNSLOOKUP
	else if (strstrf(command_string,"AT+DNSLOOKUP="))
    {
		// ********************************************************************************************* \\
		//CONNECT TO DNS SERVER TO RESOLVE AN IP ADDRESS

		connect_type = 'D';

		//SJL - Debug -->
		//dns0 and dns1 need to be set in configuration
		strcpyef(config.dns0,"8.8.8.8");	//203.50.170.2
		strcpyef(config.dns1,"8.8.4.4");
		//SJL - <-- End of Debug

		dns_server = 0;
		dns_retries = 0;

		do
		{
			if(dns_server==0)
				strcpyre(ip_address,config.dns0);
			else
				strcpyre(ip_address,config.dns1);

			//SJL - Debug -->
			sprintf(buffer,"Using DNS server %d: %s [attempt %d]\r\n",dns_server,ip_address,dns_retries+1);
			print1(buffer);
			//SJL - <-- End of Debug

			if(open_socket(&connect_type, ip_address))
			{

				// ********************************************************************************************* \\
				//USE DNS CONNECTION TO RESOLVE IP ADDRESS OF SMTP SERVER

				//SJL - Debug -->
				//needs to be set in configuration
				//strcpyef(config.smtp_serv,"smtp2.vodafone.net.nz");
				//SJL - <-- End of Debug

				strcpy(send_string,command_string+13);
				sprintf(buffer,"URL to be resolved: %s\r\n",command_string+13);print1(buffer);

				//j is a pointer to the IP address resolved in get_IP_address
				//j is used previously as a pointer in uart_handle_new so reusing it here
				j=get_IP_address(send_string);

				// ********************************************************************************************* \\
				//DISCONNECT FROM DNS SERVER

				if(!close_socket())
				{
					//retry 3 times
					//then modem restart?
				}

				if(j)
				{
					sprintf(buffer,"The resolved IP address is: %s\r\nOK\r\n",j);print1(buffer);
					break;
				}
				else
				{
					sprintf(buffer,"DNS ERROR: Lookup failed\r\n");print1(buffer);
					dns_retries++;	//try again
				}
			}
			else
			{
				sprintf(buffer,"DNS ERROR: Server failed to reply CONNECT [%s]\r\n",modem_rx_string);print1(buffer);
				dns_retries++;	//try again
			}

			if(dns_retries==2)
			{
				dns_server++;	//try the next dns (1) server
				dns_retries=0;	//and reset the retry counter
			}
		}
		while(dns_server<=1);


    }
    */
	//Display how to use this command
    else if (strcmpf(command_string,"AT+SMS=?") == 0)
    {
        //Detail how to use the command
        if(!modem_TMEnabled)
        {
            sprintf(buffer,"+SMS=\"<phone number>\",<message><CR><LF>\r\n");
            print(buffer);
        }
        else
        {
            sprintf(buffer,"ERROR\r\n");
            print(buffer);
        }
    }
    //Send an SMS from uart insructions.
    else if (strstrf(command_string,"AT+SMS"))
    {
        #ifdef DEBUG
        sprintf(buffer,"working with %s\r\n",command_string);
        print(buffer);
        #endif
        k = strchr(command_string,'"');
        if(!k)
        {
            #ifdef DEBUG
            sprintf(buffer,"no first \" found\r\n");
            print(buffer);
            #endif
            printStr("ERROR\r\n");
            return false;
        }
        else
        {
            *k = '\0';
            k++;
            #ifdef DEBUG
            sprintf(buffer,"number = %s\r\n",k);
            print(buffer);
            #endif
        }
        j = strchr(k,'"');
        if(!j)
        {
            #ifdef DEBUG
            sprintf(buffer,"no second \" found\r\n");
            print(buffer);
            #endif
            printStr("ERROR\r\n");
            return false;
        }
        *j = '\0';
        j+=2;
        if(!modem_TMEnabled)
        {
            DEBUG_printStr(">UART: Send SMS Command received\r\n");

            #ifdef DEBUG
            sprintf(buffer,"sending %s to %s\r\n",j,k);
            print(buffer);
            #endif
            sprintf(sms_newMsg.txt,"%s",j);
            sprintf(sms_newMsg.phoneNumber,"%s",k);
            sms_newMsg.usePhone = true;
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"40\r\n");print1(buffer);
			#endif
			
            msg_sendData();			
            sprintf(buffer,"OK\r\n");
            print(buffer);
        }
        else
        {
            sprintf(buffer,"ERROR\r\n");
            print(buffer);
        }
    }
    else if (strstrf(command_string,"AT+BAUD=?"))
    {
        //What are the available baud rates?
        DEBUG_printStr(">UART: +BAUD? Command received\r\n");
        //Respond to the command
        sprintf(buffer,"BAUD: {1200,2400,4800,9600,14400,19200,38400,57600}\r\n");
        print(buffer);
    }
    else if (strstrf(command_string,"AT+BAUD?"))
    {
        //Return the current baud rate
        DEBUG_printStr(">UART: +BAUD=? Command received\r\n");
        sprintf(buffer,"+BAUD=%u\r\n",uart1_setup.baud);
        print(buffer);
    }
    //Set the baud rate
    else if (strncmpf(command_string,"AT+BAUD=",8) == 0)
    {
        e.type = event_UART_REC;
        e.param = UART_SET_BAUD;
        queue_push(&q_event, &e);
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    #ifdef CALIBRATED
    else if (strstrf(command_string,"AT+CALIBRATE?"))
    {
        for(i=0;i<MAX_INPUTS;i++)
        {
            sprintf(buffer,"Input %d: V=%d,I=%d\r\n",i+1,adc_voltage_offset[i],adc_current_offset[i]);
            print(buffer);
        }

    }
    else if (strncmpf(command_string,"AT+CALIBRATE_V",14) == 0)
    {
        //What are the parameters to set up the comm port
        sprintf(buffer,"Calibrating voltage. Please ensure there is 5V across each input.\r\n");
        print(buffer);
        adc_calibrate_voltage();
    }
    else if (strncmpf(command_string,"AT+CALIBRATE_I",14) == 0)
    {
        //What are the parameters to set up the comm port
        sprintf(buffer,"Calibrating current. Please ensure there is 20mA supplied to each input.\r\n");
        print(buffer);
        adc_calibrate_current();
    }
    #else
    #warning "Not calibratable"
    #endif

    else if (strncmpf(command_string,"AT+COMM=?",9) == 0)
    {
        //What are the parameters to set up the comm port
        DEBUG_printStr(">UART: +COMM? Command received\r\n");
        sprintf(buffer,"+COMM: <data>(7,8),<parity>(O,E,N),<stop>(1,2)\r\n");
        print(buffer);
    }
    else if (strncmpf(command_string,"AT+COMM?",8) == 0)
    {
        //Return the current comm setup up.  This allows for configuration of the
        //number of data bits, parity and stop bits.
        DEBUG_printStr(">UART: +COMM=? Command received\r\n");
        sprintf(buffer,"+COMM=%u,%c,%u\r\n",uart1_setup.data_bits,uart1_setup.parity,uart1_setup.stop_bits);
        print(buffer);
    }
    else if (strncmpf(command_string,"AT+COMM",7) == 0)
    {
        //Set up the comm port
        e.type = event_UART_REC;
        e.param = UART_SETUP;
        queue_push(&q_event, &e);
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    /* Configuration commands */
    else if (strncmpf(command_string,"AT+CONFIG",9) == 0)
    {
        input_int_sms = false;		//SJL - disable input message sending from adc_isr

        //Turn off the input checking while handling the config
        input_pause();
        //Read in the config file through the uart		
		
        config_read_file();
        //Turn the input checking back on.				
		
		//print(buffer);sprintf(buffer,"resume input proc\r\n");print(buffer);
		
		input_int_sms = true;		//SJL - enable input message sending from adc_isr
		
        input_resume();
        pulse_restart();
			
        //SJL - send a status message after config dl
        //SJL - not implememted due to problems with config dl and unit reseting continually
		//startup_send_status=true;
        //startup_send_delay=30;
    }
    else if (strncmpf(command_string,"AT+PRINTEVENTQ",16) == 0)
    {
        queue_print(&q_event);
    }
    else if (strstrf(command_string,"AT+PRINTMODEMQ") != 0)
    {
        queue_print(&q_modem);
    }
    #if LOGGING_AVAILABLE
    else if (strncmpf(command_string,"AT+LOG",6) == 0)
    {
        if(strstrf(command_string,"=")!=0)
        {
			if(log_line("log.csv",strstrf(command_string,"=")+1))
            {
				if(log_line("latest.csv",strstrf(command_string,"=")+1)) //added 3.xx.7
					printStr("OK\r\n");
				else
					printStr("ERROR\r\n");
			}
            else
                printStr("ERROR\r\n");
        }
        else
            printStr("ERROR\r\n");
    }
    #endif
    //print the checksum for the configuration
    else if (strstrf(command_string,"AT+CHKSUM")!=0)
    {
        DEBUG_printStr(">UART: print the checksum\r\n");
        input_pause();
        config_print_file(true);
        input_resume();
    }
    //print the checksum for the contacts
    else if (strstrf(command_string,"AT+CONTACTCHKSUM")!=0)
    {
        DEBUG_printStr(">UART: print the contact checksum\r\n");
        config_print_contacts(true);
    }
    else if (strstrf(command_string,"AT+PRINTCONTACTS")!=0)
    {
        config_print_contacts(false);
    }
	
	#if LOGGING_AVAILABLE
	else if (strncmpf(command_string,"AT+PRINTLOG",11) == 0)
    {
		//new command v3.20.1
		//Print the log.csv file to RS232 port
		//AT+PRINTLOG	Print the complete log.csv file
		//AT+PRINTLOG=<from>-<to>
		//valid from and to strings:
		// yyyy/mm/dd hh:mm:ss;
		// yyyy/mm/dd;
		// start (beginning of file);
		// end (end of file);
		// latest (last entry emailed in regular update - from save.txt)
        if(strstrf(command_string,"=")==0) //no equals - print complete log
		{
			sprintf(command_string,"AT+PRINTLOG=start-end");
		}
		
		//copy the selected dates to the dates string (mmc.c) for further processing
		if(cmdDates_to_mmcDates(command_string))
		{
			print_log();
		}
    }
	
	#if EMAIL_AVAILABLE	
	else if (strncmpf(command_string,"AT+PRINTLATEST",14) == 0) //added 3.xx.7
	{	
		print_latest();		
	}

	//#if EMAIL_AVAILABLE	
	else if (strncmpf(command_string,"AT+EMAILLOG",11) == 0)
	{
		//new command v3.20.1
		//Email the log.csv file (DON'T save last date and time sent to hidden file) to update address
		//options as for AT+PRINTLOG above
		if(strstrf(command_string,"=")==0) //no equals - print complete log
		{
			sprintf(command_string,"AT+EMAILLOG=start-end");
		}
		
		//copy the selected dates to the dates string (mmc.c) for further processing
		if(cmdDates_to_mmcDates(command_string))
		{
			concat_0();		// set flag - select the log.csv file
			//concat_0();		// set flag - don't save latest date and time to save.txt - redundant 3.xx.7
			
			#if PULSE_COUNTING_AVAILABLE
			if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
			{
				pulse_instant_pause = 1;
				//EIMSK &= ~(0xC0);	//disable pulse interrupts
				//EIFR &= ~(0xC0);	//clear pulse interrupt flags
			}
			#endif
			
			
			if(email_log())	//email the log
			{
				sprintf(buffer,"OK");print1(buffer);
			}
			else
			{
				sprintf(buffer,"ERROR");print1(buffer);
			}			
			
			#if PULSE_COUNTING_AVAILABLE
			if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
			{
				pulse_restart();	//restart pulse inputs 7 and 8
				pulse_instant_pause = 0;
			}
			#endif

			//modem_check_status();			
		}		
	}
	
	else if (strncmpf(command_string,"AT+EMAILLATEST",14) == 0) //added 3.xx.7
	{			
		#if PULSE_COUNTING_AVAILABLE
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			pulse_instant_pause = 1;
			//EIMSK &= ~(0xC0);	//disable pulse interrupts
			//EIFR &= ~(0xC0);	//clear pulse interrupt flags
		}
		#endif
		
		i=0;
		if(email_latest(&i))	//email the log
		{
			sprintf(buffer,"OK");print1(buffer);
		}
		else
		{
			sprintf(buffer,"ERROR");print1(buffer);
		}			
		
		#if PULSE_COUNTING_AVAILABLE
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			pulse_restart();	//restart pulse inputs 7 and 8
			pulse_instant_pause = 0;
		}
		#endif
		
		//modem_check_status();
	}
	
	//Email a log file (SAVE last date and time sent to hidden file)
	/*else if (strncmpf(command_string,"AT+ELOGSAVE",11) == 0) //removed 3.xx.7
	//new command v3.20.1
	//Email a log file (SAVE last date and time sent to hidden file) to update address
	//This command is exactly the same as a regular email update
	//no options
    {	
		//removed due to memory problems
		// if(strstrf(command_string,"=")==0) //no equals - print complete log
		// {
			// sprintf(command_string,"AT+ELOGSAVE=start-end");
		// }
		
		// //copy the selected dates to the dates string (mmc.c) for further processing
		// if(cmdDates_to_mmcDates(command_string))
		// {					
			//concat_0();
			//concat_1();
			
			// if(email_log())	//QUEUE? like update
			// {
				// sprintf(buffer,"OK");print1(buffer);
			// }
			// else
			// {
				// sprintf(buffer,"ERROR");print1(buffer);
			// }
		// }
		
		//queuing the send like for log updates because the above code was not saving properly to the hidden file
		//not sure of the reason atm but looks like might be a memory issue
		e.type = sms_LOG_UPDATE;
		e.param = 0;
		queue_push(&q_modem, &e);
		
	}
	*/
	#endif
	#endif
	
    #if LOGGING_AVAILABLE
    else if (strncmpf(command_string,"AT+PRINTEVENT",13) == 0)
	{
		//new command v3.20.1
		//Print the alarm.log file to RS232 port
		//AT+PRINTEVENT	Print the complete log.csv file
		//AT+PRINTEVENT=<from>-<to>
		//valid from and to strings:
		// yyyy/mm/dd hh:mm:ss;
		// yyyy/mm/dd;
		// start (beginning of file);
		// end (end of file);
        if(strstrf(command_string,"=")==0) //no equals - print complete log
		{
			sprintf(command_string,"AT+PRINTEVENT=start-end");
		}
		
		//copy the selected dates to the dates string (mmc.c) for further processing
		if(cmdDates_to_mmcDates(command_string))
		{
			print_alarm();
		}
    }
	#if EMAIL_AVAILABLE
	else if (strncmpf(command_string,"AT+EMAILEVENT",13) == 0)
	{
		//new command v3.20.1
		//Email the alarm.log file to update address
		//options as for AT+PRINTEVENT above
		if(strstrf(command_string,"=")==0) //no equals - print complete log
		{
			sprintf(command_string,"AT+EMAILEVENT=start-end");
		}
		
		//copy the selected dates to the dates string (mmc.c) for further processing
		if(cmdDates_to_mmcDates(command_string))
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
			
			if(email_log())
			{
				sprintf(buffer,"OK");print1(buffer);
			}
			else
			{
				sprintf(buffer,"ERROR");print1(buffer);
			}
			
			#if PULSE_COUNTING_AVAILABLE
			if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
			{
				pulse_restart();	//restart pulse inputs 7 and 8
				pulse_instant_pause = 0;
			}
			#endif
			
			//modem_check_status();
		}
    }	
    #endif
	#endif
	
	#if ERROR_FILE_AVAILABLE
    else if (strncmpf(command_string,"AT+PRINTERROR",13) == 0)	
    {
		//new command v3.20.4
		//Print the error.log (hidden) file to RS232 port
		//options as for AT+PRINTEVENT above
        if(strstrf(command_string,"=")==0) //no equals - print complete log
		{
			sprintf(command_string,"AT+PRINTERROR=start-end");
		}
		
		//copy the selected dates to the dates string (mmc.c) for further processing
		if(cmdDates_to_mmcDates(command_string))
		{
			print_error();
		}
    }
	#if EMAIL_AVAILABLE
	else if (strncmpf(command_string,"AT+EMAILERROR",13) == 0)    
	{
		//new command v3.20.4
		//Email the error.log (hidden) file to update address
		//options as for AT+PRINTEVENT above
		if(strstrf(command_string,"=")==0) //no equals - print complete log
		{
			sprintf(command_string,"AT+EMAILERROR=start-end");
		}
		
		//copy the selected dates to the dates string (mmc.c) for further processing
		if(cmdDates_to_mmcDates(command_string))
		{
			#if PULSE_COUNTING_AVAILABLE
			if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
			{
				pulse_instant_pause = 1;
				//EIMSK &= ~(0xC0);	//disable pulse interrupts
				//EIFR &= ~(0xC0);	//clear pulse interrupt flags
			}
			#endif
		
			if(email_error())
			{
				sprintf(buffer,"OK");print1(buffer);
			}
			else
			{
				sprintf(buffer,"ERROR");print1(buffer);
			}
			
			#if PULSE_COUNTING_AVAILABLE
			if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
			{
				pulse_restart();	//restart pulse inputs 7 and 8
				pulse_instant_pause = 0;
			}
			#endif
			
			//modem_check_status();
		}
    }	
    #endif
	#endif
	
    #if SYSTEM_LOGGING_ENABLED
    else if (strncmpf(command_string,"AT+PRINTSYSTEM",14) == 0)
    {
	//new command v3.20.1
	//Print the system.log file to RS232 port
	//options as for AT+PRINTEVENT above
		if(strstrf(command_string,"=")==0) //no equals - print complete log
		{
			sprintf(command_string,"AT+PRINTSYSTEM=start-end");
		}
		
		//copy the selected dates to the dates string (mmc.c) for further processing
		if(cmdDates_to_mmcDates(command_string))
		{
			print_system();
		}
    }
	#if EMAIL_AVAILABLE
	else if (strncmpf(command_string,"AT+EMAILSYSTEM",14) == 0)	
    {
		//new command v3.20.1
		//Email the system.log file to update address
		//options as for AT+PRINTEVENT above
		if(strstrf(command_string,"=")==0) //no equals - print complete log
		{
			sprintf(command_string,"AT+EMAILSYSTEM=start-end");
		}
		
		//copy the selected dates to the dates string (mmc.c) for further processing
		if(cmdDates_to_mmcDates(command_string))
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
			
			if(email_log())
			{
				sprintf(buffer,"OK");print1(buffer);
			}
			else
			{
				sprintf(buffer,"ERROR");print1(buffer);
			}
			
			#if PULSE_COUNTING_AVAILABLE
			if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
			{
				pulse_restart();	//restart pulse inputs 7 and 8
				pulse_instant_pause = 0;
			}
			#endif
			
			//modem_check_status();
		}
    }
	#endif
	#endif
	
    else if (strncmpf(command_string,"AT+PRINT",8) == 0)
    {

        //Turn off the input checking while handling the config
        input_pause();
        //Print out the current configuration into a file format.
        config_print_file(false);
        //Turn the input checking back on.
        input_resume();
        pulse_restart();
 	}
    /* End of Configuration commands */
    else if (strncmpf(command_string,"@AT",3) == 0)
    {
        if(modem_TMEnabled)
        {
            if(verbose)
            {
                sprintf(buffer,"ERROR\r\n");
                print(buffer);
            }
        }
        else
        {
			sprintf(buffer,"%s\r\n",command_string+1);
			print0(buffer);		
			
			//timer=10;
			//timer_overflow = false;
			
			do	//keep reading from modem until OK, ERROR, NC or time-out
			{
				tickleRover();
				modem_read();
				print1(modem_rx_string);
				sprintf(buffer,"\r\n");print1(buffer);
			} while(!(strstrf(modem_rx_string,"OK")||strstrf(modem_rx_string,"ERROR")||strstrf(modem_rx_string,"NO CARRIER"))
				&& (!modem_timeOut)); //&& (!timer_overflow)
		}
    }
    //Return the value fo the input
    else if (strncmpf(command_string,"AT+IN",5) == 0)
    {
        e.type = event_UART_REC;
        e.param = UART_READ_INPUT;
        queue_push(&q_event, &e);
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    else if (strncmpf(command_string,"AT+ADC",6) == 0)
    {
        for(i=0;i<MAX_INPUTS;i++)
        {
            sprintf(buffer,"in%d=%f;",i+1,input[i].ADCVal);
            print(buffer);
        }
        printStr("\r\n");
    }
    //Switch all outputs
    else if (strncmpf(command_string,"AT+OUTALL=",9) == 0)
    {
        e.type = event_UART_REC;
        e.param = UART_SWITCH_OUTPUTALL;
        queue_push(&q_event, &e);
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    //Read or control the outputs
    else if (strncmpf(command_string,"AT+OUT",6) == 0)
    {
        //Switch an output
        if (strstrf(command_string, "=") != 0)
        {
            e.type = event_UART_REC;
            e.param = UART_SWITCH_OUTPUT;
            queue_push(&q_event, &e);
        }
        else
        {
            e.type = event_UART_REC;
            e.param = UART_READ_OUTPUT;
            queue_push(&q_event, &e);
        }
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    //Return the signal quality
    else if (strcmpf(command_string,"AT+CSQ") == 0)
    {
        if (!(q_modem.paused||modem_TMEnabled))
        {
            e.type = sms_GET_CSQ;
            e.param = CSQ;
            queue_push(&q_modem, &e);
        }
        else
        {
            sprintf(buffer, "+CSQ=%s,%s\r\n",modem_rssi, modem_ber);
            print(buffer);
        }

    }
    //Return the RSSI
    else if (strcmpf(command_string,"AT+RSSI") == 0)
    {
        if (!(q_modem.paused||modem_TMEnabled)) //doesn't check whether modem is in a data call
        {
            e.type = sms_GET_CSQ;
            e.param = RSSI;
            queue_push(&q_modem, &e);
        }
        else {
            sprintf(buffer, "+RSSI=%s\r\n",modem_rssi);
            print(buffer);
        }
    }
    //Return the bit error rate.
    else if (strcmpf(command_string,"AT+BER") == 0)
    {
        if (!(q_modem.paused||modem_TMEnabled))
        {
            e.type = sms_GET_CSQ;
            e.param = BER;
            queue_push(&q_modem, &e);
        }
        else
        {
            sprintf(buffer, "\r\n+BER=%s\r\n",modem_ber);
            print(buffer);
        }
    }
    else if (strcmpf(command_string,"AT+CNI") == 0)
    {
        //User want to check the status of the network connection (startupError flag)
        e.type = event_UART_REC;
        e.param = UART_CNI;
        queue_push(&q_event, &e);
    }
    else if (strcmpf(command_string,"AT+GPRSSTATE") == 0)
    {
        //check the status of the GPRS connection
		/*
        if(modem_TMEnabled)
        {
            sprintf(buffer, "+GPRSSTATE=CSD\r\n");
            print(buffer);
        }
        else  if (!(q_modem.paused||modem_TMEnabled))
        {
            e.type = sms_GPRSSTATE;
            queue_push(&q_modem, &e);
        }
        else
        {
            sprintf(buffer, "+GPRSSTATE=Busy\r\n");
            print(buffer);
        }
		*/		
		if (startupError)
		{
			printStr("+GPRSSTATE=UNAVAILABLE\r\n");
		}		
		else if (PRODUCT==EDAC320 || PRODUCT==EDAC321)
		{
			printStr("+GPRSSTATE=Idle\r\n");
		}
		else
		{
			printStr("+GPRSSTATE=UNAVAILABLE\r\n");
		}		
    }
    else if (strstrf(command_string,"AT+REMOVEDATA") != 0)
    {
        e.type = mmc_CLEAR_DATA;
        queue_push(&q_event, &e);
    }
    else if (strcmpf(command_string,"AT+RN") == 0)
    {
        //User has requested to attempt to reconnect to the network
        e.type = event_UART_REC;
        e.param = UART_RN;
        queue_push(&q_event, &e);
    }
    else if (strcmpf(command_string,"AT+SERIAL?") == 0)
    {
        //Get the serial number of the unit.
        e.type = event_UART_REC;
        e.param = UART_SERIAL;
        queue_push(&q_event, &e);
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    else if (strncmpf(command_string,"AT+SERIAL_LOAD",14) == 0)
    {
        //Load in a new serial number
        e.type = event_UART_REC;
        e.param = UART_SERIAL_LOAD;
        queue_push(&q_event, &e);
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    //Factory setup
    //Serial cmd get the format the time needs to be entered in
    else if (strcmpf(command_string,"AT+TIME=?") == 0)
    {
        //Return the parameters that are used to set the time
        sprintf(buffer,"AT+TIME=\"dd/mm/yy,hh:mm:ss\"\r\n");
        print(buffer);
    }
    //Serial cmd print the current time stored in teh RTC
    else if (strcmpf(command_string,"AT+TIME?") == 0)
    {
        //Return the time stored in the RTC
        e.type = event_UART_REC;
        e.param = UART_GET_TIME;
        queue_push(&q_event, &e);
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    //Serial cmd to update the time in the RTC
    else if (strncmpf(command_string,"AT+TIME=\"",9) == 0)
    {
        //Update the time stored in the RTC
        e.type = event_UART_REC;
        e.param = UART_SET_TIME;
        queue_push(&q_event, &e);
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    //This is a factory reset of the entire config.
    else if (strcmpf(command_string,"AT&F") == 0)
    {
        config_factoryReset();
    }
    //This is a hardware reset
    else if (strcmpf(command_string,"AT&R") == 0)
    {
        //Wait for the watchdog to reset the processor.
        while(1);
    }
    //Check the software verison
    else if (strcmpf(command_string,"AT+SVER") == 0)
    {
        sprintf(buffer,"+SVER=");print(buffer);
        //print the part number and the software version.
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
        #elif PRODUCT==EDAC320 //&& MODEM_TYPE==SIERRA_MC8775V
        sprintf(buffer,"EDAC 053-0032 Rev: ");
        #elif PRODUCT==EDAC321 //&& MODEM_TYPE==Q24NG_PLUS
        sprintf(buffer,"EDAC 053-0033 Rev: ");
        #elif PRODUCT==EDAC321 && MODEM_TYPE==Q2438F
        sprintf(buffer,"EDAC 053-0034 Rev: ");
        #elif PRODUCT==SMS300Lite
        sprintf(buffer,"EDAC 053-00xx Rev: ");
        #elif PRODUCT==HERACLES
        sprintf(buffer,"EDAC 053-0030 Rev: ");
		//#elif PRODUCT==EDAC315 && MODEM_TYPE==SIERRA_MC8795V
        //sprintf(buffer,"EDAC 053-0044 Rev: ");	//SJL - consolidated code for all 300 series projects - only 315 so far implemented
        #elif PRODUCT==DEVELOPMENT
        sprintf(buffer,"EDAC DEVELOPMENT Rev: ");
        #endif
        print(buffer);
        sprintf(buffer,"%p.%p\r\n",code_version,buildnumber);print(buffer);
    }

    //Turn the character echo off
    else if (strcmpf(command_string,"ATE0") == 0)
    {
        sprintf(buffer,"OK\r\n");print(buffer);
        uart_echo = false;
    }
    //Turn the character echo on
    else if (strcmpf(command_string,"ATE1") == 0)
    {
        sprintf(buffer,"OK\r\n");print(buffer);
        uart_echo = true;
    }
    else if (strstrf(command_string,"AT+MODEMPAUSE=")!=0)
    {
        if(strstrf(command_string,"0")!=0)
        {
            queue_resume(&q_modem);
            sprintf(buffer,"OK\r\n");
            print(buffer);
        }
        else if(strstrf(command_string,"1")!=0)
        {
            queue_pause(&q_modem);
            modem_queue_stuck=false;
            sprintf(buffer,"OK\r\n");
            print(buffer);
        }
        else if(verbose)
        {
            sprintf(buffer,"ERROR\r\n");
            print(buffer);
        }

    }

    //Return the status of the startup message print out.
    else if (strcmpf(command_string,"AT+CMGS?") == 0)
    {
        //print the current reading of the cmgs variable.
        sprintf(buffer,"+CMGS=%d\r\n", config_cmgs);print(buffer);
    }

    //Return the valid input parameters of the +cmgs command.
    else if (strcmpf(command_string,"AT+CMGS=?") == 0)
    {
        //print the current reading of the cmgs variable.
        sprintf(buffer,"+CMGS={0,1}\r\n", config_cmgs);print(buffer);
    }
    #ifdef DEBUG
    else if (strstrf(command_string,"AT+DUMPMEMORY"))
    {
        dump_memory();
    }
    #endif
    //set the status of the startup message print out.
    else if (strncmpf(command_string,"AT+CMGS=",8) == 0)
    {
        //Update the status of the config_cmgs command
        e.type = event_UART_REC;
        e.param = UART_SET_CMGS;
        queue_push(&q_event, &e);
        //Pause the uart receive until this parameter has been read from the buffer
        return true;
    }
    else if (strstrf(command_string,"AT+MODIFYCONTACT"))
    {
        /*
		if(strchr(command_string,'=')&&contact_modify(strchr(command_string,'=')+1,
                                                        atoi(strstrf(command_string,"AT+MODIFYCONTACT")+16))!=-1)
            printStr("OK\r\n");
        else if(verbose)
            printStr("ERROR\r\n");
		*/				
		
		// 3.xx.7
		k = strstrf(command_string,"AT+MODIFYCONTACT")+16;
		c = atoi(k);
	
		if(strstrf(command_string,"=")!=0)
		{				
			k = strchr(command_string,'=')+1;

			#ifdef _UART_DEBUG_
			sprintf(buffer,"modifying %d to [%s]\r\n",c,k);
			print(buffer);
			#endif
			
			c--;
			if(c<0 || c>15)
			{			
				//#ifdef _UART_DEBUG_
				sprintf(buffer,"Contact %d is an invalid number\r\n",c+1);
				print1(buffer);
				//#endif
			}
			else
			{				
				if(c!=0)
				{
					do
					{
						contact_temp = contact_read(c-1);
						strcpyre(buffer,contact_temp+1);					
						i=strlen(buffer);
						if (!contact_temp || i==0)
						{
							#ifdef _UART_DEBUG_
							sprintf(buffer,"Contact %d is empty, decrementing position\r\n",c-1);
							print1(buffer);
							#endif
							c--;
						}
						else
						{
							#ifdef _UART_DEBUG_
							sprintf(buffer,"Contact %d is occupied, saving as contact %d\r\n",c-1,c);
							print1(buffer);
							#endif
							break;
						}							
					}
					while(c>0);
				}
				
				i = contact_modify(k,c);
				
				if(i==c)
				{							
					sprintf(buffer,"Contact %d changed to \"%s\"",c+1,k);
					//#ifdef _UART_DEBUG_
					print1(buffer);
					//#endif
				}
				else if(i == -1)
				{
					sprintf(buffer,"Contact change failed");
					//#ifdef _UART_DEBUG_
					print1(buffer);
					//#endif
				}			
			}
		}
		else if(verbose)
            printStr("ERROR\r\n");
			
		
    }

    #if GPS_AVAILABLE
    /****** GPS stuff      ******/
    else if (strncmpf(command_string,"$GPGGA",6) == 0 ||      //location NMEA string received
            strncmpf(command_string,"$GPRMC",6) == 0)        //velocity NMEA string received
    {
        gps_process_string(command_string);
    }
    else if (strstrf(command_string,"AT+LOC"))
    {
        printStr("+LOC: ");
        gps_print(buffer,DEGREES_MINUTES);
        print(buffer);
        printStr("\r\nOK\r\n");
    }
    else if (strstrf(command_string,"AT+VELO"))
    {
        printStr("+VELO: ");
        gps_velocity(buffer);
        print(buffer);
        printStr("\r\nOK\r\n");
    }
    #endif

    //Debug command for EDAC use only
    else if (strcmpf(command_string,"**DEBUG**") == 0)
    {
        //A **debug** string was received in the message.
        e.type = event_UART_REC;
        e.param = EDAC_QUEREY;
        queue_push(&q_event, &e);
        DEBUG_printStr("UART: EDAC query\r\n");
    }
    /* Modem through mode commands */
    //in modem through mode, switch back to transparent through mode.

    else if (strcmpf(command_string,"AT+THROUGH") == 0)
    {
        //exit_TM=0;
		sprintf(buffer,"Modem through mode enabled (Ctrl+C to resume normal operation)\r\n");print1(buffer);
		while(rx_counter0)getchar();
        while(rx_counter1)getchar1();
        while(1)
        {
            tickleRover();
            if(rx_counter1)
            {
                i = getchar1();

				/*
				if(exit_TM>=1)
				{
					if((i==88 && exit_TM==1) || (i==120 && exit_TM==1)
					|| (i==84 && exit_TM==2) || (i==116 && exit_TM==2)
					|| (i==77 && exit_TM==3) || (i==109 && exit_TM==3)
					|| (i==13 && exit_TM==4))
					{
						exit_TM++;
					}
					else exit_TM=0;
				}
				*/

				/**
				 *if(i==43)
				 *{
				 *	sprintf(buffer,"+ received");print1(buffer);
				 *	exit_TM++;
				 *	sprintf(buffer,"exit_TM=%d",exit_TM);print1(buffer);

		 * *		}
				 *else exit_TM=0;
				 *
				 *if(exit_TM==3)
				 *{
				 *	exit_TM=0;
				 *	sprintf(buffer,"Sending escape sequence...");print1(buffer);
				 *	delay_ms(1100);
				 *	sprintf(buffer,"+++");print0(buffer);
				 *	//delay_ms(200);
				 *	//sprintf(buffer,"+");print0(buffer);
				 *	//delay_ms(200);
				 *	//sprintf(buffer,"+");print0(buffer);
				 *	//delay_ms(200);
				 *	delay_ms(1100);
				 *}
				 */


				if(i==3)
                {
                    sprintf(buffer,"Exiting modem through mode\r\n");print1(buffer);
					//sprintf(command_string,"\r\nOK\r\n");
                    //print(command_string);
                    break;
                }
                putchar(i);
            }

            if(rx_counter0)
                putchar1(getchar());
        }

    }

    else if (strcmpf(command_string,"AT+EXTM") == 0)
    {
        if (modem_TMEnabled)
        {
            modem_TMEcho = true;
            sprintf(buffer,"OK\r\n");
            print(buffer);
        }
        else if(verbose)
        {
            sprintf(buffer,"ERROR\r\n");
            print(buffer);
        }
    }
    else if (strstrf(command_string, "CONNECT") != 0)
    {
        DEBUG_printStr("CONNECT found\r\n");
        //do nothing
        if (modem_TMEnabled)
        {
            sprintf(buffer,"Connect Sucessful\r\n");print(buffer);
        }
        else if(verbose)
        {
            sprintf(buffer,"ERROR\r\n");print(buffer);
        }
    }
    else if (strstrf(command_string,"NO CARRIER"))
    {
        DEBUG_printStr("NO CARRIER found\r\n");
        if (modem_TMEnabled)
        {
            //Hang up the modem
            modem_disconnect_csd();
            queue_resume(&q_modem);
        }
        else if(verbose)
        {
            sprintf(buffer,"ERROR\r\n");print(buffer);
        }
        e.type = sms_CHECK_NETWORK_INTERNAL;
    }

    /* End of Modem through mode commands */
	
	/* RECMAIL
    //print the value of if the user wants to check emails
    else if (strcmpf(command_string,"AT+RECMAIL?") == 0)
    {
        sprintf(buffer, "+RECMAIL=%d\r\n", email_recMail);
        print(buffer);
    }
    else if (strncmpf(command_string,"AT+RECMAIL=",11) == 0)
    {
        //Update the status of the if the user wants to check email
        e.type = event_UART_REC;
        e.param = UART_RECMAIL;
        queue_push(&q_event, &e);
    }
	*/    

    //set vebosity
    else if (strstrf(command_string,"AT+VERBOSE")!=0)
    {
        if(strstrf(command_string,"AT+VERBOSE=0")!=0)
        {
            verbose = 0;
            config_cmgs = 0;
            uart_echo = 0;
            sprintf(buffer,"OK\r\n");print(buffer);
        }
        else if(strstrf(command_string,"AT+VERBOSE=1")!=0)
        {
            verbose = 1;
            config_cmgs = 1;
            uart_echo = 0;
            sprintf(buffer,"OK\r\n");print(buffer);
        }
        else if(strstrf(command_string,"AT+VERBOSE=2")!=0)
        {
            verbose = 1;
            config_cmgs = 1;
            uart_echo = 1;
            sprintf(buffer,"OK\r\n");print(buffer);
        }
        else if(strstrf(command_string,"AT+VERBOSE?")!=0)
        {
            if(verbose)
                sprintf(buffer,"+VERBOSE=1\r\n");
            else
                sprintf(buffer,"+VERBOSE=0\r\n");
            print(buffer);
        } else if(verbose)
        {
            sprintf(buffer,"ERROR\r\n");print(buffer);
        }

    }

	//FOR PURPOSES OF TESTING
	//else if (strstrf(command_string,"AT+TEST")!=0)
	//{
	//	sprintf(buffer,"Resetting the modem\r\n");print1(buffer);
	//	modem_restart();
	//	sprintf(buffer,"Modem restarted successfully - initializing\r\n");print1(buffer);
	//	modem_init(SIERRA_MC8795V); 
	//	sprintf(buffer,"Modem initialized successfully\r\n");print1(buffer);
	//}

	#if EMAIL_AVAILABLE	
	/*else if (strncmpf(command_string,"AT+LATEST=",10) == 0)
	//new command v3.20.1
	//Set the date and time in the save.txt hidden file
    {
		//check the format of the date and time is correct
		//test for string length and positions of ' ', 
		if( strlen(command_string)==29 
			&& strpos(command_string,'/')==14 && strrpos(command_string,'/')==17 
			&& strpos(command_string,' ')==20 
			&& strpos(command_string,':')==23 && strrpos(command_string,':')==26 )
		{	
			sprintf(sms_newMsg.txt,"%s\r\n",command_string);
			//Write to the save.txt file
			if(!email_log_write_date())
			{
				sprintf(buffer,"ERROR - failed to create hidden file\r\n");
				print1(buffer);
			}
		}
		else
		{
			sprintf(buffer,"ERROR - invalid command\r\n");
			print1(buffer);
		}
	
		//Read from the save.txt file and return the date and time
		
		//command_string=email_log_read_date(command_string);
		sprintf(temp,"xxxx/xx/xx xx:xx:xx");
		command_string=email_log_read_date(temp);
		if(!command_string || command_string==0)
		{
			printStr("File not found\r\n");
		}
		else
		{
			sprintf(buffer,"Date and time saved in hidden file: %s",command_string);
			print1(buffer);
		}

	}
	
	else if (strncmpf(command_string,"AT+LATEST?",10) == 0)
    //new command v3.20.1
	//Query the date and time in the save.txt hidden file
	{
		//command_string=email_log_read_date(command_string);
		sprintf(temp,"xxxx/xx/xx xx:xx:xx");
		command_string=email_log_read_date(temp);
		if(!command_string || command_string==0)
		{
			printStr("File not found\r\n");
		}
		else
		{
			sprintf(buffer,"Date and time saved in hidden file: %s",command_string);
			print1(buffer);
		}

	}
	*/
	
	else if (strncmpf(command_string,"AT+EMAILRETRYTIMER=",19) == 0)
	//new command v3.20.1
	//Set the value of the regular email update retry timer
	{		
		j = strchr(command_string,'=')+1;
		email_retry_period = atoi(j);

		//return the saved value for confirmation
		if(email_retry_period<=0)
		{
			email_retry_period=-1;
			sprintf(buffer,"The timer is disabled\r\n");
		}
		else
		{
			sprintf(buffer,"The timer is set to %d minutes\r\n",email_retry_period);
		}
		print1(buffer);
	}
	
	else if (strncmpf(command_string,"AT+EMAILRETRYTIMER?",19) == 0)
	//new command v3.20.1
	//Query the value of the regular email update retry timer
	{		
		if(email_retry_period==-1)
		{
			sprintf(buffer,"The timer is disabled\r\n");
		}
		else
		{
			sprintf(buffer,"The timer is set to %d minutes\r\n",email_retry_period);
		}
		print1(buffer);	
	}
	#endif
	
	//command not recognised.
    else
    {
        if (strlen(command_string) != 0)
        {
            //The command isn't known to the SMS300
            if(verbose)
            {
                #ifdef DEBUG
                print(command_string);
                #endif
                sprintf(buffer,"ERROR\r\n");
                print(buffer);
            }
        }
    }
    return false;
}

//Runs an 115.2k UART on UART0.
//Courtesy: Mark Atherton
void soft_uart0(unsigned int val)
{
  char loop, dly;

  val =  val << 1;
  val &= 0x01FE;				// bottom bit clear - start bit
  val |= 0xFE00;				// top bits set - stop bit(s)

  // Loop takes 52 cycles to execute

  for(loop = 0; loop < 11; loop++)		// or 10 for 1 stop bit
  {
    #asm
         LDD  R30,Y+2;                          // 2 fetch val off data stack
         ANDI R30,1;                            // 1 if((val & 1)
         BRNE SU0;                              // 1 if False 2 if True
         CBI  0x03,1;                           // 2 Clear Bit Immediate UART = 0;
         RJMP SU1;                              // 2 always true
    SU0: SBI  0x03,1;                           // 2 Set Bit Immediate UART = 1;
         NOP;                                   // 1 NOP, padding
    SU1: NOP;                                   // 1 NOP, last instruction
         NOP;
         NOP;
         NOP;
    #endasm

    val >>= 1;                                  // shift input data
    for(dly = 0; dly < 4; dly++);
  }
}


void soft_uart1(unsigned int val)
{
  char loop, dly;

  val =  val << 1;
  val &= 0x01FE;				// bottom bit clear - start bit
  val |= 0xFE00;				// top bits set - stop bit(s)

  // Loop takes 52 cycles to execute

  for(loop = 0; loop < 11; loop++)		// or 10 for 1 stop bit
  {
    #asm
         LDD  R30,Y+2;                          // 2 fetch val off data stack
         ANDI R30,1;                            // 1 if((val & 1)
         BRNE SU0;                              // 1 if False 2 if True
         CBI  0x12,3;                           // 2 Clear Bit Immediate UART = 0;
         RJMP SU1;                              // 2 always true
    SU0: SBI  0x12,3;                           // 2 Set Bit Immediate UART = 1;
         NOP;                                   // 1 NOP, padding
    SU1: NOP;                                   // 1 NOP, last instruction
         NOP;
         NOP;
         NOP;
    #endasm

    val >>= 1;                                  // shift input data
    for(dly = 0; dly < 4; dly++);
  }
}


#define RESTORE_REGISTERS_0  PORTE = i;DDRE = e;UCSR0B = b;SREG = s;

#define SOFT_UART_1  s = SREG;i = PORTD; e = DDRD;b = UCSR1B;UCSR1B = 0;PORTD = 0x08; DDRD = 0x08;
#define RESTORE_REGISTERS_1  PORTD = i;DDRD = e;UCSR1B = b;SREG = s;

#define SOFT_UART_SETUP SOFT_UART_0
#define SOFT_UART_RESTORE RESTORE_REGISTERS_0
#define SOFT_UART_FUNC soft_uart0

typedef struct uart_status_t
{
    char sreg;
    char port;
    char data_direction;
    char uart_control;
} UartStatus;

UartStatus u;

void disable_uart0()
{
    u.sreg = SREG;
    #asm("cli");
    u.port = PORTE;
    u.data_direction = DDRE;
    u.uart_control = UCSR0B;
    UCSR0B = 0;
    PORTE.0=0;
    PORTE.1=1;
    PORTE.2=0;
    PORTE.3=0;
    DDRE.0=0;
    DDRE.1=1;
    DDRE.2=0;
    DDRE.3=1;
    //DDRE=0x0A;
}

void reenable_uart0()
{
   //char temp;
   PORTE = u.port;
   DDRE = u.data_direction;
   UCSR0B = u.uart_control;
   //temp = UCSR0A;
   //temp = UDR0;
   SREG = u.sreg;
}

void soft_eleven_k(char *c)
{

    while(*c)
    {
        delay_us(100);
        SOFT_UART_FUNC(*(c++));
    }
}

void printStr(flash char *str)
{
    if(modem_TMEnabled)
        while(*str!='\0') putchar(*(str++));
    else
        while(*str!='\0') putchar1(*(str++));
}


#if EMAIL_AVAILABLE
#define ID_1 0x45	// character E
#define ID_2 0x45	// character E

bool test_smtp_code()
//Test the code returned from SMTP server or detect timeout or loss of connection (No Carrier)
//e.g. 	220 securemail.onebox.com ESMTP Server Ready
//		250 2.0.0 <unit@edacelectronics.com>
// 		354 Ready for data
//		221 2.0.0 securemail.onebox.com closing
{
	char smtp_code[3];
	
	timer=12;	//tests 6 and 7 = 10s, increased to 15s to attempt to avoid timing out leaving data mode
				//when email has been accepted and not saving date and time to hidden file
				//changed back to 9s because the idle SMTP server timeout is 10-11s
    timer_overflow = false;
	
	do
	{
		tickleRover();
		modem_read();
		//copy returned code
		strncpy(smtp_code,modem_rx_string,4);
		#ifdef SMTP_DEBUG
		sprintf(buffer,"[%s]\r\n",modem_rx_string);print1(buffer);
		#endif
	} while(!(strstrf(smtp_code,"0 ")||strstrf(smtp_code,"1 ")||strstrf(smtp_code,"2 ")||strstrf(smtp_code,"3 ")
		||strstrf(smtp_code,"4 ")||strstrf(smtp_code,"5 ")||strstrf(smtp_code,"6 ")||strstrf(smtp_code,"7 ")
		||strstrf(smtp_code,"8 ")||strstrf(smtp_code,"9 ")||strstrf(modem_rx_string,"ERROR")||strstrf(modem_rx_string,"NO CARRIER"))
		&& (!timer_overflow) && (!modem_timeOut));
	//SJL - 0 followed by a space is used to detect the final 250 reply,
	//all but the final reply start with 250-
	
	if(timer_overflow)
	{
		#ifdef SMTP_DEBUG
		sprintf(buffer,"Timer overflow [test_smtp_code] {%s}\r\n",modem_rx_string);
		print1(buffer);
		#endif
		return 0;
	}
	else if(modem_timeOut)
	{
		#ifdef SMTP_DEBUG
		sprintf(buffer,"Modem timeout [test_smtp_code] {%s}\r\n",modem_rx_string);
		print1(buffer);
		#endif
		return 0;
	}		
	else
	{
		//code detected ok
		return 1;
	}
}

bool test_end_data_mode()
//Test for the code returned from SMTP server after request to end data mode is sent
//tests for:	code 250 (message received ok)
//				or timeout or loss of connection (No Carrier)
{
	char smtp_code[3];

	timer=60;	//tests 6 and 7 = 10s, increased to 15s to attempt to avoid timing out leaving data mode
	//when email has been accepted and not saving date and time to hidden file
	//changed back to 9s because the idle SMTP server timeout is 10-11s
	timer_overflow = false;					
	do
	{
		tickleRover();
		modem_read();
		strncpy(smtp_code,modem_rx_string,4);
	} 
	while(!(strstrf(smtp_code,"250")||strstrf(modem_rx_string,"NO CARRIER"))
		&& (!timer_overflow) && (!modem_timeOut));
		
	if(strstrf(smtp_code,"250"))
	{
		#ifdef SMTP_DEBUG
		sprintf(buffer,"250 received [test_end_data_mode]\r\n");
		print1(buffer);
		#endif
		return 1;
	}	
	else if(timer_overflow)
	{
		#ifdef SMTP_DEBUG
		sprintf(buffer,"Timer overflow [test_end_data_mode]\r\n");
		print1(buffer);
		#endif
		#if ERROR_FILE_AVAILABLE	
		sprintf(buffer,"0501.");
		strcat(sms_dates,buffer);
		#ifdef ERROR_FILE_DEBUG
		print1(sms_dates);
		sprintf(buffer,"\r\n");
		print1(buffer);
		#endif
		#endif
		return 0;
	}
	else if(modem_timeOut)
	{
		#ifdef SMTP_DEBUG
		sprintf(buffer,"Modem timeout [test_end_data_mode]\r\n");
		print1(buffer);
		#endif
		#if ERROR_FILE_AVAILABLE	
		sprintf(buffer,"0502.");
		strcat(sms_dates,buffer);
		#ifdef ERROR_FILE_DEBUG
		print1(sms_dates);
		sprintf(buffer,"\r\n");
		print1(buffer);
		#endif
		#endif
		return 0;
	}
	else if(strstrf(modem_rx_string,"NO CARRIER"))
	{
		#ifdef SMTP_DEBUG
		sprintf(buffer,"NC received [test_end_data_mode]\r\n");
		print1(buffer);
		#endif
		#if ERROR_FILE_AVAILABLE	
		sprintf(buffer,"0503.");
		strcat(sms_dates,buffer);
		#ifdef ERROR_FILE_DEBUG
		print1(sms_dates);
		sprintf(buffer,"\r\n");
		print1(buffer);
		#endif
		#endif
		return 0;
	}
	return 0;
}

#define CMD_RES_OK		0
#define CMD_RES_ERROR		1
#define CMD_RES_TIMEOUT		2

static int wait_reply(int timeout)
{
	timer = timeout;
	timer_overflow = false;

	do {
		tickleRover();
		modem_read();
	} while (!(strstrf(modem_rx_string, "OK") ||
		   strstrf(modem_rx_string, "ERROR") ||
		   strstrf(modem_rx_string, "CONNECT") ||
		   strstrf(modem_rx_string, "NO CARRIER") ||
		   timer_overflow ||
		   modem_timeOut));

	if (timer_overflow || modem_timeOut) {
		DEBUG_printStr("wait_reply: timer_overflow\r\n");
		return CMD_RES_TIMEOUT;
	}

	if (strstrf(modem_rx_string, "ERROR")) {
		DEBUG_printStr("wait_reply: error\r\n");
		return CMD_RES_ERROR;
	}

	return CMD_RES_OK;
}

static int do_close(void)
{
	DEBUG_printStr("KTCP: close\r\n");
	sprintf(buffer, "AT+KTCPCLOSE=1,1\r\n");
	print0(buffer);

	if (wait_reply(3) == CMD_RES_TIMEOUT)
		return CMD_RES_TIMEOUT;

	DEBUG_printStr("KTCP: delete\r\n");
	sprintf(buffer, "AT+KTCPDEL=1\r\n");
	print0(buffer);

	if (wait_reply(3) == CMD_RES_TIMEOUT)
		return CMD_RES_TIMEOUT;

	return CMD_RES_OK;
}

bool open_socket()
//Opens a socket to either a DNS or SMTP server (POP not implemented)
{
	char cfgstr[40];

	DEBUG_printStr("KTCP: open_socket\r\n");
	do_close();

	strcpyre(cfgstr, config.apn);
	DEBUG_printStr("KTCP: CGDCONT\r\n");
	sprintf(buffer, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", cfgstr);
	print0(buffer);
	if (wait_reply(3))
		return false;

	DEBUG_printStr("KTCP: KCNXCFG\r\n");
	sprintf(buffer, "AT+KCNXCFG=1,\"GPRS\",\"%s\"\r\n", cfgstr);
	print0(buffer);
	if (wait_reply(3))
		return false;

	DEBUG_printStr("KTCP: KTCPCFG\r\n");
	strcpyre(cfgstr, config.smtp_serv);
	sprintf(buffer, "AT+KTCPCFG=1,0,\"%s\",%d\r\n",
		cfgstr, config.smtp_port);
	print0(buffer);
	if (wait_reply(3))
		return false;

	DEBUG_printStr("KTCP: KTCPSTART\r\n");
	sprintf(buffer, "AT+KTCPSTART=1\r\n");
	print0(buffer);
	if (wait_reply(10))
		return false;

	DEBUG_printStr("KTCP: connected\r\n");
	return true;
}

bool close_socket()
//Close a socket
{
	char retries=0;

	DEBUG_printStr("KTCP: close_socket\r\n");

	while (retries < 3) {
		tickleRover();
		delay_ms(1200);
		sequential_plus = 0;
		putchar('+');
		putchar('+');
		sequential_plus = 0;
		putchar('+');
		tickleRover();
		delay_ms(1200);

		if (wait_reply(5) == CMD_RES_OK)
		    break;

		retries++;
	}

	if (do_close() == CMD_RES_TIMEOUT) {
		DEBUG_printStr("Timeout on KTCP close, restarting");
		modem_restart();
		modem_init(SIERRA_MC8795V);
	}

	DEBUG_printStr("KTCP: disconnected\r\n");
	return true;
}

char *email_log_read_date(char *empty_date_time)
//Read and return the date and time from the sav.txt hidden file
{	
	FILE *fptr = NULL;
	char log_int;				//int representation of char read from file
	char log_char[1];			//char representation of log_int
	char date_time[19];			//date and time string
	char *date_time_string;		//pointer to the date and time string
	char count_char=0;			//count of number of characters read from file
	
	tickleRover();
	
	//need to receive a pointer to a string of the correct length (empty_date_time)
	//in order successfully send the time and date string back (pointer date_time_string)
	date_time_string = empty_date_time;
	
	if(!initialize_media())
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"Failed to initialize the SD card - hidden read\r\n");print1(buffer);
		#endif
        return 0;
    }
	
	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif
	
	//attempt to open the file for write
	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
	sedateRover();	
	fptr = fopenc("save.txt",READ);
	wakeRover();
	SREG |= 0x80;	
	
	if(fptr==NULL)	//file does not exists
	{
		//file does not exist - return 0 and get all log data
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[email_log_read_date] File not found\r\n");print1(buffer);
		#endif
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);
			pulse_instant_pause = 0;
		}
		#endif	
		
		return 0;
	}
	else
	{
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[email_log_read_date] File openned successfully for read\r\n");print1(buffer);
		#endif
		sprintf(date_time,"");	//ensure the date and time string is empty
		while(!feof(fptr))
		{
			SREG &= ~(0x80);	//guard file operation against interrupt
			log_int = fgetc(fptr);	//get char from file - in int format
			SREG |= 0x80;
			
			//write the received char to log_char
			sprintf(log_char,"%c",log_int);	
			
			//#ifdef SD_CARD_DEBUG
			//Print log file to UART 1
			//print1(log_char);
			//#endif
			
			//add the next character to the date + time string
			strncat(date_time,log_char,1);			
			count_char++;	//increment the number of characters received
			if(count_char==19)
				break;		//all characters received - exit
				//also guards against reading from corrupt file with large amount of data
				//i.e. will only read a maximum of 19 characters
		}		
		sprintf(date_time_string,"%s",date_time);	
		
		//#ifdef SD_CARD_DEBUG
		//sprintf(buffer,"The time and date read from the file is: ");print1(buffer);		
		//print1(date_time_string);
		//sprintf(buffer,"\r\n");print1(buffer);
		//#endif
		
		//close the file
		SREG &= ~(0x80);	//guard file operation against interrupt
		fclose(fptr);
		SREG |= 0x80;
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);
			pulse_instant_pause = 0;
		}
		#endif
		
		return date_time_string;	//return pointer to the date and time string
	}	
}

bool email_log_write_date()
//Write the date and time to the sav.txt hidden file
{
	//input a string with the date and time and save to hidden file
	FILE *fptr = NULL;
	char date_time[19];
	char *c;
	
	tickleRover();
	
	c = strchr(sms_newMsg.txt,'=')+1;	
	//c = strchr(dates,'=')+1;	
	sprintf(date_time,"%s\r\n",c);

	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif
	
	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
	sedateRover();														
	fptr = fopenc("save.txt",WRITE);
	wakeRover();
	SREG |= 0x80;
	
	
	if(fptr == NULL)
	{
		//#ifdef SD_CARD_DEBUG
		//sprintf(buffer,"[email_log] Failed to open save.txt, creating...\r\n");print1(buffer);
		//sprintf(buffer,"4 Failed to open save.txt, creating...\r\n");print1(buffer);
		//#endif
		SREG &= ~(0x80);	//guard file operation against interrupt
		fptr = fcreatec("save.txt",ATTR_HIDDEN);
		SREG |= 0x80;
		tickleRover();
	}
	if(fptr == NULL)
	{	
		//#ifdef SD_CARD_DEBUG
		//sprintf(buffer,"[email_log] Failed to create save.txt\r\n");print1(buffer);
		//sprintf(buffer,"5 Failed to create save.txt\r\n");print1(buffer);
		//#endif
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);
			pulse_instant_pause = 0;
		}
		#endif		
		
		return 0;
	}
	
	tickleRover();
	SREG &= ~(0x80);	//guard file operation against interrupt
	fprintf(fptr,"%s\r\n",date_time);
	SREG |= 0x80;
	
	tickleRover();
	SREG &= ~(0x80);	//guard file operation against interrupt
	fclose(fptr);
	SREG |= 0x80;
	
	#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
	{
		delay_ms(350);
		pulse_instant_pause = 0;
	}
	#endif
	
	return 1;
}

bool quit_smtp_server()
//Quit SMTP server
//sent the quit message to the SMTP server and process the response
{
	sprintf(buffer,"quit\r\n");print0(buffer);
	
	if(!test_smtp_code())
	{
		#ifdef SMTP_DEBUG
		sprintf(buffer,"[quit_smtp_server] Timed out waiting for response to quit\r\n");print1(buffer);
		#endif
	}
	

	if(strstrf(modem_rx_string,"221 "))
	{		
		timer=15;
		timer_overflow = false;
		do
		{
			tickleRover();
			modem_read();
		} while(!(strstrf(modem_rx_string,"NO CARRIER")) && !timer_overflow && !modem_timeOut);

		do_close();
		
		if(timer_overflow)
		{
			#ifdef SMTP_DEBUG
			sprintf(buffer,"[quit_smtp_server] Timer overflow before receiving NO CARRIER [%s]\r\n",modem_rx_string);print1(buffer);
			#endif
			return 0;
		}
		else if(modem_timeOut)
		{
			#ifdef SMTP_DEBUG
			sprintf(buffer,"[quit_smtp_server] Modem timeout before receiving NO CARRIER [%s]\r\n",modem_rx_string);print1(buffer);
			#endif
			return 0;
		}
		else
		{
			#ifdef SMTP_DEBUG
			sprintf(buffer,"[quit_smtp_server] Successfully disconnected from SMTP server\r\n");print1(buffer);
			#endif
			return 1;
		}
	}
	else
	{
		#ifdef SMTP_DEBUG
		sprintf(buffer,"SMTP ERROR: Server failed to acknowledge quit message [%s]\r\n",modem_rx_string);print1(buffer);
		#endif
		close_socket();
		return 0;
	}
}

#endif
