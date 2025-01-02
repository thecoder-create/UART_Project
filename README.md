# UART_Project

This project demonstrates the integration of UART communication and onboard LED control using the TM4C123 launchpad ARM CORTEX M4  microcontroller. It provides three operational modes and enables interaction between multiple microcontrollers via UART.

## Project Overview
The primary goal of this project is to:
- Implement UART communication between two microcontrollers (MCU1 and MCU2).
- Control onboard LEDs based on received UART commands.
- Toggle between three operational modes: Mode 1, Mode 2, and Mode 3.

## Hardware Requirements
- TM4C123G Microcontroller
- Onboard RGB LEDs (PF1, PF2, PF3)
- UART-compatible devices
- Push buttons (SW1, SW2 on Port F)

## Software Setup
Include the following header files in your project:

```c
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "UART.h"
#include "UART2.h"
#include "UART3.h"
#include <stdint.h>
#include <stdbool.h>
```

## Features
- **Mode 1**: Reserved for future functionality.
- **Mode 2**: Enables a color wheel to cycle through LED colors. The selected color can be sent to another microcontroller.
- **Mode 3**: Operates as a chat room between MCU1 and MCU2, allowing string communication.

### Defined Constants
```c
#define LED (*((volatile unsigned long *)0x40025038)) // onboard LEDs: PF3, PF2, PF1 (RGB)
#define WHEELSIZE 8
#define MAX_STR_LEN 50
#define Dark 0x00
#define Red 0x02
#define Blue 0x04
#define Green 0x08
#define Yellow 0x0A
#define Cran 0x0C
#define White 0x0E
#define Purple 0x06
#define SW1 0x10
#define SW2 0x01
#define NVIC_EN0_PORTF 0x40000000
#define PERIOD 160000 // stay on, Change brightness
#define MIN_DUTY PERIOD/10
#define MAX_DUTY PERIOD*0.9
#define DUTY_STEP PERIOD/10
```

### Interrupt Handling
- `SysTick_Handler`: Handles LED toggling.
- `UART0_Handler`: Handles UART0 communication, including string reception and transmission.
- `UART2_Handler`: Handles UART2 communication, including color code acknowledgment.
- `GPIOPortF_Handler`: Handles switch press events for SW1 and SW2.

### Functions
- `Start_Prompt`: Displays the startup prompt.
- `Mode_One`: Reserved for future development.
- `Mode_Two`: Implements the color wheel functionality and communicates selected colors to another microcontroller.
- `Mode_Three`: Implements a chat room interface for string-based communication.

## File Structure
- **Source Files**:
  - `main.c`: Contains the primary application logic.
  - `UART.c` and `UART.h`: UART initialization and helper functions.
  - `PLL.c` and `PLL.h`: PLL configuration for clock setup.
- **Headers**:
  - `tm4c123gh6pm.h`: Device-specific header.

## Usage
1. Flash the provided code to the TM4C123 microcontroller.
2. Connect MCU1 and MCU2 using UART.
3. Interact with the system via switches and observe the LED behavior.

### Mode Selection
- **Mode 2**: Use SW2 to cycle through colors and SW1 to send the selected color.
- **Mode 3**: Send and receive strings between MCU1 and MCU2.


