#include <stdio.h>
#include <string.h>

#include "global.h"


/*
    String routines that are specific to codevision
*/

//Nibble along the string length amount
void strnibble(char *str, unsigned char length) { 
    *str += length;
    return;

}

//Extract part of the string from index start to the end of the string 
char* substring(char* large,  char *returnValue, int start){
	strcpy(returnValue, large+start);
	return returnValue;
}
//Extract part of the string from start to end
char* substr(char* large, char *returnValue, int start, int end){
	int length;
	if(end >= start)
		length = end-start;
	else
		length = strlen(large+start);
	strncpy(returnValue, large+start, length);
	returnValue[length] = '\0';
	return returnValue;

}
//Copy a string into eeprom
void strcpye(eeprom char *s1,const char *s2)
{
    while (*s2)
        *s1++ = *s2++;
    *s1='\0';
}
//Copy a string from eeprom to ram
void strcpyre(char *s1,eeprom char *s2)
{
    while (*s2)
        *s1++ = *s2++;
    *s1='\0';
    return;
} 
//copy a string from flash to eeprom
void strcpyef(eeprom char *s1,flash char *s2)
{
    while (*s2)
        *s1++ = *s2++;
    *s1='\0';
    return;
} 


/* compare at most char s1[] to char s2[] */
unsigned char strncmpe(char *s1, char eeprom *s2, unsigned char n) {
	if (n == 0)
		return (0);
	do {
		if (*s1 != *s2++)
			return (*(unsigned char *)s1 - *(unsigned char *)--s2);
		if (*s1++ == 0)
			break;
	} while (--n != 0);
	return (0);
}


//Return the string length of a string in eeprom
unsigned int strlene(eeprom char *s) {
    int x=0;
    while (*s++)
        x++;
    return(x);
}                          


void strrep(char *string, flash char *find, flash char *replace)
{                            
    char *c,*back_half;                
    back_half = string;
    if(!strlen(string))
        return;
    #pragma warn-
    while(c=strstrf(back_half,find)) 
    {                     
        back_half = c+strlenf(find);
        *c = '\0';                   
        sprintf(string,"%s%p%s",string,replace,back_half);
    }
    #pragma warn+
}     