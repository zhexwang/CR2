
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

std::string SequenceInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    if(is_rip_relative()){
        ASSERTM(_dInst.ops[0].type==O_SMEM || _dInst.ops[0].type==O_MEM  || _dInst.ops[1].type==O_SMEM || _dInst.ops[1].type==O_MEM
            ||_dInst.ops[2].type==O_SMEM || _dInst.ops[2].type==O_MEM, "unknown operand in RIP-operand!\n");
        INT64 rela_pos = 0;
        //find displacement
        BOOL disp_match = false;
        INT32 *scan_start = (INT32*)_encode;
        INT32 *disp_scanner = scan_start;
        INT32 *scan_end = (INT32*)((UINT64)_encode + _dInst.size -3);
        while(disp_scanner!=scan_end){
            if((*disp_scanner)==(INT32)_dInst.disp){
                ASSERT(disp_match);
                rela_pos = (INT64)disp_scanner - (INT64)scan_start;
                disp_match = true;
            }
            INT8 *increase = (INT8*)disp_scanner;//move one byte
            disp_scanner = (INT32*)(++increase);
        }
        FATAL(!disp_match, "displacement find failed!\n");
        INSTR_RELA rela_info = {RIP_RELA_TYPE, (UINT16)rela_pos, 4, (UINT16)(_dInst.size), (INT64)_dInst.disp};
        reloc_vec.push_back(rela_info);
    }

    return std::string((const char*)_encode, (SIZE)_dInst.size);
}

DirectCallInstr::DirectCallInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

DirectCallInstr::~DirectCallInstr()
{
    ;
}

std::string DirectCallInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    ASSERT(_dInst.size==5);
    //convert call rel32 to jump rel32
    //push origin return address on main stack
    //push new return address on shadow stack
    return std::string("badbeef");
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

std::string IndirectCallInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeef");
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

std::string DirectJumpInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeef");
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

std::string IndirectJumpInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeef");
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

std::string ConditionBrInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeef");
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

std::string RetInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeef");
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

std::string SysInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    return std::string((const char*)_encode, (SIZE)_dInst.size);
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

std::string CmovInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    if(is_rip_relative()){
        ASSERTM(_dInst.ops[0].type==O_SMEM || _dInst.ops[0].type==O_MEM  || _dInst.ops[1].type==O_SMEM || _dInst.ops[1].type==O_MEM
            ||_dInst.ops[2].type==O_SMEM || _dInst.ops[2].type==O_MEM, "unknown operand in RIP-operand!\n");
        INT64 rela_pos = 0;
        //find displacement
        BOOL disp_match = false;
        INT32 *scan_start = (INT32*)_encode;
        INT32 *disp_scanner = scan_start;
        INT32 *scan_end = (INT32*)((INT64)_encode + _dInst.size -3);
        while(disp_scanner!=scan_end){
            if((*disp_scanner)==(INT32)_dInst.disp){
                ASSERT(disp_match);
                rela_pos = (INT64)disp_scanner - (INT64)scan_start;
                disp_match = true;
            }
            INT8 *increase = (INT8*)disp_scanner;//move one byte
            disp_scanner = (INT32*)(++increase);
        }
        FATAL(!disp_match, "displacement find failed!\n");
        INSTR_RELA rela_info = {RIP_RELA_TYPE, (UINT16)rela_pos, 4, (UINT16)(_dInst.size), (INT64)_dInst.disp};
        reloc_vec.push_back(rela_info);
    }

    return std::string((const char*)_encode, (SIZE)_dInst.size);

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
	
std::string IntInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    return std::string((const char*)_encode, (SIZE)_dInst.size);
}

