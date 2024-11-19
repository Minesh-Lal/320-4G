#ifndef _STR_H_
#define _STR_H_     

#include "global.h"

char* substring(char* large,  char *returnValue, int start);
char* substr(char* large, char *returnValue, int start, int end);
void strnibble(char *str, unsigned char length); 
void strcpye(eeprom char *s1,const char *s2); 
void strcpyre(char *s1,eeprom char *s2); 
void strcpyef(eeprom char *s1,flash char *s2);
void strrep(char *string, flash char *find, flash char *replace);
unsigned char strncmpe(char *s1, char eeprom *s2, unsigned char n);
unsigned int strlene(eeprom char *s);       

#endif