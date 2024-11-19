#ifndef _ADC_H_
#define _ADC_H_
#include "global.h"

#define ADC_VREF_TYPE 0x40   
//Macros
#define adc_on(void) sbi(ADCSRA,0x80)
#define adc_off(void) cbi(ADCSRA,0x80)

void adc_init(void);
// Read the AD conversion result
unsigned int adc_read(unsigned char adc_input);

#endif

