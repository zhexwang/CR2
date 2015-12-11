#include "basic-block.h"

BasicBlock::BasicBlock(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
		const std::vector<const Instruction*> &instr_vec)
    : _start(start), _size(size), _is_call_proceeded(is_call_proceeded), _instr_vec(instr_vec)
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
