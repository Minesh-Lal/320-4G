/*//#include "extrauart.h"
#include <i2c.h>
#include <delay.h>

#define PUTCHAR_CMD       0x01
#define GETCHAR_CMD       0x02
#define RX_COUNTER_CMD    0x04
#define COMMAND_READY_CMD 0x08

#define UART_2_ADDRESS    0x40
//extern char buffer[160]; //SJL - CAVR2 - include uart.h

void putchar2(char c)
{
    while(!i2c_start());
    i2c_write(UART_2_ADDRESS);
    i2c_write(PUTCHAR_CMD);
    i2c_write(c);
    i2c_stop();
}

char getchar2()
{
    char c;
    while(!i2c_start());
    i2c_write(UART_2_ADDRESS);
    i2c_write(GETCHAR_CMD);
    while(!i2c_start());
    i2c_write(UART_2_ADDRESS | 0x01);
    c = i2c_read(0);
    i2c_stop();
    return c;
}

char rx_counter2()
{
    //get over I2C the rx counter on UART2
    char i;
    while(!i2c_start());
    i2c_write(UART_2_ADDRESS);
    i2c_write(RX_COUNTER_CMD);
    while(!i2c_start());
    i2c_write(UART_2_ADDRESS | 0x01);
    i = i2c_read(0);
    i2c_stop();
    return i;
}

char command_ready2()
{
    //find out if the UART2 buffer has a full command saved
    char i;
    while(!i2c_start());
    i2c_write(UART_2_ADDRESS);
    i2c_write(COMMAND_READY_CMD);
    while(!i2c_start());
    i2c_write(UART_2_ADDRESS | 0x01);
    i = i2c_read(0);
    i2c_stop();
    return i;
}
*/