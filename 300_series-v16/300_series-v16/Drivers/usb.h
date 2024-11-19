#ifndef _USB_H_
#define _USB_H_

#include "global.h"  



#define usb_port PORTA
#define usb_send_ready(void) ~PINC.5
#define usb_read_ready(void) ~PINC.6
#define usb_read_start(void) PORTC.3=0
#define usb_read_stop(void) PORTC.3=1
#define usb_write_start(void) PORTC.4=0
#define usb_write_stop(void) PORTC.4=1 
#define usb_read_mode(void) DDRA=1
#define usb_write_mode(void) DDRA=0 

#define FT245_RD PORTC.3
#define FT245_WR PORTC.4 
#define FT245_DATA_DDR DDRA 
#define FT245_DATA_OUT PORTA
#define FT245_DATA_IN PINA

void usb_init(void);
void usb_putc(unsigned char c);
unsigned char usb_getc(void);
void usb_send(char *str);
void usb_receive(char **str);



#endif