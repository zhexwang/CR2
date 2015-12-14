#include "basic-block.h"

BasicBlock::BasicBlock(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
		const std::map<F_SIZE, Instruction *> &instr_maps)
    : _start(start), _size(size), _is_call_proceeded(is_call_proceeded), _instr_maps(instr_maps)
{
    ;
}

BasicBlock::~BasicBlock()
{
    NOT_IMPLEMENTED(wangzhe);
}

void BasicBlock::dump() const
{
    NOT_IMPLEMENTED(wangzhe);
}

SequenceBBL::SequenceBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	const std::map<F_SIZE, Instruction *> &instr_maps): \
	BasicBlock(start, size, is_call_proceeded, instr_maps)
{
    ;
}

SequenceBBL::~SequenceBBL()
{
    ;
}

RetBBL::RetBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	const std::map<F_SIZE, Instruction *> &instr_maps):\
	BasicBlock(start, size, is_call_proceeded, instr_maps)
{
    ;
}

RetBBL::~RetBBL()
{
    ;
}

CallBBL::CallBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	const std::map<F_SIZE, Instruction *> &instr_maps):
	BasicBlock(start, size, is_call_proceeded, instr_maps)
{
    ;
}
    
CallBBL::~CallBBL()
{
    ;
}

JumpBBL::JumpBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	const std::map<F_SIZE, Instruction *> &instr_maps):\
	BasicBlock(start, size, is_call_proceeded, instr_maps)
{
    ;
}

JumpBBL::~JumpBBL()
{
    ;
}

ConditionBrBBL::ConditionBrBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	const std::map<F_SIZE, Instruction *> &instr_maps):\
	BasicBlock(start, size, is_call_proceeded, instr_maps)
{
    ;
}

ConditionBrBBL::~ConditionBrBBL()
{
    ;
}
