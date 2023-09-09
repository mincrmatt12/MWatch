/*
 * MWatch RP2040 startup
 * based on SDK examples
 */

#include "pico.h"
#include "pico.h"
#include "hardware/regs/m0plus.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sio.h"
#include "hardware/regs/ssi.h"
#include "hardware/regs/pads_qspi.h"

	.syntax unified
	.arch armv6m
	.cpu cortex-m0plus
	.thumb

	.section .isr_vector, "ax"
	.align 2
	.globl __isr_vector
__isr_vector:
	.word __InitialMSPTop
	.word Reset_Handler
	.word NMI_Handler
	.word HardFault_Handler
	.word 0 // Reserved, should never fire
	.word 0 // Reserved, should never fire
	.word 0 // Reserved, should never fire
	.word 0 // Reserved, should never fire
	.word 0 // Reserved, should never fire
	.word 0 // Reserved, should never fire
	.word 0 // Reserved, should never fire
	.word SVC_Handler
	.word 0 // Reserved, should never fire
	.word 0 // Reserved, should never fire
	.word PendSV_Handler
	.word SysTick_Handler

	// EXTI

	.word TIMER_IRQ_0_Handler   
	.word TIMER_IRQ_1_Handler   
	.word TIMER_IRQ_2_Handler   
	.word TIMER_IRQ_3_Handler   
	.word PWM_IRQ_WRAP_Handler  
	.word USBCTRL_IRQ_Handler   
	.word XIP_IRQ_Handler       
	.word PIO0_IRQ_0_Handler    
	.word PIO0_IRQ_1_Handler    
	.word PIO1_IRQ_0_Handler    
	.word PIO1_IRQ_1_Handler   
	.word DMA_IRQ_0_Handler    
	.word DMA_IRQ_1_Handler    
	.word IO_IRQ_BANK0_Handler 
	.word IO_IRQ_QSPI_Handler  
	.word SIO_IRQ_PROC0_Handler
	.word SIO_IRQ_PROC1_Handler
	.word CLOCKS_IRQ_Handler   
	.word SPI0_IRQ_Handler     
	.word SPI1_IRQ_Handler     
	.word UART0_IRQ_Handler    
	.word UART1_IRQ_Handler    
	.word ADC_IRQ_FIFO_Handler 
	.word I2C0_IRQ_Handler     
	.word I2C1_IRQ_Handler     
	.word RTC_IRQ_Handler      
	
	// SW IRQ

	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0

	.size __isr_vector, . - __isr_vector

	.section ".text"

	.macro EXCP Name
	.weak \Name
	.type \Name, %function
\Name\()_Handler:
	.cfi_startproc
	b .
	.size \Name\()_Handler, . - \Name\()_Handler
	.cfi_endproc
	.endm

	EXCP NMI
	EXCP HardFault
	EXCP SVC
	EXCP PendSV
	EXCP SysTick

    .macro  IRQ handler
    .weak   \handler
    .set    \handler, __unhandled_user_irq
    .endm
	
	IRQ TIMER_IRQ_0_Handler  
    IRQ TIMER_IRQ_1_Handler  
    IRQ TIMER_IRQ_2_Handler  
    IRQ TIMER_IRQ_3_Handler  
    IRQ PWM_IRQ_WRAP_Handler 
    IRQ USBCTRL_IRQ_Handler  
    IRQ XIP_IRQ_Handler      
    IRQ PIO0_IRQ_0_Handler   
    IRQ PIO0_IRQ_1_Handler   
    IRQ PIO1_IRQ_0_Handler   
    IRQ PIO1_IRQ_1_Handler   
    IRQ DMA_IRQ_0_Handler    
    IRQ DMA_IRQ_1_Handler    
    IRQ IO_IRQ_BANK0_Handler 
    IRQ IO_IRQ_QSPI_Handler  
    IRQ SIO_IRQ_PROC0_Handler
    IRQ SIO_IRQ_PROC1_Handler
    IRQ CLOCKS_IRQ_Handler   
    IRQ SPI0_IRQ_Handler     
    IRQ SPI1_IRQ_Handler     
    IRQ UART0_IRQ_Handler    
    IRQ UART1_IRQ_Handler    
    IRQ ADC_IRQ_FIFO_Handler 
    IRQ I2C0_IRQ_Handler     
    IRQ I2C1_IRQ_Handler     
    IRQ RTC_IRQ_Handler      

.global __unhandled_user_irq
.thumb_func
__unhandled_user_irq:
    mrs  r0, ipsr
    subs r0, #16
.global unhandled_user_irq_num_in_r0
unhandled_user_irq_num_in_r0:
    bkpt #0

