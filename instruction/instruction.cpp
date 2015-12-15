
#include <string.h>

#include "utility.h"
#include "instruction.h"
#include "disassembler.h"
#include "module.h"

const std::string SequenceInstr::_type_name = "Sequence Instruction";
const std::string DirectCallInstr::_type_name = "Direct Call Instruction";
const std::string IndirectCallInstr::_type_name = "Indirect Call Instruction";
const std::string DirectJumpInstr::_type_name = "Direct Jump Instruction";
const std::string IndirectJumpInstr::_type_name = "Indirect Jump Instruction";
const std::string SysInstr::_type_name = "Sys Instruction";
const std::string IntInstr::_type_name = "Int Instruction";
const std::string CmovInstr::_type_name = "Cmovxx Instruction";
const std::string RetInstr::_type_name = "Ret Instruction";

Instruction::Instruction(const _DInst &dInst, const Module *module)
    : _dInst(dInst), _module(module)
{
    //copy instruction's encode
    _encode = new UINT8[_dInst.size]();
    void *ret = memcpy(_encode, module->get_code_offset_ptr(_dInst.addr), _dInst.size);
    FATAL(!ret, "memcpy failed!\n");
}

Instruction::~Instruction()
{
    free(_encode);
}

void Instruction::dump_pinst(const P_ADDRX load_base) const
{
    Disassembler::dump_pinst(this, load_base);
}

void Instruction::dump_file_inst() const
{
    Disassembler::dump_file_inst(this);
}

SequenceInstr::SequenceInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

SequenceInstr::~SequenceInstr()
{
    ;
}

DirectCallInstr::DirectCallInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

DirectCallInstr::~DirectCallInstr()
{

}

IndirectCallInstr::IndirectCallInstr(const _DInst &dInst, const Module *module)
     : Instruction(dInst, module)
{
    ;
}

IndirectCallInstr::~IndirectCallInstr()
{
    ;
}

DirectJumpInstr::DirectJumpInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

DirectJumpInstr::~DirectJumpInstr()
{
    ;
}

IndirectJumpInstr::IndirectJumpInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

IndirectJumpInstr::~IndirectJumpInstr()
{
    ;
}

ConditionBrInstr::ConditionBrInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

ConditionBrInstr::~ConditionBrInstr()
{
    ;
}

RetInstr::RetInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

RetInstr::~RetInstr()
{
    ;
}

SysInstr::SysInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

SysInstr::~SysInstr()
{
    ;
}
CmovInstr::CmovInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

CmovInstr::~CmovInstr()
{
    ;
}

IntInstr::IntInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}
IntInstr::~IntInstr()
{
    ;
}
	

