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
//  Selector.ino
//  AVRMultiSketch
//
//  Created by Jon on 10/21/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//
/*
*	This sketch selects which sub sketch will be active or "current".
*	The only requirement is that the 3 symbols, all in Flash/PROGMEM space, be
*	defined.  All three of these symbols will be used *after* compilation to
*	set/modify this sketch's elf file prior to linking.  Any modifications made
*	must maintain the basic function of selecting another sketch that apears at
*	the end of setup()
*
*	After modification:
*	- subSketchAddress is an array that will contain the address of each
*		interrupt vector table of each sub sketch.
*	- forwarderCurrSketchAddr will contain the address of the SRAM variable 
*		currSketchAddress in the Forwarder sketch.  It's the responsibility
*		of this sketch to set the value that forwarderCurrSketchAddr points to
*		to one of subSketchAddress.  The initial value in the forwarder sketch
*		points to this sketch's interrupt vector table.
*	- forwarderRestartPlaceholder is the address within the assembly block of
*		the jmp instruction that will be adjusted to jump to reset address
*		within the Forwarder sketch.  This assembly block must be placed at
*		the end of setup, or the end of sketch selection within loop().
*
*	This sketch may be modified provided the conditions noted above are met.
*
*	The sample code below is used on an atmega328PB to select one of three
*	sub sketches.  My AVR programmer board has 3 buttons, one for each sketch.
*	If after reset one of these buttons is held down, the selection is changed
*	to use the corresponding sketch.  If no buttons are held down after reset
*	the previous selection stored in EEPROM is used.
*/
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <EEPROM.h>

/*
*	Unless you're using the same mcu I use, you'll need to define
*	the set of pins (SketchxBtnPin) and the associated register (ButtonReg).
*	The feedback LED section can be ignored/removed.
*/
#ifdef __AVR_ATmega328PB__
#define ButtonReg		PINC
#define Sketch0BtnPin	14	// PINC0	A0
#define Sketch1BtnPin	15	// PINC1	A1
#define Sketch2BtnPin	16	// PINC2	A2
// The following pins are used on my AVR programmer board.  Status pins to show
// what functions are available on the board based on the sketch selected.
// These extra pins are only available on the atmega328PB not the atmega328P
// Note that the pins_arduino.h for the atmega328PB I'm using is kind of screwy:
// PE0,1,2,3 is not D20,21,22,23
// PE0 is D22, PE1 is D23, PE2 is D20, and PE3 is D21
#define	RedSD_Pin		20	// SD Card Red LED
#define GreenSD_Pin		21	// SD Card Green LED
#define RedICSP_Pin		22	// ISCP out, Red LED
#define GreenICSP_Pin	23	// ICSP out, Green LED
#elif defined __AVR_ATmega644P__
#define ButtonReg		PINA
#define Sketch2BtnPin	24	// A0
#define Sketch1BtnPin	25	// A1
#define Sketch0BtnPin	26	// A2

#define	RedSD_Pin		20	// SD Card Red LED
#define GreenSD_Pin		21	// SD Card Green LED
#define RedICSP_Pin		23	// ISCP out, Red LED
#define GreenICSP_Pin	22	// ICSP out, Green LED
#endif


#define SKETCH_INDEX_EEPROM_ADDR	0

volatile const PROGMEM uint16_t	forwarderCurrSketchAddr = 0;
// Reserve space for the address of the interrupt table of each sub sketch
volatile const PROGMEM uint16_t	subSketchAddress[] = {0xBEEF,0xBEEF,0xBEEF};

void setup(void)
{
	pinMode(Sketch0BtnPin, INPUT_PULLUP);
	pinMode(Sketch1BtnPin, INPUT_PULLUP);
	pinMode(Sketch2BtnPin, INPUT_PULLUP);
#if defined __AVR_ATmega328PB__ || defined __AVR_ATmega644P__
	pinMode(GreenSD_Pin, OUTPUT);
	pinMode(RedSD_Pin, OUTPUT);
	pinMode(GreenICSP_Pin, OUTPUT);
	pinMode(RedICSP_Pin, OUTPUT);
#endif
	// Read the previous sketch index from EEPROM
	// Because the initial value of an EEPROM byte is 0xFF, 1 is added to
	// get the initial default of zero.
	uint8_t sketchIndex = EEPROM.read(SKETCH_INDEX_EEPROM_ADDR) + 1;
	uint8_t	buttonPressed = 0xFF;
	switch (ButtonReg & 7) // bits 0:2
	{
		case 3:	// Button 2 pressed
			buttonPressed = 2;
			break;
		case 5:	// Button 1 pressed
			buttonPressed = 1;
			break;
		case 6:	// Button 0 pressed
			buttonPressed = 0;
			break;
	}
	if (buttonPressed < 3)
	{
		if (buttonPressed != sketchIndex)
		{
			sketchIndex = buttonPressed;
			EEPROM.write(SKETCH_INDEX_EEPROM_ADDR, sketchIndex - 1);
		}
	}
#if defined __AVR_ATmega328PB__ || defined __AVR_ATmega644P__
	switch (sketchIndex)
	{
		case 0:	// Arduino as ISP sketch
			// Turn the green ICSP LED
			// (Note the inverse logic, the common pin on the RGB LED is the (+) anode)
			digitalWrite(GreenSD_Pin, 1);
			digitalWrite(RedSD_Pin, 1);
			digitalWrite(RedICSP_Pin, 1);
			break;
		case 1: // AVR High Voltage sketch
			// Turn both red LEDs on
			digitalWrite(GreenICSP_Pin, 1);
			digitalWrite(GreenSD_Pin, 1);
			break;
		case 2:	// Hex copier sketch (copies from SD to ICSP for NOR Flash chips)
			// Turn all LEDs off
			digitalWrite(RedICSP_Pin, 1);
			digitalWrite(GreenICSP_Pin, 1);
			digitalWrite(RedSD_Pin, 1);
			digitalWrite(GreenSD_Pin, 1);
			break;
	}
#endif
	/*
	*	The code below must be maintained as-is.
	*	sketchIndex is a number from 0 to N-1, where N is the number of sub
	*	sketches, not counting the Forwarder and Selector sketches.
	*/
	*(uint16_t*)pgm_read_word_near(&forwarderCurrSketchAddr) = pgm_read_word_near(&subSketchAddress[sketchIndex]);
	asm volatile(
	"forwarderRestartPlaceholder: \n"
		"jmp 0x4444 \n"	// Restart placeholder
	);
}

void loop(void)
{
}
