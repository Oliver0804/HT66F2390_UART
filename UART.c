#include <HT66F2390.h>
typedef unsigned long u32;
typedef unsigned short u16;
#define fH 8000000
#define BR 19200       // Baud rate
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
    char buff[100]={'\0'};
    get_buff(buff,'\n'); // Receive a string (the string must have a newline character)
    send_buff(buff); // Send the received string back
    _ur0f = 0;    
}
