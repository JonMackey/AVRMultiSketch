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
//  ElfFile.h
//
//  Created by Jon on 11/2/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#ifndef ElfFile_h
#define ElfFile_h

#include <iostream>

struct SElfHeader
{
	uint32_t	magicNumber;	// 0x7f + ELF
	uint8_t		formatType;		// 1 = 32, 2 = 64 bit format
	uint8_t		endianType;		// 1 = little, 2 = big (this code assumes little)
	uint8_t		version;		// 1 = original ELF version
	uint8_t		target;
	uint8_t		abiVersion;
	uint8_t		unused[7];
	uint16_t	objFileType;	// 2 = exec
	uint16_t	isa;			// instruction set architecture, 0x53 = AVR
	uint32_t	version1;		// 1 = original ELF version
	uint32_t	entryPoint;
	uint32_t	progHdrOffset;
	uint32_t	sectHdrOffset;
	uint32_t	flags;
	uint16_t	headerSize;
	uint16_t	progHdrSize;
	uint16_t	numProgEntries;
	uint16_t	sectHdrSize;
	uint16_t	numSectEntries;
	uint16_t	sectEntryNamesIndex;
};

struct SProgEntry
{
	uint32_t	type;
	uint32_t	offset;
	uint32_t	virtualAddr;
	uint32_t	physicalAddr;
	uint32_t	sizeOnFile;
	uint32_t	sizeInMem;
	uint32_t	flags;
	uint32_t	align;
};
struct SSectEntry
{
	uint32_t	nameOffset;
	uint32_t	type;
	uint32_t	flags;
	uint32_t	addrInMem;
	uint32_t	offset;
	uint32_t	size;
	uint32_t	link;
	uint32_t	info;
	uint32_t	addrAlignment;
	uint32_t	entrySize;
};

struct SSymbolTblEntry
{
uint32_t	name;
uint32_t	value;
uint32_t	size;
uint8_t		info;
uint8_t		other;
uint16_t	shndx;
};

// ESectName mirrors kSectName.  Any change made here must be reflected in kSectName
enum ESectName
{
	eBSS,
	eComment,
	eData,
//	eDebugAbbrev,
//	eDebugAranges,
//	eDebugFrame,
//	eDebugInfo,
//	eDebugLine,
//	eDebugLoc,
//	eDebugRanges,
//	eDebugStr,
//	eNoteGnuAvrDeviceInfo,
	eShStringTable,
	eStringTable,
	eSymbolTable,
	eText,
	eNumSectNames,
	eInvalidSect
};

class ElfFile
{
public:
							ElfFile(void);
	virtual					~ElfFile(void);
	bool					ReadFile(
								const char*				inPath);
	bool					WriteFile(
								const char*				inPath);
	SSectEntry*				GetSectEntry(
								ESectName				inESectName)
							{return(mSectEntry[inESectName]);}
	void					FreeMem(void);
	uint8_t*				GetSymbolValuePtr(
								const char*				inSymbolName,
								const SSymbolTblEntry**	outSymTblEntry = NULL);
	uint8_t*				GetTextPtr(void)
								{return((uint8_t*)&mContent[GetSectEntry(eText)->offset]);}
protected:
	uint8_t*	mContent;
	fpos_t		mFileSize;
	SElfHeader*	mHeader;
	SSectEntry*	mSectEntry[eNumSectNames];
	uint16_t	mShndxLkup[eNumSectNames];
	
	virtual bool			HasRequiredSections(void) const;

	static uint32_t			SectionNameToIndex(
								const char*				inSectionName);

};

#endif /* ElfFile_h */
