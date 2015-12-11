#include "function.h"

Function::Function(const F_SIZE start, const SIZE size, const std::vector<const BasicBlock*> &bbl_vec, \
    const std::vector<const BasicBlock*> &entry_bbl_vec, const std::vector<const Instruction*> &instr_vec)
    : _start(start), _size(size), _bbl_vec(bbl_vec), _entry_bbl_vec(entry_bbl_vec), _instr_vec(instr_vec)
{
    ;
}

Function::~Function()
{
    NOT_IMPLEMENTED(wangzhe);
}

void Function::dump() const
{
    NOT_IMPLEMENTED(wangzhe);
}

