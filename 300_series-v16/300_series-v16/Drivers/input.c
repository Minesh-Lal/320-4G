/*****************************************************************************
 INPUT CONTROL
 input.c

 Checking of the inputs is interrupt driven.  Every 40ms there is an interrupt
 that starts the process of checking an input.  This starts an ADC conversion
 for the channel that we are up to (for example input 3).  At the same time we
 check to see if that input is in the middle of a debounce event, if it is
 decrement the counter.  Once this has been started the foreground task is
 returned control on the CPU.  When the ADC conversion complete interrupt occurs
 the foreground task gives up control of the CPU again, and the ADC value for the
 input being checked is read in, and checked to see what alarm zone it is
 currently in.  Once the input has readings in a new zone for 4 samples a event
 can be passed to the foreground task.  Only 2 events can be passed, rising
 event and falling event. When a sensor moves from either zone 1 or zone 0 into
 zone 2 a rising event occurs, and when a input moves from zone 1 or zone 2 into
 zone 0 a falling event occurs.  This means that analog and digital inputs can
 be handled the same, and also the abnormal sensors values (ie readings greater
 than 5V or not between 4-20mA) can also be checked with this setup.

                        Zone 2
     Rising Event  * * * * * * * *
            ------@---------------*------------  Threshold High
                 *                  *
                *        Zone 1       *
               *                        * Falling Event
            --*--------------------------@-----  Threshold Low
             *                            *
                         Zone 0

 Note: Threshold High and Low are calculated from the configuration settings
 and the set and reset points.  This is done in input_setup().

 For each ADC conversion complete, each alarm channel is checked (if
 enabled) and the sensor is also check (if analog).

 Once the input has been checked, resource is returned to the foreground task
 until the next 40ms interrupt when the next input is checked.

 The events are all pushed onto the event queue, where they are handled in the
 foreground task.


 Project : 	     SMS300
 Author: 		 Chris Cook
 Date :  		 April 2005
 (c) EDAC Electronics LTD 2005
 ******************************************************************************/

#include <stdio.h>              //SJL - CAVR2 - added
#include <delay.h>              //SJL - CAVR2 - added

#include "global.h"
#include "drivers\input.h"
#include "drivers\queue.h"
#include "drivers\event.h"
#include "drivers\config.h"
#include "drivers\str.h"
#include "drivers\modem.h"
#include "drivers\debug.h"      //SJL - CAVR2 - added
#include "drivers\uart.h"       //SJL - CAVR2 - added
#include "drivers\mmc.h"       //SJL - CAVR2 - added

//Globals
//extern eeprom config_t config;

//SJL - CAVR2 - input must be defined with values for linking in CAVR2
volatile input_alarm_t input[MAX_INPUTS];	//structure of the variables for the alarm
volatile unsigned char channel;
volatile unsigned char alarmCount;

//extern volatile unsigned char channel;				 		//The currentZone channel you are reading	//SJL - volatile added from global.c def
//extern volatile unsigned char alarmCount;					//SJL - volatile added from global.c def
//extern unsigned char power_led_state;
eeprom unsigned char saved_state[MAX_INPUTS][2];
//extern queue_t q_event;

//eeprom unsigned char* adc_voltage_offset = BYTES_FROM_END_16; //BYTES_FROM_END_16;    //16 bytes from the end of EEPROM = 4080 //compiler wouldn't allow initialization as int
eeprom unsigned char *adc_voltage_offset = (eeprom unsigned char *)BYTES_FROM_END_16;   //SJL - CARVR2
//eeprom unsigned char* adc_current_offset = BYTES_FROM_END_8;                          //8 bytes from the end of EEPROM
eeprom unsigned char *adc_current_offset = (eeprom unsigned char *)BYTES_FROM_END_8;    //SJL - CARVR2
//eeprom unsigned char* serial = BYTES_FROM_END_16 - 11;
eeprom unsigned char *serial = (eeprom unsigned char *)BYTES_FROM_END_16-11;            //SJL - CARVR2
//char adc_print = false;

float input_min[MAX_INPUTS];
float input_max[MAX_INPUTS];
unsigned long pulse_aggregate[2];

//float input_running_total[MAX_INPUTS];
//unsigned long input_samples[MAX_INPUTS];
unsigned int current_duty_high[MAX_INPUTS];
unsigned int current_duty_count[MAX_INPUTS];
//unsigned int last_duty_high[MAX_INPUTS];
//unsigned int last_duty_count[MAX_INPUTS];

unsigned long frequency6 = 0;
unsigned long frequency7 = 0;
char pulse_flag;
char counter = 0;	//SJL - CAVR2 - defined as 0

bit pulse6_timer_valid = 0;	// 1
bit pulse7_timer_valid = 0;	// 1
bit pulse_instant_pause = 0;

//unsigned int pulse_count=0;


#define PULSES_FILLED 0x80



/*******************************************************************************
 * void input_adcStart (unsigned char channel)

 * This function initialises all of the inputs DDR
 *
 * Project : 	 SMS300
 * Author: 		 Chris Cook
 * Date :  		 April 2005
 * (c) EDAC Electronics LTD 2005
 ******************************************************************************/
void input_init (void)
{
     unsigned char chan;
	 unsigned char j;

	 //Init Globals
	 channel=0;				 //The currentZone channel you are reading
     alarmCount=0;
     pulse_flag = 0;

     // ADC initialization
	 // ADC Clock frequency: 46.875 kHz
	 // ADC Voltage Reference: Int., cap. on AREF
	 // ADC Conversion complete interrupt enabled
	 ADMUX=ADC_VREF_TYPE;
	 ADCSRA=0x8F;
	 //CHANGE
	 cbi(ADCSRA,0x08);
     pulse_aggregate[0]=0x00;
     pulse_aggregate[1]=0x00;
	 for (chan=0; chan < MAX_INPUTS; chan++)
	 {
	    input[chan].ADCVal=0;
	    input_max[chan]=0x00;
	    input_min[chan]=0xFFFF;
	    input_samples[chan]=0x00;
	    input_running_total[chan]=0x00;
	    //TODO, this stuffs up the start up alarms...  May be fixed now

        for (j=0; j < INPUT_MAX_ALARMS; j++)
	    {
	        //TODO handle the startup alarm
		    input[chan].alarm[j].debounceTimer = 0;
		    input[chan].alarm[j].debounceDone = false;

		    if(config.input[chan].alarm[j].startup_alarm)
		    input[chan].alarm[j].currentZone = saved_state[chan][j];
	    }
	   // if(input[chan].alarm[0].currentZone == ZONE1 && input[chan].alarm[1].currentZone == ZONE1)
	   // {
	        input[chan].sensor.buffer = 0x55; //SJL 3.xx.8 - modified from 0x01
    	    //input[chan].sensor.currentZone = ZONE1;	//SJL 3.xx.8 - no longer used
			input[chan].sensor.alarmZone = ZONE1;	//SJL 3.xx.8 - initialization added
    //	}
   // 	else
   // 	{
    	//    input[chan].sensor.buffer = ZONE2;
    	 //   input[chan].sensor.currentZone = ZONE2;
     //	}

	 }
	 // Timer/Counter 2 initialization
     // Clock source: System Clock
     // Clock value: 5.859 kHz
     // Mode: Normal top=FFh
     // OC2 output: Disconnected

     TCCR2=0x05;
      #if CLOCK == SIX_MEG
     TCNT2=0x16;
     #elif CLOCK == SEVEN_SOMETHING_MEG
     TCNT2=0x00;
     #endif
     OCR2=0x00;


    // External Interrupt(s) initialization
    // INT0: Off
    // INT1: Off
    // INT2: Off
    // INT3: Off
    // INT4: Off
    // INT5: Off
    // INT6: On
    // INT6 Mode: Rising Edge
    // INT7: On
    // INT7 Mode: Rising Edge,
    #if PULSE_COUNTING_AVAILABLE
    EICRA=0x00;
    EICRB|=0xF0;
    EIMSK|=0xC0;
    EIFR|=0xC0;
    #endif


    // Turn on Timer 2 interrupt
    //sbi(TIMSK,0x40);

	 // ADC initialization
	 // ADC Clock frequency: 46.875 kHz
	 // ADC Voltage Reference: Int., cap. on AREF
	 // ADC Conversion complete interrupt enabled
	 ADMUX=ADC_VREF_TYPE;
	 ADCSRA=0x8F;
	//CHANGE
	 cbi(ADCSRA,0x08);
	 return;
}

/*******************************************************************************
 * void input_adcStart (unsigned char channel)

 * This function sets the ADC multiplexer to the channel you are wanting to read
 * then starts an ADC conversion.  An interrupt is thrown when the conversion
 * is complete.
 *
 * Project : 	 SMS300
 * Author: 		 Chris Cook
 * Date :  		 April 2005
 * (c) EDAC Electronics LTD 2005
 ******************************************************************************/

void input_adcStart (unsigned char channel)
{
    //Change the ADC channel to read
	ADMUX = channel | ADC_VREF_TYPE;

	//Start the conversion
	ADCSRA |= 0x40;
	//An ADC interrupt will occur when the conversion is complete.
	return;
}

#if CLOCK == SIX_MEG
#define RATE 5859.0
#elif CLOCK == SEVEN_SOMETHING_MEG
#define RATE 7200.0
#endif

#if PULSE_COUNTING_AVAILABLE
//Pulse interrupt routine for Input 7
//runtime approx 40us
interrupt [EXT_INT6] void ext_int6_isr(void)
{
	/*
    char l,last_l;
    unsigned int i,h,last_h;
    #ifdef INT_TIMING
    start_int();
    #endif
    l = TCNT1L;
    h = TCNT1H;
    last_l = OCR1AL;
    last_h = OCR1AH;

    l--;
    if(l==0xFF)
        h--;

    OCR1AH = h;
    OCR1AL = l;

    //check to see that the current
    if(frequency6 < 0xFFFFFFFF)
    {
        //counter has overflowed between pulses
        if((l + (h << 8)) < (last_l + (last_h << 8)))
        {
        #pragma warn-
            i = ((unsigned long)(l + (h << 8) + 0x10000)) - ((unsigned long)(last_l + (last_h << 8)));
        #pragma warn+
        }
        else
            i = (l + (h << 8)) - (last_l + (last_h << 8));
        
		frequency6 = i;    //frequency of the timer / ticks since last pulse
    }
    else
        frequency6 = 0xFFFFFFFE;
    pulse_aggregate[0]++;
    #ifdef INT_TIMING
    stop_int();
    #endif
	*/
	
	unsigned char l, last_l, h, last_h;
	unsigned int new_full, last_full, i;
	
    #ifdef INT_TIMING
    start_int();
    #endif
	
	//disable interrupts
	//SREG &= ~(0x80);
	
	//add to the aggregate first to make sure its counted
	pulse_aggregate[0]++;
		
	l = TCNT1L;
    h = TCNT1H;
	
    last_l = OCR1AL;
    last_h = OCR1AH;
	
	// #ifdef PULSE_DEBUG
	// sprintf(buffer,"new=0x%02X%02X,",h,l);print1(buffer);
	// sprintf(buffer,"prev=0x%02X%02X,",last_h, last_l);print1(buffer);
	// #endif	
	
	//decrement the timer value before saving to compare register...
	//this prevents the compa isr from interrupting this routine
	if(l == 0)
	{
		h--;
	}
	l--;
	
	OCR1AH = h;	
    OCR1AL = l;
		
	new_full = h;
	new_full = (new_full << 8);
	new_full = new_full + l;
	
	last_full = last_h;
	last_full = (last_full << 8);
	last_full = last_full + last_l;
	
	// #ifdef PULSE_DEBUG
	// sprintf(buffer,"0x%04X,",new_full);print1(buffer);
	// sprintf(buffer,"0x%04X,",last_full);print1(buffer);
	// #endif	
	
	if(pulse6_timer_valid)
    {
        //counter has overflowed between pulses
        if(new_full < last_full)
        {
			i = (0xFFFF - last_full) + new_full;
        }
        else
		{
            i = new_full - last_full;
		}
        
		frequency6 = i;    //frequency of the timer / ticks since last pulse
    }
    else
	{
        frequency6 = 65535; //0xFFFFFFFE;
		// #ifdef PULSE_DEBUG			
		// sprintf(buffer,"No Pulse,");
		// print1(buffer);
		// #endif
	}
	
	if(frequency6 < 58)	//de-bounce and discount frequencies higher than [ 1 / (57 * {1/5859}) = ] 102.789 Hz
	{
		pulse_aggregate[0]--;
	}
	
	pulse6_timer_valid = 1;	
	
	//enable interrupts	
	//SREG |= 0x80;    
	
    #ifdef INT_TIMING
    stop_int();
    #endif
}
#endif

// Timer 1 output compare A interrupt service routine
// no pulses in the last ~11s on pulse input 1 (input 7), current frequency = 0
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
    #ifdef INT_TIMING
    start_int();
    #endif
	
    //frequency6 = 0xFFFFFFFF;
	pulse6_timer_valid = 0;
	frequency6 = 0;
    
	#ifdef INT_TIMING
    stop_int();
    #endif
}

//Pulse interrupt routine for Input 8
/************************************************************
 * PULSE Interrupt Routines
 *
 * We need to know the frequency of pulses coming into the unit.
 * By measuring the number of clock ticks between the most recent pulse and the
 * last we can discover this value accurately, with the caveat that the frequency
 * is only updated when a pulse is recieved.
 *   In order to detect the frequency dropping to zero a timeout is needed, we are
 * using TIM1_COMPA and TIM1_COMPB for this purpose, resetting the comparison values
 * to the count on the most recently recieved pulse. After approx. 11 second the
 * comparison interrupt will trigger and we can set the current frequency (frequency6
 * or frequency7) to max scale.
 *    After the frequency has hit zero a pulse is needed to start the counter timing
 * again. If the frequency is below 1 pulse every 11 seconds the reading will
 * stay at zero, although aggregates will still move.
 *
 *
 ***********************************************************/
#if PULSE_COUNTING_AVAILABLE
interrupt [EXT_INT7] void ext_int7_isr(void)
{
	/*
    //char l,last_l;			SJL corrected to unsigned int
	unsigned int l,last_l;
    unsigned int i,h,last_h;
	#ifdef PULSE_DEBUG
	float instant_freq;
	unsigned long new, last, new_high, last_high;
	unsigned int j;
	#endif
    #ifdef INT_TIMING
    start_int();
    #endif
	
	//disable interrupts
	SREG &= ~(0x80);
	
    l = TCNT1L;	// *2 SO THIS IS NOW 65535
    h = TCNT1H;
	
	last_l = OCR1BL;
    last_h = OCR1BH;
	
	//make sure the 8 MSB's are 0
	l &= ~(0xFF00);
	h &= ~(0xFF00);
	last_l &= ~(0xFF00);
	last_h &= ~(0xFF00);

	#ifdef PULSE_DEBUG
	sprintf(buffer,"new=0x%02X%02X,",h,l);print1(buffer);
	sprintf(buffer,"prev=0x%02X%02X,",last_h, last_l);print1(buffer);
	#endif
	
	l--;		// *5 BUT THIS DECREMENT SHOULD STOP THIS HAPPENING??
				// UNLESS THE COUNTER IS COUNTING DOWN! set to count up
    //if(l==0xFF)
	if(l>0xFE)
	{
		#ifdef PULSE_DEBUG
		sprintf(buffer,"dec h\r\n");
		print1(buffer);
		#endif
        h--;
	}
		
    OCR1BH = h;	// *1 SETTING THIS TOO QUICKLY AND FIRING THE COMPARE INTERRUPT ^
    OCR1BL = l;	// *3 AND KEEPS REPEATING A NUMBER OF TIMES DEPENDING ON THE INPUT FREQUENCY

				// *4 SO MOVING LINES 346-357 TO THE END OF THE FUNCTION MIGHT STOP THIS?
		
    //check to see that the current
    if(frequency7 < 0xFFFFFFFF)
    {
        //counter has overflowed between pulses
        if((l + (h << 8)) < (last_l + (last_h << 8)))
        {
			#pragma warn-
            i = ((unsigned long)(l + (h << 8) + 0x10000)) - ((unsigned long)(last_l + (last_h << 8)));
			#pragma warn+
			
			#ifdef PULSE_DEBUG
			if(i < 380)
			{
				sprintf(buffer,"counter overflow - ticks since last = %u [0x%X]\r\n",i,i);
				print1(buffer);
			}
			#endif
        }
        else
		{
            i = (l + (h << 8)) - (last_l + (last_h << 8));
			
			#ifdef PULSE_DEBUG
			if(i < 380)
			{
				sprintf(buffer,"no overflow - ticks since last = %u\r\n",i);
				print1(buffer);
			}
			#endif
		}
        
		frequency7 = i;    //frequency of the timer / ticks since last pulse
    }
    else
	{
        frequency7 = 0xFFFFFFFE;
		#ifdef PULSE_DEBUG			
		sprintf(buffer,"f7 set\r\n");
		print1(buffer);
		#endif
	}
	
	#ifdef PULSE_DEBUG
	sprintf(buffer,"freq=%u,",frequency7);
	print1(buffer);
	instant_freq = RATE / frequency7;	//5859
	sprintf(buffer,"%f\r\n",instant_freq);
	print1(buffer);
	
	if(frequency7 > 65000)
	{
		sprintf(buffer,"frequency7 = %u \r\n",frequency7);print1(buffer);
		sprintf(buffer,"new low = 0x%X\r\n",l);print1(buffer);
		sprintf(buffer,"new high = 0x%X\r\n",h);print1(buffer);	
		sprintf(buffer,"prev low = 0x%X\r\n",last_l);print1(buffer);
		sprintf(buffer,"prev high = 0x%X\r\n",last_h);print1(buffer);
	}	
	#endif
		
    pulse_aggregate[1]++;
	
	SREG |= 0x80;
    //enable interrupts	
	
    #ifdef INT_TIMING
    stop_int();
    #endif
	*/
	
	unsigned char l, last_l, h, last_h;
	unsigned int new_full, last_full, i;
	
    #ifdef INT_TIMING
    start_int();
    #endif
	
	//disable interrupts
	//SREG &= ~(0x80);
	
	//add to the aggregate first to make sure its counted
	pulse_aggregate[1]++;
	
    l = TCNT1L;
	h = TCNT1H;
	
	last_l = OCR1BL;    
	last_h = OCR1BH;	
	
	// #ifdef PULSE_DEBUG
	// sprintf(buffer,"new=0x%02X%02X,",h,l);print1(buffer);
	// sprintf(buffer,"prev=0x%02X%02X,",last_h, last_l);print1(buffer);
	// #endif	
	
	//decrement the timer value before saving to compare register...
	//this prevents the compb isr from interrupting this routine
	if(l == 0)
	{
		h--;
	}
	l--;
	
	OCR1BH = h;	
    OCR1BL = l;
		
	new_full = h;
	new_full = (new_full << 8);
	new_full = new_full + l;
	
	last_full = last_h;
	last_full = (last_full << 8);
	last_full = last_full + last_l;
	
	// #ifdef PULSE_DEBUG
	// sprintf(buffer,"0x%04X,",new_full);print1(buffer);
	// sprintf(buffer,"0x%04X,",last_full);print1(buffer);
	// #endif	
	
	if(pulse7_timer_valid)
    {
        //counter has overflowed between pulses
        if(new_full < last_full)
        {
			i = (0xFFFF - last_full) + new_full;
        }
        else
		{
            i = new_full - last_full;
		}
        
		frequency7 = i;    //frequency of the timer / ticks since last pulse
    }
    else
	{
        frequency7 = 65535; //0xFFFFFFFE;
		// #ifdef PULSE_DEBUG			
		// sprintf(buffer,"No Pulse,");
		// print1(buffer);
		// #endif
	}
	
	if(frequency7 < 58)	//de-bounce and discount frequencies higher than [ 1 / (57 * {1/5859}) = ] 102.789 Hz
	{
		pulse_aggregate[1]--;
	}
	
	pulse7_timer_valid = 1;	
	
	//enable interrupts	
	//SREG |= 0x80;    
	
    #ifdef INT_TIMING
    stop_int();
    #endif
	
}
#endif

interrupt [TIM1_COMPB] void timer1_compb_isr(void)
{
    #ifdef INT_TIMING
    start_int();
    #endif
    //frequency7 = 0xFFFFFFFF;
	
	//use an overflow flag instead
	pulse7_timer_valid = 0;
	frequency7 = 0;	
	
	// #ifdef PULSE_DEBUG			
	// sprintf(buffer,"t1 comp\r\n");
	// print1(buffer);
	// #endif
	
    #ifdef INT_TIMING
    stop_int();
    #endif
}

//dummy interrupt
interrupt [TIM1_OVF] void timer1_ovf_isr(void)
{
	// #ifdef PULSE_DEBUG			
	// sprintf(buffer,"t1 o/f\r\n");
	// print1(buffer);
	// #endif
}

/*******************************************************************************
 * interrupt [TIM2_OVF] void timer2_ovf_isr(void)
 *
 * The timer is set to interrupt every 40mS.  This forms the tick which
 * controls when to poll the inputs.  On a timer interrupt a ADC read is
 * initiated for the input channel that the system is up to.
 *
 * Profiling : Runs for 50uS every 40mS
  * Duty:      0.125%
 *
 * Project : 	 SMS300
 * Author: 		 Chris Cook
 * Date :  		 April 2005
 * (c) EDAC Electronics LTD 2005
 ******************************************************************************/

interrupt [TIM2_OVF] void timer2_ovf_isr(void)
{

    //Disable Timer2_overflow interrupt to allow higher priority interrupts to
    //interrupt over this routine, but don't allow it to re-interrupt itself.
    #ifdef INT_TIMING
    start_int();
    #endif
    cbi(TIMSK,0x40);
    cbi(ADCSRA,0x08);

    global_interrupt_on(); //Enable global interrupts

    //sprintf(buffer,"check timer interrupt is firing");	//SJL - CAVR2 - input debug
	//print(buffer);												//SJL - CAVR2 - input debug

    // Reinitialize Timer 2 value
    //
	 #if CLOCK == SIX_MEG
     TCNT2=0x16;
     #elif CLOCK == SEVEN_SOMETHING_MEG
     TCNT2=0x00;
     #endif

	if (++channel >= MAX_INPUTS)
	{
	    channel = 0;
	}
	//sprintf(buffer,"%d ",channel);print(buffer);

	if (config.input[channel].enabled)
	{
	    input_adcStart(channel);

    	//check the debouncing of this channel
    	for (alarmCount=0; alarmCount<INPUT_MAX_ALARMS; alarmCount++)
    	{
    	    if (input[channel].alarm[alarmCount].debounceTimer)
    		{
    		    if(input[channel].alarm[alarmCount].debounceTimer<32)
    		    {
    			    input[channel].alarm[alarmCount].debounceDone = true;
    			    input[channel].alarm[alarmCount].debounceTimer = 0;
    		    }
                else
                    input[channel].alarm[alarmCount].debounceTimer-=32;
    		}
    	}
	}



    switch(led_state)
    {
        case LED_POWER_ON:
            _power_led_on();
            break;
        case LED_POWER_OFF:
            _power_led_off();
            break;
        case LED_POWER_FLASH:
            _power_led_toggle();
            break;
    }

    switch(modem_state)
    {
        case NO_CONNECTION:
            _status_led_off();
            break;
        case CELL_CONNECTION:
			//sprintf(buffer,"Modem state = %d\r\n",modem_state);print(buffer);	//SJL - 290611
            #if MODEM_TYPE == Q2406B && !TCPIP_AVAILABLE
            _status_led_toggle();
            #else
            _status_led_on();
            #endif
            break;
        case GPRS_CONNECTION:
			//sprintf(buffer,"Modem state = %d\r\n",modem_state);print(buffer);	//SJL - 290611
            _status_led_on();
            break;
    }

      //check to see if we've dropped a CSD call. Only available V4+
    #if false
   // #if (HW_REV == V6 || HW_REV == V5)
    if(modem_TMEnabled && !csd_connected)
    {
        if(!counter)
        {
            #if SYSTEM_LOGGING_ENABLED
            sprintf(buffer,"Call dropped, waiting for NO CARRIER");log_line("system.log",buffer);
            #endif
        }

        if(counter > 60)
        {
            //Hang up the modem
            _status_led_off();
            modem_restart();
            modem_init(build_type);
            modem_TMEcho = false;
            modem_TMEnabled = false;
            queue_resume(&q_modem);
            #if SYSTEM_LOGGING_ENABLED
            sprintf(buffer,"CSD call dropped without NO CARRIER");log_line("system.log",buffer);
            #endif
            if(verbose)
            {
                sprintf(buffer,"\r\nCSD dropped without notification, cleaning up\r\n");
                print(buffer);
            }
        } else counter++;
    }
    else counter = 0;
    #endif



	//Turn off global interrupts, and reenable timer2 ovf interrupt.
	//note rti will turn global int back on.
	global_interrupt_off(); //disable global interrupts
    input_resume();
    sbi(TIMSK,0x40);
    sbi(ADCSRA,0x08);
    #ifdef INT_TIMING
    stop_int();
    #endif

}



/*******************************************************************************
 * interrupt [ADC_INT] void adc_isr(void)
 *
 * When an ADC conversion is complete, this interrupt occurs.  This then checks
 * the currentZone alarming status of the inputs both alarms, and checks if the
 * sensor is still reading in acceptable values (ie it hasn't become faulty.
 * If an input has changed zone, an event is pushed onto the main system queue,
 * also sensor failures are pushed onto this queue.
 *
 * Profiling: Runs for between 200uS and 450uS (depending how input type, and
 *            alarms enabled) every 40mS.
 * Duty:      1.13%
 *
 * Project : 	 SMS300
 * Author: 		 Chris Cook
 * Date :  		 April 2005
 * (c) EDAC Electronics LTD 2005
 ******************************************************************************/
interrupt [ADC_INT] void adc_isr(void)
{
    Event e;
    float c;
	unsigned long frequency6_local, frequency7_local;
    #ifdef INT_TIMING
    start_int();
    #endif

    //Disable adc interrupt to allow higher priority interrupts to
    //interrupt over this routine, but don't allow it to re-interrupt itself.
    cbi(TIMSK,0x40);
    cbi(ADCSRA,0x08);
    global_interrupt_on(); //Enable global interrupts

    //When the ADC conversion is complete save the 10 bit value
    //input[channel].ADCVal = ADCW;	//moved below pulse caluculations

    if(config.input[channel].type == PULSE)
    {
        if(channel == 6)
        {			
			SREG &= ~(0x80);
			frequency6_local = frequency6;
			SREG |= 0x80;
			
            if(pulse_flag&PULSE_FLAG_RESTART_7)
            {
                EIFR |= 0x40; 							//clear any residue
                EIMSK |= 0x40; 							//enable EXT_INT_6
                pulse_flag &=~ PULSE_FLAG_RESTART_7;	//clear bit - no pulse detected
            }
			else if(pulse_instant_pause == 0)
			{
				if(frequency6_local == 65535)
				{
					input[channel].ADCVal = 0.0001;			//pulse detected outside of allowed period (11.186s)
				}						
				else if(pulse6_timer_valid == 0)
				{
					input[channel].ADCVal = 0.0;			//no pulses detected in allowed period
				}
				else if(frequency6_local > 57)
				{											//valid pulse period detected
					input[channel].ADCVal = RATE/frequency6_local;
					switch(config.pulse[0].period)
					{
						case HOURS:
							input[channel].ADCVal *= 3600;
							break;
						case MINUTES:
							input[channel].ADCVal *= 60;
							break;
						case SECONDS:
							break;
					}
				}
			}
        }
		
        else if(channel == 7)
        {			
			SREG &= ~(0x80);
			frequency7_local = frequency7;
			SREG |= 0x80;
			
            if(pulse_flag&PULSE_FLAG_RESTART_8)
            {
                EIFR |= 0x80; 							//clear any residue
                EIMSK |= 0x80; 							//enable EXT_INT_7
                pulse_flag &=~ PULSE_FLAG_RESTART_8;	//clear bit - no pulse detected
            }
			else if(pulse_instant_pause == 0)
			{
				if(frequency7_local == 65535)
				{
					input[channel].ADCVal = 0.0001;			//pulse detected outside of allowed period (11.186s)
				}						
				else if(pulse7_timer_valid == 0)
				{
					input[channel].ADCVal = 0.0;			//no pulses detected in allowed period
				}
				else if(frequency7_local > 57)
				{											//valid pulse period detected
					input[channel].ADCVal = RATE/frequency7_local;
					switch(config.pulse[1].period)
					{
						case HOURS:
							input[channel].ADCVal *= 3660;
							break;
						case MINUTES:
							input[channel].ADCVal *= 60;
							break;
						case SECONDS:
							break;
					}
				}
			}
		}
    }
	else
	{
		input[channel].ADCVal = ADCW;
	}

    //Work through all of the input alarms (default is 2 alarms)
	for (alarmCount=0; alarmCount<INPUT_MAX_ALARMS; alarmCount++)
	{
    	//sprintf(buffer,"Checking alarm %d\r\n",alarmCount);print(buffer);
        //sprintf(buffer,"Alarm type = %d\r\n",config.input[channel].alarm[alarmCount].type);print(buffer);

    	//Only check the alarm if it is enabled.
		if (config.input[channel].alarm[alarmCount].type != ALARM_NONE)
		{
    		//sprintf(buffer,"%d A%d=%d ",channel,alarmCount,config.input[channel].alarm[alarmCount].type);print(buffer);
    		//Using this ADC Value, write the latest sample into the buffer.
			//This sample is 8 bits wide, and each zone only takes up 2 bits.
			//Shift the buffer left by 2 to make space for the latest sample
			//then AND the latest sample to the end of the buffer.

    		input[channel].alarm[alarmCount].buffer = input_lookupZone(input[channel].ADCVal,
    		          config.input[channel].alarm[alarmCount].thresholdLow,
					  config.input[channel].alarm[alarmCount].thresholdHigh);

    		//sprintf(buffer,"input[%d][%d]:cz=%d,buffer=%d\r\n",channel,alarmCount,input[channel].alarm[alarmCount].currentZone,input[channel].alarm[alarmCount].buffer);print(buffer);

    		//Has there been a change in zone
    		if (input_zoneChange(input[channel].alarm[alarmCount].currentZone,
				               input[channel].alarm[alarmCount].buffer,
				               input[channel].alarm[alarmCount].alarmZone))
        	{
        	    //If true, check to see if we were in the middle of debouncing
			    //and if it's complete
    			if (input[channel].alarm[alarmCount].debounceDone)
        		{
        		    //Debounce complete
        			input[channel].alarm[alarmCount].debounceDone = false;

                    if(input_int_sms==true)
    				{

                        //Push events that have occured
                        if((input[channel].alarm[alarmCount].currentZone == ZONE1 &&
                               (input[channel].alarm[alarmCount].buffer & LATEST_SAMPLE) == ZONE2) ||
                               (input[channel].alarm[alarmCount].currentZone == ZONE0 &&
                               (input[channel].alarm[alarmCount].buffer & LATEST_SAMPLE) == ZONE2))
                        {
                            //A rising event has occured, push this onto the queue
                            //sprintf(buffer,"pushing a rising event from the input interrupt");print(buffer);
                            e.type = event_CHXR + alarmCount;
                            e.param = channel;
                            queue_push(&q_event, &e);
                            //sprintf(buffer,"pushR%d ",event_CHXR + alarmCount);print(buffer);
                            input[channel].alarm[alarmCount].alarmZone = (input[channel].alarm[alarmCount].buffer & LATEST_SAMPLE);
                        }
                        else if((input[channel].alarm[alarmCount].currentZone == ZONE1 &&
                                    (input[channel].alarm[alarmCount].buffer & LATEST_SAMPLE) == ZONE0) ||
                                    (input[channel].alarm[alarmCount].currentZone == ZONE2 &&
                                    (input[channel].alarm[alarmCount].buffer & LATEST_SAMPLE) == ZONE0))
                        {
                            //A falling event has occured, push this onto the queue
                            //sprintf(buffer,"pushing a falling event from the input interrupt");print(buffer);
                            e.type = event_CHXF + alarmCount;
                            e.param = channel;
                            queue_push(&q_event, &e);
                            //sprintf(buffer,"pushR%d ",event_CHXF + alarmCount);print(buffer);
                            input[channel].alarm[alarmCount].alarmZone = (input[channel].alarm[alarmCount].buffer & LATEST_SAMPLE);
                        }

                    }

        			//Update the currentZone zone
        			input[channel].alarm[alarmCount].currentZone = (input[channel].alarm[alarmCount].buffer & LATEST_SAMPLE);
        		}
    			else if (input[channel].alarm[alarmCount].debounceTimer)
    			{
    			    //Still debouncing so wait
    				continue;
    			}
        		else
    			{
        			//This is the first time the change of zone has been detected,
    			    //start the debouncing
    				input[channel].alarm[alarmCount].debounceTimer = config.input[channel].alarm[alarmCount].debounce_time+1;
    			}
        	}
        	else if (input[channel].alarm[alarmCount].debounceTimer)
        	{
        		//debounce has failed, clear the debounce timers
        		input[channel].alarm[alarmCount].debounceTimer = 0;
        		input[channel].alarm[alarmCount].debounceDone = false;
        	}
		}
     //End loop input[channel].alarm[alarmCount]
	}
	
	//ANALOGUE INPUTS ONLY - CHECK FOR SENSOR OUT OF BOUNDS CONDITION
	if ((config.input[channel].type != DIGITAL) && (config.input[channel].type != PULSE))
	{
    	//sensor.buffer stores the zone for the last four ADC coversions
		//shift the buffer left to make room for current zone
		//moves the last recorded state of the input to the next position in the buffer e.g.
		//if x was the previous ADC conversion and x+1 is the current ADC conversion
		// ADC conversion	x-3	x-2	x-1	x    after shift	x-2	x-1	x	x+1
		//					11	00	00	01					00	00	01	__
		input[channel].sensor.buffer = input[channel].sensor.buffer << 0x2;
		
		//add the current zone to the buffer __
		input[channel].sensor.buffer += input_lookupZone(input[channel].ADCVal+1,config.input[channel].sensor.thresholdLow,config.input[channel].sensor.thresholdHigh);

		/* #ifdef _INPUT_DEBUG_
		//if(channel==1)
		//{
			//sprintf(buffer,"\r\ninput[%d].ADCVal+1 = %f\r\n",channel,input[channel].ADCVal+1);print1(buffer);
			//sprintf(buffer,"config.input[%d].sensor.thresholdLow = %d\r\n",channel,config.input[channel].sensor.thresholdLow);print1(buffer);
			//sprintf(buffer,"config.input[%d].sensor.thresholdHigh = %d\r\n",channel,config.input[channel].sensor.thresholdHigh);print1(buffer);
			//sprintf(buffer,"[current  zone] buffer = %X\r\n",input[channel].sensor.buffer);print1(buffer);
			//sprintf(buffer,"[previous zone] currentZone = %X\r\n",input[channel].sensor.currentZone);print1(buffer);
			//sprintf(buffer,"channel = %d, alarmZone = %X\r\n",channel,input[channel].sensor.alarmZone);print1(buffer);
		//}
		#endif */	

		//Has the input been in the same zone for 4 successive ADC conversions?
		//4 conversions takes 3x320ms = 960ms		
		//If the zones are the same they will be either
		//All zone2 = 0b10101010 = 0xAA - OOB high
		//All zone1 = 0b01010101 = 0x55 - in bounds
		//All zone0 = 0b00000000 = 0x00 - OOB low
		if ((input[channel].sensor.buffer == ALL_IN_ZONE0) || (input[channel].sensor.buffer == ALL_IN_ZONE1) || (input[channel].sensor.buffer == ALL_IN_ZONE2))
		{
			//Has there been a change of zone?
			//the current zone (buffer & LATEST_SAMPLE) must be different to:
			//the current zone the input is in (alarmZone & LATEST_SAMPLE) (to stop multiple OOB messages while in zone 0 or 2)
			//if (((input[channel].sensor.currentZone & LATEST_SAMPLE) != (input[channel].sensor.buffer & LATEST_SAMPLE)) 
				//&& ((input[channel].sensor.alarmZone & LATEST_SAMPLE) != (input[channel].sensor.buffer & LATEST_SAMPLE)))
			if ((input[channel].sensor.alarmZone & LATEST_SAMPLE) != (input[channel].sensor.buffer & LATEST_SAMPLE))
			{	
				#ifdef _INPUT_DEBUG
				//if(channel==0)
				//{
					sprintf(buffer,"*** ZONE CHANGE - ");print1(buffer);					
				//}
				#endif
				
				//Update the alarm zone
				input[channel].sensor.alarmZone = input[channel].sensor.buffer & LATEST_SAMPLE;
				
				//Has the input moved into zone 0 or 2 - OOB?
				if(input[channel].sensor.buffer != ALL_IN_ZONE1)
				{
					#ifdef _INPUT_DEBUG
					//if(channel==0)
					//{
						sprintf(buffer,"OOB ***\r\n");print1(buffer);					
					//}
					#endif					
					
					e.type = event_SENSOR_FAIL;
					e.param = channel;
					queue_push(&q_event, &e);										
				}				
				#ifdef _INPUT_DEBUG
				else	//the input has moved into zone 1 - in bounds?
				{				
					//if(channel==0)
					//{
						sprintf(buffer,"1 ***\r\n");print1(buffer);					
					//}				
				}
				#endif
			}
		}		
	}
	
	else if(config.input[channel].type == DIGITAL)//digital
    {
    	input[channel].alarm[ALARM_A].buffer = input_lookupZone(input[channel].ADCVal,
    		config.input[channel].alarm[ALARM_A].thresholdLow,
			config.input[channel].alarm[ALARM_A].thresholdHigh);
    }



    if(config.input[channel].type == DIGITAL)
    {
        if(input[channel].alarm[ALARM_A].buffer == ZONE2)
        {
            if(input[channel].ADCVal < 1)
                input_min[channel] = 1;
            if(input[channel].ADCVal > 1)
                input_max[channel] = 1;
            input_running_total[channel]+=1;
            current_duty_high[channel]++;
            //sprintf(buffer,"input %d is in zone 2\r\n",channel);	//SJL - CAVR2 - input debug
			//print(buffer);										//SJL - CAVR2 - input debug
        }
        else
        {
            if(input[channel].ADCVal < 0)
                input_min[channel] = 0;
            if(input[channel].ADCVal > 0)
                input_max[channel] = 0;
            //sprintf(buffer,"input %d is in zone 1\r\n",channel);	//SJL - CAVR2 - input debug
			//print(buffer);										//SJL - CAVR2 - input debug
        }
        input_samples[channel]++;
        current_duty_count[channel]++;
    }
    else
    {
        if(input[channel].ADCVal < input_min[channel])
            input_min[channel] = input[channel].ADCVal;
        if(input[channel].ADCVal > input_max[channel])
            input_max[channel] = input[channel].ADCVal;
        if(config.input[channel].type != PULSE)
            input_running_total[channel] += config.input[channel].conv_int+config.input[channel].conv_grad*input[channel].ADCVal;
        input_samples[channel]++;
		
		//
		//if(log_entry_timer%100)
		//{
			//calculate the average 
		//}
    }

    //Disable global interrupts and re-enable the adc int.
    global_interrupt_off(); //disable global interrupts
    sbi(TIMSK,0x40);
    sbi(ADCSRA,0x08);
    //global interrupts get enabled when this interrupt finishes

    #ifdef INT_TIMING
    stop_int();
    #endif
}

/*******************************************************************************
 * input_lookupZone (unsigned int adc, unsigned int threshold1, unsigned int threshold2)
 *
 * This returns the zone that the ADC value corresponds to for the alarm
 *
 * Project : 	 SMS300
 * Author: 		 Chris Cook
 * Date :  		 April 2005
 * (c) EDAC Electronics LTD 2005
 ******************************************************************************/

unsigned char input_lookupZone (unsigned int adc, unsigned int threshold1, unsigned int threshold2)
{
    if (adc < threshold1)
        return 0;
    else if (adc < threshold2)
        return 1;
    else
        return 2;
}


/*******************************************************************************
 * bool input_zoneChange(unsigned char currentZone, unsigned char buffer, unsigned char alarmZone)
 *
 * This function tells you if the input alarm or sensor has changed zones.
 * 4 readings must be the same before a successul change of zone is
 * returned.
 *
 * Project : 	 SMS300
 * Author: 		 Chris Cook
 * Date :  		 April 2005
 * (c) EDAC Electronics LTD 2005
 ******************************************************************************/

bool input_zoneChange(unsigned char currentZone, unsigned char buffer, unsigned char alarmZone)
{
    //sprintf(buffer," cZ=%d lZ=%d ",currentZone, (buffer & LATEST_SAMPLE));print(buffer);
    //If the zones are the same they will be either
    //All zone2 = 0b10101010 = 0xAA
	//All zone1 = 0b01010101 = 0x55
	//All zone0 = 0b00000000 = 0x00

	//Change - hard coding off of debouncing
	//if ((buffer == ALL_IN_ZONE0) || (buffer == ALL_IN_ZONE1) || (buffer == ALL_IN_ZONE2))
	//{
	    //See if the you are in a different zone from the currentZone saved,
		//if you are return true as there has been a zone change
		//CHANGE made here.
		if ((currentZone != (buffer & LATEST_SAMPLE)) && (alarmZone != (buffer & LATEST_SAMPLE)))
		{
		    return true;
		}
	//}
	//Else, the zones are not all the same, so the input has not settled, or
	//you are already in the zone.  Therefore return false, as there has not
	//been a zone change.
	return false;
}

/*******************************************************************************
 * unsigned char input_setup(unsigned char num)
 *
 * This function converts all of the configuration parameters setup for the
 * input into the threshold values for the input alarming.  It also sets ups
 * the appropriate values for sensor out of bounds alarming.
 *
 * Project : 	 SMS300
 * Author: 		 Chris Cook
 * Date :  		 April 2005
 * (c) EDAC Electronics LTD 2005
 ******************************************************************************/

unsigned char input_setup(unsigned char num)
{
    unsigned int tempThreshold1, tempThreshold2;
    unsigned char i=0;

    #if (HW_REV == V6 || HW_REV == V5)
    //The V5 hardware requires internal pull up resistors to be used for
    //the digital inputs, make sure this is turned off for all other input
    //types.
        cbi(PORTF,(1<<num));
        #ifdef DEBUG
        sprintf(buffer,"Disabling pullup on %d\r\n",num);
        print(buffer);
        #endif
    #endif
    for (i=0; i<INPUT_MAX_ALARMS;i++)
    {
        //Hard code the digital parameters
        if (config.input[num].type == DIGITAL && i == 0)
        {

            #if (HW_REV == V6 || HW_REV == V5)
            //The V5 hardware requires internal pull up resistors to be used for
            //the digital inputs.
                #ifdef DEBUG
                sprintf(buffer,"Enabling pullup on %d\r\n",num);
                print(buffer);
                #endif
                sbi(PORTF,(1<<num));
            #endif
            //Alarm when open, means that an alarm event is thrown when the input is high
            //i=0
            if (config.input[num].alarm[0].type == ALARM_OPEN)
            {
                //Setup the thresholds to match this
                config.input[num].alarm[0].thresholdLow = DIGITAL_LOW;
                config.input[num].alarm[0].thresholdHigh = DIGITAL_HIGH;
            }
            //Alarm when closed, means that an alarm event is thrown when the input is low
            else
            {
                //Setup the thresholds to match this
                config.input[num].alarm[0].thresholdLow = DIGITAL_HIGH;
                config.input[num].alarm[0].thresholdHigh = DIGITAL_LOW;
            }
            //Disable alarm two for the digital input CHANGE now disabled at the end of this loop

            //Don't ever test the sensor out of bounds failure so set thresholds out of bounds
            config.input[num].sensor.thresholdLow = OUT_OF_BOUNDS;
            config.input[num].sensor.thresholdHigh = OUT_OF_BOUNDS;
        }
        //Handle the 0-5V input type
        else if (config.input[num].type == ANALOG_0_5V)
        {
            //Calculate the thresholds for the input, and the conversion constants
            //This is in the form y = mx + c.  Where y is the analog value you are wanting to read
            //back from the unit.  m is the \ and c is the conv_int.
            #ifdef DEBUG
            sprintf(buffer,"Working out 0-5V settings for input %d\r\n",num+1);
            print(buffer);
            #endif

            #ifndef CALIBRATED
            //Calculate the conv_grad, m = (analog max - analog min) / (ADC conversion range)
            config.input[num].conv_grad = ((config.input[num].eng_max - config.input[num].eng_min) / (float)(ADCVal_5V - ADCVal_0V));	//SJL - CAVR2 - double not supported
            config.input[num].conv_int = config.input[num].eng_min - (config.input[num].conv_grad * ADCVal_0V);
            #else
            config.input[num].conv_grad = ((config.input[num].eng_max -
              config.input[num].eng_min) / (float)(((float)ADCVAL_5V+(float)adc_voltage_offset[num]) - (float)ADCVAL_0V));	//SJL - CAVR2 - double not supported
            config.input[num].conv_int = config.input[num].eng_min - (config.input[num].conv_grad * ADCVAL_0V);
            #endif

            #ifdef DEBUG
            sprintf(buffer,"grad=%f, int=%f\r\n",config.input[num].conv_grad, config.input[num].conv_int);print(buffer);
            #endif
            //Calculate the thresholds.  Calculate the set point first, then calc the reset,
            //compare which is smallest, and store in thresholdLow.  This corresponds to throwing
            //the falling event.  The thresholdHigh corresponds to throwing the rising
            //event.
            tempThreshold1 = (unsigned int)((config.input[num].alarm[i].set - config.input[num].conv_int) / config.input[num].conv_grad);
            tempThreshold2 = (unsigned int)((config.input[num].alarm[i].reset - config.input[num].conv_int) / config.input[num].conv_grad);


            if (tempThreshold1 < tempThreshold2)
            {
                config.input[num].alarm[i].thresholdLow = tempThreshold1;
                config.input[num].alarm[i].thresholdHigh = tempThreshold2;
            }
            else
            {
                config.input[num].alarm[i].thresholdLow = tempThreshold2;
                config.input[num].alarm[i].thresholdHigh = tempThreshold1;
            }


            //Set the bounds for the sensor testing, to make sure that the sensors
            //are working correctly.
            #ifndef CALIBRATED
            //Low threshold corresponds to a 0V adc reading
            config.input[num].sensor.thresholdLow = ADCVal_0V;
            //High threshold corresponds to a 5V adc reading
            config.input[num].sensor.thresholdHigh = ADCVal_5V+10;
            #else
            //Low threshold corresponds to a 0V adc reading
            config.input[num].sensor.thresholdLow = ADCVAL_0V;
            //High threshold corresponds to a 5V adc reading
            config.input[num].sensor.thresholdHigh = ADCVAL_5V+adc_voltage_offset[num]+10;
            #endif
        }

        else if (config.input[num].type == ANALOG_4_20_mA)
        {
            //Calculate the thresholds for the input, and the conversion constants
            //This is in the form y = mx + c.  Where y is the analog value you are wanting to read
            //back from the unit.  m is the conv_grad and c is the conv_int.

            #ifndef CALIBRATED
            //Calculate the conv_grad, m = (analog max - analog min) / (ADC conversion range)
            config.input[num].conv_grad = ((config.input[num].eng_max - config.input[num].eng_min) / (double)(ADCVal_20mA - ADCVal_4mA));
            config.input[num].conv_int = config.input[num].eng_min - (config.input[num].conv_grad * ADCVal_4mA);
            #else
            config.input[num].conv_grad = ((config.input[num].eng_max -
              config.input[num].eng_min) / (float)((float)ADCVAL_20mA+(float)adc_current_offset[num] - ((float)ADCVAL_4mA+(float)((float)adc_current_offset[num]/5))));	//SJL - CAVR2 - double not supported
            config.input[num].conv_int = config.input[num].eng_min - (config.input[num].conv_grad * (ADCVAL_4mA+(adc_current_offset[num]/5)));
            #endif



            //Calculate the thresholds.  Calculate the set point first, then calc the reset,
            //compare which is smallest, and store in thresholdLow.  This corresponds to throwing
            //the falling event.  The thresholdHigh corresponds to throwing the rising
            //event.
            tempThreshold1 = (unsigned int)((config.input[num].alarm[i].set - config.input[num].conv_int) / config.input[num].conv_grad);
            tempThreshold2 = (unsigned int)((config.input[num].alarm[i].reset - config.input[num].conv_int) / config.input[num].conv_grad);


            if (tempThreshold1 < tempThreshold2)
            {
                config.input[num].alarm[i].thresholdLow = tempThreshold1;
                config.input[num].alarm[i].thresholdHigh = tempThreshold2;
            }
            else
            {
                config.input[num].alarm[i].thresholdLow = tempThreshold2;
                config.input[num].alarm[i].thresholdHigh = tempThreshold1;
            }

            //Set the bounds for the sensor testing, to make sure that the sensors
            //are working correctly.
            #ifndef CALIBRATED
            //Low threshold corresponds to a 4mA adc reading
            config.input[num].sensor.thresholdLow = ADCVal_4mA-10;
            //High threshold corresponds to a 20mA adc reading
            config.input[num].sensor.thresholdHigh = ADCVal_20mA+10;
            #else
            //Low threshold corresponds to a 4mA adc reading
            config.input[num].sensor.thresholdLow = ADCVAL_4mA-10+(adc_current_offset[num]/5);
            //High threshold corresponds to a 20mA adc reading
            config.input[num].sensor.thresholdHigh = ADCVAL_20mA+adc_current_offset[num]+10;
			
			//sprintf(buffer,"config.input[%d].sensor.thresholdLow=%d\r\n",num,config.input[num].sensor.thresholdLow);print1(buffer);
			//sprintf(buffer,"config.input[%d].sensor.thresholdHigh=%d\r\n",num,config.input[num].sensor.thresholdHigh);print1(buffer);
		
            #endif
        }
        if (config.input[num].type == PULSE)
        {
            //Calculate the thresholds for the input, and the conversion constants
            //This is in the form y = mx + c.  Where y is the analog value you are wanting to read
            //back from the unit.  m is the \ and c is the conv_int.
            #ifdef _PULSE_COUNTING_DEVEL_
            sprintf(buffer,"Working out pulse settings for input %d\r\n",num+1);
            print(buffer);
            #endif


            if(num==6)
                config.input[num].conv_grad = config.pulse[0].pulses_per_count;
            else if(num==7)
                config.input[num].conv_grad = config.pulse[1].pulses_per_count;
            config.input[num].conv_int = 0;


            #ifdef _PULSE_COUNTING_DEVEL_
            sprintf(buffer,"grad=%f, int=%f\r\n",config.input[num].conv_grad, config.input[num].conv_int);print(buffer);
            #endif
            //Calculate the thresholds.  Calculate the set point first, then calc the reset,
            //compare which is smallest, and store in thresholdLow.  This corresponds to throwing
            //the falling event.  The thresholdHigh corresponds to throwing the rising
            //event.
            tempThreshold1 = config.input[num].alarm[i].set / config.input[num].conv_grad;
            tempThreshold2 = config.input[num].alarm[i].reset / config.input[num].conv_grad;


            if (tempThreshold1 < tempThreshold2)
            {
                config.input[num].alarm[i].thresholdLow = tempThreshold1;
                config.input[num].alarm[i].thresholdHigh = tempThreshold2;
            }
            else
            {
                config.input[num].alarm[i].thresholdLow = tempThreshold2;
                config.input[num].alarm[i].thresholdHigh = tempThreshold1;
            }


            //Set the bounds for the sensor testing, to make sure that the sensors
            //are working correctly.
            //Low threshold corresponds to a 0V adc reading
            config.input[num].sensor.thresholdLow = 0;
            //High threshold corresponds to a 5V adc reading
            //config.input[num].sensor.thresholdHigh = -1;	//SJL - CAVR2 - -1 constant out of range unsigned int and don't think it matters what this is set to
            config.input[num].sensor.thresholdHigh = OUT_OF_BOUNDS;
        }
       // else config.input[num].alarm[i].type=ALARM_NONE;          //added to keep digital inputs with no alarms in sync

    }
	
	//sprintf(buffer,"config.input[%d].sensor.thresholdLow = %d\r\n",num,config.input[num].sensor.thresholdLow);print1(buffer);
	//sprintf(buffer,"config.input[%d].sensor.thresholdHigh = %d\r\n",num,config.input[num].sensor.thresholdHigh);print1(buffer);

    return 0;
}

//extern eeprom char saved_state[MAX_INPUTS][2];
bool input_startup (unsigned char chan)
{
    unsigned int val;
    unsigned char i,zone;
    Event e;

    DEBUG_printStr("start ADC");
    val = input_readADC (chan);
    delay_ms(500);
    val = input_readADC (chan);
    input_running_total[chan] = 0;
    input_max[chan] = 0;
    input_min[chan] = 0xFFFF;
    if(chan>=6)
        pulse_aggregate[chan-6]=0;
    input_samples[chan] = 0;
    //Store the ADC val in it's buffer, for accurate message sending.
    input[chan].ADCVal = val;
    if(config.input[chan].type == PULSE)
    {
        if(channel == 6)
        {
            input[channel].sensor.buffer = RATE/frequency6;
            switch(config.pulse[0].period)
            {
                case HOURS:
                    input[channel].sensor.buffer *= 60;
                case MINUTES:
                    input[channel].sensor.buffer *= 60;
                case SECONDS:
                    break;
            }
            #ifdef _PULSE_COUNTING_DEVEL_
            sprintf(buffer,"channel 7: %d\r\n",input[channel].sensor.buffer);print(buffer);
            #endif
        }
        else if(channel == 7)
        {
			SREG &= ~(0x80);
            input[channel].sensor.buffer = RATE/frequency7;
			SREG |= 0x80;
            switch(config.pulse[1].period)
            {
                case HOURS:
                    input[channel].sensor.buffer *= 60;
                case MINUTES:
                    input[channel].sensor.buffer *= 60;
                case SECONDS:
                    break;
            }
            #ifdef _PULSE_COUNTING_DEVEL_
            sprintf(buffer,"channel 8: %d\r\n",input[channel].sensor.buffer);print(buffer);
            #endif
        }
    }
    #ifdef DEBUG
    sprintf(buffer,"inputStartup chan=%d ADC =%4d ",chan,val);print(buffer);
    #endif
    for (i=0; i<INPUT_MAX_ALARMS; i++)
	{
	    input[chan].alarm[i].buffer = saved_state[chan][i];
	    input[chan].alarm[i].currentZone = saved_state[chan][i];
	    if(i>0&&config.input[chan].type == DIGITAL)
	        break;
    	//Only check the alarm if it is enabled.
		if (config.input[chan].alarm[i].type != ALARM_NONE)
		{
		    #ifdef DEBUG
    		sprintf(buffer,"\r\n%d Alarm %c=%d ",chan,i+'a',config.input[chan].alarm[i].type);print(buffer);
    		#endif
    		zone = input_lookupZone(val, config.input[chan].alarm[i].thresholdLow,
					  config.input[chan].alarm[i].thresholdHigh);
    		#ifdef DEBUG
    		sprintf(buffer,"z=%d lZ=%d ",zone,saved_state[chan][i]);print(buffer);
    		#endif
    		//Has there been a change in zone since the unit powered down do you want a start up alarm
    		if ((saved_state[chan][i] != zone) && config.input[chan].alarm[i].startup_alarm)
        	{
        	    #ifdef DEBUG
        	    sprintf(buffer,"alarm on startup: %d:%d:%d: zone %d\r\n",chan, i,saved_state[chan][i],zone);
        	    print(buffer);
        	    #endif
        	    //Push events that have occured
        		if((saved_state[chan][i] == ZONE1 && zone == ZONE2) ||
					   (saved_state[chan][i] == ZONE0 && zone == ZONE2))
		        {
        		    //A rising event has occured, push this onto the queue
        		    e.type = event_CHXR + i;
        		    e.param = chan;
        		    queue_push(&q_event, &e);
        		    #ifdef DEBUG
        		    sprintf(buffer,"pushS%d,%d ",chan,event_CHXR + i);print(buffer);
                    //queue_print(&q_event);	//SJL - Debug
        		    #endif
        		}
        		else if((saved_state[chan][i] == ZONE2 && zone == ZONE0) ||
					        (saved_state[chan][i] == ZONE1 && zone == ZONE0))
            	//SJl - added  == ZONE2 and == ZONE1 missing from conditional statement
        		{
        			//A falling event has occured, push this onto the queue
        			e.type = event_CHXF + i;
        			e.param = chan;
        			queue_push(&q_event,&e);
        			#ifdef DEBUG
        			sprintf(buffer,"pushS%d,%d ",chan,event_CHXF + i);print(buffer);
        			#endif
                }
        		//Update the currentZone zone
        		input[chan].alarm[i].currentZone = zone;
        		input[chan].alarm[i].alarmZone = zone;
                saved_state[chan][i] = zone;

        	}
        	//No start up alarm wanted or zone the same.
        	else
        	{
        	     #ifdef DEBUG
        	     sprintf(buffer,"Zone: %d\r\nAlarm type: %d\r\n",zone,config.input[chan].alarm[i].type);
        	     print(buffer);
        	     #endif
        	    input[chan].alarm[i].currentZone = zone;
        	    input[chan].alarm[i].alarmZone = zone;
        	    //Set the messages to be displayed in diagnositics mode
        	    if((config.input[chan].alarm[i].type == ALARM_ABOVE) && (zone == ZONE2))
        	    {
        	        if (i == 0)
        	            config.input[chan].msg = sms_ALARM_A;
        	        else
        	            config.input[chan].msg = sms_ALARM_B;
        	    }
        	    else if ((config.input[chan].alarm[i].type == ALARM_ABOVE) && (zone != ZONE2))
        	    {
        	        if (i == 0)
        	            config.input[chan].msg = sms_RESET_A;
        	        else
        	            config.input[chan].msg = sms_RESET_B;
        	    }
        	    else if ((config.input[chan].alarm[i].type == ALARM_BELOW) && (zone == ZONE0))
        	    {
        	        if (i == 0)
        	            config.input[chan].msg = sms_ALARM_A;
        	        else
        	            config.input[chan].msg = sms_ALARM_B;
        	    }
        	    else if ((config.input[chan].alarm[i].type == ALARM_BELOW) && (zone != ZONE0))
        	    {
        	        if (i == 0)
        	            config.input[chan].msg = sms_RESET_A;
        	        else
        	            config.input[chan].msg = sms_RESET_B;
        	    }
        	}

        	#ifdef DEBUG
        	sprintf(buffer,"cZ=%d ",input[chan].alarm[i].currentZone);print(buffer);
        	#endif

		}
     //End loop input[chan].alarm[i]
	}
	#ifdef DEBUG
	sprintf(buffer,"\r\n");print(buffer);
	#endif

    return true;
}


unsigned int input_readADC(unsigned char adc_input)
{
    unsigned int returnVal;

    input_pause();


    delay_us(80);
    ADMUX=adc_input|ADC_VREF_TYPE;

    // Start the AD conversion
    ADCSRA|=0x40;
    // Wait for the AD conversion to complete
    while ((ADCSRA & 0x10)==0);
    ADCSRA|=0x10;

    returnVal = input[adc_input].ADCVal;
    input_resume();
    pulse_restart();
    return returnVal;
}


float input_getVal(unsigned char inputNum)
{
    float val;
    val = (config.input[inputNum].conv_grad * input[inputNum].ADCVal) + config.input[inputNum].conv_int;
    #ifdef DEBUG
    sprintf(buffer,"val=%f, grad=%f, int=%f\r\n",val, config.input[inputNum].conv_grad,config.input[inputNum].conv_int);print(buffer);
    #endif
    return val;
}


//Requires
void adc_calibrate_voltage()
{
    char i;
    for(i=0;i<5;i++)
    {
        delay_ms(1000);
        tickleRover();
    }
    sprintf(buffer,"Voltage:\r\n");print(buffer);
    for(i=0;i<MAX_INPUTS;i++)
    {
        adc_voltage_offset[i] = (unsigned char)(input[i].ADCVal - ADCVAL_5V);
        sprintf(buffer,"adc%d=%f;offset=%d\r\n",i+1,input[i].ADCVal,adc_voltage_offset[i]);
        print(buffer);
        input_setup(i);
    }

}

void adc_calibrate_current()
{
    char i;
    for(i=0;i<5;i++)
    {
        delay_ms(1000);
        tickleRover();
    }
    sprintf(buffer,"Current:\r\n");print(buffer);
    for(i=0;i<MAX_INPUTS;i++)
    {
        adc_current_offset[i] = (unsigned char)(input[i].ADCVal - ADCVAL_20mA);
        sprintf(buffer,"adc%d=%f;offset=%d\r\n",i+1,input[i].ADCVal,adc_current_offset[i]);
        print(buffer);
        input_setup(i);
    }
}

bool adc_calibration_set()
{
    return (*(adc_voltage_offset-3)==0x55&&*(adc_voltage_offset-2)==0xAA);
}


void pulse_restart()
{
    pulse_flag &=~ PULSE_FLAG_FILLED_7;
    pulse_flag |= PULSE_FLAG_RESTART_7;

    pulse_flag &=~ PULSE_FLAG_FILLED_8;
    pulse_flag |= PULSE_FLAG_RESTART_8;
}




