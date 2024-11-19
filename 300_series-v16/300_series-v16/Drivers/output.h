#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include "global.h"



#define OFF off
#define ON on
#define LAST_KNOWN 2

#if (HW_REV == V2)
    #define SET_OUT_4 PORTG
    #define SET_OUT_3 PORTD.7
    #define SET_OUT_2 PORTD.5
    #define SET_OUT_1 PORTD.6

    #define READ_OUT_4 PING
    #define READ_OUT_3 PIND.7
    #define READ_OUT_2 PIND.5
    #define READ_OUT_1 PIND.6
#elif (HW_REV == V6 || HW_REV == V5)
    #define OUTPUT_PORT PORTG
    #define OUTPUT1 0x01
    #define OUTPUT2 0x02
    #define OUTPUT3 0x04
    #define OUTPUT4 0x08
#endif



//Output Tasks
#define OUTPUT_NONE 0
#define OUTPUT_1_ON 1
#define OUTPUT_1_OFF 2
#define OUTPUT_2_ON 3
#define OUTPUT_2_OFF 4
#define OUTPUT_3_ON 5
#define OUTPUT_3_OFF 6
#define OUTPUT_4_ON 7
#define OUTPUT_4_OFF 8

#define output_set_task(task) output_current_task=task





//void output_task_handler(void);
void output_toSwitch(unsigned char param, unsigned char chan);
void output_switch(char n, char val);
//void output_1_set(unsigned char val);
//void output_2_set(unsigned char val);
//void output_3_set(unsigned char val);
//void output_4_set(unsigned char val);

extern unsigned char click_back_in[4];




#endif