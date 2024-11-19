#ifndef _DEBUG_H_
#define _DEBUG_H_
                  
#include "global.h"

void DEBUG_printStr (flash char str[]);
void DEBUG_printU8 (unsigned char var);
void DEBUG_printCR (void);   
void dump_memory();

#endif
