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
//  JSONElement.cpp
//
//  Copyright © 2018 Jon Mackey. All rights reserved.
//
#include "JSONElement.h"
#include "FileInputBuffer.h"

/******************************** ~JSONObject *********************************/
JSONObject::~JSONObject(void)
{
	JSONElementMap::iterator	itr = mMap.begin();
	JSONElementMap::iterator	itrEnd = mMap.end();

	for (; itr != itrEnd; ++itr)
	{
		delete (*itr).second;
	}
}

/******************************* InsertElement ********************************/
/*
*	Takes ownership of inElement.
*/
void JSONObject::InsertElement(
	const std::string&	inKey,
	IJSONElement*		inElement)
{
	EraseElement(inKey);
	mMap.insert(JSONElementMap::value_type(inKey, inElement));
}

/******************************** EraseElement ********************************/
void JSONObject::EraseElement(
	const std::string&	inKey)
{
	JSONElementMap::iterator	itr = mMap.find(inKey);
	if (itr != mMap.end())
	{
		delete itr->second;
		mMap.erase(itr);
	}
}

/******************************* DetachElement ********************************/
/*
*	Caller takes ownersip of the detached element.
*/
IJSONElement* JSONObject::DetachElement(
	const std::string&	inKey)
{
	IJSONElement* detachedElement = NULL;
	JSONElementMap::iterator	itr = mMap.find(inKey);
	if (itr != mMap.end())
	{
		detachedElement = itr->second;
		mMap.erase(itr);
	}
	return(detachedElement);
}

/******************************* GetElement ***********************************/
IJSONElement* JSONObject::GetElement(
	const std::string&	inKey,
	EElemType			inOfType) const
{
	JSONElementMap::const_iterator itr = mMap.find(inKey);
	return((itr != mMap.end() && (itr->second->GetType() == inOfType || inOfType == eAnyType)) ? itr->second : NULL);
}

/*********************************** Read *************************************/
bool JSONObject::Read(
	InputBuffer&	inInputBuffer)
{
	bool success = false;
	if (inInputBuffer.CurrChar() == '{')
	{
		inInputBuffer.NextChar();	// Skip the object start char
		IJSONElement* token;
		uint8_t	thisChar;
		std::string	key;
		while((thisChar = inInputBuffer.SkipWhitespaceAndComments()) == '"')
		{
			inInputBuffer.NextChar();	// Skip the leading quote
			key.clear();
			// This assumes the key isn't escaped
			if (inInputBuffer.ReadTillNextQuote(false, key))
			{
				thisChar = inInputBuffer.SkipWhitespaceAndComments();
				if (thisChar == ':')
				{
					inInputBuffer.NextChar();	// Skip the colon
					JSONElementMap::iterator	itr;
					if ((token = Create(inInputBuffer)) != NULL)
					{
						InsertElement(key, token);
						thisChar = inInputBuffer.SkipWhitespaceAndComments();
						if (thisChar == ',')
						{
							inInputBuffer.NextChar();	// Skip the delimiter char
							continue;
						}
					}
				}
			}
			break;
		}
		success = thisChar == '}';
		if (success)
		{
			inInputBuffer.NextChar();	// the array end char
		}
	}
	return(success);
}

/*********************************** Copy *************************************/
IJSONElement* JSONObject::Copy(void) const
{
	JSONObject*	objectCopy = new JSONObject;
	JSONElementMap::const_iterator	itr = mMap.begin();
	JSONElementMap::const_iterator	itrEnd = mMap.end();

	for (; itr != itrEnd; itr++)
	{
		objectCopy->InsertElement(itr->first, itr->second->Copy());
	}
	return(objectCopy);
}

/********************************* IsEqual ************************************/
bool JSONObject::IsEqual(
	const IJSONElement*	inElement) const
{
	if (inElement->IsJSONObject() &&
		((const JSONObject*)inElement)->mMap.size() == mMap.size())
	{
		JSONElementMap::const_iterator	itr = mMap.begin();
		JSONElementMap::const_iterator	iItr = ((const JSONObject*)inElement)->mMap.begin();
		JSONElementMap::const_iterator	itrEnd = mMap.end();
		
		for (; itr != itrEnd; itr++, iItr++)
		{
			if (itr->first == iItr->first &&
				itr->second->IsEqual(iItr->second))
			{
				continue;
			}
			return(false);
		}
		return(true);
	}
	return(false);
}

/********************************** Write *************************************/
void JSONObject::Write(
	uint32_t		inTabs,
	std::string&	outString) const
{
	outString += '{';
	JSONElementMap::const_iterator	itr = mMap.begin();
	JSONElementMap::const_iterator	itrEnd = mMap.end();
	if (itr != itrEnd)
	{
		inTabs++;
		while (true)
		{
			outString += '\n';
			outString.append((std::string::size_type)inTabs, '\t');
			outString += '\"';
			// This assumes no escaping is needed for the key
			outString.append(itr->first);
			outString.append("\":");
			itr->second->Write(inTabs, outString);
			++itr;
			if (itr != itrEnd)
			{
				outString += ',';
				continue;
			}
			break;
		}
		inTabs--;
		outString += '\n';
		outString.append((std::string::size_type)inTabs, '\t');
	}
	outString += '}';
}

/********************************** Apply *************************************/
/*
*	Applies inObject to this object by adding any key/values that don't exist
*	and replacing any values that are different.
*/
void JSONObject::Apply(
	const JSONObject*	inObject)
{
	JSONElementMap::const_iterator	itr = inObject->GetMap().begin();
	JSONElementMap::const_iterator	itrEnd = inObject->GetMap().end();
	
	for (; itr != itrEnd; itr++)
	{
		JSONElementMap::iterator	fItr = mMap.find(itr->first);
		if (fItr != mMap.end())
		{
			// I only need apply to work for eObjects within objects, so for
			// any other type it's just a replace if not equal.
			if (itr->second->IsJSONObject() &&
				fItr->second->IsJSONObject())
			{
				((JSONObject*)fItr->second)->Apply((const JSONObject*)itr->second);
			} else if (!fItr->second->IsEqual(itr->second))
			{
				delete fItr->second;
				fItr->second = itr->second->Copy();
			}
		} else
		{
			mMap.insert(JSONElementMap::value_type(itr->first, itr->second->Copy()));
		}
	}
}

/********************************* ~JSONArray *********************************/
JSONArray::~JSONArray(void)
{
	JSONElementVec::iterator	itr = mVec.begin();
	JSONElementVec::iterator	itrEnd = mVec.end();

	for (; itr != itrEnd; ++itr)
	{
		delete (*itr);
	}
}

/******************************* AddElement ***********************************/
/*
*	Takes ownership of inElement.
*/
void JSONArray::AddElement(
	IJSONElement*	inElement)
{
	mVec.push_back(inElement);
}

/******************************* GetNthElement ********************************/
IJSONElement* JSONArray::GetNthElement(
	size_t		inIndex,
	EElemType	inOfType) const
{
	if (inIndex < mVec.size())
	{
		IJSONElement*	element = mVec.at(inIndex);
		if (element->GetType() == inOfType || inOfType == eAnyType)
		{
			return(element);
		}
	}
	return(NULL);
}

/*********************************** Read *************************************/
bool JSONArray::Read(
	InputBuffer&	inInputBuffer)
{
	bool success = false;
	if (inInputBuffer.CurrChar() == '[')
	{
		inInputBuffer.NextChar();	// Skip the array start char
		IJSONElement* token;
		uint8_t	thisChar;
		while((token = Create(inInputBuffer)) != NULL)
		{
			mVec.push_back(token);
			thisChar = inInputBuffer.SkipWhitespaceAndComments();
			if (thisChar == ',')
			{
				inInputBuffer.NextChar();	// Skip the delimiter char
				continue;
			}
			break;
		}
		success = inInputBuffer.CurrChar() == ']';
		if (success)
		{
			inInputBuffer.NextChar();	// the array end char
		}
	}
	return(success);
}

/*********************************** Copy *************************************/
IJSONElement* JSONArray::Copy(void) const
{
	JSONArray*	arrayCopy = new JSONArray;
	JSONElementVec::const_iterator	itr = mVec.begin();
	JSONElementVec::const_iterator	itrEnd = mVec.end();
	
	for (; itr != itrEnd; itr++)
	{
		arrayCopy->AddElement((*itr)->Copy());
	}
	return(arrayCopy);
}

/********************************* IsEqual ************************************/
bool JSONArray::IsEqual(
	const IJSONElement*	inElement) const
{
	if (inElement->IsJSONArray() &&
		((const JSONArray*)inElement)->mVec.size() == mVec.size())
	{
		JSONElementVec::const_iterator	itr = mVec.begin();
		JSONElementVec::const_iterator	iItr = ((const JSONArray*)inElement)->mVec.begin();
		JSONElementVec::const_iterator	itrEnd = mVec.end();
		
		for (; itr != itrEnd; itr++, iItr++)
		{
			if ((*itr)->IsEqual(*iItr))
			{
				continue;
			}
			return(false);
		}
		return(true);
	}
	return(false);
}

/********************************** Write *************************************/
void JSONArray::Write(
	uint32_t		inTabs,
	std::string&	outString) const
{
	outString += '[';
	JSONElementVec::const_iterator	itr = mVec.begin();
	JSONElementVec::const_iterator	itrEnd = mVec.end();
	if (itr != itrEnd)
	{
		inTabs++;
		while (true)
		{
			outString += '\n';
			outString.append((std::string::size_type)inTabs, '\t');
			(*itr)->Write(inTabs, outString);
			++itr;
			if (itr != itrEnd)
			{
				outString += ',';
				continue;
			}
			break;
		}
		inTabs--;
		outString += '\n';
		outString.append((std::string::size_type)inTabs, '\t');
	}
	outString += ']';
}

/********************************* JSONString *********************************/
JSONString::JSONString(
	const std::string&	inString)
	: mString(inString)
{
}

/*********************************** Read *************************************/
/*
*	Does not handle the unicode \uxxxx case
*/
bool JSONString::Read(
	InputBuffer&	inInputBuffer)
{
	if (inInputBuffer.CurrChar() == '"')
	{
		inInputBuffer.NextChar();	// Skip the leading quote
		std::string	escapedStr;
		if (inInputBuffer.ReadTillNextQuote(false, escapedStr) != 0)
		{
			const char*	substringStart = escapedStr.c_str();
			const char*	stringEnd = &substringStart[escapedStr.size()];
			const char*	stringEndMinus1 = &stringEnd[-1];	// So we don't look at the last character
			const char*	thisCharPtr = substringStart;
			
			for (; thisCharPtr < stringEndMinus1; thisCharPtr++)
			{
				if (*thisCharPtr == '\\')
				{
					/*
					*	Only remove a backslash if the next character
					*	is one of the reserved characters
					*/
					if (&thisCharPtr[1] < stringEnd)
					{
						char	charValue;
						switch(thisCharPtr[1])
						{
							case 'b':
								charValue = '\b';
								break; 
							case 'f':
								charValue = '\f';
								break; 
							case 'n':
								charValue = '\n';
								break; 
							case 'r':
								charValue = '\r';
								break; 
							case 't':
								charValue = '\t';
								break; 
							case '"':
								charValue = '"';
								break; 
							case '\\':
								charValue = '\\';
								break;
							case '/':
								charValue = '/';	// JSON allows for escaping this char, I'm not sure why
								break;
							default:
								continue;
						}
						mString.append(substringStart, thisCharPtr-substringStart);
						mString += charValue;
						// Skip the backslash and the escape character
						thisCharPtr++;
						substringStart = &thisCharPtr[1];
					}
				}
			}
			if (substringStart == escapedStr.c_str())
			{
				mString.assign(escapedStr);
			} else
			{
				mString.append(substringStart, stringEnd-substringStart);
			}
			return(true);
		}
	}
	return(false);
}

/*********************************** Copy *************************************/
IJSONElement* JSONString::Copy(void) const
{
	return(new JSONString(mString));
}

/********************************** Write *************************************/
void JSONString::Write(
	uint32_t		inTabs,
	std::string&	outString) const
{
	outString.append("\"");

	const char*	substringStart = mString.c_str();
	const char*	stringEnd = &substringStart[mString.size()];
	const char*	thisCharPtr = substringStart;
	char	escapeChar;
	
	for (; thisCharPtr < stringEnd; thisCharPtr++)
	{
		switch (*thisCharPtr)
		{
			case '\b':
				escapeChar = 'b';
				break;
			case '\f':
				escapeChar = 'f';
				break;
			case '\n':
				escapeChar = 'n';
				break;
			case '\r':
				escapeChar = 'r';
				break;
			case '\t':
				escapeChar = 't';
				break;
			case '"':
				escapeChar = '"';
				break;
			case '\\':
				escapeChar = '\\';
				break;
			default:
				continue;
		}
		outString.append(substringStart, thisCharPtr-substringStart);
		outString += '\\';
		outString += escapeChar;
		substringStart = &thisCharPtr[1];	// Skip the original character
	}
	outString.append(substringStart, stringEnd-substringStart);
	outString.append("\"");
}

/******************************** GetAsInt ************************************/
int JSONString::GetAsInt(void) const
{
	return((int)strtol(mString.c_str(), (char **)NULL, 10));
}

/********************************* JSONNumber *********************************/
JSONNumber::JSONNumber(
	double	inValue)
	: mValue(inValue)
{
}

/*********************************** Read *************************************/
bool JSONNumber::Read(
	InputBuffer&	inInputBuffer)
{
	return(inInputBuffer.ReadNumber(mValue) != 0);
}

/*********************************** Copy *************************************/
IJSONElement* JSONNumber::Copy(void) const
{
	return(new JSONNumber(mValue));
}

/********************************** Write *************************************/
void JSONNumber::Write(
	uint32_t		inTabs,
	std::string&	outString) const
{
	char numBuff[50];
#ifdef __GNUC__
	snprintf(numBuff, 50, "%g", mValue);
#else
	_snprintf_s(numBuff, 50, 49, "%g", mValue);
#endif
	outString.append(numBuff);
}

/******************************** JSONBoolean *********************************/
JSONBoolean::JSONBoolean(
	bool	inValue)
	: mValue(inValue)
{
}

/*********************************** Read *************************************/
bool JSONBoolean::Read(
	InputBuffer&	inInputBuffer)
{
	if (inInputBuffer.CurrChar() == 't')
	{
		if (strncmp((const char*)inInputBuffer.GetBufferPtr(), "true", 4) == 0)
		{
			mValue = true;
			inInputBuffer.Seek(4);
			return(true);
		}
	} else
	{
		if (strncmp((const char*)inInputBuffer.GetBufferPtr(), "false", 5) == 0)
		{
			mValue = false;
			inInputBuffer.Seek(5);
			return(true);
		}
	}
	return(false);
}

/*********************************** Copy *************************************/
IJSONElement* JSONBoolean::Copy(void) const
{
	return(new JSONBoolean(mValue));
}

/********************************** Write *************************************/
void JSONBoolean::Write(
	uint32_t		inTabs,
	std::string&	outString) const
{
	outString.append(mValue ? "true" : "false");
}

/*********************************** Read *************************************/
bool JSONNull::Read(
	InputBuffer&	inInputBuffer)
{
	if (strncmp((const char*)inInputBuffer.GetBufferPtr(), "null", 4) == 0)
	{
		inInputBuffer.Seek(4);
		return(true);
	}
	return(false);
}

/*********************************** Copy *************************************/
IJSONElement* JSONNull::Copy(void) const
{
	return(new JSONNull);
}

/********************************** Write *************************************/
void JSONNull::Write(
	uint32_t		inTabs,
	std::string&	outString) const
{
	outString.append("null");
}

/*********************************** Create ***********************************/
IJSONElement* IJSONElement::Create(
	InputBuffer&	inInputBuffer)
{
	IJSONElement*	element = NULL;
	switch (inInputBuffer.SkipWhitespace())
	{
		case '{':
			element = new JSONObject;
			break;
		case '[':
			element = new JSONArray;
			break;
		case '"':
			element = new JSONString;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
			element = new JSONNumber;
			break;
		case 'f':
		case 't':
			element = new JSONBoolean;
			break;
		case 'n':
			element = new JSONNull;
			break;
	}
	if (element)
	{
		if (!element->Read(inInputBuffer))
		{
			delete element;
			element = NULL;
		}
	}
	return(element);
}

/*********************************** Create ***********************************/
IJSONElement* IJSONElement::Create(
	const std::string&	inString)
{
	IJSONElement*	rootElement = NULL;
	StringInputBuffer	inputBuffer(inString);
	if (inputBuffer.IsValid())
	{
		rootElement = IJSONElement::Create(inputBuffer);
	}
	return(rootElement);
}
