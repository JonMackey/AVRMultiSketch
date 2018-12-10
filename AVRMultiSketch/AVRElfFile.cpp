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
//  AVRElfFile.cpp
//
//  Created by Jon on 11/2/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#include "AVRElfFile.h"

/******************************** AVRElfFile **********************************/
AVRElfFile::AVRElfFile(void)
{
}

/******************************** ~AVRElfFile *********************************/
AVRElfFile::~AVRElfFile(void)
{
}

#if 0
/****************************** GetAVRDeviceInfo ******************************/
const SAVRDeviceInfo* AVRElfFile::GetAVRDeviceInfo(void) const
{
	return((const SAVRDeviceInfo*)&mContent[mSectEntry[eNoteGnuAvrDeviceInfo]->offset]);
}
#endif

/***************************** JmpInstructionFor ******************************/
uint32_t AVRElfFile::JmpInstructionFor(
	uint32_t	inAddress)
{
	// The jmp instruction is 940C, with an address mask of 1F1 for the
	// 6 MSB of the address.  The instruction is 32 bits.
	// The instruction is stored as two 16 bit values so swapping needs
	// to take place to create a 32 bit value.
	// Addresses are even, bit 0 is removed (shift << 15, >> 17 rather than 16)

	uint32_t	addressL = inAddress << 15;
	uint32_t	jmpInst = 0x940C + (addressL & 0xFFFF0000);
	uint16_t	addressH = inAddress >> 17;
	if (addressH)
	{
		jmpInst |= ((addressH & 0x3E) << 3) + (addressH & 1);
	}
	return(jmpInst);
}

/****************************** GetVectorIndexes ******************************/
/*
*	Returns the set of implemented vector indexes.
*/
bool AVRElfFile::GetVectorIndexes(
	IndexVec&	outVectorIndexes)
{
	bool	success = false;
	if (mContent != NULL)
	{
		// Get the symbol address of __bad_interrupt
		const SSymbolTblEntry*	symTableEntry = NULL;
		uint8_t*	symbolValuePtr = GetSymbolValuePtr("__bad_interrupt", &symTableEntry);
		uint8_t*	textSectMem = GetTextPtr();
		if (symbolValuePtr && symTableEntry)
		{
			uint32_t	jmpBadInterrupt = JmpInstructionFor(symTableEntry->value);
			// Now jmpBadInterrupt can be used to scan for implemented vectors.
			// (anything that isn't jmpBadInterrupt)
			uint32_t*	vector = (uint32_t*)textSectMem;
			uint32_t vectorIndex = 1;
			// Loop as long as the instruction is jmp
			// (not 100% bulletproof, but close enough)
			for (; (vector[vectorIndex] & 0xFE0E) == 0x940C; vectorIndex++)
			{
				if (vector[vectorIndex] != jmpBadInterrupt)
				{
					//fprintf(stderr, "%d ", vectorIndex);
					outVectorIndexes.Set(vectorIndex, vectorIndex);
				}
			}
			//fprintf(stderr, "\n");
			success = true;
		}
	}
	return(success);
}

/***************************** ReplaceTextAddress *****************************/
/*
*	Replaces opcodes LDS and STS that reference inSymbolName with inNewAddress.
*	When the address is hit in the code, the opcode preceding it is checked to
*	see if it's an LDS or STS opcode.  If it is, the address it's replaced and the
*	next N opcodes are checked for increments of inSymbolName's address, where
*	N is the width of the value pointed to by the address.
*/
uint32_t AVRElfFile::ReplaceAddress(
	const char*	inSymbolName,
	uint16_t	inNewAddress)
{
	uint32_t	numAddressesReplaced = 0;
	const SSymbolTblEntry*	symbolTblEntry = NULL;
	GetSymbolValuePtr(inSymbolName, &symbolTblEntry);
	if (symbolTblEntry)
	{
		SSectEntry*	textSectEntry =	GetSectEntry(eText);
		if (textSectEntry)
		{
			uint16_t	oldAddress = symbolTblEntry->value;
			uint32_t	valueSize = symbolTblEntry->size;
			uint16_t*	textSectPtr = (uint16_t*)&mContent[GetSectEntry(eText)->offset];
			uint16_t*	textSectEnd = &textSectPtr[textSectEntry->size/2];
			for (; textSectPtr < textSectEnd; textSectPtr++)
			{
				if (*textSectPtr != oldAddress)
				{
					continue;
				}
				uint16_t	opcode = textSectPtr[-1] & 0xFE0F;
				switch(opcode)
				{
					case 0x9000:	// LDS
					case 0x9200:	// STS
					{
						*textSectPtr = inNewAddress;
						numAddressesReplaced++;
						for (uint32_t k = 1; k < valueSize; k++)
						{
							textSectPtr+=2;
							if (*textSectPtr == oldAddress + k)
							{
								opcode = textSectPtr[-1] & 0xFE0F;
								if (opcode == 0x9000 ||
									opcode == 0x9200)
								{
									*textSectPtr = inNewAddress + k;
									continue;
								}
							}
							break;
						}
						break;
					}
				}
			}
		}
	}
	return(numAddressesReplaced);
}

