#include "drivers\adc.h"




void adc_init(void)  {
	// Analog Comparator initialization
	// Analog Comparator: Off
	// Analog Comparator Input Capture by Timer/Counter 1: Off
	ACSR=0x80;
	SFIOR=0x00;

	// ADC initialization
	// ADC Clock frequency: 125.000 kHz
	// ADC Voltage Reference: AVCC pin
	// ADC High Speed Mode: Off
	ADMUX=ADC_VREF_TYPE;
	ADCSRA=0x87;
	SFIOR&=0xEF;
	return;
}

// Read the AD conversion result
unsigned int adc_read(unsigned char adc_input)
{
    adc_on();
    delay_us(80);
    ADMUX=adc_input|ADC_VREF_TYPE;
    // Start the AD conversion
    ADCSRA|=0x40;
    // Wait for the AD conversion to complete
    while ((ADCSRA & 0x10)==0);
    ADCSRA|=0x10; 
    adc_off();
    return ADCW;
}