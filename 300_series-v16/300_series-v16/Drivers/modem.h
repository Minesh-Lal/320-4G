#ifndef _MODEM_H_
#define _MODEM_H_

#include "global.h"

#if MODEM_TYPE==Q24NG_PLUS
enum
{
    IDLE,
    BUSY,
    ACTIVE
};
#endif

#if (MODEM_TYPE == Q2406B || MODEM_TYPE == Q24NG_PLUS || MODEM_TYPE == Q24NG_CLASSIC)
    #define modem_is_wavecom_gsm() true
#else
    #define modem_is_wavecom_gsm() false
#endif

// *** Interrupts ***
interrupt [TIM0_OVF] void timer0_ovf_isr(void);

// *** Modem Functions ***
void modem_read(void);
int modem_init(unsigned char modem_type);
bool modem_throughMode(void);
bool modem_readPort(void);
bool modem_handleNew(void);
void modem_restart();
bool modem_clear_channel();
void modem_check_status(char *print_flag);
void modem_disconnect_csd();
void modem_requestUnsolicited();
unsigned int modem_wait_for(unsigned int index);
char modem_get_rssi();
char modem_get_ber();
void enter_command_mode();
char *base64encode(char *c, eeprom char *source);
void reset_uarts();

#if EMAIL_AVAILABLE
void udp_putchar(char c);
#endif

#if SMS_AVAILABLE
char modem_send_sms(char *text, char *number);
char modem_read_sms(char index);
#endif

// *** Variables ****
extern char sequential_plus;
extern char packeted_data;
extern char modem_not_responding;
static const char NO_CARRIER[11] = {"NO CARRIER"};

#endif