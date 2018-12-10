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
//  ConfigurationFile.cpp
//  The class reads an Arduino configuration file as a keyed tree with strings
// 	at the end of each branch.
//
//  Created by Jon on 11/19/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#include "ConfigurationFile.h"
#include "FileInputBuffer.h"
#include "JSONElement.h"

/***************************** ConfigurationFile ******************************/
ConfigurationFile::ConfigurationFile(void)
	: mRootObject(NULL)
{
	mRootObject = new JSONObject;
}

/**************************** ~ConfigurationFile ******************************/
ConfigurationFile::~ConfigurationFile(void)
{
	delete mRootObject;
}

/*********************************** Clear ************************************/
void ConfigurationFile::Clear(void)
{
	delete mRootObject;
	mRootObject = new JSONObject;
}

/*********************************** Apply ************************************/
/*
*	Applies the contents of inConfigurationFile creating any key/values that
*	don't exist and replacing the values of any that aren't equal.
*/
void ConfigurationFile::Apply(
	const ConfigurationFile&	inConfigurationFile)
{
	mRootObject->Apply(inConfigurationFile.GetRootObject());
}

/************************************ Copy ************************************/
/*
*	Copies the contents of inConfigurationFile to this object.
*/
void ConfigurationFile::Copy(
	const ConfigurationFile&	inConfigurationFile)
{
	delete mRootObject;
	mRootObject = (JSONObject*)inConfigurationFile.GetRootObject()->Copy();
}

/********************************** ReadFile **********************************/
bool ConfigurationFile::ReadFile(
	const char*	 inPath)
{
	FileInputBuffer	inputBuffer(inPath);
	bool	success = inputBuffer.GetBufferSize() != 0;
	if (success)
	{
		while(ReadNextKeyValue(inputBuffer));
	}

	return(success);
}


/******************************* InsertKeyValue *******************************/
/*
*	Inserts a key, potentially creating and/or deleting elements based on the
*	the number of levels represented by the key.
*/
void ConfigurationFile::InsertKeyValue(
	const std::string&	inKey,
	const std::string&	inValue)
{
	JSONObject*		currentObject = (JSONObject*)mRootObject;
	StringInputBuffer	inputBuffer(inKey);
	uint8_t thisChar = inputBuffer.CurrChar();
	if (thisChar)
	{
		std::string	key;

		inputBuffer.StartSubString();
		for (; thisChar; thisChar = inputBuffer.NextChar())
		{
			if (thisChar != '.')
			{
				continue;
			} else
			{
				inputBuffer++;	// Include the Delimiter
				inputBuffer.AppendSubString(key);
				//inputBuffer--;
				JSONObject* keyObject = (JSONObject*)currentObject->GetElement(key);
				if (!keyObject)
				{
					keyObject = new JSONObject;
					currentObject->InsertElement(key, keyObject);
				}
				currentObject = keyObject;
				key.clear();
				inputBuffer.StartSubString();
			}
		}
		inputBuffer.AppendSubString(key);
		currentObject->InsertElement(key, new JSONString(inValue));
	}
}

/********************** ReadDelimitedKeyValuesFromString **********************/
uint8_t ConfigurationFile::ReadDelimitedKeyValuesFromString(
	const std::string&	inString,
	uint8_t				inDelimiter)
{
	StringInputBuffer	inputBuffer(inString);
	uint8_t thisChar = inputBuffer.CurrChar();
	while (thisChar)
	{
		std::string	key;
		std::string value;

		inputBuffer.StartSubString();
		for (; thisChar; thisChar = inputBuffer.NextChar())
		{
			if (thisChar != '=')
			{
				continue;
			} else
			{
				inputBuffer.AppendSubString(key);
				inputBuffer.NextChar();	// Skip the Delimiter
				inputBuffer.ReadTillChar(inDelimiter, false, value);
				thisChar = inputBuffer.NextChar();	// Skip the Delimiter
				if (key.length())
				{
					InsertKeyValue(key, value);
					//fprintf(stderr, "%s=%s\n", key.c_str(), value.c_str());
				}
				break;
			}
		}
	}
	return(thisChar);
}

/****************************** ReadNextKeyValue ******************************/
uint8_t ConfigurationFile::ReadNextKeyValue(
	InputBuffer&	inInputBuffer)
{
	uint8_t thisChar = inInputBuffer.SkipWhitespaceAndHashComments();
	if (thisChar)
	{
		std::string	key;
		std::string value;

		inInputBuffer.StartSubString();
		for (; thisChar; thisChar = inInputBuffer.NextChar())
		{
			if (thisChar != '=')
			{
				continue;
			} else
			{
				inInputBuffer.AppendSubString(key);
				inInputBuffer.NextChar();	// Skip the Delimiter
				thisChar = inInputBuffer.AppendToEndOfLine(value);
				if (key.length())
				{
					InsertKeyValue(key, value);
					//fprintf(stderr, "%s=%s\n", key.c_str(), value.c_str());
				}
				break;
			}
		}
	}
	return(thisChar);
}

/****************************** RawValueForKey ********************************/
bool ConfigurationFile::RawValueForKey(
	const std::string&	inKey,
	std::string&		outValue)
{
	bool	foundKeyValue = false;
	JSONObject*		currentObject = (JSONObject*)mRootObject;
	StringInputBuffer	inputBuffer(inKey);
	uint8_t thisChar = inputBuffer.CurrChar();
	if (thisChar)
	{
		std::string	key;

		inputBuffer.StartSubString();
		for (; thisChar; thisChar = inputBuffer.NextChar())
		{
			if (thisChar != '.')
			{
				continue;
			} else
			{
				inputBuffer++;	// Include the Delimiter
				inputBuffer.AppendSubString(key);
				inputBuffer--;
				currentObject = (JSONObject*)currentObject->GetElement(key, IJSONElement::eObject);
				if (!currentObject)
				{
					break;
				}
				key.clear();
				inputBuffer++;	// Skip the Delimiter
				inputBuffer.StartSubString();
			}
		}
		if (currentObject)
		{
			inputBuffer.AppendSubString(key);
			JSONString* valueStr = (JSONString*)currentObject->GetElement(key, IJSONElement::eString);
			if (valueStr)
			{
				outValue = valueStr->GetString();
				foundKeyValue = true;
			}
		}
	}
	return(foundKeyValue);
}

/******************************** ValueForKey *********************************/
/*
*	Returns true if inKey was found.
*	The value of inKey is appended to ioValue
*	ioKeysNotFound contains the number of unresolved sub keys
*	This is a recursive function, ioValue and ioKeysNotFound should be
*	initialized by the caller.
*/
bool ConfigurationFile::ValueForKey(
	const std::string&		inKey,
	std::string&			ioValue,
	uint32_t&				ioKeysNotFound)
{
	std::string	rawKeyValue;
	bool	foundKeyValue = RawValueForKey(inKey, rawKeyValue);
	if (foundKeyValue)
	{
		const char* uncomposedStrPtr = rawKeyValue.c_str();
		const char*	subStrStart = uncomposedStrPtr;
		long	subStrLen;
		for (char thisChar = *uncomposedStrPtr; thisChar; thisChar = *(++uncomposedStrPtr))
		{
			if (thisChar != '{')
			{
				continue;
			}
			// Copy everything from the start of the sub string to thisChar
			// to ioValue
			subStrLen = uncomposedStrPtr-subStrStart;
			if (subStrLen > 0)
			{
				ioValue.append((const char*)subStrStart, subStrLen);
			}
			thisChar = *(++uncomposedStrPtr);
			subStrStart = uncomposedStrPtr;
			for (; thisChar && thisChar != '}'; thisChar = *(++uncomposedStrPtr)){}
			// uncomposedStrPtr is either pointing to the closing bracket or
			// some formatting error occurred and it's pointing to the null terminator.
			subStrLen = uncomposedStrPtr-subStrStart;
			if (thisChar && subStrLen > 0)
			{
				std::string subKey((const char*)subStrStart, subStrLen);
				if (ValueForKey(subKey, ioValue, ioKeysNotFound))
				{
					subStrStart = uncomposedStrPtr +1; // skip the key delimiter
				} else
				{
					subStrStart--;	// key wasn't found, include it in the returned value
				}
			} else
			{
				ioKeysNotFound++;
				subStrStart--;	// key wasn't found, include it in the returned value
				if (thisChar != 0)
				{
					continue;
				}
				break;	// Else the null terminator was hit, exit loop
			}
		}
		subStrLen = uncomposedStrPtr-subStrStart;
		if (subStrLen > 0)
		{
			ioValue.append((const char*)subStrStart, subStrLen);
		}
	} else
	{
		ioKeysNotFound++;	// ValueForKey is recursive
	}

	return(foundKeyValue);
}

#pragma mark - BoardsConfigFile
const std::string	BoardsConfigFile::kMenuKey("menu");

/****************************** BoardsConfigFile ******************************/
BoardsConfigFile::BoardsConfigFile(void)
	: mDoKeyFiltering(false)
{
}

/****************************** BoardsConfigFile ******************************/
BoardsConfigFile::BoardsConfigFile(
	const std::string&		inFQBNStr)
	: mDoKeyFiltering(false)
{
	SetFQBNFromString(inFQBNStr);
}

/**************************** ~BoardsConfigFile *******************************/
BoardsConfigFile::~BoardsConfigFile(void)
{
}

/********************************** ReadFile **********************************/
bool BoardsConfigFile::ReadFile(
	const char*		inPath,
	bool			inDoKeyFiltering)
{
	mDoKeyFiltering = inDoKeyFiltering;
	bool success = ConfigurationFile::ReadFile(inPath);
	mDoKeyFiltering = false;
	return(success);
}

/********************** ReadDelimitedKeyValuesFromString **********************/
uint8_t BoardsConfigFile::ReadDelimitedKeyValuesFromString(
	const std::string&		inString,
	uint8_t					inDelimiter)
{
	mDoKeyFiltering = false;
	uint8_t retVal = ConfigurationFile::ReadDelimitedKeyValuesFromString(inString, inDelimiter);
	return(retVal);
}

/********************************* ClearFQBN **********************************/
void BoardsConfigFile::ClearFQBN(void)
{
	mFQBN.clear();
	mCoreFQBNPrefix.clear();
	mPackage.clear();
	mArchitecture.clear();
	mID.clear();
	mMenu.clear();
}

/***************************** SetFQBNFromString ******************************/
void BoardsConfigFile::SetFQBNFromString(
	const std::string&	inFQBNStr)
{
	ClearFQBN();
	mFQBN = inFQBNStr;
	mCoreFQBNPrefix.assign("core_");
	StringInputBuffer	inputBuffer(inFQBNStr);
	for (uint32_t i = 0; i < 4; i++)
	{
		switch (i)
		{
			case 0:
				inputBuffer.ReadTillChar(':', false, mPackage);
				inputBuffer++;
				mCoreFQBNPrefix.append(mPackage);
				mCoreFQBNPrefix += '_';
				break;
			case 1:
				inputBuffer.ReadTillChar(':', false, mArchitecture);
				inputBuffer++;
				mCoreFQBNPrefix.append(mArchitecture);
				mCoreFQBNPrefix += '_';
				break;
			case 2:
				inputBuffer.ReadTillChar(':', false, mID);
				inputBuffer++;
				mCoreFQBNPrefix.append(mID);
				mCoreFQBNPrefix += '_';
				break;
			case 3:	// Menu key=values
			{
				std::string	key;
				std::string	value;
				while(inputBuffer.ReadTillChar('=', false, key))
				{
					inputBuffer++;
					bool moreMenuKeys = inputBuffer.ReadTillChar(',', false, value);
					mMenu.insert(StringMap::value_type(key, value));

					mCoreFQBNPrefix.append(key);
					mCoreFQBNPrefix += '_';
					mCoreFQBNPrefix.append(value);
					mCoreFQBNPrefix += '_';
					if (!moreMenuKeys)
					{
						break;
					}
					inputBuffer++;
					value.clear();
					key.clear();
				}
				break;
			}
		}
	}
}

/******************************* InsertKeyValue *******************************/
/*
*	This override determines if the key value is relevant to the FQBN.
*	If it is relevant the key is adjusted to remove the ID and possibly the
*	menu item key components before the super class' InsertKeyValue is called.
*/
void BoardsConfigFile::InsertKeyValue(
	const std::string&	inKey,
	const std::string&	inValue)
{
	// Behave like a regular config file if not filtering
	if (!mDoKeyFiltering)
	{
		ConfigurationFile::InsertKeyValue(inKey, inValue);
	} else
	{
		std::string	key;
		StringInputBuffer	inputBuffer(inKey);
		inputBuffer.ReadTillChar('.', false, key);
		/*
		*	If this key matches the FQBN ID field...
		*/
		if (key == mID)
		{
			inputBuffer++;	// Skip the delimiter
			key.clear();
			bool delimiterFound = inputBuffer.ReadTillChar('.', false, key);
			/*
			*	If this isn't a menu item value (though it may be a sub menu name, which can be ignored)
			*/
			if (!delimiterFound ||
				key != kMenuKey)
			{
				key.assign(inKey, mID.length()+1);
			/*
			*	Else it's a menu item value.
			*	Determine if it's a selected value (per the FQBN)
			*/
			} else
			{
				inputBuffer++;	// Skip the delimiter
				key.clear();
				inputBuffer.ReadTillChar('.', false, key);
				StringMap::const_iterator	itr = mMenu.find(key);
				key.clear();
				/*
				*	If this is a selected menu item...
				*/
				if (itr != mMenu.end())
				{
					inputBuffer++;	// Skip the delimiter
					std::string	subKey;
					/*
					*	If it's a sub value AND
					*	it's one of the selected items THEN
					*	Add the rest of the buffer as the key string (now promoted)
					*	e.g. if inKey is 644.menu.variant.modelP.build.mcu
					*	and the mID is 644, and variant=modelP, the final
					*	inserted key is promoted to build.mcu
					*/
					if (inputBuffer.ReadTillChar('.', false, subKey) &&
						subKey == itr->second)
					{
						inputBuffer++;	// Skip the delimiter
						inputBuffer.AppendBuffer(key);
					}
				}
			}
			if (!key.empty())
			{
				ConfigurationFile::InsertKeyValue(key, inValue);
			}
		}
	}
}

/********************************** Promote ***********************************/
/*
*	Promotes the key prefix.
*	Ex: for the passed key prefix: tools.avrdude.
*	tools.avrdude.cmd.path would become cmd.path
*	On exit there will be no keys with the passed key prefix (they'll all
*	have been promoted).
*	This may end up with an empty key object.  In this example 'tools.' will still
*	exist, and if 'avrdude.' was its only child then 'tools.' will be empty.
*/
bool BoardsConfigFile::Promote(
	const std::string&	inKeyPrefix)
{
	bool	foundKeyValue = false;
	if (inKeyPrefix.end()[-1] == '.')
	{
		JSONObject*		parentObject = NULL;
		JSONObject*		childObject = (JSONObject*)mRootObject;
		StringInputBuffer	inputBuffer(inKeyPrefix);
		uint8_t thisChar = inputBuffer.CurrChar();
		if (thisChar)
		{
			std::string	key;

			inputBuffer.StartSubString();
			for (; thisChar; thisChar = inputBuffer.NextChar())
			{
				if (thisChar != '.')
				{
					continue;
				} else
				{
					key.clear();
					parentObject = childObject;
					inputBuffer++;	// Include the Delimiter
					inputBuffer.AppendSubString(key);
					inputBuffer--;
					childObject = (JSONObject*)parentObject->GetElement(key, IJSONElement::eObject);
					if (!childObject)
					{
						break;
					}
					inputBuffer++;	// Skip the Delimiter
					inputBuffer.StartSubString();
				}
			}
			if (childObject)
			{
				parentObject->DetachElement(key);
				mRootObject->Apply(childObject);
				delete childObject;
			}
		}
	}
	return(foundKeyValue);
}

/************************************ Copy ************************************/
void BoardsConfigFile::Copy(
	const BoardsConfigFile&	inConfigurationFile)
{
	ConfigurationFile::Copy(inConfigurationFile);
	mDoKeyFiltering = inConfigurationFile.mDoKeyFiltering;
	mFQBN.assign(inConfigurationFile.mFQBN);
	mCoreFQBNPrefix.assign(inConfigurationFile.mCoreFQBNPrefix);
	mPackage.assign(inConfigurationFile.mPackage);
	mArchitecture.assign(inConfigurationFile.mArchitecture);
	mID.assign(inConfigurationFile.mID);
	mMenu = inConfigurationFile.mMenu;
}


#pragma mark - BoardsConfigFiles
/***************************** BoardsConfigFiles ******************************/
BoardsConfigFiles::BoardsConfigFiles(void)
{
}

/**************************** ~BoardsConfigFiles ******************************/
BoardsConfigFiles::~BoardsConfigFiles(void)
{
	BoardsConfigFileMap::iterator	itr = mMap.begin();
	BoardsConfigFileMap::iterator	itrEnd = mMap.end();

	for (; itr != itrEnd; ++itr)
	{
		delete (*itr).second;
	}
}

/******************************* SetPrimaryFQBN *******************************/
void BoardsConfigFiles::SetPrimaryFQBN(
	const std::string&	inFQBN)
{
	mPrimaryFQBN.assign(inFQBN);
}

/****************************** GetConfigForFQBN ******************************/
BoardsConfigFile* BoardsConfigFiles::GetConfigForFQBN(
	const std::string&		inFQBN) const
{
	BoardsConfigFileMap::const_iterator itr = mMap.find(inFQBN);
	return(itr != mMap.end() ? itr->second : NULL);
}

/*************************** EraseBoardsConfigFile ****************************/
void BoardsConfigFiles::EraseBoardsConfigFile(
	const std::string&		inFQBN)
{
	BoardsConfigFileMap::iterator	itr = mMap.find(inFQBN);
	if (itr != mMap.end())
	{
		delete itr->second;
		mMap.erase(itr);
	}
}

/**************************** AdoptBoardsConfigFile ***************************/
void BoardsConfigFiles::AdoptBoardsConfigFile(
	BoardsConfigFile*		inConfigFile)
{
	EraseBoardsConfigFile(inConfigFile->GetFQBN());
	mMap.insert(BoardsConfigFileMap::value_type(inConfigFile->GetFQBN(), inConfigFile));
}
