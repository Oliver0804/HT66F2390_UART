#include <HT66F2390.h> // Include the header file for the HT66F2390 microcontroller
#include <stdlib.h>    // Include the standard library for general functions

#define fH 8000000     // Define the system oscillator frequency as 8 MHz
#define BR 19200       // Define the UART baud rate as 19200

// Type definitions for variable types
typedef unsigned char u8;    // Type definition for an unsigned 8-bit integer
typedef char s8;             // Type definition for a signed 8-bit integer
typedef unsigned short u16;  // Type definition for an unsigned 16-bit integer
typedef short s16;           // Type definition for a signed 16-bit integer
typedef unsigned long u32;   // Type definition for an unsigned 32-bit integer
typedef long s32;            // Type definition for a signed 32-bit integer

// Global variables for storing AT command values
u8 pwm_value = 100;  // Stores the PWM value from AT+PWM command
u16 pwr_value = 1;   // Stores the power value from AT+PWR command
u16 ver_value = 0;   // Stores the version value from AT+VER command

// Define NULL if it's not defined
#ifndef NULL
#define NULL ((void *)0)
#endif

// Define macros for switch input and configuration
#define SW0 _pa1    // Define switch 0 as PA1
#define SW0C _pac1  // Configuration register for switch 0
#define SW0PU _papu1  // Pull-up configuration for switch 0

#define MAGNETS_PER_REVOLUTION 12  // Number of magnets per wheel revolution
#define INTERVAL_MS 100            // Sampling interval in milliseconds
#define MS_PER_MINUTE 60000        // Number of milliseconds in a minute

// Global variables
volatile u16 magnet_count = 0;  // Magnetic sensor count variable
u16 last_magnet_count = 0;      // Last recorded magnet count
u32 timer_mark = 0;             // Timer mark for measuring intervals

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

void initpwm() {
    _pds0 = 0x12; _pds1 = 0x02; // Set PD0 as STP1 (Red LED), PD2 as PTP2 (Green LED), PD4 as PTP3 (Blue LED)
    _stm1al = 0; _stm1ah = 0;   // Set initial duty cycle to 0 for STM1
    _stm1rp = 4096;             // Set the PWM period for STM1 to 4096 / fINT
    _stm1c0 = 0b00011000;       // Set the internal clock fINT to system clock fSYS (8MHz), and turn on STM1
    _stm1c1 = 0b10101000;       // Set PWM mode for STM1, active high, reload period register STM1RP on overflow
    
    _ptm2al = 0; _ptm2ah = 0;   // Set initial duty cycle to 0 for PTM2
    _ptm2rpl = (u8)1024; _ptm2rph = 1024 >> 8; // Set the PWM period for PTM2 to 1024 / fINT
    _ptm2c0 = 0b00011000;       // Set the internal clock fINT to system clock fSYS (8MHz), and turn on PTM2
    _ptm2c1 = 0b10101000;       // Set PWM mode for PTM2, active high
    
    _ptm3al = 0; _ptm3ah = 0;   // Set initial duty cycle to 0 for PTM3
    _ptm3rpl = (u8)1024; _ptm3rph = 1024 >> 8; // Set the PWM period for PTM3 to 1024 / fINT
    _ptm3c0 = 0b00011000;       // Set the internal clock fINT to system clock fSYS (8MHz), and turn on PTM3
    _ptm3c1 = 0b10101000;       // Set PWM mode for PTM3, active high
}
// Initialize UART0
void inituart(){

	_papu6=1;	
	_papu7=1;	
    _pas1=0b11000000;      // PA7 for TX0 function, PA6 for RX0 function
    _u0cr1=0b10000000; // Enable UART0, 8-bit data format, odd-even parity function disabled, 1 stop bit, no break character sent.
    _u0cr2=0b11100100;  // Enable transmitter, enable receiver, high-speed baud rate, address match function disabled, RX0 wake-up function disabled, receiver interrupt function enabled, transmitter empty interrupt, transmitter ready interrupt.
    _brg0=fH/((u32)16*BR)-1;  // Configure baud rate
    _ur0e = 1;     // Interrupt flag bit
    _mf5e=1;        
    _emi=1;
}

// 
void initInt(){
	_papu1=1; 
	//_papu7=1;							//?P??PA3/PA7 Pull-High?q??
	//_int1ps=1;									//???wINT1?\???PA7?}???{
	_integ=0b00001010;							//???INT1/INT0???t?t??o????
	_int0e=1; _int1e=1; _emi=1;					//?P??INT0,INT1?Τ??_?`?}??
	}
	
void initGPIO(){
		SW0C=1; SW0PU=1;                      			//規劃為輸入屬性並致能提升電阻
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
                rbuff[data_count]='\r';
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


void main() {
    _wdtc = 0b10101111; // Disable watchdog timer
    inituart(); // Initialize UART
    initpwm();  // Initialize PWM
    initInt();  // Initialize interrupts
    u32 speed = 0;   // Variable to hold the speed calculation
    u8 set_pwm = 0;  // Variable to adjust PWM duty cycle

    while(1) { // Infinite loop
        timer_mark++; // Increment timer mark
        if(timer_mark >= 100) { // Check if 100ms have passed
            // Calculate speed: RPM = (magnet count * number of milliseconds per minute) / (number of magnets per revolution * interval in milliseconds)
            speed = (u32)magnet_count * MS_PER_MINUTE / (MAGNETS_PER_REVOLUTION * INTERVAL_MS);

            // Convert speed to string
            char speed_str[10];
            intToStr(speed, speed_str);

            // Send the current speed to the serial port
            send_buff("Current Speed: ");
            send_buff(speed_str);
            send_buff(" RPM\r\n");

            // Convert magnet count to string
            char count_str[10];
            intToStr(magnet_count, count_str);

            // Send the magnet count to the serial port
            send_buff("magnet_count: ");
            send_buff(count_str);
            send_buff("\r\n");

            // Reset magnet count
            magnet_count = 0;

            // Reset timer mark
            timer_mark = 0;
        }
        Delayms(10); // Wait for roughly 10 milliseconds

        // Set PWM value based on the speed
        if (speed > 0 && speed <= 5) {
            pwm_value = 50;
        } else if (speed > 5 && speed <= 10) {
            pwm_value = 80;
        } else if (speed > 10 && speed <= 15) {
            pwm_value = 110;
        } else if (speed > 15 && speed <= 20) {
            pwm_value = 140;
        } else if (speed > 20 && speed <= 25) {
            pwm_value = 170;
        } else if (speed > 25) {
            pwm_value = 200;
        } else {
            pwm_value = 0;
        }

        // Gradually adjust set_pwm towards pwm_value
        if (set_pwm > pwm_value) {
            set_pwm--;
        } else if (set_pwm < pwm_value) {
            set_pwm++;
        }

        // Update PWM duty cycle if power value is greater than or equal to 1
        if (pwr_value >= 1) {
            _stm1al = (u8)set_pwm; // Update Duty for STM1 (R)
            _stm1ah = set_pwm >> 8;
            // The lines for PTM2 and PTM3 are commented out, assuming they're for Green and Blue LEDs
        } else {
            _stm1al = 0; // Set Duty for STM1 (R) to 0 if power value is less than 1
            _stm1ah = set_pwm >> 8;
            // The lines for PTM2 and PTM3 are also set to 0, as they're commented out
        }
    }
}


// ISR for UART0, triggered when UART data is received
DEFINE_ISR(UART0, 0x3C) {
    char buff[100] = {'\0'}; // Initialize a buffer to store received data
    get_buff(buff, '\n'); // Receive a string from UART until newline character
    
    handleATCommand(buff); // Process the received AT command
    
    _ur0f = 0; // Clear the UART0 interrupt flag
}

// ISR for external interrupt INT0
DEFINE_ISR(ISR_Int0, 0x04) {
    _int0e = 0; // Disable INT0 interrupts temporarily to prevent re-entry
    _emi = 1;   // Enable global interrupts

    // Placeholder for code, could be used to send a character 'A' for debugging
    // send_char('A');

    _int0f = 0; // Clear the interrupt flag for INT0
    _int0e = 1; // Re-enable INT0 interrupts
}

// ISR for external interrupt INT1, possibly for a magnetic sensor
DEFINE_ISR(ISR_Int1, 0x08) {
    u8 i, temp;
    _int1e = 0; // Disable INT1 interrupts to prevent bounce-triggered actions

    _emi = 1;   // Enable global interrupts

    // Placeholder for code, could be used to send a character 'B' for debugging
    // send_char('B');

    magnet_count++; // Increment the magnet count, used in RPM calculation
    
    _int1f = 0; // Clear the interrupt flag for INT1 to prevent bounce effects
    _int1e = 1; // Re-enable INT1 interrupts
}
