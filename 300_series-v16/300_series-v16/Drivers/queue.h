

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "global.h"   
#include "drivers/event.h"

#define MAX_QUEUE_LENGTH 16
#define MAX_SMS_QUEUE_LENGTH 16



typedef struct queue_struct
{
    Event event[MAX_QUEUE_LENGTH];	//8 chars x 16
    unsigned char read;              //Pointer to where the read is up to
    unsigned char write;             //Pointer where the write is up to
	unsigned char length;            //function returns the number of items in queue
	bool paused;
} queue_t;




bool queue_init (queue_t *q);
unsigned char queue_push(queue_t *q, Event *e);
unsigned char queue_pop(queue_t *q, Event *e);
void queue_print(queue_t *q); 
void queue_clear(queue_t *q);

void queue_pause(queue_t *q); 
void queue_resume(queue_t *q);


/* GLOBALS */
/**********************************************************************************************************/
/* PROTOTYPE HEADRERS */

/**********************************************************************************************************/
/* STRUCTS */
//Set up the modem and event queues
volatile extern queue_t q_modem;
volatile extern queue_t q_event;

/**********************************************************************************************************/
/* VARIABLES */

/**********************************************************************************************************/

#endif
