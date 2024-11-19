#ifndef _GPRS_H_
#define _GPRS_H_
#include "global.h"  
#if TCPIP_AVAILABLE
bool gprs_startConnection();
bool gprs_stopConnection();  
bool gprs_setupSocket(unsigned char port);
bool gprs_openSocket();   
bool gprs_closeSocket();     
#endif
#endif