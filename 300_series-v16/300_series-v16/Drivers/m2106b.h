#ifndef _M2106_H_
#define _M2106_H_

#include "global.h"

bool readModem(void); 
int modem_init(unsigned char modem_type); 
unsigned char modem_send_sms(void);
unsigned char modem_handle_input(void); 
void modem_tm_task_handler(void);

#endif