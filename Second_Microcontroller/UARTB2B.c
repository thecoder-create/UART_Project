#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "UART.h"
#include "UART2.h"
#include "UART3.h"
#include <stdint.h>
#include <stdbool.h>

#define LED (*((volatile unsigned long *)0x40025038)) // onboard LEDs: PF3, PF2, PF1 (RGB)

#define WHEELSIZE 8
#define MAX_STR_LEN 50
#define Dark    	0x00
#define Red     	0x02
#define Blue    	0x04
#define Green   	0x08
#define Yellow  	0x0A
#define Cran      0x0C
#define White   	0x0E
#define Purple  	0x06
#define SW1       0x10
#define SW2       0x01
#define NVIC_EN0_PORTF 0x40000000
#define PERIOD 			160000           	// stay on, Change brightness
#define MIN_DUTY    PERIOD/10
#define MAX_DUTY    PERIOD*0.9
#define DUTY_STEP		PERIOD/10

extern void DisableInterrupts(void);
extern void EnableInterrupts(void);
extern void WaitForInterrupt(void);

void Start_Prompt(void);
void Mode_One(void);
void Mode_Two(void);
void Mode_Three(void);
void UDec_to_Str(uint8_t str[], uint32_t n);
void GPIO_PortF_Init(void);
void MCU_One_Mode_Two_Command(void);

unsigned long H, L;
unsigned char str_text[20];
static int k;
bool mode2;
bool waitformessage;
const long COLORWHEEL[WHEELSIZE] = {Dark, Red, Blue, Green, Yellow, Cran, White, Purple};
uint32_t Str_to_UDec(uint8_t str[]);
uint8_t str_idx=0;
unsigned int hex_number; 
unsigned int entered_mode;
unsigned int check;
uint8_t string[MAX_STR_LEN];
bool end_of_str=false;
bool mode1=false;
bool mode3=false;
uint32_t n;
uint8_t i;

int main(void){
	DisableInterrupts();	
	PLL_Init();
	UART0_Init(true, false);  // initialize UART 0 with interrupt
	UART2_Init(false, false); // initialize UART2 with no interrupt
	UART3_Init(false, false); // initialize UART 3 with no interrupt
	GPIO_PortF_Init();			  // initialize port F
	LED = Dark;
	hex_number = Dark;
	entered_mode = 0x00;
	mode2 = false;
	EnableInterrupts();       // enable interrupts for TExaS
	
	while(1){
		Start_Prompt();
		while(entered_mode == 0x00){
			entered_mode = UART2_InChar();
		}
		
		if(entered_mode == 0x02){
			MCU_One_Mode_Two_Command();
			while((UART2_FR_R & UART_FR_RXFE) != 0);
			check = UART2_InChar();
			if(check != 0x66){
				LED = check; // busy waiting approach for UART Rx
				hex_number = LED;
				Mode_Two();
			}
			else{
				entered_mode = 0x00;
				hex_number = Dark;
				LED = Dark;
			}
		}
		if(entered_mode == 0x05){
			Mode_Three();
		}
		
	}
}

void SysTick_Handler(void){
	NVIC_ST_CTRL_R &= ~NVIC_ST_CTRL_ENABLE;
	if(hex_number != 0x00){
		if(GPIO_PORTF_DATA_R & hex_number){   // toggle LEDs
			GPIO_PORTF_DATA_R &= ~hex_number; // make LEDs low
			NVIC_ST_RELOAD_R = L - 1;         // reload value for low phase
		} else{
			GPIO_PORTF_DATA_R |= hex_number;  // make LEDs high
			NVIC_ST_RELOAD_R = H - 1;         // reload value for high phase
		}
	}
	NVIC_ST_CURRENT_R = 0;
	NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE;
}

void UART0_Handler(void){
	uint8_t chr;
	if(UART0_RIS_R & UART_RIS_RXRIS){ // received one item
		if((UART0_FR_R & UART_FR_RXFE) == 0){
			chr = UART0_DR_R & 0xFF;
			if(chr == CR){ // end of the string
				end_of_str = true;
				string[str_idx] = NULL; // add null terminator
			} else if(str_idx < (MAX_STR_LEN - 1)){ // save one spot for null terminator
				if(chr == BS){
					UART_OutChar(BS);
					str_idx--;
				} else{
					string[str_idx++] = chr; // add the latest received symbol
					UART_OutChar(chr);
				}
			}
		}
		UART0_ICR_R = UART_ICR_RXIC; // acknowledge RX FIFO
	}
}

// Take care of Rx interrupt and ignore Tx interrupt
void UART2_Handler(void){
  if(UART2_RIS_R&UART_RIS_RXRIS){       // received one item
		if ((UART2_FR_R&UART_FR_RXFE) == 0){
		  check = UART2_DR_R;
			if (check==0x24){
			UART2_InString(str_text, 20);  // receive acknowledge from MCU2
			waitformessage = false;
			}
		}
			//	UART2_InString(str_text, 20);  // receive acknowledge from MCU2
    UART2_ICR_R = UART_ICR_RXIC;        // acknowledge RX FIFO
  }
}

void GPIOPortF_Handler(void){
	for(uint32_t time = 0; time < 80000; time++) {} // debounce
	
	if(GPIO_PORTF_RIS_R & SW2){ // SW2 is pressed
		GPIO_PORTF_ICR_R = SW2;
		if(mode2){
			
		if(k >= WHEELSIZE-1) {
			k = 0;
		} else {
			k++;
		}
		LED = COLORWHEEL[k];
		hex_number=LED;

			UART_OutString((uint8_t *)"Current color: ");
			switch(hex_number){
				case Red:    UART_OutString((uint8_t *)"Red ");    break;
				case Blue:   UART_OutString((uint8_t *)"Blue ");   break;
				case Green:  UART_OutString((uint8_t *)"Green ");  break;
				case Dark:   UART_OutString((uint8_t *)"Dark ");   break;
				case Yellow: UART_OutString((uint8_t *)"Yellow "); break;
				case Cran:   UART_OutString((uint8_t *)"Cran ");   break;
				case Purple: UART_OutString((uint8_t *)"Purple "); break;
				case White:  UART_OutString((uint8_t *)"White ");  break;
			}
			OutCRLF();

		}		
	}
	
	if(GPIO_PORTF_RIS_R & SW1){
		GPIO_PORTF_ICR_R = SW1;
		if(mode2){
			UART3_OutChar(LED);
			entered_mode = UART2_InChar();
			mode2 = false;
		}
		else if(mode3){
			mode3=false;
		}
	}
}

void Start_Prompt(void){
	OutCRLF();
	UART_OutString((uint8_t *)"Welcome to CECS 447 Project 2  - UART");
	OutCRLF();
	OutCRLF();
	UART_OutString((uint8_t *)"MCU2");
	OutCRLF();
	OutCRLF();
	UART_OutString((uint8_t *)"Waiting command from MCU1...");
}

void MCU_One_Mode_Two_Command(void){
	OutCRLF();
	OutCRLF();
	UART_OutString((uint8_t *)"Mode 2 MCU2");
	OutCRLF();
	OutCRLF();
	UART_OutString((uint8_t *)"Waiting for color code from MCU1...");
}

void Mode_One(void){
	// Mode 1 implementation
}

void Mode_Two(void){
	UART3_OutChar(0x03);
	OutCRLF();
	UART_OutString((uint8_t *)"Mode 2 MCU1: press ^ to exit this mode");
	OutCRLF();
	OutCRLF();
	UART_OutString((uint8_t *)"In color Wheel State.");
	OutCRLF();
	OutCRLF();
	UART_OutString((uint8_t *)"Please press sw2 to go through the colors in the following color wheel: Dark, Red, Green, Blue, Yellow, Cran, Purple, White.");
	OutCRLF();
	OutCRLF();
	UART_OutString((uint8_t *)"Once a color is selected, press sw1 to send the color to MCU2.");
	OutCRLF();
	OutCRLF();
	UART_OutString((uint8_t *)"Current color: ");
	switch(hex_number){
		case Red:    UART_OutString((uint8_t *)"Red ");    break;
		case Blue:   UART_OutString((uint8_t *)"Blue ");   break;
		case Green:  UART_OutString((uint8_t *)"Green ");  break;
		case Dark:   UART_OutString((uint8_t *)"Dark ");   break;
		case Yellow: UART_OutString((uint8_t *)"Yellow "); break;
		case Cran:   UART_OutString((uint8_t *)"Cran ");   break;
		case Purple: UART_OutString((uint8_t *)"Purple "); break;
		case White:  UART_OutString((uint8_t *)"White ");  break;
	}
	OutCRLF();

	do{
		mode2 = true;
		while(!end_of_str && mode2){ // wait until the whole string is received
			WaitForInterrupt();
		}
		end_of_str = false;
		str_idx = 0;
		if(string[str_idx] == '^'){
			mode2 = false;
			LED = Dark;
			hex_number = Dark;
			entered_mode = 0x00;
			UART3_OutChar(0x66);
		}
	} while(mode2);
	OutCRLF();
}

void Mode_Three(void){
	mode3=true;
	waitformessage = true;
	unsigned char prompt[]="Please input a string:\n\r";
	unsigned char sent_str[]="Sent message!\n\r";
  OutCRLF();
  OutCRLF();
	UART_OutString((uint8_t *)"Mode 3 MCU2: Chat Room");
  OutCRLF();
	UART_OutString((uint8_t *)"Press sw1 at any time to exit the chat room.");
  OutCRLF();
	UART_OutString((uint8_t *)"Waiting for a message from MCU1 ...");
  OutCRLF();
	
	while(mode3){
		OutCRLF();
		
//		while(waitformessage){}; //Wait until it recieves messages from MCU2
//		waitformessage = true;
		UART2_InString(str_text, 20);  // receive acknowledge from MCU2
		OutCRLF();
		UART_OutString(str_text);  // display the acknowledge in serial terminal
		UART_OutChar(CR);
		OutCRLF();
		UART_InString(str_text, 20); // receive a string from PC, user is typing
		UART3_OutString(str_text);  // sent string to MCU2
		UART3_OutChar(CR);
	}
}

uint32_t Str_to_UDec(uint8_t str[]){
	uint32_t number = 0;;
	uint8_t character, idx = 0;
	character = str[idx];
	while(character != NULL){
		if((character >= '0') && (character <= '9')) {
			number = 10 * number + (character - '0');
		} else {
			return number;
		}
		character = str[++idx];
	}
	return number;
}

void UDec_to_Str(uint8_t str[], uint32_t n){
	static uint8_t idx = 0;
	if((n / 10) != 0){
		UDec_to_Str(str, n / 10);
		UDec_to_Str(str, n % 10);
	} else{
		str[idx++] = n % 10 + '0';
		return;
	}
	str[idx] = '\0';
	return;
}

void GPIO_PortF_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5; // enable clock for Port F
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0); // wait for Port F to be ready

	H = L = PERIOD / 2; // 50% duty cycle, assuming system clock 16MHz

	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
	GPIO_PORTF_CR_R |= 0x01; // allow changes to PF0

	GPIO_PORTF_DIR_R &= ~0x11; // PF0 and PF4 input
	GPIO_PORTF_PUR_R |= 0x11;  // enable pull-up resistors for PF0 and PF4
	GPIO_PORTF_DEN_R |= 0x11;  // enable digital I/O on PF0 and PF4
	GPIO_PORTF_IS_R &= ~0x11;  // PF0, PF4 is edge-sensitive
	GPIO_PORTF_IBE_R &= ~0x11; // PF0, PF4 is not both edges
	GPIO_PORTF_IEV_R |= 0x11;  // PF4 rising ege event
	GPIO_PORTF_ICR_R = 0x11;   // clear flag4
	GPIO_PORTF_IM_R |= 0x11;   // arm interrupt on PF4
	NVIC_PRI7_R = (NVIC_PRI7_R & ~0x00E00000) | 0x00C00000; // priority 6
	NVIC_EN0_R |= NVIC_EN0_PORTF; // enable interrupt 30 in NVIC

	GPIO_PORTF_DIR_R |= 0x0E; // PF1, PF2, PF3 output
	GPIO_PORTF_DEN_R |= 0x0E; // enable digital I/O on PF1, PF2, PF3
	GPIO_PORTF_AFSEL_R &= ~0x0E; // disable alternate functions on PF1, PF2, PF3

	GPIO_PORTF_DATA_R &= ~0x0E; // clear PF1, PF2, PF3
}
