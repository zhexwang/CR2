#include "basic-block.h"

BasicBlock::BasicBlock(const F_SIZE start, const SIZE size, const std::vector<const Instruction*> &instr_vec)
    : _start(start), _size(size), _instr_vec(instr_vec)
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
