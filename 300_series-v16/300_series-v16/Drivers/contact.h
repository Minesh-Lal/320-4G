#ifndef CONTACT_H
#define CONTACT_H

#include "global.h"
#include "drivers\sms.h"

#define area contact_area
#define CONTACT_AREA_SIZE 700

int contact_write(char *content);
char contact_modify(char *content, char position);
eeprom char *contact_read(char index);
void contact_reset();
msg_type contact_getType(unsigned char index);

#endif