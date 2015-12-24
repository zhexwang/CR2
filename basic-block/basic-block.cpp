#include "basic-block.h"
#include "module.h"

const std::string SequenceBBL::_bbl_type = "SequenceBBL";
const std::string RetBBL::_bbl_type = "RetBBL";
const std::string DirectCallBBL::_bbl_type = "DirectCallBBL";
const std::string IndirectCallBBL::_bbl_type = "IndirectCallBBL";

const std::string DirectJumpBBL::_bbl_type = "DirectJumpBBL";
const std::string IndirectJumpBBL::_bbl_type = "IndirectJumpBBL";
const std::string ConditionBrBBL::_bbl_type = "ConditionBrBBL";

BasicBlock::BasicBlock(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps)
    : _start(start), _size(size), _is_call_proceeded(is_call_proceeded), _has_prefix(has_prefix),\
        _instr_maps(instr_maps)
{
    ;
}

BasicBlock::~BasicBlock()
{
    NOT_IMPLEMENTED(wangzhe);
}

void BasicBlock::dump_in_va(const P_ADDRX load_base) const
{
    P_ADDRX second_entry = 0;
    P_ADDRX first_entry = get_bbl_paddr(load_base, second_entry);
    BLUE("%s[0x%lx - 0x%lx)(INSTR_NUM: %d): <Path:%s>", \
        get_type().c_str(), first_entry, first_entry+_size, (INT32)_instr_maps.size(), get_module()->get_path().c_str());
    if(second_entry==0)//only one entry
        BLUE("BBL_Entry(0x%lx)\n", first_entry);
    else
        BLUE("BBL_Entry(0x%lx<Normal>, 0x%lx<Prefix>)\n", first_entry, second_entry);
    //dump instrs
    for(INSTR_MAPS_ITERATOR it = _instr_maps.begin(); it!=_instr_maps.end(); it++){
        it->second->dump_pinst(load_base);
    }
}

void BasicBlock::dump_in_off() const
{
    F_SIZE second_entry = 0;
    F_SIZE first_entry = get_bbl_offset(second_entry);
    BLUE("%s[0x%lx - 0x%lx)(INSTR_NUM: %d): <Path:%s>", \
        get_type().c_str(), first_entry, first_entry+_size, (INT32)_instr_maps.size(), get_module()->get_path().c_str());
    if(second_entry==0)//only one entry
        BLUE("BBL_Entry(0x%lx)\n", first_entry);
    else
        BLUE("BBL_Entry(0x%lx<Normal>, 0x%lx<Prefix>)\n", first_entry, second_entry);
    //dump instrs
    for(INSTR_MAPS_ITERATOR it = _instr_maps.begin(); it!=_instr_maps.end(); it++){
        it->second->dump_file_inst();
    }
}

SequenceBBL::SequenceBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_prefix, instr_maps)
{
    _target = 0;
    _fallthrough = start+size;
}

SequenceBBL::~SequenceBBL()
{
    ;
}

RetBBL::RetBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_prefix, instr_maps)
{
    _target = 0;
    _fallthrough = 0;
}

RetBBL::~RetBBL()
{
    ;
}

DirectCallBBL::DirectCallBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_prefix, instr_maps)
{
    _target = instr_maps.rbegin()->second->get_target_offset();
    _fallthrough = start+size;
}
    
DirectCallBBL::~DirectCallBBL()
{
    ;
}

IndirectCallBBL::IndirectCallBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_prefix, instr_maps)
{
    _target = 0;
    _fallthrough = start+size;
}
    
IndirectCallBBL::~IndirectCallBBL()
{
    ;
}

DirectJumpBBL::DirectJumpBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_prefix, instr_maps)
{
    _target = instr_maps.rbegin()->second->get_target_offset();
    _fallthrough = 0;
}

DirectJumpBBL::~DirectJumpBBL()
{
    ;
}

IndirectJumpBBL::IndirectJumpBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_prefix, instr_maps)
{
    _target = 0;
    _fallthrough = 0;
}

IndirectJumpBBL::~IndirectJumpBBL()
{
    ;
}

ConditionBrBBL::ConditionBrBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_prefix, instr_maps)
{
    _target = instr_maps.rbegin()->second->get_target_offset();
    _fallthrough = start+size;
}

ConditionBrBBL::~ConditionBrBBL()
{
    ;
}
