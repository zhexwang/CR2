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

BOOL Module::is_instr_entry_in_off(const F_SIZE target_offset) const
{
    INSTR_MAP_ITERATOR ret = _instr_maps.find(target_offset);
    if(ret!=_instr_maps.end())
        return true;
    else
        return false;    
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

BOOL Module::is_instr_entry_in_va(const P_ADDRX addr) const
{
    return is_instr_entry_in_off(addr - _real_load_base);
}

BOOL Module::is_bbl_entry_in_va(const P_ADDRX addr) const
{
    return is_bbl_entry_in_off(addr - _real_load_base);
}

BOOL Module::is_func_entry_in_va(const P_ADDRX addr) const
{
    return is_func_entry_in_off(addr - _real_load_base);
}

void Module::insert_br_target(const F_SIZE target_offset)
{
    _br_targets.insert(target_offset);
}

BOOL Module::is_br_target(const F_SIZE target_offset) const
{
    return _br_targets.find(target_offset)==_br_targets.end();
}

void Module::insert_instr(Instruction *instr)
{
    _instr_maps.insert(std::make_pair(instr->get_instr_offset(), instr));
}

void Module::erase_instr(Instruction *instr)
{
    SIZE erased_num = _instr_maps.erase(instr->get_instr_offset());
    FATAL(erased_num==0, "erase instruction error!\n");
    delete instr;
}

//[target_offset, next_offset_of_target_instr)
void Module::erase_instr_range(F_SIZE target_offset, F_SIZE next_offset_of_target_instr, \
    std::vector<Instruction*> &erased_instrs)
{
    F_SIZE guess_start_instr = target_offset;
    Instruction *range_start_instr;
    
    //find range_start_instr
    while(!(range_start_instr = get_instr_by_off(guess_start_instr)))
        guess_start_instr--;

    if(range_start_instr->get_next_offset()<=target_offset){//data in code 
        guess_start_instr = target_offset;
            
        while(!is_instr_entry_in_off(guess_start_instr))
            guess_start_instr++;
    }

    INSTR_MAP_ITERATOR it = _instr_maps.find(guess_start_instr);
    std::vector<Instruction *> record;
    for(; it!=_instr_maps.end(); it++){
        if(it->second->get_next_offset() <= next_offset_of_target_instr)
           record.push_back(it->second);
    }

    for(std::vector<Instruction*>::iterator it = record.begin();\
        it!=record.end(); it++){
        Instruction *erased_instr = *it;
        if(erased_instr->is_direct_call() || erased_instr->is_direct_jump() || \
            erased_instr->is_condition_branch()){
            //erase one br target
            BR_TARGETS_ITERATOR br_it = _br_targets.find(erased_instr->get_target_offset());
            ASSERT(br_it!=_br_targets.end());
            _br_targets.erase(br_it);
        }
            
        erase_instr(erased_instr);
        erased_instrs.push_back(erased_instr);
    }
}

void Module::insert_bbl(BasicBlock *bbl)
{
    _bbl_maps.insert(std::make_pair(bbl->get_bbl_offset(), bbl));
}

void Module::insert_func(Function *func)
{
    _func_maps.insert(std::make_pair(func->get_func_offset(), func));
}

void Module::check_br_targets() const
{
    F_SIZE checked_target = 0;
    for(std::set<F_SIZE>::const_iterator it = _br_targets.begin(); it!=_br_targets.end(); it++){
        F_SIZE target_offset = *it;
        //multiset handling
        if(target_offset==checked_target)
            continue;
        else
            checked_target = target_offset;
        
        if(!is_instr_entry_in_off(target_offset)){
            //judge is prefix. instruction or not 
            target_offset -= 1;
            Instruction *instr = get_instr_by_off(target_offset);
            FATAL(!(instr && instr->has_prefix()), \
                "find one br target (offset:0x%lx) is not in module (%s) instruction list!\n", *it, get_path().c_str());
        }
    }
}

static BasicBlock *construct_bbl(const std::map<F_SIZE, Instruction*> &instr_maps, BOOL is_call_proceeded)
{
    //PRINT("construct bbl!\n");
    const Instruction *first_instr = instr_maps.begin()->second;
    F_SIZE bbl_start = first_instr->get_instr_offset();
    const Instruction *last_instr = instr_maps.rbegin()->second;
    F_SIZE bbl_end = last_instr->get_next_offset();
    SIZE bbl_size = bbl_end - bbl_start;
    
    if(last_instr->is_sequence() || last_instr->is_sys() || last_instr->is_int() || last_instr->is_cmov())
        return new SequenceBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
    else if(last_instr->is_call())
        return new CallBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
    else if(last_instr->is_jump())
        return new JumpBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
    else if(last_instr->is_condition_branch())
        return new ConditionBrBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
    else if(last_instr->is_ret())
        return new RetBBL(bbl_start, bbl_size, is_call_proceeded, instr_maps);
	else
        return NULL;
}


void Module::split_bbl()
{
    // 1.check!
    check_br_targets();
    // 2. init bbl range
    INSTR_MAP_ITERATOR bbl_entry = _instr_maps.end();
    INSTR_MAP_ITERATOR bbl_exit = _instr_maps.end();
    // 3. record last instruction information
    F_SIZE last_next_instr_offset = 0;
    INSTR_MAP_ITERATOR last_iterator = _instr_maps.end();
    BOOL last_bbl_is_call = false;
    // 4. scan all instructions to build bbl 
    for(INSTR_MAP_ITERATOR it = _instr_maps.begin(); it!=_instr_maps.end(); it++){
        // 4.1 get current instruction information
        Instruction *instr = it->second;
        F_SIZE curr_instr_offset = instr->get_instr_offset();
        F_SIZE next_instr_offset = instr->get_next_offset();
        // 4.2 judge
        if(last_next_instr_offset==curr_instr_offset){
            /*these two instructions are sequence */
            // 1. find bbl entry
            if(is_br_target(curr_instr_offset) || \
                (instr->has_prefix() && is_br_target(curr_instr_offset+1))){//consider prefix instruction
                /* curr instruction is bbl entry, so last instr is bbl exit */
                // <bbl_entry, last_iterator> is a bbl
                if(bbl_exit!=last_iterator){
                    bbl_exit = last_iterator;
                    BasicBlock *new_bbl = construct_bbl(INSTR_MAP(bbl_entry, ++bbl_exit), last_bbl_is_call);
                    bbl_exit--;
                    ASSERTM(new_bbl, "unkown instruction list!\n");
                    insert_bbl(new_bbl);
                }//already create new bbl
                // set next bbl entry
                bbl_entry = it;
            }
            // 2. find bbl exit
            if(instr->is_br()){
                // <bbl_entry, bbl_exit> new bbl
                ASSERT(bbl_exit!=it);
                bbl_exit = it;
                BasicBlock *new_bbl = construct_bbl(INSTR_MAP(bbl_entry, ++bbl_exit), last_bbl_is_call);
                bbl_exit--;
                ASSERTM(new_bbl, "unkown instruction list!\n");
                insert_bbl(new_bbl);
                // set next bbl entry
                bbl_entry = ++it;
                it--;
                // judge next bbl is call proceeded
                last_bbl_is_call = instr->is_call() ? true : false;
            }
        }else if (last_next_instr_offset<curr_instr_offset){
            /*these two instructions are not sequence (maybe data in it)*/
            if((last_next_instr_offset!=0) && (bbl_exit!=last_iterator)){
                //data in it, last instruction is bbl exit
                bbl_exit = last_iterator;
                BasicBlock *new_bbl = construct_bbl(INSTR_MAP(bbl_entry, ++bbl_exit), last_bbl_is_call);
                bbl_exit--;
                ASSERTM(new_bbl, "unkown instruction list!\n");
                insert_bbl(new_bbl);
            }
            //set new bbl entry, first instruction or data in it
            bbl_entry = it;
        }else
            ASSERTM(0, "instr map list is not ordered!\n");
        //record curr instruction
        last_next_instr_offset = next_instr_offset;
        last_iterator = it;
    }
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

