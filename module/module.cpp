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

void Module::split_bbl()
{
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

