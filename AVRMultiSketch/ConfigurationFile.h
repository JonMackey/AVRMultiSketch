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
//  ConfigurationFile.h
//  AVRMultiSketch
//
//  Created by Jon on 11/19/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#ifndef ConfigurationFile_h
#define ConfigurationFile_h

#include <map>
#include <stdio.h>
#include <vector>
#include <string>
class InputBuffer;
class JSONObject;

class ConfigurationFile
{
public:
							ConfigurationFile(void);
	virtual					~ConfigurationFile(void);
	virtual bool			ReadFile(
								const char*				inPath);
	virtual uint8_t			ReadDelimitedKeyValuesFromString(
								const std::string&		inString,
								uint8_t					inDelimiter = ',');
							// Return the resolved value for inKey
	virtual void			InsertKeyValue(
								const std::string&		inKey,
								const std::string&		inValue);
	virtual bool			ValueForKey(
								const std::string&		inKey,
								std::string&			outValue,
								uint32_t&				ioKeysNotFound);
	virtual bool			RawValueForKey(
								const std::string&		inKey,
								std::string&			outValue);
	void					Apply(
								const ConfigurationFile& inConfigurationFile);
	void					Copy(
								const ConfigurationFile& inConfigurationFile);
	void					Clear(void);
	const JSONObject*		GetRootObject(void) const
								{return(mRootObject);}
protected:
	JSONObject*	mRootObject;

	uint8_t					ReadNextKeyValue(
								InputBuffer&			inInputBuffer);
};

typedef std::map<std::string, std::string> StringMap;

class BoardsConfigFile : public ConfigurationFile
{
public:
							BoardsConfigFile(void);
							BoardsConfigFile(
								const std::string&		inFQBNStr);
	virtual					~BoardsConfigFile(void);
	bool					ReadFile(
								const char*				inPath,
								bool					inDoKeyFiltering);
	virtual uint8_t			ReadDelimitedKeyValuesFromString(
								const std::string&		inString,
								uint8_t					inDelimiter = ',');
	void					SetFQBNFromString(
								const std::string&		inFQBNStr);
	void					ClearFQBN(void);
	const std::string&		GetFQBN(void) const
								{return(mFQBN);}
	const std::string&		GetCoreFQBNPrefix(void) const
								{return(mCoreFQBNPrefix);}
	const std::string&		GetPackage(void) const
								{return(mPackage);}
	const std::string&		GetArchitecture(void) const
								{return(mArchitecture);}
	const std::string&		GetID(void) const
								{return(mID);}
	const StringMap&		GetMenu(void) const
								{return(mMenu);}
	virtual void			InsertKeyValue(
								const std::string&		inKey,
								const std::string&		inValue);
	void					DoKeyFiltering(
								bool					inDoKeyFiltering)
								{mDoKeyFiltering = inDoKeyFiltering;}
	bool					Promote(
								const std::string&		inKeyPrefix);
	void					Copy(
								const BoardsConfigFile& inConfigurationFile);
protected:
	bool			mDoKeyFiltering;
	std::string		mFQBN;
	std::string		mCoreFQBNPrefix;
	std::string		mPackage;
	std::string		mArchitecture;
	std::string		mID;
	StringMap		mMenu;
	static const std::string	kMenuKey;

};

typedef std::map<std::string, BoardsConfigFile*> BoardsConfigFileMap;
class BoardsConfigFiles
{
public:
							BoardsConfigFiles(void);
							~BoardsConfigFiles(void);

	BoardsConfigFile*		GetConfigForFQBN(
								const std::string&		inFQBN) const;
	BoardsConfigFile*		GetPrimaryConfig(void) const
								{return(GetConfigForFQBN(mPrimaryFQBN));}
	void					SetPrimaryFQBN(
								const std::string&		inFQBN);
	void					EraseBoardsConfigFile(
								const std::string&		inFQBN);
	void					AdoptBoardsConfigFile(
								BoardsConfigFile*		inConfigFile); // Takes ownership of inConfigFile.
protected:
	std::string			mPrimaryFQBN;
	BoardsConfigFileMap	mMap;
};

#endif /* ConfigurationFile_h */
