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
//  IndexVec.cpp
//
//  Copyright © 2018 Jon Mackey. All rights reserved.
//
#include "IndexVec.h"

/********************************* IndexVec ***********************************/
IndexVec::IndexVec(void)
	: mFirstRunValue(0)
{
	mRuns.push_back(0);
}

/********************************* IndexVec ***********************************/
IndexVec::IndexVec(
	const IndexVec&	inIndexVec)
	: mFirstRunValue(inIndexVec.GetFirstRunValue()),
		mRuns(inIndexVec.GetRuns())
{
}

/********************************* IndexVec ***********************************/
IndexVec::IndexVec(
	const std::string&	inSerializedIndexVec)
{
	SetFromSerial(inSerializedIndexVec);
}

/********************************** SetRun ************************************/
void IndexVec::SetRun(
	uint32_t	inStart,
	uint32_t	inEnd,
	uint8_t		inValue)
{
	if (inStart < inEnd)
	{
		if (inStart <= GetMax())
		{
			if (inValue != 0 ||
				inEnd > GetMin())
			{
				uint8_t	value = inValue ? 1:0;
				size_t leftRunIndex = GetRunIndex(inStart, 0);
				size_t rightRunIndex = GetRunIndex(inEnd, leftRunIndex);
				if (leftRunIndex < rightRunIndex)
				{
					size_t	removeEndIndex = rightRunIndex;
					if (GetRunValue(rightRunIndex) != value)
					{
						mRuns.at(removeEndIndex) = inEnd;
					} else
					{
						removeEndIndex++;
					}

					size_t	removeStartIndex = leftRunIndex +1;
					if (GetRunValue(leftRunIndex) != value)
					{
						if (inStart == 0)
						{
							mFirstRunValue = !mFirstRunValue;
						} else if (mRuns.at(leftRunIndex) != inStart)
						{
							mRuns.at(removeStartIndex) = inStart;
							removeStartIndex++;
						} else
						{
							removeStartIndex--;
						}
					}

					if (removeEndIndex > removeStartIndex)
					{
						mRuns.erase(mRuns.begin() + removeStartIndex, mRuns.begin() + removeEndIndex);
					}
				} else if (GetRunValue(leftRunIndex) != value)
				{
					// At this point leftRunIndex == rightRunIndex
					if (inStart == 0)
					{
						mFirstRunValue = !mFirstRunValue;
						leftRunIndex++;
						mRuns.insert(mRuns.begin() + leftRunIndex, inEnd);
					} else if (mRuns.at(rightRunIndex) != inStart)
					{
						leftRunIndex++;
						mRuns.insert(mRuns.begin() + leftRunIndex, 2, inEnd);
						mRuns.at(leftRunIndex) = inStart;
					} else
					{
						mRuns.at(rightRunIndex) = inEnd;
					}
				}
			}
		} else if (inValue)
		{
			mRuns.push_back(inStart);
			mRuns.push_back(inEnd);
		}
	}
}

/******************************** GetRunStart *********************************/
size_t IndexVec::GetRunStart(
	size_t	inRunIndex) const
{
	return(inRunIndex < mRuns.size() ? mRuns.at(inRunIndex) : -1);
}

/********************************* Contains ***********************************/
/*
*	Returns the value of the run that contains inPosition as a bool
*/
bool IndexVec::Contains(
	uint32_t	inPosition)
{
	size_t runIndex = GetRunIndex(inPosition, 0);
	return(GetRunValue(runIndex) != 0);
}

/****************************** GetIndexNumber ********************************/
/*
*	Returns the number of the index within the set of indexes
*	in the vec (0 to N).
*/
size_t IndexVec::GetIndexNumber(
	size_t	inIndex) const
{
	if (mFirstRunValue != (mRuns.size() & 1))
	{
		size_t	indexNumber = 0;
		Runs::const_iterator	itr = mRuns.begin();
		Runs::const_iterator	itrEnd = mRuns.end();

		if (mFirstRunValue == 0)
		{
			++itr;
		}
		size_t	runStart;
		for (; itr != itrEnd; ++itr)
		{
			runStart = *itr;
			++itr;
			if (inIndex >= runStart)
			{
				if (inIndex >= *itr)
				{
					indexNumber += (*itr - runStart);
				} else
				{
					return(indexNumber + inIndex - runStart);
				}
			}
		}
	}
	return(IndexVecIterator::end);
}

/****************************** GetNthIndex ********************************/
/*
*	Returns the index at the Nth position in the vec.
*/
uint32_t IndexVec::GetNthIndex(
	size_t	inIndexNumber) const
{
	if (mFirstRunValue != (mRuns.size() & 1))
	{
		uint32_t	startIndexNumber = 0;
		uint32_t	endIndexNumber = 0;
		Runs::const_iterator	itr = mRuns.begin();
		Runs::const_iterator	itrEnd = mRuns.end();

		if (mFirstRunValue == 0)
		{
			++itr;
		}
		uint32_t	runStart;
		for (; itr != itrEnd; ++itr)
		{
			runStart = *itr;
			++itr;
			endIndexNumber += (*itr - runStart);
			if ((uint32_t)inIndexNumber >= endIndexNumber)
			{
				startIndexNumber = endIndexNumber;
				continue;
			}
			return(runStart - startIndexNumber + (uint32_t)inIndexNumber);
		}
	}
	return(GetMax()-1);
}

/******************************** GetRunIndex *********************************/
/*
*	Returns the index of the run that contains inPosition
*/
size_t  IndexVec::GetRunIndex(
	uint32_t	inPosition,
	size_t		inStartFrom) const
{
	if (mRuns.size() > 0)
	{
		size_t current = inStartFrom;
		size_t leftIndex = current;
		size_t rightIndex = mRuns.size()-1;

		while (leftIndex <= rightIndex)
		{
			current = (leftIndex + rightIndex) / 2;

			if (mRuns.at(current) > inPosition)
			{
				rightIndex = current - 1;
			} else
			{
				leftIndex = current + 1;
				/*
				*	If we've reached the list end OR
				*	the next run starts after inPosition THEN
				*	current contains the run containing inPosition
				*/
				if (leftIndex > rightIndex ||
					mRuns.at(leftIndex) > inPosition)
				{
					return(current);
				}
			}
		}
	}
	return(0);
}

/********************************** GetMin ************************************/
uint32_t IndexVec::GetMin(void) const
{
	return(mFirstRunValue ? mRuns.front() : (mRuns.size() > 1 ? mRuns.at(1) : 0));
}

/******************************** SetIndexes **********************************/
void IndexVec::Set(
	uint32_t	inFrom,
	uint32_t	inTo)
{
	SetRun(inFrom, inTo+1, 1);
}

/******************************* ClearIndexes *********************************/
void IndexVec::Clear(
	uint32_t	inFrom,
	uint32_t	inTo)
{
	SetRun(inFrom, inTo+1, 0);
}

/********************************** Clear *************************************/
void IndexVec::Clear(void)
{
	mRuns.clear();
	mFirstRunValue = 0;
	mRuns.push_back(0);
}

/********************************** Empty *************************************/
bool IndexVec::Empty(void) const
{
	return(mRuns.size() == 1);
}

/********************************* GetCount ***********************************/
size_t IndexVec::GetCount(void) const
{
	size_t	count = 0;
	if (mFirstRunValue != (mRuns.size() & 1))
	{
		Runs::const_iterator	itr = mRuns.begin();
		Runs::const_iterator	itrEnd = mRuns.end();

		if (mFirstRunValue == 0)
		{
			++itr;
		}
		size_t	runStart;
		for (; itr != itrEnd; ++itr)
		{
			runStart = *itr;
			++itr;
			count += (*itr - runStart);
		}
	}
	return(count);
}

/*********************************** Copy *************************************/
void IndexVec::Copy(
	const IndexVec&	inIndexVec)
{
	mFirstRunValue = inIndexVec.GetFirstRunValue();
	mRuns = inIndexVec.GetRuns();
}

/******************************** operator = **********************************/
IndexVec& IndexVec::operator = (
	const IndexVec&	inIndexVec)
{
	Copy(inIndexVec);
	return(*this);
}

/*********************************** Union ************************************/
void IndexVec::Union(
	const IndexVec&	inIndexVec)
{
	if (&inIndexVec != this)
	{
		/*
		*	If mFirstRunValue is 0, then there should be an odd number of run offsets.
		*	If mFirstRunValue is 1, then there should be an even number of run offsets.
		*/
		if (mFirstRunValue != (mRuns.size() & 1))
		{
			Runs::const_iterator	itr = inIndexVec.GetRuns().begin();
			Runs::const_iterator	itrEnd = inIndexVec.GetRuns().end();

			if (inIndexVec.GetFirstRunValue() == 0)
			{
				++itr;
			}
			uint32_t	runStart;
			for (; itr != itrEnd; ++itr)
			{
				runStart = *itr;
				++itr;
				SetRun(runStart, *itr, 1);
			}
		}
	}
}

/******************************** operator += *********************************/
IndexVec& IndexVec::operator += (
	const IndexVec&	inIndexVec)
{
	Union(inIndexVec);
	return(*this);
}

/******************************** operator |= *********************************/
IndexVec& IndexVec::operator |= (
	const IndexVec&	inIndexVec)
{
	Union(inIndexVec);
	return(*this);
}


/*********************************** Diff *************************************/
bool IndexVec::Diff(
	const IndexVec&	inIndexVec)
{
	if (&inIndexVec != this)
	{
		/*
		*	If mFirstRunValue is 0, then there should be an odd number of run offsets.
		*	If mFirstRunValue is 1, then there should be an even number of run offsets.
		*/
		if (mFirstRunValue != (mRuns.size() & 1))
		{
			Runs::const_iterator	itr = inIndexVec.GetRuns().begin();
			Runs::const_iterator	itrEnd = inIndexVec.GetRuns().end();

			if (inIndexVec.GetFirstRunValue() == 0)
			{
				++itr;
			}
			uint32_t	runStart = 0;
			uint32_t	max = GetMax();
			uint32_t	min = GetMin();
			for (; itr != itrEnd && runStart < max; ++itr)
			{
				runStart = *itr;
				++itr;
				if (*itr > min)
				{
					SetRun(runStart, *itr, 0);
				}
			}
		}
	}
	return(Empty());
}

/******************************** operator -= *********************************/
IndexVec& IndexVec::operator -= (
	const IndexVec&	inIndexVec)
{
	Diff(inIndexVec);
	return(*this);
}

/*********************************** Sect *************************************/
bool IndexVec::Sect(
	const IndexVec&	inIndexVec)
{
	if (&inIndexVec != this &&
		!Empty())
	{
		/*
		*	If mFirstRunValue is 0, then there should be an odd number of run offsets.
		*	If mFirstRunValue is 1, then there should be an even number of run offsets.
		*/
		if (mFirstRunValue != (mRuns.size() & 1))
		{
			Runs::const_iterator	itr = inIndexVec.GetRuns().begin();
			Runs::const_iterator	itrEnd = inIndexVec.GetRuns().end();

			if (inIndexVec.GetFirstRunValue() == 1)
			{
				++itr;
			}
			uint32_t	runStart = *itr;
			++itr;
			uint32_t	max = GetMax();
			uint32_t	min = GetMin();
			for (; itr != itrEnd && runStart < max; ++itr)
			{
				if (*itr > min)
				{
					SetRun(runStart, *itr, 0);
				}
				++itr;
				runStart = *itr;
			}
			SetRun(runStart, 0xFFFFFFFF, 0);
		}
	}
	return(Empty());
}

/******************************** operator &= *********************************/
IndexVec& IndexVec::operator &= (
	const IndexVec&	inIndexVec)
{
	Sect(inIndexVec);
	return(*this);
}

/********************************** IsEqual ***********************************/
bool IndexVec::IsEqual(
	const IndexVec&	inIndexVec)
{
	if (inIndexVec.GetFirstRunValue() == mFirstRunValue &&
		inIndexVec.GetRuns().size() == mRuns.size())
	{
		Runs::const_iterator	itr = mRuns.begin();
		Runs::const_iterator	itrEnd = mRuns.end();
		Runs::const_iterator	otherItr = inIndexVec.GetRuns().begin();

		for (; itr != itrEnd; ++itr, ++otherItr)
		{
			if (*itr == *otherItr)
			{
				continue;
			}
			return(false);
		}
		return(true);
	}
	return(false);
}							


/****************************** SetFromSerial *********************************/
bool IndexVec::SetFromSerial(
	const std::string&	inSerializedIndexVec)
{
	const char* charPtr = inSerializedIndexVec.c_str();
	char		thisChar = *(charPtr++);
	uint32_t	currIndex = 0;
	uint32_t	thisValue = 0;
	uint32_t	endIndex = 0;
	bool		hasValue = false;
	bool		firstRunValueIsSet = false;
	bool		endIndexIsSet = false;
	for (; thisChar != 0; thisChar = *(charPtr++))
	{
		if (isspace(thisChar))
		{
			if (hasValue)
			{
				if (firstRunValueIsSet)
				{
					if (endIndexIsSet)
					{
						currIndex += thisValue;
						mRuns.push_back(currIndex);
					} else
					{
						endIndex = thisValue;
						endIndexIsSet = true;
					}
				} else
				{
					mFirstRunValue = thisValue != 0 ? 1:0;
					firstRunValueIsSet = true;
				}
				hasValue = false;
				thisValue = 0;
			}
		} else if (thisChar >= '0' && thisChar <= '9')
		{
			thisValue = (thisValue*10) + (thisChar - '0');
			hasValue = true;
		} else
		{
			// Fatal error, unexpected character
			firstRunValueIsSet = false;	// To force an error condition below
			break;
		}
	}
	
	if (hasValue &&
		firstRunValueIsSet &&
		endIndexIsSet)
	{
		currIndex += thisValue;
		mRuns.push_back(currIndex);
	}
	
	/*
	*	Validate
	*	If mFirstRunValue is 0, then there should be an odd number of run offsets.
	*	If mFirstRunValue is 1, then there should be an even number of run offsets.
	*/
	if (firstRunValueIsSet == false ||
		endIndexIsSet == false ||
		mFirstRunValue == (mRuns.size() & 1) ||
		currIndex != endIndex)
	{
		mFirstRunValue = 0;
		mRuns.clear();
		mRuns.push_back(0);
		return(false);
	}
	return(true);
}

/******************************** Serialize ***********************************/
/*
*	The serialized string is represented as an array of numbers, where the first
*	number is either 0 or 1 representing the first run value, followed by the
*	last index as an error check, followed by index offsets.
*
*	Example: frv = 0 indexes = 0 3 7 10 14 18 22 35, could be written as 0 35 0 3 4 3 4 4 4 13
*	The sum of 0 3 4 3 4 4 4 13 = 35 (error check)
*/
const std::string& IndexVec::Serialize(
	std::string&	outSerializedIndexVec) const
{
	char numBuff[15];
#ifdef __GNUC__
	snprintf(numBuff, 15, "%d", (int)mFirstRunValue);
#else
	_snprintf_s(numBuff, 15, 14, "%d", (int)mFirstRunValue);
#endif
	outSerializedIndexVec.assign(numBuff);

#ifdef __GNUC__
	snprintf(numBuff, 15, " %d", (int)mRuns.at(mRuns.size()-1));
#else
	_snprintf_s(numBuff, 15, 14, " %d", (int)mRuns.at(mRuns.size()-1));
#endif
	outSerializedIndexVec.append(numBuff);
	
	Runs::const_iterator	itr = mRuns.begin();
	Runs::const_iterator	itrEnd = mRuns.end();
	
	uint32_t	previousIndex = 0;

	for (; itr != itrEnd; ++itr)
	{
	#ifdef __GNUC__
		snprintf(numBuff, 15, " %d", (int)*itr-previousIndex);
	#else
		_snprintf_s(numBuff, 15, 14, " %d", (int)*itr-previousIndex);
	#endif
		previousIndex = *itr;
		outSerializedIndexVec.append(numBuff);
	}
	return(outSerializedIndexVec);
}

const size_t	IndexVecIterator::end = -1;

/***************************** IndexVecIterator *******************************/
IndexVecIterator::IndexVecIterator(
	const IndexVec*	inIndexVec,
	bool			inWrap)
	: mIndexVec(inIndexVec), mWrap(inWrap), mCurrentRun(0), mRunStartIndex(0),
		mRunEndIndex(0), mCurrentIndex(end), mLastCurrentIndex(end)
{
	MoveToStart();
}

/******************************* SetIndexVec **********************************/
void IndexVecIterator::SetIndexVec(
	const IndexVec*	inIndexVec)
{
	mCurrentIndex = end;
	mLastCurrentIndex = end;
	mIndexVec = inIndexVec;
	MoveToStart();
}

/*********************************** Next *************************************/
size_t IndexVecIterator::Next(void)
{
	if (mIndexVec)
	{
		/*
		*	Don't allow entry if the end has been reached or set.
		*	The only way to clear this is to call one of the MoveTo functions.
		*/
		if (mCurrentIndex != end)
		{
			mLastCurrentIndex = mCurrentIndex;
			mCurrentIndex++;
			if (mCurrentIndex >= mRunEndIndex)
			{
				mCurrentRun++;
				const Runs&	runs = mIndexVec->GetRuns();
				if (mCurrentRun >= runs.size())
				{
					if (mWrap)
					{
						mCurrentRun = 1 - mIndexVec->GetFirstRunValue();
					} else
					{
						mCurrentRun--;
						mCurrentIndex = end;
						return(end);
					}
				}
				mRunStartIndex = mCurrentIndex = runs.at(mCurrentRun);
				mCurrentRun++;
				mRunEndIndex = runs.at(mCurrentRun);
			}
		}
	}
	return(mCurrentIndex);
}

/********************************* Previous ***********************************/
size_t IndexVecIterator::Previous(void)
{
	if (mIndexVec)
	{
		/*
		*	Don't allow entry if the end has been reached or set.
		*	The only way to clear this is to call one of the MoveTo functions.
		*/
		if (mCurrentIndex != end)
		{
			mLastCurrentIndex = mCurrentIndex;
			if (mCurrentIndex == mRunStartIndex)
			{
				const Runs&	runs = mIndexVec->GetRuns();
				/*
				*	Each positive run requires 2 offsets.
				*	If we have at least 2 offsets before the mCurrentRun.
				*	(either 0,1 or 1,2, depending on the first run value)
				*/
				if (mCurrentRun > 2)
				{
					mCurrentRun-=2;
				} else if (mWrap)
				{
					mCurrentRun = runs.size() - 1;
				} else
				{
					mCurrentIndex = end;
					return(end);
				}
				mRunEndIndex = runs.at(mCurrentRun);
				mCurrentIndex = mRunEndIndex -1;
				mRunStartIndex = runs.at(mCurrentRun-1);
			} else
			{
				mCurrentIndex--;
			}
		}
	}
	return(mCurrentIndex);
}
/********************************** MoveToValue ************************************/
/*
*	Will attempt to move to the value of inIndex.  If inIndex is not in the
*	IndexVec then the closest index will be returned.  The closest index
*	may be before inIndex.
*/
size_t IndexVecIterator::MoveToValue(
	size_t	inIndex)
{
	mLastCurrentIndex = mCurrentIndex;
	mCurrentIndex = end;
	if (mIndexVec)
	{
		const Runs&	runs = mIndexVec->GetRuns();
		size_t	numOffsets = runs.size();
		/*
		*	Verify that the IndexVec is not empty and is valid
		*	(has the expected number of offsets)
		*/
		if (numOffsets > 1 &&
			mIndexVec->GetFirstRunValue() != (numOffsets & 1))
		{
			mCurrentRun = mIndexVec->GetRunIndex((uint32_t)inIndex, 0);
			/*
			*	If inIndex isn't positive THEN
			*	jump to the closest positive run (without wrapping)
			*/
			if (mIndexVec->GetRunValue(mCurrentRun) == 0)
			{
				uint32_t	rightDelta = 0xFFFFFFFF;
				uint32_t	leftDelta = 0xFFFFFFFF;
				if ((mCurrentRun+1) < numOffsets)
				{
					rightDelta = runs.at(mCurrentRun+1) - (uint32_t)inIndex;
				}
				if (mCurrentRun > 1)
				{
					leftDelta = (uint32_t)inIndex - runs.at(mCurrentRun);
				}
				if (rightDelta < leftDelta ||
					leftDelta == 0xFFFFFFFF)
				{
					mCurrentRun++;
					mCurrentIndex = runs.at(mCurrentRun);
				} else
				{
					mCurrentIndex = runs.at(mCurrentRun) -1;
					mCurrentRun--;
				}
			} else
			{
				mCurrentIndex = inIndex;
			}

			mRunStartIndex = runs.at(mCurrentRun);
			mCurrentRun++;
			mRunEndIndex = runs.at(mCurrentRun);
		}
	}
	return(mCurrentIndex);
}

/**************************** CurrentIndexNumber ******************************/
/*
*	Returns the position of the the current index within the IndexVec.
*/
size_t IndexVecIterator::CurrentIndexNumber(void) const
{
	return(end != mCurrentIndex ? mIndexVec->GetIndexNumber(mCurrentIndex) : end);
}

/***************************** MoveToIndexNumber ******************************/
/*
*	Will attempt to MoveTo Nth index.  If inIndexNumber is past the end of the
*	IndexVec then the last index will be returned or end if the vec is empty.
*/
size_t IndexVecIterator::MoveToIndexNumber(
	size_t	inIndexNumber)
{
	mLastCurrentIndex = mCurrentIndex;
	mCurrentIndex = end;
	if (mIndexVec)
	{
		const Runs&	runs = mIndexVec->GetRuns();
		size_t	numOffsets = runs.size();
		/*
		*	Verify that the IndexVec is not empty and is valid
		*	(has the expected number of offsets)
		*/
		if (numOffsets > 1 &&
			mIndexVec->GetFirstRunValue() != (numOffsets & 1))
		{
			Runs::const_iterator	itr = runs.begin();
			Runs::const_iterator	itrEnd = runs.end();

			if (mIndexVec->GetFirstRunValue() == 0)
			{
				++itr;
			}
			size_t	runStart;
			size_t	count = 0;
			for (; itr != itrEnd; ++itr)
			{
				runStart = *itr;
				++itr;
				count += *itr - runStart;
				if (inIndexNumber >= count)
				{
					continue;
				}
				mCurrentRun = itr - runs.begin();
				mRunEndIndex = *itr;
				mRunStartIndex = runStart;
				mCurrentIndex = mRunEndIndex - count + inIndexNumber;
				break;
			}
			if (mCurrentIndex == end)
			{
				mCurrentRun = numOffsets - 1;
				mRunEndIndex = runs.at(mCurrentRun);
				mCurrentIndex = mRunEndIndex -1;
				mRunStartIndex = runs.at(mCurrentRun-1);
			}
		}
	}
	return(mCurrentIndex);
}

/******************************** MoveToStart *********************************/
size_t IndexVecIterator::MoveToStart(void)
{
	mLastCurrentIndex = mCurrentIndex;
	mCurrentIndex = end;
	if (mIndexVec)
	{
		const Runs&	runs = mIndexVec->GetRuns();
		size_t	numOffsets = runs.size();
		mCurrentRun = 0;
		/*
		*	Verify that the IndexVec is not empty and is valid
		*	(has the expected number of offsets)
		*/
		if (numOffsets > 1 &&
			mIndexVec->GetFirstRunValue() != (numOffsets & 1))
		{
			if (mIndexVec->GetFirstRunValue() == 0)
			{
				mCurrentRun++;
			}
			mRunStartIndex = mCurrentIndex = runs.at(mCurrentRun);
			mCurrentRun++;
			mRunEndIndex = runs.at(mCurrentRun);
		}
	}
	return(mCurrentIndex);
}

/********************************* MoveToEnd **********************************/
size_t IndexVecIterator::MoveToEnd(void)
{
	mLastCurrentIndex = mCurrentIndex;
	mCurrentIndex = end;
	if (mIndexVec)
	{
		const Runs&	runs = mIndexVec->GetRuns();
		size_t	numOffsets = runs.size();
		/*
		*	Verify that the IndexVec is not empty and is valid
		*	(has the expected number of offsets)
		*/
		if (numOffsets > 1 &&
			mIndexVec->GetFirstRunValue() != (numOffsets & 1))
		{
			mCurrentRun = numOffsets - 1;
			mRunEndIndex = runs.at(mCurrentRun);
			mCurrentIndex = mRunEndIndex -1;
			mRunStartIndex = runs.at(mCurrentRun-1);
		}
	}
	return(mCurrentIndex);
}
