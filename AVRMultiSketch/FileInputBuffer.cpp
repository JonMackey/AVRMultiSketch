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
//  FileInputBuffer.cpp
//
//  Copyright © 2018 Jon Mackey. All rights reserved.
//
#include "FileInputBuffer.h"
#include "math.h"

/******************************* InputBuffer *******************************/
InputBuffer::InputBuffer(void)
	: mBufferPtr(NULL), mEndBufferPtr(NULL),
	  mBuffer(NULL), mBufferSize(0)
{
}

/******************************* ~InputBuffer ******************************/
InputBuffer::~InputBuffer(void)
{
}

/******************************* AppendSubString *******************************/
void InputBuffer::AppendSubString(
	std::string&	ioString)
{
	size_t	subStrLen = mBufferPtr-mSubStringStart;
	if (subStrLen > 0)
	{
		ioString.append((const char*)mSubStringStart, subStrLen);
	}
	mSubStringStart = mBufferPtr;
}

/***************************** AppendToEndOfLine ******************************/
uint8_t InputBuffer::AppendToEndOfLine(
	std::string&	ioString)
{
	StartSubString();
	uint8_t thisChar = *mBufferPtr;
	for (; thisChar; thisChar = NextChar())
	{
		if (thisChar != '\r')
		{
			if (thisChar != '\n')
			{
				continue;
			}
			AppendSubString(ioString);
		} else if (mBufferPtr[1] != '\n')
		{
			continue;
		} else
		{
			AppendSubString(ioString);
			mBufferPtr++;	// Skip the line terminator
		}
		thisChar = NextChar();
		break;
	}
	return(thisChar);
}

/******************************** AppendBuffer ********************************/
void InputBuffer::AppendBuffer(
	std::string&	ioString)
{
	ioString.append((const char*)mBufferPtr, mEndBufferPtr-mBufferPtr);
	mBufferPtr = mEndBufferPtr;
}

/************************************ Seek ************************************/
void InputBuffer::Seek(
	long	inOffset,
	int		inWhence) // SEEK_SET, SEEK_CUR, or SEEK_END
{
	switch(inWhence)
	{
		case SEEK_SET:
			mBufferPtr = &mBuffer[inOffset];
			break;
		case SEEK_CUR:
			mBufferPtr += inOffset;
			break;
		case SEEK_END:
			mBufferPtr = &mEndBufferPtr[inOffset <= 0 ? inOffset : 0];
			break;
	}
}

/********************************** NextChar **********************************/
uint8_t InputBuffer::NextChar(void)
{
	mBufferPtr++;
	return(CurrChar());
}

/********************************** CurrChar **********************************/
uint8_t InputBuffer::CurrChar(void)
{
	if (mBufferPtr < mEndBufferPtr ||
		LoadBuffer())
	{
		return(*mBufferPtr);
	}
	return(0);
}

/******************************* GetLineNumber ********************************/
/*
*	Only valid for InputBuffers that load the entire file (like FileInputBuffer)
*/
size_t InputBuffer::GetLineNumber(void)
{
	size_t	lineNumber = 1;
	const uint8_t*	bufferPtr = mBuffer;

	for (uint8_t thisChar = *(bufferPtr++); bufferPtr < mBufferPtr; thisChar = *(bufferPtr++))
	{
		if (thisChar != 0xA)
		{
			continue;
		}
		lineNumber++;
	}
	return(lineNumber);
}

/****************************** IsEqual ************************************/
/*
*	Binary compare of two buffers
*/
bool InputBuffer::IsEqual(
	const InputBuffer&	inBuffer) const
{
	return(mBufferSize == inBuffer.GetBufferSize() && memcmp(mBuffer, inBuffer.GetBuffer(), mBufferSize) == 0);
}

/****************************** SkipWhitespace ************************************/
uint8_t InputBuffer::SkipWhitespace(void)
{
	uint8_t	thisChar = CurrChar();
	for (; thisChar != 0; thisChar = NextChar())
	{
		if (isspace(thisChar))
		{
			continue;
		}
		break;
	}
	return(thisChar);
}

/**************************** SkipWhitespaceOnLine ****************************/
/*
*	Same as SkipWhitespace except it doesn't treat \n as whitespace
*/
uint8_t InputBuffer::SkipWhitespaceOnLine(void)
{
	uint8_t	thisChar = NextChar();
	for (; thisChar; thisChar = NextChar())
	{
		if (thisChar != 0xA &&
			isspace(thisChar))
		{
			continue;
		}
		break;
	}
	return(thisChar);
}

/************************** SkipWhitespaceAndComments *************************/
uint8_t InputBuffer::SkipWhitespaceAndComments(void)
{
	uint8_t	thisChar = CurrChar();
	while (thisChar)
	{
		if (isspace(thisChar))
		{
			thisChar = NextChar();
			continue;
		} else if (thisChar == '/')
		{
			PushMark();
			thisChar = NextChar();
			if (thisChar == '/')
			{
				thisChar = SkipToNextLine();
				if (thisChar)
				{
					PopMark(false);
					continue;
				}
			} else if (thisChar == '*')
			{
				thisChar = SkipTillEndOfBlockComment();
				if (thisChar)
				{
					PopMark(false);
					continue;
				}
			}
			PopMark(true);
		}
		return(thisChar);
	}
	return(0);
}

/************************* SkipTillEndOfBlockComment **************************/
uint8_t InputBuffer::SkipTillEndOfBlockComment()
{
	uint8_t	thisChar = NextChar();
	PushMark();
	while (thisChar)
	{
		if (thisChar != '*')
		{
			thisChar = NextChar();
			continue;
		}
		thisChar = NextChar();
		if (thisChar == '/')
		{
			PopMark(false);
			return(NextChar());
		}
	}
	PopMark(true);
	return(0);
}

/*********************** SkipWhitespaceAndHashComments ************************/
uint8_t InputBuffer::SkipWhitespaceAndHashComments(void)
{
	uint8_t	thisChar = CurrChar();
	while (thisChar)
	{
		if (isspace(thisChar))
		{
			thisChar = NextChar();
			continue;
		} else if (thisChar == '#')
		{
			thisChar = SkipToNextLine();
			continue;
		}
		break;
	}
	return(thisChar);
}

/******************************* SkipToNextLine *******************************/
uint8_t InputBuffer::SkipToNextLine(void)
{
	uint8_t thisChar = NextChar();
	for (; thisChar; thisChar = NextChar())
	{
		if (thisChar != '\r')
		{
			if (thisChar != '\n')
			{
				continue;
			}
		} else if (mBufferPtr[1] != '\n')
		{
			continue;
		}
		thisChar = NextChar();
		break;
	}
	return(thisChar);
}

/******************************** ReadTillChar ********************************/
bool InputBuffer::ReadTillChar(
	uint8_t			inChar,
	bool			inIncludeChar,
	std::string&	outString)
{
	PushMark();
	while(LoadBuffer())
	{
		StartSubString();
		for (; NotAtEOB(); mBufferPtr++)
		{
			if (CurrChar() != inChar)
			{
				continue;
			}
			if (inIncludeChar)
			{
				mBufferPtr++;
			}
			AppendSubString(outString);
			PopMark(false);
			return(true);
		}
		AppendSubString(outString);
	}
	PopMark(true);
	return(false);
}

#if 0
/******************************* ReadNextToken ********************************/
/*
*	Parses either a quoted token or an unquoted token.
*	Can optionally remove the quotes of the quoted token.
*/
uint8_t InputBuffer::ReadNextToken(
	uint8_t			inDelimiterChar,
	bool			inStripQuotes,
	std::string&	outString)
{
	PushMark();
	uint8_t thisChar = SkipWhitespaceAndComments();
	if (thisChar != 0)
	{
		bool	whitespaceHit = false;
		StartSubString();
		for (; thisChar != 0; thisChar = NextChar())
		{
			if (thisChar != inDelimiterChar)
			{
				if (isspace(thisChar) == false)
				{
					/*
					*	If a character other than the delimiter was hit after whitespace THEN
					*	this is an error, rewind and return.
					*/
					if (whitespaceHit)
					{
						PopMark(true);
						return(0);
					} else if (thisChar == '\"')
					{
						if (inStripQuotes)
						{
							mBufferPtr--;	// Don't include the quote.
							AppendSubString(outString);	// In most cases outString will be empty after calling AppendSubString.
							mBufferPtr++;
						} else
						{
							AppendSubString(outString);
						}
						mBufferPtr++;	// Skip this quote, it's already been handled
						thisChar = ReadTillNextQuote(inStripQuotes==false, outString);
						if (thisChar != 0)
						{
							StartSubString();
							whitespaceHit = true;
							mBufferPtr--;	// Re-read the last character
						} else
						{
							PopMark(true);
							return(0);
						}
					}
					continue;
				} else if (whitespaceHit)
				{
					continue;
				}
				whitespaceHit = true;
				AppendSubString(outString);
				continue;
			} else if (whitespaceHit == false)
			{
				AppendSubString(outString);
			}
			PopMark(false);
			return(thisChar);
		}
		AppendSubString(outString);
	}
	PopMark(true);
	return(0);
}
#endif
/***************************** ReadTillNextQuote ******************************/
uint8_t InputBuffer::ReadTillNextQuote(
	bool			inIncludeQuote,
	std::string&	outString)
{
	PushMark();
	bool skipNext = false;

	uint8_t thisChar = CurrChar();
	StartSubString();
	while (thisChar != 0)
	{
		if (skipNext == false)
		{
			switch(thisChar)
			{
				case '\"':
					if (inIncludeQuote)
					{
						thisChar = NextChar();
					}
					AppendSubString(outString);
					PopMark(false);
					return(inIncludeQuote ? thisChar : NextChar());  // << this is the only valid exit point
				case '\\':
					skipNext = true;
					break;
			#if 0
				case '&':
					if (mEntityMap)
					{
						AppendSubString(outString); // Upto but not including the ampersand
						StartSubString();
						wchar_t	entityChar = ReadTillEndOfEntity();
						if (entityChar)
						{
							if (entityChar < 0x80)
							{
								outString += (char)entityChar;
							} else if (entityChar < 0x800)
							{
								outString += (0xC0 | entityChar >> 6);
								outString += (0x80 | entityChar & 0x3F);
							} else
							{
								outString += (0xE0 | entityChar >> 0xC);
								outString += (0x80 | entityChar >> 6 & 0x3F);
								outString += (0x80 | entityChar & 0x3F);
							}
							thisChar = NextChar();	// Move past the terminating semicolon
							StartSubString();		// Start the substring on the character following the semicolon
							continue;
						}
					}
					break;
			#endif
				case 0xA:	// If we hit the end of the line before hitting the quote, then fail
					PopMark(false);
					fprintf(stderr, "End of line hit before matching quote found\n");
					return(0);
			}
		} else
		{
			skipNext = false;
		}
		thisChar = NextChar();
	}
	AppendSubString(outString);
	PopMark(true);
	fprintf(stderr, "End of file hit before matching quote found\n");
	return(0);
}

/******************************* FindEndOfToken *******************************/
/*
*	This routine skips valid token characters returning the first invalid character OR
*	the end of buffer, whatever is hit first.
*	Valid characters: Any valid C name character or anything within brackets
*/
uint8_t InputBuffer::FindEndOfToken(void)
{
	uint8_t	thisChar = 0;
	for (; NotAtEOB(); mBufferPtr++)
	{
		thisChar = CurrChar();

		if (isalnum(thisChar) ||
			thisChar == '_')
		{
			continue;
		/*
		*	I don't see any need to parse the contents of the brackets
		*	If bracket THEN
		*	skip contents  (I suppose I should check for eol, but why?)
		*/
		} else if (thisChar == '(')
		{
			for (; NotAtEOB(); mBufferPtr++)
			{
				if (CurrChar() == ')')
				{
					thisChar = NextChar();
					break;
				}
			}
		}
		break;
	}
	return(thisChar);
}

#ifdef __GNUC__
static const uint8_t* kDirectives[] = {"\pdefine", "\pelif", "\pelse", "\pendif",
										"\perror", "\pifndef", "\pifdef", "\pif",
										"\pimport", "\pinclude", "\pline", "\ppragma",
										"\pundef", "\pusing"};
/***************************** ReadDirectiveKind ******************************/
/*
*	This routine returns the directive type.  If a directive type was found, the
*	buffer pointer is advanced by the size of the directive string and points to
*	the character immediately following the directive.
*	The buffer pointer is not changed if no directive is found.
*/
uint8_t InputBuffer::ReadDirectiveKind(void)
{
	uint32_t numDirectives = sizeof(kDirectives)/sizeof(uint8_t*);
	if (CurrChar() == '#')
	{
		for (uint32_t i = 0; i < numDirectives; i++)
		{
			const uint8_t*	directiveStr = kDirectives[i];
			fprintf(stderr, "%d \"%s\"\n", (int)directiveStr[0], (const char*)&directiveStr[1]);
			if (strncmp((const char*)mBufferPtr+1, (const char*)&directiveStr[1], directiveStr[0]))
			{
				continue;
			}
			if ((mBufferPtr + directiveStr[0] + 1) < mEndBufferPtr)
			{
				mBufferPtr += (directiveStr[0] + 1);
				return(i);
			}
			break;	// Error past EOB
		}
		return(eUnknown);
	}
	return(eNotADirective);
}

/********************************** IsAtDirectiveKind **********************************/
/*
*	This returns true if the buffer pointer points to a directive of inKind.
*	If inSkipIfKind is true, the buffer is advanced by the size of the directive string
*	and points to the character immediately following the directive.
*/
bool InputBuffer::IsAtDirectiveKind(
	uint8_t		inKind,
	bool		inSkipIfKind)
{
	if (CurrChar() == '#' &&
		inKind < sizeof(kDirectives)/sizeof(uint8_t*))
	{
		const uint8_t*	directiveStr = kDirectives[inKind];
		if (strncmp((const char*)mBufferPtr+1, (const char*)&directiveStr[1], directiveStr[0]) == 0)
		{
			if (inSkipIfKind)
			{
				mBufferPtr += (directiveStr[0] + 1);
			}
			return(true);
		}
	}
	return(false);
}

/**************************** ReadNextDirectiveToken ****************************/
/*
*	This routine limit's its search for a token to the current, [ ToDo: or extended] line.
*/
uint8_t InputBuffer::ReadNextDirectiveToken(
	std::string&	outString)
{
	PushMark();
	bool	gotoMark = true;
	uint8_t thisChar = SkipWhitespaceOnLine();
	if (thisChar != 0)
	{
		StartSubString();
		for (; NotAtEOB(); mBufferPtr++)
		{
			thisChar = CurrChar();

			if (isalnum(thisChar) ||
				thisChar == '_')
			{
				continue;
			} else
			{
				if (isspace(thisChar))
				{
					AppendSubString(outString);
					gotoMark = false;
				} else
				{
					thisChar = 0;
				}
				break;
			}
		}
	}
	
	PopMark(gotoMark);
	return(thisChar);
}
#endif
/********************************** ReadNumber ***********************************/
/*
*	This routine consumes characters till whitespace is hit or the delimiter, whichever
*	occurs first.
*/
uint8_t InputBuffer::ReadNumber(
	double&	outValue)
{
	enum {
		eNagativeSignHit	= 0x01,			// Negative sign hit
		eBosHit				= 0x80,			// Beginning of string hit (past white space)
		eDecimalPtHit		= 0x8000,		// Decimal point hit
		eExponentHit		= 0x100000
	};
	
	bool		isValid = false;
	uint32_t	flags = 0;
	uint64_t	theValue = 0;
	uint32_t	dPlaceHolder = 1;				// decimal place holder
	uint32_t	exponentValue = 0;
	uint32_t	bExponentDiv = false;
	uint8_t		thisChar = 0;

	for (; NotAtEOB(); mBufferPtr++)
	{
		thisChar = CurrChar();
		if (thisChar >= '0' && thisChar <= '9')
		{
			flags |= eBosHit;	// BOS hit
			thisChar -= '0';	// Convert from ASCII to hex
			
			/*
			*  If exponent was hit
			*/
			if (flags & eExponentHit)
			{
				exponentValue = (exponentValue * 10) + thisChar;
			} else
			{
				/*
				*	If the decimal point has been hit
				*/
				if (flags & eDecimalPtHit)
				{
					dPlaceHolder *= 10;
				}
				theValue = (theValue * 10) + thisChar;
			}
		} else if (thisChar == '.')
		{
			/*
			*	If the EOS (unit specifier) has been hit OR
			*	there already is a decimal point THEN
			*	fail
			*/
			if (flags & eDecimalPtHit)
			{
				break;
			}
			flags |= (eDecimalPtHit + eBosHit);	//decimal point + BOS hit
		} else if (thisChar == 'e' || thisChar == 'E')
		{

			/*
			*	Exponent - either 'e+','e-','E+','E-', 'e', 'E'
			*	if encountered signal exponent so that number collector
			*	can start calculating exponent value
			*/
			if (flags & eExponentHit)
			{
				break;
			}

			if (NotAtEOB())
			{	
				thisChar = NextChar();
				if (thisChar == '-')
				{
					bExponentDiv = true;
				} else if (thisChar != '+')
				{
					mBufferPtr--;
				}
			}
			flags |= (eExponentHit + eBosHit);

		} else if (thisChar == '-')
		{
			/*
			*	If the BOS was hit THEN
			*	fail
			*/
			if (flags & eBosHit)
			{
				break;
			}

			flags |= eBosHit + eNagativeSignHit;	//BOS and negative sign hit
		} else
		{
			isValid = (flags & eBosHit) != 0;
			break;
		}
	}
	if (isValid)
	{
		outValue = (double)theValue/dPlaceHolder;
		
		if (flags & eExponentHit)
		{
			double toDiv = pow(10.0,(double)exponentValue);
			if (bExponentDiv)
			{
				outValue = outValue / toDiv;
			} else
			{
				outValue = outValue * toDiv;
			}
		}
		
		if (flags & eNagativeSignHit)outValue = -outValue;
		return(thisChar);
	}
	return(0);
}

/******************************* StringInputBuffer *******************************/
StringInputBuffer::StringInputBuffer(
	const std::string&	inString)
{
	mBufferSize = inString.size();
	mBuffer = (uint8_t*)const_cast<char*>(inString.c_str());
	mSubStringStart = mBufferPtr = mBuffer;
	mEndBufferPtr = &mBufferPtr[mBufferSize];
}

/******************************* ~StringInputBuffer ******************************/
StringInputBuffer::~StringInputBuffer(void)
{
}

/****************************** LoadBuffer ************************************/
bool StringInputBuffer::LoadBuffer(void)
{
	return(mBufferPtr < mEndBufferPtr);
}

/****************************** PushMark ************************************/
void StringInputBuffer::PushMark(void)
{
	mMarkStack.push_back(mBufferPtr);
}

/******************************* PopMark *******************************/
void StringInputBuffer::PopMark(
	bool	inGoToMark)
{
	if (mMarkStack.size())
	{
		if (inGoToMark)
		{
			mBufferPtr = mMarkStack.back();
		}
		mMarkStack.pop_back();
	} else
	{
		fprintf(stderr, "PopMark called on an empty mark stack!\n");
	}
}

#ifdef __GNUC__
/******************************* FileInputBuffer *******************************/
FileInputBuffer::FileInputBuffer(
	const char*	inFilePath)
	: mFileBuffer(NULL)
{
	FILE*	file = fopen(inFilePath, "r");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		fpos_t	fileSize = 0;
		fgetpos(file, &fileSize);
		rewind(file);
		if (fileSize > 0)
		{
			mFileBuffer = new uint8_t[fileSize];
			fread(mFileBuffer, 1, fileSize, file);
			mBufferSize = fileSize;
			mBuffer = mFileBuffer;
			mSubStringStart = mBufferPtr = mBuffer;
			mEndBufferPtr = &mBufferPtr[mBufferSize];
		}
		fclose(file);
	}
}

/******************************* ~FileInputBuffer ******************************/
FileInputBuffer::~FileInputBuffer(void)
{
	if (mFileBuffer)
	{
		delete [] mFileBuffer;
	}
}
#endif
