#include "global.h" 
#include "drivers\gprs.h"
                                   

#if TCPIP_AVAILABLE
#include <stdlib.h>
#include <delay.h>              //SJL - CAVR2 - added
#include <stdio.h>              //SJL - CAVR2 - added
#include <string.h>             //SJL - CAVR2 - added

#include "drivers\modem.h"
#include "drivers\uart.h"       //SJL - added to access uart functions
#include "drivers\debug.h"      //SJL - added to access debug functions
#include "drivers\config.h"     //SJL - added to access config timer variables
#include "drivers\str.h"        //SJL - added to access string functions  

bool gprs_startConnection()
{ 
    //Open the TCP/IP stack      
    if(verbose)
    {
        printStr("Attempting to open GPRS connection...\r\n");     
    }
    tickleRover();            
                     
    modem_clear_channel();  
                    
    #if MODEM_TYPE == Q2406B                             
       // sprintf(buffer,"Setting APN to ");
       // print(buffer);                                                  
        strcpyre(buffer,config.apn);
        printf("AT#APNSERV=\"%s\"\r\n",buffer); 
        modem_wait_for(MSG_OK | MSG_ERROR);

        strcpyre(buffer,config.dns0);
	    printf("AT#DNSSERV1=\"%s\"\r\n",buffer);
        modem_wait_for(MSG_OK | MSG_ERROR);               
        
        strcpyre(buffer,config.dns1);
	    printf("AT#DNSSERV2=\"%s\"\r\n",buffer);
        modem_wait_for(MSG_OK | MSG_ERROR);
    
        //Set PPPmode
        tickleRover();
        printf("at#pppmode=1\r\n");
        modem_wait_for(MSG_OK | MSG_ERROR);
        #ifdef DEBUG
        sprintf(buffer,"AT#PPPmode=1 >> %s\r\n",modem_rx_string);print(buffer);
        #endif
        if (!strstrf(modem_rx_string, "OK"))
        {
            DEBUG_printStr("GPRS Connection start Failed\r\n");
            return false;
        } 
        
        //Set the GPRSmode
        tickleRover();    
        printf("at#gprsmode=1\r\n");    
        modem_wait_for(MSG_OK | MSG_ERROR);
        #ifdef DEBUG
        sprintf(buffer,"AT#GPRSMODE=1 >> %s\r\n",modem_rx_string);print(buffer);
        #endif
        if (strcmpf(modem_rx_string, "OK") != 0)
        {
            DEBUG_printStr("GPRS Connection start Failed\r\n");
            return false;
        }       
    GPRS_retry2:    
        //modem_register = true;
        //Attach the modem onto the GPRS network    
        tickleRover();                
        printf("AT+CGATT=1\r\n");         
        modem_wait_for(MSG_OK | MSG_ERROR);
        if (strcmpf(modem_rx_string, "OK") != 0)
        {
            DEBUG_printStr("GPRS Connection start Failed\r\n");
            return false;
        }             
        tickleRover();
        delay_ms(1000);  
        tickleRover();  
        modem_wait_for(MSG_OK | MSG_ERROR | MSG_WIND | MSG_CGREG_1 | MSG_CGREG_5);
    
    
    GPRS_retry:   
        
        //Start the connection
        tickleRover();
        printf("at#connectionstart\r\n");
        modem_wait_for(MSG_ERROR | MSG_Ok_InfoGprsActivation);  
    #elif MODEM_TYPE == Q24NG_PLUS   
        DEBUG_printStr("Starting bearer\r\n");
        
        printf("AT+WIPCFG=1\r\n");        
        modem_wait_for(MSG_OK | MSG_ERROR);   
        
        printf("AT+WIPBR=1,6\r\n");        
        modem_wait_for(MSG_OK | MSG_ERROR);   
        
       // sprintf(buffer,"Setting APN to ");
      //  print(buffer);                                                  
        strcpyre(buffer,config.apn);
       // print(buffer);
        printf("AT+WIPBR=2,6,11,\"%s\"\r\n",buffer);
        sprintf(buffer,"\r\n");     
       // print(buffer);         
        modem_wait_for(MSG_OK | MSG_ERROR);             
       
        DEBUG_printStr("Opening link\r\n");
        printf("AT+WIPBR=4,6,0\r\n");       
        modem_wait_for(MSG_OK | MSG_ERROR);
    #elif HW_REV == V6     
                                          
        strcpyre(buffer,config.apn);
        printf("AT+CGDCONT=1,\"IP\",\"%s\"\r\n",buffer);
        modem_wait_for(MSG_OK | MSG_ERROR);   
        
        strcpyre(buffer,config.dns0);
        printf("AT+iDNS1=\"%s\"\r\n",buffer);
        modem_wait_for(MSG_OK | MSG_ERROR);
        
        strcpyre(buffer,config.dns1);
        printf("AT+iDNS2=\"%s\"\r\n",buffer);
        modem_wait_for(MSG_OK | MSG_ERROR);    
        
        printf("AT+iISP1=\"*99#\"\r\n",buffer);
        modem_wait_for(MSG_OK | MSG_ERROR);            
        
      //  printf("AT+iUP:1\r\n");
      //  modem_wait_for(MSG_OK | MSG_ERROR);
        
    #else 
        #error "Modem type not defined properly!"
    #endif
                                  
    #if MODEM_TYPE == Q2406B
    if (strstrf(modem_rx_string, "49155"))
    { 
        DEBUG_printStr("GPRS Session didn't start, try again\n\r");
        delay_ms(2000);
        tickleRover();
        goto GPRS_retry;
    }
    else if (strstrf(modem_rx_string, "35865"))   
    { 
        DEBUG_printStr("GPRS Session didn't start, try again\n\r");
        delay_ms(2000);
        tickleRover();
        goto GPRS_retry2;
    }             
    else 
    #elif MODEM_TYPE == Q24NG_PLUS
    if(strstrf(modem_rx_string,"803")||strstrf(modem_rx_string,"804"))
    {
        if(verbose)
            printStr("GPRS Connection established\r\n");
    } 
    else
    #endif
    if (strstrf(modem_rx_string, "ERROR")) 
    {                     
        if(verbose)
            printStr("GPRS Connection start Failed\r\n");
        return false;
    }                          
    else if(verbose)
    {
        printStr("GPRS Connection established\r\n");
    }
    modem_state = GPRS_CONNECTION;
    startupError = false;
    return true;  
}
   
bool gprs_stopConnection()
{                                                    
    active_led_on(); 
    timer = 60;
    timer_overflow = false;
    if(verbose) 
        printStr("Disconnecting from GPRS\r\n");
    #if MODEM_TYPE == Q2406B
        printf("AT#CONNECTIONSTOP\r\n"); 
        modem_wait_for(MSG_OK | MSG_ERROR);
    #elif MODEM_TYPE == Q24NG_PLUS
        modem_clear_channel();             
        printf("AT+WIPBR=5,6\r\n");
        modem_wait_for(MSG_OK | MSG_ERROR);
    #elif HW_REV == V6
    //    #warning "ConnectOne stuff missing here!"
        modem_state = CELL_CONNECTION;
        return true; 
    #else 
        #error "Modem type not properly defined!"
    #endif
    
    
    #if MODEM_TYPE != Q24NG_PLUS
    if(!(strstrf(modem_rx_string,"OK")||
         strstrf(modem_rx_string,"ERROR: 805")))
    {
        if(verbose)
            printStr("Failed to disconnect, resetting...\r\n");
        system_reset();
    }                           
    #endif
    
    if(verbose) 
    {
        if(strstrf(modem_rx_string,"OK")) 
            printStr("Disconnected\r\n");    
        else printStr("GPRS already disconnected\r\n");
    }
    active_led_off();
    modem_state = CELL_CONNECTION;
    return true;    
}

bool gprs_setupSocket(unsigned char port)
{                   
    modem_clear_channel();
    printf("at#tcpserv=\"");
    print0_eeprom(gprs_ipAddress);
    printf("\"\r\n");   
    modem_wait_for(MSG_OK | MSG_ERROR);              
    #ifdef DEBUG
    sprintf(buffer,"at#tcpserv >> %s\r\n",modem_rx_string);print(buffer);
    #endif
    if (strcmpf(modem_rx_string, "OK") != 0)
    {
        DEBUG_printStr("GPRS failed to set IP address\r\n");
        return false;
    }
    
    printf("at#tcpport=\"%d\"\r\n",port);
    modem_wait_for(MSG_OK | MSG_ERROR);
    #ifdef DEBUG
    sprintf(buffer,"at#tcpport >> %s\r\n",modem_rx_string);print(buffer);
    #endif
    if (strcmpf(modem_rx_string, "OK") != 0)
    {
        DEBUG_printStr("GPRS Failed to set port\r\n");
        return false;
    }
    return true;
}

bool gprs_openSocket()
{                    
    modem_clear_channel();
    printf("at#otcp\r\n");
    #ifdef DEBUG
    sprintf(buffer,"at#otcp\r\n");print(buffer);
    #endif
    modem_wait_for(MSG_Ok_WaitingForData | MSG_ERROR);
    #ifdef DEBUG
    sprintf(buffer,"at#otcp >> %s\r\n",modem_rx_string);print(buffer);
    #endif
    if (strcmpf(modem_rx_string, "Ok_Info_WaitingForData") == 0)
    {
        DEBUG_printStr("TCP socket on port %d open at ");
        #ifdef DEBUG
        print1_eeprom(gprs_ipAddress);
        #endif
        DEBUG_printStr("\r\n");
    } 
    else 
    { 
        DEBUG_printStr("Failed to open TCP socket on port %d open at ");
        #ifdef DEBUG
        print1_eeprom(gprs_ipAddress);
        #endif
        DEBUG_printStr("\r\n");
        return false;
    }   
    return true;
}   

bool gprs_closeSocket()
{ 
    printf("%c", 3);   
    modem_clear_channel();
    return true;
}                      
#endif           
               
