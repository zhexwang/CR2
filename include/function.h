#pragma once
#include <vector>

#include "type.h"
#include "utility.h"

class Instruction;
class BasicBlock;

class Function
{
protected:
	F_SIZE _start;
	SIZE _size;
	std::vector<const BasicBlock*> _bbl_vec;
	std::vector<const BasicBlock*> _entry_bbl_vec;
	std::vector<const Instruction*> _instr_vec;
public:
	Function(const F_SIZE start, const SIZE size, const std::vector<const BasicBlock*> &bbl_vec, \
		const std::vector<const BasicBlock*> &entry_bbl_vec, const std::vector<const Instruction*> &instr_vec);
	~Function();
	//get functions
	F_SIZE get_func_offset() const {return _start;}
	F_SIZE get_func_size() const {return _size;}
	P_ADDRX get_func_paddr(P_ADDRX load_base) const {return _start + load_base;}
	//dump functions
	void dump() const;
};
