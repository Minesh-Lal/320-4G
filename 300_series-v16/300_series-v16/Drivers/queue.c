/**
 * queue.c

 Implementation for the FIFO queues.

 **/
#include <stdio.h>              //SJL - CAVR2 - added

#include "global.h"
#include "drivers\queue.h"
#include "drivers\error.h"      //SJL - CAVR2 - added
#include "drivers\uart.h"       //SJL - CAVR2 - added

//SJL - CAVR2 definition for linking
volatile queue_t q_event;
volatile queue_t q_modem;

//Set up the queue when they are first used.
bool queue_init (queue_t *q)
{
    unsigned char i;

    //init all the elements of the queue
    for(i=0; i<MAX_QUEUE_LENGTH; i++)
    {
        q->event[i].type=0;
        q->event[i].param=0;
        q->event[i].year=0;
        q->event[i].month=0;
        q->event[i].day=0;
        q->event[i].hour=0;
        q->event[i].minute=0;
        q->event[i].second=0;
    }
    q->write=0;
    q->read=0;
    q->length=0;
    q->paused=false;
    return true;
}

//Add an event to a queue
bool queue_push(queue_t *q, Event *e)
{
    char h,m,s;

    #ifdef _SMS_DEBUG_
    sprintf(buffer,"** pushing event - %d **\r\n",e->type);print(buffer);	//SJL - Debug
    #endif

    if (q->length >= MAX_QUEUE_LENGTH)
    {
        if(q == &q_event)
        {
    		if (error.q_event_ovf != 0xFFFF)
                error.q_event_ovf++;
            system_reset();      //won't return
        }
        else
        {
    		if (error.q_modem_ovf != 0xFFFF)
                error.q_modem_ovf++;
            return false;           //SMS overflow is less drastic
        }
	}
	q->event[q->write].type = e->type;
	q->event[q->write].param = e->param;
	event_populate_time(&q->event[q->write]);
//    rtc_get_date(&h,&m,&s);
//	q->event[q->write].year = h;
//	q->event[q->write].month = m;
//	q->event[q->write].day = s;
//	rtc_get_time(&h,&m,&s);
//	q->event[q->write].hour = h;
//	q->event[q->write].minute = m;
//	q->event[q->write].second = s;

    if (++(q->write) >= MAX_QUEUE_LENGTH)
    {
        q->write = 0;
    }
    //Increment the length of the queue
    q->length++;

    tickleRover();
    return true;
}



//Pop off an event from the queue.
bool queue_pop(queue_t *q, Event *e)
{
    if (q->paused)
    {
        //The queue is paused, do not return anything
        return false;
    }
    else if (!q->length)
    {
        return false;
    }
    else
    {
        e->type = q->event[q->read].type;
        e->param = q->event[q->read].param;
        e->year = q->event[q->read].year;
        e->month = q->event[q->read].month;
        e->day = q->event[q->read].day;
        e->hour = q->event[q->read].hour;
        e->minute = q->event[q->read].minute;
        e->second = q->event[q->read].second;
        if (++q->read >= MAX_QUEUE_LENGTH)
        {
            q->read = 0;
        }
        q->length--;
    }
	return true;
}

//Debug only.  Prints the current values in the queue.
void queue_print(queue_t *q)
{
    unsigned char i;

		sprintf(buffer,"QUEUE PRINTOUT\r\nindex\t|EVENT\t|PARAM\r\n==============================\r\n");print(buffer);

		i=q->read;
		while (i!=q->write)
		{
			tickleRover();
			sprintf(buffer,"%d\t|%d\t|%d\r\n",i,q->event[i].type,q->event[i].param);print(buffer);
			if(++i >= MAX_QUEUE_LENGTH)
			    i = 0;

		}

		return;
}

//Reset the queue
void queue_clear(queue_t *q)
{
    q->write=0;
    q->read=0;
    q->length=0;
    q->paused=false;

    return;
}

//Stop the user from poping events off the queue.
void queue_pause(queue_t *q)
{
    //sprintf(buffer,"Paused queue 0x%04x\r\n",q);print(buffer);
    q->paused = true;
    return;
}

//Resume the queue to normal operation
void queue_resume(queue_t *q)
{
    //sprintf(buffer,"Resume queue 0x%04x\r\n",q);print(buffer);
    q->paused = false;
    return;
}