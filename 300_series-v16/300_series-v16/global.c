/*
    Global variables for the SMS300 Project
*/

#include <delay.h>
#include <stdio.h>

#include "global.h"
#include "drivers\uart.h"
#include "drivers\queue.h"
#include "drivers\sms.h"
#include "drivers\config.h"
#include "drivers\error.h"
#include "drivers\input.h"
#include "drivers\output.h"
#include "drivers\contact.h"
#include "drivers\modem.h"
#include "drivers\ds1337.h"
#include "drivers\mmc.h"

/*  IMPORTANT GLOBAL VARIABLES */
//Save the code version.  This can't be changed
flash char *code_version = VERSION_STRING;
//Bit used to decide if this is the first power-up of the device
//eeprom unsigned char startup_first=0;	//unused
unsigned int last_duty_high[MAX_INPUTS];
unsigned int last_duty_count[MAX_INPUTS];


eeprom unsigned char build_type = NONE;
eeprom bool config_cmgs = true;
unsigned char startupError=0;                          //Starting up the modem threw an error, don't use the modem

/* UART Globals */
//eeprom uart_t uart1_setup={9600,8,'n',1};       //Default setup for the external comm port
char buffer[160]={0};                             //buffer for the string to be sent, used in sprintf
//bool uart_paused;                       //can't write to the SW buffer
//char uart_rx_string[UART_SWBUFFER_LEN];                       //uart software buffer
//unsigned char rx_write; //index to write into the buffer //uart.h
/* Uart setup*/
//volatile unsigned int rx_wr_index1,rx_rd_index1,rx_counter1; //uart.h
//eeprom bool uart_echo = true; //uart.h
//bool uart_caps = true; //uart.h
//bool uart0_flowPaused = false; //uart.h
//bool uart1_flowPaused = false; //uart.h
//char modem_state = NO_CONNECTION; //uart.h

/* Modem Global Variables */
#if HW_REV == V6
eeprom unsigned int modem_baud = 9600;                //Baud rate of the modem
#else
eeprom unsigned int modem_baud = 9600;                //Baud rate of the modem
#endif
char modem_rx_string[MODEM_SWBUFFER_LEN];       //Software buffer for modem
unsigned char m_rx_write=0;                             //index to write into the buffer
unsigned char modem_sendCount=0;                        //number of messages sent from modem
bit modem_paused = false;                      //the modem buffer can't be written to
volatile unsigned int modem_timerCount=0;             //time out counter, used in timer0 isr
volatile unsigned char modem_timeOut=0;                 //flag to indicate that modem has timed out
bool modem_smsListActive = false;
bit modem_TMEnabled=false;                     //modem is in through mode
bit long_time_up=false;
bit modem_TMEcho=false;                        //through mode echo's to external serial port
unsigned char modem_register = false;
/* Modem Setup */
//volatile unsigned int rx_wr_index0,rx_rd_index0,rx_counter0; //uart.h
//char modem_rssi; //uart.h
//char modem_ber; //uart.h
bool email_getMail = false;
eeprom bool email_recMail = false;
bit log_event_on_queue = false;
bit email_log_event_on_queue = false;
bit mmc_init_event_on_queue = false;
bit check_status_event_on_queue = false;

eeprom int email_retry_period = -1;
int email_retry_timer = -1;

/* Queue's */
//volatile queue_t q_modem;                         //queue of sms's to be sent
//volatile queue_t q_event;                       //queue of events to be handled

/* SMS Globals */ //moved to sms.h and .c
//sms_t sms_newMsg;                               //structure storing the message to be sent or rec
//unsigned char msg_index;                                //index the contact list is up to
//unsigned char sms_charCount;                            //
//unsigned char sms_delIndex;                             //sms that is to be deleted
//bool sms_header = false;                        //flag set if you want to add sitename and time to sms
////bool sms_send_active = false;                 //the modem is currently in the middle of a send
//unsigned char msg_send = false;

/* Input Variables */
//volatile input_alarm_t input[MAX_INPUTS];	    //structure of the variables for the alarm 	//input.h
//volatile unsigned char channel;				        //The currentZone channel you are reading 	//input.h
//volatile unsigned char alarmCount;                    //which alarm channel is being checked      //input.h

/* Config Globals */
//bool config_continue;                           //flag to indicate config is continueing
//bool config_break;                              //a break character was found
#if GPS_AVAILABLE
    float latitude, longitude;
#endif

/* Configuration stored data */
eeprom config_t config = {
                            //Phone list
                            //{"","","","","","","","","","","","","","","","",""},
                            //auto reporting settings
                            {0,0,0,0,NO_ALARM},
                            //General Parmameters
                            //0,"SiteName",0,"1234",true,true,false,"",59,59,true,false,true,false,true,
							0,"SiteName",0,"1234",true,true,false,"",-1,-1,true,false,true,false,true,
#if SMTP_CLIENT_AVAILABLE
                            25,"smtp2.vodafone.net.nz",
#if SMTP_AUTH_AVAILABLE
                            "","",
#endif
                            "edacelectronics.com",
#endif
#if SMTP_CLIENT_AVAILABLE || POP3_CLIENT_AVAILABLE
                            "unit@edacelectronics.com",
#endif
#if POP3_CLIENT_AVAILABLE
                            110,"","","",
#endif
#if MODEM_TYPE == Q24NG_PLUS || MODEM_TYPE == SIERRA_MC8795V
                            "8.8.8.8",	//Default dns0 and dns1 set to google dns servers for comp with all carriers
                            "8.8.4.4",	//VF NZ - old defaults: "202.73.198.15" and "202.73.198.15"
                            "www.vodafone.net.nz",
#endif
                            {//info for pulse inputs
                              {1,SECONDS},{1,SECONDS}
                            },
                            {   //INPUTS

                                {  //Input 1
                                    DISABLE,DIGITAL,LOG_NONE,"V",0.0,5.0,1.0,1.0,true,{{ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 1 Alarm A","Input 1 Reset A",0x00,0x00,0, true,{off,0x00,0,on},{off,0x00,0,on}},
                                        {ALARM_NONE,ALARM_RESET,100,0,0,0,"Input 1 Alarm B","Input 1 Reset B",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}}},
                                        {0,0},sms_RESET_A
                                }
#if MAX_INPUTS > 1
                                ,{  //Input 2
                                    DISABLE,DIGITAL,LOG_NONE,"V",0,5,1.0,1.0,true,{{ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 2 Alarm A","Input 2 Reset A",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}},
                                        {ALARM_NONE,ALARM_RESET,100,0,0,0,"Input 2 Alarm B","Input 2 Reset B",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}}},
                                        {0,0},sms_RESET_A
                                }
#if MAX_INPUTS > 2
                                ,{  //Input 3
                                    DISABLE,DIGITAL,LOG_NONE,"V",0.0,5.0,1.0,1.0,true,{{ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 3 Alarm A","Input 3 Reset A",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}},
                                        {ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 3 Alarm B","Input 3 Reset B",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}}},
                                        {0,0},sms_RESET_A
                                }
#if MAX_INPUTS > 3
                                ,{  //Input 4
                                    DISABLE,DIGITAL,LOG_NONE,"V",0.0,5.0,1.0,1.0,true,{{ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 4 Alarm A","Input 4 Reset A",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}},
                                        {ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 4 Alarm B","Input 4 Reset B",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}}},
                                        {0,0},sms_RESET_A
                                }
#if MAX_INPUTS > 4
                                ,{  //Input 5
                                    DISABLE,DIGITAL,LOG_NONE,"V",0.0,5.0,1.0,1.0,true,{{ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 5 Alarm A","Input 5 Reset A",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}},
                                        {ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 5 Alarm B","Input 5 Reset B",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}}},
                                        {0,0},sms_RESET_A
                                }
#if MAX_INPUTS > 5
                                ,{  //Input 6
                                    DISABLE,DIGITAL,LOG_NONE,"V",0.0,5.0,1.0,1.0,true,{{ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 6 Alarm A","Input 6 Reset A",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}},
                                        {ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 6 Alarm B","Input 6 Reset B",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}}},
                                        {0,0},sms_RESET_A
                                }
#if MAX_INPUTS > 6
                                ,{  //Input 7
                                    DISABLE,DIGITAL,LOG_NONE,"V",0.0,5.0,1.0,1.0,true,{{ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 7 Alarm A","Input 7 Reset A",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}},
                                        {ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 7 Alarm B","Input 7 Reset B",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}}},
                                        {0,0},sms_RESET_A
                                }
#if MAX_INPUTS > 7
                                ,{  //Input 8
                                    DISABLE,DIGITAL,LOG_NONE,"V",0.0,5.0,1.0,1.0,true,{{ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 8 Alarm A","Input 8 Reset A",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}},
                                        {ALARM_NONE,ALARM_RESET,5,0,0,0,"Input 8 Alarm B","Input 8 Reset B",0x00,0x00,0, false,{off,0x00,0,on},{off,0x00,0,on}}},
                                        {0,0},sms_RESET_A
                                }
#endif
#endif
#endif
#endif
#endif
#endif
#endif
                            } ,
                            { //Outputs
                                {DISABLE, 0, OFF, {"Output 1",OFF, "Output 1 On","Output 1 Off",0x00,0x00}}
#if MAX_OUTPUTS > 1
                                ,{DISABLE, 0, ON, {"Output 2",OFF, "Output 2 On","Output 2 Off",0x00,0x00}}
#if MAX_OUTPUTS > 2
                                ,{DISABLE, 0, LAST_KNOWN, {"Output 3",OFF, "Output 3 On","Output 3 Off",0x00,0x00}}
#if MAX_OUTPUTS > 3
                                ,{DISABLE, 0, OFF, {"Output 4",OFF, "Output 4 On","Output 4 Off",0x00,0x00}}
#endif
#endif
#endif
                            }
                        };

eeprom char contact_area[CONTACT_AREA_SIZE]="\0";
eeprom char verbose = 1;

/* Error handling stored data */
eeprom error_t error = {0,                  //q_event_ovf
                            0,
                            {0,0,0,0,0,0},   //Date
                            0,              //Power on reset counter
                            0,              //External reset counter
                            0,              //brownout reset counter
                            0,              //watchdog reset counter
                            0,              //jtag reset counter
                            0,              //unknown reset counter
                            0,              //sms retry counter
                            0,              //sms fail counter
                            0,              //modem error command returned counter
                            0,              //through mode usage counter
                            0              //sms send counter
                        };

//Phone number of vodafoneNZ that send you prepay warnings
eeprom char *VODAFONE = "Vodafon";

//eeprom char gprs_ipAddress[20]="203.97.70.151";
//eeprom char gprs_enabled = true;
//eeprom bool email_enabled = true; //never used




void increment_chksum(char *c,char *chksum)
{
	//sprintf(buffer,"c = %s\r\n",c);print(buffer);	//SJL - CAVR2 - debug
    while(*c != 0)
    {
    	if(*c != '\r' && *c != '\n')//{
        	//sprintf(buffer,"old *chksum = %X\r\n",*chksum);print(buffer);	//SJL - CAVR2 - debug
            (*chksum)=(*chksum)+*(c++);
            //sprintf(buffer,"c = %s\r\n",c);print(buffer);	//SJL - CAVR2 - debug
            //sprintf(buffer,"new *chksum = %X\r\n",*chksum);print(buffer);}	//SJL - CAVR2 - debug
        else
            c++;
    }
}

void increment_chksume(eeprom char *c, char *chksum)
{
	char debug_buffer[50];										//SJL - CAVR2 - debug
    //sprintf(debug_buffer,"c = ");print(debug_buffer); 			//SJL - CAVR2 - debug
    //print_eeprom(c);
    //sprintf(debug_buffer,"\r\n");print(debug_buffer); 			//SJL - CAVR2 - debug
    while(*c != 0)
    {
        if(*c != '\r' && *c != '\n')
        {
        	//sprintf(debug_buffer,"while: prev *chksumn = %X\r\n",*chksum);print(debug_buffer);

            (*chksum)=(*chksum)+*(c++);

            //sprintf(debug_buffer,"while: c = ");print(debug_buffer); 	//SJL - CAVR2 - debug
    		//print_eeprom(c);
    		//sprintf(debug_buffer,"\r\n");print(debug_buffer); 			//SJL - CAVR2 - debug
            //sprintf(debug_buffer,"while: new *chksum = %X\r\n\r\n",*chksum);print(debug_buffer);
        }
        else
            c++;
    }
}


void system_reset()
{

    //yank power on the modem
    #if HW_REV == V5
    M_RST = 1;
    delay_ms(100);
    #elif HW_REV == V6
    M_RST = 0;
    delay_ms(100);
    #endif
    #if SYSTEM_LOGGING_ENABLED
    sprintf(buffer,"System reset");log_line("system.log",buffer);
    #endif
    //last_op_e = last_op;
    //error.systemReset++;
    if(error.watchdogReset > 0)
        error.watchdogReset--;
    wakeRover();
    while(1);
}

#if MODEM_TYPE==Q2406B
const char *RETURNED_MESSAGES[] = {"OK","ERR","Ok_Info_WaitingForData", "+WIND: 4","+CGREG: 1","+CGREG: 5",
                                "Ok_Info_GprsActivation", "+CREG:", "+CSQ:", "#STATE:", "+CDS:", "+CMGR:", "Ok_Info",
                                "+CPIN:","+WIPPEERCLOSE:"};
#elif MODEM_TYPE==Q24NG_PLUS
const char *RETURNED_MESSAGES[] = {"OK","ERR","Ok_Info_WaitingForData", "+WIND:","+CGREG: 1","+CGREG: 5",
                                "Ok_Info_GprsActivation", "+CREG:", "+CSQ:", "#STATE:", "+CDS:", "+CMGR:", "Ok_Info",
                                "+CPIN:","+WIPPEERCLOSE:"};
#else
const char *RETURNED_MESSAGES[] = {"OK","ERR","Ok_Info_WaitingForData", "IND:8","+CGREG: 1","+CGREG: 5",
                                "Ok_Info_GprsActivation", "+CREG:", "+CSQ:", "#STATE:", "+CDS:", "+CMGR:", "Ok_Info",
                                "+CPIN:","!GETBAND:","NO CARRIER"}; //16 entries
#endif
