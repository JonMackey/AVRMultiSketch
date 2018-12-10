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
//  JSONElement.h
//
//  Copyright © 2018 Jon Mackey. All rights reserved.
//
#pragma once
#include <map>
#include <vector>
#include <string>
#include <stdint.h>

class InputBuffer;

class IJSONElement
{
public:
	enum EElemType
	{
		eAnyType,
		eObject,
		eArray,
		eString,
		eNumber,
		eBoolean,
		eNull
	};
							IJSONElement(void){}
	virtual					~IJSONElement(void){}
	virtual EElemType		GetType(void) const = 0;
	bool					IsJSONObject(void) const
								{return(GetType() == eObject);}
	bool					IsJSONArray(void) const
								{return(GetType() == eArray);}
	bool					IsJSONString(void) const
								{return(GetType() == eString);}
	bool					IsJSONNumber(void) const
								{return(GetType() == eNumber);}
	bool					IsJSONBoolean(void) const
								{return(GetType() == eBoolean);}
	bool					IsJSONNull(void) const
								{return(GetType() == eNull);}
	static IJSONElement*	Create(
								const std::string&		inString);
	static IJSONElement*	Create(
								InputBuffer&			inInputBuffer);
	virtual IJSONElement*	Copy(void) const = 0;
	virtual bool			IsEqual(
								const IJSONElement*		inElement) const = 0;
	virtual void			Write(
								uint32_t				inTabs,
								std::string&			outString) const = 0;
protected:
	virtual bool			Read(
								InputBuffer&			inInputBuffer) = 0;
};

typedef std::map<std::string, IJSONElement*> JSONElementMap;
typedef std::vector<IJSONElement*> JSONElementVec;

class JSONObject : public IJSONElement
{
public:
							JSONObject(void){}
	virtual					~JSONObject(void);
	virtual EElemType		GetType(void) const
								{return(eObject);}
	JSONElementMap&			GetMap(void)
								{return(mMap);}
	const JSONElementMap&	GetMap(void) const
								{return(mMap);}
	void					InsertElement(
								const std::string&		inKey,
								IJSONElement*			inElement); // Takes ownership of inElement.
	
	void					EraseElement(
								const std::string&		inKey);
	IJSONElement*			DetachElement(
								const std::string&		inKey);
	IJSONElement*			GetElement(
								const std::string&		inKey,
								EElemType				inOfType = eAnyType) const;
	virtual IJSONElement*	Copy(void) const;
	virtual bool			IsEqual(
								const IJSONElement*		inElement) const;
	virtual void			Write(
								uint32_t				inTabs,
								std::string&			outString) const;
	void					Apply(
								const JSONObject*		inObject);
protected:
	JSONElementMap	mMap;

	virtual bool			Read(
								InputBuffer&			inInputBuffer);
};

class JSONArray : public IJSONElement
{
public:
							JSONArray(void){}
	virtual					~JSONArray(void);
	virtual EElemType		GetType(void) const
								{return(eArray);}
	void					AddElement(
								IJSONElement*			inElement); // Takes ownership of inElement.
	IJSONElement*			GetNthElement(
								size_t					inIndex,
								EElemType				inOfType = eAnyType) const;
	JSONElementVec&			GetVec(void)
								{return(mVec);}
	virtual IJSONElement*	Copy(void) const;
	virtual bool			IsEqual(
								const IJSONElement*		inElement) const;
	virtual void			Write(
								uint32_t				inTabs,
								std::string&			outString) const;
protected:
	JSONElementVec	mVec;

	virtual bool			Read(
								InputBuffer&			inInputBuffer);
};

class JSONString : public IJSONElement
{
public:
							JSONString(void){}
							JSONString(
								const std::string&		inString);
	virtual					~JSONString(void){}
	virtual EElemType		GetType(void) const
								{return(eString);}
	std::string&			GetString(void)
								{return(mString);}
	const std::string&		GetString(void) const
								{return(mString);}
	int						GetAsInt(void) const;
	virtual IJSONElement*	Copy(void) const;
	virtual bool			IsEqual(
								const IJSONElement*		inElement) const
								{return(inElement->IsJSONString() &&
								 ((const JSONString*)inElement)->GetString() == mString);}
	virtual void			Write(
								uint32_t				inTabs,
								std::string&			outString) const;
protected:
	std::string	mString;

	virtual bool			Read(
								InputBuffer&			inInputBuffer);
};

class JSONNumber : public IJSONElement
{
public:
							JSONNumber(
								double					inValue = 0);
	virtual					~JSONNumber(void){}
	virtual EElemType		GetType(void) const
								{return(eNumber);}
	double&					GetValue(void)
								{return(mValue);}
	const double&			GetValue(void) const
								{return(mValue);}
	virtual IJSONElement*	Copy(void) const;
	virtual bool			IsEqual(
								const IJSONElement*		inElement) const
								{return(inElement->IsJSONNumber() &&
								 ((const JSONNumber*)inElement)->GetValue() == mValue);}
	virtual void			Write(
								uint32_t				inTabs,
								std::string&			outString) const;
protected:
	double	mValue;

	virtual bool			Read(
								InputBuffer&			inInputBuffer);
};

class JSONBoolean : public IJSONElement
{
public:
							JSONBoolean(
								bool					inValue = false);
	virtual					~JSONBoolean(void){}
	virtual EElemType		GetType(void) const
								{return(eBoolean);}
	bool&					GetValue(void)
								{return(mValue);}
	const bool&				GetValue(void) const
								{return(mValue);}
	virtual IJSONElement*	Copy(void) const;
	virtual bool			IsEqual(
								const IJSONElement*		inElement) const
								{return(inElement->IsJSONBoolean() &&
								 ((const JSONBoolean*)inElement)->GetValue() == mValue);}
	virtual void			Write(
								uint32_t				inTabs,
								std::string&			outString) const;
protected:
	bool	mValue;
	
	virtual bool			Read(
								InputBuffer&			inInputBuffer);
};

class JSONNull : public IJSONElement
{
public:
							JSONNull(void){}
	virtual					~JSONNull(void){}
	virtual EElemType		GetType(void) const
								{return(eNull);}
	virtual IJSONElement*	Copy(void) const;
	virtual bool			IsEqual(
								const IJSONElement*		inElement) const
								{return(inElement->IsJSONNull());}
	virtual void			Write(
								uint32_t				inTabs,
								std::string&			outString) const;
protected:
	virtual bool			Read(
								InputBuffer&			inInputBuffer);
};
