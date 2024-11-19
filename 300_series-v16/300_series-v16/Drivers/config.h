#ifndef _CONFIG_H_
#define _CONFIG_H_


#define CONFIG_DELIMITER 'ß'
#define CONFIG_BREAK '§'
#define CONFIG_ALARM '¡'

#define CONFIG_USER 1
#define CONFIG_SERIAL 2
#define CONFIG_SITEMSG 3
#define CONFIG_PHONECOUNT 5
#define CONFIG_PHONELIST 6
#define CONFIG_SERIAL_FAIL 7
#define CONFIG_PIN 8
#define CONFIG_INPUT_START 9
#define CONFIG_INPUT_ALARM_MSG 28
#define CONFIG_INPUT_RESET_MSG 10
#define CONFIG_SYNC 11
#define CONFIG_INPUT_SETTINGS 12
#define CONFIG_INPUT_UNIT 13
#define CONFIG_INPUT_MAXENG 14
#define CONFIG_INPUT_MINENG 15
#define CONFIG_INPUT_ALARMVAL  16
#define CONFIG_INPUT_RESETVAL  17
#define CONFIG_INPUT_ACONTACT 18
#define CONFIG_INPUT_RCONTACT 19
#define CONFIG_INPUT_DEBOUNCE 20
#define CONFIG_INPUT_NEXT 21
#define CONFIG_OUTPUT_START 22
#define CONFIG_OUTPUT_NAME 23
#define CONFIG_OUTPUT_DEFAULT_STATE 24
#define CONFIG_OUTPUT_ONCONTACT 25
#define CONFIG_OUTPUT_OFFCONTACT 26
#define CONFIG_OUTPUT_OFF_MSG 27
#define CONFIG_OUTPUT_ON_MSG 29
#define CONFIG_COMPLETE 30
#define CONFIG_INPUT_OUTPUTLIST 31
#define CONFIG_OUTPUT_NEXT 32
#define CONFIG_ALARM_ENABLED 33
#define CONFIG_ALARM_SETTINGS 34
#define CONFIG_OUTPUT_ALARM_LIST 35
#define CONFIG_OUTPUT_RESET 36
#define CONFIG_OUTPUT_RESET_LIST 37
#define CONFIG_ALARM_NEXT 38

//#define DEBOUNCE_FILTER 1

#define config_set_task(task) config_current_task=task


/* Input
 All of the 8 inputs have information associated with them that is set by
 configuring the units and also information to do with their current alarm
 state. Note this is all stored in eerpom
*/


/* This structure stores the current information about how the alarm is being
 handled
*/


typedef eeprom struct controlOutputStruct
{
    bool me;
    unsigned int contact_list;      //external devices to be sent the message
    unsigned char outswitch;
    unsigned char outaction;
} out_t;

/* This structure stores the configuration settings of the inut, setup through
 the config software
*/

typedef eeprom struct inputAlarmStruct
{
    int type;	            //type of alarm (none, alarm above, below...)
    unsigned char lastZone;            //stores the current alarm state
	float set;			    //alarm set point	//SJL - CAVR2 - double not supported
	float reset;		    //alarm reset point	//SJL - CAVR2 - double not supported
	unsigned int thresholdLow;    //The lowest threshold for the alarm (either set or reset)
	unsigned int thresholdHigh;   //The highest threshold
	char alarm_msg[MAX_MSG_LEN+1];	    //alarm message
	char reset_msg[MAX_MSG_LEN+1];	    //Alarm reset message
	long alarm_contact;	//phone numbers to sms on alarm unsigned int
	unsigned int reset_contact;	//phone numbers to sms on alarm
	unsigned long debounce_time;   //period of time to debounce input in 100th of seconds
	bool startup_alarm;     //this setting allows the user to send an alarm msg
	                        //on start up if the alarm is present or not.
	out_t alarmAction;
    out_t resetAction;
} inputAlarm_t;

typedef eeprom struct sensorStruct
{
    unsigned int thresholdLow;
    unsigned int thresholdHigh;
} sensor_t;

typedef eeprom struct inputConfigStruct
{
	bool enabled;	    //This is set if the input is configured.
	unsigned char type;			//The type on input- digital, 0-5V, 4-20mA
    char log_type;      //type of log to record. LOG_NONE,LOG_INSTANT,LOG_AVERAGE,LOG_MIN,LOG_MAX,LOG_AGGREGATE,LOG_DUTY,LOG_ALARM
	char units[9]; 		//The engineering units the input is measured in
	float eng_min;         //Min value that corresponds to 0v or 4mA	//SJL - CAVR2 - double not supported
	float eng_max;         //max value that corresponds to 5V or 20mA	//SJL - CAVR2 - double not supported
    float conv_grad;	    //data conversion constant					//SJL - CAVR2 - double not supported
	float conv_int;	    //data conversion constant						//SJL - CAVR2 - double not supported
	unsigned char dp;			//number of decimal places the input is read to.
	inputAlarm_t alarm[INPUT_MAX_ALARMS];   //Structure to store all the data about each alarm	//117x2
	sensor_t sensor;    //Structure to store the data about hte sensor, tracks	//4
	                    //if sensor is out of bounds.
	char msg;
	//268bytes
} input_t;

typedef eeprom struct outputConfigStruct {
    char name[21];
    unsigned char default_state;    //This stores the default start up state, ON, OFF or LAST_KNOWN
    char on_msg[MAX_MSG_LEN+1];        //The msg sent when the output is turned on
    char off_msg[MAX_MSG_LEN+1];       //The msg sent when the output is turned off
    unsigned int on_contact;    //Phone numbers that are asscociated to the output
    unsigned int off_contact;    //Phone numbers that are asscociated to the output
    unsigned char momentaryLength;
	//109
} output_config_t;

typedef eeprom struct outputStruct {
    bool enabled;           //The output is configured and can be used.  If this is set false, no
                            //msg's are sent about it
    void (*set) (unsigned char);    //Turn on/off the output
    unsigned char state;            //stores the last set state of the output, this allows the outputs to
                            //be restored on startup (this is tied to default_state in the config
    output_config_t config; //config structure
	//112bytes
}
output_t;

enum{
    SECONDS,
    MINUTES,
    HOURS
};

typedef eeprom struct pulseConfig {
    float pulses_per_count;	//SJL - CAVR2 - double not supported
    char period;
}
pulse_t;

typedef eeprom struct auto_report_settings{
    char second;
    char minute;
    char hour;
    char day;
    char type;
} AutoReportSettings;



//Data structure of the overall config setep
typedef eeprom struct mainConfig {

    //char contact_list[17][MAX_PHONE_LEN+1];  //Array of the phone numbers in the unit's contact list
    AutoReportSettings autoreport;	//5 chars
    unsigned char contact_count;        //number of contact phone numbers stored in the memory
    char site_name[21];         //The site name, appended to all msgs
    unsigned int sms_retry;            //Number of times the SMS controller retries to send an SMS
                                //if a send fails
    char pin_code[5];           //Pin code to turn on the outputs. Note this is not the SIM cards
                                //Pin number
    bool public_queries;        //Will the unit respond to queries that are from phones not on the
                                //contact list
    bool forwarding;            //If a SMS is received from VODAFONE (eg prepay a/c low sms) will
                                //it be forwarded
    bool psu_monitoring;        //Is the power supply monitoring enabled.  This uses input 1
    char update_address[64];
    long log_entry_period;		//SJL - CAVR2 - from main.c - changed to signed because -1 value is used
    int log_update_period;  	//SJL - CAVR2 - from main.c - changed to signed because -1 value is used

    char data_log_enabled;
    char alarm_log_enabled;
    char rssi_log_enabled;
    char loc_log_enabled;
    char advanced_features;

    #if SMTP_CLIENT_AVAILABLE
    unsigned int smtp_port;
    char smtp_serv[60];
    #if SMTP_AUTH_AVAILABLE
    char smtp_un[30];
    char smtp_pw[30];
    #endif
    char email_domain[60];
    #endif

    #if POP3_CLIENT_AVAILABLE || SMTP_CLIENT_AVAILABLE
    char from_address[60];
    #endif
	
    #if POP3_CLIENT_AVAILABLE
    unsigned int pop3_port;
    char pop3_serv[60];
    char pop3_un[30];
    char pop3_pw[30];
    #endif

    #if MODEM_TYPE == Q24NG_PLUS || MODEM_TYPE == SIERRA_MC8795V
    char dns0[16];
    char dns1[16];
	char apn[40];
    #endif

	//422 up to here
	
    //ick, but I'm putting it here to save space
    pulse_t pulse[2];				//5bytes x 2
	input_t input[MAX_INPUTS];		//268bytes x 8
    output_t output[MAX_OUTPUTS];	//112bytes x 4
	
	//

} config_t;


//extern eeprom unsigned char* serial;
eeprom extern unsigned char* serial;

void config_read_file(void);
void config_print_file(bool checksum);
void config_print_contacts(bool checksum);
//void config_load_str();
//void config_print_str();
//char *config_strip_zeroes (double var);
void config_test_com();
void config_factoryReset(void);

/* GLOBALS */
/**********************************************************************************************************/
/* PROTOTYPE HEADRERS */

/**********************************************************************************************************/
/* STRUCTS */
eeprom extern config_t config;
//extern eeprom int eeprom_yr;
//extern eeprom char* eeprom_mth,eeprom_dy,eeprom_hr,eeprom_min,eeprom_sec;

/**********************************************************************************************************/
/* VARIABLES */
extern bit config_continue; //flag to indicate config is continuing  	//SJL - CAVR2 - from global.c
extern bit config_break; //a break character was found                	//SJL - CAVR2 - from global.c
extern long log_entry_timer;                                  			//SJL - CAVR2 - from main.c - changed to signed
extern int log_update_timer;                                  			//SJL - CAVR2 - from main.c - changed to signed
extern bit timer_overflow;                                            	//SJL - CAVR2 - from config.c
extern char timer;                                                     	//SJL - CAVR2 - from config.c

//extern char _FF_buff[512];

/**********************************************************************************************************/

#endif