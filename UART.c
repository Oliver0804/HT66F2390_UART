#include <HT66F2390.h>
#include <stdlib.h>


#define fH 8000000
#define BR 19200       // Baud rate


typedef	unsigned char	u8;
typedef	char			s8;
typedef	unsigned short	u16;
typedef	short			s16;
typedef	unsigned long	u32;
typedef	long			s32;



u8 pwm_value = 0;  // For storing value of AT+PWM
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

void initpwm(){
	_pds0=0x12; _pds1=0x02;					//PD0->STP1(R),PD2->PTP2(G),PD4->PTP3(B)
	_stm1al=0; _stm1ah=0;					//Duty=0	
	_stm1rp=4;								//PWM ?g??=1024/fINT
	_stm1c0=0b00011000;						//fINT=fSYS(8MHz),ST1ON=1
	_stm1c1=0b10101000;						//PWM???,Active High,STM1RP????g??
	_ptm2al=0; _ptm2ah=0;					//Duty=0
	_ptm2rpl=(u8)1024; _ptm2rph=1024>>8;	//PWM ?g??=1024/fINT
	_ptm2c0=0b00011000;						//fINT=fSYS(8MHz),PT2ON=1
	_ptm2c1=0b10101000;						//PWM???, Active High
	_ptm3al=0; _ptm3ah=0;					//Duty=0
	_ptm3rpl=(u8)1024; _ptm3rph=1024>>8;;	//PWM ?g??=1024/fINT
	_ptm3c0=0b00011000;						//fINT=fSYS(8MHz),PT3ON=1
	_ptm3c1=0b10101000;						//PWM???, Active High}
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
    char temp[10]; // 臨時陣列
    int i = 0;
    int j = 0;

    // 處理0的特殊情況
    if (value == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    // 為負數增加負號
    if (value < 0)
    {
        str[j++] = '-';
        value = -value;
    }

    // 轉換數字並儲存在臨時陣列中
    while (value != 0)
    {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }

    // 從臨時陣列複製到結果陣列
    while (i > 0)
    {
        str[j++] = temp[--i];
    }

    str[j] = '\0'; // 結束字符串

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
    initpwm();
    while(1)
    {
        send_char('.');
        Delayms(250);
        _stm1al=(u8)pwm_value; _stm1ah=pwm_value>>8;		//Update Duty(R)
		_ptm2al=(u8)pwm_value; _ptm2ah=pwm_value>>8;		//Update Duty(G)
		_ptm3al=(u8)pwm_value; _ptm3ah=pwm_value>>8;		//Update Duty(B)
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
