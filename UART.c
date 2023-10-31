#include <HT66F2390.h>
#include <stdlib.h>

typedef unsigned long u32;
typedef unsigned short u16;
#define fH 8000000
#define BR 19200       // Baud rate





u16 pwm_value = 0;  // For storing value of AT+PWM
u16 pwr_value = 0;  // For storing value of AT+PWR
u16 ver_value = 0;  // For storing value of AT+VER

#ifndef NULL
#define NULL ((void *)0)
#endif



void Delayms(u16 del)
{   u16 i;
    for(i=0;i<del;i++) GCC_DELAY(2000);
}


// Send a single character
void send_char(char s)
{
    _txr_rxr0=s;
    while(!_txif0);    
}
// Send a string, note that if you are sending a character array, the end must be added with '\0', otherwise an error will occur.
void send_buff(char *s)
{
    while(*s)
    {
        send_char(*s);
        s++;
    }
}
// Initialize UART0
void inituart(){
    _wdtc=0b10101111;      // Disable watchdog
    _pas1=0b11000000;      // PA7 for TX0 function, PA6 for RX0 function
    _u0cr1=0b10000000; // Enable UART0, 8-bit data format, odd-even parity function disabled, 1 stop bit, no break character sent.
    _u0cr2=0b11100100;  // Enable transmitter, enable receiver, high-speed baud rate, address match function disabled, RX0 wake-up function disabled, receiver interrupt function enabled, transmitter empty interrupt, transmitter ready interrupt.
    _brg0=fH/((u32)16*BR)-1;  // Configure baud rate
    _ur0e = 1;     // Interrupt flag bit
    _mf5e=1;        
    _emi=1;
}
// Receive a string from UART, store in array, stopping character is c. Note: The received UART data at the end must have a stopping character c to be received correctly and stored in the character array.
int get_buff(char *rbuff,char c){
        int data_count=0;
        char  rdata;
        while(1){
            while(!_rxif0); 
            rdata =_txr_rxr0;
            if(rdata==c)
            {
                rbuff[data_count]='\0';
                return data_count;
            }
            rbuff[data_count]=rdata;
            data_count++;
        }
}


char* intToStr(int value, char* str)
{
    char temp[10]; // �{�ɰ}�C
    int i = 0;
    int j = 0;

    // �B�z0���S���p
    if (value == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    // ���t�ƼW�[�t��
    if (value < 0)
    {
        str[j++] = '-';
        value = -value;
    }

    // �ഫ�Ʀr���x�s�b�{�ɰ}�C��
    while (value != 0)
    {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }

    // �q�{�ɰ}�C�ƻs�쵲�G�}�C
    while (i > 0)
    {
        str[j++] = temp[--i];
    }

    str[j] = '\0'; // �����r�Ŧ�

    return str;
}

/*
AT+CMD?#
AT+PWM=<val>#
AT+PWR=<val>#
AT+VER=<val>#
*/

void handleATCommand(char *cmd)
{
    char response[50];
    char *endptr = strchr(cmd, '#'); 
    if (endptr == NULL)
    {
        send_buff("Invalid format!\r\n");
        return;
    }

    *endptr = '\0';

    if(strncmp(cmd, "AT+PWM=", 7) == 0)
    {
        pwm_value = atoi(&cmd[7]);
        send_buff("PWM value set!\r\n");
    }
    else if(strncmp(cmd, "AT+PWR=", 7) == 0)
    {
        pwr_value = atoi(&cmd[7]);
        send_buff("Power value set!\r\n");
    }
    else if(strncmp(cmd, "AT+VER=", 7) == 0)
    {
        ver_value = atoi(&cmd[7]);
        send_buff("Version value set!\r\n");
    }
    else if(strncmp(cmd, "AT+CMD?", 7) == 0)
    {
        // Construct the response manually
        intToStr(pwm_value, response);
        send_buff("PWM: ");
        send_buff(response);

        send_buff(", PWR: ");
        intToStr(pwr_value, response);
        send_buff(response);

        send_buff(", VER: ");
        intToStr(ver_value, response);
        send_buff(response);

        send_buff("\r\n");
    }
    else
    {
        send_buff("Unknown command!\r\n");
    }
}




void main()
{   
    inituart();
    while(1)
    {
        send_char('.');
        Delayms(500);
    }
}
// UART interrupt, when data is sent to UART, the microcontroller receives the data and sends it back through UART.
DEFINE_ISR(UART0,0x3C)
{
    char buff[100] = {'\0'};
    get_buff(buff, '\n'); // Receive a string (the string must have a newline character)
    
    handleATCommand(buff); // Handle the received AT command
    
    _ur0f = 0;
}
