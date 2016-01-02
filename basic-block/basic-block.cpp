#include <limits.h>
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
    _is_nop = true;
    for(INSTR_MAPS_ITERATOR iter = _instr_maps.begin(); iter!=_instr_maps.end(); iter++)
        if(!iter->second->is_nop())
            _is_nop = false;
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
    BLUE("%s[0x%lx - 0x%lx)(INSTR_NUM: %d): <Path:%s> ", \
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

std::string SequenceBBL::generate_code_template(std::vector<BBL_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    std::vector<INSTR_RELA> instr_reloc_vec;
    for(INSTR_MAPS_ITERATOR iter = _instr_maps.begin(); iter!=_instr_maps.end(); iter++){
        Instruction *instr = iter->second;
        std::string instr_template = instr->generate_instr_template(instr_reloc_vec);
        //relocation
        
    }
    return std::string("badbeef");
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

std::string RetBBL::generate_code_template(std::vector<BBL_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeaf");
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

std::string DirectCallBBL::generate_code_template(std::vector<BBL_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeaf");
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

std::string IndirectCallBBL::generate_code_template(std::vector<BBL_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeaf");
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

std::string DirectJumpBBL::generate_code_template(std::vector<BBL_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeaf");
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

std::string IndirectJumpBBL::generate_code_template(std::vector<BBL_RELA> &reloc_vec) const
{
    INSTR_RELA_VEC instr_reloc_vec;
    std::string bbl_template;
    UINT16 curr_pc_pos = 0;
    for(INSTR_MAPS_ITERATOR iter = _instr_maps.begin(); iter!=_instr_maps.end(); iter++){
        Instruction *instr = iter->second;
        curr_pc_pos += instr->get_instr_size();
        SIZE curr_bbl_template_len = bbl_template.length();
        ASSERTM(curr_bbl_template_len < USHRT_MAX, "bbl template should be not larger than USHRT_MAX\n");
        std::string instr_template = instr->generate_instr_template(instr_reloc_vec);
        //normalize the instruction relocations into bbl start relocation
        for(INSTR_RELA_VEC_ITER it = instr_reloc_vec.begin(); it!=instr_reloc_vec.end(); it++){
            INSTR_RELA &rela = *it;
            switch(rela.r_type){
                case RIP_RELA_TYPE:
                    {
                        UINT16 r_addend = curr_pc_pos - rela.r_base_pos - (UINT16)curr_bbl_template_len;
                        UINT64 r_value = rela.r_value;
                        UINT16 r_byte_pos = rela.r_byte_pos + (UINT16)curr_bbl_template_len;
                        UINT16 r_byte_size = rela.r_byte_size;
                        BBL_RELA bbl_rela = {RIP_RELA_TYPE, r_byte_pos, r_byte_size, r_addend, r_value};
                        reloc_vec.push_back(bbl_rela);
                    }
                    break;
                case BRANCH_RELA_TYPE:
                    {
                        NOT_IMPLEMENTED(wangzhe);
                    }
                    break;
                case SS_RELA_TYPE:
                    {
                        NOT_IMPLEMENTED(wangzhe);
                    }
                    break;
                case CC_RELA_TYPE:
                    {
                        UINT16 r_byte_pos = rela.r_byte_pos + (UINT16)curr_bbl_template_len;
                        BBL_RELA bbl_rela = {CC_RELA_TYPE, r_byte_pos, 4, 0, 0};
                        reloc_vec.push_back(bbl_rela);
                    }
                    break;
                default:
                    ASSERTM(0, "unkown instruction relocation type (%d)!\n", rela.r_type);
            };
        }
        //merge the new instruction template
        bbl_template += instr_template;
        //clear the instr_reloc_vec
        instr_reloc_vec.clear();
    }

    return bbl_template;
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

std::string ConditionBrBBL::generate_code_template(std::vector<BBL_RELA> &reloc_vec) const
{
    NOT_IMPLEMENTED(wangzhe);
    return std::string("badbeaf");
}

