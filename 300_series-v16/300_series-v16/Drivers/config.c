
/*
    Handling the new configuration
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>      //SJl - CAVR2 - added
#include <delay.h>      //SJl - CAVR2 - added

#include "flash\file_sys.h"

#include "global.h"
#include "drivers\config.h"
#include "drivers\uart.h"
#include "drivers\str.h"
#include "drivers\input.h"
#include "drivers\output.h"
#include "drivers\contact.h"
#include "drivers\queue.h"      //SJl - CAVR2 - added to access queue functions
#include "drivers\modem.h"      //SJl - CAVR2 - added to access modem functions
#include "drivers\mmc.h"        //SJl - CAVR2 - added to access logging functions
#include "drivers\ds1337.h"     //SJL - CAVR2 - added to access real time clock functions
#include "drivers\uart.h"       //SJL - CAVR2 - added to access uart functions

#define MAX_REC_STR 4

/* Global Variable Definitions */
/* Global variables needing to be defined for linking with CAVR2 */
long log_entry_timer = -1;      //from main.c - changed to signed	5
int log_update_timer = -1;      //from main.c - changed to signed	1
bit config_continue;
bit config_break;
//char _FF_buff[512];
bit timer_overflow;
char timer;

//extern eeprom char verbose;
//char counter; - input.h

void check_csd()
{
#if (HW_REV == V6 || HW_REV == V5)
    if(counter<2)
        return;
    if(modem_TMEnabled && !csd_connected)
    {
        //Hang up the modem
        _status_led_off();
        modem_restart();
        modem_init(build_type);
        modem_TMEcho = false;
        modem_TMEnabled = false;
        queue_resume(&q_modem);
        #if SYSTEM_LOGGING_ENABLED
        sprintf(buffer,"CSD call dropped");log_line("system.log",buffer);
        #endif
        if(verbose)
        {
            sprintf(buffer,"\r\nCSD dropped without notification, cleaning up\r\n");
            print(buffer);
        }
        return;
    }
#else
    return;
#endif
}

struct temp_email_settings
{
    unsigned int smtp_port;
    char smtp_serv[60];
    char smtp_un[30];
    char smtp_pw[30];
    char email_domain[60];

    char from_address[60];

    unsigned int pop3_port;
    char pop3_serv[60];
    char pop3_un[30];
    char pop3_pw[30];
    char site_name[21];
    char dns0[16];
    char dns1[16];
	char apn[40];
};

void config_read_file(void)
{
    //SJL - CAVR2 - temp variables
	//char* temp_ptr;
	//char temp_char[60];
    unsigned int temp_int;
    unsigned int y,x;
    //unsigned long test_long;


    //unsigned char config_paused = false;

    char *str_rec[MAX_REC_STR];
    char *command[MAX_REC_STR];
    char *temp;
    char *savedContacts; //SJL-310111
    struct temp_email_settings *savedEmailSettings;
    int savedContactLength=0;
    unsigned char i=0;
    unsigned int j, k;
    unsigned char input_number = ONE;
    unsigned char alarm_number = ONE;
    unsigned char input_rec = false;
    unsigned char output_number = 0;
    unsigned char output_rec = false;
    eeprom char* temp_eeprom;
    config_continue=true;

    #if SYSTEM_LOGGING_ENABLED
    sprintf(buffer,"Configuring");
    log_line("system.log",buffer);
    #endif
   	contact_reset();
   	//SJL - CAVR2 - char *savedContacts; file_sys.h: extern unsigned char _FF_buff[512];
    //savedContacts = _FF_buff;	//malloc(512);

    //sprintf(buffer,"savedContacts(=_FF_buff) = [%s]\r\n",savedContacts);log_line("system.log",buffer);

   	//savedEmailSettings = malloc(sizeof(struct temp_email_settings));	//SJL - CAVR2 - debug - memory problems, commented out
   	//memset(savedContacts,0,512);										//SJL - CAVR2 - debug - memory problems, commented out
   	//memset(savedEmailSettings,0,512);									//SJL - CAVR2 - debug - memory problems, commented out
    //sprintf(buffer,"Start the File Transfer\r\n");
	//print(buffer);
    //log_line("system.log",buffer);	//SJL - debug - download config from config manager

	timer_overflow=false;
	timer=10;	//10

	email_retry_period = -1;	
	
	sprintf(buffer,"\r\nStart the File Transfer\r\n");print(buffer);
	sprintf(buffer,"Please Wait...\r\n");print(buffer);

    //global_interrupt_off();	//SJL - CAVR2 - test to see if an interrupt is halting the config dl
    //sedateRover();			//SJL - CAVR2 - test to see if WDT is halting the config dl
	//it's not just the WDT that's timing out

	while(config_continue && !timer_overflow) {

	    check_csd();

        /*************************************************************************************
        SJL - CAVR2 - debug                                                                  */
        //sprintf(buffer,"csd Checked\r\n");
		//print(buffer);
    	//log_line("system.log",buffer);
        /*************************************************************************************/

        tickleRover();
        /*************************************************************************************
        SJL - CAVR2 - debug                                                                  */
        //sprintf(buffer,"WDT reset\r\n");
		//print(buffer);
    	//log_line("system.log",buffer);
        /*************************************************************************************/

	    if(uart_readPortNoEcho())
	    {

        	/*************************************************************************************
        	SJL - CAVR2 - debug - need to know what the contents of uart_rx_string                                                                */
        	//tickleRover();
            //sprintf(buffer,"Reading config file\r\n");
    		//log_line("system.log",buffer);
            //tickleRover();
            //sprintf(buffer,"The received command is : [%s]\r\n",uart_rx_string);
    		//log_line("system.log",buffer);
            //tickleRover();
        	/*************************************************************************************/

            //sprintf(buffer,"config.c received - [%s]\r\n",uart_rx_string);	//SJL - CARV2 - debug
            //log_line("system.log",buffer);              						//SJL - CARV2 - debug

	        //clear line flag here if needed
	        if(strstrf(uart_rx_string,"NO CARRIER"))
	        {
            	/*************************************************************************************
        		SJL - CAVR2 - debug - need to know what the contents of uart_rx_string                                                                */
            	//sprintf(buffer,"The received command is : [%s]\r\n",uart_rx_string);
    			//log_line("system.log",buffer);
                /*************************************************************************************/

	            timer_overflow = true;
	            modem_disconnect_csd();
                reset_uarts();
                break;
	        }

	        print_char(46);
            timer=10;
			
			sprintf(buffer,"uart_rx_string: (%s)\r\n",uart_rx_string);//print1(buffer);
			
	        k = strlen(uart_rx_string);
	        i=0;
	        //Break the string at the equals sign
            ///*
            str_rec[0]=uart_rx_string;
            if(strchr(uart_rx_string,'='))
            {
                temp = strchr(uart_rx_string,'=');	//pointer at '='
                *temp = '\0';   					//replace equals character with null character
                temp++;                             //incrememt pointer
            }

            //uart_rx_string = pin\0"1234"

            //#define _READ_DEBUG2_
            #ifdef _READ_DEBUG2_
            sprintf(buffer, "[%s]",str_rec[i]);print1(buffer);
            #endif
		    i=1;

            if(strchr(temp,'"')!=NULL)
            {
		        str_rec[i] = strchr(temp, '"');
		        *str_rec[i] = '\0';
		        str_rec[i]++;
		        #ifdef _READ_DEBUG2_
                sprintf(buffer, "str_rec[%d] = {%s}\r\n",i,str_rec[i]);print1(buffer);
                #endif
                if(strrchr(str_rec[i],'"')!=NULL)
    		        *strrchr(str_rec[i],'"') = '\0';
    		    #ifdef _READ_DEBUG2_
                sprintf(buffer, "{%s}\r\n",str_rec[i]);print1(buffer);
                #endif
		    }

            //str_rec[1] = \01234\0

            /*************************************************************************************
        	SJL - CAVR2 - debug - need to know what the contents of uart_rx_string                                                                */
        	//sprintf(buffer,"str_rec[0] : [%s]\r\n",str_rec[0]);print(buffer);
    		//log_line("system.log",buffer);
            //sprintf(buffer,"str_rec[1] : [%s]\r\n",str_rec[1]);print(buffer);
    		//log_line("system.log",buffer);
        	/*************************************************************************************/

	        i=0;
	        //Tokenise the command at start with white space
	        command[i++]=strtok(str_rec[0]," \t");
	        while ((command[i] = strtok(NULL," \t")) != NULL) {
			    i++;
			    if (i > MAX_REC_STR)
			    {
			        break;
			    }
		    }

            //command[0] = pin
            //command[1] = 1234
            /*************************************************************************************
        	SJL - CAVR2 - debug - need to know what the contents of uart_rx_string                                                                */
        	//sprintf(buffer,"command 0 : [%s], ",command[0]);log_line("system.log",buffer);
    		//sprintf(buffer,"command 1 : [%s]\r\n",command[1]);log_line("system.log",buffer);
            //sprintf(buffer,"str_rec[1] : [%s]\r\n",str_rec[1]);log_line("system.log",buffer);
        	/*************************************************************************************/
            
			delay_ms(50);
			delay_ms(25); //25 works - doesn't work 10?

		    //#ifdef DEBUG
		    //if(strstrf(str_rec[0],"out"))
		    //{
		    //     sprintf(buffer,"[%s]{%s}\r\n",str_rec[0],str_rec[1]);
		    //     print1(buffer);
		    //}
		    //#endif

	        if (strcmpf(command[0],"public") == 0)
	        {
	            if (strcmpf(str_rec[1],"true") == 0)//{
	                config.public_queries=true;
                    /*************************************************************************************
        			SJL - CAVR2 - debug - need to know what the contents of uart_rx_string                                                                */
        			//tickleRover();
                    // *temp_ptr=1;
                	//sprintf(buffer,"Public Queries : [%c]\r\n",temp_ptr);
    				//log_line("system.log",buffer);

                    // *temp_ptr=(char)config.public_queries;
                    //sprintf(buffer,"Public Queries 2 : [%c]\r\n",temp_ptr);
    				//log_line("system.log",buffer);}
                	//tickleRover();
        			/*************************************************************************************/
	            else if (strcmpf(str_rec[1],"false") == 0)
	                config.public_queries=false;
	        }
	        else if (strncmpf(command[0],"phone",5) == 0) {
                //sprintf(buffer,"entering: config.c received - [%s]\r\n",uart_rx_string);log_line("system.log",buffer);	//SJL - CARV2 - debug
                //sprintf(buffer,"command - [%s]\r\n",command[0]);log_line("system.log",buffer);					//SJL - CARV2 - debug
                //sprintf(buffer,"extracted number - [%s]\r\n",str_rec[1]);log_line("system.log",buffer);			//SJL - CARV2 - debug

                #ifdef _READ_DEBUG_
    	        //sprintf(buffer, "command[0] = {%s}\r\n",command[0]);  print1(buffer);
                #endif
	            if (strlen(str_rec[1]) > 0){
                    if(strcmpf(str_rec[1],"<>") != 0)
                    {
                        if(strchr(str_rec[1],'@') != 0)
                        {
                            sprintf(buffer,"E%s",str_rec[1]);
                        }
                        /** This code may be used later. Removing for config speed during CSD calls
                        else if(strstrf(str_rec[1],"ftp://") != 0)
                        {
                            sprintf(buffer,"F%s",str_rec[1]);
                        }
                        else if(strstrf(str_rec[1],"mailto://") != 0)
                        {
                            sprintf(buffer,"E%s",str_rec[1]);
                        }
                        else if(strstrf(str_rec[1],"file://") != 0)
                        {
                            sprintf(buffer,"D%s",str_rec[1]);
                        }
                        else if(strstrf(str_rec[1],"http://") != 0)
                        {
                            sprintf(buffer,"H%s",str_rec[1]);
                        } */
                        else
                        {
                            sprintf(buffer,"S%s",str_rec[1]);
                            //log_line("system.log",buffer);	//SJL - CAVR2 - Debug - last contact + 1 saves as YSTEM  LOG
                        }
                        //log_line("system.log",buffer);	//SJL - CAVR2 - Debug - last contact + 1 saves as YSTEM  LOG
                        contact_write(buffer);
						
						//sprintf(buffer,"Contact saved");log_line("system.log",buffer);
                        
						/*
						//this is diiiiirty, but I'm doing it anyway.
                        if(savedContactLength+strlen(buffer)<512)
                        {
                            strcpy(savedContacts+savedContactLength,buffer);
                            savedContactLength+=strlen(buffer)+1;
                	     	//sprintf(buffer, "saved end @ %x= {%s}\r\n",savedContacts+savedContactLength,str_rec[1]);log_line("system.log",buffer);  //print1(buffer);
                        }
						*/
                    }
	            }
                //sprintf(buffer,"exiting\r\n");log_line("system.log",buffer);			//SJL - CARV2 - debug
	        }
	        else if (strcmpf(command[0],"fwdmsg") == 0)
	        {
	            if (strcmpf(str_rec[1],"true") == 0)
	                config.forwarding=true;
	            else if (strcmpf(str_rec[1],"false") == 0)
	                config.forwarding=false;
	        }
	        else if (strcmpf(command[0],"retry") == 0)
	        {
	            if (strcmpf(str_rec[1],"true") == 0)
	                config.sms_retry=true;
	            else if (strcmpf(str_rec[1],"false") == 0)
	                config.sms_retry=false;
	        }
	        else if (strcmpf(command[0],"pin") == 0) {
	           //Read PIN working - SJL - CAVR2

               //sprintf(buffer,"setting pin to {%s}\r\n",substr(str_rec[1],str_rec[1],0,4)); //%04X,
               //log_line("system.log",buffer);
	           //print1(buffer);

               strcpye(config.pin_code,substr(str_rec[1],str_rec[1],0,4));
	        }
	        else if (strcmpf(command[0],"sitemsg") == 0) {
	            //strcpy(savedEmailSettings->site_name,substr(str_rec[1],str_rec[1],0,20));

				//SJL - CAVR2
                //I've put this here because copying from savedEmailSettings at the bottom
                //wasn't working
				strcpye(config.site_name,substr(str_rec[1],str_rec[1],0,20));

             	//sprintf(buffer,"setting site to {%s}\r\n",substr(str_rec[1],str_rec[1],0,20));
               	//log_line("system.log",buffer);

             //set from field for email
//                  #if SMTP_CLIENT_AVAILABLE && MODEM_TYPE==Q2406B
//    	            printf("AT#SENDERNAME=\"%s\"\r\n",str_rec[1]);//
//	                modem_read();
//	                #endif
	            //}
	        }
	        else if (strcmpf(command[0],"logsamplerate") == 0) {
            	//sprintf(buffer,"logsamplerate : [%s]\r\n",str_rec[1]);log_line("system.log",buffer);
                config.log_entry_period = atol(str_rec[1])-1;				
				//log_entry_timer = config.log_entry_period;
                if(config.log_entry_period<0)
				{
					log_entry_timer = -1;	//config.log_entry_period;
				}
				else
				{
					log_entry_timer = config.log_entry_period+1;
				}			
	        }
	        else if (strcmpf(command[0],"logupdaterate") == 0) {
            	//sprintf(buffer,"logupdaterate : [%s]\r\n",str_rec[1]);log_line("system.log",buffer);
                config.log_update_period = atoi(str_rec[1])-1;
				//log_update_timer = config.log_update_period;
				if(config.log_update_period<0)
				{
					log_update_timer = -1;	//config.log_update_period;
				}
				else
				{
					log_update_timer = config.log_update_period+1;
				}
				
	        }
	        else if (strcmpf(command[0],"logdata") == 0) {
            	//sprintf(buffer,"logdata\r\n");log_line("system.log",buffer);
                if(strcmpf(str_rec[1],"enabled") == 0)
                {
                    config.data_log_enabled = true;
                }
                else
                {
                    config.data_log_enabled = false;
                }
	        }
	        else if (strcmpf(command[0],"logalarm") == 0) {
                if(strcmpf(str_rec[1],"enabled") == 0)
                {
                    config.alarm_log_enabled = true;
                }
                else
                {
                    config.alarm_log_enabled = false;
                }
	        }
	        else if (strcmpf(command[0],"logrssi") == 0) {
                if(strcmpf(str_rec[1],"enabled") == 0)
                {
                    config.rssi_log_enabled = true;
                }
                else
                {
                    config.rssi_log_enabled = false;
                }
	        }
	        else if (strcmpf(command[0],"loglocation") == 0) {
                if(strcmpf(str_rec[1],"enabled") == 0)
                {
                    config.loc_log_enabled = true;
                }
                else
                {
                    config.loc_log_enabled = false;
                }
	        }
	        else if (strcmpf(command[0],"advancedfeatures") == 0) {
                if(strcmpf(str_rec[1],"enabled") == 0)
                {
                    config.advanced_features = true;
                }
                else
                {
                    config.advanced_features = false;
                }
	        }
			else if (strcmpf(command[0],"logcontact") == 0) {
			//else if (strcmpf(str_rec[0],"logcontact") == 0) {
                //#ifdef DEBUG				
				//sprintf(buffer,"str_rec[0]: (%s)\r\n",str_rec[0]);print1(buffer);
                //sprintf(buffer,"logcontact [str_rec[1]]: (%s)\r\n",str_rec[1]);print1(buffer);
				//sprintf(buffer,"found logcontact (%s)\r\n",str_rec[1]);log_line("system.log",buffer);
                //#endif
	            
				strcpye(config.update_address,substr(str_rec[1],str_rec[1],0,63));
				//strcpye(config.update_address,str_rec[1]);
				
				//sprintf(buffer,"logcontact set: ");print(buffer);
				//strcpyre(buffer,config.update_address);
				//print(buffer);sprintf(buffer,"\r\n");print(buffer);
	        }
	        #if EMAIL_AVAILABLE		//changed from TCPIP to EMAIL
			//#if TCPIP_AVAILABLE	//now defunct
	        else if (strcmpf(command[0],"connection") == 0){
	            if(strcmpf(command[1],"apn") == 0)
	            {
	                //strcpye(config.apn,str_rec[1]);	//already commented out
	                //strcpy(savedEmailSettings->apn,str_rec[1]);
					
					strcpye(config.apn,substr(str_rec[1],str_rec[1],0,39));
	            }
	            else if (strcmpf(command[1],"primarydns") == 0)
	            {
	                //set dns1
	                //#if MODEM_TYPE == Q2406B
	                //printf("AT#DNSSERV1=\"%s\"\r\n",str_rec[1]);
	                //modem_read();
	                //#elif MODEM_TYPE == Q24NG_PLUS
	               // sprintf(buffer,"DNS1 is now [%s]\r\n",str_rec[1]);
	               // print1(buffer);
	                //strcpy(savedEmailSettings->dns0,str_rec[1]);	//just this line commented out
	               // #endif
				   
				   strcpye(config.dns0,substr(str_rec[1],str_rec[1],0,15));
	            }
	            else if (strcmpf(command[1],"secondarydns") == 0)
	            {
	                //set dns2
	                //#if MODEM_TYPE == Q2406B
	                //printf("AT#DNSSERV2=\"%s\"\r\n",str_rec[1]);
	                //modem_read();
	                //#endif
	               // sprintf(buffer,"DNS2 is now [%s]\r\n",str_rec[1]);
	               // print1(buffer);
	                //strcpy(savedEmailSettings->dns1,str_rec[1]);	//just this line commented out
					
					strcpye(config.dns1,substr(str_rec[1],str_rec[1],0,15));
	            }
	        }
	        #endif
	        else if (strcmpf(command[0],"autoreport") == 0){
	            if(strcmpf(command[1],"time")==0)
	            {
	               // sprintf(buffer,"parse time from %s\r\n",str_rec[1]);
	               // print(buffer);
	                config.autoreport.hour = atoi(strtok(str_rec[1],":"));
	                config.autoreport.minute = atoi(strtok(0,":"));
	                config.autoreport.second = atoi(strtok(0,""));
                    //print("autpreport time set");	//SJL - Debug config crash
	            }
	            else if(strcmpf(command[1],"day")==0)
	            {
	               // sprintf(buffer,"parse day from %s\r\n",str_rec[1]);
	               // print(buffer);
	                config.autoreport.day = atoi(str_rec[1]);
                    //print("autpreport day set");	//SJL - Debug config crash
	            }
	            else if(strcmpf(command[1],"frequency")==0)
	            {
	               // sprintf(buffer,"parse frequency from %s\r\n",str_rec[1]);
	               // print(buffer);
	                if(strstrf(str_rec[1],"daily"))
                        config.autoreport.type = ALARM_DAY;
	                else if(strstrf(str_rec[1],"weekly"))
                        config.autoreport.type = ALARM_WEEK;
                    else if(strstrf(str_rec[1],"monthly"))
                        config.autoreport.type = ALARM_MONTH;
                    else if(strstrf(str_rec[1],"never"))
                        config.autoreport.type = NO_ALARM;
                    //print("autpreport frequency selected");	//SJL - Debug config crash
                    rtc_init();		//SJL
                    //print("RTC initiated");	//SJL - Debug config crash
                    if(config.autoreport.type != NO_ALARM) //SJL - inserted to avoid crash in RTC
                    {
                    	rtc_set_alarm_1(config.autoreport.day,config.autoreport.hour,config.autoreport.minute,
                                    config.autoreport.second,config.autoreport.type);
                    	//print("RTC set alarm");	//SJL - Debug config crash
                    }
                    //print("config continue");	//SJL - Debug config crash1
                    //rtc_init();	//SJL - moved above set alarm because the init clears the alarm
	            }
	        }
	        #if EMAIL_AVAILABLE
			// Currently supported email configuration options: 
			// SMTP server and port
			// from address
			// email domain
	        else if ((strcmpf(command[0],"email") == 0)){
	            if(strstrf(command[1],"smtp"))
	            {
	                if(strcmpf(command[1],"smtpserver") == 0)
    	            {
	                    //strcpy(savedEmailSettings->smtp_serv,str_rec[1]);
						
						//strcpye(config.smtp_serv,str_rec[1]);
						strcpye(config.smtp_serv,substr(str_rec[1],str_rec[1],0,59));
						
						//strcpye(config.smtp_serv,(*savedEmailSettings).smtp_serv);
						//substr(char* large, char *returnValue, int start, int end)
						//strcpye(config.site_name,substr(str_rec[1],str_rec[1],0,20));
	                }
	                else if (strcmpf(command[1],"smtpport") == 0)
	                {
    	                config.smtp_port = atoi(str_rec[1]);
	                }
					
					// smtp username and password not used but required for successful config download
	                else if (strcmpf(command[1],"smtppassword") == 0)
	                {
						strcpye(config.smtp_pw,substr(str_rec[1],str_rec[1],0,29));
	                    //strcpy(savedEmailSettings->smtp_pw,str_rec[1]);
    	            }
	                else if (strcmpf(command[1],"smtpusername") == 0)
	                {
						strcpye(config.smtp_un,substr(str_rec[1],str_rec[1],0,29));
						//strcpy(savedEmailSettings->smtp_un,str_rec[1]);
	                }
					
	            }
	            // pop3 connection disabled but required for successful config download
				else if(strstrf(command[1],"pop3"))
	            {
	                if (strcmpf(command[1],"pop3server") == 0)
    	            {
						strcpye(config.pop3_serv,substr(str_rec[1],str_rec[1],0,59));
	                    //strcpy(savedEmailSettings->pop3_serv,str_rec[1]);
	                }
	                else if (strcmpf(command[1],"pop3port") == 0)
	                {
	                    config.pop3_port = atoi(str_rec[1]);
    	            }
	                else if (strcmpf(command[1],"pop3password") == 0)
	                {
						strcpye(config.pop3_pw,substr(str_rec[1],str_rec[1],0,29));
	                    //strcpy(savedEmailSettings->pop3_pw,str_rec[1]);
	                }
    	            else if (strcmpf(command[1],"pop3username") == 0)
	                {
						strcpye(config.pop3_un,substr(str_rec[1],str_rec[1],0,29));
	                    //strcpy(savedEmailSettings->pop3_un,str_rec[1]);
	                }
	            }
				
	            else if (strcmpf(command[1],"fromaddress") == 0)
    	        {
	                //strcpy(savedEmailSettings->from_address,str_rec[1]);
					strcpye(config.from_address,substr(str_rec[1],str_rec[1],0,59));
	            }
	            else if (strcmpf(command[1],"emaildomain") == 0)
	            {
	                //strcpy(savedEmailSettings->email_domain,str_rec[1]);
					strcpye(config.email_domain,substr(str_rec[1],str_rec[1],0,59));
    	        }
				/* enable receipt of email disabled - incoming email is disabled
	            else if (strcmpf(command[1],"enablereceipt") == 0)
	            {
	                //set fetch email flag
                    if(strcmpf(str_rec[1],"true") == 0)
                    {
                        email_recMail = true;
                    }
                    else
                    {
                        email_recMail = false;
                    }
	            }
				*/
	        }
	        #endif

	        else if(!strncmpf(command[0],"in",2))
	        {
    	        input_rec = true;
    	        output_rec = false;
    	        input_number = atoi(command[0]+2)-1;
    	        if(command[0][3] == 'a')
    	            alarm_number = 0;
    	        else if(command[0][3] == 'b')
    	            alarm_number = 1;
	        }
	        else if(!strncmpf(command[0],"out",3))
	        {
	            output_rec = true;
	            input_rec = false;
	            output_number = atoi(command[0]+3)-1;
	        }

	        else if (strcmpf(str_rec[0], "AT+END") == 0)
	        {
            	/*************************************************************************************
        		SJL - CAVR2 - debug - need to know what the contents of uart_rx_string                                                                */
            	//sprintf(buffer,"AT+END received\r\n");
    			//log_line("system.log",buffer);
                /*************************************************************************************/
	            //config_continue = false;
				
				sprintf(buffer,"\r\nConfig Loaded Completely\r\n");print(buffer);
	        }
			
			else if (strcmpf(str_rec[0], "AT+CHKSUM") == 0)
	        {
            	//replacement for detection of at+end			
				
				//print(buffer);sprintf(buffer,"checksum request\r\n");print(buffer);
				
				config_print_file(true); //input_resume() in here
				
				config_continue = false;
	        }

	        tickleRover();
	        if (input_rec)
	        {
	            input_rec = false;

    	        if (strcmpf(command[1],"enabled") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"true") == 0)
    	            {
    	                config.input[input_number].enabled = ENABLE;
    	            }
    	            else if (strcmpf(str_rec[1],"false") == 0)
    	            {
    	                config.input[input_number].enabled = DISABLE;
    	            }
    	        }
    	        if(input_number >= 6)
    	        {
					#if PULSE_COUNTING_AVAILABLE
        	        if (strcmpf(command[1],"pulseperiod") == 0)
        	        {
    	                if (strcmpf(str_rec[1],"seconds") == 0)
    	                {
    	                    config.pulse[input_number-6].period = SECONDS;
    	                }
        	            else if (strcmpf(str_rec[1],"minutes") == 0)
        	            {
    	                    config.pulse[input_number-6].period = MINUTES;
    	                }
    	                else if (strcmpf(str_rec[1],"hours") == 0)
    	                {
        	                config.pulse[input_number-6].period = HOURS;
        	            }
    	            }
					#else
					config.pulse[input_number-6].period = SECONDS;
					#endif
    	            if (strcmpf(command[1],"pulsespercount") == 0)
    	            {
        	           config.pulse[input_number-6].pulses_per_count = atof(str_rec[1]);
        	        }
    	        }
    	        if (strcmpf(command[1],"loginstant") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"enabled") == 0)
    	            {
    	                config.input[input_number].log_type |= LOG_INSTANT;
    	            }
    	            else
    	            {
    	                config.input[input_number].log_type &= ~LOG_INSTANT;
    	            }

    	        }
    	        else if (strcmpf(command[1],"logaverage") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"enabled") == 0)
    	            {
    	                config.input[input_number].log_type |= LOG_AVERAGE;
    	            }
    	            else
    	            {
    	                config.input[input_number].log_type &= ~LOG_AVERAGE;
    	            }

    	        }
    	        else if (strcmpf(command[1],"alarmlog") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"enabled") == 0)
    	            {
    	                config.input[input_number].log_type |= LOG_ALARM;
    	            }
    	            else
    	            {
    	                config.input[input_number].log_type &= ~LOG_ALARM;
    	            }

    	        }
    	        else if (strcmpf(command[1],"logmin") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"enabled") == 0)
    	            {
    	                config.input[input_number].log_type |= LOG_MIN;
    	            }
    	            else
    	            {
    	                config.input[input_number].log_type &= ~LOG_MIN;
    	            }

    	        }
    	        else if (strcmpf(command[1],"logmax") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"enabled") == 0)
    	            {
    	                config.input[input_number].log_type |= LOG_MAX;
    	            }
    	            else
    	            {
    	                config.input[input_number].log_type &=~ LOG_MAX;
    	            }
    	        }
    	        else if (strcmpf(command[1],"logaggregate") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"enabled") == 0)
    	            {
    	                config.input[input_number].log_type |= LOG_AGGREGATE;
    	            }
    	            else
    	            {
    	                config.input[input_number].log_type &=~ LOG_AGGREGATE;
    	            }
    	        }
    	        else if (strcmpf(command[1],"logduty") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"enabled") == 0)
    	            {
    	                config.input[input_number].log_type |= LOG_DUTY;
    	            }
    	            else
    	            {
    	                config.input[input_number].log_type &=~ LOG_DUTY;
    	            }
    	        }
    	        else if (strcmpf(command[1],"type") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"digital") == 0)
    	            {
    	                config.input[input_number].type = DIGITAL;
    	            }
    	            else if (strcmpf(str_rec[1],"4-20mA") == 0 || strcmpf(str_rec[1],"current") == 0)
    	            {
    	                config.input[input_number].type = ANALOG_4_20_mA;
    	            }
    	            else if (strcmpf(str_rec[1],"0-5V") == 0 || strcmpf(str_rec[1],"0-5v") == 0 || strcmpf(str_rec[1],"voltage") == 0)
    	            {
    	                config.input[input_number].type = ANALOG_0_5V;
    	            }
    	            #if PULSE_COUNTING_AVAILABLE
    	            else if (strcmpf(str_rec[1],"pulse") == 0)
    	            {
    	                config.input[input_number].type = PULSE;
    	            }
    	            #endif
    	        }
    	        else if (strcmpf(command[1],"alarmmsg") == 0)
    	        {
    	            strcpye(config.input[input_number].alarm[alarm_number].alarm_msg,substr(str_rec[1],str_rec[1],0,MAX_MSG_LEN));
    	        }
    	        else if (strcmpf(command[1],"resetmsg") == 0)
    	        {
    	            strcpye(config.input[input_number].alarm[alarm_number].reset_msg,substr(str_rec[1],str_rec[1],0,MAX_MSG_LEN));
    	        }
    	        else if (strcmpf(command[1],"decimal") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"true") == 0)
    	            {
    	                config.input[input_number].dp = true;
    	            }
    	            else if (strcmpf(str_rec[1],"false") == 0)
    	            {
    	                config.input[input_number].dp = false;
    	            }
    	        }
    	        else if (strcmpf(command[1],"alarmtype") == 0)
    	        {
    	            //sprintf(buffer,"in%d alarm%d type=\"%s\"\r\n",input_number,alarm_number,str_rec[1]);
    	            //print(buffer);
    	            if (strstrf(str_rec[1],"above") != 0)
    	            {
    	                config.input[input_number].alarm[alarm_number].type = ALARM_ABOVE;
    	                //sprintf(buffer,"Alarming above, i%da%d=%d\r\n",input_number,alarm_number,config.input[input_number].alarm[alarm_number].type);
    	                //print(buffer);
    	            }
    	            else if (strstrf(str_rec[1],"below") != 0)
    	            {
    	                config.input[input_number].alarm[alarm_number].type = ALARM_BELOW;
    	                //sprintf(buffer,"Alarming below, i%da%d=%d\r\n",input_number,alarm_number,config.input[input_number].alarm[alarm_number].type);
    	                //print(buffer);
    	            }
    	            else if (strstrf(str_rec[1],"none") != 0)
    	            {
    	                config.input[input_number].alarm[alarm_number].type = ALARM_NONE;
    	                //sprintf(buffer,"No alarm, i%da%d=%d\r\n",input_number,alarm_number,config.input[input_number].alarm[alarm_number].type);
    	                //print(buffer);
    	            }
    	            else if (strstrf(str_rec[1],"open") != 0)
    	            {
        	            config.input[input_number].alarm[alarm_number].type = ALARM_OPEN;
    	            }
    	            else if (strstrf(str_rec[1],"closed") != 0)
    	            {
        	            config.input[input_number].alarm[alarm_number].type = ALARM_CLOSED;
    	            }
    	        }
    	        else if (strcmpf(command[1],"alarmstart") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"true") == 0)
    	            {
    	                config.input[input_number].alarm[alarm_number].startup_alarm = true;
    	            }
    	            else if (strcmpf(str_rec[1],"false") == 0)
    	            {
    	                config.input[input_number].alarm[alarm_number].startup_alarm = false;
    	            }
    	        }
    	        else if (strcmpf(command[1],"alarmoutputswitch") == 0)
    	        {
    	            config.input[input_number].alarm[alarm_number].alarmAction.outswitch = atoi(str_rec[1]);
    	        }
    	        else if (strcmpf(command[1],"alarmoutaction") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"on") == 0)
    	            {
    	                config.input[input_number].alarm[alarm_number].alarmAction.outaction = on;
    	            }
    	            else if (strcmpf(str_rec[1],"off") == 0)
    	            {
    	                config.input[input_number].alarm[alarm_number].alarmAction.outaction = off;
    	            }
    	        }
    	        else if (strcmpf(command[1],"alarmoutputcontact") == 0)
    	        {
        	        //crack off the first bit, which indicates if internal switching is enabled
        	        if (str_rec[1][0] == '1')
        	            config.input[input_number].alarm[alarm_number].alarmAction.me = on;
        	        else
        	            config.input[input_number].alarm[alarm_number].alarmAction.me = off;

                    temp_int = 0;

        	        for (j=1; j < strlen(str_rec[1]); j++)
        	        {
        	            if (str_rec[1][j] == '1')
                        {
                        	y=1;
                            if(j==1)temp_int=temp_int+1;
                            else
                            {
                            for (x=1; x < j; x++) y=y*2;
                            temp_int=temp_int+y;
                            }
                            //config.input[input_number].alarm[alarm_number].alarmAction.contact_list = temp_int;

        	                //config.input[input_number].alarm[alarm_number].alarmAction.contact_list |= (0x0001 << j-1);
        	            }
                        /* SJL - CAVR2 - not rewuired - 0 by default
                        else
        	                config.input[input_number].alarm[alarm_number].alarmAction.contact_list &= ~(0x0001 << j-1);
                        */
        	        }
                    config.input[input_number].alarm[alarm_number].alarmAction.contact_list = temp_int;
    	        }
    	        else if (strcmpf(command[1],"resetoutputswitch") == 0)
    	        {
    	            config.input[input_number].alarm[alarm_number].resetAction.outswitch = atoi(str_rec[1]);
    	        }
    	        else if (strcmpf(command[1],"resetoutaction") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"on") == 0)
    	            {
    	                config.input[input_number].alarm[alarm_number].resetAction.outaction = on;
    	            }
    	            else if (strcmpf(str_rec[1],"off") == 0)
    	            {
    	                config.input[input_number].alarm[alarm_number].resetAction.outaction = off;
    	            }
    	        }
    	        else if (strcmpf(command[1],"resetoutputcontact") == 0)
    	        {
        	        //crack off the first bit, which indicates if internal switching is enabled
        	        if (str_rec[1][0] == '1')
        	            config.input[input_number].alarm[alarm_number].resetAction.me = on;
        	        else
        	            config.input[input_number].alarm[alarm_number].resetAction.me = off;

                    temp_int=0;

        	        for (j=1; j < strlen(str_rec[1]); j++)
        	        {
                    	if (str_rec[1][j] == '1')
                        {
                        	y=1;
                            if(j==1)temp_int=temp_int+1;
                            else
                            {
                            for (x=1; x < j; x++) y=y*2;
                            temp_int=temp_int+y;
                            }
                            //sprintf(buffer,"j=%d, temp_int=%ld",j,temp_int);log_line("system.log",buffer);
                            //config.input[input_number].alarm[alarm_number].resetAction.contact_list = temp_int;

        	                //config.input[input_number].alarm[alarm_number].resetAction.contact_list |= (0x0001 << j-1);
                        }
                        /* SJL - CAVR2 - Not required - 0 by default
        	            else
        	                config.input[input_number].alarm[alarm_number].resetAction.contact_list &= ~(0x0001 << j-1);
                        */
        	        }
                    config.input[input_number].alarm[alarm_number].resetAction.contact_list = temp_int;
    	        }
    	        else if (strcmpf(command[1],"engmax") == 0)
    	        {
    	            config.input[input_number].eng_max = atof(str_rec[1]);
    	        }
    	        else if (strcmpf(command[1],"engmin") == 0)
    	        {
    	            config.input[input_number].eng_min = atof(str_rec[1]);
    	        }
				else if (strcmpf(command[1],"engunit") == 0)
    	        {
    	            strcpye(config.input[input_number].units,substr(str_rec[1],str_rec[1],0,8));
    	        }
    	        else if (strcmpf(command[1],"alarmlevel") == 0)
    	        {
    	            config.input[input_number].alarm[alarm_number].set = atof(str_rec[1]);
    	        }
    	        else if (strcmpf(command[1],"resetlevel") == 0)
    	        {
    	            config.input[input_number].alarm[alarm_number].reset = atof(str_rec[1]);
    	        }
    	        else if (strcmpf(command[1],"alarmcontact") == 0)
    	        {
                    temp_int = 0;

        	        for (j=0; j < 16; j++)
        	        {
        	            if (str_rec[1][j] == '1')
                        {
                            y=1;
                            if(j==0)temp_int=temp_int+1;
                            else
                            {
                            for (x=1; x < (j+1); x++) y=y*2;
                            temp_int=temp_int+y;
                            }
                            //sprintf(buffer,"temp_int=%ld",temp_int);log_line("system.log",buffer);
                            //config.input[input_number].alarm[alarm_number].alarm_contact = temp_int;

                            //config.input[input_number].alarm[alarm_number].alarm_contact |= (0x0001 << j);
                            //sprintf(buffer,"input=%d, alarm=%d, j=%d, contact=%ld \r\n",input_number,alarm_number,j,config.input[input_number].alarm[alarm_number].alarm_contact);log_line("system.log",buffer);
                        }
                        /* SJl - CAVR2 - not required - 0 by default
        	            else
                        {
                        	//sprintf(buffer,"else (0)\r\n",str_rec[1][j]);log_line("system.log",buffer);
                            config.input[input_number].alarm[alarm_number].alarm_contact &= ~(0x0001 << j);}
        	        	}*/
                    }
                    config.input[input_number].alarm[alarm_number].alarm_contact = temp_int;
    	        }
    	        else if (strcmpf(command[1],"resetcontact") == 0)
    	        {
                    temp_int = 0;

        	        for (j=0; j < 16; j++)
        	        {
        	            if (str_rec[1][j] == '1')
                        {
                        	y=1;
                            if(j==0)temp_int=temp_int+1;
                            else
                            {
                            for (x=1; x < (j+1); x++) y=y*2;
                            temp_int=temp_int+y;
                            }
                            //sprintf(buffer,"temp_int=%ld",temp_int);log_line("system.log",buffer);
                            //config.input[input_number].alarm[alarm_number].reset_contact = temp_int;

        	                //config.input[input_number].alarm[alarm_number].reset_contact |= (0x0001 << j);
                        }
                        /* SJl - CAVR2 - not required - 0 by default
        	            else
        	                config.input[input_number].alarm[alarm_number].reset_contact &= ~(0x0001 << j);
                    	*/
        	        }
                    config.input[input_number].alarm[alarm_number].reset_contact = temp_int;
    	        }
    	        else if (strcmpf(command[1],"debounce") == 0)
    	        {
					//sprintf(buffer,"reading debounce from: %s\r\n",str_rec[1]);
					//print1(buffer);
					//config.input[input_number].alarm[alarm_number].debounce_time = (unsigned long)((atof(str_rec[1])+0.16)/0.32);
    	            
					config.input[input_number].alarm[alarm_number].debounce_time = 100*atol(strtok(str_rec[1],"."));
    	            //sprintf(buffer,"first part: %lu\r\n",config.input[input_number].alarm[alarm_number].debounce_time);
    	            //print1(buffer);
    	            j = atol(strtok(0,""));
    	            //sprintf(buffer,"second part: %lu\r\n",j);
    	            //print1(buffer);
    	            while(j > 100) j /= 10;
    	            config.input[input_number].alarm[alarm_number].debounce_time += j;
    	            //sprintf(buffer,"whole thing: %lu\r\n",config.input[input_number].alarm[alarm_number].debounce_time);
    	            //print1(buffer);
				
					
					#ifdef DEBOUNCE_FILTER
    	            if (config.input[input_number].alarm[alarm_number].debounce_time >= DEBOUNCE_FILTER)
    	                config.input[input_number].alarm[alarm_number].debounce_time -= DEBOUNCE_FILTER;
    	            #endif
    	        }
    	    }
    	    tickleRover();
    	    if (output_rec)
	        {
	            output_rec = false;

    	        if (strcmpf(command[1],"enabled") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"true") == 0)
    	            {
    	                config.output[output_number].enabled = ENABLE;
    	            }
    	            else if (strcmpf(str_rec[1],"false") == 0)
    	            {
    	                config.output[output_number].enabled = DISABLE;
    	            }
    	        }
    	        else if (strcmpf(command[1],"name") == 0)
    	        {
    	            strcpye(config.output[output_number].config.name,substr(str_rec[1],str_rec[1],0,20));
    	        }
    	        else if (strcmpf(command[1],"defstate") == 0)
    	        {
    	            if (strcmpf(str_rec[1],"on") == 0 || strcmpf(str_rec[1],"1") == 0)
    	            {
    	                config.output[output_number].config.default_state = ON;
    	            }
    	            else if (strcmpf(str_rec[1],"off") == 0 || strcmpf(str_rec[1],"0") == 0)
    	            {
    	                config.output[output_number].config.default_state = OFF;
    	            }
    	            else config.output[output_number].config.default_state = LAST_KNOWN;

    	        }
    	        else if (strcmpf(command[1],"momentary") == 0)
    	        {
					//sprintf(buffer,"reading momentary from: %s\r\n",str_rec[1]);
					//print1(buffer);
                    config.output[output_number].config.momentaryLength = atoi(str_rec[1]);
					//sprintf(buffer,"momentary set to: %lu\r\n",config.input[input_number].alarm[alarm_number].debounce_time);
    	            //print1(buffer);
    	        }
    	        else if (strcmpf(command[1],"onmsg") == 0)
    	        {
    	            strcpye(config.output[output_number].config.on_msg,substr(str_rec[1],str_rec[1],0,MAX_MSG_LEN));
    	        }
    	        else if (strcmpf(command[1],"offmsg") == 0)
    	        {
    	            strcpye(config.output[output_number].config.off_msg,substr(str_rec[1],str_rec[1],0,MAX_MSG_LEN));
    	        }
    	        else if (strcmpf(command[1],"oncontact") == 0)
    	        {
                	temp_int=0;

        	        for (j=0; j < 16; j++)
        	        {
                        if (str_rec[1][j] == '1')
                        {
                            y=1;
                            if(j==0)temp_int=temp_int+1;
                            else
                            {
                            for (x=1; x < (j+1); x++) y=y*2;
                            temp_int=temp_int+y;
                            }
                            //config.output[output_number].config.on_contact = temp_int;

        	                //config.output[output_number].config.on_contact |= (0x0001 << j);
                        }
                        /* SJL - CAVR2 - not required - 0 by default
        	            else
        	                config.output[output_number].config.on_contact &= ~(0x0001 << j);
                        */
        	        }
                    config.output[output_number].config.on_contact = temp_int;
    	        }
    	        else if (strcmpf(command[1],"offcontact") == 0)
    	        {
                	temp_int=0;

        	        for (j=0; j < 16; j++)
        	        {
        	            if (str_rec[1][j] == '1')
                        {
                        	y=1;
                            if(j==0)temp_int=temp_int+1;
                            else
                            {
                            for (x=1; x < (j+1); x++) y=y*2;
                            temp_int=temp_int+y;
                            }
                            //config.output[output_number].config.off_contact = temp_int;

        	                //config.output[output_number].config.off_contact |= (0x0001 << j);
                        }
                        /* SJL - CAVR2 - not required - 0 by default
        	            else
        	                config.output[output_number].config.off_contact &= ~(0x0001 << j);
                        */
        	        }
                    config.output[output_number].config.off_contact = temp_int;
    	        }
    	    }
	    }
	}

	/*
    sprintf(buffer,"savedContacts = [%s]\r\n",savedContacts);log_line("system.log",buffer);
    if(savedContactLength > 0)
    {
        savedContactLength = 0;
        do
        {
        	sprintf(buffer,"savedContacts = [%s]\r\n",savedContacts);log_line("system.log",buffer);
			sprintf(buffer,"savedContacts+savedContactLength = [%s]\r\n",savedContacts+savedContactLength);log_line("system.log",buffer);
			contact_write(savedContacts+savedContactLength);
            savedContactLength += strlen(savedContacts+savedContactLength)+1;
			sprintf(buffer,"savedContactLength = [%d]\r\n",savedContactLength);log_line("system.log",buffer);
        } while ( savedContacts[savedContactLength] != 0);
    }
	*/

    /*************************************************************************************
    SJL - CAVR2 - debug               													 */
    //sprintf(buffer,"All commands received\r\n");
    //log_line("system.log",buffer);
    /*************************************************************************************/

	//SJL - CAVR2 - if email_enabled ?? no site name is universal
	//#define COPY_FROM_TEMP(a) strcpye(config.a,savedEmailSettings->a)
    //COPY_FROM_TEMP(smtp_serv);
    //strcpye(config.smtp_serv,(*savedEmailSettings).smtp_serv);
    //COPY_FROM_TEMP(smtp_un);
    //strcpye(config.smtp_un,(*savedEmailSettings).smtp_un);
    //COPY_FROM_TEMP(smtp_pw);
    //strcpye(config.smtp_pw,(*savedEmailSettings).smtp_pw);
    //COPY_FROM_TEMP(email_domain);
    //strcpye(config.email_domain,(*savedEmailSettings).email_domain);
    //COPY_FROM_TEMP(from_address);
    //strcpye(config.from_address,(*savedEmailSettings).from_address);
    //COPY_FROM_TEMP(pop3_serv);
    //strcpye(config.pop3_serv,(*savedEmailSettings).pop3_serv);
    //COPY_FROM_TEMP(pop3_un);
    //strcpye(config.pop3_un,(*savedEmailSettings).pop3_un);
    //COPY_FROM_TEMP(pop3_pw);
    //strcpye(config.pop3_pw,(*savedEmailSettings).pop3_pw);
    //COPY_FROM_TEMP(dns0);
    //strcpye(config.dns0,(*savedEmailSettings).dns0);
    //COPY_FROM_TEMP(dns1);
    //strcpye(config.dns1,(*savedEmailSettings).dns1);
	//COPY_FROM_TEMP(apn);
    //strcpye(config.apn,(*savedEmailSettings).apn);
	//COPY_FROM_TEMP(site_name);
    //strcpye(config.site_name,(*savedEmailSettings).site_name);

    //free(savedEmailSettings);	//SJL - CAVR2 - not required

    /*************************************************************************************
	SJL - CAVR2 - debug - need to know what the contents of uart_rx_string              */
    //sprintf(buffer,"New config saved\r\n");
    //log_line("system.log",buffer);
    /*************************************************************************************/

	if (timer_overflow)
	{
	    timer_overflow=false;
	    #if SYSTEM_LOGGING_ENABLED
	    sprintf(buffer,"Config timed out");log_line("system.log",buffer);
	    #endif
	    sprintf(buffer,"\r\nComm timed out, Config may not be fully loaded.\r\n\n");
	    print(buffer);
	    check_csd();

	}
	else
	{
    	#if SYSTEM_LOGGING_ENABLED
    	sprintf(buffer,"Config completed");
    	log_line("system.log",buffer);
    	#endif
		startup_config_reset();
	    //sprintf(buffer,"\r\nConfig Loaded Completely\r\n");
	    //print(buffer);
        
    }
}

char *get_payload(char *c)
{
    char *rtn;
    if(strchr(c,'"'))
    {
        while(*c!='"'&&*c!=0) c++;
        rtn=c;
        while(*(++c)!='"'&&*c!=0);
        *c='\0';
        return ++rtn;
    }
    else if(strchr(c, ' '))
    {
        while(*(++c)!=' '&&*c!=0);
        return ++c;
    }
    else
    {
        return c;
    }
}

void config_print_contacts(bool checksum) {
    unsigned char i;
    eeprom char *temp;
    char chksum = 0;
    for (i=0; i < MAX_CONTACTS; i++) {
        temp = contact_read(i);
        if (temp == 0)
        {
                sprintf(buffer,"phone%d=\"\"\r\n",i+1);
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
                sprintf(buffer,"phone%d=\"",i+1);
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
                if(temp[0] == '<')
                    if(checksum) increment_chksume(temp,&chksum); else print1_eeprom(temp);
                else
                    if(checksum) increment_chksume(temp+1,&chksum); else print1_eeprom(temp+1);//remove type character
                sprintf(buffer,"\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
    }
    tickleRover();
    if(checksum)
    {
        sprintf(buffer,"+CHKSUM: %02X\r\n",chksum);
        print(buffer);
    }
    else
    {
        sprintf(buffer,"\r\nAT+END\r\n");
        print(buffer);
    }
}

void config_print_file(bool checksum) {


    unsigned char i,j,k;
    unsigned char int1, int2, int3;
    eeprom char *temp;
    unsigned char chksum = 0;
    unsigned char serialchksum = 0;
    char debug_buffer[50];																	//SJL - CAVR2 - debug

    //sprintf(debug_buffer,"Print the checksum");   										//SJL - CAVR2 - debug
	//log_line("system.log",debug_buffer);													//SJL - CAVR2 - debug

    //sprintf(debug_buffer,"after initialization +CHKSUM=%X\r\n",chksum);print(debug_buffer);	//SJL - CAVR2 - debug

    rtc_get_time(&int1, &int2, &int3);
    sprintf(buffer,"#EDAC320 Configuration File     %02d:%02d.%02d ", int1,int2,int3);
    if(!checksum) print(buffer);
    rtc_get_date(&int1, &int2, &int3);
    sprintf(buffer,"%02d/%02d/20%02d\r\n\r\n", int1,int2,int3);
    if(!checksum) print(buffer);
    sprintf(buffer,"serial=\"");

    increment_chksum(buffer,&serialchksum);
    increment_chksume(serial,&serialchksum);
    //sprintf(debug_buffer,"serialchksum 1 = %X\r\n",serialchksum);print(debug_buffer);				//SJL - CAVR2 - debug

    if(checksum)
    {
    	//sprintf(debug_buffer,"buffer = %s\r\n",buffer);print(debug_buffer);				//SJL - CAVR2 - debug
        increment_chksum(buffer,&chksum);
        //sprintf(debug_buffer,"serial part 1 +CHKSUM=%X\r\n",chksum);print(debug_buffer);	//SJL - CAVR2 - debug

		//strcpye(serial,"0522820");
        //sprintf(debug_buffer,"\r\nserial new = ");print(debug_buffer);					//SJL - CAVR2 - debug
        //print_eeprom(serial);      														//SJL - CAVR2 - debug
        //sprintf(debug_buffer,"\r\n");print(debug_buffer);									//SJL - CAVR2 - debug
        //sprintf(debug_buffer,"serial = %d\r\n",serial);print(debug_buffer);				//SJL - CAVR2 - debug
        increment_chksume(serial,&chksum);
        //sprintf(debug_buffer,"serial part 2 +CHKSUM=%X\r\n",chksum);print(debug_buffer);	//SJL - CAVR2 - debug
	}
    else
    {
        print(buffer);
        print_eeprom(serial);
    }

    sprintf(buffer,"\"\r\n");
    increment_chksum(buffer,&serialchksum);
    //sprintf(debug_buffer,"serialchksum 2 = %X\r\n",serialchksum);print(debug_buffer);		//SJL - CAVR2 - debug
    if(checksum)
        increment_chksum(buffer,&chksum);
    else print(buffer);

    //sprintf(debug_buffer,"serial complete +CHKSUM=%X\r\n",chksum);print(debug_buffer);		//SJL - CAVR2 - debug

    tickleRover();	//SJL - CAVR2 - debug

    //serial offset so that it always adds 0 to the checksum
    //****CHECK THIS - SJL COMMENTED OUT
    sprintf(buffer,"serialchksumoffset=\"");
    increment_chksum(buffer,&serialchksum);
    if(checksum)
        increment_chksum(buffer,&chksum);
    else print(buffer);

    //sprintf(debug_buffer,"before while loop +CHKSUM=%X\r\n",chksum);print(debug_buffer);	//SJL - CAVR2 - debug
    //sprintf(debug_buffer,"serialchksum 3 = %X\r\n",serialchksum);print(debug_buffer);		//SJL - CAVR2 - debug

    #define target 0xDE

    while(1)
    {

        if(serialchksum == target)
            break;
        if((target-'A') >= serialchksum && (target-'Z') <= serialchksum)
        {
            sprintf(buffer,"%c",target-serialchksum);
            increment_chksum(buffer,&serialchksum);
            if(checksum)
                increment_chksum(buffer,&chksum);
            else print(buffer);
            break;
        }
        else
        {
            sprintf(buffer,"F");
            increment_chksum(buffer,&serialchksum);
            if(checksum)
                increment_chksum(buffer,&chksum);
            else print(buffer);

            //sprintf(debug_buffer,"else: chksum = %X (hex) %d (dec)\r\n",chksum,chksum);print(debug_buffer);				//SJL - CAVR2 - debug
    		//sprintf(debug_buffer,"else: serialchksum = %X (hex) %d (dec)\r\n",(serialchksum&=~(0xFF00)),serialchksum);print(debug_buffer);	//SJL - CAVR2 - debug
        }
    }

    sprintf(buffer,"\"\r\n");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    increment_chksum(buffer,&serialchksum);


    sprintf(buffer,"pin=\"");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    if(checksum) increment_chksume(config.pin_code,&chksum); else print_eeprom(config.pin_code);


    tickleRover();
    if (config.public_queries)
        sprintf(buffer,"\"\r\npublic=\"true\"");
    else
        sprintf(buffer,"\"\r\npublic=\"false\"");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

    if (config.forwarding)
        sprintf(buffer,"\r\nfwdmsg=\"true\"");
    else
        sprintf(buffer,"\r\nfwdmsg=\"false\"");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

    if (config.sms_retry)
        sprintf(buffer,"\r\nretry=\"true\"");
    else
        sprintf(buffer,"\r\nretry=\"false\"");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

    sprintf(buffer,"\r\nsitemsg=\"");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    if(checksum) increment_chksume(config.site_name,&chksum); else print_eeprom(config.site_name);

    #ifdef DEBUG
    if(checksum)
    {
        sprintf(buffer,"After sitemsg: %d, %X\r\n",chksum,chksum);	//SJL - CAVR2 - print hex added
        print(buffer);
    }
    #endif

    for (i=0; i < MAX_CONTACTS; i++) {
        temp = contact_read(i);
        if (temp == 0)
        {
                sprintf(buffer,"\"\r\nphone%d=\"",i+1);
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
                sprintf(buffer,"\"\r\nphone%d=\"",i+1);
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
                if(temp[0] == '<')
                    if(checksum) increment_chksume(temp,&chksum); else print_eeprom(temp);
                else
                    if(checksum) increment_chksume(temp+1,&chksum); else print_eeprom(temp+1);//remove type character
        }
    }
    tickleRover();

    #ifdef DEBUG
    if(checksum)
    {
        sprintf(buffer,"After phone numbers: %X\r\n",chksum);
        print(buffer);
        //log_line("system.log",buffer);							//SJL - CAVR2 - debug
    }
    #endif

    sprintf(buffer,"\"\r\n\r\nautoreport time=\"%02d:%02d:%02d",config.autoreport.hour,config.autoreport.minute,
                                                              config.autoreport.second);
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    sprintf(buffer,"\"\r\nautoreport day=\"%d",config.autoreport.day);
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    sprintf(buffer,"\"\r\nautoreport frequency=\"");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    switch(config.autoreport.type)
    {
        case ALARM_DAY:
            sprintf(buffer,"daily");
            break;
        case ALARM_WEEK:
            sprintf(buffer,"weekly");
            break;
        case ALARM_MONTH:
            sprintf(buffer,"monthly");
            break;
        case NO_ALARM:
        default:
            sprintf(buffer,"never");
            break;
    }
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    #ifdef DEBUG
    if(checksum)
    {
        sprintf(buffer,"After auto reporting: %X\r\n",chksum);
        print(buffer);
    }
    #endif
    sprintf(buffer,"\"\r\n\r\nlogsamplerate=\"%lu",config.log_entry_period+1);
	//sprintf(buffer,"\"\r\n\r\nlogsamplerate=\"%lu",config.log_entry_period);
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    sprintf(buffer,"\"\r\nlogupdaterate=\"%d",config.log_update_period+1);
	//sprintf(buffer,"\"\r\nlogupdaterate=\"%d",config.log_update_period);
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    sprintf(buffer,"\"\r\nlogcontact=\"");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    if(checksum) increment_chksume(config.update_address,&chksum); else print_eeprom(config.update_address);

    if(config.alarm_log_enabled)
        sprintf(buffer,"\"\r\nlogalarm=\"enabled");
    else
        sprintf(buffer,"\"\r\nlogalarm=\"disabled");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    if(config.data_log_enabled)
        sprintf(buffer,"\"\r\nlogdata=\"enabled\"\r\n");
    else
        sprintf(buffer,"\"\r\nlogdata=\"disabled\"\r\n");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

    #ifdef DEBUG
    if(checksum)
    {
        sprintf(buffer,"After log data: %X\r\n",chksum);
        print(buffer);
    }
    #endif

    if(config.rssi_log_enabled)
        sprintf(buffer,"logrssi=\"enabled\"\r\n");
    else
        sprintf(buffer,"logrssi=\"disabled\"\r\n");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

    if(config.loc_log_enabled)
        sprintf(buffer,"loglocation=\"enabled\"\r\n");
    else
        sprintf(buffer,"loglocation=\"disabled\"\r\n");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

    if(config.advanced_features)
        sprintf(buffer,"advancedfeatures=\"enabled\"\r\n");
    else
        sprintf(buffer,"advancedfeatures=\"disabled\"\r\n");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

    #ifdef DEBUG
    if(checksum)
    {
        sprintf(buffer,"After advanced features: %X\r\n",chksum);
        print(buffer);
    }
    #endif

	#if EMAIL_AVAILABLE		//initial section changed from TCPIP to EMAIL
    //#if TCPIP_AVAILABLE	//now defunct
       //get then print modem / internet / email settings.
       // printf("ATE0\r\n");
       // modem_read();
       // if(strstrf(modem_rx_string,"ATE0")) modem_read();

        strcpyre(modem_rx_string,config.apn);
        sprintf(buffer,"\r\nconnection apn=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        strcpyre(modem_rx_string,config.dns0);
        sprintf(buffer,"connection primarydns=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        strcpyre(modem_rx_string,config.dns1);
        sprintf(buffer,"connection secondarydns=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

    //#if EMAIL_AVAILABLE
        strcpyre(modem_rx_string,config.smtp_serv);
        sprintf(buffer,"\r\nemail smtpserver=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        sprintf(modem_rx_string,"%d",config.smtp_port);
        sprintf(buffer,"email smtpport=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        strcpyre(modem_rx_string,config.smtp_pw);
        sprintf(buffer,"email smtppassword=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        strcpyre(modem_rx_string,config.smtp_un);
        sprintf(buffer,"email smtpusername=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        //not used but required for successful config download start...
		strcpyre(modem_rx_string,config.pop3_serv);
        sprintf(buffer,"email pop3server=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        sprintf(modem_rx_string,"%d",config.pop3_port);
        sprintf(buffer,"email pop3port=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        strcpyre(modem_rx_string,config.pop3_pw);
        sprintf(buffer,"email pop3password=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        strcpyre(modem_rx_string,config.pop3_un);
        sprintf(buffer,"email pop3username=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
		//...end
		
        strcpyre(modem_rx_string,config.from_address);
        sprintf(buffer,"email fromaddress=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        strcpyre(modem_rx_string,config.email_domain);
        sprintf(buffer,"email emaildomain=\"%s\"\r\n",get_payload(modem_rx_string));
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        sprintf(buffer,"email enablereceipt=\"%p\"\r\n",email_recMail?"true":"false");
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);


    #endif

    //#endif
	
    for (i=0;i<MAX_INPUTS;i++)
    {
        tickleRover();
        sprintf(buffer,"\r\nin%d enabled=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        if (config.input[i].enabled)
        {
            sprintf(buffer,"true\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"false\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        //print out the input type
        sprintf(buffer,"in%d type=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        if (config.input[i].type == DIGITAL)
        {
            sprintf(buffer,"digital\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else if (config.input[i].type == ANALOG_4_20_mA)
        {
            sprintf(buffer,"4-20mA\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else if (config.input[i].type == ANALOG_0_5V)
        {
            sprintf(buffer,"0-5V\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else if (config.input[i].type == PULSE)
        {
            sprintf(buffer,"pulse\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        if(config.input[i].log_type&LOG_INSTANT)
        {
            sprintf(buffer,"in%d loginstant=\"enabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"in%d loginstant=\"disabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        if(config.input[i].log_type&LOG_AVERAGE)
        {
            sprintf(buffer,"in%d logaverage=\"enabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"in%d logaverage=\"disabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        if(config.input[i].log_type&LOG_MIN)
        {
            sprintf(buffer,"in%d logmin=\"enabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"in%d logmin=\"disabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        if(config.input[i].log_type&LOG_MAX)
        {
            sprintf(buffer,"in%d logmax=\"enabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"in%d logmax=\"disabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        if(config.input[i].log_type&LOG_AGGREGATE)
        {
            sprintf(buffer,"in%d logaggregate=\"enabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"in%d logaggregate=\"disabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        if(config.input[i].log_type&LOG_DUTY)
        {
            sprintf(buffer,"in%d logduty=\"enabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"in%d logduty=\"disabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        if(config.input[i].log_type&LOG_ALARM)
        {
            sprintf(buffer,"in%d alarmlog=\"enabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"in%d alarmlog=\"disabled\"\r\n",i+1);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        if(i>=6)
        {
        	#if PULSE_COUNTING_AVAILABLE
            switch(config.pulse[i-6].period)
            {
                case SECONDS:
                    sprintf(buffer,"in%d pulseperiod=\"seconds\"\r\n",i+1);
                    break;
                case MINUTES:
                    sprintf(buffer,"in%d pulseperiod=\"minutes\"\r\n",i+1);
                    break;
                case HOURS:
                    sprintf(buffer,"in%d pulseperiod=\"hours\"\r\n",i+1);
                    break;
            }
            #else
            sprintf(buffer,"in%d pulseperiod=\"seconds\"\r\n",i+1);
            #endif
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            sprintf(buffer,"in%d pulsespercount=\"%.5f\"\r\n",i+1,config.pulse[i-6].pulses_per_count);	//SJL - CAVR2 - limited to 5d.p. to ensure correct checksum
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        sprintf(buffer,"in%d decimal=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        if (config.input[i].dp)
        {
            sprintf(buffer,"true\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"false\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }


        sprintf(buffer, "in%d engmax=\"%.5f\"\r\n",i+1, config.input[i].eng_max);	//SJL - CAVR2 - limited to 5d.p. to ensure correct checksum
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        sprintf(buffer, "in%d engmin=\"%.5f\"\r\n",i+1, config.input[i].eng_min);	//SJL - CAVR2 - limited to 5d.p. to ensure correct checksum
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        sprintf(buffer, "in%d engunit=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        if(checksum) increment_chksume(config.input[i].units,&chksum); else print_eeprom(config.input[i].units);
        sprintf(buffer, "\"\r\n");
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);


        for(k=0;k<INPUT_MAX_ALARMS;k++)
        {
            tickleRover();
            sprintf(buffer,"in%d%c alarmtype=\"",i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            if (config.input[i].alarm[k].type == ALARM_NONE)
            {
                sprintf(buffer,"none\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            }
            else if (config.input[i].alarm[k].type == ALARM_ABOVE)
            {
                sprintf(buffer,"above\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            }
            else
            {
                sprintf(buffer,"below\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            }
                 //print out the alarm message
            sprintf(buffer,"in%d%c alarmmsg=\"",i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            if(checksum) increment_chksume(config.input[i].alarm[k].alarm_msg,&chksum); else print_eeprom(config.input[i].alarm[k].alarm_msg);

            sprintf(buffer,"\"\r\nin%d%c resetmsg=\"",i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            if(checksum) increment_chksume(config.input[i].alarm[k].reset_msg,&chksum); else print_eeprom(config.input[i].alarm[k].reset_msg);

            sprintf(buffer,"\"\r\nin%d%c alarmstart=\"",i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            if (config.input[i].alarm[k].startup_alarm)
            {
                sprintf(buffer,"true\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            }
            else
            {
                sprintf(buffer,"false\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            }

            sprintf(buffer,"in%d%c alarmoutputswitch=\"%d\"\r\n", i+1, k+97, config.input[i].alarm[k].alarmAction.outswitch);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

            sprintf(buffer,"in%d%c alarmoutaction=\"", i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

            if (config.input[i].alarm[k].alarmAction.outaction)
            {
                sprintf(buffer,"on\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            }
            else
            {
                sprintf(buffer,"off\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            }

            sprintf(buffer,"in%d%c alarmoutputcontact=\"",i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            if (config.input[i].alarm[k].alarmAction.me)
                if(checksum) chksum += '1'; else print_char('1');
            else
                if(checksum) chksum += '0'; else print_char('0');

            for (j=0; j<16; j++)
            {
                if ((config.input[i].alarm[k].alarmAction.contact_list >> j) & 0x01)
                    if(checksum) chksum += '1'; else print_char('1');
                else
                    if(checksum) chksum += '0'; else print_char('0');
            }

            sprintf(buffer,"\"\r\nin%d%c resetoutputswitch=\"%d\"\r\n", i+1, k+97, config.input[i].alarm[k].resetAction.outswitch);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

            sprintf(buffer,"in%d%c resetoutaction=\"", i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

            if (config.input[i].alarm[k].resetAction.outaction)
            {
                sprintf(buffer,"on\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            }
            else
            {
                sprintf(buffer,"off\"\r\n");
                if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            }

            sprintf(buffer,"in%d%c resetoutputcontact=\"",i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            if (config.input[i].alarm[k].resetAction.me)
                if(checksum) chksum += '1'; else print_char('1');
            else
                if(checksum) chksum += '0'; else print_char('0');

            for (j=0; j<16; j++)
            {
                if ((config.input[i].alarm[k].resetAction.contact_list >> j) & 0x01)
                    if(checksum) chksum += '1'; else print_char('1');
                else
                    if(checksum) chksum += '0'; else print_char('0');
            }

            sprintf(buffer, "\"\r\nin%d%c alarmlevel=\"%.5f\"\r\n",i+1, k+97, config.input[i].alarm[k].set);	//SJL - CAVR2 - limited to 5d.p. to ensure correct checksum
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

            sprintf(buffer, "in%d%c resetlevel=\"%.5f\"\r\n",i+1, k+97, config.input[i].alarm[k].reset);		//SJL - CAVR2 - limited to 5d.p. to ensure correct checksum
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

            sprintf(buffer,"in%d%c alarmcontact=\"",i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            for (j=0; j<16; j++)
            {
                if ((config.input[i].alarm[k].alarm_contact >> j) & 0x01)
                    if(checksum) chksum += '1'; else print_char('1');
                else
                    if(checksum) chksum += '0'; else print_char('0');
            }

            sprintf(buffer,"\"\r\nin%d%c resetcontact=\"",i+1, k+97);
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
            for (j=0; j<16; j++)
            {
                if ((config.input[i].alarm[k].reset_contact >> j) & 0x01)
                    if(checksum) chksum += '1'; else print_char('1');
                else
                    if(checksum) chksum += '0'; else print_char('0');
            }
            #ifdef DEBOUNCE_FILTER
            sprintf(buffer,"\"\r\nin%d%c debounce=\"%f\"\r\n",i+1, k+97, (double)((config.input[i].alarm[k].debounce_time+DEBOUNCE_FILTER) *0.32));
            #else
            sprintf(buffer,"\"\r\nin%d%c debounce=\"%d.%02d\"\r\n",i+1, k+97, config.input[i].alarm[k].debounce_time / 100, config.input[i].alarm[k].debounce_time % 100);
            #endif
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

          }
          #ifdef DEBUG
            if(checksum)
            {
                sprintf(buffer,"After input %d: %X\r\n",i+1,chksum);
                print(buffer);
            }
          #endif
    }
    tickleRover();
    for (i=0;i<MAX_OUTPUTS;i++)
    {
        tickleRover();
        sprintf(buffer,"\r\nout%d enabled=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        if (config.output[i].enabled)
        {
            sprintf(buffer,"true\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"false\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        //print out the input type
        sprintf(buffer,"out%d name=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        if(checksum) increment_chksume(config.output[i].config.name,&chksum); else print_eeprom(config.output[i].config.name);

        sprintf(buffer,"\"\r\nout%d defstate=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        if (config.output[i].config.default_state==ON)
        {
            sprintf(buffer,"1\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else if (config.output[i].config.default_state==OFF)
        {
            sprintf(buffer,"0\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }
        else
        {
            sprintf(buffer,"2\"\r\n");
            if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        }

        sprintf(buffer,"out%d momentary=\"%d\"\r\n",i+1,config.output[i].config.momentaryLength);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);

        //print out the alarm message
        sprintf(buffer,"out%d onmsg=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        if(checksum) increment_chksume(config.output[i].config.on_msg,&chksum); else print_eeprom(config.output[i].config.on_msg);

        sprintf(buffer,"\"\r\nout%d offmsg=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        if(checksum) increment_chksume(config.output[i].config.off_msg,&chksum); else print_eeprom(config.output[i].config.off_msg);

        sprintf(buffer,"\"\r\nout%d oncontact=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        for (j=0; j<16; j++)
        {
            if ((config.output[i].config.on_contact >> j) & 0x01)
                if(checksum) chksum += '1'; else print_char('1');
            else
                if(checksum) chksum += '0'; else print_char('0');
        }
        sprintf(buffer,"\"\r\nout%d offcontact=\"",i+1);
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        for (j=0; j<16; j++)
        {
            if ((config.output[i].config.off_contact >> j) & 0x01)
                if(checksum) chksum += '1'; else print_char('1');
            else
                if(checksum) chksum += '0'; else print_char('0');
        }
        sprintf(buffer,"\"\r\n");
        if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
        #ifdef DEBUG
            if(checksum)
            {
                sprintf(buffer,"After output %d: %X\r\n",i+1,chksum);
                print(buffer);
            }
        #endif
    }
    sprintf(buffer,"\r\nAT+END\r\n");
    if(checksum) increment_chksum(buffer,&chksum); else print(buffer);
    #ifdef DEBUG
    if(checksum)
    {
        sprintf(buffer,"before negation: %d (dec) %X (hex)\r\n",chksum,chksum);
        print(buffer);
    }
    #endif
    if(checksum)
    {
        sprintf(buffer,"+CHKSUM=%02X\r\n",~chksum);print(buffer);

        //log_line("system.log",buffer);	//SJL - CAVR2 - debug
    }
}

void config_test_com(){
    int i;
    char c;
    for(i = 0; i < 128; i++){
        tickleRover();
        c = 0x20;
        while(c < 0x40){
            sprintf(buffer, "%d%c",i%10,c++);
            print(buffer);
        }
        sprintf(buffer, "<- line %d\r\n", i);
        print(buffer);
    }
    sprintf(buffer, "%c", 6);
    print(buffer);
    return;
}

/*******************************************************************************
 * void config_factoryReset(void)
 *
 * Reset the entire config eeprom
 *
 * Project : 	 SMS300
 * Author: 		 Chris Cook
 * Date :  		 May 2005
 * (c) EDAC Electronics LTD 2005
 ******************************************************************************/
void config_factoryReset(void)
{
    unsigned char i,j;
    verbose = 0;
    config_cmgs = 0;
    uart_echo = 0;

	
    //Clear the site name
    strcpyef(config.site_name , "");
    //Set the retry count to be 1
    config.sms_retry = 0;
    //reset the pin code
    strcpyef(config.pin_code, "");
    //allow public queries
    config.public_queries = false;
    //disable forwarding
    config.forwarding = false;
    //disable psu montitoring
    config.psu_monitoring = false;

    contact_reset();

    config.contact_count = 0;

    //Reset the inputs
    for (i = 0; i < MAX_INPUTS; i++)
    {

        tickleRover();
        sprintf(buffer,"Input %d Alarm A",i+1);
        strcpye(config.input[i].alarm[ALARM_A].alarm_msg, buffer);
        sprintf(buffer,"Input %d Reset A",i+1);
        strcpye(config.input[i].alarm[ALARM_A].reset_msg, buffer);
        sprintf(buffer,"Input %d Alarm B",i+1);
        strcpye(config.input[i].alarm[ALARM_B].alarm_msg, buffer);
        sprintf(buffer,"Input %d Reset B",i+1);
        strcpye(config.input[i].alarm[ALARM_B].reset_msg, buffer);
        tickleRover();
        config.input[i].enabled = DISABLE;
        config.input[i].type = DIGITAL;
        strcpyef(config.input[i].units, "");
        config.input[i].eng_min = 0.0;
        config.input[i].eng_max = 0.0;
        config.input[i].conv_grad = 0.0;
        config.input[i].conv_int = 0.0;
        config.input[i].dp = false;
        config.input[i].log_type = NONE;
        for (j = 0; j < INPUT_MAX_ALARMS; j++)
        {
            config.input[i].alarm[j].type = ALARM_NONE;
            config.input[i].alarm[j].lastZone = ZONE0;
            config.input[i].alarm[j].set = 0.0;
            config.input[i].alarm[j].reset = 0.0;
            config.input[i].alarm[j].thresholdLow = DIGITAL_LOW;
            config.input[i].alarm[j].thresholdHigh = DIGITAL_HIGH;
            config.input[i].alarm[j].alarm_contact = 0x00;
            config.input[i].alarm[j].reset_contact = 0x00;
            config.input[i].alarm[j].debounce_time = 0;
            config.input[i].alarm[j].startup_alarm = false;
            config.input[i].alarm[j].alarmAction.me = false;
            config.input[i].alarm[j].alarmAction.contact_list = 0x00;
            config.input[i].alarm[j].alarmAction.outswitch = 0;
            config.input[i].alarm[j].alarmAction.outaction = on;
            config.input[i].alarm[j].resetAction.me = false;
            config.input[i].alarm[j].resetAction.contact_list = 0x00;
            config.input[i].alarm[j].resetAction.outswitch = 0;
            config.input[i].alarm[j].resetAction.outaction = on;
        }
        config.input[i].sensor.thresholdLow = OUT_OF_BOUNDS;
        config.input[i].sensor.thresholdHigh = OUT_OF_BOUNDS;
    }
    tickleRover();
    //Now configure the outputs
    //Output 1
    strcpyef(config.output[0].config.name, "Output 1");
    strcpyef(config.output[0].config.on_msg, "Output 1 On");
    strcpyef(config.output[0].config.off_msg, "Output 1 Off");
    //Output 2
    strcpyef(config.output[1].config.name, "Output 2");
    strcpyef(config.output[1].config.on_msg, "Output 2 On");
    strcpyef(config.output[1].config.off_msg, "Output 2 Off");
    //Output 3
    strcpyef(config.output[2].config.name, "Output 3");
    strcpyef(config.output[2].config.on_msg, "Output 3 On");
    strcpyef(config.output[2].config.off_msg, "Output 3 Off");
    //Output 4
    strcpyef(config.output[3].config.name, "Output 4");
    strcpyef(config.output[3].config.on_msg, "Output 4 On");
    strcpyef(config.output[3].config.off_msg, "Output 4 Off");
    tickleRover();

    for (i = 0; i < MAX_OUTPUTS; i++)
    {
        tickleRover();
        sprintf(buffer,"Output %d",i+1);
        strcpye(config.output[i].config.name,buffer);
        sprintf(buffer,"Output %d on",i+1);
        strcpye(config.output[i].config.on_msg,buffer);
        sprintf(buffer,"Output %d off",i+1);
        strcpye(config.output[i].config.off_msg,buffer);

        config.output[i].enabled = false;
        output_switch(i,OFF);
        config.output[i].state = off;
        config.output[i].config.default_state = off;
        config.output[i].config.on_contact = 0x00;
        config.output[i].config.off_contact = 0x00;
    }
    email_recMail = false;

	//config.pop3_serv[0] = 0;
	//config.pop3_un[0] = 0;
	//config.pop3_pw[0] = 0;
	config.smtp_serv[0] = 0;
	config.smtp_un[0] = 0;
	config.smtp_pw[0] = 0;

	config.from_address[0] = 0;
	config.email_domain[0] = 0;

	config.dns0[0] = 0;
	config.dns1[0] = 0;
	config.apn[0] = 0;

	config.update_address[0] = '\0';
    config.log_entry_period = -1;
    config.log_update_period = -1;
    config.data_log_enabled = false;
    config.alarm_log_enabled = false;
	
	//SJL - disable input 1 on factory reset
	//config.input[0].enabled = ENABLE;
	//config.input[0].alarm[0].type = ALARM_CLOSED;
    
	config.rssi_log_enabled = false;
    config.loc_log_enabled = false;
    config.advanced_features = false;
	
	email_retry_period = -1;	

    sprintf(buffer,"OK\r\n");print(buffer);
    return;
}
