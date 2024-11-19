#include <stdio.h>				//SJl - CAVR2 - added to access sprintf for debug functions

#include "drivers\output.h"
#include "drivers\sms.h"
#include "drivers\config.h"
#include "drivers\input.h"
#include "drivers\event.h"
#include "drivers\debug.h"      //SJl - CAVR2 - added to access debug functions
#include "drivers\queue.h"      //SJl - CAVR2 - added to access queue functions
#include "drivers\uart.h"      //SJl - CAVR2 - added to access print for debug functions

#define OUTPUT_ALARM_A 0x00
#define OUTPUT_ALARM_B 0x20
#define OUTPUT_RESET_A 0x80
#define OUTPUT_RESET_B 0xA0

unsigned char click_back_in[4];

void output_toSwitch(unsigned char param, unsigned char chan)
{
    unsigned char action=0;
    Event e;
    #ifdef DEBUG
    sprintf(buffer, "Output switch in=%d, alarm=%d, ", param, chan-1);print(buffer);
    #endif

    switch(chan)
    {
        case sms_ALARM_A :
            if(config.input[param].alarm[0].alarmAction.outswitch == 0)
            {
                DEBUG_printStr("No output was selected 1\r\n");
                return;
            }
            break;
        case sms_ALARM_B :
            if(config.input[param].alarm[1].alarmAction.outswitch == 0)
            {
                DEBUG_printStr("No output was selected 2\r\n");
                return;
            }
            break;
        case sms_RESET_A :
            if(config.input[param].alarm[0].resetAction.outswitch == 0)
            {
                DEBUG_printStr("No output was selected 3\r\n");
                return;
            }
            break;
        case sms_RESET_B :
            if(config.input[param].alarm[1].resetAction.outswitch == 0)
            {
                DEBUG_printStr("No output was selected 4\r\n");
                return;
            }
            break;
    }

    action = 0;
    //SMS Input Alarm A
    action = param & INPUT_NUMBER_BITS;

    switch (chan)
    {
        case sms_ALARM_A :
            action |= OUTPUT_ALARM_A;
            action |= (((config.input[param].alarm[ALARM_A].alarmAction.outswitch-1) << 3) & OUTPUT_NUMBER_BITS);

            #ifdef DEBUG
            sprintf(buffer,"action=0x%x\r\n",action);print(buffer);
            #endif
            if (config.input[param].alarm[ALARM_A].alarmAction.outaction == on)
            {
                if (config.input[param].alarm[ALARM_A].alarmAction.me)
                {
                     e.type = event_OUT_ON;
                     e.param = config.input[param].alarm[ALARM_A].alarmAction.outswitch-1;
                    queue_push(&q_event, &e);
                }
                if (config.input[param].alarm[ALARM_A].alarmAction.contact_list != 0x00)
                {
                    e.type = sms_REMOTE_OUT_ON;
                    e.param = action;
                    queue_push(&q_modem, &e);
                }
            }
            else
            {
                if (config.input[param].alarm[ALARM_A].alarmAction.me)
                {
					#ifdef _SMS_DEBUG_
					sprintf(buffer,"7d\r\n");print1(buffer);
					#endif
                    e.type = event_OUT_OFF;
                    e.param = config.input[param].alarm[ALARM_A].alarmAction.outswitch-1;
                    queue_push(&q_event, &e);
                }
                if (config.input[param].alarm[ALARM_A].alarmAction.contact_list != 0x00)
                {
                    e.type = sms_REMOTE_OUT_OFF;
                    e.param = action;
                    queue_push(&q_modem, &e);
                }
            }
            break;

        //SMS Input Alarm B
        case sms_ALARM_B :
            action |= OUTPUT_ALARM_B;
            action |= (((config.input[param].alarm[ALARM_B].alarmAction.outswitch-1) << 3) & OUTPUT_NUMBER_BITS);
            #ifdef DEBUG
            sprintf(buffer,"action=0x%x\r\n",action);print(buffer);
            #endif
            if (config.input[param].alarm[ALARM_B].alarmAction.outaction == on)
            {
                if (config.input[param].alarm[ALARM_B].alarmAction.me)
                {
                    e.type = event_OUT_ON;
                    e.param = config.input[param].alarm[ALARM_B].alarmAction.outswitch-1;
                    queue_push(&q_event, &e);
                }
                if (config.input[param].alarm[ALARM_B].alarmAction.contact_list != 0x00)
                {
                    e.type = sms_REMOTE_OUT_ON;
                    e.param = action;
                    queue_push(&q_modem, &e);
                }
            }
            else
            {
                if (config.input[param].alarm[ALARM_B].alarmAction.me)
                {
					#ifdef _SMS_DEBUG_
					sprintf(buffer,"6d\r\n");print1(buffer);
					#endif
                    e.type = event_OUT_OFF;
                    e.param = config.input[param].alarm[ALARM_B].alarmAction.outswitch-1;
                    queue_push(&q_event, &e);
                }
                if (config.input[param].alarm[ALARM_B].alarmAction.contact_list != 0x00)
                {
                    e.type = sms_REMOTE_OUT_OFF;
                    e.param = action;
                    queue_push(&q_modem, &e);
                }
            }
            break;

        //SMS Input Reset A
        case sms_RESET_A :
           action |= OUTPUT_RESET_A;
           action |= (((config.input[param].alarm[ALARM_A].resetAction.outswitch-1) << 3) & OUTPUT_NUMBER_BITS);
           #ifdef DEBUG
           sprintf(buffer,"action=0x%x\r\n",action);print(buffer);
           #endif
           if (config.input[param].alarm[ALARM_A].resetAction.outaction == on)
            {
                if (config.input[param].alarm[ALARM_A].resetAction.me)
                {
                    e.type = event_OUT_ON;
                    e.param = config.input[param].alarm[ALARM_A].resetAction.outswitch-1;
                    queue_push(&q_event, &e);
                }
                if (config.input[param].alarm[ALARM_A].resetAction.contact_list != 0x00)
                {
                    e.type = sms_REMOTE_OUT_ON;
                    e.param = action;
                    queue_push(&q_modem, &e);
                }
            }
            else
            {
                if (config.input[param].alarm[ALARM_A].resetAction.me)
                {
					#ifdef _SMS_DEBUG_
					sprintf(buffer,"5d\r\n");print1(buffer);
					#endif
                    e.type = event_OUT_OFF;
                    e.param = config.input[param].alarm[ALARM_A].resetAction.outswitch-1;
                    queue_push(&q_event, &e);
                }
                if (config.input[param].alarm[ALARM_A].resetAction.contact_list != 0x00)
                {
                    e.type = sms_REMOTE_OUT_OFF;
                    e.param = action;
                    queue_push(&q_modem, &e);
                }
            }
            break;

        //SMS Input Reset B
        case sms_RESET_B :
            action |= OUTPUT_RESET_B;
            action |= (((config.input[param].alarm[ALARM_B].alarmAction.outswitch-1) << 3) & OUTPUT_NUMBER_BITS);
            #ifdef DEBUG
            sprintf(buffer,"action=0x%x\r\n",action);print(buffer);
            #endif
            if (config.input[param].alarm[ALARM_B].resetAction.outaction == on)
            {
                if (config.input[param].alarm[ALARM_B].resetAction.me)
                {
                    e.type = event_OUT_ON;
                    e.param = config.input[param].alarm[ALARM_B].resetAction.outswitch-1;
                    queue_push(&q_event, &e);
                }
                if (config.input[param].alarm[ALARM_B].resetAction.contact_list != 0x00)
                {
                    e.type = sms_REMOTE_OUT_ON;
                    e.param = action;
                    queue_push(&q_modem, &e);
                }
            }
            else
            {
                if (config.input[param].alarm[ALARM_B].resetAction.me)
                {
					#ifdef _SMS_DEBUG_
					sprintf(buffer,"4d\r\n");print1(buffer);
					#endif
                    e.type = event_OUT_OFF;
                    e.param = config.input[param].alarm[ALARM_B].resetAction.outswitch-1;
                    queue_push(&q_event, &e);
                }
                if (config.input[param].alarm[ALARM_B].resetAction.contact_list != 0x00)
                {
                    e.type = sms_REMOTE_OUT_OFF;
                    e.param = action;
                    queue_push(&q_modem, &e);
                }
            }
            break;
    }
    return;
}

void output_switch(char n, char val)
{
    #if HW_REV == V2
        switch(n)
        {
            case 0:
                SET_OUT_1=val;
                break;
            case 1:
                SET_OUT_2=val;
                break;
            case 2:
                SET_OUT_3=val;
                break;
            case 3:
                val?sbi(SET_OUT_4,0x01):cbi(SET_OUT_4,0x01);
                break;
            default:
                break;
        }
    #elif HW_REV == V6 || HW_REV == V5
        if(val==ON)
        {
            switch(n)
            {
                case 0:
                    sbi(OUTPUT_PORT,OUTPUT1);
                    break;
                case 1:
                    sbi(OUTPUT_PORT,OUTPUT2);
                    break;
                case 2:
                    sbi(OUTPUT_PORT,OUTPUT3);
                    break;
                case 3:
                    sbi(OUTPUT_PORT,OUTPUT4);
                    break;
                default:
                    break;
            }
        }
        else
        {
            switch(n)
            {
                case 0:
                    cbi(OUTPUT_PORT,OUTPUT1);
                    break;
                case 1:
                    cbi(OUTPUT_PORT,OUTPUT2);
                    break;
                case 2:
                    cbi(OUTPUT_PORT,OUTPUT3);
                    break;
                case 3:
                    cbi(OUTPUT_PORT,OUTPUT4);
                    break;
                default:
                    break;
            }
        }
    #endif
  //  sprintf(buffer,"turning %p, def state %p, click back %d\r\n",val==ON?"ON":"OFF",
  //                                     config.output[n].config.default_state==OFF?"OFF":config.output[n].config.default_state==ON?"ON":"OTHER",
  //                                     click_back_in[n]);
  //  print(buffer);
    if(val == ON && config.output[n].config.default_state == OFF && click_back_in[n] == 0)
        click_back_in[n] = config.output[n].config.momentaryLength;
    else if(val == OFF && config.output[n].config.default_state == ON && click_back_in[n] == 0)
        click_back_in[n] = config.output[n].config.momentaryLength;
    else
    {
   //     DEBUG_printStr("no outputs getting switched\r\n");
    }
}

void output_1_set(unsigned char val)
{
    #if (HW_REV == V2)
        SET_OUT_1=val;
    #elif (HW_REV == V6 || HW_REV == V5)
        if(val==ON)
            sbi(OUTPUT_PORT,OUTPUT1);
        else
            cbi(OUTPUT_PORT,OUTPUT1);
    #endif
}


void output_2_set(unsigned char val)
{
    #if (HW_REV == V2)
        SET_OUT_2=val;
    #elif (HW_REV == V6 || HW_REV == V5)
        if(val==ON)
            sbi(OUTPUT_PORT,OUTPUT2);
        else
            cbi(OUTPUT_PORT,OUTPUT2);
    #endif
    #ifdef DIRTY_MOMENTARY_OUTPUT_HACK
    click_back_in[1] = 8;
    #endif
}

void output_3_set(unsigned char val)
{
    #if (HW_REV == V2)
        SET_OUT_3=val;
    #elif (HW_REV == V6 || HW_REV == V5)
        if(val==ON)
            sbi(OUTPUT_PORT,OUTPUT3);
        else
            cbi(OUTPUT_PORT,OUTPUT3);
    #endif
    #ifdef DIRTY_MOMENTARY_OUTPUT_HACK
    click_back_in[2] = 8;
    #endif
}

void output_4_set(unsigned char val)
{
    #if (HW_REV == V2)
        if(val)
            sbi(SET_OUT_4,0x01);
        else
            cbi(SET_OUT_4,0x01);
    #elif (HW_REV == V6 || HW_REV == V5)
        if(val==ON)
            sbi(OUTPUT_PORT,OUTPUT4);
        else
            cbi(OUTPUT_PORT,OUTPUT4);
    #endif
    #ifdef DIRTY_MOMENTARY_OUTPUT_HACK
    click_back_in[3] = 8;
    #endif
}