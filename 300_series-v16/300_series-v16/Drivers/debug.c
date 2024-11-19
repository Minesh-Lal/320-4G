/* 
    Debug printing for strings
*/
#include <stdio.h>

#include "global.h"
#include "drivers\debug.h"
#include "drivers\uart.h"

void DEBUG_printStr (flash char str[])
{ 
    #ifdef DEBUG
    sprintf(buffer,"%p",str);
    print(buffer);
    #endif
    return;
}      
void DEBUG_printU8 (unsigned char var)
{ 
    #ifdef DEBUG
    sprintf(buffer,"%d",var);
    print(buffer);
    #endif
    return;
}
void DEBUG_printCR (void)
{ 
    #ifdef DEBUG
    sprintf(buffer,"\r\n");
    print(buffer);
    #endif
    return;
}               

void dump_memory()
{
    int i,j;
    for(i=0;i<4096;i++)
    {               
        sprintf(buffer,"%04X> ",i);
        print(buffer);
        for(j=0;j<16;j++,i++)
        {
            sprintf(buffer,"%02X ",*(char*)i);
            print(buffer);
        }                 
        printStr("\r\n");
    }
}
                                       