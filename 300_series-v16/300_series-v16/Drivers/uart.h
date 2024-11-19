#ifndef _UART_H_
#define _UART_H_


#include <mega128.h>
#include "global.h"

#if (HW_REV == V2)
    #define UART0_CTS PINE.6
    #define UART0_CTS_DDR DDRE.6
    #define UART0_RTS PORTE.5
    #define UART0_RTS_DDR DDRE.5

#elif (HW_REV == V5)

    #define UART0_CTS PINE.2
    #define UART0_CTS_DDR DDRE.2
    #define UART0_RTS PORTE.3
    #define UART0_RTS_DDR DDRE.3

    #define UART1_CTS PIND.4
    #define UART1_CTS_DDR DDRD.4
    #define UART1_RTS PORTD.5
    #define UART1_RTS_DDR DDRD.5
#elif HW_REV == V6
    #define UART0_CTS PINE.2
    #define UART0_CTS_DDR DDRE.2
    #define UART0_RTS PORTE.3
    #define UART0_RTS_DDR DDRE.3

    #define UART1_CTS PIND.4
    #define UART1_CTS_DDR DDRD.4
    #define UART1_RTS PORTD.5
    #define UART1_RTS_DDR DDRD.5

#endif


#define RXB8 1
#define TXB8 0
#define UPE 2
#define OVR 3
#define FE 4
#define UDRE 5
#define RXC 7

#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR (1<<UPE)
#define DATA_OVERRUN (1<<OVR)
#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE (1<<RXC)



#define RX_BUFFER_SIZE1 128
#define RX_BUFFER_SIZE0 128

//Define all of the tasks that are part of the string handler for the uart
enum uart_Tasks {UART_CHECK_STR,
                   UART_SET_BAUD, UART_SETUP, UART_READ_INPUT, UART_READ_OUTPUT,
                   UART_SERIAL, UART_SERIAL_LOAD, UART_GET_TIME, UART_SET_TIME,
                   UART_FACTORY_RESET, UART_RESET,EDAC_QUEREY, UART_CNI, UART_RN,
                   UART_SWITCH_OUTPUT, UART_SWITCH_OUTPUTALL, UART_SET_CMGS,
                   UART_EMAILCMD, UART_RECMAIL};


typedef eeprom struct uartStruct {
    unsigned int baud;
    unsigned int data_bits;
    char parity;
    unsigned int stop_bits;
} uart_t;

bool uart0_init(unsigned long baud);
bool uart1_init(eeprom uart_t *setup);
interrupt [USART0_RXC] void usart0_rx_isr(void);
char getchar(void);
interrupt [USART0_TXC] void usart0_tx_isr(void);
void putchar(char c);
interrupt [USART1_RXC] void usart1_rx_isr(void);
char getchar1(void);
interrupt [USART1_DRE] void usart1_tx_isr(void);
void putchar1(char c);
void print(char *str);
void print0(char *str);
void print1(char *str);
void print0_eeprom(eeprom char *p);
void print1_eeprom(eeprom char *p);
void print_eeprom(eeprom char *p);
void print_char(char c);

#if EMAIL_AVAILABLE
// SJL - MC8795V 320/1 NG Email function
bool test_smtp_code();
bool test_end_data_mode();
char *get_IP_address();	//(char *url);
bool open_socket();
bool close_socket();
bool email_log_write_date();
char *email_log_read_date(char *empty_date_time);
bool quit_smtp_server();
#endif

bool uart_readPort(void);
bool uart_readPortNoEcho(void);
bool uart_readConfigSegment(void);
bool uart_handleNew(char *cmd);
void printStr(flash char *str);
void disable_uart0();
void reenable_uart0();
void soft_eleven_k(char *c);
//void uart_get_string();
//void uart_task_handler(void);
//void modem_get_string();
//void modem_task_handler(void);
//void modem_flush_buffer(void);

// USART1 Transmitter buffer
#if  (PRODUCT == EDAC320 || PRODUCT == EDAC321) && MODEM_TYPE == Q24NG_PLUS
#define TX_BUFFER_SIZE1 16
#else
#define TX_BUFFER_SIZE1 128
#endif


/**********************************************************************************************************/
/* UART Global Variables */
// SJL - CAVR2

extern eeprom uart_t uart1_setup;

extern char tx_buffer0[];       // USART0 Transmitter buffer
#if TX_BUFFER_SIZE0<256
extern unsigned char tx_wr_index0,tx_rd_index0,tx_counter0;
#else
extern unsigned int tx_wr_index0,tx_rd_index0,tx_counter0;
#endif

extern char tx_buffer1[];
#if TX_BUFFER_SIZE1<256
extern volatile unsigned char tx_wr_index1,tx_rd_index1,tx_counter1;
#else
extern volatile unsigned int tx_wr_index1,tx_rd_index1,tx_counter1;
#endif

extern bit uart0_held;

// Modem Setup - from global.c
extern volatile unsigned int rx_wr_index0,rx_rd_index0,rx_counter0;
extern char modem_rssi;
extern char modem_ber;
extern eeprom bool uart_echo;
extern bit uart_caps;
extern bit uart0_flowPaused;
extern bit uart1_flowPaused;
extern char modem_state;
extern unsigned char rx_write;
extern char uart_rx_string[]; //uart software buffer	//SJL - CAVR2 - variable not used anywhere but in uart.c - initialized in uart.c - commented out in global

//UART 1 Setup
extern volatile unsigned int rx_wr_index1,rx_rd_index1,rx_counter1;
extern bit uart_paused; //can't write to the SW buffer

extern volatile unsigned char modem_read_ready;
extern volatile unsigned char uart_read_ready;

extern char *command_string;	//commented out of event.c
extern char no_carrier_count;	//moved from uart.c
extern char rx_buffer0[];

/**********************************************************************************************************/
#endif
