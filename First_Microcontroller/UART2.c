// Runs on TM4C123
// Simple device driver for the UART. This is an example code for UART board to board communication.
// board to board communitation uses UART1
// By Dr.Min He
#include "UART2.h"
#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdbool.h>  // for C boolean data type

#define NVIC_EN0_UART1 0x40
#define NVIC_EN1_UART2 0x02

//------------UART_Init------------
// Initialize the UART for 115,200 baud rate (assuming 50 MHz UART clock),
// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
// Input: none
// Output: none
void UART2_Init(bool RxInt, bool TxInt){
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART2;  // Activate UART2
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD;  // Activate port D
  while((SYSCTL_RCGC2_R & SYSCTL_RCGC2_GPIOD) == 0); // Wait for port D to be ready
  
  GPIO_PORTD_LOCK_R = GPIO_LOCK_KEY;  // Unlock GPIO Port D
  GPIO_PORTD_CR_R |= 0xC0;  // Allow changes to PD7 and PD6
  GPIO_PORTD_LOCK_R = 0x00;  // Lock GPIO Port D

  UART2_CTL_R = 0;  // Reset UART1
  UART2_IBRD_R = 54;  // IBRD = int(16,000,000 / (16 * 115,200)) = int(8.68)
  UART2_FBRD_R = 16;  // Baud rate: 38400, FBRD = int(0.68 * 64 + 0.5) = 44
  UART2_LCRH_R = UART_LCRH_WLEN_8;  // 8 bit word length (no parity bits, one stop bit, FIFOs)
  
  if (RxInt) {
    UART2_IM_R |= UART_IM_RXIM;  // Enable RX interrupt
  }
  
  if (TxInt) {
    UART2_IM_R |= UART_IM_TXIM;  // Enable TX interrupt
  }
  
  UART2_CTL_R |= UART_CTL_UARTEN | UART_CTL_RXE | UART_CTL_TXE;  // Enable UART, Rx, Tx
  
  if (RxInt || TxInt) {
    NVIC_PRI8_R = (NVIC_PRI8_R & 0xFFFF1FFF) | 0x0000A000;  // Bits 15-13, priority 5 (bit |15|14|13| = 101
    NVIC_EN1_R = NVIC_EN1_UART2;  // Enable interrupt 5 in NVIC
  }
  
  GPIO_PORTD_AFSEL_R |= 0xC0;  // Enable alternate function on PD7-6
  GPIO_PORTD_DEN_R |= 0xC0;    // Enable digital I/O on PD7-6
  GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0x00FFFFFF) + 0x11000000;  // Configure PD7-6 as UART
  GPIO_PORTD_AMSEL_R &= ~0xC0;  // Disable analog functionality on PD7-6
}


//------------UART_InChar------------
// Wait for new serial port input
// Input: none
// Output: ASCII code for key typed
uint8_t UART2_InChar(void){
  while((UART2_FR_R&UART_FR_RXFE) != 0); // wait until the receiving FIFO is not empty
  return((uint8_t)(UART2_DR_R&0xFF));
}
//------------UART_OutChar------------
// Output 8-bit to serial port
// Input: letter is an 8-bit ASCII character to be transferred
// Output: none
void UART2_OutChar(uint8_t data){
  while((UART2_FR_R&UART_FR_TXFF) != 0);
  UART2_DR_R = data;
}

//------------UART_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART2_OutString(unsigned char *pt){
  while(*pt){
    UART2_OutChar(*pt);
    pt++;
  }
}

//------------UART_InString------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
//    when max length is reach, no more input will be accepted
//    the display will wait for the <enter> key to be pressed.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART2_InString(unsigned char *bufPt, unsigned short max) {
int length=0;
char character;
  character = UART2_InChar();
  while((character!=CR) && (length < max-1)){
		*bufPt = character;
		bufPt++;
		length++;
    character = UART2_InChar();
	}
  *bufPt = 0; // adding null terminator to the end of the string.
}
