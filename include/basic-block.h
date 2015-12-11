#pragma once

#include <vector>

#include "type.h"
#include "utility.h"
#include "instruction.h"


class BasicBlock
{
protected:
	const F_SIZE _start;
	const SIZE _size;
	const BOOL _is_call_proceeded;
	std::vector<const Instruction *> _instr_vec;
public:
	BasicBlock(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
		const std::vector<const Instruction*> &instr_vec);
	virtual ~BasicBlock();
	//get functions
	F_SIZE get_bbl_offset() const {return _start;}
	P_ADDRX get_bbl_paddr(const P_ADDRX load_base) const {return _start + load_base;}
	const Module *get_module() const {return _instr_vec[0]->get_module();}
	//dump functions
	void dump() const;
};

class SequenceBBL : public BasicBlock
{
	SequenceBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
		const std::vector<const Instruction*> &instr_vec);
	~SequenceBBL();
};

class RetBBL : public BasicBlock
{
public:
	RetBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
		const std::vector<const Instruction*> &instr_vec);
	~RetBBL();
};

class CallBBL : public BasicBlock
{
public:
	CallBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
		const std::vector<const Instruction*> &instr_vec);
	~CallBBL();
};

class JumpBBL : public BasicBlock
{
public:
	JumpBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
		const std::vector<const Instruction*> &instr_vec);
	~JumpBBL();
};

class ConditionBrBBL : public BasicBlock
{
public:
	ConditionBrBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
		const std::vector<const Instruction*> &instr_vec);
	~ConditionBrBBL();
};



