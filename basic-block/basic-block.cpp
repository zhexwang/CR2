#include <limits.h>
#include "basic-block.h"
#include "module.h"
#include "instr_generator.h"
#include "disassembler.h"

const std::string SequenceBBL::_bbl_type = "SequenceBBL";
const std::string RetBBL::_bbl_type = "RetBBL";
const std::string DirectCallBBL::_bbl_type = "DirectCallBBL";
const std::string IndirectCallBBL::_bbl_type = "IndirectCallBBL";

const std::string DirectJumpBBL::_bbl_type = "DirectJumpBBL";
const std::string IndirectJumpBBL::_bbl_type = "IndirectJumpBBL";
const std::string ConditionBrBBL::_bbl_type = "ConditionBrBBL";

BasicBlock::BasicBlock(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_lock_and_repeat_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps)
    : _start(start), _size(size), _is_call_proceeded(is_call_proceeded), _has_lock_and_repeat_prefix(has_lock_and_repeat_prefix),\
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

static std::string generate_instr_templates(std::vector<BBL_RELA> &reloc_vec, BasicBlock::INSTR_MAPS &instr_maps)
{
    INSTR_RELA_VEC instr_reloc_vec;
    std::string bbl_template;
    UINT16 curr_pc_pos = 0;
    for(BasicBlock::INSTR_MAPS_ITERATOR iter = instr_maps.begin(); iter!=instr_maps.end(); iter++){
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
                        INT16 r_addend = (INT16)curr_pc_pos - (INT16)(rela.r_base_pos + curr_bbl_template_len);
                        INT64 r_value = rela.r_value;
                        UINT16 r_byte_pos = rela.r_byte_pos + curr_bbl_template_len;
                        UINT16 r_byte_size = rela.r_byte_size;
                        BBL_RELA bbl_rela = {RIP_RELA_TYPE, r_byte_pos, r_byte_size, r_addend, r_value};
                        reloc_vec.push_back(bbl_rela);
                    }
                    break;
                case BRANCH_RELA_TYPE:
                    {
                        INT16 r_addend = 0 - (INT16)(rela.r_base_pos + curr_bbl_template_len);
                        INT64 r_value = rela.r_value;
                        UINT16 r_byte_pos = rela.r_byte_pos + curr_bbl_template_len;
                        UINT16 r_byte_size = rela.r_byte_size;
                        BBL_RELA bbl_rela = {BRANCH_RELA_TYPE, r_byte_pos, r_byte_size, r_addend, r_value};
                        reloc_vec.push_back(bbl_rela);
                    }
                    break;
                case SS_RELA_TYPE:
                    {
                        INT16 r_addend = (INT16)rela.r_value;
                        UINT16 r_byte_pos = rela.r_byte_pos + curr_bbl_template_len;
                        UINT16 r_byte_size = rela.r_byte_size;
                        BBL_RELA bbl_rela = {SS_RELA_TYPE, r_byte_pos, r_byte_size, r_addend, 0};
                        reloc_vec.push_back(bbl_rela);
                    }
                    break;
                case CC_RELA_TYPE:
                    {
                        UINT16 r_byte_pos = rela.r_byte_pos + curr_bbl_template_len;
                        UINT16 r_byte_size = rela.r_byte_size;
                        INT16 r_addend = (INT16)rela.r_value;
                        BBL_RELA bbl_rela = {CC_RELA_TYPE, r_byte_pos, r_byte_size, r_addend, 0};
                        reloc_vec.push_back(bbl_rela);
                    }
                    break;
                case LOW32_CC_RELA_TYPE:
                    {
                        UINT16 r_byte_pos = rela.r_byte_pos + curr_bbl_template_len;
                        UINT16 r_byte_size = rela.r_byte_size;
                        INT64 r_value = rela.r_value;
                        BBL_RELA bbl_rela = {LOW32_CC_RELA_TYPE, r_byte_pos, r_byte_size, 0, r_value};
                        reloc_vec.push_back(bbl_rela);
                    }
                    break;
                case HIGH32_CC_RELA_TYPE:
                    {
                        UINT16 r_byte_pos = rela.r_byte_pos + curr_bbl_template_len;
                        UINT16 r_byte_size = rela.r_byte_size;
                        INT64 r_value = rela.r_value;
                        BBL_RELA bbl_rela = {HIGH32_CC_RELA_TYPE, r_byte_pos, r_byte_size, 0, r_value};
                        reloc_vec.push_back(bbl_rela);
                    }
                    break;
                case LOW32_ORG_RELA_TYPE:
                    {
                        UINT16 r_byte_pos = rela.r_byte_pos + curr_bbl_template_len;
                        UINT16 r_byte_size = rela.r_byte_size;
                        INT64 r_value = rela.r_value;
                        BBL_RELA bbl_rela = {LOW32_ORG_RELA_TYPE, r_byte_pos, r_byte_size, 0, r_value};
                        reloc_vec.push_back(bbl_rela);
                    }
                    break;
                case TRAMPOLINE_RELA_TYPE:
                    {
                        UINT16 r_byte_pos = rela.r_byte_pos + curr_bbl_template_len;
                        UINT16 r_byte_size = rela.r_byte_size;
                        INT16 r_addend = (INT16)rela.r_value;
                        BBL_RELA bbl_rela = {TRAMPOLINE_RELA_TYPE, r_byte_pos, r_byte_size, r_addend, 0};
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

SequenceBBL::SequenceBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_lock_and_repeat_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps)
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
    std::string bbl_template = generate_instr_templates(reloc_vec, _instr_maps);
    SIZE curr_bbl_template_len = bbl_template.length();
    
    UINT16 rela_jump_pos;
    std::string jmp_rel32_template = InstrGenerator::gen_jump_rel32_instr(rela_jump_pos, 0);
    
    UINT16 r_byte_pos = curr_bbl_template_len + rela_jump_pos;
    bbl_template += jmp_rel32_template;
    
    UINT16 jump_next_pos = bbl_template.length();
    INT16 r_addend = 0 - (INT16)jump_next_pos;
    INT64 r_value = (INT64)get_fallthrough_offset();
    
    BBL_RELA rela = {BRANCH_RELA_TYPE, r_byte_pos, 4, r_addend, r_value};
    reloc_vec.push_back(rela);
    
    return bbl_template;
}

RetBBL::RetBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_lock_and_repeat_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps)
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
    return generate_instr_templates(reloc_vec, _instr_maps);    
}

DirectCallBBL::DirectCallBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_lock_and_repeat_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps)
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
    return generate_instr_templates(reloc_vec, _instr_maps);
}

IndirectCallBBL::IndirectCallBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_lock_and_repeat_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps)
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
    return generate_instr_templates(reloc_vec, _instr_maps);    
}

DirectJumpBBL::DirectJumpBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_lock_and_repeat_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps)
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
    return generate_instr_templates(reloc_vec, _instr_maps);
}

IndirectJumpBBL::IndirectJumpBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_lock_and_repeat_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps)
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
    return generate_instr_templates(reloc_vec, _instr_maps);    
}

ConditionBrBBL::ConditionBrBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, \
	BOOL has_lock_and_repeat_prefix, BasicBlock::INSTR_MAPS &instr_maps)
	: BasicBlock(start, size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps)
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
    return generate_instr_templates(reloc_vec, _instr_maps);    
}

