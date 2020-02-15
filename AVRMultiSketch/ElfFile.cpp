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
//  ElfFile.cpp
//
//  Created by Jon on 11/2/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#include "ElfFile.h"

// kSectName mirrors ESectName.  Any change made here must be reflected in ESectName
// This array is maintained in alphabetical order
static const char*	kSectName[] =	{
									//	"",
										".bss",
										".comment",
										".data",
									//	".debug_abbrev",
									//	".debug_aranges",
									//	".debug_frame",
									//	".debug_info",
									//	".debug_line",
									//	".debug_loc",
									//	".debug_ranges",
									//	".debug_str",
									//	".note.gnu.avr.deviceinfo",	<< not available for all avr devices
										".shstrtab",
										".strtab",
										".symtab",
										".text"
									};

/********************************** ElfFile ***********************************/
ElfFile::ElfFile(void)
	: mContent(NULL)
{
}

/********************************** ~ElfFile **********************************/
ElfFile::~ElfFile(void)
{
	FreeMem();
}

/********************************** FreeMem ***********************************/
void ElfFile::FreeMem(void)
{
	if (mContent)
	{
		delete [] mContent;
		mContent = NULL;
	}
}

/********************************** ReadFile **********************************/
bool ElfFile::ReadFile(
	const char*				inPath)
{
	bool	success = false;

	FreeMem();
	FILE*    file = fopen(inPath, "rb");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		mFileSize = 0;
		fgetpos(file, &mFileSize);
		rewind(file);
		mContent = new uint8_t[mFileSize];
		size_t	bytesRead = fread(mContent, 1, mFileSize, file);
		fclose(file);
		mHeader = (SElfHeader*)mContent;
		if (bytesRead == mFileSize &&
			mHeader->magicNumber == 0x464c457f)
		{
			for (uint32_t j = 0; j < eNumSectNames; j++)
			{
				mShndxLkup[j] = 0;
				mSectEntry[j] = NULL;
			}
			
			SSectEntry*	sectEntry = (SSectEntry*)&mContent[mHeader->sectHdrOffset];
			const char* sectEntryNames = (const char*)&mContent[sectEntry[mHeader->sectEntryNamesIndex].offset];
			uint32_t	numSectEntriesFound = 0;
			for (uint32_t i = 0; i < mHeader->numSectEntries; i++)
			{
				uint32_t	index = SectionNameToIndex(&sectEntryNames[sectEntry[i].nameOffset]);
				if (index < eNumSectNames)
				{
					numSectEntriesFound++;
					mSectEntry[index] = &sectEntry[i];
				}
				if (i < eNumSectNames)
				{
					mShndxLkup[i] = index;
				}
			}
			success = HasRequiredSections();
		}
	}
	if (!success)
	{
		FreeMem();
	}

	return(success);
}

/**************************** HasRequiredSections *****************************/
bool ElfFile::HasRequiredSections(void) const
{
	return(mSectEntry[eText] != NULL &&
		mSectEntry[eStringTable] != NULL &&
		mSectEntry[eSymbolTable] != NULL);
}

/********************************* WriteFile **********************************/
bool ElfFile::WriteFile(
	const char*	inPath)
{
	bool success = false;
	if (mContent)
	{
		FILE*    file = fopen(inPath, "wb");
		if (file)
		{
			fwrite(mContent, 1, mFileSize, file);
			fclose(file);
			success = true;
		}
	}
	return(success);
}

/***************************** GetSymbolValuePtr ******************************/
/*
*	Returns NULL if not found, else it's the address within the elf file of
*	this symbol.
*
*	For ABS values pass a NULL outSymTblEntry to check to see if the symbol
*	was found.  If found the value is the absolute value (no related section).
*/
uint8_t* ElfFile::GetSymbolValuePtr(
	const char*				inSymbolName,
	const SSymbolTblEntry**	outSymTblEntry)
{
	uint8_t*	symbolOffset = NULL;
	
	const SSectEntry*	symTableSectEntry = GetSectEntry(eSymbolTable);
	const char* stringTable = (const char*)&mContent[GetSectEntry(eStringTable)->offset];
	const SSymbolTblEntry*	symbolTable = (const SSymbolTblEntry*)&mContent[symTableSectEntry->offset];
	uint32_t	numSymTableEntries = symTableSectEntry->size/symTableSectEntry->entrySize;
	const SSymbolTblEntry*	symbolTableEnd = &symbolTable[numSymTableEntries];

	for (; symbolTable < symbolTableEnd; symbolTable++)
	{
		//fprintf(stderr, "%s = %d\n", &stringTable[symbolTable->name], symbolTable->shndx);
		if (strcmp(inSymbolName, &stringTable[symbolTable->name]) != 0)
		{
			continue;
		}
		if (symbolTable->shndx < eNumSectNames)
		{
			const SSectEntry*	thisSectEntry = mSectEntry[mShndxLkup[symbolTable->shndx]];
			uint32_t	contentOffset = thisSectEntry->offset + (symbolTable->value - thisSectEntry->addrInMem);
			symbolOffset = &mContent[contentOffset];
		}
		if (outSymTblEntry)
		{
			*outSymTblEntry = symbolTable;
		}
		break;
	}
	return(symbolOffset);
}

/*************************** SectionNameToIndex *******************************/
uint32_t ElfFile::SectionNameToIndex(
	const char*	inSectionName)
{
	int32_t current;
	int32_t leftIndex = 0;
	int32_t rightIndex = eNumSectNames -1;
	while (leftIndex <= rightIndex)
	{
		current = (leftIndex + rightIndex) / 2;
		
		int	cmpResult = strcmp(kSectName[current], inSectionName);
		if (cmpResult == 0)
		{
			return(current);
		} else if (cmpResult > 0)
		{
			rightIndex = current - 1;
		} else
		{
			leftIndex = current + 1;
		}
	}
	return(eInvalidSect);
}
