/*
*	AVR_HighVoltageSerialProgrammer.ino
*	Jonathan Mackey
*
*	AVR High-voltage Serial Fuse Reprogrammer
*	Adapted from code and design by Paul Willoughby 03/20/2010
*	http://www.rickety.us/2010/03/arduino-avr-high-voltage-serial-programmer/
*
*	See https://arduinodiy.wordpress.com/2015/05/16/high-voltage-programmingunbricking-for-attiny/
*	for a AVR High-voltage Serial Fuse Reprogrammer circuit and more info.
*
*	I took the original work and made it a little more user friendly.
*	The changes I made make it easier to selectively set fuses.  Another change
*	introduces a timeout so that the code doesn't hang waiting for a response.
*
*	To set a fuse, use any of Lhh Hhh Ehh (not case sensitive)
*	To set a default use 0 for the value e.g. L0 H0
*	To simply read the current fuse settings, pass anything other than L,H,or E
*
*	Ex: Passing a space character will return:
*	Signature: 930B = ATtiny85
*	LFuse: 62, HFuse: DF, EFuse: FF
*	No changes, nothing written
*
*	If you then pass h5F it will return: 
*	Signature: 930B = ATtiny85
*	LFuse: 62, HFuse: DF, EFuse: FF
*	Written ->> HFuse: 5F
*	LFuse: 62, HFuse: 5F, EFuse: FF
*	
*	In the example above, setting 5F will enable PB5 to be used as an IO
*	pin rather than the reset pin (i.e. use all 6 IO pins rather than only 5)
*
*
*	Very helpful fuse value calculator:
*	http://www.engbedded.com/fusecalc/
*/

//         __________
// Reset -|          |- VCC
//   SCI -| ATtiny85 |- SDO
//       -|          |- SII
//   GND -|__________|- SDI


#if defined __AVR_ATmega328PB__ || defined __AVR_ATmega328P__
// Pin numbers for AVR High Voltage v1.3 board with ATmega328PB/P.
#define	RESET	(3)		// Output to level shifter for !RESET from MOSFET
#define	SCI		(5)		// Target Clock Input
#define	SDO		(6)		// Target Data Output
#define	SII		(7)		// Target Instruction Input
#define	SDI		(8)		// Target Data Input
#define	VCC		(4)		// Target VCC
#elif defined __AVR_ATmega644P__
// Pin numbers for AVR High Voltage v1.3 board with ATmega644P.
#define	RESET	(11)		// Output to level shifter for !RESET from MOSFET
#define	SCI		(13)		// Target Clock Input
#define	SDO		(14)		// Target Data Output
#define	SII		(15)		// Target Instruction Input
#define	SDI		(18)		// Target Data Input
#define	VCC		(12)		// Target VCC
#endif
#define BAUD_RATE	19200

#define	HFUSE  0x747C
#define	LFUSE  0x646C
#define	EFUSE  0x666E

// ATtiny series signatures
#define  ATTINY13   0x9007  // L: 0x6A, H: 0xFF             8 pin
#define  ATTINY24   0x910B  // L: 0x62, H: 0xDF, E: 0xFF   14 pin
#define  ATTINY25   0x9108  // L: 0x62, H: 0xDF, E: 0xFF    8 pin
#define  ATTINY44   0x9207  // L: 0x62, H: 0xDF, E: 0xFFF  14 pin
#define  ATTINY45   0x9206  // L: 0x62, H: 0xDF, E: 0xFF    8 pin
#define  ATTINY84   0x930C  // L: 0x62, H: 0xDF, E: 0xFFF  14 pin
#define  ATTINY85   0x930B  // L: 0x62, H: 0xDF, E: 0xFF    8 pin
enum ETinyIndex
{
	eATtiny13,
	eATtiny24,
	eATtiny25,
	eATtiny44,
	eATtiny45,
	eATtiny84,
	eATtiny85,
	eATtinyUnknown
};
static const PROGMEM uint16_t	sTinySignatures[] = {ATTINY13, ATTINY24, ATTINY25,
										ATTINY44, ATTINY45, ATTINY84, ATTINY85};
static const PROGMEM uint8_t	sTinySuffixes[] = {13, 24, 25, 44, 45, 84, 85, 0};

void ClearSerialReadBuffer(void);
void EnterHVProgMode(void);
void ExitHVProgMode(void);
ETinyIndex ReadAndPrintSignature(void);
uint8_t GetEFuseDefaultFor(
	ETinyIndex	inTinyIndex);
uint8_t GetHFuseDefaultFor(
	ETinyIndex	inTinyIndex);
uint8_t GetLFuseDefaultFor(
	ETinyIndex	inTinyIndex);
bool ReadAndPrintCalibration(void);
bool ReadAndPrintFuses(
	uint8_t&	outLFuse,
	uint8_t&	outHFuse,
	uint8_t&	outEFuse);
bool ShiftOut(
	uint16_t	inData,
	uint16_t	inInstruction,
	uint8_t*	outBitsRead = NULL);
bool WriteFuse(
	uint16_t	inFuse,
	uint8_t		inValue);

/********************************** setup *************************************/
void setup(void)
{
	pinMode(VCC, OUTPUT);
	pinMode(RESET, OUTPUT);
	pinMode(SDI, OUTPUT);
	pinMode(SII, OUTPUT);
	pinMode(SCI, OUTPUT);
	pinMode(SDO, OUTPUT);     // Configured as input when in programming mode
	digitalWrite(RESET, HIGH);  // Level shifter is inverting, this shuts off 12V
	Serial.begin(BAUD_RATE);
	Serial.println(F("High voltage AVR fuse setter"));
	Serial.println(F("To set a fuse use any of Lhh Hhh Ehh (not case sensitive)"));
	Serial.println(F("To set a default use 0 for the value e.g. L0 H0"));
	Serial.println();
}

/**************************** ReadHexFromSerial *******************************/
uint8_t ReadHexFromSerial(void)
{
	uint8_t	hexVal = 0;
	for (uint8_t i = 2; i > 0 && Serial.available(); i--)
	{
		int thisByte = Serial.peek();
		if (thisByte >= '0' && thisByte <= '9')
		{
			Serial.read();
			hexVal <<= 4;
			hexVal += (thisByte - '0');
		} else if ((thisByte >= 'a' && thisByte <= 'f') ||
			(thisByte >= 'A' && thisByte <= 'F'))
		{
			Serial.read();
			hexVal <<= 4;
			hexVal += ((thisByte | 0x20) - 'a' + 10);
		} else
		{
			break;
		}
	}
	return(hexVal);
}

/*********************************** loop *************************************/
void loop(void)
{
	if (Serial.available() > 0)
	{
		/*
		*	Allow time for the rest of the string to arrive (if any)
		*/
		delay(500);
		uint8_t	lFuseOverride = 0;
		uint8_t	hFuseOverride = 0;
		uint8_t	eFuseOverride = 0;
		uint8_t	lFuse, hFuse, eFuse;
		EnterHVProgMode();
		ETinyIndex tinyIndex = ReadAndPrintSignature();
		if (tinyIndex != eATtinyUnknown &&
			ReadAndPrintCalibration())
		{
			if (!ReadAndPrintFuses(lFuse, hFuse, eFuse))
			{
				ClearSerialReadBuffer();
			}
			while(Serial.available())
			{
				switch(Serial.read())
				{
					// Override current LFuse value
					case 'l':
					case 'L':
						lFuseOverride = ReadHexFromSerial();
						/*
						*	If requesting the default...
						*/
						if (lFuseOverride == 0)
						{
							lFuseOverride = GetLFuseDefaultFor(tinyIndex);
						}
						/*
						*	If the override is the same as the current fuse setting THEN
						*	ignore it.
						*/
						if (lFuseOverride == lFuse)
						{
							lFuseOverride = 0;
						}
						break;
					// Override current HFuse value
					case 'h':
					case 'H':
						hFuseOverride = ReadHexFromSerial();
						/*
						*	If requesting the default...
						*/
						if (hFuseOverride == 0)
						{
							hFuseOverride = GetHFuseDefaultFor(tinyIndex);
						}
						/*
						*	If the override is the same as the current fuse setting THEN
						*	ignore it.
						*/
						if (hFuseOverride == hFuse)
						{
							hFuseOverride = 0;
						}
						break;
					// Override current EFuse value
					case 'e':
					case 'E':
						eFuseOverride = ReadHexFromSerial();
						/*
						*	If attempting to set an EFuse on an ATtiny13 THEN
						*	ignore it.
						*/
						if (tinyIndex == eATtiny13)
						{
							eFuseOverride = 0;
						/*
						*	Else if requesting the default...
						*/
						} else if (eFuseOverride == 0)
						{
							eFuseOverride = GetEFuseDefaultFor(tinyIndex);
						}
						/*
						*	If the override is the same as the current fuse setting THEN
						*	ignore it.
						*/
						if (eFuseOverride == eFuse)
						{
							eFuseOverride = 0;
						}
						break;					
				}
			}
			if (lFuseOverride || hFuseOverride || eFuseOverride)
			{
				bool	success = true;
				Serial.print(F("Written ->> "));
				if (lFuseOverride)
				{
					Serial.print(F("LFuse: "));
					Serial.print(lFuseOverride, HEX);
					success = WriteFuse(LFUSE, lFuseOverride);
				}
				if (success && hFuseOverride)
				{
					if (lFuseOverride)
					{
						Serial.print(F(", "));
					}
					Serial.print(F("HFuse: "));
					Serial.print(hFuseOverride, HEX);
					success = WriteFuse(HFUSE, hFuseOverride);
				}
				if (success && eFuseOverride)
				{
					if (lFuseOverride || hFuseOverride)
					{
						Serial.print(F(", "));
					}
					Serial.print(F("EFuse: "));
					Serial.print(eFuseOverride, HEX);
					success = WriteFuse(EFUSE, eFuseOverride);
				}
				if (success)
				{
					Serial.println();
				} else
				{
					Serial.println(F("<<< Write error"));
				}
				if (!ReadAndPrintFuses(lFuse, hFuse, eFuse))
				{
					ClearSerialReadBuffer();
				}
			} else
			{
				Serial.println(F("No changes, nothing written"));
			}
		} else
		{
			ClearSerialReadBuffer();
		}
		ExitHVProgMode();
		Serial.println();
	}
}

/**************************** ClearSerialReadBuffer ***************************/
void ClearSerialReadBuffer(void)
{
	while(Serial.available())
	{
		Serial.read();
	}
}

/****************************** EnterHVProgMode *******************************/
void EnterHVProgMode(void)
{
	pinMode(SDO, OUTPUT);     // Set SDO to output
	digitalWrite(SDI, LOW);
	digitalWrite(SII, LOW);
	digitalWrite(SDO, LOW);
	digitalWrite(RESET, HIGH);  // 12v Off
	digitalWrite(VCC, HIGH);  // Vcc On
	delayMicroseconds(20);
	digitalWrite(RESET, LOW);   // 12v On
	delayMicroseconds(10);
	pinMode(SDO, INPUT);      // Set SDO to input
	delayMicroseconds(300);
}

/****************************** ExitHVProgMode ********************************/
void ExitHVProgMode(void)
{
	digitalWrite(SCI, LOW);
	digitalWrite(VCC, LOW);		// Vcc Off
	digitalWrite(RESET, HIGH);	// 12v Off
}

/********************************* ShiftOut ***********************************/
bool ShiftOut(
	uint16_t	inData,
	uint16_t	inInstruction,
	uint8_t*	outBitsRead)
{
	uint16_t bitsRead = 0;
	/*
	*	Wait until SDO goes high or timeout, whichever occurs first.
	*	High-voltage Serial Programming Characteristics in the datasheet
	*	states that the typical time for SDO to go valid (high) after read is
	*	16ns, but when writing it's typically 2.5ms
	*/
	uint32_t	timeout = millis() + 5;
	while (!digitalRead(SDO))
	{
		if (timeout > millis())
		{
			continue;
		}
		if (outBitsRead)
		{
			*outBitsRead = 0;
		}
		//Serial.print(F(" ShiftOut error "));
		//Serial.println(millis() - (timeout - 5));
		return(false);
	}
	uint16_t dataOut = inData << 2;
	uint16_t instructionOut = inInstruction << 2;
	for (uint16_t mask = 0x400; mask != 0; mask >>= 1)
	{
		digitalWrite(SDI, !!(dataOut & mask));
		digitalWrite(SII, !!(instructionOut & mask));
		bitsRead <<= 1;
		bitsRead |= digitalRead(SDO);
		digitalWrite(SCI, HIGH);
		digitalWrite(SCI, LOW);
	} 
	
	if (outBitsRead)
	{
		*outBitsRead = bitsRead >> 2;
	}
	return(true);
}

/********************************* WriteFuse **********************************/
bool WriteFuse(
	uint16_t	inFuse,
	uint8_t		inValue)
{
	return(ShiftOut(0x40, 0x4C) &&
			ShiftOut(inValue, 0x2C) &&
			ShiftOut(0x00, (uint8_t) (inFuse >> 8)) &&
			ShiftOut(0x00, (uint8_t) inFuse));
}

/******************************** ReadFuses ***********************************/
bool ReadFuses(
	uint8_t&	outLFuse,
	uint8_t&	outHFuse,
	uint8_t&	outEFuse)
{
	return (ShiftOut(0x04, 0x4C) &&  // LFuse
			ShiftOut(0x00, 0x68) &&
			ShiftOut(0x00, 0x6C, &outLFuse) &&
			ShiftOut(0x04, 0x4C) &&  // HFuse
			ShiftOut(0x00, 0x7A) &&
			ShiftOut(0x00, 0x7E, &outHFuse) &&
			ShiftOut(0x04, 0x4C) &&  // EFuse
			ShiftOut(0x00, 0x6A) &&
			ShiftOut(0x00, 0x6E, &outEFuse));
}

/******************************** PrintFuses **********************************/
void PrintFuses(
	uint8_t	inLFuse,
	uint8_t	inHFuse,
	uint8_t	inEFuse)
{
	Serial.print(F("LFuse: "));
	Serial.print(inLFuse, HEX);
	Serial.print(F(", HFuse: "));
	Serial.print(inHFuse, HEX);
	Serial.print(F(", EFuse: "));
	Serial.println(inEFuse, HEX);
}

/***************************** ReadAndPrintFuses ******************************/
bool ReadAndPrintFuses(
	uint8_t&	outLFuse,
	uint8_t&	outHFuse,
	uint8_t&	outEFuse)
{
	bool success = ReadFuses(outLFuse, outHFuse, outEFuse);
	if (success)
	{
		PrintFuses(outLFuse, outHFuse, outEFuse);
	} else
	{
		Serial.println(F("*** Chip read error or no chip found ***"));
	}
	return(success);
}

/****************************** ReadSignature *********************************/
bool ReadSignature(
	uint16_t&	outSignature)
{
	uint16_t sig = 0;
	uint8_t val;
	for (int i = 1; i < 3; i++)
	{
		if (ShiftOut(0x08, 0x4C) &&
			ShiftOut(  i, 0x0C) &&
			ShiftOut(0x00, 0x68) &&
		 	ShiftOut(0x00, 0x6C, &val))
		{
			sig = (sig << 8) + val;
			continue;
		}
		return(false);
	}
	outSignature = sig;
	return(true);
}

/****************************** ReadCalibration *********************************/
bool ReadCalibration(
	uint16_t&	outCalibration)
{
	uint16_t calib = 0;
	uint8_t val;
	if (ShiftOut(0x08, 0x4C) &&
		ShiftOut(0x00, 0x0C) &&
		ShiftOut(0x00, 0x78) &&
		ShiftOut(0x00, 0x7C, &val))
	{
		calib = (calib << 8) + val;
	} else
	{
		return(false);
	}

	outCalibration = calib;
	return(true);
}

/************************** ReadAndPrintCalibration ***************************/
bool ReadAndPrintCalibration(void)
{
	uint16_t calib;
	if (ReadCalibration(calib))
	{
		Serial.print(F("Calibration: 0x"));
		Serial.print(calib, HEX);
		Serial.print(F(" / "));
		Serial.println(calib);
	} else
	{
		Serial.println(F("*** Chip read error or no chip found ***"));
		return(false);
	}
	return(true);
}

/*************************** SignatureToTinyIndex *****************************/
ETinyIndex SignatureToTinyIndex(
	uint16_t	inSignature)
{
	uint8_t	index = 0;
	for (; index < eATtinyUnknown; index++)
	{
		if (pgm_read_word_near(&sTinySignatures[index]) != inSignature)
		{
			continue;
		}
		break;
	}
	return((ETinyIndex)index);
}

/************************** ReadAndPrintSignature *****************************/
ETinyIndex ReadAndPrintSignature(void)
{
	uint16_t sig;
	ETinyIndex	tinyIndex = eATtinyUnknown;
	if (ReadSignature(sig))
	{
		tinyIndex = SignatureToTinyIndex(sig);
		Serial.print(F("Signature: "));
		Serial.print(sig, HEX);
		Serial.print(F(" = ATtiny"));
		if (tinyIndex != eATtinyUnknown)
		{
			Serial.println(pgm_read_byte_near(&sTinySuffixes[tinyIndex]));
		} else
		{
			Serial.println(F("Unknown"));
		}
	} else
	{
		Serial.println(F("*** Chip read error or no chip found ***"));
	}
	return(tinyIndex);
}

/**************************** GetLFuseDefaultFor ******************************/
uint8_t GetLFuseDefaultFor(
	ETinyIndex	inTinyIndex)
{
	return(inTinyIndex != eATtiny13 ? 0x62 : 0x6A);
}

/**************************** GetHFuseDefaultFor ******************************/
uint8_t GetHFuseDefaultFor(
	ETinyIndex	inTinyIndex)
{
	return(inTinyIndex != eATtiny13 ? 0xDF : 0xFF);
}

/**************************** GetEFuseDefaultFor ******************************/
uint8_t GetEFuseDefaultFor(
	ETinyIndex	inTinyIndex)
{
	return(inTinyIndex != eATtiny13 ? 0xFF : 0);
}

