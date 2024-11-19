#ifndef _GPS_H_
#define _GPS_H_
//void gps_process_string(char *nmea_sentance); //SJL - CAVR2 - variable didn't match function
void gps_process_string(char *c);
float gps_get_north();
float gps_get_east();
//char *gps_print(char *buffer, char format);	//SJL - CAVR2 - variable didn't match function
char *gps_print(char *c, char format);
void gps_velocity(char *c);

enum
{
    DEGREES,
    DEGREES_MINUTES,
    DEGREES_MINUTES_SECONDS
};

#endif