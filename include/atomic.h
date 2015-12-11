#pragma once

#include "type.h"

class SpinLock
{
protected:
	volatile UINT32 _lock;
public:
	SpinLock():_lock(0){;}

	void lock()
	{
		volatile UINT32 curr_value;
		__asm__ __volatile__(
			"1: \n\t"
			"mov $1, %[temp] \n\t"
			"xchg %[temp], %[value] \n\t"
			"test %[temp], %[temp] \n\t"
			"jnz 1b \n\t"
			:[value]"+m"(_lock), [temp]"=&q"(curr_value)
			::"cc"
		);
	}

	void unlock()
	{
		_lock = 0;
	}
};

