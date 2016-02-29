#pragma once

#include "type.h"
#include "utility.h"

template<class T>
class Range
{
private:
	T _mLow;
	T _mHigh;
public:
	explicit Range(T item) : _mLow(item), _mHigh(item)
	{// [item,item]
		;	
	}  
	Range(T low, T high) : _mLow(low), _mHigh(high)
	{  // [low,high]
		ASSERT(high>=low);
	}

	bool operator<(const Range& rhs) const
	{
		if (_mLow < rhs._mLow)
			return true;
		else
			return false;
	} // operator<
	SIZE operator-(const Range& rhs) const
	{
		return _mLow - rhs._mHigh - 1;
	}
	SIZE size() const {return _mHigh - _mLow + 1;}
	T low() const { return _mLow;}
	T high() const { return _mHigh;}
};

