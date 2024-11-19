#ifndef _INPUT_H_
#define _INPUT_H_

#include "global.h"

enum input_numbers {ONE=0,TWO,THREE,FOUR,FIVE,SIX,SEVEN,EIGHT};

//State of the input
#define DISABLE 0
#define ENABLE 1

//Type of input
#define DIGITAL 0
#define ANALOG_0_5V 1
#define ANALOG_4_20_mA 2
#define PULSE 3

#define ALARM_RESET 0
#define ALARM_SET 1

#define ALARM_A 0
#define ALARM_B 1
#define RESET_A 2
#define RESET_B 3

//Alarming condition
#define ALARM_ABOVE 1
#define ALARM_BELOW 2
#define ALARM_NONE 0
#define ALARM_BETWEEN 3
#define ALARM_OUTSIDE 4

#define ALARM_OPEN 1
#define ALARM_CLOSED 2

//Definitions for inputs configured to be digital
#define ALARM_WHEN_HIGH 1.0
#define ALARM_WHEN_LOW 0.0

//Definitions for the ADC values that correspond to 0V, 5V, 4mA and 20mA
#define CALIBRATED

#define BYTES_FROM_END_16 4080
#define BYTES_FROM_END_8  4088

#ifndef CALIBRATED

#define ADCVal_20mA     598
#define ADCVAL_4mA (ADCVAL_20mA/5)
#define ADCVal_0V       0
#define ADCVal_5V       776

#else

#if HW_REV == 2
    #define ADCVAL_5V 649
    #define ADCVAL_0V 0
    #define ADCVAL_20mA 502
    #define ADCVAL_4mA 113
#else
    #define ADCVAL_5V 598
    #define ADCVAL_0V 0
    #define ADCVAL_20mA 488
    #define ADCVAL_4mA (ADCVAL_20mA/5)
#endif

void adc_calibrate_voltage();
void adc_calibrate_current();

bool adc_calibration_set();

#endif


#define OUT_OF_BOUNDS 0xFFFF        //Max 16bit value that you will never get too
#if (HW_REV == V2)
    #define DIGITAL_HIGH (ADCVAL_5V*0.7)            //Just below logical high
    #define DIGITAL_LOW (ADCVAL_5V*0.2)             //Just above logical low
#elif (HW_REV == V6 || HW_REV == V5)
    #define DIGITAL_HIGH 1000            //Just below logical high
    #define DIGITAL_LOW 900             //Just above logical low
#endif





//Define the different alarming Zones
#define ZONE0 0
#define ZONE1 1
#define ZONE2 2

#define LATEST_SAMPLE 0x3

//Define the comparisoms for the zoneChange function
#define ALL_IN_ZONE0 0xAA
#define ALL_IN_ZONE1 0x55
#define ALL_IN_ZONE2 0x00

//ADC Setup
#define ADC_VREF_TYPE 0x40

//Turn off the timer and adc interrupts
#define input_pause() cbi(TIMSK,0x40); \    // Turn off Timer 2 interrupt
                      cbi(ADCSRA,0x08);//\    //turns off adc interrupt
                      //EIMSK=0x00;            //disable the external interrupts
//Turn back on the timer and adc interrupts
#define input_resume() sbi(TIMSK,0x40); \   // Turn on Timer 2 interrupt
	                   sbi(ADCSRA,0x08);// \  //turns on adc interrupt
	                 //  EIMSK=0xE0;          //reenable the external interrupts when we have a full
                                            //sample period rather than here

//#define input_pause() cbi(TIMSK,0x40);
//#define input_resume() sbi(TIMSK,0x40);
											
											
//Structure for each alarm A and alarm B of each input to be stored in
typedef struct alarmStruct
{
     unsigned char 	  buffer;			 	//Store latest zone buffer
	 unsigned char 	  currentZone;			//The currentZone zone the alarm is in
	 unsigned long   debounceTimer;		//Debounce timer
	 bool 	  debounceDone;			//Is the debounce complete
	 unsigned char    alarmZone;            //The zone that the alarm event is in
} alarm_t;

//Structure for storing the currentZone state of the sensor too make sure
//that it does not go out of bounds, ie become faulty.
typedef struct sensorStorage
{
    char 	  buffer;			 	//Store latest zone buffer
	unsigned char 	  currentZone;		    //The currentZone zone the sensor is in
	unsigned char     alarmZone;
} sensorE_t;

//Combination of the structures that are part of each input.
typedef struct input_alarmStruct
{
    float ADCVal;
    alarm_t alarm[INPUT_MAX_ALARMS];	//8bytes x 2
    sensorE_t sensor;					//3bytes
    unsigned char msg;
} input_alarm_t;

//SJL - CAVR2 - moved from global.c
extern volatile input_alarm_t input[];       //structure of the variables for the alarm

//Function Prototypes

//void input_task_handler(unsigned char current_input);
unsigned char input_setup(unsigned char num);

void input_init (void);
void input_adcStart (unsigned char channel);
unsigned char input_lookupZone (unsigned int adc, unsigned int threshold1, unsigned int threshold2);
bool input_zoneChange(unsigned char currentZone, unsigned char buffer, unsigned char alarmZone);
bool input_startup (unsigned char chan);
unsigned int input_readADC(unsigned char adc_input);
float input_getVal(unsigned char inputNum);	//SJL - CAVR2 - double not supported
void pulse_restart();

#define PULSE_FLAG_RESTART_7 0x01
#define PULSE_FLAG_FILLED_7 0x02
#define PULSE_FLAG_RESTART_8 0x04
#define PULSE_FLAG_FILLED_8 0x08

#if (HW_REV == V2)
    #define _power_led_on(void) cbi(PORTB,0x80)
    #define _power_led_off(void) sbi(PORTB, 0x80)
    #define _power_led_toggle(void) tbi(PORTB,0x80)
    #define _active_led_on(void) cbi(PORTG, 0x02)
    #define _active_led_off(void) sbi(PORTG, 0x02)
    #define _active_led_toggle(void) tbi(PORTG, 0x02)
    #define _status_led_on(void) cbi(PORTC, 0x04)
    #define _status_led_off(void) sbi(PORTC, 0x04)
    #define _status_led_toggle(void) tbi(PORTC, 0x04)
#elif (HW_REV == V5 || HW_REV == V6)
    #define _power_led_on(void) sbi(PORTB,0x20)
    #define _power_led_off(void) cbi(PORTB, 0x20)
    #define _power_led_toggle(void) tbi(PORTB,0x20)
    #define _active_led_on(void) sbi(PORTB, 0x40)
    #define _active_led_off(void) cbi(PORTB, 0x40)
    #define _active_led_toggle(void) tbi(PORTB,0x40)
    #define _status_led_on(void) sbi(PORTB, 0x80)
    #define _status_led_off(void) cbi(PORTB, 0x80)
    #define _status_led_toggle(void) tbi(PORTB,0x80)
#endif

extern eeprom unsigned char* adc_voltage_offset;
extern eeprom unsigned char* adc_current_offset;
extern volatile unsigned char channel;				 		//The currentZone channel you are reading	//SJL - volatile added from global.c def
extern volatile unsigned char alarmCount;					//SJL - volatile added from global.c def
//extern unsigned char power_led_state;
extern float input_min[];
extern float input_max[];
extern unsigned long pulse_aggregate[];
extern unsigned int current_duty_high[];
extern unsigned int current_duty_count[];
extern char counter;
extern eeprom unsigned char saved_state[MAX_INPUTS][2];


//extern unsigned int pulse_count;
extern bit pulse6_timer_valid;
extern bit pulse7_timer_valid;
extern bit pulse_instant_pause;


interrupt [TIM1_COMPA] void timer1_compa_isr(void);
#if PULSE_COUNTING_AVAILABLE
interrupt [EXT_INT6] void ext_int6_isr(void);
interrupt [EXT_INT7] void ext_int7_isr(void);
#endif
interrupt [TIM1_COMPB] void timer1_compb_isr(void);
interrupt [TIM1_OVF] void timer1_ovf_isr(void);
interrupt [TIM2_OVF] void timer2_ovf_isr(void);
interrupt [ADC_INT] void adc_isr(void);


#endif



