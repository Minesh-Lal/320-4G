/*
    Implementation for Linked list contact list
*/
#include <stdio.h>             //SJL - CAVR2 - added
#include <string.h>             //SJL - CAVR2 - added

#include "drivers\contact.h"
#include "drivers\str.h"
#include "drivers\uart.h"		//SJL - CAVR2 - added
#include "drivers\mmc.h"		//SJL - CAVR2 - added

//#define _CONTACT_LIST_DEBUG_
//#ifdef _CONTACT_LIST_DEBUG_
//char contact_buff[40];
//#endif
//Write a contact into the linked list
int contact_write(char *content)
{
    int next;
    char pos;
	
	#ifdef _CONTACT_LIST_DEBUG_
	char contact_buff[40];
	#endif
	
    next = (int)area;
	
    #ifdef _CONTACT_LIST_DEBUG_
    sprintf(contact_buff,"writing {%p}\r\n",content);log_line("system.log",contact_buff);
    print(contact_buff);
    sprintf(contact_buff,"area=0x%04x\r\n",area);log_line("system.log",contact_buff);
    print(contact_buff);
    #endif
    pos = 0;

    while(1)
    {
        tickleRover();
        #ifdef _CONTACT_LIST_DEBUG_
        sprintf(contact_buff,"next = 0x%04x\r\n",next);log_line("system.log",contact_buff);
        print(contact_buff);
        #endif
        if(*(eeprom int*)next == 0)
        {
            //if((*(eeprom int*)next)+strlen(content)+5>=area+CONTACT_AREA_SIZE) //SJL - CAVR2 - types incompatible
            if((*(eeprom int*)next)+strlen(content)+5>=((int)area)+CONTACT_AREA_SIZE)
                return -1;
            //*(eeprom int*)next = next+strlen(content)+5;
			*(eeprom int*)next = next+45;	//SJL - make all contact entries 40 chars long
											//allows contacts to be added or modified (with more chars) by SMS
            #ifdef _CONTACT_LIST_DEBUG_
            sprintf(contact_buff,"*next now points to 0x%04x\r\n",*(eeprom int*)next);log_line("system.log",contact_buff);
            print(contact_buff);
            #endif
            strcpye((eeprom char*)next+2, content);
            next = *(eeprom int*)next;
            *(eeprom int*)next = 0;   //setting the next next to null. Part of the clearing contacts change
            #ifdef _CONTACT_LIST_DEBUG_
            sprintf(contact_buff,"set 0x%04x to 0x%04x\r\n",next,*(eeprom int*)next);log_line("system.log",contact_buff);
            print(contact_buff);
            #endif
            return pos;
        }
        else
        {
            pos++;
            #ifdef _CONTACT_LIST_DEBUG_
            sprintf(contact_buff,"next moving from 0x%04x to 0x%04x\r\n",next,*(eeprom int*)next);log_line("system.log",contact_buff);
            print(contact_buff);
            #endif
            next = *(eeprom int*)next;
        }
    }
}

//Read a contact from a known index
eeprom char *contact_read(char index)
{
    int next;
    //char pos;
	
	#ifdef _CONTACT_LIST_DEBUG_
	char contact_buff[40];
	#endif
	
    next = (int)area;
    //pos = 0;
    #ifdef _CONTACT_LIST_DEBUG_
    sprintf(contact_buff,"looking for contact %d\r\n",index);
    print(contact_buff);
    #endif
    while(index)
    {
        tickleRover();
        index--;
        #ifdef _CONTACT_LIST_DEBUG_
        sprintf(contact_buff,"skipping...\r\n",index);
        print(contact_buff);
        #endif
        if(*(eeprom int*)next == 0)
        {
            return 0;
        }
        else
        {
              next = *(eeprom int*)next;
        }
    }
    return (eeprom char*)next+2;
}

char contact_modify(char *content,char position)
{
    eeprom char *ptr, *temp;

    if (strlen(content) > 0){
        if(strcmpf(content,"<>") != 0)
        {
            if(strstrf(content,"@") != 0)
                sprintf(buffer,"E<%s>",content);
            else if(strstrf(content,"ftp://") != 0)
                sprintf(buffer,"F<%s>",content);
            else if(strstrf(content,"mailto://") != 0)
                sprintf(buffer,"E<%s>",content);
            else if(strstrf(content,"file://") != 0)
                sprintf(buffer,"D<%s>",content);
            else if(strstrf(content,"http://") != 0)
                sprintf(buffer,"H<%s>",content);
            else
                sprintf(buffer,"S<%s>",content);
            content = buffer;
        }
    }


    ptr = contact_read(position)-2; //eeprom int pointer to the end of the string.
    if(*(eeprom int*)ptr != 0)
    {
        if(strlen(content)+3+(int)ptr < *(eeprom int*)ptr) //SJL - CAVR2 - incompatible types for use with < operator
        //if(strlen(content)+3+ptr < ptr) //save space at the end
        {
            strcpye(ptr+2,content);
            return position;
        } else return -1;
    }
    else return contact_write(content);

}

//Reset the contact list
void contact_reset()
{
    int i;
	
	#ifdef _CONTACT_LIST_DEBUG_
	char contact_buff[40];
	#endif
	
    #ifdef _CONTACT_LIST_DEBUG_
    sprintf(contact_buff,"resetting contact area with zeros\r\n");
    print(contact_buff);
    #endif
    for(i=0;i<CONTACT_AREA_SIZE;i++)
    {
        tickleRover();
        area[i] = '\0';
    }
}

//Return the type of contact at a certain index
msg_type contact_getType(unsigned char index)
{
    char type;
    eeprom char* temp;
    temp = contact_read(index);
    if(temp == 0)
        return EMPTY_CONTACT;
    strcpyre(buffer, temp);
    type = buffer[0];
    switch (type)
    {
        case 'S':
            return SMS;
        case 'E':
            return EMAIL;
        case 'F':
            return FTP;
        case 'T':
            return TCP;
        case 'H':
            return HTTP;
        case 'V':
            return VOICE;
        case 'D':
            return DATA;
        default:
            return SMS;
    }
}