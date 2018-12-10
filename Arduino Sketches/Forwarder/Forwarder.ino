/*******************************************************************************
	License
	****************************************************************************
	This program is free software; you can redistribute it
	and/or modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation; either version 3 of the License, or
	(at your option) any later version.
 
	This program is distributed in the hope that it will
	be useful, but WITHOUT ANY WARRANTY; without even the
	implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE. See the GNU General Public
	License for more details.
 
	Licence can be viewed at
	http://www.gnu.org/licenses/gpl-3.0.txt

	Please maintain this license information along with authorship
	and copyright notices in any redistribution of this code
*******************************************************************************/
//
//  Forwarder.ino
//  AVRMultiSketch
//
//  Created by Jon on 10/28/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//
#include <avr/pgmspace.h>
#include <avr/io.h>

// volatile is added to keep the optimizer from removing these vars
// currSketchAddress initially points to the Selector.  The Selector will change
// this to whatever sketch is selected.
// Note that the stored value is the actual address/2, as required by the
// ijmp and ret instructions.
volatile static uint16_t	currSketchAddress = 0xBEEF;
void setup(void)
{
	/*
	*	This gets executed whenever there's a reset or restart.  On power-up the
	*	currSketchAddress points to the start of the selector sketch vector table.
	*	After the Forwarder boilerplate initialization code, the restart code
	*	below is executed. The Selector sketch is initialized and briefly takes
	*	over control till its setup function completes.  The Selector sketch's
	*	job is to fill in currSketchAddress, then jump to restart below.
	*/
	asm volatile(
	"restart: \n"
		"cli ; \n" \
		"lds r30, %0 \n"
		"lds r31, %0+1 \n"
		"ijmp \n"
		::"m" (currSketchAddress):"r30","r31"
	);
}

void loop(void)
{
}

// (X=R27:R26, Y=R29:R28 and Z=R31:R30)


/*
*	All of the sub sketches use this sketch's TIMER0_OVF_vect to maintain timing
*	accuracy.  This ISR supports millis() and micros().  AVRMultiSketch patches
*	all of the sub sketches to use the timer global vars defined for this sketch
*	by way of address replacement of LDS instructions.
*/
#if defined(USART_RX_vect)
ISR(USART_RX_vect, ISR_NAKED)
#elif defined(USART0_RX_vect)
	#define USART_RX_vect_num	USART0_RX_vect_num
ISR(USART0_RX_vect, ISR_NAKED)
#elif defined(USART_RXC_vect)
	#define USART_RX_vect_num	USART_RXC_vect_num
ISR(USART_RXC_vect, ISR_NAKED) // ATmega8
#else
  #error "Don't know what the Data Received vector is called for Serial"
#endif
{
	/*
	*	On entry the stack points to the return address of the interrupt.
	*	We want to jump to the sub ISR of the sub sketch and have the
	*	sub ISR return to the address on the stack
	*
	*	In order to call the ISR of the sub sketch without modifying any
	*	registers the jump to the sub ISR is via a ret instruction.
	*	Using 2 push instructions to reserve space on the stack for the sub ISR
	*	address, preserve the registers needed to poke the address of the
	*	sub ISR in place of the placeholder created by the 2 push instructions.
	*
	*	>>> This assumes 2 instruction words/vector, 128K code limit.
	*/
	asm volatile(
		"push r0 ; Increment SP for return address placeholder L \n" \
		"push r0 ; Increment SP for return address placeholder H \n" \
		"push r28 \n"
		"ldi r28, %0 ; \n" 
		// All of the forwarded ISRs enter here.
	"commonISREpilogue: \n"
		"push r29 \n"
		"in r29, __SREG__ \n"
		"push r29 \n"
 		"push r30 \n"
		"push r31 \n"
		"push r1 \n"
		"eor r1, r1 \n"
		// Get the current sub application address (actually a pm word index)
		"lds r30, %1 \n"		// currSketchAddress L
		"lds r31, %1+1 \n"		// currSketchAddress H
 		// Offset Z to point to the Nth interrupt vector entry (jmp xxx) of the
 		// interrupt being handled.
 		// Multiply the ISR vector index by the size of one entry.
 		// 4 bytes per vector table entry, but ret expects a word index
 		// (128K pm range), so the passed index is only doubled.
 		"add r28, r28 \n"
 		// Add the index offset to Z
		"add r30, r28 \n"
 		"adc r31, r1 \n"
 		// Z now points to the Nth interrupt vector entry (jmp xxx) of the
 		// sub sketch's interrupt being handled.
 		//
		// Put the SP into Y
		"in	r28, __SP_L__ \n"
		"in	r29, __SP_H__ \n"
		// Account for the 8 pushes.
		"adiw r28, 8 \n"
		// Y now points to the placeholder"
		// Copy the ISR vector address to the placeholder
		"st Y, r30 \n"
		"st -Y, r31 \n"
		// Restore the registers leaving the ISR vector address for the ret
		// statement to pop off the stack.
		"pop r1 \n"
		"pop r31 \n"
		"pop r30 \n"
		"pop r29 \n"
		"out __SREG__, r29 \n"
		"pop r29 \n"
		"pop r28 \n"
		"ret \n"
		:: "i" (USART_RX_vect_num), "m" (currSketchAddress) \
	);
	

}
#define FORWARD_ISR(vect_num) \
ISR(_VECTOR(vect_num), ISR_NAKED) \
{ \
	asm volatile( \
		"push r0 ; Increment SP for address placeholder L \n" \
		"push r0 ; Increment SP for address placeholder H \n" \
		"push r28 \n" \
		"ldi r28,%0 \n" \
		"rjmp commonISREpilogue \n"  \
		:: "i" (vect_num) \
	); \
}
// >>>>>>> Place FORWARD_ISRs here...
//FORWARD_ISR(19)
