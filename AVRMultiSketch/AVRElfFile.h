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
//  AVRElfFile.h
//
//  Created by Jon on 11/2/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#ifndef AVRElfFile_h
#define AVRElfFile_h

#include "ElfFile.h"
#include "IndexVec.h"

// The following struct was copied from:
// https://www.eit.lth.se/fileadmin/eit/courses/edi021/Avr-libc-2.0.0/mem_sections.html

#if 0
#define __NOTE_NAME_LEN 4
struct SAVRDeviceInfo
{
	struct
	{
		uint32_t namesz;		  /* = __NOTE_NAME_LEN */
		uint32_t descsz;		  /* =	size of avr_desc */
		uint32_t type;			  /* = 1 - no other AVR note types exist */
		char note_name[__NOTE_NAME_LEN]; /* = "AVR\0" */
	} header;
	struct
	{
		uint32_t flashStart;
		uint32_t flashSize;
		uint32_t sramStart;
		uint32_t sramSize;
		uint32_t eepromStart;
		uint32_t eepromSize;
		uint32_t offsetTableSize;
		uint32_t offsetTable[1];  /* Offset table containing byte offsets into
									  string table that immediately follows it.
									  index 0: Device name byte offset
									*/
		char nulStr;
		char deviceName[1]; /* Standard ELF string table.
										   index 0 : NULL
										   index 1 : Device name
										   index 2 : NULL
										*/
	} avr;
};
#endif
class AVRElfFile : public ElfFile
{
public:
							AVRElfFile(void);
	virtual					~AVRElfFile(void);
//	const SAVRDeviceInfo*	GetAVRDeviceInfo(void) const;
	bool					GetVectorIndexes(
								IndexVec&				outVectorIndexes);
	uint32_t				GetFlashUsed(void) const
							{ return(mSectEntry[eData]->size + mSectEntry[eText]->size); }
	uint32_t				GetDataSize(void) const
							{ return(mSectEntry[eData]->size + mSectEntry[eBSS]->size); }
	static uint32_t			JmpInstructionFor(
								uint32_t				inAddress);
	uint32_t				ReplaceAddress(
								const char*				inSymbolName,
								uint16_t				inNewAddress);
};

#endif /* AVRElfFile_h */
