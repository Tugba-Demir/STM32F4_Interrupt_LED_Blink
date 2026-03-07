# STM32F4_Interrupt_LED_Blink

Demonstration of EXTI, IRQ, NVIC, and ISR flow with LED toggle on STM32F4.

## Overview
This project shows how an external button press triggers an **external interrupt (EXTI)**, which generates an **IRQ** handled by the **NVIC**, invoking the **ISR (Interrupt Service Routine)** to toggle an LED. It visually demonstrates the interrupt handling flow in a Cortex-M MCU.

**Flow:**  
Button Press → EXTI → IRQ → NVIC → CPU → ISR → LED toggle

## Keywords
GPIO, EXTI, NVIC, ISR, IRQ, Priority

## Usage
1. Connect a button to PA0 and an LED to PD12.  
2. Compile `main.c` in STM32CubeIDE or another compatible IDE.  
3. Flash the MCU and press the button to see the LED toggle.

## Important Note

Before compiling, make sure to set the external oscillator frequency to match the STM32F4 Discovery board:

**File:** `Project > Core > src > system_stm32f4xx.c`  
**Line:** `HSE_VALUE = 8000000;`  

The STM32F4 Discovery board uses an **8 MHz external crystal oscillator**, so this value must be set correctly for proper system clock operation.
