#include "global.h"
#include <string.h> 
#include <delay.h>


#define modem_turnOn(void) PORTE.2=1
#define modem_turnOff(void) PORTE.2=0
#define modem_reset(void) PORTE.3=1; delay_ms(5); PORTE.3=0
#define FIVE_SECOND_DELAY 305 

#define MODEM_READ_MODEM 1
#define MODEM_READ_UART 2 
#define MAX_ERROR_RETRY 5

static unsigned char modem_tm_current_task=MODEM_READ_MODEM; 

#define modem_tm_set_task(task) modem_tm_current_task=task

static unsigned char no_carrier_count=0;
static unsigned char uart_plus_count=0;
static const char NO_CARRIER[11] = {"NO CARRIER"};
static bool start=false;

extern char modemStr[];
eeprom unsigned int modemPinNumber = 1234;
extern char buffer[];

bool readModem() {
	char input;
	unsigned char endLoop = false;   
	unsigned char strPos = 0;
	
	retry:
	
	endLoop = false;
	strPos = 0;
	do {
	   input = getchar();
	   switch (input) {
	       case '\0':
		       break;                           //nothing
		   case '\n':  
		   	   break;                           // we do not need this one
		   case '\r' : 
		   	   if (strPos > 0) {		//if we have received something
			   	   endLoop = true;  
			   	   modemStr[strPos] = '\0';
			   } 
			   break;
		   default:
		   	   modemStr[strPos++] = input;			// build string    
	    }
	}
    while (!endLoop);
	
	
    return true;
} 




int modem_init(unsigned char modem_type) {
    unsigned int timer;
    unsigned char error_count;
    
    start=false;
    
    uart0_init(modem_baud); 
    
    
    modem_turnOff();
	//Turn on the modem  
	//RESET and boot must be low
	DDRE.2=1;DDRE.3=1;DDRE.4=1;
	PORTE.3 = 0; 
	PORTE.4 = 0;
	delay_ms(500);
	modem_turnOn();
	#ifdef _MODEM_DEBUG_
    sprintf(buffer, ">MODEM: Modem Init()\r\n");
    print(buffer);
    #endif
 	if (!modem_throughmode) {
   	delay_ms(1000); 
   	
   	if (modem_type == CDMA_Q2438F_TAU || modem_type == CDMA_Q2438F_TNZ)
   	{
   	    #ifdef _MODEM_DEBUG_
   	    sprintf(buffer, ">MODEM: Waiting for +WIND: 8\r\n");
   	    print(buffer);
   	    #endif
   	    //Wait for the modem to output +WIND, this tells us the modem
   	    //is ready to start the init sequence.
   	    readModem();  
   	    
   	    printf("AT\r\n");
   	    while (!readModem())
   	        printf("AT\r\n");
   	       
   	
   	
   	}
   	else if (modem_type == GSM_Q2403B) 
   	{
   	
    RESET:
       	
    	timer=debounce_counter + FIVE_SECOND_DELAY;
    	
      	//Wake up the modem
       	printf("AT\r\n");
       	//wait for a character to become available.  If there has been no character after 
       	//5seconds, reset the modem.
       	while (!modem_read_ready) { 
       	    if (debounce_counter >= timer) {
       	        #ifdef _MODEM_DEBUG_
       	        sprintf(buffer, ">MODEM: Reset modem\r\n");
       	        print(buffer);
       	        #endif
       	       // modem_reset();
       	        //delay_ms(500);
       	        goto RESET;
       	        break;
       	    }
       	}
    
   	
   	    while (!readModem())
   	        readModem();
    }
   	start=true;
   	//TODO catch if there is a baud rate problem...
   	if (strstrf(modemStr,"OK") == 0) {
   	    #ifdef _MODEM_DEBUG_
   		sprintf(buffer, ">AT RESPONSE: (%s)\r\n",modemStr);
		print(buffer); 
		#endif
		if (strcmpf(modemStr,"AT") == 0) { 
			while (!readModem())
   	            readModem(); 
    		if (strstrf(modemStr,"OK") == 0) {
        		#ifdef _MODEM_DEBUG_
        			sprintf(buffer, ">MODEM: Need to autobaud....(%s)\r\n", modemStr);
        			print(buffer);
        		#endif  
      		}
      		else {
      		    #ifdef _MODEM_DEBUG_ 
        		sprintf(buffer, ">2nd RESPONSE: (%s)\r\n",modemStr);
        		print(buffer);
        		#endif
        	}
    	}
  		else {
    		#ifdef _MODEM_DEBUG_
    			sprintf(buffer, ">MODEM: Need to autobaud....(%s)\r\n", modemStr);
    			print(buffer);
    		#endif
    	}
	}  
	else {
	    #ifdef _MODEM_DEBUG_ 
		sprintf(buffer, ">RESPONSE: (%s)\r\n",modemStr);
		print(buffer);
		#endif
	}
    delay_ms(250);
    //Wake up the modem
   	printf("AT\r\n");
   	//while (!readModem())
   	    readModem();
   	if (strstrf(modemStr,"OK") == 0) {  
   	    #ifdef _MODEM_DEBUG_
   		sprintf(buffer, ">AT RESPONSE: (%s)\r\n",modemStr);
		print(buffer); 
		#endif
		if (strcmpf(modemStr,"AT") == 0) { 
			//while (!readModem())
   	            readModem(); 
    		if (strstrf(modemStr,"OK") == 0) {
        		#ifdef _MODEM_DEBUG_
        			sprintf(buffer, ">MODEM: Need to autobaud....(%s)\r\n", modemStr);
        			print(buffer);
        		#endif  
      		}
      		else {  
      		    #ifdef _MODEM_DEBUG_
        		sprintf(buffer, ">2nd RESPONSE: %s\r\n",modemStr);
        		print(buffer); 
        		#endif
        	}
    	}
  		else {
    		#ifdef _MODEM_DEBUG_
    			sprintf(buffer, ">MODEM: Need to autobaud....(%s)\r\n", modemStr);
    			print(buffer);
    		#endif
    	}
	}  
	else { 
	    #ifdef _MODEM_DEBUG_
		sprintf(buffer, ">RESPONSE: (%s)\r\n",modemStr);
		print(buffer);  
	    #endif
	}
	
	delay_ms(500);
	
	
	
	//Enable full modem functionality
	printf("AT+CFUN=1\r\n");     
	//while (!readModem())
   	    readModem();
	#ifdef _MODEM_DEBUG_  
	sprintf(buffer, ">MODEM: AT+CFUN=1 (%s)\r\n", modemStr);
	print(buffer);
	#endif
	//while (!readModem())
   	    readModem();
	if (strstrf(modemStr,"OK") == '\0') { 
		//The command has not been successfully completed.  Try again
		printf("AT+CFUN=1\r\n");
		//while (!readModem())
   	        readModem();
        //while (!readModem())
   	        readModem();
		if (strstrf(modemStr,"OK") == '\0') {
			//might be able to add a auto baud fix here...?
			#ifdef _MODEM_DEBUG_
				sprintf(buffer, ">MODEM: is not fully functional\r\n");
				print(buffer);
			#endif
			return ERROR;
		}
	}
	#ifdef _MODEM_DEBUG_
	else { 
		sprintf(buffer, ">+CFUN RESPONSE: (%s)\r\n",modemStr);
		print(buffer);
	}
	#endif
	
	delay_ms(500);
	
//REDO_ATE0:
	//Turn off the modem echo
	printf("ATE0\r\n");
	//while (!readModem())
   	    readModem();
	#ifdef _MODEM_DEBUG_
	sprintf(buffer, ">MODEM: ATE0 response (%s)\r\n", modemStr);
	print(buffer);    
    #endif
	if (strstrf(modemStr,"ERROR") == 0) {
    	//while (!readModem())
   	        readModem();
    	#ifdef _MODEM_DEBUG_
    	sprintf(buffer, ">MODEM: ATE0 response complete (%s)\r\n", modemStr);
    	print(buffer);  
    	#endif   
 	}
 	
 	delay_ms(1000); 
 	
// REDO_ATF:
 	 
 	
 	//Give the modem a factory reset.
 	printf("AT&F\r\n");
	//while (!readModem())
   	    readModem();
   	#ifdef _MODEM_DEBUG_    
	sprintf(buffer, ">MODEM: AT&F response (%s)\r\n",modemStr);
	print(buffer);
	#endif
	if (strstrf(modemStr,"OK") == 0) { 
		//The command has not been successfully completed.  Try again
		delay_ms(500); 
		
		/*if (strstrf(modemStr, "+WIND") != 0) 
		{ 
		    #ifdef _MODEM_DEBUG_
		    sprintf(buffer, ">MODEM: caught +WIND (%s)\r\n",modemStr);
			print(buffer);
			#endif 
			goto REDO_ATF;
		}
		else if (strstrf(modemStr, "AT&F") != 0) 
		{ 
		    goto REDO_ATE0;
		}
		else 
		{ 
		    while (!modem_read_ready);
		    if (strstrf(modemStr, "+WIND") != 0) 
    		{ 
    		    #ifdef _MODEM_DEBUG_
    		    sprintf(buffer, ">MODEM: waited and caught +WIND (%s)\r\n",modemStr);
    			print(buffer);
    			#endif 
    			goto REDO_ATF;
    		}
    		else { 
    		    #ifdef _MODEM_DEBUG_
    		    sprintf(buffer, ">MODEM: response? (%s)\r\n",modemStr);
    			print(buffer);
    			#endif 
    		}
        }
		  */  
		printf("AT&F\r\n");
		//while (!readModem())
   	        readModem();
		if (strstrf(modemStr,"OK") == 0) {
			//might be able to add a auto baud fix here...?
			#ifdef _MODEM_DEBUG_
				sprintf(buffer, ">MODEM: could not factory reset the modem (%s)\r\n",modemStr);
				print(buffer);
			#endif
			return ERROR;
		}
	} 
	#ifdef _MODEM_DEBUG_
	else { 
		sprintf(buffer, ">AT&F RESPONSE: (%s)\r\n",modemStr);
		print(buffer);
	}
	#endif
 	
	delay_ms(500);
	
	//Turn off the modem echo
	printf("ATE0\r\n");
	//while (!readModem())
   	    readModem();
	#ifdef _MODEM_DEBUG_
	sprintf(buffer, ">MODEM: ATE0 response (%s)\r\n", modemStr);
	print(buffer);    
    #endif
	if (strstrf(modemStr,"ERROR") == 0) {
    	//while (!readModem())
   	        readModem();
    	#ifdef _MODEM_DEBUG_
    	sprintf(buffer, ">MODEM: ATE0 response complete (%s)\r\n", modemStr);
    	print(buffer);  
    	#endif   
 	}
 	
 	delay_ms(1000); 
	
 	//Enable error reports
 	printf("AT+CMEE=1\r\n");
	readModem();
	if (strstrf(modemStr,"OK") == 0) { 
		//The command has not been successfully completed.  Try again
		printf("AT+CMEE=1\r\n");
		readModem();
		if (strstrf(modemStr,"OK") == 0) {
			//might be able to add a auto baud fix here...?
			#ifdef _MODEM_DEBUG_
				sprintf(buffer, ">MODEM: could not enable error reports (%s)\r\n",modemStr);
				print(buffer);
			#endif
			return ERROR;
		}
	} 
	#ifdef _MODEM_DEBUG_
	else { 
		sprintf(buffer, ">+CMEE RESPONSE: (%s)\r\n",modemStr);
		print(buffer);
	}
	#endif
	
	
	//Enable roaming
 	printf("AT+WRMP=2\r\n");
	readModem();
	if (strstrf(modemStr,"OK") == 0) { 
		//The command has not been successfully completed.  Try again
		printf("AT+WRMP=2\r\n");
		readModem();
		if (strstrf(modemStr,"OK") == 0) {
			//might be able to add a auto baud fix here...?
			#ifdef _MODEM_DEBUG_
				sprintf(buffer, ">MODEM: could not enable roaming (%s)\r\n",modemStr);
				print(buffer);
			#endif
			return ERROR;
		}
	} 
	#ifdef _MODEM_DEBUG_
	else { 
		sprintf(buffer, ">+WRMP RESPONSE: (%s)\r\n",modemStr);
		print(buffer);
	}
	#endif

	
	delay_ms(500);
	
    error_count=0;
    
    //Set the mode preference to be automatic
    //TODO catch SIM card error
 	printf("AT+COPS=0\r\n");  
	readModem();
	if (strstrf(modemStr,"OK") == '\0') { 
		//The command has not been successfully completed.  Try again
		#ifdef _MODEM_DEBUG_
		sprintf(buffer, ">MODEM: ERROR +COPS (%s)\r\n", modemStr);
		print(buffer); 
		#endif
		if ((strncmpf(modemStr,"+CME ERROR: 515", strlen(modemStr)) == 0) || 
		    (strncmpf(modemStr,"+CMS ERROR: 515", strlen(modemStr)) == 0)) {    
		    while(1) {
    			//might be able to add a auto baud fix here...?
    			delay_ms(500);
    			printf("AT+COPS=0\r\n");
    			
    			readModem();
    			if ((strncmpf(modemStr,"+CME ERROR: 515", strlen(modemStr)) == 0) || 
    			    (strncmpf(modemStr,"+CMS ERROR: 515", strlen(modemStr)) == 0)) {
        			#ifdef _MODEM_DEBUG_
        			sprintf(buffer, ">MODEM: busy AT+COPS=1 (%s)\r\n", modemStr);
        			print(buffer);
        			#endif
        			if(++error_count > MAX_ERROR_RETRY) { 
        			    return ERROR;
        			}
        		}
        		
            	else if (strncmpf(modemStr,"OK",2) == 0) { 
            	    #ifdef _MODEM_DEBUG_
            		sprintf(buffer, ">+COPS RESPONSE: %s\r\n",modemStr);
            		print(buffer);
            		#endif 
            		break;
            	}
            	else { 
            	    #ifdef _MODEM_DEBUG_
            		sprintf(buffer, ">MODEM: Fatal Error - %s\r\n",modemStr);
            		print(buffer);
            		#endif
            	    return ERROR;
            	}	
            }
        }
        else { 
            #ifdef _MODEM_DEBUG_
            sprintf(buffer, ">MODEM: Fatal Error - %s\r\n",modemStr);
            print(buffer);
            #endif
            return ERROR;
        }
	}
	#ifdef _MODEM_DEBUG_
	else { 
		sprintf(buffer, ">+COPS RESPONSE: %s\r\n",modemStr);
		print(buffer);
	}
	#endif
	
	delay_ms(1500);
	
	if (modem_type == GSM_Q2403B)
	{
      	//Report regirstration
      	printf("AT+CREG=1\r\n");   
    	readModem();
    	if (strstrf(modemStr,"OK") == 0) { 
            #ifdef _MODEM_DEBUG_
    		sprintf(buffer, ">+CREG RESPONSE 1 (%s)\r\n",modemStr);
    		print(buffer); 
    		#endif
    		//The command has not been successfully completed.  Try again
    		printf("AT+CREG=1\r\n");
    		readModem();
    		if (strstrf(modemStr,"OK") == 0) {
    			//might be able to add a auto baud fix here...?
    			#ifdef _MODEM_DEBUG_
    				sprintf(buffer, ">MODEM: not registered on network (%s)\r\n",modemStr);
    				print(buffer);
    			#endif
    			return ERROR;
    		}
    	}
    	#ifdef _MODEM_DEBUG_
    	else { 
    		sprintf(buffer, ">+CREG RESPONSE: (%s)\r\n",modemStr);
    		print(buffer);
    	}
    	#endif  
    	readModem();
    	#ifdef _MODEM_DEBUG_
    	sprintf(buffer, ">+CREG 2nd RESPONSE: (%s)\r\n",modemStr);
    	print(buffer);
    	#endif
    }  
    else if (modem_type == CDMA_Q2438F_TAU || modem_type == CDMA_Q2438F_TNZ)
	{
      	//Report regirstration
      	printf("AT+CREG=1\r\n");   
    	readModem();
    	if (strstrf(modemStr,"+CREG") == 0) { 
            #ifdef _MODEM_DEBUG_
    		sprintf(buffer, ">+CREG RESPONSE 1 (%s)\r\n",modemStr);
    		print(buffer); 
    		#endif
    		//The command has not been successfully completed.  Try again
    		printf("AT+CREG=1\r\n");
    		readModem();
    		if (strstrf(modemStr,"OK") == 0) {
    			//might be able to add a auto baud fix here...?
    			#ifdef _MODEM_DEBUG_
    				sprintf(buffer, ">MODEM: not registered on network (%s)\r\n",modemStr);
    				print(buffer);
    			#endif
    			return ERROR;
    		}
    	}
    	else { 
    	    #ifdef _MODEM_DEBUG_
    		sprintf(buffer, ">+CREG RESPONSE: (%s)\r\n",modemStr);
    		print(buffer);
    		#endif
    		//See what the status of the CREG command.
    		
    		if (strstrf (modemStr, "1,0") != 0) 
    		{ 
        		sprintf(buffer, "The Modem could not register on a network and not searching\r\n");
        		print(buffer);
     		    return false;
     	    }
     	    else if (strstrf (modemStr, "1,1") != 0) 
    		{ 
        		#ifdef _MODEM_DEBUG_
        		sprintf(buffer, "Modem registered OK");
        		print(buffer);
     		    #endif
     	    }
     	    else if (strstrf (modemStr, "1,2") != 0) 
    		{ 
        		#ifdef _MODEM_DEBUG_
        		sprintf(buffer, "The Modem could not register on a network but is searching\r\n");
        		print(buffer);
     		    #endif
     		    //TODO......  keep trying to find the network
     		    return false;
     	    }
     	    else if (strstrf (modemStr, "1,3") != 0) 
    		{ 
        		sprintf(buffer, "The Modem was denied registration on the network\r\n");
        		print(buffer);
     		    return false;
     	    }
     	    else if (strstrf (modemStr, "1,4") != 0) 
    		{ 
        		sprintf(buffer, "The Modem registration on the network is unknown\r\n");
        		print(buffer);
     		    return false;
     	    }
     	    else if (strstrf (modemStr, "1,5") != 0) 
    		{ 
        		#ifdef _MODEM_DEBUG_
        		sprintf(buffer, "The Modem registered OK and is roaming\r\n");
        		print(buffer);
     		    #endif
     	    } 
     	    
    	}
    	 
    	readModem();
    	#ifdef _MODEM_DEBUG_
    	sprintf(buffer, ">+CREG 2nd RESPONSE: (%s)\r\n",modemStr);
    	print(buffer);
    	#endif
    }
    

    
	delay_ms(500);	
	
	//Only check the SIM on GSM units
	if (modem_type == GSM_Q2403B)
	{
    	//Check to see if there is a pin on the sim card.
    	printf("AT+CPIN?\r\n");
    	readModem();
    	#ifdef _MODEM_DEBUG_
    	sprintf(buffer, ">MODEM: CPIN (%s)\r\n",modemStr);
    	print(buffer); 
    	#endif
    	
    	if (strstrf(modemStr, "+CPIN: READY") != '\0') {
    		#ifdef _MODEM_DEBUG_
    			sprintf(buffer, ">MODEM: SIM not needed\r\n");
    			print(buffer);
    		#endif
    	}
    	else if (strstrf(modemStr, "+CPIN: SIM PIN") != '\0')  {
    		#ifdef _MODEM_DEBUG_
    			sprintf(buffer, ">MODEM: Need SIM PIN\r\n");
    			print(buffer);
    		#endif
    		printf("AT+CPIN=%4d\n\r", modemPinNumber);
    	}
    	else { 
    		#ifdef _MODEM_DEBUG_
    			sprintf(buffer, ">MODEM: SIM State not handled (%s)\r\n",modemStr);
    			print(buffer);
    		#endif
    	}
    }
	
	
	//Setup the connection with the home network
	
	delay_ms(1000);
	
	error_count=0;
 	//Setup SMS parameters for delivering and receiving SMS
	//Set SMS to be sent in Text mode
	printf("AT+CMGF=1\r\n");
	readModem();
	if (strstrf(modemStr,"OK") == '\0') { 
		//The command has not been successfully completed.  Try again
		#ifdef _MODEM_DEBUG_
		sprintf(buffer, ">MODEM: ERROR +CMGF (%s)\r\n", modemStr);
		print(buffer); 
		#endif
		if ((strncmpf(modemStr,"+CME ERROR: 515", strlen(modemStr)) == 0) ||
		    (strncmpf(modemStr,"+CMS ERROR: 515", strlen(modemStr)) == 0)) {    
		    while(1) {
    			//might be able to add a auto baud fix here...?
    			delay_ms(500);
    			printf("AT+CMGF=1\r\n");
    			
    			readModem();
    			if ((strncmpf(modemStr,"+CME ERROR: 515", strlen(modemStr)) == 0) || 
    			    (strncmpf(modemStr,"+CMS ERROR: 515", strlen(modemStr)) == 0)) {
        			#ifdef _MODEM_DEBUG_
        			sprintf(buffer, ">MODEM: busy AT+CMGF=1 (%s)\r\n", modemStr);
        			print(buffer);
        			#endif
        			if(++error_count > MAX_ERROR_RETRY) { 
        			    return ERROR;
        			}
        		}
        		
            	else if (strncmpf(modemStr,"OK",2) == 0) { 
            	    #ifdef _MODEM_DEBUG_
            		sprintf(buffer, ">+CMGF RESPONSE: %s\r\n",modemStr);
            		print(buffer);
            		#endif 
            		break;
            	}
            	else { 
            	    #ifdef _MODEM_DEBUG_
            		sprintf(buffer, ">MODEM: Fatal Error - %s\r\n",modemStr);
            		print(buffer);
            		#endif
            	    return ERROR;
            	}	
            }
        }
        else { 
            #ifdef _MODEM_DEBUG_
            sprintf(buffer, ">MODEM: Fatal Error - %s\r\n",modemStr);
            print(buffer);
            #endif
            return ERROR;
        }
	}
	#ifdef _MODEM_DEBUG_
	else { 
		sprintf(buffer, ">+CMGF RESPONSE: %s\r\n",modemStr);
		print(buffer);
	}
	#endif
	
	
	
	delay_ms(250);
	
	//Config for the GSM modem
	if (modem_type == GSM_Q2403B) 
	{
       	//Set in Text mode parameters
    	printf("AT+CSMP=1,169,0,0\r\n");
    	readModem();
    	if (strstrf(modemStr,"OK") == '\0') { 
    		//The command has not been successfully completed.  Try again
    		printf("AT+CSMP=17,169,0,0\r\n");
    		readModem();
    		if (strstrf(modemStr,"OK") == '\0') {
    			//might be able to add a auto baud fix here...?
    			#ifdef _MODEM_DEBUG_
    				sprintf(buffer, ">MODEM: could not config AT+CMGF\r\n");
    				print(buffer);
    			#endif
    			return ERROR;
    		}
    	}
    	#ifdef _MODEM_DEBUG_
    	else { 
    		sprintf(buffer, ">+CSMP %s\r\n",modemStr);
    		print(buffer);
    	}
    	#endif
    	
    	delay_ms(250);
    	
    	//Set how the modem handles incoming messages
    	printf("AT+CNMI=0,1,1,1,0\r\n");  
    	readModem();
    	if (strstrf(modemStr,"OK") == '\0') { 
    		//The command has not been successfully completed.  Try again
    		printf("AT+CNMI=0,1,1,1,0\r\n");
    		readModem();
    		if (strstrf(modemStr,"OK") == '\0') {
    			//might be able to add a auto baud fix here...?
    			#ifdef _MODEM_DEBUG_
    				sprintf(buffer, ">MODEM: could not setup CNMI register\r\n");
    				print(buffer);
    			#endif
    			return ERROR;
    		}
    	}
    	#ifdef _MODEM_DEBUG_
    	else { 
    		sprintf(buffer, ">+CNMI %s\r\n",modemStr);
    		print(buffer);
    	}
    	#endif 
    } 
     
    //Config for the CDMA modems
    else if (modem_type == CDMA_Q2438F_TAU || modem_type == CDMA_Q2438F_TNZ)
    { 
        //Set how the modem handles incoming messages
    	printf("AT+CNMI=2,1,1,1,0\r\n");  
    	readModem();
    	if (strstrf(modemStr,"OK") == '\0') { 
    		//The command has not been successfully completed.  Try again
    		printf("AT+CNMI=2,1,1,1,0\r\n");
    		readModem();
    		if (strstrf(modemStr,"OK") == '\0') {
    			//might be able to add a auto baud fix here...?
    			#ifdef _MODEM_DEBUG_
    				sprintf(buffer, ">MODEM: could not setup CNMI register\r\n");
    				print(buffer);
    			#endif
    			return ERROR;
    		}
    	}
    	#ifdef _MODEM_DEBUG_
    	else { 
    		sprintf(buffer, ">+CNMI %s\r\n",modemStr);
    		print(buffer);
    	}
    	#endif 
    }   
	
	delay_ms(250);
	
	
	//Turn the caller ID block off.
	printf("AT+CLIR=0\r\n");
	readModem();
	if (strstrf(modemStr,"OK") == '\0') { 
		//The command has not been successfully completed.  Try again
		printf("AT+CLIR=0\r\n");
		readModem();
		if (strstrf(modemStr,"OK") == '\0') {
			//might be able to add a auto baud fix here...?
			#ifdef _MODEM_DEBUG_
				sprintf(buffer, ">MODEM: could not setup CLIR register\r\n");
				print(buffer);
			#endif
			return ERROR;
		}
	}
	#ifdef _MODEM_DEBUG_
	else { 
		sprintf(buffer, ">+CLIR %s\r\n",modemStr);
		print(buffer);
	}
	#endif
	
	delay_ms(250);
	
	//Set the incoming call indication
	printf("AT+CRC=1\r\n");
	readModem();
	if (strstrf(modemStr,"OK") == '\0') { 
		//The command has not been successfully completed.  Try again
		printf("AT+CRC=1\r\n");
		readModem();
		if (strstrf(modemStr,"OK") == '\0') {
			//might be able to add a auto baud fix here...?
			#ifdef _MODEM_DEBUG_
				sprintf(buffer, ">MODEM: could not setup CRC register\r\n");
				print(buffer);
			#endif
			return ERROR;
		}
	}
	#ifdef _MODEM_DEBUG_
	else { 
		sprintf(buffer, ">+CRC %s\r\n",modemStr);
		print(buffer);
	}
	#endif
	
	 
	modem_read_ready=false;
	
	return OKAY;
	
	
}	 






void modem_tm_task_handler(void) { 
    char ch;
    switch (modem_tm_current_task) { 
        case MODEM_READ_MODEM:
            if (rx_counter0 > 0) { 
                ch = getchar(); 
                if (ch == NO_CARRIER[no_carrier_count]) { 
                    if (++no_carrier_count >= 10) {
                        #ifdef _MODEM_DEBUG_
                        sprintf(buffer,">MODEM: NO CARRIER found, through mode off\r\n");
                        print(buffer);
                        #endif
                        modem_through_mode = false;
                        no_carrier_count=0;
                    }                       
                } 
                else { 
                    no_carrier_count=0;
                }
                putchar(ch);
                putchar1(ch);      
            }
            modem_tm_set_task(MODEM_READ_UART);
            break;
        case MODEM_READ_UART : 
            if (rx_counter1 > 0) { 
                ch = getchar1();
                putchar(ch);
                if (ch == '+') { 
                    if (++uart_plus_count >= 3) {
                        #ifdef _MODEM_DEBUG_
                        sprintf(buffer,">MODEM: +++ found, disconnect\r\n");
                        print(buffer);
                        #endif
                        printf("\r\nATH\r\n");
                        modem_through_mode = false;
                    }                       
                } 
                else { 
                    uart_plus_count=0;
                }
                   
            }
            modem_tm_set_task(MODEM_READ_MODEM);
            break; 
        default: 
            break;
    }
                
     

    return;
}