#ifndef MMC_H
#define MMC_H
#if LOGGING_AVAILABLE

// *** SD Card Functions ***
void mmc_init();

// *** Logging Functions ***
//void log_entry();
void log_alarm(char event, char param);
bool log_line(flash char *filename,char *c);
bool mmc_clear_data();
void latest_log_entry();


// *** Date and Time String Functions
void log_get_date_from_string(char *c,unsigned int *year,unsigned char *month,unsigned char *day,
	unsigned char *hour,unsigned char *minute,unsigned char *second);
bool cmdDates_to_mmcDates(char *command_dateTime);
#if EMAIL_AVAILABLE
void concat_0();
void concat_1();
void concat_2();
#endif

// *** Print Logs to UART Functions
void print_log();
void print_system();
void print_alarm();
void print_latest();

#if ERROR_FILE_AVAILABLE
void print_error();
#endif

#if EMAIL_AVAILABLE
// *** Email Functions ***
bool email_log();
bool email_send();
char *dns_lookup2();	//(char *url);
bool email_error();
bool email_latest(char *remove_file);
#endif

// *** Global Varibales ***
extern bit mmc_fail_sent;
extern float input_running_total[MAX_INPUTS];
extern unsigned long input_samples[MAX_INPUTS];

#endif
#endif