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
//  FileInputBuffer.h
//
//  Copyright © 2018 Jon Mackey. All rights reserved.
//
#pragma once
#ifndef InputBuffer_H
#define InputBuffer_H
#include <map>
#include <stdio.h>
#include <vector>
#include <string>
#include <stdint.h>

class InputBuffer
{
public:
							InputBuffer(void);
	virtual					~InputBuffer(void);
	
	virtual bool			LoadBuffer(void) = 0;
	virtual void			PushMark(void) = 0;
	virtual void			PopMark(
								bool					inGoToMark) = 0;
	virtual bool			IsValid(void) = 0;

	void					StartSubString(void)
								{mSubStringStart = mBufferPtr;}
	bool					SubStringIsEmpty(void)
								{return(mSubStringStart == mBufferPtr);}
	size_t					SubStringLength(void)
								{return(mBufferPtr - mSubStringStart);}
	void					AppendSubString(
								std::string&			ioString);
	uint8_t 				AppendToEndOfLine(
								std::string&			ioString);
	void					AppendBuffer(
								std::string&			ioString);
	void					Seek(
								long					inOffset,
								int						inWhence = SEEK_CUR); // SEEK_SET, SEEK_CUR, or SEEK_END
	InputBuffer&			operator ++ (int)
								{mBufferPtr++; return(*this);}
	InputBuffer&			operator -- (int)
								{mBufferPtr--; return(*this);}
	bool					IsEqual(
								const InputBuffer&		inBuffer) const;
	uint8_t					CurrChar(void);
	uint8_t					NextChar(void);
	bool					NotAtEOB(void) const
								{return(mBufferPtr < mEndBufferPtr);}
	const uint8_t*			GetBufferPtr(void) const
								{return(mBufferPtr);}
	size_t					GetBufferSize(void) const
								{return(mBufferSize);}
	const uint8_t*			GetBuffer(void) const
								{return(mBuffer);}
	size_t					GetLineNumber(void);
	uint8_t					SkipWhitespace(void);
	uint8_t					SkipWhitespaceOnLine(void);
	uint8_t					SkipToNextLine(void);
	uint8_t					SkipWhitespaceAndComments(void);
	uint8_t					SkipWhitespaceAndHashComments(void);
	uint8_t					SkipTillEndOfBlockComment(void);
	uint8_t					FindEndOfToken(void);
	bool					ReadTillChar(
								uint8_t					inChar,
								bool					inIncludeChar,
								std::string&			outString);
	/*uint8_t					ReadNextToken(
								uint8_t					inDelimiterChar,
								bool					inStripQuotes,
								std::string&			outString);*/
	uint8_t					ReadTillNextQuote(
								bool					inIncludeQuote,
								std::string&			outString);
#ifdef __GNUC__
	uint8_t					ReadDirectiveKind(void);
	bool					IsAtDirectiveKind(
								uint8_t					inKind,
								bool					inSkipIfKind = true);
	uint8_t					ReadNextDirectiveToken(
								std::string&			outString);
#endif
	uint8_t					ReadNumber(
								double&					outNumber);
enum EDirectiveKind
{
	eDefine,
	eElif,
	eElse,
	eEndif,
	eError,
	eIfndef,
	eIfdef,
	eIf,
	eImport,
	eInclude,
	eLine,
	ePragma,
	eUndef,
	eUsing,
	eUnknown,
	eNotADirective
};

protected:
	const uint8_t*	mSubStringStart;

	const uint8_t*	mBufferPtr;
	const uint8_t*	mEndBufferPtr;
	const uint8_t*	mBuffer;
	size_t			mBufferSize;
};

typedef std::vector<const unsigned char*> StringInputMarkStack;

class StringInputBuffer : public InputBuffer
{
public:
							StringInputBuffer(
								const std::string&		inString);
	virtual					~StringInputBuffer(void);
	virtual bool			LoadBuffer(void);
	virtual void			PushMark(void);
	virtual void			PopMark(
								bool					inGoToMark);
	virtual bool			IsValid(void)
								{return(mBuffer != NULL);}
protected:
	StringInputMarkStack	mMarkStack;

							StringInputBuffer(void){}	// For FileInputBuffer
};
#ifdef __GNUC__
class FileInputBuffer : public StringInputBuffer
{
public:
							FileInputBuffer(
								const char*				inFilePath);
	virtual					~FileInputBuffer(void);
protected:
	uint8_t* mFileBuffer;
};
#endif

#endif // InputBuffer_H
