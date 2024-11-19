#include "drivers\usb.h"
#include <delay.h> 
#include <string.h> 

void usb_init(void) { 
	//Set up the DDR's for the bus control
	DDRC.5 = 0;
	DDRC.6 = 0;
	DDRC.3 = 1;
	DDRC.4 = 1; 
	FT245_RD = 1;  
	PORTC.4 = 0;    
	
	DDRC.7 = 0;
    
	FT245_DATA_DDR=0x00;
   	FT245_DATA_OUT=0xFF;
	

 
	return;
	

}
void usb_putc(unsigned char c) { 
	//Set the DDR registers 
	FT245_DATA_DDR=0xFF;
	//Wait for the FT245 to be ready to accept data
	while(!usb_send_ready()); 
	//Load the data onto the parallel bus
	FT245_DATA_OUT = c; 
	//Tell the FT245 the data is available 
	usb_write_start();  
	//Wait until the data has been read off the bus
	while(!usb_send_ready());
	//Deactivate the write pin
	usb_write_stop(); 
	//Set the bus into tristate mode.
   	FT245_DATA_OUT=0xFF;
	FT245_DATA_DDR=0x00;  
	
	return;
	
	

}
unsigned char usb_getc(void) {
	unsigned char data; 
	
	//Set the DDR registers 
	FT245_DATA_DDR=0x00; 
	//Wait for data to become available
   	while(!usb_read_ready())
   		continue; 
    //Activate the read process
	FT245_RD = 0;
	//Wait
	#asm("nop");
    #asm("nop");
    #asm("nop");
    #asm("nop");
    //Read the data onto the bus
	data = FT245_DATA_IN;  
	//Complete the read process
	FT245_RD = 1;
	//return the read character.
	return data;	
}
void usb_send(char *str) { 
	unsigned char i = 0;
	unsigned char length = 0;
	length=strlen(str);  

	for (i=0;i<length;i++) { 
        usb_putc(str[i]); 
    } 
	return;
}
void usb_receive(char **str) {
    *str++;
}

