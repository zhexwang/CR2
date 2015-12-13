#include "module.h"
#include "elf-parser.h"
#include "instruction.h"
#include "basic-block.h"
#include "function.h"

Module::MODULE_MAP Module::_all_module_maps;
Module::Module(const ElfParser *elf): _elf(elf), _real_load_base(0)
{
    _all_module_maps.insert(make_pair(_elf->get_elf_name(), this));
}

Module::~Module()
{
    NOT_IMPLEMENTED(wangzhe);
}

Instruction *Module::get_instr_by_off(const F_SIZE off) const
{
    INSTR_MAP_ITERATOR it = _instr_maps.find(off);
    if(it!=_instr_maps.end())
        return it->second;
    else
        return NULL;
}

Instruction *Module::get_instr_by_va(const P_ADDRX addr) const
{
    ASSERTM(_real_load_base!=0, "Forgot setting real load base of module(%s)\n", get_name().c_str());
    return get_instr_by_off(addr - _real_load_base);
}

BasicBlock *Module::get_bbl_by_off(const F_SIZE off) const
{
    BBL_MAP_ITERATOR it = _bbl_maps.find(off);
    if(it!=_bbl_maps.end())
        return it->second;
    else
        return NULL;
}

BasicBlock *Module::get_bbl_by_va(const P_ADDRX addr) const
{
    ASSERTM(_real_load_base!=0, "Forgot setting real load base of module(%s)\n", get_name().c_str());
    return get_bbl_by_off(addr - _real_load_base);
}

Function *Module::get_func_by_off(const F_SIZE off) const
{
    FUNC_MAP_ITERATOR it = _func_maps.find(off);
    if(it!=_func_maps.end())
        return it->second;
    else
        return NULL;
}

Function *Module::get_func_by_va(const P_ADDRX addr) const
{
    ASSERTM(_real_load_base!=0, "Forgot setting real load base of module(%s)\n", get_name().c_str());
    return get_func_by_off(addr - _real_load_base);
}

BOOL Module::is_bbl_entry_in_off(const F_SIZE target_offset) const
{
    BBL_MAP_ITERATOR ret = _bbl_maps.find(target_offset);
    if(ret!=_bbl_maps.end())
        return true;
    else
        return false;    
}

BOOL Module::is_func_entry_in_off(const F_SIZE target_offset) const
{
    FUNC_MAP_ITERATOR it = _func_maps.find(target_offset);
    if(it!=_func_maps.end())
        return true;
    else
        return false;
}

BOOL Module::is_bbl_entry_in_va(const P_ADDRX addr) const
{
    return is_bbl_entry_in_off(addr - _real_load_base);
}

BOOL Module::is_func_entry_in_va(const P_ADDRX addr) const
{
    return is_func_entry_in_off(addr - _real_load_base);
}

void Module::insert_instr(Instruction *instr)
{
    _instr_maps.insert(std::make_pair(instr->get_inst_offset(), instr));
}

void Module::insert_bbl(BasicBlock *bbl)
{
    _bbl_maps.insert(std::make_pair(bbl->get_bbl_offset(), bbl));
}

void Module::insert_func(Function *func)
{
    _func_maps.insert(std::make_pair(func->get_func_offset(), func));
}

static BasicBlock *construct_bbl(std::map<const F_SIZE, const Instruction*> &instr_maps, BOOL is_call_proceeded)
{
    const Instruction *first_instr = instr_maps.begin()->second;
    F_SIZE bbl_start = first_instr->get_inst_offset();
    const Instruction *last_instr = instr_maps.rbegin()->second;
    F_SIZE bbl_end = last_instr->get_next_offset();
    SIZE bbl_size = bbl_end - bbl_start;
    
    if(last_instr->is_sequence() || last_instr->is_sys() || last_instr->is_int() || last_instr->is_cmov())
        return new SequenceBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
    else if(last_instr->is_direct_call() || last_instr->is_indirect_call())
        return new CallBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
    else if(last_instr->is_direct_jump() || last_instr->is_indirect_jump())
        return new JumpBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
    else if(last_instr->is_condition_branch())
        return new ConditionBrBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
    else if(last_instr->is_ret())
        return new RetBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
	else
        return NULL;
}


void Module::split_bbl()
{//TODO: data in code 
    INFO("Split %s\n", get_name().c_str());
}



void Module::split_all_modules_into_bbls()
{
   MODULE_MAP_ITERATOR it = _all_module_maps.begin();
   for(; it!=_all_module_maps.end(); it++){
        it->second->split_bbl();
   }
}

void Module::analysis_indirect_jump_targets()
{
    NOT_IMPLEMENTED(wangzhe);
}

void Module::analysis_all_modules_indirect_jump_targets()
{
   MODULE_MAP_ITERATOR it = _all_module_maps.begin();
   for(; it!=_all_module_maps.end(); it++){
        it->second->analysis_indirect_jump_targets();
   }
}

