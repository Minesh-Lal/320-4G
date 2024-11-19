#ifndef _GLOBAL_
#define _GLOBAL_

/* Header file to include the datatypes */
#include <mega128.h>

#define true 1
#define false 0

//Build type
#define SMS300 0
#define EDAC310 1
#define EDAC320 2
#define EDAC321 3
#define SMS300Lite 4
#define HERACLES 5
#define EDAC315 HERACLES
#define DEVELOPMENT 255


#define Q2438F 1
#define Q2438_NEW 2
#define Q24NG_CLASSIC 3
#define Q24NG_PLUS 4
#define Q2406B 5
#define SIERRA_MC8775V 6
#define SIERRA_MC8795V 7

#define SIX_MEG 0
#define SEVEN_SOMETHING_MEG 1

#define V2 2
#define V5 5
#define V6 6

#define V002_0002    2
#define V002_0005    5
#define VO02_0029_VA 6
#define V002_0029_V1 6

//#define _DEBUG_ALL_
//PRODUCT defines functionality
#define PRODUCT EDAC321	          //[SMS300,EDAC310,EDAC315,EDAC320,EDAC321]
//#define BETA
//MODEM_TYPE defines... modem type
#define MODEM_TYPE   SIERRA_MC8795V //[Q2438F,Q24NG_PLUS,Q2406B,SIERRA_MC8775V,SIERRA_MC8795V]
//Hardware version  (v2, v5 or ConnectOne board)
#define HW_REV 6        //[V002_0002,V002_0005,V002_0029_VA,V002_0029_V1]
#define CLOCK SIX_MEG              //[SIX_MEG (6MHZ),SEVEN_SOMETHING_MEG (7.3728)]

//future development for third UART that resides on i2c
#define EXTERNAL_UART false
//time out messages if they're 30 minutes late
#define MESSAGE_EXPIRY true


#if MODEM_TYPE == Q2406B || MODEM_TYPE == SIERRA_MC8795V
#define SMTP_CLIENT_AVAILABLE true
#define SMTP_AUTH_AVAILABLE true
#define POP3_CLIENT_AVAILABLE true			//SJL - This is required for successful config DL using config mngr 3.2.1
#else
#define SMTP_CLIENT_AVAILABLE false
#define SMTP_AUTH_AVAILABLE false
#define POP3_CLIENT_AVAILABLE false
#endif

#define CONNECT_ONE_AVAILABLE false

#if (PRODUCT==SMS300)
    #define VERSION_STRING "3.00"
    #define SMS_AVAILABLE true
    #define TCPIP_AVAILABLE false			//not used but required for config manager?
    #define EMAIL_AVAILABLE false
    #define LOGGING_AVAILABLE false
    #define SYSTEM_LOGGING_ENABLED false
    #define PULSE_COUNTING_AVAILABLE false
    #define GPS_AVAILABLE false	//true
    #define MAX_INPUTS 8
    #define MAX_OUTPUTS 4
	#define ERROR_FILE_AVAILABLE false
#elif (PRODUCT==SMS300Lite)
    #define VERSION_STRING "3.00L"
    #define SMS_AVAILABLE true
    #define TCPIP_AVAILABLE false
    #define EMAIL_AVAILABLE false
    #define LOGGING_AVAILABLE false
    #define SYSTEM_LOGGING_ENABLED false
    #define PULSE_COUNTING_AVAILABLE false
    #define GPS_AVAILABLE false	//true
    #define MAX_INPUTS 4
    #define MAX_OUTPUTS 4
	#define ERROR_FILE_AVAILABLE false
#elif (PRODUCT==EDAC310)
    #define VERSION_STRING "3.10"
    #define SMS_AVAILABLE true
    #define TCPIP_AVAILABLE true
    #define EMAIL_AVAILABLE true
    #define LOGGING_AVAILABLE false
    #define SYSTEM_LOGGING_ENABLED false
    #define PULSE_COUNTING_AVAILABLE false
    #define GPS_AVAILABLE false	//true
    #define MAX_INPUTS 8
    #define MAX_OUTPUTS 4
	#define ERROR_FILE_AVAILABLE false
#elif (PRODUCT==EDAC320)
    #define VERSION_STRING "3.20"
    #define SMS_AVAILABLE true
    #define TCPIP_AVAILABLE true	//true
    #define EMAIL_AVAILABLE true
    #define LOGGING_AVAILABLE true
    #define SYSTEM_LOGGING_ENABLED true
    #define PULSE_COUNTING_AVAILABLE false
    #define GPS_AVAILABLE false	//true
    #define MAX_INPUTS 8
    #define MAX_OUTPUTS 4
	#define ERROR_FILE_AVAILABLE true
#elif (PRODUCT==EDAC321)
    #define VERSION_STRING "3.21"
    #define SMS_AVAILABLE true
    #define TCPIP_AVAILABLE true	//true
    #define EMAIL_AVAILABLE true
    #define LOGGING_AVAILABLE true
    #define SYSTEM_LOGGING_ENABLED true
    #define PULSE_COUNTING_AVAILABLE true
    #define GPS_AVAILABLE false	//true
    #define MAX_INPUTS 8
    #define MAX_OUTPUTS 4
	#define ERROR_FILE_AVAILABLE true
#elif (PRODUCT==DEVELOPMENT)
    #define VERSION_STRING "DEV"
    #define SMS_AVAILABLE true
    #define TCPIP_AVAILABLE true
    #define EMAIL_AVAILABLE true
    #define LOGGING_AVAILABLE false
    #define SYSTEM_LOGGING_ENABLED false
    #define PULSE_COUNTING_AVAILABLE true
    #define GPS_AVAILABLE false	//true
    #define MAX_INPUTS 8
    #define MAX_OUTPUTS 4
	#define ERROR_FILE_AVAILABLE false
#elif (PRODUCT==HERACLES)
    #define VERSION_STRING "Heracles"
    #define SMS_AVAILABLE true
    #define TCPIP_AVAILABLE false
    #define EMAIL_AVAILABLE false
    #define LOGGING_AVAILABLE true
    #define SYSTEM_LOGGING_ENABLED true
    #define PULSE_COUNTING_AVAILABLE false
    #define GPS_AVAILABLE false	//true
    #define MAX_INPUTS 8
    #define MAX_OUTPUTS 4
	#define ERROR_FILE_AVAILABLE false
#endif


#if TCPIP_AVAILABLE == true
    #if !(MODEM_TYPE == Q2406B || MODEM_TYPE == Q24NG_PLUS || HW_REV>5)
    #error "TCP/IP not available on that modem. Please check #define of MODEM_TYPE"
    #endif
#endif

#if (EMAIL_AVAILABLE == true) && !(TCPIP_AVAILABLE == true)
    #error "Email requires a TCP/IP connection. Please check #define of EMAIL_AVAILABLE"
#endif

#if (LOGGING_AVAILABLE == true) && (HW_REV<5)
    #error "Logging requires a version 5+ hardware platform"
#endif

#if (SYSTEM_LOGGING_ENABLED == true) && !(LOGGING_AVAILABLE==true)
    #error "System Logging requires a unit with logging enabled"
#endif

#if (PULSE_COUNTING_AVAILABLE == true) && (HW_REV<5)
    #error "Pulse Counting requires a version 5+ hardware platform"
#endif

#if MODEM_TYPE == Q24NG_PLUS && HW_REV < 6
#if SMTP_CLIENT_AVAILABLE
#error "No SMTP client available on this modem sorry."
#endif
#if POP3_CLIENT_AVAILABLE
#error "No POP3 client available on this modem sorry."
#endif
#endif


#ifdef _DEBUG_ALL_
//#define _MODEM_DEBUG_
//#define _DEBUG_
//#define _ALARM_DEBUG_
//#define _UART_DEBUG_
//#define _INPUT_DEBUG_
//#define _SMS_DEBUG_
//#define _OUTPUT_DEBUG_
//#define _CONFIG_DEBUG_	//SJL - always disabled
//#define _CONFIG_DEBUG2_ 	//SJL - always disabled
//#define EVENT_DEBUG
//#define INT_TIMING 		//SJL - always disabled
//#define DEBUG

#define EMAIL_DEBUG
#ifdef EMAIL_DEBUG
	#define SD_CARD_DEBUG
	#define SOCKET_DEBUG
	#define DNS_DEBUG
	#define SMTP_DEBUG
	#define ERROR_FILE_DEBUG
#endif

//#define ADMIN_DEBUG
//#define PULSE_DEBUG
#endif

#ifdef INT_TIMING
#define start_int()  //PORTC.0 = 1;
#define start_x_int() PORTC.0 = 1;
#define stop_int() PORTC.0 = 0;
#endif

//#define _PULSE_COUNTING_DEVEL_

//#define DEBUG(i) sprintf(buff, "%s",#i);print(buff)
//#define DEBUG_VAR(i) sprintf(buff, "%d",#i);print(buff)

#define MAGIC_NUM 14

enum{
    NO_CONNECTION,
    CELL_CONNECTION,
    GPRS_CONNECTION,
    CSD_CALL
};

#define NONE 0

#define DEBOUNCE_OVERFLOW 82399

#define INPUT_MAX_ALARMS 2
#define MAX_CONTACTS 16

//#define unsigned char unsigned char
//#define unsigned int unsigned int
#define bool unsigned char
//#define unsigned long unsigned long

#define cbi(sfr, bit_num) ((sfr) &= ~(bit_num))
#define sbi(sfr, bit_num) ((sfr) |= (bit_num))
#define tbi(sfr, bit_num) ((sfr) ^= (bit_num))

#define global_interrupt_off()  #asm("cli")
                              //  PORTB.6 = 0;
#define global_interrupt_on()   #asm("sei")
//                                PORTB.6 = 1;



#define OKAY 1
#define ERROR 0

#define on 1
#define off 0

//change all led switching to be done via interupt

#if (HW_REV == V2)
    #define status_led_on(void) cbi(PORTC, 0x04)
    #define status_led_off(void) sbi(PORTC, 0x04)
#elif (HW_REV == V6 || HW_REV == V5)
    #define status_led_on(void) cbi(PORTD, 0x80)
    #define status_led_off(void) sbi(PORTD, 0x80)
#endif

extern char led_state;
#define power_led_on(void)   (led_state=LED_POWER_ON)
#define power_led_off(void)  (led_state=LED_POWER_OFF)
#define power_led_flashing() (led_state=LED_POWER_FLASH)
#define active_led_on(void)  sbi(PORTB, 0x40)
#define active_led_off(void) cbi(PORTB, 0x40)


#if (HW_REV == V2)
    #define M_RST PORTE.3
#elif (HW_REV == V5)
    #if MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE==SIERRA_MC8795V
        #define M_RST PORTE.4
    #else
        #define M_RST PORTD.6
    #endif
#elif (HW_REV == V6)
    #define M_RST PORTD.6
#endif


#define LED_POWER_FLASH     0x04
#define LED_POWER_ON        0x02
#define LED_POWER_OFF       0x01


#define STR_START 0
#define MAX_MSG_LEN 40
#define MAX_PHONE_LEN 25
#define MAX_PIN_LEN 4
#define MAX_USER_LEN 10
#define MAX_NAME_LEN 20

#define MSG_COUNT 16						//SJL - added +1 for MSG_NO_CARRIER
#define MSG_OK    0x0001					// 0000 0000 0000 0001
#define MSG_ERROR 0x0002					// 0000 0000 0000 0010
#define MSG_Ok_WaitingForData 0x0004		// 0000 0000 0000 0100
#define MSG_WIND 0x0008						// 0000 0000 0000 1000
#define MSG_CGREG_1 0x0010					// 0000 0000 0001 0000
#define MSG_CGREG_5 0x0020					// 0000 0000 0010 0000
#define MSG_Ok_InfoGprsActivation 0x0040	// 0000 0000 0100 0000
#define MSG_CREG 0x0080						// 0000 0000 1000 0000
#define MSG_CSQ 0x0100						// 0000 0001 0000 0000
#define MSG_STATE 0x0200					// 0000 0010 0000 0000
#define MSG_CDS 0x0400						// 0000 0100 0000 0000
#define MSG_CMGR 0x0800						// 0000 1000 0000 0000
#define MSG_Ok_Info 0x1000					// 0001 0000 0000 0000
#define MSG_CPIN 0x2000						// 0010 0000 0000 0000
//#define MSG_PEERCLOSE 0x4000				// 0100 0000 0000 0000
#define MSG_GETBAND 0x4000				// changed for MC8795V
#define MSG_NO_CARRIER 0x8000				// 1000 0000 0000 0000	SJL - added for MC8795V

extern const char *RETURNED_MESSAGES[];

//Clear the watchdog timer
#define tickleRover() #asm("wdr")
extern eeprom int sector_read_count;
extern eeprom int sector_write_count;
extern char modem_state;



#define UART_SWBUFFER_LEN 128
#define MODEM_SWBUFFER_LEN 255

	#define uchar	unsigned char
	#define uint	unsigned int
	#define ulong	unsigned long
	#define schar	char
	#define sint	int
	#define slong	long

#define LOG_NONE      0x00
#define LOG_INSTANT   0x01
#define LOG_AVERAGE   0x02
#define LOG_MIN       0x04
#define LOG_MAX       0x08
#define LOG_AGGREGATE 0x10
#define LOG_DUTY      0x20
#define LOG_ALARM     0x40

#define TX_BUFFER_SIZE0 16

void increment_chksum(char *c,char *chksum);
void increment_chksume(eeprom char *c, char *chksum);
void system_reset();
//const char *RETURNED_MESSAGES(unsigned int i);

#if (HW_REV == V6 || HW_REV == V5)
#if MODEM_TYPE == SIERRA_MC8775V || MODEM_TYPE==SIERRA_MC8795V
    #define csd_connected 1
#else
    #define csd_connected (!(PIND.7))
#endif
#endif

/* GLOBALS */
/**********************************************************************************************************/
/* PROTOTYPE HEADRERS */
void wakeRover(void);                           //SJL - CAVR2 - from main.c - function in main.c
void sedateRover(void);                         //SJL - CAVR2 - from main.c - function in main.c
void startup_config_reset(void);                //SJL - CAVR2 - from main.c - function in main.c
void port_init(void);							//SJL - CAVR2 - from main.c - function in main.c

/**********************************************************************************************************/
/* VARIABLES */

/* Only used in modem.c - doesn't need to be in globals */
volatile extern unsigned char modem_timeOut;             //SJL - CAVR2 - volatile added to match initialization in .c
volatile extern unsigned int modem_timerCount;  //SJL - CAVR2 - volatile added to match initialization in .c
extern eeprom unsigned char build_type;
extern eeprom unsigned int modem_baud;        //SJL - CAVR2 - moved from main.c
//extern eeprom unsigned char startup_first;      //SJL - CAVR2 - moved from main.c - unused
extern bit long_time_up;				//SJL - CAVR2 - moved from main.c
extern eeprom bool config_cmgs;			//SJL - CAVR2 - moved from main.c
extern unsigned char startupError;				//SJL - CAVR2 - moved from main.c
extern eeprom char verbose;				//SJL - CAVR2 - moved from main.c
extern eeprom bool email_recMail;		//SJL - CAVR2 - moved from main.c
extern flash char *code_version;		//SJL - CAVR2 - moved from main.c
//extern eeprom char gprs_ipAddress[];	//SJL - CAVR2 - moved from global.c
extern eeprom char contact_area[];		//SJL - CAVR2 - moved from global.c
extern unsigned int last_duty_high[];	//SJL - CAVR2 - moved from main.c
extern unsigned int last_duty_count[];  //SJL - CAVR2 - moved from main.c

extern char modem_rx_string[MODEM_SWBUFFER_LEN];//SJL - CAVR2 - moved from global.c

//extern bool startup_send_status;
//extern char startup_send_delay;
extern bit input_int_sms;
//extern eeprom char *contact_one_eeprom;
//extern char char1_temp[2], char2_temp[1];

/* UART Globals */
extern char buffer[160];                        //buffer for the string to be sent, used in sprintf
                                                //SJL - CAVR2 - moved from global.c
//extern bool uart0_flowPaused;                   //SJL - CAVR2 - from global.c - not previously defined seperately
//extern bool uart1_flowPaused;                   //SJL - CAVR2 - from global.c - not previously defined seperately

/* Modem Globals */
extern unsigned char modem_register;                    //SJL - CAVR2 - from global.c - not previously defined seperately
extern bit modem_TMEcho;                       //SJL - CAVR2 - from global.c - not previously defined seperately
extern bit modem_TMEnabled;                    //SJL - CAVR2 - from global.c - not previously defined seperately
extern unsigned char m_rx_write;                        //SJL - CAVR2 - from global.c - not previously defined seperately
extern bit modem_paused;                       //SJL - CAVR2 - from global.c - not previously defined seperately
extern bit log_event_on_queue;                 //SJL - CAVR2 - from global.c - not previously defined seperately
extern bit email_log_event_on_queue;
extern bit mmc_init_event_on_queue;            //SJL - CAVR2 - from global.c - not previously defined seperately
extern bit check_status_event_on_queue;

extern eeprom int email_retry_period;
extern int email_retry_timer;

/**********************************************************************************************************/
#endif
