#include "drivers\gps.h"
#include "global.h"

#if GPS_AVAILABLE
	#include <math.h>
	#include <string.h> to gps.c		//SJL - CAVR2 - added
	#include <stdlib.h> to gps.c        //SJL - CAVR2 - added
	#include <stdio.h> to gps.c			//SJL - CAVR2 - added

	#include "global.h" to gps.c		//SJL - CAVR2 - added
	#include "drivers\uart.h" to gps.c	//SJL - CAVR2 - added


float north,east,speed,direction;

void gps_process_string(char *c)
{
    char *temp;

    //location marker
    if(strstrf(c,"$GPGGA"))
    {
    #ifdef DEBUG
        printStr("Location NMEA sentance found\r\n");
    #endif
        c = strchr(c,',')+1; //discard the $GPPGA
        if(!c)
        {
            if(verbose)
                printStr("ERROR\r\n");
            return;
        }

        c = strchr(c,',')+1; //discard the time
        if(!c)
        {
             if(verbose)
                printStr("ERROR\r\n");
            return;
        }

        //next digits are latitude
        temp = strchr(c,',');
        *temp = '\0';
        temp++;
    #ifdef DEBUG
        sprintf(buffer,"latitude: %s\r\n",c);print(buffer);
    #endif

        north = atof(c); //still needs processing, good for now
        if(*temp == 'S' || *temp == 's')
            north *= -1;
        temp+=2;

        c = temp;

        //next digits are longitude
        temp = strchr(c,',');
        *temp = '\0';
        temp++;
    #ifdef DEBUG
        sprintf(buffer,"longitude: %s\r\n",c);print(buffer);
    #endif

        east = atof(c); //still needs processing, good for now
        if(*temp == 'W' || *temp == 'w')
            east *= -1;
    } else if (strstrf(c,"$GPRMC"))
    {
    #ifdef DEBUG
        printStr("Velocity NMEA sentance found\r\n");
    #endif
        //skip straight to the speed
        if(strchr(c,'E'))
            c = strchr(c,'E')+2;
        else
            c = strchr(c,'W')+2;

        if(!c)
        {
            if(verbose)
                printStr("ERROR\r\n");
            return;
        }
    #ifdef DEBUG
        sprintf(buffer,"speed: %s\r\n",c);print(buffer);
    #endif
        speed = atof(c);

        if(strchr(c,','))
            c = strchr(c,',')+1;
        else
        {
            if(verbose)
                printStr("ERROR\r\n");
            return;
        }
    #ifdef DEBUG
        sprintf(buffer,"bearing: %s\r\n",c);print(buffer);
    #endif
        direction = atof(c);
    }
    else
    {
        if(verbose)
            printStr("ERROR\r\n");
        return;
    }
    if(verbose)
        printStr("OK\r\n");
}

float gps_get_north()
{
    return north;
}

float gps_get_east()
{
    return east;
}

char *gps_print(char *c, char format)
{
    float f1,f2;
    switch(format)
    {
        case DEGREES:
            f1 = ((int)(north/100))+(fmod(north,100)/60);
            f2 = ((int)(east/100)) +(fmod(east,100)/60);
            if(north == 0 && east == 0)
                sprintf(c,"UNKNOWN,UNKNOWN");
            else if(north >= 0)
            {
                if(east >= 0)
                    sprintf(c,"%fN,%fE",f1,f2);
                else
                    sprintf(c,"%fN,%fW",f1,-f2);
            }
            else
            {
                if(east >= 0)
                    sprintf(c,"%fS,%fE",-f1,f2);
                else
                    sprintf(c,"%fS,%fW",-f1,-f2);
            }
            return c;
            break;
        case DEGREES_MINUTES:
        default:
            if(north == 0 && east == 0)
                sprintf(c,"UNKNOWN");
            else if(north >= 0)
            {
                if(east >= 0)
                    sprintf(c,"%2.0f* %02.4f'N, %3.0f* %02.4f'E",(north-fmod(north,100))/100,fmod(north,100),(east-fmod(east,100))/100,fmod(east,100));
                else
                    sprintf(c,"%2.0f* %02.4f'N, %3.0f* %02.4f'W",(north-fmod(north,100))/100,fmod(north,100),-(east-fmod(east,100))/100,fmod(-east,100));
            }
            else
            {
                if(east >= 0)
                    sprintf(c,"%2.0f* %02.4f'S, %3.0f* %02.4f'E",-(north-fmod(north,100))/100,fmod(-north,100),(east-fmod(east,100))/100,fmod(east,100));
                else
                    sprintf(c,"%2.0f* %02.4f'S, %3.0f* %02.4f'W",-(north-fmod(north,100))/100,fmod(-north,100),-(east-fmod(east,100))/100,fmod(-east,100));
            }
            return c;
            break;
        }
}

void gps_velocity(char *c)
{
    float s;
    s = speed*1852/3600;
    sprintf(c,"%f m/s bearing %f degrees",s,direction);
}

#endif