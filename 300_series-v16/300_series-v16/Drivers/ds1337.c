/******************************************************************
 * DS1337 Driver
 *
 * This file contains the protocol to control the DS1337 real time
 * Clock.
 * NOTE: the century bit of the date is not handled.
 *
 * SJL - RTC corruption issue: error checking added to all functions
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/
#include <i2c.h>
#include <bcd.h>
#include <stdio.h>
#include <delay.h>

#include "global.h"
#include "drivers/ds1337.h"
#include "drivers/input.h"
#include "drivers/event.h"
#include "drivers/config.h"      //SJL - CAVR2 - added
#include "drivers/debug.h"       //SJL - CAVR2 - added
#include "drivers/queue.h"       //SJL - CAVR2 - added
#include "drivers/uart.h"       //SJL - CAVR2 - added
#include "drivers/mmc.h"       //SJL - CAVR2 - added DEBUG

//char month_length[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

//char DEBUG_buffer[100];
flash char date_code[12] = {6,2,2,5,0,3,5,1,4,6,2,4};

//RTC interrupt
//#if HW_REV == V5 || HW_REV == V6
interrupt [EXT_INT5] void ext_int5_isr(void)
//#elif HW_REV == V2
//interrupt [EXT_INT7] void ext_int7_isr(void)
//#else
//#endif
{
    Event e;
    char day;
    #ifdef INT_TIMING
    start_int();
    #endif

    // SJL - Debug weekly alarm
    //print("interrupt firing\r\n");
    //rtc_get_day(&day);
    //sprintf(buffer,"config.autoreport.day=%d\r\n",config.autoreport.day);print(buffer);
	//sprintf(buffer,"day=%d\r\n",day);print(buffer);

    e.type = sms_AUTO_REPORT;
    e.param = 0;
    if(config.autoreport.type==ALARM_DAY||config.autoreport.type==ALARM_WEEK)
    {
    	//print("daily or weekly\r\n");	// SJL - Debug
        queue_push(&q_modem, &e);
    }
    else
    {
        char d,m,y;
        //char day;
        rtc_get_date(&d,&m,&y);

        //print("monthly\r\n");	// SJL - Debug

        if(config.autoreport.day==d||(config.autoreport.day > month_length[m-1] && d == month_length[m-1]))
        //print("valid day");	// SJL - Debug
        queue_push(&q_modem, &e);
    }
    rtc_clear_1();
    #ifdef INT_TIMING
    stop_int();
    #endif

}

/******************************************************************
 * void rtc_init(void)
 *
 * Initilise the DS1339 to enable the oscillator and set both of the
 * outputs to be alarms.
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/
void rtc_init(void)
{

   /*
   #asm("cli");
   i2c_stop();
   i2c_start();
   //Write I2C address of DS1339
   i2c_write(0xd0);
   //Write to the control register
   i2c_write(0x0E);
   //Set the control register
   //Enable Oscillator
   //Both alarms active
   if(config.autoreport.type == NO_ALARM)
       i2c_write(0x04);
   else
       i2c_write(0x05);
   i2c_stop();
   */

   //SJL - RTC ISSUE
   //added conditional statement in place of just i2c_start()
   //to try to avoid corruption of data on i2c bus
   unsigned char retries=0;
   #asm("cli");

START:
    //increment the number of communication attempts
    if(retries++>20)
    {
   		//sprintf(DEBUG_buffer,"Too many retries (20), returning\r\n");print(DEBUG_buffer);
        return;
    }

    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x0E))	//control register address = 0E
            {
            	if(config.autoreport.type < NO_ALARM)
                {
       				if(!i2c_write(0x05))	// 0000 0101 - Enable interrupt on Alarm 1
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to write to control register [0x05] (initialization)\r\n");print(DEBUG_buffer);
    					goto START;
                    }
                else
                    if(!i2c_write(0x04))	// 0000 0100 -
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to write to control register [0x04] (initialization)\r\n");print(DEBUG_buffer);
    					goto START;
                    }
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge control register address + write bit [0] (initialization)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (initialization)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (initialization)\r\n");print(DEBUG_buffer);
    	goto START;
    }

	i2c_stop();
    delay_ms(10);

    //sprintf(DEBUG_buffer,"Successfully written to control register after %d attempt(s) (initialization)\r\n",retries);print(DEBUG_buffer);

    //SJL - End of added conditional statements

   	#asm("sei");

   	rtc_clear_1();
   	//rtc_clear_2();
 	//#if HW_REV == V5 || HW_REV == V6
   	DDRE &=~ 0x20;
    EIMSK &=~ 0x20;	//SJL - added this line and set (EIMSK |= 0x20) removed below for compatabiliity with ATMega128A
   	EICRB |= 0x08;
   	//EIMSK |= 0x20; //enable EXT_INT_5 (RTC_INT)
   	//#else
   	//DDRE &=~ 0x80;
   	//EICRB |= 0x80;
   	//EIMSK |= 0x80; //enable EXT_INT_7 (RTC_INT)
   	//#endif

}

/******************************************************************
 * void rtc_get_time(unsigned char *hour,unsigned char *min,
 							unsigned char *sec)
 *
 * Get the current time in the RTC.
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/
void rtc_get_time(unsigned char *hour,unsigned char *min,unsigned char *sec)
{
    char data0=0, data1=0, data2=0;
    unsigned char retries=0;

START:
    //increment the number of communication attempts
    if(retries++>20)
    {
   		//sprintf(DEBUG_buffer,"Too many retries (20), returning\r\n");print(DEBUG_buffer);
        return;
    }

    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x00))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data0=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"Data = %d,",data0);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get time-seconds)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get time-seconds)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get time-seconds)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get time-seconds)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get time-seconds)\r\n");print(DEBUG_buffer);
        goto START;
    }

   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x01))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data1=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"%d,",data1);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get time-minutes)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get time-minutes)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get time-minutes)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get time-minutes)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get time-minutes)\r\n");print(DEBUG_buffer);
        goto START;
    }

   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x02))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data2=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"%d	",data2);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get time-hours)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get time-hours)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get time-hours)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get time-hours)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get time-hours)\r\n");print(DEBUG_buffer);
        goto START;
    }

    *sec=bcd2bin(data0);
    *min=bcd2bin(data1);
    *hour=bcd2bin(data2);

   	//sprintf(DEBUG_buffer,"Time: %d:%d:%d\r\n",*hour, *min, *sec);print(DEBUG_buffer);

}

/******************************************************************
 * void rtc_set_time(unsigned char hour,unsigned char min,
 * 						unsigned char sec)
 *
 * Set the time in the RTC
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/
void rtc_set_time(unsigned char hour,unsigned char min,unsigned char sec)
{
	/*
   //  #asm("cli");
   i2c_start();
   //Write to the DS1339
   i2c_write(0xd0);
   //Start writing at 0x00 (time)
   i2c_write(0x00);
   //Write the time
   i2c_write(bin2bcd(sec));
   i2c_write(bin2bcd(min));
   i2c_write(bin2bcd(hour));
   i2c_stop();
   #asm("sei");
   */

   	//SJL - RTC ISSUE
   	//added conditional statement in place of just i2c_start()
   	//to try to avoid corruption of data on i2c bus
   	unsigned char retries=0;
   	//#asm("cli");

START:
    //increment the number of communication attempts
    if(retries++>20)
    {
   		//sprintf(DEBUG_buffer,"Too many retries (20), returning\r\n");print(DEBUG_buffer);
        return;
    }

    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x00))	// seconds register address
            {
            	if(!i2c_write(bin2bcd(sec)))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set time-seconds)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set time-seconds)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set time-seconds)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set time-seconds)\r\n");print(DEBUG_buffer);
    	goto START;
    }

	i2c_stop();
    delay_ms(10);

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x01))	// minutes register address
            {
            	if(!i2c_write(bin2bcd(min)))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set time-minutes)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set time-minutes)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set time-minutes)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set time-minutes)\r\n");print(DEBUG_buffer);
    	goto START;
    }

    i2c_stop();
    delay_ms(10);

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x02))	// minutes register address
            {
            	if(!i2c_write(bin2bcd(hour)))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set time-hours)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set time-hours)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set time-hours)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set time-hours)\r\n");print(DEBUG_buffer);
    	goto START;
    }

    i2c_stop();
    delay_ms(10);

    //sprintf(DEBUG_buffer,"Successfully set time after %d attempt(s)\r\n",retries);

    #asm("sei");
}


/******************************************************************
 * void rtc_get_date(unsigned char *date,unsigned char *month,
 * 						unsigned char *year)
 *
 * Read the date from the RTC
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/
void rtc_get_date(unsigned char *date,unsigned char *month,unsigned char *year)
{
   /*
   //char day;	//SJL - CAVR2 - local variable day set but not used - linker warning
   //#asm("cli");
   i2c_start();			//start i2c bus
   i2c_write(0xd0);     //write rtc address and lsb=0 (write)
   //Start the read at the date address
   i2c_write(0x03);		//send data to rtc, data = 0000 0011
   i2c_start();         //start i2c bus
   i2c_write(0xd1);     //write rtc address and lsb=1 (read)

   //day = i2c_read(1);	//SJL - CAVR2 - local variable day set but not used - linker warning

	i2c_read(1); //extracts the day - not utilized atm - see rtc_set_date
   *date=bcd2bin(i2c_read(1));    		//day
   *month=bcd2bin(i2c_read(1)&0x7F);    //date
   *year=bcd2bin(i2c_read(0));          //month

   //#ifdef DEBUG
   sprintf(DEBUG_buffer,"%d/%d/%d\r\n",*date, *month, *year);print(DEBUG_buffer);
   //#endif

   i2c_stop();
   #asm("sei");

   */

   	char data0=0, data1=0, data2=0;
   	unsigned char retries=0;

START:
    //increment the number of communication attempts
    if(retries++>20)
    {
   		//sprintf(DEBUG_buffer,"Too many retries (20), returning\r\n");print(DEBUG_buffer);
        return;
    }

    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x04))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data0=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"Data = %d,",data0);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get date-date)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get date-date)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get date-date)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get date-date)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get date-date)\r\n");print(DEBUG_buffer);
        goto START;
    }

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x05))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data1=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"%d,",data1);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get date-month)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get date-month)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get date-month)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get date-month)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get date-month)\r\n");print(DEBUG_buffer);
        goto START;
    }

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x06))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data2=i2c_read(0)&0x7F;
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"%d	",data2);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get date-year)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get date-year)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get date-year)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get date-year)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get date-year)\r\n");print(DEBUG_buffer);
        goto START;
    }

    *date=bcd2bin(data0);
    *month=bcd2bin(data1);
    *year=bcd2bin(data2);

   	//sprintf(DEBUG_buffer,"Date: %d/%d/%d\r\n",*date, *month, *year);print(DEBUG_buffer);
}

/*
void rtc_get_day(unsigned char *day)
{
   	char data0=0;
   	unsigned char retries=0;

START:
    //increment the number of communication attempts
    if(retries++>20)
    {
   		//sprintf(DEBUG_buffer,"Too many retries (20), returning\r\n");print(DEBUG_buffer);
        return;
    }

    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x03))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data0=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"Data = %d,",data0);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get day)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get day)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get day)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get day)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get day)\r\n");print(DEBUG_buffer);
        goto START;
    }

    *day=bcd2bin(data0);
}
*/

/******************************************************************
 * void rtc_set_date(unsigned char date,unsigned char month,
 * 						unsigned char year)
 *
 * Set the date in the RTC
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/

void rtc_set_date(unsigned char date,unsigned char month,unsigned char year)
{
	/*
   char day;
 //    #asm("cli");
   i2c_start();
   i2c_write(0xd0);
   //Start the write at the day address
   i2c_write(0x03);

   //calculate day of the week.
   //with thanks to http://www.terra.es/personal2/grimmer
   //only, it's broken for jan / feb of a leap year.
  // sprintf(buffer,"passed %02d/%02d/%02d\r\n",date,month,year);
  // print(buffer);
   day = (char)(year * 1.25);
  // sprintf(buffer,"step 1: %d\r\n",day);
  // print(buffer);
   if((!(year%4))&&month<3)
   {
    day++;
  //  sprintf(buffer,"step 1a: %d\r\n",day);
  //  print(buffer);
   }
   day += date_code[month-1];
   //sprintf(buffer,"step 2: %d\r\n",day);
  // print(buffer);
   day += date;
  // sprintf(buffer,"step 3: %d\r\n",day);
  // print(buffer);
   day = day%7;

  // sprintf(buffer,"Day Calculated as %d (0=sunday)\r\n",day);
  // print(buffer);

   i2c_write(day+1);
   i2c_write(bin2bcd(date));
   i2c_write(bin2bcd(month));
   i2c_write(bin2bcd(year));
   i2c_stop();
   	*/

   	//SJL - RTC ISSUE
   	//added conditional statement in place of just i2c_start()
   	//to try to avoid corruption of data on i2c bus
   	unsigned char retries=0;
   	//#asm("cli");

START:
    //increment the number of communication attempts
    if(retries++>20)
    {
   		//sprintf(DEBUG_buffer,"Too many retries (20), returning\r\n");print(DEBUG_buffer);
        return;
    }

    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x04))	// date register address
            {
            	if(!i2c_write(bin2bcd(date)))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set date-date)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set date-date)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set date-date)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set date-date)\r\n");print(DEBUG_buffer);
    	goto START;
    }

	i2c_stop();
    delay_ms(10);

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x05))	// month register address
            {
            	if(!i2c_write(bin2bcd(month)))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set date-month)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set date-month)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set date-month)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set date-month)\r\n");print(DEBUG_buffer);
    	goto START;
    }

    i2c_stop();
    delay_ms(10);

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x06))	// year register address
            {
            	if(!i2c_write(bin2bcd(year)))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set date-year)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set date-year)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set date-year)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set date-year)\r\n");print(DEBUG_buffer);
    	goto START;
    }

    i2c_stop();
    delay_ms(10);

    //sprintf(DEBUG_buffer,"Successfully set date after %d attempt(s)\r\n",retries);

   #asm("sei");
}

/* SJL - UNUSED
unsigned char rtc_check_alarm() {
   	unsigned char readChar=0;
	// #asm("cli");
   	//i2c_start();
   	//i2c_write(0xd0);
   	//Start the read at the date address
   	//i2c_write(0x0F);
   	//i2c_start();
   	//i2c_write(0xd1);
   	//readChar=i2c_read(0);
   	//i2c_stop();

    //SJL - RTC ISSUE
   	//added conditional statement in place of just i2c_start()
   	//to try to avoid corruption of data on i2c bus
   	unsigned char retries=0;

   START:
    //increment the number of communication attempts
    if(retries++>20)
    {
   		sprintf(DEBUG_buffer,"Too many retries (20), returning\r\n");print(DEBUG_buffer);
        return 0;
    }

    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x0F))	//status register address = 0E
            {
            	if(i2c_start())
       			{
                	if(i2c_write(0xd1))
                    {
                    	readChar=i2c_read(0);
   						sprintf(DEBUG_buffer,"successfully read status register: %d\r\n",readChar);print(DEBUG_buffer);
                    }
                    else
                    {
                    	sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (check alarm) (check alarm)\r\n");print(DEBUG_buffer);
    					goto START;
                    }
                }
                else
       			{
                	sprintf(DEBUG_buffer,"i2c failed to restart (check alarm)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	sprintf(DEBUG_buffer,"i2c failed to acknowledge status register address (check alarm)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (check alarm)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		sprintf(DEBUG_buffer,"i2c failed to start (check alarm)\r\n");print(DEBUG_buffer);
    	goto START;
    }

	i2c_stop();
    delay_ms(10);

   	#asm("sei");
   	if (readChar & 0x01) {
   		return 1;
   	}
   	else if (readChar & 0x02) {
   		return 2;
   	}

   	return 0;
}
*/

/******************************************************************
 * void rtc_set_alarm_1(unsigned char hour,unsigned char min,
 *								unsigned char sec)
 *
 * Set alarm 1 in the RTC.  This is set to alarm when the seconds
 * and minutes match.  Used to signal when to drive the solenoid.
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/
void rtc_set_alarm_1(char day,char hour,char min,char sec, unsigned char type) {

   	unsigned char temp[4];
    unsigned char retries=0;
    unsigned char data0=0, data1=0, data2=0, data3=0, data4=0;	//DEBUG
    //unsigned char *reg07,*reg08,*reg09,*reg0A; 			//DEBUG

  	//  #asm("cli");
   	//i2c_start();
   	//i2c_write(0xd0);
   	//Start the write at the date address
   	//i2c_write(0x07);
   	//Alarm when mins and sec match.

    temp[0] = bin2bcd(sec);
   	temp[1] = bin2bcd(min);
   	temp[2] = bin2bcd(hour);
   	temp[3] = bin2bcd(day);

   	//sprintf(buffer,"alarming on the %dth at %02d:%02d:%02d - %d",day,hour,min,sec,type);log_line("system.log",buffer);
   	//print(buffer);

    //sprintf(buffer,"type = %d",type);log_line("system.log",buffer);

   	switch(type)
   	{

    	//case ALARM_SECOND:
        //    temp[0] |= 0x80;
        //case ALARM_MINUTE:
        //    temp[1] |= 0x80;
        //case ALARM_HOUR:
        //    temp[2] |= 0x80;
        case ALARM_DAY:
        	temp[0] &= ~0x80;
        	temp[1] &= ~0x80;
        	temp[2] &= ~0x80;
        	temp[3] |= 0x80;    // 0x80 = 1000 0000 - set bit 7 register 0A A1M4
            //sprintf(buffer,"Alarm - day");log_line("system.log",buffer);
            break;
        case ALARM_MONTH: 		//doing extra checking in the interrupt routine to deal with
            temp[0] &= ~0x80;	//the 29th, 30th and 31st of a month
            temp[1] &= ~0x80;
        	temp[2] &= ~0x80;
            temp[3] &= ~0xC0;
            //sprintf(buffer,"Alarm - month");log_line("system.log",buffer);
            break;
        case ALARM_WEEK:
        	//temp[0] &= ~0x80;
            //temp[1] &= ~0x80;
        	//temp[2] &= ~0x80;
            //temp[3] |= 0x40;	// 0x40 = 0100 0000 - set bit 6 register 0A DY/~DT
            //set to daily below because day function doesn't work
            temp[0] &= ~0x80;
        	temp[1] &= ~0x80;
        	temp[2] &= ~0x80;
        	temp[3] |= 0x80;
            //sprintf(buffer,"Alarm - week");log_line("system.log",buffer);
            break;
        default:
            break;
   	}

   	//SJL - RTC ISSUE
   	//added conditional statement in place of just i2c_start()
   	//to try to avoid corruption of data on i2c bus
   	//#asm("cli");

START:
    //increment the number of communication attempts
    if(retries++>20)
    {
   		//sprintf(DEBUG_buffer,"Too many retries (20), returning\r\n");print(DEBUG_buffer);
        return;
    }

    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x07))	// alarm 1 seconds register address
            {
            	if(!i2c_write(temp[0]))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set alarm 1-seconds)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set alarm 1-seconds)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set alarm 1-seconds)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set alarm 1-seconds)\r\n");print(DEBUG_buffer);
    	goto START;
    }
	i2c_stop();
    delay_ms(10);

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x08))	// alarm 1 minutes register address
            {
            	if(!i2c_write(temp[1]))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set alarm 1-minutes)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set alarm 1-minutes)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set alarm 1-minutes)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set alarm 1-minutes)\r\n");print(DEBUG_buffer);
    	goto START;
    }
	i2c_stop();
    delay_ms(10);

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x09))	// alarm 1 hours register address
            {
            	if(!i2c_write(temp[2]))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set alarm 1-hours)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set alarm 1-hours)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set alarm 1-hours)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set alarm 1-hours)\r\n");print(DEBUG_buffer);
    	goto START;
    }
	i2c_stop();
    delay_ms(10);

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x0A))	// alarm 1 day/date register address
            {
            	if(!i2c_write(temp[3]))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (set alarm 1-day/date)\r\n");print(DEBUG_buffer);
    				goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address + write bit [0] (set alarm 1-day/date)\r\n");print(DEBUG_buffer);
    			goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (set alarm 1-day/date)\r\n");print(DEBUG_buffer);
    		goto START;
        }
    }
    else
    {
		//sprintf(DEBUG_buffer,"i2c failed to start (set alarm 1-day/date)\r\n");print(DEBUG_buffer);
    	goto START;
    }
	i2c_stop();
    delay_ms(10);

    /*
   	i2c_write(temp[0]);
   	i2c_write(temp[1]);
   	i2c_write(temp[2]);
   	i2c_write(temp[3]);

   	i2c_stop();
    */


    /*
    //SJL - DEBUG - read alarm1
    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x07))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data0=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"Data = %d,",data0);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get time-seconds)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get time-seconds)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get time-seconds)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get time-seconds)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get time-seconds)\r\n");print(DEBUG_buffer);
        goto START;
    }

   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x08))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data1=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"%d,",data1);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get time-minutes)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get time-minutes)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get time-minutes)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get time-minutes)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get time-minutes)\r\n");print(DEBUG_buffer);
        goto START;
    }

   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x09))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data2=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"%d	",data2);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get time-hours)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get time-hours)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get time-hours)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get time-hours)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get time-hours)\r\n");print(DEBUG_buffer);
        goto START;
    }

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x0A))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data3=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"%d	",data2);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get time-hours)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get time-hours)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get time-hours)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get time-hours)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get time-hours)\r\n");print(DEBUG_buffer);
        goto START;
    }

    if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x0E))
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	data4=i2c_read(0);
    					i2c_stop();
    					//sprintf(DEBUG_buffer,"%d	",data2);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (get time-hours)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (get time-hours)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (get time-hours)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (get time-hours)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (get time-hours)\r\n");print(DEBUG_buffer);
        goto START;
    }

    // *reg07=bcd2bin(data0);
    // *reg08=bcd2bin(data1);
    // *reg09=bcd2bin(data2);
    // *reg0A=bcd2bin(data3);

	sprintf(buffer,"07H: %d",data0);log_line("system.log",buffer);
    sprintf(buffer,"08H: %d",data1);log_line("system.log",buffer);
    sprintf(buffer,"09H: %d",data2);log_line("system.log",buffer);
    sprintf(buffer,"0AH: %d",data3);log_line("system.log",buffer);
    sprintf(buffer,"con: %d",data4);log_line("system.log",buffer);
    //end of debug
    */

    EIMSK |= 0x20;	//SJL - added - moved from rtc_init to stop crash in system initialization with 128A
    #asm("sei");
}

/******************************************************************
 * void rtc_clear_1 (void)
 *
 * Clear the interrupt flag in the DS1339 so a new alarm can be set
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************/
void rtc_clear_1 (void)  {
    unsigned char statusReg = 0;
   	//#asm("cli");
	//i2c_start();
    //i2c_write(0xd0);
    //Start the read at the date address
    //i2c_write(0x0F);
    //i2c_start();
    //i2c_write(0xd1);
    //statusReg=i2c_read(0);
    //i2c_stop();

    unsigned char retries=0;

START:
    //increment the number of communication attempts
    if(retries++>20)
    {
   		//sprintf(DEBUG_buffer,"Too many retries (20), returning\r\n");print(DEBUG_buffer);
        return;
    }

    /*
    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x0F))	// status register address
            {
            	if(i2c_start())
                {
                	if(i2c_write(0xd1))
                    {
                    	statusReg=i2c_read(0);
    					//sprintf(DEBUG_buffer,"Data = %d,",data0);print(DEBUG_buffer);
                    }
                    else
                    {
                    	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + read bit [1] (clear alarm 1-read)\r\n");print(DEBUG_buffer);
                        goto START;
                    }
                }
                else
                {
                	//sprintf(DEBUG_buffer,"i2c failed to restart (clear alarm 1-read)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (clear alarm 1-read)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (clear alarm 1-read)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (clear alarm 1-read)\r\n");print(DEBUG_buffer);
        goto START;
    }
    */

    i2c_stop();
   	if(i2c_start())
    {
    	if(i2c_write(0xd0))
        {
        	if(i2c_write(0x0F))	// status register address
            {
            	//if(!i2c_write(statusReg & 0x82))
                if(!i2c_write(0x00))
                {
                	//sprintf(DEBUG_buffer,"i2c failed to acknowledge write to data address (clear alarm 1-write)\r\n");print(DEBUG_buffer);
                    goto START;
                }
            }
            else
            {
            	//sprintf(DEBUG_buffer,"i2c failed to acknowledge data address (clear alarm 1-write)\r\n");print(DEBUG_buffer);
                goto START;
            }
        }
        else
        {
        	//sprintf(DEBUG_buffer,"i2c failed to acknowledge RTC address + write bit [0] (clear alarm 1-write)\r\n");print(DEBUG_buffer);
            goto START;
        }
    }
    else
    {
    	//sprintf(DEBUG_buffer,"i2c failed to start (clear alarm 1-write)\r\n");print(DEBUG_buffer);
        goto START;
    }
    i2c_stop();
    delay_ms(10);

    //i2c_start();
    //Write I2C address of DS1339
    //i2c_write(0xd0);
    //Write to the status register
    //i2c_write(0x0F);
    //Clear the interrupt0 flag
    //i2c_write(statusReg & 0x82);
    //i2c_stop();
    #asm("sei");
}

/* SJL - UNSUSED
******************************************************************
 * void rtc_set_alarm_2(unsigned char hour,unsigned char min)
 *
 * Set the time for the second alarm to interrupt at.  This alarms
 * when there is a match in the hours and minutes.
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************
void rtc_set_alarm_2(unsigned char hour,unsigned char min, unsigned char type) {
 //#asm("cli");
	i2c_start();
   i2c_write(0xd0);
   //Start the write at the date address
   i2c_write(0x0B);

   //Alarm when hours, min match.
   i2c_write(bin2bcd(min));
   i2c_write(bin2bcd(hour));
   i2c_write(0x80);
   i2c_stop();
       #asm("sei");

}

******************************************************************
 * void rtc_clear_2 (void)
 *
 * Clear the interrupt flag in the DS1339 so a new alarm can be set
 *
 * Author: Chris Cook
 * May 2004
 * (c) EDAC Electronics LTD 2004
 ******************************************************************
void rtc_clear_2 (void)  {
  	unsigned char statusReg = 0;
 // 	 #asm("cli");
	i2c_start();
   i2c_write(0xd0);
   //Start the read at the date address
   i2c_write(0x0F);
   i2c_start();
   i2c_write(0xd1);
   statusReg=i2c_read(0);
   i2c_stop();

   i2c_start();
   //Write I2C address of DS1339
   i2c_write(0xd0);
   //Write to the status register
   i2c_write(0x0F);
   //Clear the interrupt0 flag
   i2c_write(statusReg & 0x81);
   i2c_stop();
    #asm("sei");
}
*/