#pragma once

#include <vector>

#include "type.h"
#include "utility.h"

class Instruction;
class BasicBlock{
protected:
	F_SIZE _start;
	SIZE _size;
	std::vector<const Instruction*> _instr_vec;
public:
	BasicBlock(const F_SIZE start, const SIZE size, const std::vector<const Instruction*> &instr_vec);
	~BasicBlock();
	//get functions
	F_SIZE get_bbl_offset() const {return _start;}
	P_ADDRX get_bbl_paddr(const P_ADDRX load_base) const {return _start + load_base;}
	//dump functions
	void dump() const;
};

