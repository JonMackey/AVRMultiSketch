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
//  IndexVec.h
//
//  Copyright © 2018 Jon Mackey. All rights reserved.
//
#pragma once
#ifndef IndexVec_H
#define IndexVec_H
#include <vector>
#include <string>
#ifndef __GNUC__
#include <cstdint>
#endif
/*
*	Efficiently stores indexes as runs.
*	The IndexVec is initially all 0.
*	The array only records transitions to or from 0 to 1.
*	The last transition is always to 0.  For this reason there will always be an odd number
*	of array offsets when the first run value is 0, and an even number of runs when the
*	first run value is 1.
*	The first run value determines the value of all runs (See GetRunValue below).
*
*	Example: adding indexes 3,4,5,6 and 10,11,12,13 would result in an array of 5.
*	You would call SetRun(3,7,1) and SetRun(10,14,1)
*	Resulting in: 0,3,7,10,14 with the mFirstRunValue set to 0.
*
*	See the source for Union to see how to iterate/make sense of the index array offsets.
*
*	frv -> first run value, either 1 or 0
*	lrv -> last run value, sanity check, should always be 0
*	IndexVec	indexVecA;
*	indexVecA.SetRun(4, 11, 1);
*	indexVecA.SetRun(14, 21, 1);
*	indexVecA.SetRun(24, 31, 1);
*	frv = 0, offsets = 0,4,11,14,21,24,31 lrv = 0	<< indexVecA
*	IndexVec	indexVecB;
*	indexVecB.SetRun(0, 7, 1);
*	indexVecB.SetRun(10, 17, 1);
*	indexVecB.SetRun(20, 27, 1);
*	frv = 1, offsets = 0,7,10,17,20,27 lrv = 0	<< indexVecB
*
*	0123456789012345678901234567890123
*	....1111111...1111111...1111111..	<< indexVecA
*	1111111...1111111...1111111..		<< indexVecB
*
*	indexVecA.Sect(indexVecB); (or indexVecA &= indexVecB;)
*	0123456789012345678901234567890123
*	....111...1...111...1...111......
*	frv = 0, offsets = 0,4,7,10,11,14,17,20,21,24,27 lrv = 0
*
*	indexVecA.Union(indexVecB); (or indexVecA |= indexVecB;)
*	0123456789012345678901234567890123
*	1111111...1111111...1111111........
*	frv = 1, offsets = 0,7,10,17,20,27 lrv = 0
*
*	indexVecA.Diff(indexVecB); (or indexVecA -= indexVecB;)
*	0123456789012345678901234567890123
*	...............................
*	frv = 0, offsets = 0 lrv = 0
*/

typedef std::vector<uint32_t> Runs;
class IndexVec
{
public:
							IndexVec(void);
							IndexVec(
								const IndexVec&			inIndexVec);
							IndexVec(
								const std::string&		inSerializedIndexVec);
							~IndexVec(void){}
							
	void					SetRun(
								uint32_t				inStart,
								uint32_t				inEnd,
								uint8_t					inValue);	// Must be 0 or 1
	/*
	*	Contains is a convenience function that returns the value
	*	at inPosition as a bool.
	*/
	bool					Contains(
								uint32_t				inPosition);
	inline uint8_t			GetRunValue(
								size_t					inRunIndex) const
								{return(mFirstRunValue != (inRunIndex & 1));}
	uint8_t					GetFirstRunValue(void) const
								{return(mFirstRunValue);}
	size_t					GetRunStart(
								size_t					inRunIndex) const;
	size_t					GetRunIndex(
								uint32_t				inPosition,
								size_t					inStartFrom) const;
	size_t					GetIndexNumber(
								size_t					inIndex) const;
	uint32_t				GetNthIndex(
								size_t					inIndexNumber) const;
	const Runs&				GetRuns(void) const
								{return(mRuns);}
							// Get the Max index
	inline uint32_t			GetMax(void) const
								{return(mRuns.back());}
							// Get the Min index
	uint32_t				GetMin(void) const;
	
	/*
	*	Returns the number of indexes
	*/
	size_t					GetCount(void) const;
	/*
	*	Similar to SetRun above only you pass the actual range like 10:12
	*	to mean the indexes 10, 11, and 12.
	*/
	void					Set(
								uint32_t				inFrom,
								uint32_t				inTo);
	void					Clear(
								uint32_t				inFrom,
								uint32_t				inTo);
	/*
	*	Sets/Resets the run vec to all zeros
	*/
	void					Clear(void);
	
	void					Copy(
								const IndexVec&		inIndexVec);
	IndexVec&				operator = (	// Same as Copy
								const IndexVec&		inIndexVec);
	
	/*
	*	Returns true if the vector has no indexes
	*/
	bool					Empty(void) const;

	/*
	*	Determine the common indexes.
	*	Returns false if there are no common indexes (vec is empty)
	*/
	bool					Sect(
								const IndexVec&		inIndexVec);
	IndexVec&				operator &= (	// Same as Sect
								const IndexVec&		inIndexVec);
	void					Union(
								const IndexVec&		inIndexVec);
	IndexVec&				operator += (	// Same as Union
								const IndexVec&		inIndexVec);
	IndexVec&				operator |= (	// Same as Union
								const IndexVec&		inIndexVec);
	/*
	*	Returns false if the result is an empty vec.
	*/
	bool					Diff(
								const IndexVec&		inIndexVec);
	IndexVec&				operator -= (	// Same as Diff
								const IndexVec&		inIndexVec);
	bool					IsEqual(
								const IndexVec&		inIndexVec);
	
	bool					SetFromSerial(
								const std::string&	inSerializedIndexVec);
	const std::string&		Serialize(
								std::string&		outSerializedIndexVec) const;
protected:
	uint8_t	mFirstRunValue;
	Runs	mRuns;
};

class IndexVecIterator
{
public:
	/*
	*	Note that IndexVecIterator takes a pointer to a IndexVec, so you must
	*	ensure that inIndexVec is valid for the life of the iterator.
	*/

							IndexVecIterator(
								const IndexVec*			inIndexVec = NULL,
								bool					inWrap = false);
							~IndexVecIterator(void){}

	/*
	*	Sets the mIndexVec, resets to the first index
	*/
	void					SetIndexVec(
								const IndexVec*			inIndexVec);

	/*
	*	All of the iterator routines that return an index may return
	*	IndexVecIterator::end if the vec is empty OR inWrap is false.
	*/
	size_t					CurrentIndexNumber(void) const;
	size_t					Current(void) const
								{return(mCurrentIndex);}
	/*
	*	Last Current is the index it changed from (last current)
	*	This is used to optimize the update of UI elements so that
	*	only the items that need to be updated are updated (rather than updating everything)
	*/
	size_t					LastCurrent(void) const
								{return(mLastCurrentIndex);}
	size_t					Next(void);
	size_t					Previous(void);
	/*
	*	Will attempt to move to the value of inIndex.  If inIndex is not in the
	*	IndexVec then the closest index will be returned.  The closest index
	*	may be before inIndex.
	*/
	size_t					MoveToValue(
								size_t					inIndex);
	/*
	*	Will attempt to MoveTo Nth index.  If inIndex is past the end of the
	*	IndexVec then the last index will be returned or end if the vec is empty.
	*/
	size_t					MoveToIndexNumber(
								size_t					inIndexNumber);
	size_t					MoveToStart(void);
	size_t					MoveToEnd(void);
	
	static const size_t	end;
protected:
	const IndexVec*	mIndexVec;
	bool			mWrap;
	size_t			mCurrentRun;
	size_t			mRunStartIndex;
	size_t			mCurrentIndex;
	size_t			mRunEndIndex;
	size_t			mLastCurrentIndex;
};
#endif
