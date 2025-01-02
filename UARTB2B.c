
// board to board communitation use UART1
// Ground connected ground in the USB cable
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "UART.h"
#include "UART2.h"
#include "UART3.h"
#include <stdint.h>

// bit address definition for port data registers
#define LED (*((volatile unsigned long *)0x40025038)) // use onboard three LEDs: PF3210

////////// Constants //////////  
// Color    LED(s) PortF
// dark     ---    0
// red      R--    0x02
// blue     --B    0x04
// green    -G-    0x08
// yellow   RG-    0x0A
// white    RGB    0x0E
// pink     R-B    0x06
// Cran     -GB    0x0C

#define WHEELSIZE 8           // must be an integer multiple of 2
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
//#define PERIOD 			1600           	// stay on, change brightness
//#define PERIOD 			16000           	// stay on, change brightness
#define PERIOD 			160000           	// stay on, Change brightness
//#define PERIOD 			1600000           	// faster blink
//#define PERIOD 			16000000           	// Slow blink
#define MIN_DUTY    PERIOD/10							// minimum duty cycle 10%
#define MAX_DUTY    PERIOD*0.9						// maximum duty cycle 90%
#define DUTY_STEP		PERIOD/10							// duty cycle change for each button press

extern void DisableInterrupts(void);
extern void EnableInterrupts(void);  
extern void WaitForInterrupt(void);
void Start_Prompt(void);
void Mode_One(void);
void Mode_Two(void);
void Mode_Three(void);
void MCU_Two_Mode_Two_Command(void);
void UDec_to_Str(uint8_t str[], uint32_t n);
void GPIO_PortF_Init(void);


//Global Veriables
// H: number of clocks cycles for duty cycle
// L: number of clock cycles for non-duty cycle
unsigned long H,L;
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
bool mode1;
bool mode2;
bool mode3;
uint32_t n;
uint8_t i;


int main(void){

  DisableInterrupts();	
	PLL_Init();
	UART0_Init(true,false); // initialize UART
  UART2_Init(false,false);  // initialize UART with no interrupt
  UART3_Init(false,false);  // initialize UART with interrupt
	GPIO_PortF_Init();			  // initialize port F
	hex_number = Dark;
	entered_mode=0x00;
	mode1=false;
	mode2=false;
	mode3=false;
  EnableInterrupts();       // needed for TExaS
	
  while(1){
		Start_Prompt();
		while (!end_of_str) { // wait until the whole string is received.
			WaitForInterrupt();
    }
    end_of_str = false;
    str_idx = 0;
    OutCRLF();
		
		if (string[str_idx] == '1') {
			mode1=true;
			do{
				Mode_One();
			}while(mode1);
		}
		if (string[str_idx] == '2') {
			do{
				Mode_Two();
			}while(entered_mode==0x02);
		}
		if (string[str_idx] == '3') {
			Mode_Three();
		}
  }
}

uint32_t Str_to_UDec(uint8_t str[]){
    uint32_t number=0;;
    uint8_t character,idx=0;
    character = str[idx];
    while(character != NULL){
        if((character>='0') && (character<='9')) {
            number = 10*number+(character-'0'); // this line overflows if above 2^32-1
        }
        else { // none decimal digit fond, stop converting and return previous digits
            return number;
        }
        character = str[++idx]; // get the next digit
    }
    return number;
}
void UDec_to_Str(uint8_t str[], uint32_t n){
    static uint8_t idx=0;
    if ((n/10)!=0) {
        UDec_to_Str(str,n/10);
        UDec_to_Str(str,n%10);
    }
    else {
        str[idx++] = n%10+'0';
        return;
    }

    str[idx] = '\0';
    return;
}

void GPIO_PortF_Init(void)
{
//	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // (a) activate clock for port F
//	while ((SYSCTL_RCGC2_R &= SYSCTL_RCGC2_GPIOF)==0){};
//		
//  GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY; // 2) unlock PortF PF0  
//  GPIO_PORTF_CR_R |= 0x1F;           // allow changes to PF4,1,0       
//  GPIO_PORTF_DIR_R |= 0x09;          // (c) make RED LED output
//  GPIO_PORTF_DIR_R &= ~0x11;
//  GPIO_PORTF_AFSEL_R &= ~0x13;       //     disable alt funct 
//  GPIO_PORTF_DEN_R |= 0x13;          //     enable digital   
//  GPIO_PORTF_PCTL_R &= ~0x000F00FF; // configure as GPIO
//  GPIO_PORTF_AMSEL_R &= ~0x13;       //     disable analog functionality on PF
//  GPIO_PORTF_PUR_R |= 0x11;     //     enable weak pull-up on PF0,4
//		
//  GPIO_PORTF_IS_R &= ~0x11;     // (d) PF0,4 is edge-sensitive
//  GPIO_PORTF_IBE_R &= ~0x11;    //     PF0,4 is not both edges
//  GPIO_PORTF_IEV_R |= 0x11;    //     PF4 rising edge event
//  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag4
//  GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4
//  NVIC_PRI7_R = (NVIC_PRI7_R&~0x00E00000)|0x00C00000; // (g) priority 6
//  NVIC_EN0_R |= NVIC_EN0_PORTF;      // (h) enable interrupt 30 in NVIC

    // Enable clock for Port F
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    // Wait for Port F to be ready
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0);
		H = L = PERIOD/2;            // 50% duty cycle, assume system clock 16MHz, 

    // Unlock GPIO Port F
    GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
    GPIO_PORTF_CR_R |= 0x01;  // Allow changes to PF0

    // Configure PF0 and PF4 as inputs (SW1 and SW2)
    GPIO_PORTF_DIR_R &= ~0x11; // PF0 and PF4 input
    GPIO_PORTF_PUR_R |= 0x11;  // Enable pull-up resistors for PF0 and PF4
    GPIO_PORTF_DEN_R |= 0x11;  // Enable digital I/O on PF0 and PF4
		GPIO_PORTF_IS_R &= ~0x11;     // (d) PF0,4 is edge-sensitive
		GPIO_PORTF_IBE_R &= ~0x11;    //     PF0,4 is not both edges
		GPIO_PORTF_IEV_R |= 0x11;    //     PF4 rising edge event
		GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag4
		GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4
		NVIC_PRI7_R = (NVIC_PRI7_R&~0x00E00000)|0x00C00000; // (g) priority 6
		NVIC_EN0_R |= NVIC_EN0_PORTF;      // (h) enable interrupt 30 in NVIC
    // Configure PF1, PF2, PF3 as outputs (RGB LEDs)
    GPIO_PORTF_DIR_R |= 0x0E;  // PF1, PF2, PF3 output
    GPIO_PORTF_DEN_R |= 0x0E;  // Enable digital I/O on PF1, PF2, PF3
    GPIO_PORTF_AFSEL_R &= ~0x0E; // Disable alternate functions on PF1, PF2, PF3

    // Set initial values for RGB LEDs (all off)
    GPIO_PORTF_DATA_R &= ~0x0E; // Clear PF1, PF2, PF3

		NVIC_ST_CTRL_R = 0;           // disable SysTick during setup
		NVIC_ST_RELOAD_R = L-1;       // reload value for 50% duty cycle
		NVIC_ST_CURRENT_R = 0;        // any write to current clears it
		//NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x1FFFFFFF)|0x40000000; // bit 31-29 for SysTick, set priority to 2
		NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE|NVIC_ST_CTRL_INTEN|NVIC_ST_CTRL_CLK_SRC;  // enable with core clock and interrupts, start systick timer
		
}

// Take care of Rx interrupt and ignore Tx interrupt
void UART0_Handler(void){
    uint8_t chr;
    if(UART0_RIS_R&UART_RIS_RXRIS){ // received one item
        if ((UART0_FR_R&UART_FR_RXFE) == 0) {
            chr = UART0_DR_R&0xFF;
            if (chr==CR){ // reach end of the string
                end_of_str=true;
                string[str_idx]=NULL; // add null terminator to end a C string.
            }
            else if (str_idx<(MAX_STR_LEN-1)){ // save one spot for C null terminator '\0'.
                if (chr==BS) {
                    UART_OutChar(BS);
                    str_idx--;
                }
                else {
                    string[str_idx++]=chr; // add the latest received symbol to end a C string.
                    UART_OutChar(chr);
                }
            }
        }
        UART0_ICR_R = UART_ICR_RXIC; // acknowledge RX FIFO
    }
}

void UART3_Handler(void){
  if(UART3_RIS_R&UART_RIS_RXRIS){       // received one item
		if ((UART3_FR_R&UART_FR_RXFE) == 0){
		  char chr = UART3_DR_R & 0xFF;
			if (chr != '\0'){
			UART3_InString(str_text, 20);  // receive acknowledge from MCU2
			waitformessage = false;
			}
		}
    UART3_ICR_R = UART_ICR_RXIC;        // acknowledge RX FIFO
  }
}
void GPIOPortF_Handler(void)
{		
	// simple debouncing code: generate 20ms to 30ms delay
	for (uint32_t time=0;time<80000;time++) {}
	
  if(GPIO_PORTF_RIS_R & SW2) //When SW2 is pressed on Microcontroller 1 or 2, the led will become red.
	{
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
	if(hex_number ==Red){
		UART_OutString((uint8_t *)"Red ");
	}
	if(hex_number ==Blue){
		UART_OutString((uint8_t *)"Blue ");
  }
  if(hex_number ==Green){
		UART_OutString((uint8_t *)"Green ");
	}
	if(hex_number ==Dark){
		UART_OutString((uint8_t *)"Dark ");
	}
	if(hex_number ==Yellow){
		UART_OutString((uint8_t *)"Yellow ");
	}
	if(hex_number ==Cran){
		UART_OutString((uint8_t *)"Cran ");
	}
	if(hex_number ==Purple){
		UART_OutString((uint8_t *)"Purple ");
	}
	if(hex_number ==White){
		UART_OutString((uint8_t *)"White ");
	}
	  OutCRLF();
		}		
	}
	
	if(GPIO_PORTF_RIS_R & SW1)//If I pressed SW2 on microcontroller 1 and not 2, I can press sw1 to make microcontroller 2 red and vice versa.
	{
		GPIO_PORTF_ICR_R = SW1;
		if(mode2){
			UART2_OutChar(LED);
			entered_mode=UART3_InChar();		
		}
		if(mode3){
			mode3=false;
		}
	}
}

void Start_Prompt(void){
   OutCRLF();
	 UART_OutString((uint8_t *)"Welcome to CECS 447 Project 2  - UART");
   OutCRLF();
   OutCRLF();
	 UART_OutString((uint8_t *)"MCU1");
	
	 OutCRLF();
	 OutCRLF();
	 UART_OutString((uint8_t *)"Main Menu");
	 OutCRLF();
	 OutCRLF();
	 UART_OutString((uint8_t *)"	1. PC <-> MCU1 LED Control.");
	 OutCRLF();
	 UART_OutString((uint8_t *)"	2. MCU1 <-> MCU2  Color Wheel");
	 OutCRLF();
	 UART_OutString((uint8_t *)"	3. PC1 <-> MCU1 <-> MCU2 <-> PC2 Chat Room");
	 OutCRLF();
	 UART_OutString((uint8_t *)"Please choose a communication mode");
	 OutCRLF();
   UART_OutString((uint8_t *)"(enter 1 or 2 or 3):	");
}

void MCU_Two_Mode_Two_Command(void){
	 OutCRLF();
	 OutCRLF();
	 UART_OutString((uint8_t *)"Mode 2 MCU2");
	 OutCRLF();
	 OutCRLF();
	 UART_OutString((uint8_t *)"Waiting for color code from MCU2...");
	 while((UART3_FR_R&UART_FR_RXFE) != 0); // wait until the receiving FIFO is not empty
	 check = UART3_InChar();
	 if(check!=0x66){
			LED = check; // busy waiting approach for UART Rx
		  entered_mode=0x02;
	 }
	 else{
			entered_mode=0x00;
			hex_number = Dark;
			LED=Dark;
		}

	 //UART2_OutChar(entered_mode);
	 mode2=false;
}

void Mode_One(void){
        OutCRLF();
        UART_OutString((uint8_t *)"Mode 1 Menu: ");
        OutCRLF();
        UART_OutString((uint8_t *)"Please select an option from the following list (enter 1 or 2 or 3): ");
        OutCRLF();
        UART_OutString((uint8_t *)"1. Choose an LED color. ");
        OutCRLF();
        UART_OutString((uint8_t *)"2. Change the brightness of current LED(s). ");
        OutCRLF();
        UART_OutString((uint8_t *)"3. Exit ");
			   OutCRLF();
        while (!end_of_str) { // wait until the whole string is received.
            WaitForInterrupt();
        }
        end_of_str = false;
        str_idx = 0;
        OutCRLF();
        OutCRLF();


            if (string[str_idx] == '1') {
							UART_OutString((uint8_t *)"Please select a color from the following list: ");
			        OutCRLF();
							UART_OutString((uint8_t *)"d(dark), r(red), g(green), b(blue), y(yellow), c(cran), p(purple), w(white): ");

                while (!end_of_str) { // wait until the whole string is received.
                    WaitForInterrupt();
                }
                end_of_str = false;
                str_idx = 0;
                OutCRLF();
                    switch (string[str_idx]) {
                        case 'R':
                        case 'r':
                            LED = Red;
												    hex_number = Red;
                            UART_OutString((uint8_t *)"Red LED is on. ");
                            break;
                        case 'B':
                        case 'b':
                            LED = Blue;
												    hex_number = Blue;
                            UART_OutString((uint8_t *)"Blue LED is on. ");
                            break;
                        case 'G':
                        case 'g':
                            LED = Green;
												    hex_number = Green;
                            UART_OutString((uint8_t *)"Green LED is on. ");
                            break;
                        case 'D':
                        case 'd':
                            LED = Dark;
												    hex_number = Dark;
                            UART_OutString((uint8_t *)"LED is dark. ");
                            break;
                        case 'Y':
                        case 'y':
                            LED = Yellow;
												    hex_number = Yellow;
                            UART_OutString((uint8_t *)"Yellow LED is on. ");
                            break;
                        case 'C':
                        case 'c':
                            LED = Cran;
												    hex_number = Cran;
                            UART_OutString((uint8_t *)"Cran LED is on. ");
                            break;
                        case 'P':
                        case 'p':
                            LED = Purple;
												    hex_number = Purple;
                            UART_OutString((uint8_t *)"Purple LED is on. ");
                            break;
                        case 'W':
                        case 'w':
                            LED = White;
												    hex_number = White;
                            UART_OutString((uint8_t *)"White LED is on. ");
                            break;
                    }
                    OutCRLF();
                    OutCRLF();
            } else if (string[str_idx] == '2') {
                // Handle option 2
//								if(H>MIN_DUTY) H = H-DUTY_STEP;    // reduce delivered power
//						     L = PERIOD-H; // constant period, variable duty cycle
								OutCRLF();
								UART_OutString((uint8_t *)"Please enter a decimal number from 0 to 100 followed by a return:  ");
								while (!end_of_str) { // wait until the whole string is received.
									WaitForInterrupt();
								}		
								end_of_str = false;
								str_idx = 0;
								OutCRLF();
		
								n = Str_to_UDec(string);
								if(n!=0){
										H = (n*PERIOD)/100;
									  L = PERIOD-H;
								}
								else{
									LED = Dark;
									hex_number = Dark;
								}

								UDec_to_Str(string,n);
								if(hex_number ==Red){
									UART_OutString((uint8_t *)"Red ");
								}
								if(hex_number ==Blue){
									UART_OutString((uint8_t *)"Blue ");
								}
								if(hex_number ==Green){
									UART_OutString((uint8_t *)"Green ");
								}
								if(hex_number ==Dark){
									UART_OutString((uint8_t *)"Dark ");
								}
								if(hex_number ==Yellow){
									UART_OutString((uint8_t *)"Yellow ");
								}
								if(hex_number ==Cran){
									UART_OutString((uint8_t *)"Cran ");
								}
								if(hex_number ==Purple){
									UART_OutString((uint8_t *)"Purple ");
								}
								if(hex_number ==White){
									UART_OutString((uint8_t *)"White ");
								}
								UART_OutString((uint8_t *)"displayed at "); 
								UART_OutString(string);
								UART_OutString((uint8_t *)"% brightness."); 
								
								OutCRLF();
            } else if (string[str_idx] == '3') {
							  LED = Dark;
							  hex_number = Dark;
							  mode1=false;
							  string[str_idx]='\0';
            }
}

void Mode_Two(void){
	UART2_OutChar(0x02);
	OutCRLF();
//Mode 2 MCU1: press ^ to exit this mode
//In color Wheel State.
//Please press sw2 to go through the colors in the following color wheel: Dark, Red, Green, Blue, Yellow, Cran, Purple, White.
//Once a color is selected, press sw1 to send the color to MCU2.
	UART_OutString((uint8_t *)"Mode 2 MCU1: press ^ to exit this mode  ");
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
	if(hex_number ==Red){
		UART_OutString((uint8_t *)"Red ");
	}
	if(hex_number ==Blue){
		UART_OutString((uint8_t *)"Blue ");
  }
  if(hex_number ==Green){
		UART_OutString((uint8_t *)"Green ");
	}
	if(hex_number ==Dark){
		UART_OutString((uint8_t *)"Dark ");
	}
	if(hex_number ==Yellow){
		UART_OutString((uint8_t *)"Yellow ");
	}
	if(hex_number ==Cran){
		UART_OutString((uint8_t *)"Cran ");
	}
	if(hex_number ==Purple){
		UART_OutString((uint8_t *)"Purple ");
	}
	if(hex_number ==White){
		UART_OutString((uint8_t *)"White ");
	}
	OutCRLF();

	
	do{
		mode2 = true;
		while (!end_of_str) { // wait until the whole string is received.
			if(entered_mode==0x03){
				MCU_Two_Mode_Two_Command();
				break;
			}  
			WaitForInterrupt();
    }
  end_of_str = false;
  str_idx = 0;
	if(string[str_idx] == '^'){
		mode2=false;
		entered_mode=0x00;
		UART2_OutChar(0x66);
		LED=Dark;
		hex_number = Dark;
	}
  }while(mode2);
	OutCRLF();
}

void Mode_Three(void){
	UART2_OutChar(0x05);
	mode3=true;
	waitformessage = true;
	unsigned char prompt[]="Please input a string:\n\r";
	unsigned char sent_str[]="Sent message!\n\r";
  OutCRLF();
  OutCRLF();
	UART_OutString((uint8_t *)"Mode 3 MCU1: Chat Room");
  OutCRLF();
	UART_OutString((uint8_t *)"Press sw1 at any time to exit the chat room.");
  OutCRLF();
	UART_OutString((uint8_t *)"Please type a message end with a return");
  OutCRLF();
	UART_OutString((uint8_t *)"(less than 20 characters):");
  OutCRLF();
	
	while(1){
		OutCRLF();
		UART_InString(str_text, 20); // receive a string from PC, user is typing
		
		UART2_OutString(str_text);  // sent string to MCU2
		UART2_OutChar(CR);
//		while(waitformessage){}; //Wait until it recieves messages from MCU2
//		waitformessage = true;
		UART3_InString(str_text, 20);  // receive acknowledge from MCU2
		OutCRLF();
		UART_OutString(str_text);  // display the acknowledge in serial terminal
		UART_OutChar(CR);
	}
}



// SysTick ISR:
// 1. Implement timing control for duty cycle and non-duty cycle
// 2. utput a waveform based on current duty cycle
void SysTick_Handler(void){
	NVIC_ST_CTRL_R &= ~NVIC_ST_CTRL_ENABLE;// turn off SysTick to reset reload value
	if(hex_number!=0x00){
		if(GPIO_PORTF_DATA_R&hex_number){   // toggle PF2:previous one is a duty cycle
    GPIO_PORTF_DATA_R &= ~hex_number; // make PF2 low
    NVIC_ST_RELOAD_R = L-1;     // reload value for low phase
		} else{ // come from non-duty, going to duty cycle
    GPIO_PORTF_DATA_R |= hex_number;  // make PF2 high
    NVIC_ST_RELOAD_R = H-1;     // reload value for high phase
		}
	}
	NVIC_ST_CURRENT_R = 0;
	NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE; // turn on systick to continue
}
