/* global.h and mmc.h required whether logging is available or not
   for  mmc_clear_data() */
#include "global.h"
#include "drivers\mmc.h"

/* the rest of the code is only built if logging is available on the particular unit */
#if LOGGING_AVAILABLE
	#include <stdlib.h>
    #include <delay.h>
 	#include <math.h>

    #include "drivers\input.h"
	#include "options.h"

    #include "flash\sd_cmd.h"		//SJL - CAVR2 - added, used to be included through options.h
	#include "flash\file_sys.h"		//SJL - CAVR2 - added, used to be included through options.h

	#include "drivers\event.h"
    #include "drivers\config.h"
	#include "drivers\sms.h"
	#include "drivers\modem.h"
	#include "drivers\queue.h"		//SJL - CAVR2 - added
	#include "drivers\ds1337.h"    	//SJL - CAVR2 - added
	#include "drivers\uart.h"       //SJL - CAVR2 - added
	#include "drivers\str.h"		//SJL - CAVR2 - added
	#include "drivers\debug.h"	   	//SJL - CAVR2 - added
    #include "drivers\contact.h"	//SJL - CAVR2 - added for email_send to multiple recipients

	#if GPS_AVAILABLE
		#include "drivers\gps.h"
	#endif

 	#define READY
    #define LOG_FILE_NAME "log.csv"

    #ifndef TRUE
 		#define TRUE 1
 	#endif

    #ifndef FALSE
 		#define FALSE 0
 	#endif
	
	#if CLOCK == SIX_MEG
	#define RATE 5859.0
	#elif CLOCK == SEVEN_SOMETHING_MEG
	#define RATE 7200.0
	#endif

//enum { EOT = '|', ACK = '+', NAK = '-' };

float input_running_total[MAX_INPUTS];
unsigned long input_samples[MAX_INPUTS];
bit mmc_fail_sent=false;

char dates[50];

/* Initialize the SD Card ***************************************************************
Function uses the flash file driver function initialize_media() to initialize the SD card
SD card not initialized successfully - Power LED flashes
SD card initialized ok - Power LED solid
************************************************************************************** */
void mmc_init()
{
    if(!initialize_media())
        power_led_flashing();
    else
    {
        power_led_on();
    }
}

/* char mmc_gets(char *buffer, int count, FILE *fptr) //unused function
{
    int i;
    char c;
    if(fptr==NULL || feof(fptr))
        return false;
    for(i=0;i<count-1;i++)
    {
        if(feof(fptr))
            break;
        c = fgetc(fptr);
        if(c=='\n')
        {
            buffer[i] = '\0';
            return true;
        }
        buffer[i] = c;
    }
    buffer[i] = '\0';
    return true;
}
*/

/* void log_entry() //all log entries done via latest_log_entry() - commented out 3.xx.7
{
	//int close_result;
	char *c;
    FILE *fptr = NULL;
    Event e;
    char year,month,day,hour,minute,second,i,j,rssi;
    float f, g;
    unsigned long l, local_input_samples, local_pulse_aggregate;
	float local_input_running_total;
    bool data_logging_enabled;	
	
	//
	//sbi(PORTG,0x02);
	//	
	
	data_logging_enabled = false;

	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[log_entry] Making a log entry to log.csv...\r\n");print1(buffer);
	#endif
	
    for(i=0;i<MAX_INPUTS;i++)
        data_logging_enabled |= (config.input[i].enabled && config.input[i].log_type);

	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif

    //  SJL - CAVR2
    //	both data_logging_enabled and config.loc_log_enabled = 0
    //  this code has been removed so you can log RSSI only if required (no input data)
    //if(!(data_logging_enabled||config.loc_log_enabled))return;	//if both are false
    //DEBUG_printStr("Attempting to initialize SD card...\r\n");
    
	//if(!(data_logging_enabled||config.loc_log_enabled||config.rssi_log_enabled))
	//	return;	//if all are false
    //DEBUG_printStr("Attempting to initialize SD card...\r\n");

    if(!initialize_media())
    {
    	//DEBUG_printStr("SD card failed to initialize\r\n");
        e.type = sms_MMC_ACCESS_FAIL;
        queue_push(&q_modem,&e);
        return;
    }
	//#ifdef SD_CARD_DEBUG
	//sprintf(buffer,"[log_entry] SD card initialized ok\r\n");print1(buffer);
	//#endif
	
	rssi = modem_get_rssi();
	
	SREG &= ~(0x80);
	tickleRover();
	sedateRover();
    fptr = fopenc("log.csv",APPEND);   //we need to guard this with sedate / wakeRover for when the
                                       //file is real big, it can take quite a while to return
	wakeRover();
	SREG |= 0x80;
	
    if(fptr==NULL)
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[log_entry] Could not open log.csv,creating file...\r\n");print1(buffer);
		#endif
		
		SREG &= ~(0x80);
        fptr = fcreatec("log.csv",0);
		SREG |= 0x80;
        if(fptr==NULL)
        {
			#ifdef SD_CARD_DEBUG
			sprintf(buffer,"[log_entry] Failed to create log.csv\r\n");print1(buffer);
			#endif
            power_led_flashing();
            e.type = sms_MMC_ACCESS_FAIL;
            queue_push(&q_modem,&e);
            return;
        }
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[log_entry] log.csv created successfully\r\n");print1(buffer);
		#endif
		
		SREG &= ~(0x80);
        fprintf(fptr,"Date / Time");
		SREG |= 0x80;
        for(i=0;i<MAX_INPUTS;i++)
        {
            if(!config.input[i].enabled)
                continue;
            if(config.input[i].log_type&LOG_INSTANT)
            {
                if(config.input[i].type==DIGITAL)
				{
                    SREG &= ~(0x80);
					fprintf(fptr,",Input %d Instant",i+1);
					SREG |= 0x80;
				}
                else
                {
                    SREG &= ~(0x80);
					fprintf(fptr,",Input %d Instant (",i+1);
					SREG |= 0x80;
                    for(j=0;config.input[i].units[j]!='\0';j++)
					{	
						SREG &= ~(0x80);
                        fputc(config.input[i].units[j],fptr);
						SREG |= 0x80;
					}
                    #if PULSE_COUNTING_AVAILABLE
					if(config.input[i].type == PULSE)
                    {
                        switch(config.pulse[i-6].period)
                        {
							case SECONDS:
								SREG &= ~(0x80);
								fprintf(fptr,"/s");
								SREG |= 0x80;
								break;
							case MINUTES:
								SREG &= ~(0x80);
								fprintf(fptr,"/m");
								SREG |= 0x80;
								break;
							case HOURS:
								SREG &= ~(0x80);
								fprintf(fptr,"/h");
								SREG |= 0x80;
								break;
                        }
						SREG &= ~(0x80);
						fputc(')',fptr);
						SREG |= 0x80;
						
						//SJL added log heading for instantaneous frequency error
						SREG &= ~(0x80);
						fprintf(fptr,",Input %d Instant Error +/- (",i+1);
						SREG |= 0x80;
						for(j=0;config.input[i].units[j]!='\0';j++)
						{	
							SREG &= ~(0x80);
							fputc(config.input[i].units[j],fptr);
							SREG |= 0x80;
						}
						switch(config.pulse[i-6].period)
                        {
							case SECONDS:
								SREG &= ~(0x80);
								fprintf(fptr,"/s");
								SREG |= 0x80;
								break;
							case MINUTES:
								SREG &= ~(0x80);
								fprintf(fptr,"/m");
								SREG |= 0x80;
								break;
							case HOURS:
								SREG &= ~(0x80);
								fprintf(fptr,"/h");
								SREG |= 0x80;
								break;
                        }
						//end of instantaneous frequency error heading
						
                    }
					#endif
					SREG &= ~(0x80);
                    fputc(')',fptr);
					SREG |= 0x80;
                }
            }   //add stuff for average / min / max here. Min / Max analog only
            if(config.input[i].log_type&LOG_MIN)
            {
				if(config.input[i].type==DIGITAL)
				{
					SREG &= ~(0x80);
					fprintf(fptr,",Input %d Minimum",i+1);
					SREG |= 0x80;
				}
				else
				{
					SREG &= ~(0x80);
					fprintf(fptr,",Input %d Minimum (",i+1);
					SREG |= 0x80;
					for(j=0;config.input[i].units[j]!='\0';j++)
					{
						SREG &= ~(0x80);
						fputc(config.input[i].units[j],fptr);
						SREG |= 0x80;
					}
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[i].type == PULSE)
					{
                        switch(config.pulse[i-6].period)
                        {
                            case SECONDS:
								SREG &= ~(0x80);
                                fprintf(fptr,"/s");
								SREG |= 0x80;
                                break;
                            case MINUTES:
                                SREG &= ~(0x80);
								fprintf(fptr,"/m");
								SREG |= 0x80;
                                break;
                            case HOURS:
								SREG &= ~(0x80);
                                fprintf(fptr,"/h");
								SREG |= 0x80;
                                break;
                        }
                    }
					#endif
					SREG &= ~(0x80);
					fputc(')',fptr);
					SREG |= 0x80;
				}
            }
            if(config.input[i].log_type&LOG_MAX)
            {
				if(config.input[i].type==DIGITAL)
				{
					SREG &= ~(0x80);
					fprintf(fptr,",Input %d Maximum",i+1);
					SREG |= 0x80;
				}
				else
				{
					SREG &= ~(0x80);
					fprintf(fptr,",Input %d Maximum (",i+1);
					SREG |= 0x80;
					for(j=0;config.input[i].units[j]!='\0';j++)
					{
						SREG &= ~(0x80);
						fputc(config.input[i].units[j],fptr);
						SREG |= 0x80;
					}
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[i].type == PULSE)
                    {
                        switch(config.pulse[i-6].period)
                        {
                            case SECONDS:
								SREG &= ~(0x80);
                                fprintf(fptr,"/s");
								SREG |= 0x80;
                                break;
                            case MINUTES:
								SREG &= ~(0x80);
                                fprintf(fptr,"/m");
								SREG |= 0x80;
                                break;
                            case HOURS:
								SREG &= ~(0x80);
                                fprintf(fptr,"/h");
								SREG |= 0x80;
                                break;
                        }
                    }
					#endif
					SREG &= ~(0x80);
					fputc(')',fptr);
					SREG |= 0x80;
				}
            }
            if(config.input[i].log_type&LOG_AVERAGE)
            {
				if(config.input[i].type==DIGITAL)
				{
					SREG &= ~(0x80);
					fprintf(fptr,",Input %d Log Period Duty (%%)",i+1);
					SREG |= 0x80;
				}
				else
				{
					SREG &= ~(0x80);
					fprintf(fptr,",Input %d Average (",i+1);
					SREG |= 0x80;
					for(j=0;config.input[i].units[j]!='\0';j++)
					{
						SREG &= ~(0x80);
						fputc(config.input[i].units[j],fptr);
						SREG |= 0x80;
					}
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[i].type == PULSE)
                    {
                        switch(config.pulse[i-6].period)
                        {
                            case SECONDS:
								SREG &= ~(0x80);
                                fprintf(fptr,"/s");
								SREG |= 0x80;
                                break;
                            case MINUTES:
								SREG &= ~(0x80);
								fprintf(fptr,"/m");
								SREG |= 0x80;
                                break;
                            case HOURS:
								SREG &= ~(0x80);
                                fprintf(fptr,"/h");
								SREG |= 0x80;
                                break;
                        }						
						SREG &= ~(0x80);
						fputc(')',fptr);
						SREG |= 0x80;
						
						//SJL added log heading for average frequency error
						SREG &= ~(0x80);
						fprintf(fptr,",Input %d Average Error +/- (",i+1);
						SREG |= 0x80;
						for(j=0;config.input[i].units[j]!='\0';j++)
						{	
							SREG &= ~(0x80);
							fputc(config.input[i].units[j],fptr);
							SREG |= 0x80;
						}
						switch(config.pulse[i-6].period)
                        {
							case SECONDS:
								SREG &= ~(0x80);
								fprintf(fptr,"/s");
								SREG |= 0x80;
								break;
							case MINUTES:
								SREG &= ~(0x80);
								fprintf(fptr,"/m");
								SREG |= 0x80;
								break;
							case HOURS:
								SREG &= ~(0x80);
								fprintf(fptr,"/h");
								SREG |= 0x80;
								break;
                        }
						//end of instantaneous average error heading
                    }
					#endif
					SREG &= ~(0x80);
					fputc(')',fptr);
					SREG |= 0x80;
				}
            }
            if(config.input[i].log_type&LOG_DUTY)
            {
				if(config.input[i].type==DIGITAL)
				{
					SREG &= ~(0x80);
					fprintf(fptr,",Input %d Current Duty (%%)",i+1);
					SREG |= 0x80;
				}					
            }

            if(config.input[i].log_type&LOG_AGGREGATE)
            {
				if(config.input[i].type==DIGITAL)
				{
					SREG &= ~(0x80);
					fprintf(fptr,",Input %d Aggregate",i+1);
					SREG |= 0x80;
				}
				else
				{
					SREG &= ~(0x80);
					fprintf(fptr,",Input %d Aggregate (",i+1);
					SREG |= 0x80;
					//#if PULSE_COUNTING_AVAILABLE
					if(config.input[i].type == PULSE)
					{
						for(j=0;config.input[i].units[j]!='\0';j++)
						{
							SREG &= ~(0x80);
							fputc(config.input[i].units[j],fptr);
							SREG |= 0x80;
						}
					}
					//#endif
					else
					{
                        for(j=0;config.input[i].units[j]!='\0'&&config.input[i].units[j]!='/';j++)
						{
							SREG &= ~(0x80);
                            fputc(config.input[i].units[j],fptr);
							SREG |= 0x80;
						}
					}
					SREG &= ~(0x80);
					fputc(')',fptr);
					SREG |= 0x80;
				}
            }
        }
		#if GPS_AVAILABLE
        if(config.loc_log_enabled)
		{
			SREG &= ~(0x80);
            fprintf(fptr,",latitude,longitude");
			SREG |= 0x80;
		}
		#endif
        if(config.rssi_log_enabled)
        {
			//#ifdef SD_CARD_DEBUG
            //sprintf(buffer,"Adding RSSI heading to log.csv\r\n");
			//print1(buffer);
			//#endif
			SREG &= ~(0x80);
			//fprintf(fptr,",RSSI");
			fprintf(fptr,",RSSI,Band");
			//fprintf(fptr,",RSSI,Band,Network Registration Status");
			SREG |= 0x80;
        }
		SREG &= ~(0x80);
        fprintf(fptr,"\r\n");
		SREG |= 0x80;
    }
	
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[log_entry] log.csv openned successfully\r\n");print1(buffer);
	#endif
    mmc_fail_sent=false; //we've successfully opened the file
	
    //DEBUG_printStr("Getting time and date\r\n");
    rtc_get_date(&day,&month,&year);
    rtc_get_time(&hour,&minute,&second);
    //DEBUG_printStr("Printing time and date to file\r\n");
    
	//SREG &= ~(0x80);	//guard file operation against interrupt
	//sprintf(buffer,"[log_entry:fprintf] Interrupts disabled [%X]",SREG);print1(buffer);	
	SREG &= ~(0x80);
	fprintf(fptr,"%d/%02d/%02d %02d:%02d:%02d",year+2000,month,day,hour,minute,second);
	SREG |= 0x80;
	//sprintf(buffer,"after: [%X]\r\n",SREG);print1(buffer);
	
    for(i=0;i<MAX_INPUTS;i++)
    {
        if(config.input[i].enabled == DISABLE)
            continue;
        if(config.input[i].log_type&LOG_INSTANT)
        {
            if(config.input[i].type==DIGITAL)
            {
               if((input[i].alarm[ALARM_A].buffer & LATEST_SAMPLE) == ZONE2)
               {
                    switch(config.input[i].alarm[0].type)
                    {
                        case ALARM_OPEN:
                            strcpyre(buffer,config.input[i].alarm[0].alarm_msg);
							SREG &= ~(0x80);
                            fprintf(fptr,",%s",buffer);
							SREG |= 0x80;
                            break;
                        case ALARM_CLOSED:
                            strcpyre(buffer,config.input[i].alarm[0].reset_msg);
                            SREG &= ~(0x80);
							fprintf(fptr,",%s",buffer);
							SREG |= 0x80;
                            break;
                        default:
							SREG &= ~(0x80);
                            fprintf(fptr,",1");
							SREG |= 0x80;
                            break;
                    }
               }
               else
               {
                    switch(config.input[i].alarm[0].type)
                    {
                        case ALARM_CLOSED:
                            strcpyre(buffer,config.input[i].alarm[0].alarm_msg);
							SREG &= ~(0x80);
                            fprintf(fptr,",%s",buffer);
							SREG |= 0x80;
                            break;
                        case ALARM_OPEN:
                            strcpyre(buffer,config.input[i].alarm[0].reset_msg);
							SREG &= ~(0x80);
                            fprintf(fptr,",%s",buffer);
							SREG |= 0x80;
                            break;
                        default:
							SREG &= ~(0x80);
                            fprintf(fptr,",0");
							SREG |= 0x80;
                            break;
                    }
               }
            }
            else
			{
				f = input_getVal(i);
				
				SREG &= ~(0x80);
                //fprintf(fptr,",%3.2f", input_getVal(i));
				fprintf(fptr,",%f",f);
				SREG |= 0x80;
				
				#if PULSE_COUNTING_AVAILABLE
				if(config.input[i].type == PULSE)
				{
					switch(config.pulse[i-6].period)
					{
						case HOURS:							
							g = f - ( ( f * (RATE*3600) ) / ( f + (RATE*3600) ) );
							break;
						case MINUTES:
							g = f - ( ( f * (RATE*60) ) / ( f + (RATE*60) ) );
							break;
						case SECONDS:
						default:
							g = f - ( ( f * RATE ) / ( f + RATE ) );
							break;
					}				
					//g = f - ( ( f * RATE ) / ( f + RATE ) );
					
					SREG &= ~(0x80);
					fprintf(fptr,",%f",g);
					SREG |= 0x80;
				}
				#endif						
			}
        }
        if(config.input[i].log_type&LOG_MIN)
        {
            if(config.input[i].type==DIGITAL)
			{
				SREG &= ~(0x80);
                fprintf(fptr,",%d",input_min[i]);
				SREG |= 0x80;
			}
            else
			{
				SREG &= ~(0x80);
                fprintf(fptr,",%f", config.input[i].conv_int+config.input[i].conv_grad*input_min[i]);
				SREG |= 0x80;
			}
        }
        if(config.input[i].log_type&LOG_MAX)
        {
            if(config.input[i].type==DIGITAL)
			{
				SREG &= ~(0x80);
                fprintf(fptr,",%d",input_max[i]);
				SREG |= 0x80;
			}
            else
			{
				SREG &= ~(0x80);
                fprintf(fptr,",%f", config.input[i].conv_int+config.input[i].conv_grad*input_max[i]);
				SREG |= 0x80;
			}
        }
		
		SREG &= ~(0x80);
		local_input_samples = input_samples[i];
		input_samples[i] = 0x00;
		local_input_running_total = input_running_total[i];
		input_running_total[i] = 0x00;		
		#if PULSE_COUNTING_AVAILABLE
		if(config.input[i].type == PULSE)
		{
			local_pulse_aggregate = pulse_aggregate[i-6];
			pulse_aggregate[i-6] = 0x00;
		}
		#endif
		SREG |= 0x80;
		
        if(config.input[i].log_type&LOG_AVERAGE)
        {
            if(config.input[i].type != PULSE)
            {
                //f = input_running_total[i];
                //l = input_samples[i];
                //f = f/l;
                
				f = local_input_running_total / local_input_samples;
				
				if(config.input[i].type==DIGITAL)
                {
					SREG &= ~(0x80);
                    fprintf(fptr,",%f",f*100);
					SREG |= 0x80;
                }
                else
                {
					SREG &= ~(0x80);
                    fprintf(fptr,",%f",f);
					SREG |= 0x80;
                }
            }
			#if PULSE_COUNTING_AVAILABLE
            else //PULSE, figure average from the aggregate
            {
                // f = pulse_aggregate[i-6];
                // l = input_samples[i];
                // f = f / l;
                // f /= 0.320; //get the average per second
				
				f = local_pulse_aggregate / (0.320 * (local_input_samples));
				
                //average pulses per 320ms chunk
                switch(config.pulse[i-6].period)
                {
                    case HOURS:
                        f *= 3600;
						break;
                    case MINUTES:
                        f *= 60;
						break;
                    case SECONDS:
                    default:
                        break;
                }
				SREG &= ~(0x80);
                fprintf(fptr,",%f",f);	//fprintf(fptr,",%3.3f",f);
				SREG |= 0x80;
				
				f = local_pulse_aggregate / (0.320 * (local_input_samples)) - local_pulse_aggregate / (0.320 * (local_input_samples + 1));
				switch(config.pulse[i-6].period)
                {
                    case HOURS:
                        f *= 3600;
						break;
                    case MINUTES:
                        f *= 60;
						break;
                    case SECONDS:
                    default:
                        break;
                }
				SREG &= ~(0x80);
                fprintf(fptr,",%f",f);		//fprintf(fptr,",%3.3f",f);
				SREG |= 0x80;					
            }
			#endif
        }

        if(config.input[i].log_type&LOG_DUTY)
        {
            if(config.input[i].type==DIGITAL)
            {
                f = last_duty_high[i];
                l = last_duty_count[i];
                f = f/l;
				SREG &= ~(0x80);
                fprintf(fptr,",%f",f*100);
				SREG |= 0x80;
            }
        }
        if(config.input[i].log_type&LOG_AGGREGATE)
        {
            if(config.input[i].type == PULSE)
            {
                if(i>=6)
                    //sprintf(buffer,",%f",config.input[i].conv_grad*pulse_aggregate[i-6]);
					f = config.input[i].conv_grad*local_pulse_aggregate;
					sprintf(buffer,",%f",f);
            }
            else
            {
                strcpyre(buffer,config.input[i].units);
                
				if(strstrf(buffer,"/h")!=0)
                    local_input_running_total /= 3600;
                
				else if(strstrf(buffer,"/m")!=0)
                    local_input_running_total /= 60;
                
				sprintf(buffer,",%f", local_input_running_total*.320); //seconds for each sample
            }
			SREG &= ~(0x80);
            fprintf(fptr,"%s",buffer);
			SREG |= 0x80;
        }
        input_max[i] = 0x00;
        input_min[i] = 0xFFFFFFFF;
        //input_samples[i] = 0x00;
        // #if PULSE_COUNTING_AVAILABLE
        // if(i>=6)
            // pulse_aggregate[i-6] = 0x00;
        // #endif
		//input_running_total[i] = 0x00;
    }

    #if GPS_AVAILABLE
    if(config.loc_log_enabled)
    {
        gps_print(buffer,DEGREES);
		SREG &= ~(0x80);
        fprintf(fptr,",%s",buffer);
		SREG |= 0x80;
    }
    #endif

    if(config.rssi_log_enabled)
    {
    	//DEBUG_printStr("Getting RSSI and printing to file\r\n");
        //fprintf(fptr,",%d",modem_get_rssi());
		//SREG &= ~(0x80);	//guard file operation against interrupt
		//sprintf(buffer,"[log_entry:fprintf] Interrupts disabled [%X]",SREG);print1(buffer);	
		SREG &= ~(0x80);
		fprintf(fptr,",%d",rssi);
		SREG |= 0x80;
		
		// Band 
		modem_clear_channel();
		sprintf(buffer,"AT!GETBAND?\r\n");
		print0(buffer);
		modem_wait_for(MSG_GETBAND | MSG_ERROR);		
		c = strchr(modem_rx_string,':')+2;		
		SREG &= ~(0x80);
		fprintf(fptr,",%s",c);
		SREG |= 0x80;
		
		// Network registration status 
		//modem_clear_channel();
		//sprintf(buffer,"AT+CREG?\r\n");
		//print0(buffer);		
		//modem_wait_for(MSG_CREG | MSG_ERROR);		
		//c = strchr(modem_rx_string,',')+1;		
		//SREG &= ~(0x80);
		//fprintf(fptr,",%s",c);
		//SREG |= 0x80;
			
		//sprintf(buffer,"after: [%X]\r\n",SREG);print1(buffer);
    }

    SREG &= ~(0x80);
	fprintf(fptr,"\r\n");
	SREG |= 0x80;
	
    //#ifdef SD_CARD_DEBUG
	//sprintf(buffer,"[log_entry] Closing log file\r\n");print1(buffer);
	//#endif
	
	//pulse 1 detected - the compare register is set
		
	SREG &= ~(0x80);	//guard file operation against interrupt	
    //close_result=fclose(fptr);
	fclose(fptr);
	SREG |= 0x80;
	//#ifdef SD_CARD_DEBUG
    //sprintf(buffer,"Data added\r\n");print1(buffer);
	//sprintf(buffer,"[log_entry] File closed = %d\r\n",close_result);print1(buffer);
	//#endif
	
	//pulses missed during file close operation
	
	//pulse x detected - pulse period caluclated wrongly because pulses have been missed
	//results in lower frequency measured than expected - this is avoided using the 350ms delay
		
	//fptr = NULL;	
	
	#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		delay_ms(350);	//delay skips the next input scan (all inputs)
						//ensures that the pulse period is valid
		pulse_instant_pause = 0;
	}
	#endif
	
	//
	//cbi(PORTG,0x02);
	//
		
    return;
}
*/

/* Perform a log update *****************************************************************
Logs to the log.csv (315) and latest.csv files (320 + 321)
Files are created if they don't exist on the SD card - column headings are added when a
new file is created.
log.csv - log enties for all time
latest.csv - log entries since last successful email update. latest.csv is deleted from
the SD card after a successful log update in function email_latest()

log_entry consolidated into latest_log_entry in 3.xx.8 - see
FW Revision History-Consolidated Code-315,320,321 NG.doc
************************************************************************************** */
void latest_log_entry() //added 3.xx.7 - modified 3.xx.8: write to latest.csv AND log.csv
{
	//int close_result;
	char *c;
    FILE *fptr = NULL;
    Event e;
    char year,month,day,hour,minute,second,i,j,k,rssi;
    float f, g;
    unsigned long l;
	
	//LOCAL LOG VARIABLES
	//minimum
	float local_min[MAX_INPUTS];
	//maximum
	float local_max[MAX_INPUTS];
	//average
	float local_input_running_total[MAX_INPUTS];
	unsigned long local_input_samples[MAX_INPUTS];
	//pulse
	unsigned long local_pulse_aggregate[2];
	//duty cycle - dont done yet - required?
	
	//pause the pulse inputs
	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif
	
	// *************************************************************************************************
	// Set Local Variables
	// *************************************************************************************************
	for(i=0;i<MAX_INPUTS;i++)
    {
		if(config.input[i].enabled == DISABLE)
            continue;
		if(config.input[i].log_type&LOG_MIN)
		{
			local_min[i] = input_min[i];
			input_min[i] = 0xFFFFFFFF;
		}
        if(config.input[i].log_type&LOG_MAX)
		{
			local_max[i] = input_max[i];
			input_max[i] = 0x00;
		}
		if(config.input[i].log_type&LOG_AVERAGE || config.input[i].log_type&LOG_AGGREGATE)
        {
			local_input_running_total[i] = input_running_total[i];
			input_running_total[i] = 0x00;
		}
		if(config.input[i].log_type&LOG_AVERAGE)
		{
			local_input_samples[i] = input_samples[i];
			input_samples[i] = 0x00;
		}		
        #if PULSE_COUNTING_AVAILABLE
		if(config.input[i].type == PULSE)
		{
			local_pulse_aggregate[i-6] = pulse_aggregate[i-6];
			pulse_aggregate[i-6] = 0x00;
		}
		#endif
	}
	//Query the modem RSSI
	rssi = modem_get_rssi();	

	#if EMAIL_AVAILABLE
	k=0;	//save to log.csv AND latest.csv
	#else
	k=1;	//save to log.csv ONLY
	#endif
	
	do //for(k=0;k<2;k++)
	{
		if(!initialize_media())
		{
			//DEBUG_printStr("SD card failed to initialize\r\n");
			e.type = sms_MMC_ACCESS_FAIL;
			queue_push(&q_modem,&e);
			return;
		}
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[latest_log_entry] SD card initialized ok\r\n");print1(buffer);
		#endif
		
		if(k==1)
		{
			// *************************************************************************************************
			// LOG.CSV
			// *************************************************************************************************
			#ifdef SD_CARD_DEBUG
			sprintf(buffer,"[latest_log_entry] Making a log entry to log.csv...\r\n");print1(buffer);
			#endif			
			
			SREG &= ~(0x80);
			tickleRover();
			sedateRover();
			fptr = fopenc("log.csv",APPEND);   //we need to guard this with sedate / wakeRover for when the
											   //file is real big, it can take quite a while to return
			wakeRover();
			SREG |= 0x80;
			
			if(fptr==NULL)
			{
				#ifdef SD_CARD_DEBUG
				sprintf(buffer,"[latest_log_entry] Could not open log.csv,creating file...\r\n");print1(buffer);
				#endif
				
				SREG &= ~(0x80);
				fptr = fcreatec("log.csv",0);
				SREG |= 0x80;
				if(fptr==NULL)
				{
					#ifdef SD_CARD_DEBUG
					sprintf(buffer,"[latest_log_entry] Failed to create log.csv\r\n");print1(buffer);
					#endif
					power_led_flashing();
					e.type = sms_MMC_ACCESS_FAIL;
					queue_push(&q_modem,&e);
					return;
				}
				#ifdef SD_CARD_DEBUG
				sprintf(buffer,"[latest_log_entry] log.csv created successfully\r\n");print1(buffer);
				#endif
				
				SREG &= ~(0x80);
				fprintf(fptr,"Date / Time");
				SREG |= 0x80;
				for(i=0;i<MAX_INPUTS;i++)
				{
					if(!config.input[i].enabled)
						continue;
					if(config.input[i].log_type&LOG_INSTANT)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Instant",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Instant (",i+1);
							SREG |= 0x80;
							for(j=0;config.input[i].units[j]!='\0';j++)
							{	
								SREG &= ~(0x80);
								fputc(config.input[i].units[j],fptr);
								SREG |= 0x80;
							}
							#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
								SREG &= ~(0x80);
								fputc(')',fptr);
								SREG |= 0x80;
								
								//SJL added log heading for instantaneous frequency error
								SREG &= ~(0x80);
								fprintf(fptr,",Input %d Instant Error +/- (",i+1);
								SREG |= 0x80;
								for(j=0;config.input[i].units[j]!='\0';j++)
								{	
									SREG &= ~(0x80);
									fputc(config.input[i].units[j],fptr);
									SREG |= 0x80;
								}
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
								//end of instantaneous frequency error heading
								
							}
							#endif
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}   //add stuff for average / min / max here. Min / Max analog only
					if(config.input[i].log_type&LOG_MIN)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Minimum",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Minimum (",i+1);
							SREG |= 0x80;
							for(j=0;config.input[i].units[j]!='\0';j++)
							{
								SREG &= ~(0x80);
								fputc(config.input[i].units[j],fptr);
								SREG |= 0x80;
							}
							#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
							}
							#endif
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}
					if(config.input[i].log_type&LOG_MAX)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Maximum",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Maximum (",i+1);
							SREG |= 0x80;
							for(j=0;config.input[i].units[j]!='\0';j++)
							{
								SREG &= ~(0x80);
								fputc(config.input[i].units[j],fptr);
								SREG |= 0x80;
							}
							#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
							}
							#endif
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}
					if(config.input[i].log_type&LOG_AVERAGE)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Log Period Duty (%%)",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Average (",i+1);
							SREG |= 0x80;
							for(j=0;config.input[i].units[j]!='\0';j++)
							{
								SREG &= ~(0x80);
								fputc(config.input[i].units[j],fptr);
								SREG |= 0x80;
							}
							#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}						
								SREG &= ~(0x80);
								fputc(')',fptr);
								SREG |= 0x80;
								
								//SJL added log heading for average frequency error
								SREG &= ~(0x80);
								fprintf(fptr,",Input %d Average Error +/- (",i+1);
								SREG |= 0x80;
								for(j=0;config.input[i].units[j]!='\0';j++)
								{	
									SREG &= ~(0x80);
									fputc(config.input[i].units[j],fptr);
									SREG |= 0x80;
								}
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
								//end of instantaneous average error heading
							}
							#endif
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}
					if(config.input[i].log_type&LOG_DUTY)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Current Duty (%%)",i+1);
							SREG |= 0x80;
						}					
					}

					if(config.input[i].log_type&LOG_AGGREGATE)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Aggregate",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Aggregate (",i+1);
							SREG |= 0x80;
							//#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								for(j=0;config.input[i].units[j]!='\0';j++)
								{
									SREG &= ~(0x80);
									fputc(config.input[i].units[j],fptr);
									SREG |= 0x80;
								}
							}
							//#endif
							else
							{
								for(j=0;config.input[i].units[j]!='\0'&&config.input[i].units[j]!='/';j++)
								{
									SREG &= ~(0x80);
									fputc(config.input[i].units[j],fptr);
									SREG |= 0x80;
								}
							}
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}
				}
				#if GPS_AVAILABLE
				if(config.loc_log_enabled)
				{
					SREG &= ~(0x80);
					fprintf(fptr,",latitude,longitude");
					SREG |= 0x80;
				}
				#endif
				if(config.rssi_log_enabled)
				{
					//#ifdef SD_CARD_DEBUG
					//sprintf(buffer,"Adding RSSI heading to latest.csv\r\n");
					//print1(buffer);
					//#endif
					SREG &= ~(0x80);
					//fprintf(fptr,",RSSI");
					fprintf(fptr,",RSSI,Band");
					//fprintf(fptr,",RSSI,Band,Network Registration Status");
					SREG |= 0x80;
				}
				SREG &= ~(0x80);
				fprintf(fptr,"\r\n");
				SREG |= 0x80;
			}
		}
		else if(k==0)
		{
			// *************************************************************************************************
			// LATEST.CSV
			// *************************************************************************************************
			#ifdef SD_CARD_DEBUG
			sprintf(buffer,"[latest_log_entry] Making a log entry to latest.csv...\r\n");print1(buffer);
			#endif			
			
			SREG &= ~(0x80);
			tickleRover();
			sedateRover();
			fptr = fopenc("latest.csv",APPEND);   //we need to guard this with sedate / wakeRover for when the
											   //file is real big, it can take quite a while to return
			wakeRover();
			SREG |= 0x80;
			
			if(fptr==NULL)
			{
				#ifdef SD_CARD_DEBUG
				sprintf(buffer,"[latest_log_entry] Could not open latest.csv,creating file...\r\n");print1(buffer);
				#endif
				
				SREG &= ~(0x80);
				fptr = fcreatec("latest.csv",0);
				SREG |= 0x80;
				if(fptr==NULL)
				{
					#ifdef SD_CARD_DEBUG
					sprintf(buffer,"[latest_log_entry] Failed to create latest.csv\r\n");print1(buffer);
					#endif
					power_led_flashing();
					e.type = sms_MMC_ACCESS_FAIL;
					queue_push(&q_modem,&e);
					return;
				}
				#ifdef SD_CARD_DEBUG
				sprintf(buffer,"[latest_log_entry] latest.csv created successfully\r\n");print1(buffer);
				#endif
				
				SREG &= ~(0x80);
				fprintf(fptr,"Date / Time");
				SREG |= 0x80;
				for(i=0;i<MAX_INPUTS;i++)
				{
					if(!config.input[i].enabled)
						continue;
					if(config.input[i].log_type&LOG_INSTANT)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Instant",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Instant (",i+1);
							SREG |= 0x80;
							for(j=0;config.input[i].units[j]!='\0';j++)
							{	
								SREG &= ~(0x80);
								fputc(config.input[i].units[j],fptr);
								SREG |= 0x80;
							}
							#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
								SREG &= ~(0x80);
								fputc(')',fptr);
								SREG |= 0x80;
								
								//SJL added log heading for instantaneous frequency error
								SREG &= ~(0x80);
								fprintf(fptr,",Input %d Instant Error +/- (",i+1);
								SREG |= 0x80;
								for(j=0;config.input[i].units[j]!='\0';j++)
								{	
									SREG &= ~(0x80);
									fputc(config.input[i].units[j],fptr);
									SREG |= 0x80;
								}
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
								//end of instantaneous frequency error heading
								
							}
							#endif
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}   //add stuff for average / min / max here. Min / Max analog only
					if(config.input[i].log_type&LOG_MIN)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Minimum",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Minimum (",i+1);
							SREG |= 0x80;
							for(j=0;config.input[i].units[j]!='\0';j++)
							{
								SREG &= ~(0x80);
								fputc(config.input[i].units[j],fptr);
								SREG |= 0x80;
							}
							#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
							}
							#endif
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}
					if(config.input[i].log_type&LOG_MAX)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Maximum",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Maximum (",i+1);
							SREG |= 0x80;
							for(j=0;config.input[i].units[j]!='\0';j++)
							{
								SREG &= ~(0x80);
								fputc(config.input[i].units[j],fptr);
								SREG |= 0x80;
							}
							#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
							}
							#endif
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}
					if(config.input[i].log_type&LOG_AVERAGE)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Log Period Duty (%%)",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Average (",i+1);
							SREG |= 0x80;
							for(j=0;config.input[i].units[j]!='\0';j++)
							{
								SREG &= ~(0x80);
								fputc(config.input[i].units[j],fptr);
								SREG |= 0x80;
							}
							#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}						
								SREG &= ~(0x80);
								fputc(')',fptr);
								SREG |= 0x80;
								
								//SJL added log heading for average frequency error
								SREG &= ~(0x80);
								fprintf(fptr,",Input %d Average Error +/- (",i+1);
								SREG |= 0x80;
								for(j=0;config.input[i].units[j]!='\0';j++)
								{	
									SREG &= ~(0x80);
									fputc(config.input[i].units[j],fptr);
									SREG |= 0x80;
								}
								switch(config.pulse[i-6].period)
								{
									case SECONDS:
										SREG &= ~(0x80);
										fprintf(fptr,"/s");
										SREG |= 0x80;
										break;
									case MINUTES:
										SREG &= ~(0x80);
										fprintf(fptr,"/m");
										SREG |= 0x80;
										break;
									case HOURS:
										SREG &= ~(0x80);
										fprintf(fptr,"/h");
										SREG |= 0x80;
										break;
								}
								//end of instantaneous average error heading
							}
							#endif
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}
					if(config.input[i].log_type&LOG_DUTY)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Current Duty (%%)",i+1);
							SREG |= 0x80;
						}					
					}

					if(config.input[i].log_type&LOG_AGGREGATE)
					{
						if(config.input[i].type==DIGITAL)
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Aggregate",i+1);
							SREG |= 0x80;
						}
						else
						{
							SREG &= ~(0x80);
							fprintf(fptr,",Input %d Aggregate (",i+1);
							SREG |= 0x80;
							//#if PULSE_COUNTING_AVAILABLE
							if(config.input[i].type == PULSE)
							{
								for(j=0;config.input[i].units[j]!='\0';j++)
								{
									SREG &= ~(0x80);
									fputc(config.input[i].units[j],fptr);
									SREG |= 0x80;
								}
							}
							//#endif
							else
							{
								for(j=0;config.input[i].units[j]!='\0'&&config.input[i].units[j]!='/';j++)
								{
									SREG &= ~(0x80);
									fputc(config.input[i].units[j],fptr);
									SREG |= 0x80;
								}
							}
							SREG &= ~(0x80);
							fputc(')',fptr);
							SREG |= 0x80;
						}
					}
				}
				#if GPS_AVAILABLE
				if(config.loc_log_enabled)
				{
					SREG &= ~(0x80);
					fprintf(fptr,",latitude,longitude");
					SREG |= 0x80;
				}
				#endif
				if(config.rssi_log_enabled)
				{
					//#ifdef SD_CARD_DEBUG
					//sprintf(buffer,"Adding RSSI heading to latest.csv\r\n");
					//print1(buffer);
					//#endif
					SREG &= ~(0x80);
					//fprintf(fptr,",RSSI");
					fprintf(fptr,",RSSI,Band");
					//fprintf(fptr,",RSSI,Band,Network Registration Status");
					SREG |= 0x80;
				}
				SREG &= ~(0x80);
				fprintf(fptr,"\r\n");
				SREG |= 0x80;
			}
		}
				
		
		
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[latest_log_entry] file openned successfully\r\n");print1(buffer);
		#endif
		mmc_fail_sent=false; //Successfully opened the file
		
		//DEBUG_printStr("Getting time and date\r\n");
		rtc_get_date(&day,&month,&year);
		rtc_get_time(&hour,&minute,&second);
		//DEBUG_printStr("Printing time and date to file\r\n");
		
		SREG &= ~(0x80);
		fprintf(fptr,"%d/%02d/%02d %02d:%02d:%02d",year+2000,month,day,hour,minute,second);
		SREG |= 0x80;
			
		for(i=0;i<MAX_INPUTS;i++)
		{
			if(config.input[i].enabled == DISABLE)
				continue;
			if(config.input[i].log_type&LOG_INSTANT) //not done
			{
				if(config.input[i].type==DIGITAL)
				{
				   if((input[i].alarm[ALARM_A].buffer & LATEST_SAMPLE) == ZONE2)
				   {
						switch(config.input[i].alarm[0].type)
						{
							case ALARM_OPEN:
								strcpyre(buffer,config.input[i].alarm[0].alarm_msg);
								SREG &= ~(0x80);
								fprintf(fptr,",%s",buffer);
								SREG |= 0x80;
								break;
							case ALARM_CLOSED:
								strcpyre(buffer,config.input[i].alarm[0].reset_msg);
								SREG &= ~(0x80);
								fprintf(fptr,",%s",buffer);
								SREG |= 0x80;
								break;
							default:
								SREG &= ~(0x80);
								fprintf(fptr,",1");
								SREG |= 0x80;
								break;
						}
				   }
				   else
				   {
						switch(config.input[i].alarm[0].type)
						{
							case ALARM_CLOSED:
								strcpyre(buffer,config.input[i].alarm[0].alarm_msg);
								SREG &= ~(0x80);
								fprintf(fptr,",%s",buffer);
								SREG |= 0x80;
								break;
							case ALARM_OPEN:
								strcpyre(buffer,config.input[i].alarm[0].reset_msg);
								SREG &= ~(0x80);
								fprintf(fptr,",%s",buffer);
								SREG |= 0x80;
								break;
							default:
								SREG &= ~(0x80);
								fprintf(fptr,",0");
								SREG |= 0x80;
								break;
						}
				   }
				}
				else
				{
					f = input_getVal(i);
					
					SREG &= ~(0x80);
					//fprintf(fptr,",%3.2f", input_getVal(i));
					fprintf(fptr,",%f",f);
					SREG |= 0x80;
					
					#if PULSE_COUNTING_AVAILABLE
					if(config.input[i].type == PULSE)
					{
						switch(config.pulse[i-6].period)
						{
							case HOURS:							
								g = f - ( ( f * (RATE*3600) ) / ( f + (RATE*3600) ) );
								break;
							case MINUTES:
								g = f - ( ( f * (RATE*60) ) / ( f + (RATE*60) ) );
								break;
							case SECONDS:
							default:
								g = f - ( ( f * RATE ) / ( f + RATE ) );
								break;
						}				
						//g = f - ( ( f * RATE ) / ( f + RATE ) );
						
						SREG &= ~(0x80);
						fprintf(fptr,",%f",g);
						SREG |= 0x80;
					}
					#endif						
				}
			}
			if(config.input[i].log_type&LOG_MIN)
			{
				if(config.input[i].type==DIGITAL)
				{
					SREG &= ~(0x80);
					fprintf(fptr,",%d",local_min[i]);
					SREG |= 0x80;
				}
				else
				{
					SREG &= ~(0x80);
					fprintf(fptr,",%f", config.input[i].conv_int+config.input[i].conv_grad*local_min[i]);
					SREG |= 0x80;
				}
			}
			if(config.input[i].log_type&LOG_MAX)
			{
				if(config.input[i].type==DIGITAL)
				{
					SREG &= ~(0x80);
					fprintf(fptr,",%d",local_max[i]);
					SREG |= 0x80;
				}
				else
				{
					SREG &= ~(0x80);
					fprintf(fptr,",%f", config.input[i].conv_int+config.input[i].conv_grad*local_max[i]);
					SREG |= 0x80;
				}
			}	
			if(config.input[i].log_type&LOG_AVERAGE)
			{
				if(config.input[i].type != PULSE)
				{                
					f = local_input_running_total[i] / local_input_samples[i];
					
					if(config.input[i].type==DIGITAL)
					{
						SREG &= ~(0x80);
						fprintf(fptr,",%f",f*100);
						SREG |= 0x80;
					}
					else
					{
						SREG &= ~(0x80);
						fprintf(fptr,",%f",f);
						SREG |= 0x80;
					}
				}
				#if PULSE_COUNTING_AVAILABLE
				else //PULSE, figure average from the aggregate
				{
					f = local_pulse_aggregate[i-6] / (0.320 * (local_input_samples[i]));
					
					//average pulses per 320ms chunk
					switch(config.pulse[i-6].period)
					{
						case HOURS:
							f *= 3600;
							break;
						case MINUTES:
							f *= 60;
							break;
						case SECONDS:
						default:
							break;
					}
					SREG &= ~(0x80);
					fprintf(fptr,",%f",f);	//fprintf(fptr,",%3.3f",f);
					SREG |= 0x80;
					
					f = (local_pulse_aggregate[i-6] / (0.320 * (local_input_samples[i]))) - (local_pulse_aggregate[i-6] / (0.320 * (local_input_samples[i] + 1)));
					switch(config.pulse[i-6].period)
					{
						case HOURS:
							f *= 3600;
							break;
						case MINUTES:
							f *= 60;
							break;
						case SECONDS:
						default:
							break;
					}
					SREG &= ~(0x80);
					fprintf(fptr,",%f",f);		//fprintf(fptr,",%3.3f",f);
					SREG |= 0x80;					
				}
				#endif
			}
			if(config.input[i].log_type&LOG_DUTY) //not done - not sure if required
			{
				if(config.input[i].type==DIGITAL)
				{
					f = last_duty_high[i];
					l = last_duty_count[i];
					f = f/l;
					SREG &= ~(0x80);
					fprintf(fptr,",%f",f*100);
					SREG |= 0x80;
				}
			}
			if(config.input[i].log_type&LOG_AGGREGATE)
			{
				if(config.input[i].type == PULSE)
				{
					if(i>=6)
						//sprintf(buffer,",%f",config.input[i].conv_grad*pulse_aggregate[i-6]);
						f = config.input[i].conv_grad*local_pulse_aggregate[i-6];
						sprintf(buffer,",%f",f);
				}
				else
				{
					strcpyre(buffer,config.input[i].units);
					
					if(strstrf(buffer,"/h")!=0)
						local_input_running_total[i] /= 3600;
					
					else if(strstrf(buffer,"/m")!=0)
						local_input_running_total[i] /= 60;
					
					sprintf(buffer,",%f", local_input_running_total[i]*.320); //seconds for each sample
				}
				SREG &= ~(0x80);
				fprintf(fptr,"%s",buffer);
				SREG |= 0x80;
			}        
		}

		#if GPS_AVAILABLE
		if(config.loc_log_enabled)
		{
			gps_print(buffer,DEGREES);
			SREG &= ~(0x80);
			fprintf(fptr,",%s",buffer);
			SREG |= 0x80;
		}
		#endif

		if(config.rssi_log_enabled)
		{
			//DEBUG_printStr("Getting RSSI and printing to file\r\n");
			//fprintf(fptr,",%d",modem_get_rssi());
			//SREG &= ~(0x80);	//guard file operation against interrupt
			//sprintf(buffer,"[log_entry:fprintf] Interrupts disabled [%X]",SREG);print1(buffer);	
			SREG &= ~(0x80);
			fprintf(fptr,",%d",rssi);
			SREG |= 0x80;
			
			// Band 
			modem_clear_channel();
			sprintf(buffer,"AT!GETBAND?\r\n");
			print0(buffer);
			modem_wait_for(MSG_GETBAND | MSG_ERROR);		
			c = strchr(modem_rx_string,':');		
			SREG &= ~(0x80);
			if (c && c[1])
				fprintf(fptr,",%s",c + 2);
			else
				fprintf(fptr,",?");
			SREG |= 0x80;
			
			// Network registration status 
			//modem_clear_channel();
			//sprintf(buffer,"AT+CREG?\r\n");
			//print0(buffer);		
			//modem_wait_for(MSG_CREG | MSG_ERROR);		
			//c = strchr(modem_rx_string,',')+1;		
			//SREG &= ~(0x80);
			//fprintf(fptr,",%s",c);
			//SREG |= 0x80;
				
			//sprintf(buffer,"after: [%X]\r\n",SREG);print1(buffer);
		}

		SREG &= ~(0x80);
		fprintf(fptr,"\r\n");
		SREG |= 0x80;
		
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[latest_log_entry] Closing log file\r\n");print1(buffer);
		#endif
				
		SREG &= ~(0x80);	//guard file operation against interrupt	
		//close_result=fclose(fptr);
		fclose(fptr);
		SREG |= 0x80;
		//#ifdef SD_CARD_DEBUG
		//sprintf(buffer,"Data added\r\n");print1(buffer);
		//sprintf(buffer,"[latest_log_entry] File closed = %d\r\n",close_result);print1(buffer);
		//#endif
				
		fptr = NULL;
		k++;
	}
	while(k<2);
	
	//Restart the pulse inputs
	#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		delay_ms(350);	//delay skips the next input scan (all inputs)
						//ensures that the pulse period is valid
		pulse_instant_pause = 0;
	}
	#endif
	
	return;
}

#if EMAIL_AVAILABLE	//320 and 321 only
/* Email a section (or complete) of the log.csv, system.log or alarm.log file ***********
User can enter a date and time string for start and end of section of log required to be
emailed
WARNING: This function searches through every character of log.csv until the start date
and time is found - then every proceeding character until the end date and time. This can
take a long time to search large files and is not really recommended to use often. Much
better to use email_latest() which does not have any search function (emails complete
file).

DNS attempts: DNS0 then DNS1
SMTP attempts: 2
************************************************************************************** */
bool email_log() //email a log file to address specified by the update address
{
	// -----------------------------------------------------------------------------------------
	// -- GET LOG DATA VARIBLES ----------------------------------------------------------------
	FILE *fptr = NULL;
	unsigned char log_char;
	char read_char = 0;
	int count_char = 0;	//changed from char to int because 127 chracters not enough for log lines
	char date_time[19];
	//last log entry emailed - to be written to hidden file
	unsigned int yr;
	unsigned char mth, dy, hr, min, sec;
	//date and time to start log data
	unsigned int s_yr;
	unsigned char s_mth, s_dy, s_hr, s_min, s_sec;
	//date and time to end log data
	unsigned int e_yr;
	unsigned char e_mth, e_dy, e_hr, e_min, e_sec;
	bit dates_valid = 0;
	bit logcsv_select;
	bit system0_alarm1;
	//bit hidden_flag=0;
	
	//bit start_date_ok = 0;

	// -----------------------------------------------------------------------------------------
	// -- DNS VARIABLES ------------------------------------------------------------------------
	char *ip_address;

	// -----------------------------------------------------------------------------------------
	// -- SMTP VARIABLES ------------------------------------------------------------------------
	char retries=0;
	bit retry;
	
	#if ERROR_FILE_AVAILABLE
	sprintf(sms_dates,"");	//clear string for error codes
	#endif
	
	tickleRover();

	//#ifdef EMAIL_DEBUG
	//strcpye(config.site_name,"test7");
	//strcpye(serial,"0123456");
	//strcpye(config.from_address,"test7@edacelectronics.com");
	//strcpye(config.update_address,"samlea@edacelectronics.com");	
	//samjlea@gmail.com, samlea@edacelectronics.com, samjlea@ymail.com
	
	//sprintf(buffer,"Sending email - ");print1(buffer);
	//sprintf(buffer,"[email_log] Data retrieval: %s\r\n",dates);print1(buffer);
	//sprintf(buffer,"[email_log] sms_dates: %s\r\n",sms_dates);print1(buffer);
	//#endif
	
	//convert date and time string to chars
	log_get_date_from_string(substr(dates,date_time,0,19),&s_yr,&s_mth,&s_dy,&s_hr,&s_min,&s_sec);
	//sprintf(buffer,"[email_log] Start date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
	log_get_date_from_string(substr(dates,date_time,20,39),&e_yr,&e_mth,&e_dy,&e_hr,&e_min,&e_sec);
	//sprintf(buffer,"[email_log] End date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);

	// *****************************************************************************************
	// ** INITLIALIZE THE SD CARD **************************************************************
	if(!initialize_media())
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to initialize the SD card");print1(buffer);
		#endif		
		
		#if ERROR_FILE_AVAILABLE	
		sprintf(buffer,"1001#");
		strcat(sms_dates,buffer);		
			#ifdef ERROR_FILE_DEBUG
			print1(sms_dates);
			sprintf(buffer,"\r\n");
			print1(buffer);
			#endif		
		log_line("error.log",sms_dates);
		#endif
		
        return 0;
    }
	
	// *****************************************************************************************
	// ** READ THE FILE OPTIONS AND OPEN THE FILE FOR READ *************************************

	// *** OPTION 1 - log file select ***
	//substr(dates,c,40,41) == '0,1,2' 	where 0=log.csv, 1=system.log, 2=alarm.log
	substr(dates,&log_char,40,41);
	switch(log_char)
	{
		case '0':
			//#ifdef SD_CARD_DEBUG
			//sprintf(buffer,"[email_log] Openning the log.csv file...\r\n");print1(buffer);
			//#endif
			SREG &= ~(0x80);	//guard file operation against interrupt
			tickleRover();
			sedateRover();		
			fptr = fopenc("log.csv",READ);
			wakeRover();
			SREG |= 0x80;
			
			logcsv_select=1;
			break;
		case '1':
			//#ifdef SD_CARD_DEBUG
			//sprintf(buffer,"[email_log] Openning the system.log file...\r\n");print1(buffer);
			//#endif
			SREG &= ~(0x80);	//guard file operation against interrupt
			tickleRover();
			sedateRover();		
			fptr = fopenc("system.log",READ);
			wakeRover();
			SREG |= 0x80;

			system0_alarm1 = 0;
			break;
		case '2':
			//#ifdef SD_CARD_DEBUG
			//sprintf(buffer,"[email_log] Openning the alarm.log file...\r\n");print1(buffer);
			//#endif
			SREG &= ~(0x80);	//guard file operation against interrupt
			tickleRover();
			sedateRover();		
			fptr = fopenc("alarm.log",READ);
			wakeRover();
			SREG |= 0x80;

			system0_alarm1 = 1;
			break;
		default:
			#ifdef EMAIL_DEBUG
			sprintf(buffer,"ERROR: Invalid date and/or time entered\r\n");print1(buffer);
			#endif
			
			#if ERROR_FILE_AVAILABLE	
			sprintf(buffer,"1002#");
			strcat(sms_dates,buffer);			
				#ifdef ERROR_FILE_DEBUG
				print1(sms_dates);
				sprintf(buffer,"\r\n");
				print1(buffer);
				#endif			
			log_line("error.log",sms_dates);			
			// #else			
				// #if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
				// if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
				// {
					// pulse_instant_pause = 0;
				// }
				// #endif			
			#endif
			
			return 0;
			//break;
	}

	// *** OPTION 2 - save to hidden file flag - if log.csv selected only ***
	//if(logcsv_select) //commented out 3.xx.7 - hidden flag no longer required
	//{
	//	substr(dates,&log_char,42,43);
	//	switch(log_char)
	//	{
	//		case '0':
				//#ifdef EMAIL_DEBUG
				//sprintf(buffer,"[email_log] 0 - do not save time and date to hidden file\r\n");print1(buffer);
				//#endif
	//			break;
	//		case '1':
				//#ifdef EMAIL_DEBUG
				//sprintf(buffer,"[email_log] 1 - save time and date to hidden file\r\n");print1(buffer);
				//#endif
	//			hidden_flag=1;
	//			break;
	//		default:
				//#ifdef EMAIL_DEBUG
				//sprintf(buffer,"[email_log] Flag not detected - not saving time and date to hidden file\r\n");print1(buffer);
				//#endif
	//			break;
	//	}
	//}

	if(fptr==NULL)
	{
		//unable to open file
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to open file\r\n");print1(buffer);
		#endif
		
		#if ERROR_FILE_AVAILABLE	
		sprintf(buffer,"1003#");
		strcat(sms_dates,buffer);		
			#ifdef ERROR_FILE_DEBUG
			print1(sms_dates);
			sprintf(buffer,"\r\n");
			print1(buffer);
			#endif		
		log_line("error.log",sms_dates);		
		// #else		
			// #if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
			// if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
			// {
				// delay_ms(350);	//delay skips the next input scan (all inputs)
								//ensures that the pulse period is valid
				// pulse_instant_pause = 0;
			// }
			// #endif		
		#endif
		
		return 1;
	}
	else	//file exists
	{
		//#ifdef EMAIL_DEBUG
		//sprintf(buffer,"[email_log] Log file openned successfully\r\n");print1(buffer);
		//#endif
		while(!feof(fptr))
		{
			tickleRover();
			
			SREG &= ~(0x80);	//guard file operation against interrupt
			log_char = fgetc(fptr);
			SREG |= 0x80;
						
			sprintf(&read_char,"%c",log_char);

			if(count_char<19) //max number of date + time chars = 19
			{
				//if the string is full empty it for the next log date and time
				if(count_char==0)
				{
					sprintf(date_time,"");
				}
				//add the next character to the date + time string
				strncat(date_time,&read_char,1);
			}
			count_char++;
			if(count_char==19)
			{
				log_get_date_from_string(date_time,&yr,&mth,&dy,&hr,&min,&sec);
				
				
				//Compare the date from the current log line and the date saved in the hidden file
				//if the time and date of last log line emailed (from hidden file) is earlier than the
				//date and time of the curent log line set the email_line flag to add line to message body
				if ((yr>s_yr) ||
				(yr==s_yr && mth>s_mth) ||
				(yr==s_yr && mth==s_mth && dy>s_dy) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr>s_hr) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min>s_min) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min==s_min && sec>s_sec))	//> ensures hidden file date is not sent again
				{
					if((yr<e_yr) ||
					(yr==e_yr && mth<e_mth) ||
					(yr==e_yr && mth==e_mth && dy<e_dy) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr<e_hr) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min<e_min) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min==e_min && sec<=e_sec))	//upto and inluding end date and time
					{
						//data is valid - set the pointer and exit loop
						//sprintf(buffer,"[email_log] Valid data found\r\n");print1(buffer);
						dates_valid = 1;
						break;
					}
				}
								
				/*
				if(yr>s_yr)
				{
					start_date_ok = 1;
				}
				else if(yr==s_yr)
				{
					if(mth>s_mth)
					{
						start_date_ok = 1;
					}
					else if(mth==s_mth)
					{
						if(dy>s_dy)
						{
							start_date_ok = 1;
						}
						else if(dy==s_dy)
						{
							if(hr>s_hr)
							{
								start_date_ok = 1;
							}
							else if(hr==s_hr)
							{
								if(min>s_min)
								{
									start_date_ok = 1;
								}
								else if(min==s_min)
								{
									if(sec>s_sec)
									{
										start_date_ok = 1;
									}
								}
							}
						}
					}
				}
				
				if(start_date_ok)
				{
					if(yr<e_yr)
					{
						dates_valid = 1;
						break;
					}
					else if(yr==e_yr)
					{
						if(mth<e_mth)
						{
							dates_valid = 1;
							break;
						}
						else if(mth==e_mth)
						{
							if(dy<e_dy)
							{
								dates_valid = 1;
								break;
							}
							else if(dy==e_dy)
							{
								if(hr<e_hr)
								{
									dates_valid = 1;
									break;
								}
								else if(hr==e_hr)
								{
									if(min<e_min)
									{
										dates_valid = 1;
										break;
									}
									else if(min==e_min)
									{
										if(sec<=e_sec)
										{
											dates_valid = 1;
											break;
										}
									}
								}
							}
						}
					}
				}
				*/				
			}

			//detect end of logged line
			if(log_char==10) //line feed character \n (10) detected  && carr_rtrn==1
			{
				//reset the char counter for a new line in the log
				count_char = 0;
				//email_line = 0;
				//sprintf(buffer,"The last log entry emailed is: %04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
			}
		}

		if(dates_valid)
		{
			// *****************************************************************************************
			// ** RESOLVE THE IP ADDRESS ***************************************************************
			//#ifdef EMAIL_DEBUG
			//strcpyef(config.smtp_serv,"smtp2.vodafone.net.nz");
			//#endif
			
			//strcpyre(command_string,config.smtp_serv);
			//sprintf(buffer,"[email_log] Resolving IP address...\r\n");print1(buffer);

			do
			{
				retry=FALSE;
				// *****************************************************************************************
				// ** OPEN A SOCKET TO SMTP SERVER *********************************************************
				read_char = 'S';		//set connection type
				//if(open_socket(&read_char, ip_address))
				//{
				//	sprintf(buffer,"[email_log] SMTP socket open\r\n");print1(buffer);
				//}
				if(!(open_socket()))
				{
					#ifdef SMTP_DEBUG
					sprintf(buffer,"SMTP ERROR: Failed to open connection to SMTP server [%s]\r\n",modem_rx_string);print1(buffer);
					#endif
					#if ERROR_FILE_AVAILABLE	
					sprintf(buffer,"1006.");
					strcat(sms_dates,buffer);
					#ifdef ERROR_FILE_DEBUG
					print1(sms_dates);
					sprintf(buffer,"\r\n");
					print1(buffer);
					#endif
					#endif
					//return;
					retry=TRUE;
				}

				if(!retry)
				{
					// *****************************************************************************************
					// ** WRITE EMAIL HEADER *******************************************************************
					if(!test_smtp_code())
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"[email_log] Timed out waiting for 220 from SMTP server\r\n");print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"1007.");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						retry=TRUE;
					}
					
					if (strstrf(modem_rx_string,"220"))
					{

						//SJL - Debug -->
						//sprintf(buffer,"[email header]\r\n");print1(buffer);
						//sprintf(buffer,"Connected to VF SMTP server successfully, saying ehlo\r\n");print1(buffer);
						//SJL - <-- End of Debug

						sprintf(buffer,"ehlo edacelectronics.com\r\n");print0(buffer);
						#ifdef SMTP_DEBUG
						print1(buffer);
						#endif
						if(!test_smtp_code())
						{
							#ifdef SMTP_DEBUG
							sprintf(buffer,"[email_log] Timed out waiting for response to ehlo\r\n");print1(buffer);
							#endif
							#if ERROR_FILE_AVAILABLE	
							sprintf(buffer,"1009.");
							strcat(sms_dates,buffer);
							#ifdef ERROR_FILE_DEBUG
							print1(sms_dates);
							sprintf(buffer,"\r\n");
							print1(buffer);
							#endif
							#endif
							retry=TRUE;
						}

						if(strstrf(modem_rx_string,"250 "))
						{
							//SJL - Debug -->
							//sprintf(buffer,"Setting from address\r\n");print1(buffer);
							//SJL - <-- End of Debug
							sprintf(buffer,"mail from:");print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif
							strcpyre(buffer,config.from_address);print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif
							sprintf(buffer,"\r\n");print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif

							if(!test_smtp_code())
							{
								#ifdef SMTP_DEBUG
								sprintf(buffer,"[email_log] Timed out waiting for response to mail from\r\n");print1(buffer);
								#endif
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"100B.");
								strcat(sms_dates,buffer);
								#ifdef ERROR_FILE_DEBUG
								print1(sms_dates);
								sprintf(buffer,"\r\n");
								print1(buffer);
								#endif
								#endif
								retry=TRUE;
							}

							if(strstrf(modem_rx_string,"250 "))
							{
								//SJL - Debug -->
								//sprintf(buffer,"Setting recipient address\r\n");print1(buffer);
								//SJL - <-- End of Debug
								sprintf(buffer,"rcpt to:");print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif
								strcpyre(buffer,config.update_address);print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif
								sprintf(buffer,"\r\n");print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif

								if(!test_smtp_code())
								{
									#ifdef SMTP_DEBUG
									sprintf(buffer,"[email_log] Timed out waiting for response to rcpt to\r\n");print1(buffer);
									#endif
									#if ERROR_FILE_AVAILABLE	
									sprintf(buffer,"100D.");
									strcat(sms_dates,buffer);
									#ifdef ERROR_FILE_DEBUG
									print1(sms_dates);
									sprintf(buffer,"\r\n");
									print1(buffer);
									#endif
									#endif
									retry=TRUE;
								}

								if(strstrf(modem_rx_string,"250 "))
								{

									//SJL - Debug -->
									//sprintf(buffer,"Request data mode\r\n");print1(buffer);
									//SJL - <-- End of Debug
									sprintf(buffer,"data\r\n");print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									if(!test_smtp_code())
									{
										#ifdef SMTP_DEBUG
										sprintf(buffer,"[email_log] Timed out waiting for response to data request\r\n");print1(buffer);
										#endif
										#if ERROR_FILE_AVAILABLE	
										sprintf(buffer,"100F.");
										strcat(sms_dates,buffer);
										#ifdef ERROR_FILE_DEBUG
										print1(sms_dates);
										sprintf(buffer,"\r\n");
										print1(buffer);
										#endif
										#endif
										retry=TRUE;
									}

									if(strstrf(modem_rx_string,"354 "))
									{
										//DATE and TIME
										// removed because wrong format for SMTP  - needs to be Thu, 21 May 2008 05:33:29 -0700
										// uses server date and time instead which is ok
										//rtc_get_date(&dy,&mth,&yr);rtc_get_time(&hr,&min,&sec);
										//sprintf(buffer,"Date: %d,%d,%d %02d:%02d:%02d\r\n",dy,mth,yr,hr,min,sec);print0(buffer);
										//#ifdef SMTP_DEBUG
										//print1(buffer);
										//#endif
										rtc_get_date(&s_dy,&s_mth,&log_char);rtc_get_time(&s_hr,&s_min,&s_sec);

										//FROM
										sprintf(buffer,"From: ");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										strcpyre(buffer,config.from_address);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										sprintf(buffer,"\r\n");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif

										//TO
										sprintf(buffer,"To: ");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										strcpyre(buffer,config.update_address);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										sprintf(buffer,"\r\n");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif

										//SUBJECT
										if(logcsv_select)
										{
											sprintf(buffer,"Subject: Log request for ");
										}
										else if(!system0_alarm1)
										{
											sprintf(buffer,"Subject: System log for ");
										}
										else if(system0_alarm1)
										{
											sprintf(buffer,"Subject: Alarm log for ");
										}
										print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										strcpyre(buffer,config.site_name);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										sprintf(buffer," @ %d/%d/%d %02d:%02d:%02d (",s_dy,s_mth,log_char,s_hr,s_min,s_sec);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										strcpyre(buffer,serial);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										sprintf(buffer,")\r\n\r\n");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
									}
									else
									{
										#ifdef SMTP_DEBUG
										sprintf(buffer,"SMTP ERROR: Failed to enter data mode [%s]\r\n",modem_rx_string);print1(buffer);
										#endif
										#if ERROR_FILE_AVAILABLE	
										sprintf(buffer,"1010.");
										strcat(sms_dates,buffer);
										#ifdef ERROR_FILE_DEBUG
										print1(sms_dates);
										sprintf(buffer,"\r\n");
										print1(buffer);
										#endif
										#endif
										quit_smtp_server();
										retry=TRUE;
									}
								}
								else
								{
									#ifdef SMTP_DEBUG
									sprintf(buffer,"SMTP ERROR: Failed to set recipient address [%s]\r\n",modem_rx_string);print1(buffer);
									#endif
									#if ERROR_FILE_AVAILABLE	
									sprintf(buffer,"100E.");
									strcat(sms_dates,buffer);
									#ifdef ERROR_FILE_DEBUG
									print1(sms_dates);
									sprintf(buffer,"\r\n");
									print1(buffer);
									#endif
									#endif
									quit_smtp_server();
									retry=TRUE;
								}
							}
							else
							{
								#ifdef SMTP_DEBUG
								sprintf(buffer,"SMTP ERROR: Failed to set from address [%s]\r\n",modem_rx_string);print1(buffer);
								#endif
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"100C.");
								strcat(sms_dates,buffer);
								#ifdef ERROR_FILE_DEBUG
								print1(sms_dates);
								sprintf(buffer,"\r\n");
								print1(buffer);
								#endif
								#endif
								quit_smtp_server();
								retry=TRUE;
							}
						}
						else
						{
							#ifdef SMTP_DEBUG
							sprintf(buffer,"SMTP ERROR: Failed to ehlo [%s]\r\n",modem_rx_string);print1(buffer);
							#endif
							#if ERROR_FILE_AVAILABLE	
							sprintf(buffer,"100A.");
							strcat(sms_dates,buffer);
							#ifdef ERROR_FILE_DEBUG
							print1(sms_dates);
							sprintf(buffer,"\r\n");
							print1(buffer);
							#endif
							#endif
							quit_smtp_server();
							retry=TRUE;
							//then stuck in data mode
						}

					}
					else //NO CARRIER received
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"SMTP ERROR: Server failed to reply 220 [%s]\r\n",modem_rx_string);print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"1008.");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						quit_smtp_server();
						retry=TRUE;
					}
				}

				if(!retry)
				{
					if(logcsv_select) //write the log.csv headings
					{
						// *****************************************************************************************
						// ** WRITE LOG HEADINGS *******************************************************************
						//SJL - Debug -->
						//sprintf(buffer,"[email_log] Writing the log headings...\r\n");print1(buffer);
						//SJL - <-- End of Debug
						sprintf(buffer,"Date / Time");print0(buffer);
						for(read_char=0;read_char<MAX_INPUTS;read_char++)
						{
							if(!config.input[read_char].enabled)
								continue;
							if(config.input[read_char].log_type&LOG_INSTANT)
							{
								if(config.input[read_char].type==DIGITAL)
								{
									sprintf(buffer,",Input %d Instant",read_char+1);print0(buffer);
								}
								else
								{
									sprintf(buffer,",Input %d Instant (",read_char+1);print0(buffer);
									for(log_char=0;config.input[read_char].units[log_char]!='\0';log_char++)
									{
										sprintf(buffer,"%c",config.input[read_char].units[log_char]);print0(buffer);
									}
									
									#if PULSE_COUNTING_AVAILABLE
									if(config.input[read_char].type == PULSE)
									{
										switch(config.pulse[read_char-6].period)
										{
											case SECONDS:
												sprintf(buffer,"/s");print0(buffer);
												break;
											case MINUTES:
												sprintf(buffer,"/m");print0(buffer);
												break;
											case HOURS:
												sprintf(buffer,"/h");print0(buffer);
												break;
										}			
									
										//SJL added log heading for instantaneous frequency error										
										sprintf(buffer,",Input %d Instant Error +/- (",read_char+1);print0(buffer);									
										for(log_char=0;config.input[read_char].units[log_char]!='\0';log_char++)
										{
											sprintf(buffer,"%c",config.input[read_char].units[log_char]);print0(buffer);
										}
										switch(config.pulse[read_char-6].period)
										{
											case SECONDS:
												sprintf(buffer,"/s");print0(buffer);
												break;
											case MINUTES:
												sprintf(buffer,"/m");print0(buffer);
												break;
											case HOURS:
												sprintf(buffer,"/h");print0(buffer);
												break;
										}
										sprintf(buffer,")");print0(buffer);
										//end of instantaneous frequency error heading				
									}
									#endif
									
									sprintf(buffer,")");print0(buffer);
								}
							}   //add stuff for average / min / max here. Min / Max analog only
							if(config.input[read_char].log_type&LOG_MIN)
							{
								if(config.input[read_char].type==DIGITAL)
								{
								   sprintf(buffer,",Input %d Minimum",read_char+1);print0(buffer);
								}
								else
								{
									sprintf(buffer,",Input %d Minimum (",read_char+1);print0(buffer);
									for(log_char=0;config.input[read_char].units[log_char]!='\0';log_char++)
									{
										sprintf(buffer,"%c",config.input[read_char].units[log_char]);print0(buffer);
									}
									#if PULSE_COUNTING_AVAILABLE
									if(config.input[read_char].type == PULSE)
									{
										switch(config.pulse[read_char-6].period)
										{
											case SECONDS:
												sprintf(buffer,"/s");print0(buffer);
												break;
											case MINUTES:
												sprintf(buffer,"/m");print0(buffer);
												break;
											case HOURS:
												sprintf(buffer,"/h");print0(buffer);
												break;
										}
									}
									#endif
								    sprintf(buffer,")");print0(buffer);
							   }
							}
							if(config.input[read_char].log_type&LOG_MAX)
							{
								if(config.input[read_char].type==DIGITAL)
								{
									sprintf(buffer,",Input %d Maximum",read_char+1);print0(buffer);
								}
								else
								{
									sprintf(buffer,",Input %d Maximum (",read_char+1);print0(buffer);
									for(log_char=0;config.input[read_char].units[log_char]!='\0';log_char++)
									{
										sprintf(buffer,"%c",config.input[read_char].units[log_char]);print0(buffer);
									}
									#if PULSE_COUNTING_AVAILABLE
									if(config.input[read_char].type == PULSE)
									{
										switch(config.pulse[read_char-6].period)
										{
											case SECONDS:
												sprintf(buffer,"/s");print0(buffer);
												break;
											case MINUTES:
												sprintf(buffer,"/m");print0(buffer);
												break;
											case HOURS:
												sprintf(buffer,"/h");print0(buffer);
												break;
										}
									}
									#endif
								    sprintf(buffer,")");print0(buffer);
							    }
							}
							if(config.input[read_char].log_type&LOG_AVERAGE)
							{
								if(config.input[read_char].type==DIGITAL)
								{
									sprintf(buffer,",Input %d Log Period Duty (%%)",read_char+1);print0(buffer);
								}
							   else
							   {
									sprintf(buffer,",Input %d Average (",read_char+1);print0(buffer);
									for(log_char=0;config.input[read_char].units[log_char]!='\0';log_char++)
									{
										sprintf(buffer,"%c",config.input[read_char].units[log_char]);print0(buffer);
									}
									#if PULSE_COUNTING_AVAILABLE
									if(config.input[read_char].type == PULSE)
									{
										switch(config.pulse[read_char-6].period)
										{
											case SECONDS:
												sprintf(buffer,"/s");print0(buffer);
												break;
											case MINUTES:
												sprintf(buffer,"/m");print0(buffer);
												break;
											case HOURS:
												sprintf(buffer,"/h");print0(buffer);
												break;
										}
										
										//SJL added log heading for average frequency error
										sprintf(buffer,",Input %d Average Error +/- (",read_char+1);print0(buffer);
										for(log_char=0;config.input[read_char].units[log_char]!='\0';log_char++)
										{	
											sprintf(buffer,"%c",config.input[read_char].units[log_char]);print0(buffer);
										}
										switch(config.pulse[read_char-6].period)
										{
											case SECONDS:
												sprintf(buffer,"/s");print0(buffer);
												break;
											case MINUTES:
												sprintf(buffer,"/m");print0(buffer);
												break;
											case HOURS:
												sprintf(buffer,"/h");print0(buffer);
												break;
										}
										sprintf(buffer,")");print0(buffer);
										//end of instantaneous average error heading										
									}
									#endif
									sprintf(buffer,")");print0(buffer);
							   }
							}
							if(config.input[read_char].log_type&LOG_DUTY)
							{
								if(config.input[read_char].type==DIGITAL)
								{
								   sprintf(buffer,",Input %d Current Duty (%%)",read_char+1);print0(buffer);
								}
							}

							if(config.input[read_char].log_type&LOG_AGGREGATE)
							{
								if(config.input[read_char].type==DIGITAL)
								{
								   sprintf(buffer,",Input %d Aggregate",read_char+1);print0(buffer);
								}
								else
								{
									sprintf(buffer,",Input %d Aggregate (",read_char+1);print0(buffer);
									if(config.input[read_char].type == PULSE)
									{
										for(log_char=0;config.input[read_char].units[log_char]!='\0';log_char++)
										{
										   sprintf(buffer,"%c",config.input[read_char].units[log_char]);print0(buffer);
										}
									}
									else
									{
										for(log_char=0;config.input[read_char].units[log_char]!='\0'&&config.input[read_char].units[log_char]!='/';log_char++)
										{
											sprintf(buffer,"%s",config.input[read_char].units[log_char]);print0(buffer);
										}
									}
									sprintf(buffer,")");print0(buffer);
								}
							}

						}
						#if GPS_AVAILABLE
						if(config.loc_log_enabled)
							fprintf(fptr,",latitude,longitude");
						#endif
						if(config.rssi_log_enabled)
						{
							
							//sprintf(buffer,",RSSI,Band,Network Registration Status");print0(buffer);
							sprintf(buffer,",RSSI,Band");print0(buffer);
							//sprintf(buffer,",RSSI");print0(buffer);
						}
						sprintf(buffer,"\r\n");print0(buffer);
					}

					// *****************************************************************************************
					// ** WRITE LOG DATA TO MESSAGE BODY *******************************************************
					//sprintf(buffer,"[email_log] Getting valid data from log file...\r\n");print1(buffer);
					sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);print0(dates);//print1(dates);
					while(!feof(fptr))
					{
						tickleRover();
						//sprintf(buffer,"The file pointer is: %d - ",fptr->position);print1(buffer);
						
						SREG &= ~(0x80);	//guard file operation against interrupt
						log_char = fgetc(fptr);
						SREG |= 0x80;
						
						sprintf(&read_char,"%c",log_char);
						//#ifdef SMTP_DEBUG
						//print0(&read_char);
						//sprintf(buffer,"%c[%d] ",read_char,log_char);print1(buffer);
						//#endif

						if(count_char<19) //max number of date + time chars = 19
						{
							//if the string is full empty it for the next log date and time
							if(count_char==0)
							{
								sprintf(date_time,"");
							}
							//add the next character to the date + time string
							strncat(date_time,&read_char,1);
						}
						count_char++;
						if(count_char==19)
						{
							log_get_date_from_string(date_time,&yr,&mth,&dy,&hr,&min,&sec);	//prints 2000/01/01 00:00:00 after headings line
							//#ifdef SMTP_DEBUG
							//sprintf(buffer,"%04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
							//#endif
														
							if((yr<e_yr) ||
							(yr==e_yr && mth<e_mth) ||
							(yr==e_yr && mth==e_mth && dy<e_dy) ||
							(yr==e_yr && mth==e_mth && dy==e_dy && hr<e_hr) ||
							(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min<e_min) ||
							(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min==e_min && sec<=e_sec))	//upto and inluding end date and time
							{
								//print the date and time to the message body
								sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
								//sprintf(date_time,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
								//if(uart_select)
									//print1(dates);
								//else
									print0(dates);

								//copy the date and time back into the date_time string to be saved to the hidden file later
								//[because sec is always 0 after leaving the while loop - memory problem???]
								//sprintf(date_time,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
								//email_line = 1;
								dates_valid = 1;
							}
							else
								break;
								
							/*
							if(yr<e_yr)
							{
								sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
								print0(dates);
								dates_valid = 1;
							}
							else if(yr==e_yr)
							{
								if(mth<e_mth)
								{
									sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
									print0(dates);
									dates_valid = 1;
								}
								else if(mth==e_mth)
								{
									if(dy<e_dy)
									{
										sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
										print0(dates);
										dates_valid = 1;
									}
									else if(dy==e_dy)
									{
										if(hr<e_hr)
										{
											sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
											print0(dates);
											dates_valid = 1;
										}
										else if(hr==e_hr)
										{
											if(min<e_min)
											{
												sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
												print0(dates);
												dates_valid = 1;
											}
											else if(min==e_min)
											{
												if(sec<=e_sec)
												{
													sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
													print0(dates);
													dates_valid = 1;
												}
												else break;
											}
											else break;
										}
										else break;
									}
									else break;
								}
								else break;
							}
							else break;
							*/							
						}

						//sprintf(buffer,"(%d,%d)",count_char,email_line);print1(buffer);
						if(count_char>19)	// && email_line==1)
						//if(count_char>19)
						{
							//#ifdef SMTP_DEBUG
							//print1(&read_char);
							//#endif
							print0(&read_char);
						}

						//detect end of logged line
						if(log_char==10) //line feed character \n (10) detected  && carr_rtrn==1
						{
							//reset the char counter for a new line in the log
							count_char = 0;
							//email_line = 0;
							//#ifdef SMTP_DEBUG
							//sprintf(buffer,"The last log entry emailed is: %04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
							//#endif
						}
					}
					#ifdef SMTP_DEBUG
					sprintf(buffer,"[email_log] Log data written to message body\r\n");print1(buffer);
					#endif
					
					SREG &= ~(0x80);	//commented out 100811
					count_char = fclose(fptr);
					SREG |= 0x80;
					
					tickleRover();
					
					//#ifdef SMTP_DEBUG
					//sprintf(buffer,"[email_log] result of fclose = %d\r\n",count_char);print1(buffer);
					//if(!fclose(fptr))
					//{
					//	sprintf(buffer,"File closed successfully\r\n");print1(buffer);
					//}
					//else
					//{
					//	sprintf(buffer,"FILE ERROR [403]: Failed to close file\r\n");print1(buffer);
					//}
					//fptr=NULL;
					//#endif
					
					// *****************************************************************************************
					// ** END SMTP DATA MODE *******************************************************************
					sprintf(buffer,"\r\n.\r\n");print0(buffer);
					
					if(test_end_data_mode())
					{		
						// *****************************************************************************************
						// ** QUIT SMTP SERVER *********************************************************************
						quit_smtp_server();
						
						#ifdef EMAIL_DEBUG
						sprintf(buffer,"[email_log] The last log entry is: %s\r\n",dates);print1(buffer);
						#endif
						
						/* code to save latest date and time to hidden file - no longer used in 3.xx.7
						// *****************************************************************************************
						// ** SAVE DATE AND TIME TO HIDDEN FILE ****************************************************
						if(hidden_flag==1 && logcsv_select==1) //logcsv_select is just an extra check - not really needed
						{		
							//SJL added 200911
							if(!initialize_media())
							{
								#ifdef SD_CARD_DEBUG
								sprintf(buffer,"FILE ERROR: Failed to initialize the SD card");print1(buffer);
								#endif
								
								// #if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
								// if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
								// {
									// delay_ms(350);	//delay skips the next input scan (all inputs)
													//ensures that the pulse period is valid
									// pulse_instant_pause = 0;
								// }
								// #endif
								
								return 1;
							}
							
							#ifdef SD_CARD_DEBUG
							sprintf(buffer,"[email_log] Openning the save.txt file...\r\n");print1(buffer);
							#endif
							
							SREG &= ~(0x80);	//guard file operation against interrupt
							tickleRover();
							sedateRover();														
							fptr = fopenc("save.txt",WRITE);
							wakeRover();
							SREG |= 0x80;					
							
							if(fptr == NULL)
							{	
								#ifdef SD_CARD_DEBUG
								sprintf(buffer,"[email_log] Failed to open save.txt, creating...\r\n");print1(buffer);
								#endif
								SREG &= ~(0x80);	//guard file operation against interrupt
								fptr = fcreatec("save.txt",ATTR_HIDDEN);
								//fptr = fcreatec("save.txt",0);
								SREG |= 0x80;
								tickleRover();
							}
							if(fptr == NULL)
							{
								#ifdef SD_CARD_DEBUG
								sprintf(buffer,"[email_log] Failed to create save.txt\r\n");print1(buffer);
								#endif
								//email_retry_timer = -1;
								
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"1012#");
								strcat(sms_dates,buffer);
									#ifdef ERROR_FILE_DEBUG
									print1(sms_dates);
									sprintf(buffer,"\r\n");
									print1(buffer);
									#endif
								log_line("error.log",sms_dates);
								// #else								
									// #if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
									// if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
									// {
										// delay_ms(350);	//delay skips the next input scan (all inputs)
														//ensures that the pulse period is valid
										// pulse_instant_pause = 0;
									// }
									// #endif								
								#endif
								
								return 1;
							}
							tickleRover();
							SREG &= ~(0x80);	//guard file operation against interrupt
							fprintf(fptr,"%s\r\n",dates);
							SREG |= 0x80;
							
							tickleRover();
							SREG &= ~(0x80);	//guard file operation against interrupt
							count_char = fclose(fptr);
							SREG |= 0x80;
							
							// #if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
							// if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
							// {
								// delay_ms(350);	//delay skips the next input scan (all inputs)
												//ensures that the pulse period is valid
								// pulse_instant_pause = 0;
							// }
							// #endif
							
							#ifdef SD_CARD_DEBUG
							//sprintf(buffer,"[email_log] result of fclose = %d\r\n",count_char);print1(buffer);
							sprintf(buffer,"[email_log] Date and time written to hidden file\r\n",count_char);print1(buffer);
							#endif
							fptr=NULL; //SJL - 110811
							
							return 1;
						}
						else
						{
							// #if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
							// if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
							// {
								// delay_ms(350);	//delay skips the next input scan (all inputs)
												//ensures that the pulse period is valid
								// pulse_instant_pause = 0;
							// }
							// #endif
							
							return 1;
						}
						*/
						return 1;

					}
					else
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"SMTP ERROR: Data mode could not be ended\r\n");print1(buffer);
						sprintf(buffer,"Email may or may not have been sent\r\n");print1(buffer);
						sprintf(buffer,"Latest date and time has not been saved\r\n");print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"1011#");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						
						retries=2;	//must exit while loop because file pointer is at end of file
						//retry=TRUE;
						//break;
					}
				}
				retries++;
			}
			while(retries<2 && retry==TRUE);
			if(retry)
			{
				SREG &= ~(0x80);	//added 100811
				count_char = fclose(fptr);
				SREG |= 0x80;
								
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"SMTP ERROR: Failed to send email\r\n");print1(buffer);
				#endif
				
				#if ERROR_FILE_AVAILABLE	
				sprintf(buffer,"1013#");
				strcat(sms_dates,buffer);				
					#ifdef ERROR_FILE_DEBUG
					print1(sms_dates);
					sprintf(buffer,"\r\n");
					print1(buffer);
					#endif				
				log_line("error.log",sms_dates);				
				#endif
				
				return 0;				
			}
			else		
			{			
				//failed to end data mode correctly
				#if ERROR_FILE_AVAILABLE
				log_line("error.log",sms_dates);				
				#endif
							
				return 0;
			}
		}
		else	//no valid log data
		{
			SREG &= ~(0x80);	//guard file operation against interrupt
			count_char = fclose(fptr);
			SREG |= 0x80;			
			
			#ifdef SD_CARD_DEBUG
			sprintf(buffer,"[email_log] result of fclose = %d\r\n",count_char);print1(buffer);
			#endif
			
			#if ERROR_FILE_AVAILABLE	
			sprintf(buffer,"1004#");
			strcat(sms_dates,buffer);
				#ifdef ERROR_FILE_DEBUG
				print1(sms_dates);
				sprintf(buffer,"\r\n");
				print1(buffer);
				#endif
			log_line("error.log",sms_dates);
			// #else			
				// #if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
				// if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
				// {
					// delay_ms(350);	//delay skips the next input scan (all inputs)
									//ensures that the pulse period is valid
					// pulse_instant_pause = 0;
				// }
				// #endif			
			#endif			
			
			//email this
			strcpyre(sms_newMsg.phoneNumber,config.update_address);
			
			sprintf(buffer,"No log data for specified time range (%04u/%02u/%02u %02u:%02u:%02u - %04u/%02u/%02u %02u:%02u:%02u)\n"
			,s_yr,s_mth,s_dy,s_hr,s_min,s_sec,e_yr,e_mth,e_dy,e_hr,e_min,e_sec);
			
			#ifdef EMAIL_DEBUG
			print1(buffer);
			#endif
			
			sprintf(sms_newMsg.txt,"%s",buffer);			
			//modem_send_sms(sms_newMsg.txt,sms_newMsg.phoneNumber);

			sms_newMsg.usePhone=true;
			
			#ifdef _SMS_DEBUG_
			sprintf(buffer,"1\r\n");print1(buffer);
			#endif
			
			msg_sendData();	
			
			return 0;
		}
	}
}

/* Email the log updates since the last regular email update ****************************
Perform a regular update - Emails the complete latest.csv file to the regular update
email address and deletes the file if the mail is sent sucesfully.
Request by AT+EMAILLATEST or "email latest" SMS commands - Emails the complete latest.csv
file to the regular update address and DOES NOT delete the file.

DNS attempts: DNS0 then DNS1
SMTP attempts: 2

If the email fails to send the email retry timer is set (if enabled).
3.xx.8 - email retry timer is disabled by default.
************************************************************************************** */
bool email_latest(char *remove_file) //added 3.xx.7
{
	// -----------------------------------------------------------------------------------------
	// -- GET LOG DATA VARIBLES ----------------------------------------------------------------
	FILE *fptr = NULL;
	unsigned char log_char;
	char read_char = 0;
	unsigned int s_yr;
	unsigned char s_mth, s_dy, s_hr, s_min, s_sec;

	// -----------------------------------------------------------------------------------------
	// -- DNS VARIABLES ------------------------------------------------------------------------
	char *ip_address;

	// -----------------------------------------------------------------------------------------
	// -- SMTP VARIABLES ------------------------------------------------------------------------
	char retries=0;
	bit retry;
	
	#if ERROR_FILE_AVAILABLE
	sprintf(sms_dates,"");	//clear string for error codes
	#endif
	
	tickleRover();

	// *****************************************************************************************
	// ** INITLIALIZE THE SD CARD **************************************************************
	if(!initialize_media())
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to initialize the SD card");print1(buffer);
		#endif		
		
		#if ERROR_FILE_AVAILABLE	
		sprintf(buffer,"1001#");
		strcat(sms_dates,buffer);		
			#ifdef ERROR_FILE_DEBUG
			print1(sms_dates);
			sprintf(buffer,"\r\n");
			print1(buffer);
			#endif		
		log_line("error.log",sms_dates);
		#endif
		
        return 0;
    }
	
	// *****************************************************************************************
	// ** OPEN THE LATEST.CSV LOG FILE *********************************************************
	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
	sedateRover();		
	fptr = fopenc("latest.csv",READ);
	wakeRover();
	SREG |= 0x80;

	if(fptr==NULL)
	{
		//unable to open file
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to open file\r\n");print1(buffer);
		#endif
		
		#if ERROR_FILE_AVAILABLE	
		sprintf(buffer,"1003#");
		strcat(sms_dates,buffer);		
			#ifdef ERROR_FILE_DEBUG
			print1(sms_dates);
			sprintf(buffer,"\r\n");
			print1(buffer);
			#endif		
		log_line("error.log",sms_dates);
		#endif
		
		return 0;
	}
	else	//file exists
	{		
		// *****************************************************************************************
		// ** RESOLVE THE IP ADDRESS ***************************************************************
		
		#ifdef EMAIL_DEBUG
		sprintf(buffer,"[email_latest] Resolving IP address...\r\n");print1(buffer);
		#endif

		do
		{
			retry=FALSE;
			
			// *****************************************************************************************
			// ** OPEN A SOCKET TO SMTP SERVER *********************************************************
			read_char = 'S';		//set connection type 'S' = SMTP server			
			if(!(open_socket()))
			{
				#ifdef SMTP_DEBUG
				sprintf(buffer,"SMTP ERROR: Failed to open connection to SMTP server [%s]\r\n",modem_rx_string);print1(buffer);
				#endif
				#if ERROR_FILE_AVAILABLE	
				sprintf(buffer,"1006.");
				strcat(sms_dates,buffer);
				#ifdef ERROR_FILE_DEBUG
				print1(sms_dates);
				sprintf(buffer,"\r\n");
				print1(buffer);
				#endif
				#endif
				//return;
				retry=TRUE;
			}

			if(!retry)
			{
				// *****************************************************************************************
				// ** WRITE EMAIL HEADER *******************************************************************
				if(!test_smtp_code())
				{
					#ifdef SMTP_DEBUG
					sprintf(buffer,"[email_latest] Timed out waiting for 220 from SMTP server\r\n");print1(buffer);
					#endif
					#if ERROR_FILE_AVAILABLE	
					sprintf(buffer,"1007.");
					strcat(sms_dates,buffer);
					#ifdef ERROR_FILE_DEBUG
					print1(sms_dates);
					sprintf(buffer,"\r\n");
					print1(buffer);
					#endif
					#endif
					retry=TRUE;
				}
				
				if (strstrf(modem_rx_string,"220"))
				{
					sprintf(buffer,"ehlo edacelectronics.com\r\n");print0(buffer);
					#ifdef SMTP_DEBUG
					print1(buffer);
					#endif
					if(!test_smtp_code())
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"[email_latest] Timed out waiting for response to ehlo\r\n");print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"1009.");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						retry=TRUE;
					}

					if(strstrf(modem_rx_string,"250 "))
					{
						sprintf(buffer,"mail from:");print0(buffer);
						#ifdef SMTP_DEBUG
						print1(buffer);
						#endif
						strcpyre(buffer,config.from_address);print0(buffer);
						#ifdef SMTP_DEBUG
						print1(buffer);
						#endif
						sprintf(buffer,"\r\n");print0(buffer);
						#ifdef SMTP_DEBUG
						print1(buffer);
						#endif

						if(!test_smtp_code())
						{
							#ifdef SMTP_DEBUG
							sprintf(buffer,"[email_latest] Timed out waiting for response to mail from\r\n");print1(buffer);
							#endif
							#if ERROR_FILE_AVAILABLE	
							sprintf(buffer,"100B.");
							strcat(sms_dates,buffer);
							#ifdef ERROR_FILE_DEBUG
							print1(sms_dates);
							sprintf(buffer,"\r\n");
							print1(buffer);
							#endif
							#endif
							retry=TRUE;
						}

						if(strstrf(modem_rx_string,"250 "))
						{
							sprintf(buffer,"rcpt to:");print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif
							strcpyre(buffer,config.update_address);print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif
							sprintf(buffer,"\r\n");print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif

							if(!test_smtp_code())
							{
								#ifdef SMTP_DEBUG
								sprintf(buffer,"[email_latest] Timed out waiting for response to rcpt to\r\n");print1(buffer);
								#endif
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"100D.");
								strcat(sms_dates,buffer);
								#ifdef ERROR_FILE_DEBUG
								print1(sms_dates);
								sprintf(buffer,"\r\n");
								print1(buffer);
								#endif
								#endif
								retry=TRUE;
							}

							if(strstrf(modem_rx_string,"250 "))
							{
								sprintf(buffer,"data\r\n");print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif
								if(!test_smtp_code())
								{
									#ifdef SMTP_DEBUG
									sprintf(buffer,"[email_latest] Timed out waiting for response to data request\r\n");print1(buffer);
									#endif
									#if ERROR_FILE_AVAILABLE	
									sprintf(buffer,"100F.");
									strcat(sms_dates,buffer);
									#ifdef ERROR_FILE_DEBUG
									print1(sms_dates);
									sprintf(buffer,"\r\n");
									print1(buffer);
									#endif
									#endif
									retry=TRUE;
								}

								if(strstrf(modem_rx_string,"354 "))
								{
									//DATE and TIME
									// removed because wrong format for SMTP  - needs to be Thu, 21 May 2008 05:33:29 -0700
									// uses server date and time instead which is ok
									//rtc_get_date(&dy,&mth,&yr);rtc_get_time(&hr,&min,&sec);
									//sprintf(buffer,"Date: %d,%d,%d %02d:%02d:%02d\r\n",dy,mth,yr,hr,min,sec);print0(buffer);
									//#ifdef SMTP_DEBUG
									//print1(buffer);
									//#endif
									rtc_get_date(&s_dy,&s_mth,&log_char);rtc_get_time(&s_hr,&s_min,&s_sec);

									//FROM
									sprintf(buffer,"From: ");print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									strcpyre(buffer,config.from_address);print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									sprintf(buffer,"\r\n");print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif

									//TO
									sprintf(buffer,"To: ");print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									strcpyre(buffer,config.update_address);print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									sprintf(buffer,"\r\n");print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif

									//SUBJECT
									//if(logcsv_select)
									if(*remove_file==0)
									{
										sprintf(buffer,"Subject: Latest log entries for ");										
									}
									else
									{
										sprintf(buffer,"Subject: Log update for ");
									}
									//else if(!system0_alarm1)
									//{
									//	sprintf(buffer,"Subject: System log for ");
									//}
									//else if(system0_alarm1)
									//{
									//	sprintf(buffer,"Subject: Alarm log for ");
									//}
									print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									strcpyre(buffer,config.site_name);print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									sprintf(buffer," @ %d/%d/%d %02d:%02d:%02d (",s_dy,s_mth,log_char,s_hr,s_min,s_sec);print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									strcpyre(buffer,serial);print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									sprintf(buffer,")\r\n\r\n");print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
								}
								else
								{
									#ifdef SMTP_DEBUG
									sprintf(buffer,"SMTP ERROR: Failed to enter data mode [%s]\r\n",modem_rx_string);print1(buffer);
									#endif
									#if ERROR_FILE_AVAILABLE	
									sprintf(buffer,"1010.");
									strcat(sms_dates,buffer);
									#ifdef ERROR_FILE_DEBUG
									print1(sms_dates);
									sprintf(buffer,"\r\n");
									print1(buffer);
									#endif
									#endif
									quit_smtp_server();
									retry=TRUE;
								}
							}
							else
							{
								#ifdef SMTP_DEBUG
								sprintf(buffer,"SMTP ERROR: Failed to set recipient address [%s]\r\n",modem_rx_string);print1(buffer);
								#endif
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"100E.");
								strcat(sms_dates,buffer);
								#ifdef ERROR_FILE_DEBUG
								print1(sms_dates);
								sprintf(buffer,"\r\n");
								print1(buffer);
								#endif
								#endif
								quit_smtp_server();
								retry=TRUE;
							}
						}
						else
						{
							#ifdef SMTP_DEBUG
							sprintf(buffer,"SMTP ERROR: Failed to set from address [%s]\r\n",modem_rx_string);print1(buffer);
							#endif
							#if ERROR_FILE_AVAILABLE	
							sprintf(buffer,"100C.");
							strcat(sms_dates,buffer);
							#ifdef ERROR_FILE_DEBUG
							print1(sms_dates);
							sprintf(buffer,"\r\n");
							print1(buffer);
							#endif
							#endif
							quit_smtp_server();
							retry=TRUE;
						}
					}
					else
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"SMTP ERROR: Failed to ehlo [%s]\r\n",modem_rx_string);print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"100A.");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						quit_smtp_server();
						retry=TRUE;
						//then stuck in data mode
					}

				}
				else //NO CARRIER received
				{
					#ifdef SMTP_DEBUG
					sprintf(buffer,"SMTP ERROR: Server failed to reply 220 [%s]\r\n",modem_rx_string);print1(buffer);
					#endif
					#if ERROR_FILE_AVAILABLE	
					sprintf(buffer,"1008.");
					strcat(sms_dates,buffer);
					#ifdef ERROR_FILE_DEBUG
					print1(sms_dates);
					sprintf(buffer,"\r\n");
					print1(buffer);
					#endif
					#endif
					quit_smtp_server();
					retry=TRUE;
				}
			}	// EMAIL HEADER COMPLETE

			// *****************************************************************************************
			// ** WRITE LATEST LOG DATA TO MESSAGE BODY ************************************************
			while(!feof(fptr))
			{
				tickleRover();
				
				SREG &= ~(0x80);	//guard file operation against interrupt
				log_char = fgetc(fptr);
				SREG |= 0x80;
				
				sprintf(&read_char,"%c",log_char);
				//#ifdef SMTP_DEBUG
				//print0(&read_char);
				//sprintf(buffer,"%c[%d] ",read_char,log_char);print1(buffer);
				//#endif
				print0(&read_char);						
			}
			#ifdef SMTP_DEBUG
			sprintf(buffer,"[email_latest] Log data written to message body\r\n");print1(buffer);
			#endif
			
			SREG &= ~(0x80);
			fclose(fptr);
			SREG |= 0x80;
			
			tickleRover();
			
			// *****************************************************************************************
			// ** END SMTP DATA MODE *******************************************************************
			sprintf(buffer,"\r\n.\r\n");print0(buffer);			
			if(test_end_data_mode())
			{		
				// *****************************************************************************************
				// ** QUIT SMTP SERVER *********************************************************************
				quit_smtp_server();
				
				
				//EMAIL SUCCESSFULLY SENT
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"Email sent sucessfully\r\n");print1(buffer);
				#endif
				
				// *****************************************************************************************
				// ** REMOVE THE LATEST.CSV DATA FILE ******************************************************
				if(*remove_file==1)
				//if(read_char==1)
				{
					#ifdef EMAIL_DEBUG
					sprintf(buffer,"Deleting latest.csv\r\n");print1(buffer);
					#endif
					SREG &= ~(0x80);
					removec("latest.csv");
					SREG |= 0x80;
				}
				#ifdef EMAIL_DEBUG
				else
				{
					sprintf(buffer,"Not deleting latest.csv\r\n");print1(buffer);
				}
				#endif
				return 1;
			}
			else
			{
				#ifdef SMTP_DEBUG
				sprintf(buffer,"SMTP ERROR: Data mode could not be ended\r\n");print1(buffer);
				sprintf(buffer,"Email may or may not have been sent\r\n");print1(buffer);
				sprintf(buffer,"Latest date and time has not been saved\r\n");print1(buffer);
				#endif
				#if ERROR_FILE_AVAILABLE	
				sprintf(buffer,"1011#");
				strcat(sms_dates,buffer);
				#ifdef ERROR_FILE_DEBUG
				print1(sms_dates);
				sprintf(buffer,"\r\n");
				print1(buffer);
				#endif
				#endif
				
				retry == FALSE;	//added to stop 1013 error after failure of second SMTP connection attempt
				retries=2;		//must exit while loop because file pointer is at end of file
			}
		}
		while(retries<2 && retry==TRUE);
		
		if(retry)
		{
			SREG &= ~(0x80);	//added 100811
			fclose(fptr);
			SREG |= 0x80;
							
			#ifdef EMAIL_DEBUG
			sprintf(buffer,"SMTP ERROR: Failed to send email\r\n");print1(buffer);
			#endif
			
			#if ERROR_FILE_AVAILABLE	
			sprintf(buffer,"1013#");
			strcat(sms_dates,buffer);				
				#ifdef ERROR_FILE_DEBUG
				print1(sms_dates);
				sprintf(buffer,"\r\n");
				print1(buffer);
				#endif				
			log_line("error.log",sms_dates);			
			#endif
			
			return 0;				
		}
		else		
		{			
			//failed to end data mode correctly
			#if ERROR_FILE_AVAILABLE
			log_line("error.log",sms_dates);			
			#endif
						
			return 0;
		}
	}
}

/* Print the log updates since the last regular email update to RS232 port **************
Requested by AT+PRINTLATEST - prints complete latest.csv file to RS232 port
************************************************************************************** */
void print_latest() //added 3.xx.7
{
	// -----------------------------------------------------------------------------------------
	// -- GET LOG DATA VARIBLES ----------------------------------------------------------------
	FILE *fptr = NULL;
	unsigned char log_char;
	char read_char = 0;
	int count_char = 20;	//changed from char to int because 127 chracters not enough for log lines
	bit dates_valid = 0;	//not sure why but function needs a bit variable defined? (not used)

	// *****************************************************************************************
	// ** INITLIALIZE THE SD CARD **************************************************************
	if(!initialize_media())
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to initialize the SD card [print_log]");print1(buffer);
		#endif
        return;
    }

	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif
	
	// *****************************************************************************************
	// ** OPEN THE LOG FILE	********************************************************************
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_latest] Openning the log file...\r\n");print1(buffer);
	#endif
	
	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
	sedateRover();	
	fptr = fopenc("latest.csv",READ);
	wakeRover();
	SREG |= 0x80;
	
	if(fptr==NULL)
	{
		//unable to open file
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to open file\r\n");print1(buffer);
		#endif
	}
	else	//file exists
	{
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_latest] Log file openned successfully\r\n");print1(buffer);
		#endif
		
		while(!feof(fptr))
		{
			tickleRover();
			SREG &= ~(0x80);	//guard file operation against interrupt
			log_char = fgetc(fptr);
			SREG |= 0x80;
			sprintf(&read_char,"%c",log_char);

			print1(&read_char);
		}
		
		SREG &= ~(0x80);	//guard file operation against interrupt
		count_char = fclose(fptr);
		SREG |= 0x80;
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_latest] result of fclose = %d\r\n",count_char);print1(buffer);
		#endif
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);	//delay skips the next input scan (all inputs)
							//ensures that the pulse period is valid
			pulse_instant_pause = 0;
		}
		#endif

		sprintf(buffer,"Print complete\r\n");print1(buffer);
	}
	return;
}
#endif

/* Print a section of the log.csv file to the RS232 port ********************************
Requested by AT+PRINTLOG - see uart.c for format and keywords of command
************************************************************************************** */
void print_log() //print a log file to uart 1
{
	// -----------------------------------------------------------------------------------------
	// -- GET LOG DATA VARIBLES ----------------------------------------------------------------
	FILE *fptr = NULL;
	unsigned char log_char;
	char read_char = 0;
	int count_char = 20;	//changed from char to int because 127 chracters not enough for log lines
	char date_time[19];
	//last log entry emailed - to be written to hidden file
	unsigned int yr;
	unsigned char mth, dy, hr, min, sec;
	//date and time to start log data
	unsigned int s_yr;
	unsigned char s_mth, s_dy, s_hr, s_min, s_sec;
	//date and time to end log data
	unsigned int e_yr;
	unsigned char e_mth, e_dy, e_hr, e_min, e_sec;
	bit dates_valid = 0;
	bit email_line = 1;

	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_log] Data retrieval: %s\r\n",dates);print1(buffer);
	#endif
	//convert date and time string to chars
	log_get_date_from_string(substr(dates,date_time,0,19),&s_yr,&s_mth,&s_dy,&s_hr,&s_min,&s_sec);
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_log] Start date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
	#endif
	log_get_date_from_string(substr(dates,date_time,20,39),&e_yr,&e_mth,&e_dy,&e_hr,&e_min,&e_sec);
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_log] End date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);
	#endif

	// *****************************************************************************************
	// ** INITLIALIZE THE SD CARD **************************************************************
	if(!initialize_media())
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to initialize the SD card [print_log]");print1(buffer);
		#endif
        return;
    }

	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif
	
	// *****************************************************************************************
	// ** OPEN THE LOG FILE	********************************************************************
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_log] Openning the log file...\r\n");print1(buffer);
	#endif
	
	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
	sedateRover();	
	fptr = fopenc("log.csv",READ);
	wakeRover();
	SREG |= 0x80;
	
	if(fptr==NULL)
	{
		//unable to open file
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to open file\r\n");print1(buffer);
		#endif
		//return 0;
	}
	else	//file exists
	{
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_log] Log file openned successfully\r\n");print1(buffer);
		#endif
		while(!feof(fptr))
		{
			tickleRover();
			SREG &= ~(0x80);	//guard file operation against interrupt
			log_char = fgetc(fptr);
			SREG |= 0x80;
			sprintf(&read_char,"%c",log_char);

			if(count_char<19) //max number of date + time chars = 19
			{
				//if the string is full empty it for the next log date and time
				if(count_char==0)
				{
					sprintf(date_time,"");
				}
				//add the next character to the date + time string
				strncat(date_time,&read_char,1);
			}
			count_char++;
			if(count_char==19)
			{
				log_get_date_from_string(date_time,&yr,&mth,&dy,&hr,&min,&sec);	//prints 2000/01/01 00:00:00 after headings line

				//Compare the date from the current log line and the date saved in the hidden file
				//if the time and date of last log line emailed (from hidden file) is earlier than the
				//date and time of the curent log line set the email_line flag to add line to message body
				if ((yr>s_yr) ||
				(yr==s_yr && mth>s_mth) ||
				(yr==s_yr && mth==s_mth && dy>s_dy) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr>s_hr) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min>s_min) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min==s_min && sec>s_sec))	//> ensures hidden file date is not sent again
				{
					if((yr<e_yr) ||
					(yr==e_yr && mth<e_mth) ||
					(yr==e_yr && mth==e_mth && dy<e_dy) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr<e_hr) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min<e_min) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min==e_min && sec<=e_sec))	//upto and inluding end date and time
					{
						//print the date and time to the message body
						sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
						//sprintf(date_time,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
						print1(dates);

						email_line = 1;
						dates_valid = 1;
					}
					else
						break;
				}
			}

			//sprintf(buffer,"(%d,%d)",count_char,email_line);print1(buffer);
			if(count_char>19 && email_line==1)
			//if(count_char>19)
			{
				print1(&read_char);
			}

			//detect end of logged line
			if(log_char==10) //line feed character \n (10) detected  && carr_rtrn==1
			{
				//reset the char counter for a new line in the log
				count_char = 0;
				email_line = 0;
				//sprintf(buffer,"The last log entry emailed is: %04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
			}
		}
		SREG &= ~(0x80);	//guard file operation against interrupt
		count_char = fclose(fptr);
		SREG |= 0x80;
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_log] result of fclose = %d\r\n",count_char);print1(buffer);
		#endif
		//fptr=NULL;
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);	//delay skips the next input scan (all inputs)
							//ensures that the pulse period is valid
			pulse_instant_pause = 0;
		}
		#endif

		if(dates_valid)
		{
			sprintf(buffer,"Print complete\r\n");print1(buffer);
		}
		else	//no valid log data
		{
			sprintf(buffer,"ERROR: No log data for specified time range (%04u/%02u/%02u %02u:%02u:%02u - ",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
			sprintf(buffer,"%04u/%02u/%02u %02u:%02u:%02u)",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);

		}
	}
}

/* Print a section of the system.log file to the RS232 port *****************************
Requested by AT+PRINTSYSTEM - see uart.c for format and keywords of command
************************************************************************************** */
void print_system() //print the system file to uart 1
{
	// -----------------------------------------------------------------------------------------
	// -- GET LOG DATA VARIBLES ----------------------------------------------------------------
	FILE *fptr = NULL;
	unsigned char log_char;
	char read_char = 0;
	int count_char = 0;	//changed from char to int because 127 chracters not enough for log lines
	char date_time[19];
	//last log entry emailed - to be written to hidden file
	unsigned int yr;
	unsigned char mth, dy, hr, min, sec;
	//date and time to start log data
	unsigned int s_yr;
	unsigned char s_mth, s_dy, s_hr, s_min, s_sec;
	//date and time to end log data
	unsigned int e_yr;
	unsigned char e_mth, e_dy, e_hr, e_min, e_sec;
	bit dates_valid = 0;
	bit email_line = 1;

	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_system] Data retrieval: %s\r\n",dates);print1(buffer);
	#endif
	//convert date and time string to chars
	log_get_date_from_string(substr(dates,date_time,0,19),&s_yr,&s_mth,&s_dy,&s_hr,&s_min,&s_sec);
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_system] Start date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
	#endif
	log_get_date_from_string(substr(dates,date_time,20,39),&e_yr,&e_mth,&e_dy,&e_hr,&e_min,&e_sec);
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_system] End date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);
	#endif

	// *****************************************************************************************
	// ** INITLIALIZE THE SD CARD **************************************************************
	if(!initialize_media())
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to initialize the SD card [print_system]");print1(buffer);
		#endif
        return;
    }
	
	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif

	// *****************************************************************************************
	// ** OPEN THE LOG FILE	********************************************************************
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_system] Openning the log file...\r\n");print1(buffer);
	#endif
	
	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
	sedateRover();	
	fptr = fopenc("system.log",READ);	
	wakeRover();
	SREG |= 0x80;

	if(fptr==NULL)
	{
		//unable to open file
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to open file\r\n");print1(buffer);
		#endif
		//return 0;
	}
	else	//file exists
	{
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_system] Log file openned successfully\r\n");print1(buffer);
		#endif
		//SJL - <-- End of Debug
		while(!feof(fptr))
		{
			tickleRover();
			SREG &= ~(0x80);	//guard file operation against interrupt
			log_char = fgetc(fptr);
			SREG |= 0x80;
			sprintf(&read_char,"%c",log_char);

			if(count_char<19) //max number of date + time chars = 19
			{
				//if the string is full empty it for the next log date and time
				if(count_char==0)
				{
					sprintf(date_time,"");
				}
				//add the next character to the date + time string
				strncat(date_time,&read_char,1);
			}
			count_char++;
			if(count_char==19)
			{
				log_get_date_from_string(date_time,&yr,&mth,&dy,&hr,&min,&sec);	//prints 2000/01/01 00:00:00 after headings line

				//Compare the date from the current log line and the date saved in the hidden file
				//if the time and date of last log line emailed (from hidden file) is earlier than the
				//date and time of the curent log line set the email_line flag to add line to message body
				if ((yr>s_yr) ||
				(yr==s_yr && mth>s_mth) ||
				(yr==s_yr && mth==s_mth && dy>s_dy) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr>s_hr) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min>s_min) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min==s_min && sec>s_sec))	//> ensures hidden file date is not sent again
				{
					if((yr<e_yr) ||
					(yr==e_yr && mth<e_mth) ||
					(yr==e_yr && mth==e_mth && dy<e_dy) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr<e_hr) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min<e_min) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min==e_min && sec<=e_sec))	//upto and inluding end date and time
					{
						//print the date and time to the message body
						sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
						//sprintf(date_time,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
						print1(dates);

						email_line = 1;
						dates_valid = 1;
					}
					else
						break;
				}
			}

			//sprintf(buffer,"(%d,%d)",count_char,email_line);print1(buffer);
			if(count_char>19 && email_line==1)
			//if(count_char>19)
			{
				print1(&read_char);
			}

			//detect end of logged line
			if(log_char==10) //line feed character \n (10) detected  && carr_rtrn==1
			{
				//reset the char counter for a new line in the log
				count_char = 0;
				email_line = 0;
				//sprintf(buffer,"The last log entry emailed is: %04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
			}
		}
		SREG &= ~(0x80);	//guard file operation against interrupt
		count_char = fclose(fptr);
		SREG |= 0x80;
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_system] result of fclose = %d\r\n",count_char);print1(buffer);
		#endif
		//fptr=NULL;
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);	//delay skips the next input scan (all inputs)
							//ensures that the pulse period is valid
			pulse_instant_pause = 0;
		}
		#endif

		if(dates_valid)
		{
			sprintf(buffer,"Print complete\r\n");print1(buffer);
		}
		else	//no valid log data
		{
			sprintf(buffer,"ERROR: No log data for specified time range (%04u/%02u/%02u %02u:%02u:%02u - ",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
			sprintf(buffer,"%04u/%02u/%02u %02u:%02u:%02u)",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);

		}
	}
}

/* Print a section of the system.log file to the RS232 port *****************************
Requested by AT+PRINTALARM - see uart.c for format and keywords of command
************************************************************************************** */
void print_alarm() //print the alarm file to uart 1
{
	// -----------------------------------------------------------------------------------------
	// -- GET LOG DATA VARIBLES ----------------------------------------------------------------
	FILE *fptr = NULL;
	unsigned char log_char;
	char read_char = 0;
	int count_char = 0;	//changed from char to int because 127 chracters not enough for log lines
	char date_time[19];
	//last log entry emailed - to be written to hidden file
	unsigned int yr;
	unsigned char mth, dy, hr, min, sec;
	//date and time to start log data
	unsigned int s_yr;
	unsigned char s_mth, s_dy, s_hr, s_min, s_sec;
	//date and time to end log data
	unsigned int e_yr;
	unsigned char e_mth, e_dy, e_hr, e_min, e_sec;
	bit dates_valid = 0;
	bit email_line = 1;

	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_alarm] Data retrieval: %s\r\n",dates);print1(buffer);
	#endif
	//convert date and time string to chars
	log_get_date_from_string(substr(dates,date_time,0,19),&s_yr,&s_mth,&s_dy,&s_hr,&s_min,&s_sec);
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_alarm] Start date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
	#endif
	log_get_date_from_string(substr(dates,date_time,20,39),&e_yr,&e_mth,&e_dy,&e_hr,&e_min,&e_sec);
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_alarm] End date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);
	#endif

	// *****************************************************************************************
	// ** INITLIALIZE THE SD CARD **************************************************************
	if(!initialize_media())
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to initialize the SD card [print_alarm]");print1(buffer);
		#endif
        return;
    }
	
	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif

	// *****************************************************************************************
	// ** OPEN THE LOG FILE	********************************************************************
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_alarm] Openning the log file...\r\n");print1(buffer);
	#endif
	
	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
	sedateRover();	
	fptr = fopenc("alarm.log",READ);
	wakeRover();
	SREG |= 0x80;

	if(fptr==NULL)
	{
		//unable to open file
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to open file\r\n");print1(buffer);
		#endif
		//return 0;
	}
	else	//file exists
	{
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_alarm] Log file openned successfully\r\n");print1(buffer);
		#endif
		//SJL - <-- End of Debug
		while(!feof(fptr))
		{
			tickleRover();
			SREG &= ~(0x80);	//guard file operation against interrupt
			log_char = fgetc(fptr);
			SREG |= 0x80;
			sprintf(&read_char,"%c",log_char);

			if(count_char<19) //max number of date + time chars = 19
			{
				//if the string is full empty it for the next log date and time
				if(count_char==0)
				{
					sprintf(date_time,"");
				}
				//add the next character to the date + time string
				strncat(date_time,&read_char,1);
			}
			count_char++;
			if(count_char==19)
			{
				log_get_date_from_string(date_time,&yr,&mth,&dy,&hr,&min,&sec);	//prints 2000/01/01 00:00:00 after headings line

				//Compare the date from the current log line and the date saved in the hidden file
				//if the time and date of last log line emailed (from hidden file) is earlier than the
				//date and time of the curent log line set the email_line flag to add line to message body
				if ((yr>s_yr) ||
				(yr==s_yr && mth>s_mth) ||
				(yr==s_yr && mth==s_mth && dy>s_dy) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr>s_hr) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min>s_min) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min==s_min && sec>s_sec))	//> ensures hidden file date is not sent again
				{
					if((yr<e_yr) ||
					(yr==e_yr && mth<e_mth) ||
					(yr==e_yr && mth==e_mth && dy<e_dy) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr<e_hr) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min<e_min) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min==e_min && sec<=e_sec))	//upto and inluding end date and time
					{
						//print the date and time to the message body
						sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
						//sprintf(date_time,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
						print1(dates);

						email_line = 1;
						dates_valid = 1;
					}
					else
						break;
				}
			}

			//sprintf(buffer,"(%d,%d)",count_char,email_line);print1(buffer);
			if(count_char>19 && email_line==1)
			//if(count_char>19)
			{
				print1(&read_char);
			}

			//detect end of logged line
			if(log_char==10) //line feed character \n (10) detected  && carr_rtrn==1
			{
				//reset the char counter for a new line in the log
				count_char = 0;
				email_line = 0;
				//sprintf(buffer,"The last log entry emailed is: %04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
			}
		}
		SREG &= ~(0x80);	//guard file operation against interrupt
		count_char = fclose(fptr);
		SREG |= 0x80;
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_alarm] result of fclose = %d\r\n",count_char);print1(buffer);
		#endif
		//fptr=NULL;
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);	//delay skips the next input scan (all inputs)
							//ensures that the pulse period is valid
			pulse_instant_pause = 0;
		}
		#endif

		if(dates_valid)
		{
			sprintf(buffer,"Print complete\r\n");print1(buffer);
		}
		else	//no valid log data
		{
			sprintf(buffer,"ERROR: No log data for specified time range (%04u/%02u/%02u %02u:%02u:%02u - ",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
			sprintf(buffer,"%04u/%02u/%02u %02u:%02u:%02u)",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);

		}
	}
}

/* Print a section of the error.log file to the RS232 port ******************************
error.log contains error information from the email process - log updates and other email
Requested by AT+PRINTERROR - see uart.c for format and keywords of command
************************************************************************************** */
#if ERROR_FILE_AVAILABLE	//320 + 321 only - email error file
void print_error() //print the alarm file to uart 1
{
	// -----------------------------------------------------------------------------------------
	// -- GET LOG DATA VARIBLES ----------------------------------------------------------------
	FILE *fptr = NULL;
	unsigned char log_char;
	char read_char = 0;
	int count_char = 0;	//changed from char to int because 127 chracters not enough for log lines
	char date_time[19];
	//last log entry emailed - to be written to hidden file
	unsigned int yr;
	unsigned char mth, dy, hr, min, sec;
	//date and time to start log data
	unsigned int s_yr;
	unsigned char s_mth, s_dy, s_hr, s_min, s_sec;
	//date and time to end log data
	unsigned int e_yr;
	unsigned char e_mth, e_dy, e_hr, e_min, e_sec;
	bit dates_valid = 0;
	bit email_line = 1;

	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_error] Data retrieval: %s\r\n",dates);print1(buffer);
	#endif
	//convert date and time string to chars
	log_get_date_from_string(substr(dates,date_time,0,19),&s_yr,&s_mth,&s_dy,&s_hr,&s_min,&s_sec);
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_error] Start date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
	#endif
	log_get_date_from_string(substr(dates,date_time,20,39),&e_yr,&e_mth,&e_dy,&e_hr,&e_min,&e_sec);
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_error] End date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);
	#endif

	// *****************************************************************************************
	// ** INITLIALIZE THE SD CARD **************************************************************
	if(!initialize_media())
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to initialize the SD card [print_error]");print1(buffer);
		#endif
        return;
    }
	
	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif

	// *****************************************************************************************
	// ** OPEN THE LOG FILE	********************************************************************
	#ifdef SD_CARD_DEBUG
	sprintf(buffer,"[print_error] Openning the log file...\r\n");print1(buffer);
	#endif
	
	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
	sedateRover();	
	fptr = fopenc("error.log",READ);
	wakeRover();
	SREG |= 0x80;

	if(fptr==NULL)
	{
		//unable to open file
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to open file\r\n");print1(buffer);
		#endif
		//return 0;
	}
	else	//file exists
	{
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_error] Log file openned successfully\r\n");print1(buffer);
		#endif
		//SJL - <-- End of Debug
		while(!feof(fptr))
		{
			tickleRover();
			SREG &= ~(0x80);	//guard file operation against interrupt
			log_char = fgetc(fptr);
			SREG |= 0x80;
			sprintf(&read_char,"%c",log_char);

			if(count_char<19) //max number of date + time chars = 19
			{
				//if the string is full empty it for the next log date and time
				if(count_char==0)
				{
					sprintf(date_time,"");
				}
				//add the next character to the date + time string
				strncat(date_time,&read_char,1);
			}
			count_char++;
			if(count_char==19)
			{
				log_get_date_from_string(date_time,&yr,&mth,&dy,&hr,&min,&sec);	//prints 2000/01/01 00:00:00 after headings line

				//Compare the date from the current log line and the date saved in the hidden file
				//if the time and date of last log line emailed (from hidden file) is earlier than the
				//date and time of the curent log line set the email_line flag to add line to message body
				if ((yr>s_yr) ||
				(yr==s_yr && mth>s_mth) ||
				(yr==s_yr && mth==s_mth && dy>s_dy) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr>s_hr) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min>s_min) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min==s_min && sec>s_sec))	//> ensures hidden file date is not sent again
				{
					if((yr<e_yr) ||
					(yr==e_yr && mth<e_mth) ||
					(yr==e_yr && mth==e_mth && dy<e_dy) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr<e_hr) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min<e_min) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min==e_min && sec<=e_sec))	//upto and inluding end date and time
					{
						//print the date and time to the message body
						sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
						//sprintf(date_time,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
						print1(dates);

						email_line = 1;
						dates_valid = 1;
					}
					else
						break;
				}
			}

			//sprintf(buffer,"(%d,%d)",count_char,email_line);print1(buffer);
			if(count_char>19 && email_line==1)
			//if(count_char>19)
			{
				print1(&read_char);
			}

			//detect end of logged line
			if(log_char==10) //line feed character \n (10) detected  && carr_rtrn==1
			{
				//reset the char counter for a new line in the log
				count_char = 0;
				email_line = 0;
				//sprintf(buffer,"The last log entry emailed is: %04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
			}
		}
		SREG &= ~(0x80);	//guard file operation against interrupt
		count_char = fclose(fptr);
		SREG |= 0x80;
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"[print_error] result of fclose = %d\r\n",count_char);print1(buffer);
		#endif
		//fptr=NULL;
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);	//delay skips the next input scan (all inputs)
							//ensures that the pulse period is valid
			pulse_instant_pause = 0;
		}
		#endif

		if(dates_valid)
		{
			sprintf(buffer,"Print complete\r\n");print1(buffer);
		}
		else	//no valid log data
		{
			sprintf(buffer,"ERROR: No log data for specified time range (%04u/%02u/%02u %02u:%02u:%02u - ",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
			sprintf(buffer,"%04u/%02u/%02u %02u:%02u:%02u)",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);

		}
	}
}
#endif

/* Check validity of user entered date/time string **************************************
Check the length of the start date/time: location of '-' must be in
position 5 or greater as shortest valid start date/time is keyword 'start'. 

Check the length of the user entered date/time string: string length must be longer than
8 characters long - shortest string possible 'start-end'

Tests for keywords and replaces with date and time:
'start' replaced with '1900/01/01 00:00:00'
'end' replaced with '-3000/01/01 00:00:00'

User is allowed to enter a date with no time - if this is detected then the time
'00:00:00' is added.
************************************************************************************** */
bool cmdDates_to_mmcDates(char *command_dateTime)
{
	char *c;
	unsigned char charPos=0;	//, charPos2=99; //charPos2 commented out v3.xx.7
	char lastLog[19];
	char *lastLog_dateTime;

	c = strchr(command_dateTime,'=')+1;
	//sprintf(dates,"%s\r\n",c);
	sprintf(dates,"%s",c);	
	#ifdef SD_CARD_DEBUG
	print1(dates);sprintf(buffer,"[1] \r\n");print1(buffer);
	#endif

	charPos = strpos (c,'-');

	if(charPos<5 || strlen(c)<9)	//string entered is invalid if '-' is found at index less than 5 (index is 5 for 'start-')
	{								//shortest valid string is 9 long start-end
		sprintf(buffer,"CMD ERROR: Invalid date and/or time entered");print1(buffer);
		return 0;
	}

	//if(strstrf(c,"latest"))			commented out v3.xx.7
	//	charPos2 = strpos (c,'l');

	if(strstrf(c,"start"))
	{
		//sprintf(buffer,"start word found");print1(buffer);
		//replace start with a date and time - 1900/01/01 00:00:00
		sprintf(dates,"1900/01/01 00:00:00");	//print1(dates);
		#ifdef SD_CARD_DEBUG
		print1(dates);sprintf(buffer,"[2] \r\n");print1(buffer);
		#endif
	}
	/*#if EMAIL_AVAILABLE	//commented out v3.xx.7
	else if(charPos2 == 0)
	{
		//dummy string (lastLog) is sent to email_log_read_date so completed string may be successfully returned
		sprintf(lastLog,"xxxx/xx/xx xx:xx:xx");
		//lastLog_dateTime - pointer to the time and date string extracted from the hidden file on SD card
		lastLog_dateTime=email_log_read_date(lastLog);
		if(!lastLog_dateTime || lastLog_dateTime==0)
		{
			#ifdef SD_CARD_DEBUG
			sprintf(buffer,"ERROR: latest date not found - using start\r\n");print1(buffer);
			#endif
			//return 0;
			sprintf(dates,"1900/01/01 00:00:00");
		}
		else
		{
			sprintf(dates,"%s",lastLog_dateTime);	//print1(dates);
		}
		
		//sprintf(dates,"1900/01/01 00:00:00");	//DEBUG
		
	}
	#endif
	*/
	else
	{
		//sprintf(buffer,"start date found");print1(buffer);
		substr(c,dates,0,charPos);	//print1(dates);
		
		if(charPos==10)
		{
			//strncat(dates," 00:00:00",9);
			sprintf(buffer," 00:00:00");
			strncat(dates,buffer,9);			
		}
		#ifdef SD_CARD_DEBUG
		print1(dates);sprintf(buffer,"[3] \r\n");print1(buffer);
		#endif
	}

	if(strstrf(c,"end"))
	{
		//replace end with a date and time - 3000/01/01 00:00:00
		//strncat(dates,"-3000/01/01 00:00:00",20);	//print1(dates);		
		sprintf(buffer,"-3000/01/01 00:00:00");
		strncat(dates,buffer,20);
		
		#ifdef SD_CARD_DEBUG
		print1(dates);sprintf(buffer,"[4] \r\n");print1(buffer);
		#endif
	}
	//else if(charPos2>0 && charPos2<99)
	
	/*#if EMAIL_AVAILABLE		//commented out v3.xx.7
	else if(charPos2>charPos && charPos2<99)
	{
		//dummy string (lastLog) is sent to email_log_read_date so completed string may be successfully returned
		sprintf(lastLog,"xxxx/xx/xx xx:xx:xx");
		//lastLog_dateTime - pointer to the time and date string extracted from the hidden file on SD card
		lastLog_dateTime=email_log_read_date(lastLog);
		if(!lastLog_dateTime || lastLog_dateTime==0)
		{
			#ifdef SD_CARD_DEBUG
			sprintf(buffer,"ERROR: latest date not found\r\n");print1(buffer);
			#endif
			return 0;
		}
		else
		{
			strncat(dates,"-",1);
			strncat(dates,lastLog_dateTime,19);	//print1(dates);
		}

	}
	#endif
	*/
	else
	{		
		if( (strlen(command_dateTime) - strpos(command_dateTime,'-')) == 11)
		{
			//only the date has been entered so concatenate with ' 00:00:00'
			//strncat(dates,substr(c,c,charPos,strlen(command_dateTime)),11);
			//strncat(dates," 00:00:00",9);
			
			sprintf(buffer,"%s 00:00:00",substr(c,c,charPos,strlen(command_dateTime)) );
			strncat(dates,buffer,20);
			#ifdef SD_CARD_DEBUG
			print1(dates);sprintf(buffer,"[5] \r\n");print1(buffer);
			#endif
		}
		else
		{
			//otherwise copy date and time entered to the string
			//strncat(dates,substr(c,c,charPos,strlen(command_dateTime)),20);	//print1(dates);
			
			sprintf(buffer,"%s",substr(c,c,charPos,strlen(command_dateTime)) );
			strncat(dates,buffer,20);
			#ifdef SD_CARD_DEBUG
			print1(dates);sprintf(buffer,"[6] \r\n");print1(buffer);
			#endif	
		}
	}

	return 1;
}

/* Email options - add flags to end of dates string *************************************
************************************************************************************** */
#if EMAIL_AVAILABLE
void concat_0()	//concatenate dates string with ,0
{
	strncat(dates,",0",2);
}
void concat_1() //concatenate dates string with ,1
{
	strncat(dates,",1",2);
}
void concat_2() //concatenate dates string with ,2
{
	strncat(dates,",2",2);
}
#endif

/* Extract year, month, day, hours, minutes, seconds from string ************************
String format: yyyy/mm/dd hh:mm:ss
************************************************************************************** */
void log_get_date_from_string(char *c,unsigned int *year,unsigned char *month,unsigned char *day,
                                unsigned char *hour,unsigned char *minute,unsigned char *second)
{
    //takes a (partial) date in YYYY/MM/DD HH:MM:SS format
    char length = 0;

    length = strlen(c);
    if(length>=4)
    {
        c[4] = '\0';
        *year = atoi(c);
        c+=5;
    } else *year = 2000;
    if(length>=7)
    {
        c[2] = '\0';
        *month = atoi(c);
        c+=3;
    } else *month = 1;
    if(length>=10)
    {
        c[2] = '\0';
        *day = atoi(c);
        c+=3;
    } else *day = 1;
    if(length>=13)
    {
        c[2] = '\0';
        *hour = atoi(c);
        c+=3;
    } else *hour = 0;
    if(length>=16)
    {
        c[2] = '\0';
        *minute = atoi(c);
        c+=3;
    } else *minute = 0;
    if(length>=19)
    {
        c[2] = '\0';
        *second = atoi(c);
        c+=3;
    } else *second = 0;
    return;
}

/* Clear both the UART channels *********************************************************
While the character receive counter on UART 0 and 1 is non-zero get the character from
the UART.
************************************************************************************** */
void clear_channel()
{
    if(modem_TMEnabled)
        while(rx_counter0>0) getchar();
    else
        while(rx_counter1>0) getchar1();
}

/* Log an alarm event to the alarm.log file *********************************************
Function creates file if required.
************************************************************************************** */
void log_alarm(char event, char param)
{
    FILE *fptr = NULL;
    Event e;
    char year,month,day,hour,minute,second;

    if(!(event==ALARM_A || event == RESET_A || event==ALARM_B || event==RESET_B))
    {
        return;
    }
    if(!(config.input[param].log_type&LOG_ALARM)) //check if we want to log this at all
    {
        return;
    }

    if(config.input[param].type==DIGITAL&&event==ALARM_B)
        event = RESET_A;
    if(config.input[param].type==DIGITAL&&event==RESET_B)
        event = RESET_B;

    initialize_media();
	
	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif

	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
    sedateRover();	
    fptr = fopenc("ALARM.LOG",APPEND);
	wakeRover();
	SREG |= 0x80;
	
    if(fptr == NULL)
	{
		SREG &= ~(0x80);	//guard file operation against interrupt
        fptr = fcreatec("ALARM.LOG",0);
		SREG |= 0x80;
	}
    if(fptr == NULL)
    {
        e.type = sms_MMC_ACCESS_FAIL;
        queue_push(&q_modem,&e);
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);	//delay skips the next input scan (all inputs)
							//ensures that the pulse period is valid
			pulse_instant_pause = 0;
		}
		#endif
		
        return;
    }
    else mmc_fail_sent=false;

    rtc_get_date(&day,&month,&year);
    rtc_get_time(&hour,&minute,&second);

    switch(event)
    {
        case ALARM_A:
            strcpyre(buffer,config.input[param].alarm[ALARM_A].alarm_msg);
            break;
        case ALARM_B:
            strcpyre(buffer,config.input[param].alarm[ALARM_B].alarm_msg);
            break;
        case RESET_A:
            strcpyre(buffer,config.input[param].alarm[ALARM_A].reset_msg);
            break;
        case RESET_B:
            strcpyre(buffer,config.input[param].alarm[ALARM_B].reset_msg);
            break;
        default:
            sprintf(buffer,"");
            break;
    }
    if(strcmpf(buffer,"")==0)
    {
		SREG &= ~(0x80);	//guard file operation against interrupt
        fclose(fptr);
		SREG |= 0x80;
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);	//delay skips the next input scan (all inputs)
							//ensures that the pulse period is valid
			pulse_instant_pause = 0;
		}
		#endif
		
        return;
    }

	SREG &= ~(0x80);	//guard file operation against interrupt
    fprintf(fptr,"%d/%02d/%02d %02d:%02d:%02d,",year+2000,month,day,hour,minute,second);
    SREG |= 0x80;
	
	SREG &= ~(0x80);	//guard file operation against interrupt
	fprintf(fptr,"%s",buffer);
	SREG |= 0x80;

    if (config.input[param].type != DIGITAL)
    {
		SREG &= ~(0x80);	//guard file operation against interrupt
		fprintf(fptr," Current Value: %2.1f", (config.input[param].conv_grad * input[param].ADCVal) + config.input[param].conv_int);
		SREG |= 0x80;
		
		strcpyre(buffer,config.input[param].units);
		
		SREG &= ~(0x80);	//guard file operation against interrupt
        fprintf(fptr,"%s",buffer);
		SREG |= 0x80;
		
        if(config.input[param].type == PULSE)
        {
            switch(config.pulse[param-6].period)
            {
                case SECONDS:
					SREG &= ~(0x80);	//guard file operation against interrupt
                    fprintf(fptr,"/s");
					SREG |= 0x80;
                    break;
                case MINUTES:
					SREG &= ~(0x80);	//guard file operation against interrupt
                    fprintf(fptr,"/m");
					SREG |= 0x80;
                    break;
                case HOURS:
					SREG &= ~(0x80);	//guard file operation against interrupt
                    fprintf(fptr,"/h");
					SREG |= 0x80;
					break;
            }
        }
    }
	SREG &= ~(0x80);	//guard file operation against interrupt
    fprintf(fptr,"\r\n");
    SREG |= 0x80;
	
	SREG &= ~(0x80);	//guard file operation against interrupt
	fclose(fptr);
	SREG |= 0x80;
	
	#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
	{
		delay_ms(350);	//delay skips the next input scan (all inputs)
						//ensures that the pulse period is valid
		pulse_instant_pause = 0;
	}
	#endif
}

/* Log an alarm event to the alarm.log file *********************************************
Function creates file if required.
************************************************************************************** */
bool log_line(flash char *filename,char *c)
{
    FILE *fptr = NULL;

    char year,month,day,hour,minute,second;
   
	if(!initialize_media())
    {
        return false;
    }
	
	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif
	
	SREG &= ~(0x80);
	tickleRover();
	sedateRover();		
	fptr = fopenc(filename,APPEND);
	wakeRover();
	SREG |= 0x80;
	
    if(fptr == NULL)
	{	
		SREG &= ~(0x80);
        fptr = fcreatec(filename,0);
		SREG |= 0x80;
	}
    if(fptr == NULL)
	{
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);	//delay skips the next input scan (all inputs)
							//ensures that the pulse period is valid
			pulse_instant_pause = 0;
		}
		#endif
	
        return false;
	}
    
	rtc_get_date(&day,&month,&year);
    rtc_get_time(&hour,&minute,&second);	
    
	SREG &= ~(0x80);
	fprintf(fptr,"%d/%02d/%02d %02d:%02d:%02d,",year+2000,month,day,hour,minute,second);
	fprintf(fptr,"%s\r\n",c);    
	fclose(fptr);
	SREG |= 0x80;
	
	#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
	{
		delay_ms(350);	//delay skips the next input scan (all inputs)
						//ensures that the pulse period is valid
		pulse_instant_pause = 0;
	}
	#endif

    return true;
}

enum {
    CREATE_NEW = 'a',
    MMC_APPEND = 'b',
    MMC_CLOSE = 'c',
    MMC_READ = 'd',
    MMC_DELETE = 'e',
    QUIT = 'q'
};

/* void mmc_profiler()	//unused
void mmc_profiler()
{
    char operation;
    int count = 0;
    int i;
    FILE *fptr;
    sprintf(buffer,"Welcome to MMC profiling\r\nPlease enter your desired test: ");
    print(buffer);
    initialize_media();
    while(1)
    {
        tickleRover();
        if(rx_counter1)
            operation = getchar1();
        putchar1(operation);


        switch(operation)
        {
            case CREATE_NEW:
                for(i=0;i<64;i++)
                {
                    tickleRover();
                    sprintf(buffer,"test%d.txt",count++);
                    _active_led_toggle();
                    fptr = fcreate(buffer,APPEND);
                    _active_led_toggle();
                    putchar1('.');
                    if(fptr)
                        fprintf(fptr,"%s",buffer);
                    fclose(fptr);
                   // remove(buffer);
                }
                break;
            case MMC_APPEND:
                fptr = fcreatec("append.txt",APPEND);
                if(fptr)
                    for(i=0;i<64;i++)
                    {
                        tickleRover();
                        _active_led_toggle();
                        fprintf(fptr,"appending this\r\n");
                        _active_led_toggle();
                        putchar1('.');
                    }
                fclose(fptr);
                removec("append.txt");
                break;
            case MMC_CLOSE:
                for(i=0;i<64;i++);
                {
                    tickleRover();
                    fptr = fopenc("log.csv",APPEND);
                    delay_ms(100);
                    _active_led_toggle();
                    fclose(fptr);
                    _active_led_toggle();
                    putchar1('.');
                }
                break;
            case MMC_READ:
                fptr = fopenc("log.csv",READ);
                for(i=0;i<64;i++)
                {
                    _active_led_toggle();
                    mmc_gets(buffer,128,fptr);
                    _active_led_toggle();
                    putchar1('.');
                }
                fclose(fptr);
                break;
            case MMC_DELETE:
                for(i=0;i<64;i++)
                {
                    tickleRover();
                    sprintf(buffer,"test%d.txt",count++);
                    fptr = fcreate(buffer,APPEND);
                    if(fptr)
                        fprintf(fptr,"%s",buffer);
                    delay_ms(100);
                    fclose(fptr);
                    delay_ms(100);
                     _active_led_toggle();
                    remove(buffer);
                    _active_led_toggle();
                    delay_ms(100);
                    sprintf(buffer,"removed %s, fptr = 0x%X",buffer,fptr);
                    print(buffer);
                    putchar1('.');
                }
                break;
            case QUIT:
                sprintf(buffer,"OK\r\n");
                print(buffer);
                return;
            default:
                break;
        }
    }
}
*/

/* Clear the data from the SD card ******************************************************
Remove log.csv and latest.csv from the SD card.
************************************************************************************** */
bool mmc_clear_data()
{
	int check;

    initialize_media();
	
	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif
	
	SREG &= ~(0x80);
	//removec("hidden.dat");
	removec("latest.csv");
	SREG |= 0x80;
    
	SREG &= ~(0x80);
	check=removec("LOG.CSV");
	SREG |= 0x80;
	
	#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
	{
		delay_ms(350);	//delay skips the next input scan (all inputs)
						//ensures that the pulse period is valid
		pulse_instant_pause = 0;
	}
	#endif
	
	if(check==0)
		return 0;
	else
		return 1;
	
    //return removec("LOG.CSV");
}

#if EMAIL_AVAILABLE
/* Send an email - alarm, auto-report messages ******************************************
Single email is sent to all alarm contacts (email addresses added with RCPT: rather than
a seperate email sent to each contact).

DNS attempts: DNS0 then DNS1
SMTP attempts: 2

Any errors are logged to error.log file.

WARNING: There is no retry timer implemented for these messages so if an alarm message
fails to send after 2 DNS atempts and 2 SMTP attempts then the email is not sent!
IT IS RECOMMENDED THAT EMAIL ALARM MESSAGES ARE ONLY USED AS BACKUP TO SMS MESSAGES
************************************************************************************** */
bool email_send()
{
	// -----------------------------------------------------------------------------------------
	// -- GET LOG DATA VARIBLES ----------------------------------------------------------------
	FILE *fptr = NULL;
	char read_char = 0;
	unsigned char yr, mth, dy, hr, min, sec;
	char i, int1, int2;

	// -----------------------------------------------------------------------------------------
	// -- DNS VARIABLES ------------------------------------------------------------------------
	char *ip_address;

	// -----------------------------------------------------------------------------------------
	// -- SMTP VARIABLES ------------------------------------------------------------------------
	char retries=0;
	bit retry;
	
	tickleRover();
	
	#if ERROR_FILE_AVAILABLE
	sprintf(sms_dates,"");	//clear string for error codes
	#endif

	//#ifdef EMAIL_DEBUG
	//strcpye(config.site_name,"test8");
	//strcpye(serial,"0123456");
	//strcpye(config.from_address,"test8@edacelectronics.com");
	//strcpye(config.update_address,"samlea@edacelectronics.com");	
	//#endif	

	// *****************************************************************************************
	// ** RESOLVE THE IP ADDRESS ***************************************************************
	//#ifdef EMAIL_DEBUG
	//strcpyef(config.smtp_serv,"smtp2.vodafone.net.nz");
	//#endif
	
	do
	{
		retry=FALSE;
		// *****************************************************************************************
		// ** OPEN A SOCKET TO SMTP SERVER *********************************************************
		read_char = 'S';		//set connection type
		
		if(!(open_socket()))
		{
			#ifdef SMTP_DEBUG
			sprintf(buffer,"SMTP ERROR: Failed to open connection to SMTP server [%s]\r\n",modem_rx_string);print1(buffer);
			#endif
			#if ERROR_FILE_AVAILABLE	
			sprintf(buffer,"1106.");
			strcat(sms_dates,buffer);
			#ifdef ERROR_FILE_DEBUG
			print1(sms_dates);
			sprintf(buffer,"\r\n");
			print1(buffer);
			#endif
			#endif
			//return;
			retry=TRUE;
		}

		if(!retry)
		{
			// *****************************************************************************************
			// ** WRITE EMAIL HEADER *******************************************************************
			if(!test_smtp_code())
			{
				#ifdef SMTP_DEBUG
				sprintf(buffer,"[email_log] Timed out waiting for 220 from SMTP server\r\n");print1(buffer);
				#endif
				#if ERROR_FILE_AVAILABLE	
				sprintf(buffer,"1107.");
				strcat(sms_dates,buffer);
				#ifdef ERROR_FILE_DEBUG
				print1(sms_dates);
				sprintf(buffer,"\r\n");
				print1(buffer);
				#endif
				#endif
				retry=TRUE;
			}
			
			if (strstrf(modem_rx_string,"220"))
			{
				//#ifdef SMTP_DEBUG
				//sprintf(buffer,"[email header]\r\n");print1(buffer);
				//sprintf(buffer,"Connected to VF SMTP server successfully, saying ehlo\r\n");print1(buffer);
				//#endif

				sprintf(buffer,"ehlo edacelectronics.com\r\n");print0(buffer);
				#ifdef SMTP_DEBUG
				print1(buffer);
				#endif
				if(!test_smtp_code())
				{
					#ifdef SMTP_DEBUG
					sprintf(buffer,"[email_log] Timed out waiting for response to ehlo\r\n");print1(buffer);
					#endif
					#if ERROR_FILE_AVAILABLE	
					sprintf(buffer,"1109.");
					strcat(sms_dates,buffer);
					#ifdef ERROR_FILE_DEBUG
					print1(sms_dates);
					sprintf(buffer,"\r\n");
					print1(buffer);
					#endif
					#endif
					retry=TRUE;
				}

				if(strstrf(modem_rx_string,"250 "))
				{
					//#ifdef SMTP_DEBUG
					//sprintf(buffer,"Setting from address\r\n");print1(buffer);
					//#endif
					sprintf(buffer,"mail from:");print0(buffer);
					#ifdef SMTP_DEBUG
					print1(buffer);
					#endif
					strcpyre(buffer,config.from_address);print0(buffer);
					#ifdef SMTP_DEBUG
					print1(buffer);
					#endif
					sprintf(buffer,"\r\n");print0(buffer);
					#ifdef SMTP_DEBUG
					print1(buffer);
					#endif

					if(!test_smtp_code())
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"[email_log] Timed out waiting for response to mail from\r\n");print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"110B.");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						retry=TRUE;
					}

					if(strstrf(modem_rx_string,"250 "))
					{								
						if(sms_newMsg.usePhone==true)
						{
							sprintf(buffer,"rcpt to:%s",sms_newMsg.phoneNumber);print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif
							sprintf(buffer,"\r\n");print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif
							
							if(!test_smtp_code())
							{
								#ifdef SMTP_DEBUG
								sprintf(buffer,"[email_log] Timed out waiting for response to rcpt to\r\n");print1(buffer);
								#endif
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"110D.");
								strcat(sms_dates,buffer);
								#ifdef ERROR_FILE_DEBUG
								print1(sms_dates);
								sprintf(buffer,"\r\n");
								print1(buffer);
								#endif
								#endif
								retry=TRUE;
								break;
							}
						}
						else
						{
							//#ifdef SMTP_DEBUG
							//sprintf(buffer,"sms_newMsg.contactList=%u\r\n",sms_newMsg.contactList);print1(buffer);
							//#endif
							for(i=0;i<MAX_CONTACTS;i++)
							{
								//#ifdef SMTP_DEBUG
								//sprintf(buffer,"i=%d,sms_newMsg.contactList & (0x01 << i)=%d\r\n",i,(sms_newMsg.contactList & (0x01 << i)));print1(buffer);
								//#endif
								if(sms_newMsg.contactList & ((unsigned int)0x01 << i))
								{
									//#ifdef SMTP_DEBUG
									//sprintf(buffer,"Contact detected\n\n");print1(buffer);
									//#endif
									if(contact_getType(i)==EMAIL)
									{
										#ifdef SMTP_DEBUG
										sprintf(buffer,"Email Add found at contact %d\r\n",i+1);print1(buffer);
										#endif
									
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
										}									
										
										sprintf(buffer,"rcpt to:%s",sms_newMsg.phoneNumber);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										sprintf(buffer,"\r\n");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										
										if(!test_smtp_code())
										{
											#ifdef SMTP_DEBUG
											sprintf(buffer,"[email_log] Timed out waiting for response to rcpt to\r\n");print1(buffer);
											#endif
											#if ERROR_FILE_AVAILABLE	
											sprintf(buffer,"110D.");
											strcat(sms_dates,buffer);
											#ifdef ERROR_FILE_DEBUG
											print1(sms_dates);
											sprintf(buffer,"\r\n");
											print1(buffer);
											#endif
											#endif
											retry=TRUE;
											break;
										}
									}
								}
							}												
						}
						
						if(strstrf(modem_rx_string,"250 "))
						{

							//#ifdef SMTP_DEBUG
							//sprintf(buffer,"Request data mode\r\n");print1(buffer);
							//#endif
							sprintf(buffer,"data\r\n");print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif
							if(!test_smtp_code())
							{
								#ifdef SMTP_DEBUG
								sprintf(buffer,"[email_log] Timed out waiting for response to data request\r\n");print1(buffer);
								#endif
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"110F.");
								strcat(sms_dates,buffer);
								#ifdef ERROR_FILE_DEBUG
								print1(sms_dates);
								sprintf(buffer,"\r\n");
								print1(buffer);
								#endif
								#endif
								retry=TRUE;
							}

							if(strstrf(modem_rx_string,"354 "))
							{
								//DATE and TIME
								// removed because wrong format for SMTP  - needs to be Thu, 21 May 2008 05:33:29 -0700
								// uses server date and time instead which is ok
								//rtc_get_date(&dy,&mth,&yr);rtc_get_time(&hr,&min,&sec);
								//sprintf(buffer,"Date: %d,%d,%d %02d:%02d:%02d\r\n",dy,mth,yr,hr,min,sec);print0(buffer);
								//#ifdef SMTP_DEBUG
								//print1(buffer);
								//#endif

								//FROM
								sprintf(buffer,"From: ");print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif
								strcpyre(buffer,config.from_address);print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif	
								sprintf(buffer,"\r\n");print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif

								//TO field:
								//a single email address
								if(sms_newMsg.usePhone==true)
								{
									sprintf(buffer,"To:%s",sms_newMsg.phoneNumber);print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									sprintf(buffer,"\r\n");print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
								}
								else
								{
									for(i=0;i<MAX_CONTACTS;i++)
									{
										if(sms_newMsg.contactList & ((unsigned int)0x01 << i))
										{
											if(contact_getType(i)==EMAIL)
											{
												//#ifdef SMTP_DEBUG
												//sprintf(buffer,"Email Add found at contact %d\r\n",i+1);print1(buffer);
												//#endif
											
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
												}									
												
												//TO
												sprintf(buffer,"To: %s",sms_newMsg.phoneNumber);print0(buffer);
												#ifdef SMTP_DEBUG
												print1(buffer);
												#endif
												sprintf(buffer,"\r\n");print0(buffer);
												#ifdef SMTP_DEBUG
												print1(buffer);
												#endif
											}
										}
									}
								}

								//SUBJECT
								sprintf(buffer,"Subject: ");
								print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif
								dy=0;
								for(dy=0;dy<170;dy++)
								{
									//sprintf(buffer,"%d - %c\r\n",sms_newMsg.txt[dy],sms_newMsg.txt[dy]);
									//print1(buffer);
									
									if(sms_newMsg.txt[dy]==13)
									{
										sprintf(buffer," ");
										print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
									}
									else if(sms_newMsg.txt[dy]==10)
									{
										sprintf(buffer," ");
										print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
									}
									else if(sms_newMsg.txt[dy]==0)
									{
										break;
									}
									else
									{
										sprintf(buffer,"%c",sms_newMsg.txt[dy]);
										print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
									}
								}									
								sprintf(buffer,"\r\n\r\n");print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif
							}
							else
							{
								#ifdef SMTP_DEBUG
								sprintf(buffer,"SMTP ERROR: Failed to enter data mode [%s]\r\n",modem_rx_string);print1(buffer);
								#endif
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"1110.");
								strcat(sms_dates,buffer);
								#ifdef ERROR_FILE_DEBUG
								print1(sms_dates);
								sprintf(buffer,"\r\n");
								print1(buffer);
								#endif
								#endif
								quit_smtp_server();
								retry=TRUE;
							}
						}
						else
						{
							#ifdef SMTP_DEBUG
							sprintf(buffer,"SMTP ERROR: Failed to set recipient address [%s]\r\n",modem_rx_string);print1(buffer);
							#endif
							#if ERROR_FILE_AVAILABLE	
							sprintf(buffer,"110E.");
							strcat(sms_dates,buffer);
							#ifdef ERROR_FILE_DEBUG
							print1(sms_dates);
							sprintf(buffer,"\r\n");
							print1(buffer);
							#endif
							#endif
							quit_smtp_server();
							retry=TRUE;
						}
					}
					else
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"SMTP ERROR: Failed to set from address [%s]\r\n",modem_rx_string);print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"110C.");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						quit_smtp_server();
						retry=TRUE;
					}
				}
				else
				{
					#ifdef SMTP_DEBUG
					sprintf(buffer,"SMTP ERROR: Failed to ehlo [%s]\r\n",modem_rx_string);print1(buffer);
					#endif
					#if ERROR_FILE_AVAILABLE	
					sprintf(buffer,"110A.");
					strcat(sms_dates,buffer);
					#ifdef ERROR_FILE_DEBUG
					print1(sms_dates);
					sprintf(buffer,"\r\n");
					print1(buffer);
					#endif
					#endif
					quit_smtp_server();
					retry=TRUE;
					//then stuck in data mode
				}

			}
			else //NO CARRIER received
			{
				#ifdef SMTP_DEBUG
				sprintf(buffer,"SMTP ERROR: Server failed to reply 220 [%s]\r\n",modem_rx_string);print1(buffer);
				#endif
				#if ERROR_FILE_AVAILABLE	
				sprintf(buffer,"1108.");
				strcat(sms_dates,buffer);
				#ifdef ERROR_FILE_DEBUG
				print1(sms_dates);
				sprintf(buffer,"\r\n");
				print1(buffer);
				#endif
				#endif
				quit_smtp_server();
				retry=TRUE;
			}
		}

		if(!retry)
		{
			// *****************************************************************************************
			// ** WRITE LOG DATA TO MESSAGE BODY *******************************************************
			
			sprintf(buffer,"%s",sms_newMsg.txt);
			print0(buffer);
			#ifdef SMTP_DEBUG
			print1(buffer);
			#endif
			
			
			// *****************************************************************************************
			// ** END SMTP DATA MODE *******************************************************************
			sprintf(buffer,"\r\n.\r\n");print0(buffer);
			#ifdef SMTP_DEBUG
			print1(buffer);
			#endif
			
			if(test_end_data_mode())
			{		
				// *****************************************************************************************
				// ** QUIT SMTP SERVER *********************************************************************
				quit_smtp_server();
			}
			else
			{
				#ifdef SMTP_DEBUG
				sprintf(buffer,"SMTP ERROR: Data mode could not be ended\r\n");print1(buffer);
				#endif
				#if ERROR_FILE_AVAILABLE	
				sprintf(buffer,"1111.");
				strcat(sms_dates,buffer);
				#ifdef ERROR_FILE_DEBUG
				print1(sms_dates);
				sprintf(buffer,"\r\n");
				print1(buffer);
				#endif
				#endif
				quit_smtp_server();
				retry=TRUE;
				//break;
			}
		}
		retries++;
	}
	while(retries<2 && retry==TRUE);
	if(retry)
	{
		#ifdef SMTP_DEBUG
		sprintf(buffer,"SMTP ERROR: Failed to send email\r\n");print1(buffer);
		#endif
		
		#if ERROR_FILE_AVAILABLE	
		sprintf(buffer,"1113#");
		strcat(sms_dates,buffer);
		log_line("error.log",sms_dates);
			#ifdef ERROR_FILE_DEBUG
			print1(sms_dates);
			sprintf(buffer,"\r\n");
			print1(buffer);
			#endif		
		#endif
		return 0;
	}
	return 1;
}

/* Email a section (or complete) of the error.log file **********************************
User can enter a date and time string for start and end of section of log required to be
emailed

DNS attempts: DNS0 then DNS1
SMTP attempts: 2
************************************************************************************** */
bool email_error() //email a log file to address specified by the update address
{
	// -----------------------------------------------------------------------------------------
	// -- GET LOG DATA VARIBLES ----------------------------------------------------------------
	FILE *fptr = NULL;
	unsigned char log_char;
	char read_char = 0;
	int count_char = 0;	//changed from char to int because 127 chracters not enough for log lines
	char date_time[19];
	//last log entry emailed - to be written to hidden file
	unsigned int yr;
	unsigned char mth, dy, hr, min, sec;
	//date and time to start log data
	unsigned int s_yr;
	unsigned char s_mth, s_dy, s_hr, s_min, s_sec;
	//date and time to end log data
	unsigned int e_yr;
	unsigned char e_mth, e_dy, e_hr, e_min, e_sec;
	bit dates_valid = 0;
	//bit logcsv_select;
	bit system0_alarm1;
	//bit hidden_flag=0;

	// -----------------------------------------------------------------------------------------
	// -- DNS VARIABLES ------------------------------------------------------------------------
	char *ip_address;

	// -----------------------------------------------------------------------------------------
	// -- SMTP VARIABLES ------------------------------------------------------------------------
	char retries=0;
	bit retry;
	
	#if ERROR_FILE_AVAILABLE
	sprintf(sms_dates,"");	//clear string for error codes
	#endif
	
	tickleRover();
	
	//convert date and time string to chars
	log_get_date_from_string(substr(dates,date_time,0,19),&s_yr,&s_mth,&s_dy,&s_hr,&s_min,&s_sec);
	//sprintf(buffer,"[email_log] Start date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",s_yr,s_mth,s_dy,s_hr,s_min,s_sec);print1(buffer);
	log_get_date_from_string(substr(dates,date_time,20,39),&e_yr,&e_mth,&e_dy,&e_hr,&e_min,&e_sec);
	//sprintf(buffer,"[email_log] End date and time: %04u/%02u/%02u %02u:%02u:%02u\r\n",e_yr,e_mth,e_dy,e_hr,e_min,e_sec);print1(buffer);

	// *****************************************************************************************
	// ** INITLIALIZE THE SD CARD **************************************************************
	if(!initialize_media())
    {
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to initialize the SD card");print1(buffer);
		#endif		
		
		#if ERROR_FILE_AVAILABLE	
		sprintf(buffer,"1201#");
		strcat(sms_dates,buffer);
			#ifdef ERROR_FILE_DEBUG
			print1(sms_dates);
			sprintf(buffer,"\r\n");
			print1(buffer);
			#endif
		log_line("error.log",sms_dates);
		#endif
		
        return 0;
    }
	
	#if PULSE_COUNTING_AVAILABLE
	if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
    {
		pulse_instant_pause = 1;
	}
	#endif	

	SREG &= ~(0x80);	//guard file operation against interrupt
	tickleRover();
	sedateRover();		
	fptr = fopenc("error.log",READ);
	wakeRover();
	SREG |= 0x80;

	if(fptr==NULL)
	{
		//unable to open file
		#ifdef SD_CARD_DEBUG
		sprintf(buffer,"FILE ERROR: Failed to open file\r\n");print1(buffer);
		#endif
		
		#if ERROR_FILE_AVAILABLE		
		sprintf(buffer,"1203#");
		strcat(sms_dates,buffer);		
			#ifdef ERROR_FILE_DEBUG
			print1(sms_dates);
			sprintf(buffer,"\r\n");
			print1(buffer);
			#endif		
		log_line("error.log",sms_dates);		
		#else
		
		#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
		if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
		{
			delay_ms(350);	//delay skips the next input scan (all inputs)
							//ensures that the pulse period is valid
			pulse_instant_pause = 0;
		}
		#endif		
		
		#endif
		
		return 0;
	}
	else	//file exists
	{
		while(!feof(fptr))
		{
			tickleRover();
			
			SREG &= ~(0x80);	//guard file operation against interrupt
			log_char = fgetc(fptr);
			SREG |= 0x80;
						
			sprintf(&read_char,"%c",log_char);

			if(count_char<19) //max number of date + time chars = 19
			{
				//if the string is full empty it for the next log date and time
				if(count_char==0)
				{
					sprintf(date_time,"");
				}
				//add the next character to the date + time string
				strncat(date_time,&read_char,1);
			}
			count_char++;
			if(count_char==19)
			{
				log_get_date_from_string(date_time,&yr,&mth,&dy,&hr,&min,&sec);

				//Compare the date from the current log line and the date saved in the hidden file
				//if the time and date of last log line emailed (from hidden file) is earlier than the
				//date and time of the curent log line set the email_line flag to add line to message body
				if ((yr>s_yr) ||
				(yr==s_yr && mth>s_mth) ||
				(yr==s_yr && mth==s_mth && dy>s_dy) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr>s_hr) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min>s_min) ||
				(yr==s_yr && mth==s_mth && dy==s_dy && hr==s_hr && min==s_min && sec>s_sec))	//> ensures hidden file date is not sent again
				{
					if((yr<e_yr) ||
					(yr==e_yr && mth<e_mth) ||
					(yr==e_yr && mth==e_mth && dy<e_dy) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr<e_hr) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min<e_min) ||
					(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min==e_min && sec<=e_sec))	//upto and inluding end date and time
					{
						//data is valid - set the pointer and exit loop
						//sprintf(buffer,"[email_log] Valid data found\r\n");print1(buffer);
						dates_valid = 1;
						break;
					}
				}
			}

			//detect end of logged line
			if(log_char==10) //line feed character \n (10) detected  && carr_rtrn==1
			{
				//reset the char counter for a new line in the log
				count_char = 0;
				//email_line = 0;
				//sprintf(buffer,"The last log entry emailed is: %04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
			}
		}

		if(dates_valid)
		{
			// *****************************************************************************************
			// ** RESOLVE THE IP ADDRESS ***************************************************************
			//#ifdef EMAIL_DEBUG
			//strcpyef(config.smtp_serv,"smtp2.vodafone.net.nz");
			//#endif
			
			//strcpyre(command_string,config.smtp_serv);
			//sprintf(buffer,"[email_log] Resolving IP address...\r\n");print1(buffer);
			
			do
			{
				retry=FALSE;
				// *****************************************************************************************
				// ** OPEN A SOCKET TO SMTP SERVER *********************************************************
				read_char = 'S';		//set connection type
				//if(open_socket(&read_char, ip_address))
				//{
				//	sprintf(buffer,"[email_log] SMTP socket open\r\n");print1(buffer);
				//}
				if(!(open_socket()))
				{
					#ifdef SMTP_DEBUG
					sprintf(buffer,"SMTP ERROR: Failed to open connection to SMTP server [%s]\r\n",modem_rx_string);print1(buffer);
					#endif
					#if ERROR_FILE_AVAILABLE	
					sprintf(buffer,"1206.");
					strcat(sms_dates,buffer);
					#ifdef ERROR_FILE_DEBUG
					print1(sms_dates);
					sprintf(buffer,"\r\n");
					print1(buffer);
					#endif
					#endif
					//return;
					retry=TRUE;
				}

				if(!retry)
				{
					// *****************************************************************************************
					// ** WRITE EMAIL HEADER *******************************************************************
					if(!test_smtp_code())
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"[email_log] Timed out waiting for 220 from SMTP server\r\n");print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"1207.");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						retry=TRUE;
					}
					
					if (strstrf(modem_rx_string,"220"))
					{

						//SJL - Debug -->
						//sprintf(buffer,"[email header]\r\n");print1(buffer);
						//sprintf(buffer,"Connected to VF SMTP server successfully, saying ehlo\r\n");print1(buffer);
						//SJL - <-- End of Debug

						sprintf(buffer,"ehlo edacelectronics.com\r\n");print0(buffer);
						#ifdef SMTP_DEBUG
						print1(buffer);
						#endif
						if(!test_smtp_code())
						{
							#ifdef SMTP_DEBUG
							sprintf(buffer,"[email_log] Timed out waiting for response to ehlo\r\n");print1(buffer);
							#endif
							#if ERROR_FILE_AVAILABLE	
							sprintf(buffer,"1209.");
							strcat(sms_dates,buffer);
							#ifdef ERROR_FILE_DEBUG
							print1(sms_dates);
							sprintf(buffer,"\r\n");
							print1(buffer);
							#endif
							#endif
							retry=TRUE;
						}

						if(strstrf(modem_rx_string,"250 "))
						{
							//SJL - Debug -->
							//sprintf(buffer,"Setting from address\r\n");print1(buffer);
							//SJL - <-- End of Debug
							sprintf(buffer,"mail from:");print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif
							strcpyre(buffer,config.from_address);print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif
							sprintf(buffer,"\r\n");print0(buffer);
							#ifdef SMTP_DEBUG
							print1(buffer);
							#endif

							if(!test_smtp_code())
							{
								#ifdef SMTP_DEBUG
								sprintf(buffer,"[email_log] Timed out waiting for response to mail from\r\n");print1(buffer);
								#endif
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"120B.");
								strcat(sms_dates,buffer);
								#ifdef ERROR_FILE_DEBUG
								print1(sms_dates);
								sprintf(buffer,"\r\n");
								print1(buffer);
								#endif
								#endif
								retry=TRUE;
							}

							if(strstrf(modem_rx_string,"250 "))
							{
								//SJL - Debug -->
								//sprintf(buffer,"Setting recipient address\r\n");print1(buffer);
								//SJL - <-- End of Debug
								sprintf(buffer,"rcpt to:");print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif
								strcpyre(buffer,config.update_address);print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif
								sprintf(buffer,"\r\n");print0(buffer);
								#ifdef SMTP_DEBUG
								print1(buffer);
								#endif

								if(!test_smtp_code())
								{
									#ifdef SMTP_DEBUG
									sprintf(buffer,"[email_log] Timed out waiting for response to rcpt to\r\n");print1(buffer);
									#endif
									#if ERROR_FILE_AVAILABLE	
									sprintf(buffer,"120D.");
									strcat(sms_dates,buffer);
									#ifdef ERROR_FILE_DEBUG
									print1(sms_dates);
									sprintf(buffer,"\r\n");
									print1(buffer);
									#endif
									#endif
									retry=TRUE;
								}

								if(strstrf(modem_rx_string,"250 "))
								{

									//SJL - Debug -->
									//sprintf(buffer,"Request data mode\r\n");print1(buffer);
									//SJL - <-- End of Debug
									sprintf(buffer,"data\r\n");print0(buffer);
									#ifdef SMTP_DEBUG
									print1(buffer);
									#endif
									if(!test_smtp_code())
									{
										#ifdef SMTP_DEBUG
										sprintf(buffer,"[email_log] Timed out waiting for response to data request\r\n");print1(buffer);
										#endif
										#if ERROR_FILE_AVAILABLE	
										sprintf(buffer,"120F.");
										strcat(sms_dates,buffer);
										#ifdef ERROR_FILE_DEBUG
										print1(sms_dates);
										sprintf(buffer,"\r\n");
										print1(buffer);
										#endif
										#endif
										retry=TRUE;
									}

									if(strstrf(modem_rx_string,"354 "))
									{
										//DATE and TIME
										// removed because wrong format for SMTP  - needs to be Thu, 21 May 2008 05:33:29 -0700
										// uses server date and time instead which is ok
										//rtc_get_date(&dy,&mth,&yr);rtc_get_time(&hr,&min,&sec);
										//sprintf(buffer,"Date: %d,%d,%d %02d:%02d:%02d\r\n",dy,mth,yr,hr,min,sec);print0(buffer);
										//#ifdef SMTP_DEBUG
										//print1(buffer);
										//#endif
										rtc_get_date(&s_dy,&s_mth,&log_char);rtc_get_time(&s_hr,&s_min,&s_sec);

										//FROM
										sprintf(buffer,"From: ");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										strcpyre(buffer,config.from_address);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										sprintf(buffer,"\r\n");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif

										//TO
										sprintf(buffer,"To: ");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										strcpyre(buffer,config.update_address);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										sprintf(buffer,"\r\n");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif

										//SUBJECT										
										sprintf(buffer,"Subject: Error log for ");										
										print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										strcpyre(buffer,config.site_name);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										sprintf(buffer," @ %d/%d/%d %02d:%02d:%02d (",s_dy,s_mth,log_char,s_hr,s_min,s_sec);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										strcpyre(buffer,serial);print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
										sprintf(buffer,")\r\n\r\n");print0(buffer);
										#ifdef SMTP_DEBUG
										print1(buffer);
										#endif
									}
									else
									{
										#ifdef SMTP_DEBUG
										sprintf(buffer,"SMTP ERROR: Failed to enter data mode [%s]\r\n",modem_rx_string);print1(buffer);
										#endif
										#if ERROR_FILE_AVAILABLE	
										sprintf(buffer,"1210.");
										strcat(sms_dates,buffer);
										#ifdef ERROR_FILE_DEBUG
										print1(sms_dates);
										sprintf(buffer,"\r\n");
										print1(buffer);
										#endif
										#endif
										quit_smtp_server();
										retry=TRUE;
									}
								}
								else
								{
									#ifdef SMTP_DEBUG
									sprintf(buffer,"SMTP ERROR: Failed to set recipient address [%s]\r\n",modem_rx_string);print1(buffer);
									#endif
									#if ERROR_FILE_AVAILABLE	
									sprintf(buffer,"120E.");
									strcat(sms_dates,buffer);
									#ifdef ERROR_FILE_DEBUG
									print1(sms_dates);
									sprintf(buffer,"\r\n");
									print1(buffer);
									#endif
									#endif
									quit_smtp_server();
									retry=TRUE;
								}
							}
							else
							{
								#ifdef SMTP_DEBUG
								sprintf(buffer,"SMTP ERROR: Failed to set from address [%s]\r\n",modem_rx_string);print1(buffer);
								#endif
								#if ERROR_FILE_AVAILABLE	
								sprintf(buffer,"120C.");
								strcat(sms_dates,buffer);
								#ifdef ERROR_FILE_DEBUG
								print1(sms_dates);
								sprintf(buffer,"\r\n");
								print1(buffer);
								#endif
								#endif
								quit_smtp_server();
								retry=TRUE;
							}
						}
						else
						{
							#ifdef SMTP_DEBUG
							sprintf(buffer,"SMTP ERROR: Failed to ehlo [%s]\r\n",modem_rx_string);print1(buffer);
							#endif
							#if ERROR_FILE_AVAILABLE	
							sprintf(buffer,"120A.");
							strcat(sms_dates,buffer);
							#ifdef ERROR_FILE_DEBUG
							print1(sms_dates);
							sprintf(buffer,"\r\n");
							print1(buffer);
							#endif
							#endif
							quit_smtp_server();
							retry=TRUE;
							//then stuck in data mode
						}

					}
					else //NO CARRIER received
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"SMTP ERROR: Server failed to reply 220 [%s]\r\n",modem_rx_string);print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"1208.");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						quit_smtp_server();
						retry=TRUE;
					}
				}

				if(!retry)
				{
					// *****************************************************************************************
					// ** WRITE LOG DATA TO MESSAGE BODY *******************************************************
					sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);print0(dates);//print1(dates);
					while(!feof(fptr))
					{
						tickleRover();
						//sprintf(buffer,"The file pointer is: %d - ",fptr->position);print1(buffer);
						
						SREG &= ~(0x80);	//guard file operation against interrupt
						log_char = fgetc(fptr);
						SREG |= 0x80;
						
						sprintf(&read_char,"%c",log_char);
						//#ifdef SMTP_DEBUG
						//print0(&read_char);
						//sprintf(buffer,"%c[%d] ",read_char,log_char);print1(buffer);
						//#endif

						if(count_char<19) //max number of date + time chars = 19
						{
							//if the string is full empty it for the next log date and time
							if(count_char==0)
							{
								sprintf(date_time,"");
							}
							//add the next character to the date + time string
							strncat(date_time,&read_char,1);
						}
						count_char++;
						if(count_char==19)
						{
							log_get_date_from_string(date_time,&yr,&mth,&dy,&hr,&min,&sec);	//prints 2000/01/01 00:00:00 after headings line
							//#ifdef SMTP_DEBUG
							//sprintf(buffer,"%04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
							//#endif

							if((yr<e_yr) ||
							(yr==e_yr && mth<e_mth) ||
							(yr==e_yr && mth==e_mth && dy<e_dy) ||
							(yr==e_yr && mth==e_mth && dy==e_dy && hr<e_hr) ||
							(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min<e_min) ||
							(yr==e_yr && mth==e_mth && dy==e_dy && hr==e_hr && min==e_min && sec<=e_sec))	//upto and inluding end date and time
							{
								//print the date and time to the message body
								sprintf(dates,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
								//sprintf(date_time,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
								//if(uart_select)
									//print1(dates);
								//else
									print0(dates);

								//copy the date and time back into the date_time string to be saved to the hidden file later
								//[because sec is always 0 after leaving the while loop - memory problem???]
								//sprintf(date_time,"%04u/%02u/%02u %02u:%02u:%02u",yr,mth,dy,hr,min,sec);
								//email_line = 1;
								dates_valid = 1;
							}
							else
								break;
						}

						//sprintf(buffer,"(%d,%d)",count_char,email_line);print1(buffer);
						if(count_char>19)	// && email_line==1)
						//if(count_char>19)
						{
							//#ifdef SMTP_DEBUG
							//print1(&read_char);
							//#endif
							print0(&read_char);
						}

						//detect end of logged line
						if(log_char==10) //line feed character \n (10) detected  && carr_rtrn==1
						{
							//reset the char counter for a new line in the log
							count_char = 0;
							//email_line = 0;
							//#ifdef SMTP_DEBUG
							//sprintf(buffer,"The last log entry emailed is: %04u/%02u/%02u %02u:%02u:%02u\r\n",yr,mth,dy,hr,min,sec);print1(buffer);
							//#endif
						}
					}
					#ifdef SMTP_DEBUG
					sprintf(buffer,"[email_log] Log data written to message body\r\n");print1(buffer);
					#endif
					
					SREG &= ~(0x80);	//commented out 100811
					count_char = fclose(fptr);
					SREG |= 0x80;
					
					tickleRover();
										
					// *****************************************************************************************
					// ** END SMTP DATA MODE *******************************************************************
					sprintf(buffer,"\r\n.\r\n");print0(buffer);
					
					if(test_end_data_mode())
					{		
						// *****************************************************************************************
						// ** QUIT SMTP SERVER *********************************************************************
						quit_smtp_server();
						
						#ifdef EMAIL_DEBUG
						sprintf(buffer,"[email_log] The last log entry is: %s\r\n",dates);print1(buffer);
						#endif
						
						#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
						if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
						{
							delay_ms(350);	//delay skips the next input scan (all inputs)
											//ensures that the pulse period is valid
							pulse_instant_pause = 0;
						}
						#endif	
						
						return 1;
					}
					else
					{
						#ifdef SMTP_DEBUG
						sprintf(buffer,"SMTP ERROR: Data mode could not be ended\r\n");print1(buffer);
						sprintf(buffer,"Email may or may not have been sent\r\n");print1(buffer);
						sprintf(buffer,"Latest date and time has not been saved\r\n");print1(buffer);
						#endif
						#if ERROR_FILE_AVAILABLE	
						sprintf(buffer,"1211#");
						strcat(sms_dates,buffer);
						#ifdef ERROR_FILE_DEBUG
						print1(sms_dates);
						sprintf(buffer,"\r\n");
						print1(buffer);
						#endif
						#endif
						
						retries=2;	//must exit while loop because file pointer is at end of file
						//retry=TRUE;
						//break;
					}
				}
				retries++;
			}
			while(retries<2 && retry==TRUE);
			if(retry)
			{
				SREG &= ~(0x80);	//added 100811
				count_char = fclose(fptr);
				SREG |= 0x80;
				
				#ifdef EMAIL_DEBUG
				sprintf(buffer,"SMTP ERROR: Failed to send email\r\n");print1(buffer);
				#endif
				
				#if ERROR_FILE_AVAILABLE	
				sprintf(buffer,"1213#");
				strcat(sms_dates,buffer);				
					#ifdef ERROR_FILE_DEBUG
					print1(sms_dates);
					sprintf(buffer,"\r\n");
					print1(buffer);
					#endif				
				log_line("error.log",sms_dates);				
				#else				
					#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						delay_ms(350);	//delay skips the next input scan (all inputs)
										//ensures that the pulse period is valid
						pulse_instant_pause = 0;
					}
					#endif					
				#endif
				
				//email_retry_timer = email_retry_period;
				//sprintf(buffer,"Setting timer to retry in %d (minutes)\r\n",email_retry_timer);
				//print1(buffer);
				
				return 0;				
			}
			else		
			{
				//failed to end data mode correctly
				#if ERROR_FILE_AVAILABLE
				log_line("error.log",sms_dates);
				#else				
					#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
					if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
					{
						delay_ms(350);	//delay skips the next input scan (all inputs)
										//ensures that the pulse period is valid
						pulse_instant_pause = 0;
					}
					#endif				
				#endif
				
				return 0;
			}
		}
		else	//no valid log data
		{
			SREG &= ~(0x80);	//guard file operation against interrupt
			count_char = fclose(fptr);
			SREG |= 0x80;
			
			#ifdef SD_CARD_DEBUG
			sprintf(buffer,"[email_log] result of fclose = %d\r\n",count_char);print1(buffer);
			#endif
			
			#if ERROR_FILE_AVAILABLE	
			sprintf(buffer,"1204#");
			strcat(sms_dates,buffer);
				#ifdef ERROR_FILE_DEBUG
				print1(sms_dates);
				sprintf(buffer,"\r\n");
				print1(buffer);
				#endif
			log_line("error.log",sms_dates);			
			#else			
				#if PULSE_COUNTING_AVAILABLE //see FW revision doc for full explanation of this code section
				if(config.input[6].type == PULSE  || config.input[7].type == PULSE)
				{
					delay_ms(350);	//delay skips the next input scan (all inputs)
									//ensures that the pulse period is valid
					pulse_instant_pause = 0;
				}
				#endif			
			#endif			
			
			//email this
			strcpyre(sms_newMsg.phoneNumber,config.update_address);
			
			sprintf(buffer,"No log data for specified time range (%04u/%02u/%02u %02u:%02u:%02u - %04u/%02u/%02u %02u:%02u:%02u)\n"
			,s_yr,s_mth,s_dy,s_hr,s_min,s_sec,e_yr,e_mth,e_dy,e_hr,e_min,e_sec);
			
			#ifdef EMAIL_DEBUG
			print1(buffer);
			#endif
			
			sprintf(sms_newMsg.txt,"%s",buffer);			
			//modem_send_sms(sms_newMsg.txt,sms_newMsg.phoneNumber);

			sms_newMsg.usePhone=true;
						
			msg_sendData();	
			
			return 0;
		}
	}
}
#endif

#endif
