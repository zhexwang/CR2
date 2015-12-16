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

BOOL Module::is_br_target(const F_SIZE target_offset) const
{
    return _br_targets.find(target_offset)!=_br_targets.end();
}

void Module::dump_br_target(const F_SIZE target_offset) const
{
    BR_TARGETS_CONST_ITER br_it = _br_targets.find(target_offset);
    if(br_it!=_br_targets.end()){
        CONST_BR_TARGET_SRCS &srcs = br_it->second;
        for(BR_TARGET_SRCS_CONST_ITER it = srcs.begin(); it!=srcs.end(); it++)
            PRINT("0x%lx, ", *it);
    }
}

void Module::insert_instr(Instruction *instr)
{
    _instr_maps.insert(std::make_pair(instr->get_instr_offset(), instr));
}

void Module::erase_instr(Instruction *instr)
{
    F_SIZE instr_offset = instr->get_instr_offset();
    SIZE erased_num = _instr_maps.erase(instr_offset);
    FATAL(erased_num==0, "erase instruction error!\n");
    //erase jmpin
    _indirect_jump_maps.erase(instr_offset);
    delete instr;
    //TODO:pattern should be maintance!
}

void Module::erase_br_target(const F_SIZE target, const F_SIZE src)
{
    BR_TARGETS_ITERATOR br_it = _br_targets.find(target);
    ASSERT(br_it!=_br_targets.end());
    BR_TARGET_SRCS &srcs = br_it->second;
    BR_TARGET_SRCS_ITERATOR src_it = srcs.find(src);
    ASSERT(src_it!=srcs.end());
    srcs.erase(src_it);
    
    if(srcs.size()==0)
        _br_targets.erase(br_it);
}

void Module::insert_br_target(const F_SIZE target, const F_SIZE src)
{      
    BR_TARGETS_ITERATOR br_it = _br_targets.find(target);   
    if(br_it==_br_targets.end()){
        BR_TARGET_SRCS srcs;
        srcs.insert(src);
        _br_targets.insert(make_pair(target, srcs));
    }else{
        BR_TARGET_SRCS &srcs = br_it->second;
        ASSERT(srcs.size()!=0);
        srcs.insert(src);
    }
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
            F_SIZE target = erased_instr->get_target_offset();
            F_SIZE src = erased_instr->get_instr_offset();
            erase_br_target(target, src);
        }
            
        erase_instr(erased_instr);
        erased_instrs.push_back(erased_instr);
    }
}

void Module::insert_bbl(BasicBlock *bbl)
{
    F_SIZE second_offset = 0;
    _bbl_maps.insert(std::make_pair(bbl->get_bbl_offset(second_offset), bbl));
}

void Module::insert_func(Function *func)
{
    _func_maps.insert(std::make_pair(func->get_func_offset(), func));
}

void Module::insert_align_entry(F_SIZE offset)
{
    _align_entries.insert(offset);
}

BOOL Module::is_align_entry(F_SIZE offset) const
{
    return _align_entries.find(offset)!=_align_entries.end();
}

void Module::insert_likely_jump_table_pattern(Module::PATTERN_INSTRS pattern)
{
    _jump_table_pattern.push_back(pattern);
}

void Module::insert_indirect_jump(F_SIZE offset)
{
    JUMPIN_INFO info = {UNKNOW, 0, 0};
    _indirect_jump_maps.insert(std::make_pair(offset, info));
}

void Module::check_br_targets()
{
    for(BR_TARGETS_ITERATOR it = _br_targets.begin(); it!=_br_targets.end(); it++){
        F_SIZE target_offset = it->first;
        
        if(!is_instr_entry_in_off(target_offset)){
            //judge is prefix. instruction or not 
            F_SIZE target_offset2 = target_offset - 1;
            Instruction *instr = get_instr_by_off(target_offset2);
            FATAL(!(instr && instr->has_prefix()), \
                "find one br target (offset:0x%lx, src:0x%lx) is not in module (%s) instruction list!\n", \
                target_offset, *(it->second.begin()), get_path().c_str());
        }
    }
}

static BasicBlock *construct_bbl(const std::map<F_SIZE, Instruction*> &instr_maps, BOOL is_call_proceeded)
{
    const Instruction *first_instr = instr_maps.begin()->second;
    F_SIZE bbl_start = first_instr->get_instr_offset();
    const Instruction *last_instr = instr_maps.rbegin()->second;
    F_SIZE bbl_end = last_instr->get_next_offset();
    SIZE bbl_size = bbl_end - bbl_start;
    BOOL has_prefix = first_instr->has_prefix();        
    
    if(last_instr->is_sequence() || last_instr->is_sys() || last_instr->is_int() || last_instr->is_cmov())
        return new SequenceBBL(bbl_start, bbl_size, is_call_proceeded, has_prefix, instr_maps);
    else if(last_instr->is_call())
        return new CallBBL(bbl_start, bbl_size, is_call_proceeded, has_prefix, instr_maps);
    else if(last_instr->is_jump())
        return new JumpBBL(bbl_start, bbl_size, is_call_proceeded, has_prefix, instr_maps);
    else if(last_instr->is_condition_branch())
        return new ConditionBrBBL(bbl_start, bbl_size, is_call_proceeded, has_prefix, instr_maps);
    else if(last_instr->is_ret())
        return new RetBBL(bbl_start, bbl_size, is_call_proceeded, has_prefix, instr_maps);
	else
        return NULL;
}


void Module::split_bbl()
{
    // 1.check!
    check_br_targets();
    analysis_jump_table();
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
            // 1. find bbl entry (consider prefix instruction and align entries)
            if(is_br_target(curr_instr_offset) || is_align_entry(curr_instr_offset) || \
                (instr->has_prefix() && (is_br_target(curr_instr_offset+1) || is_align_entry(curr_instr_offset+1)))){
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

void Module::analysis_jump_table()
{    
    JUMP_TABLE_PATTERN_ITER pattern_it = _jump_table_pattern.begin();
    while(pattern_it!=_jump_table_pattern.end()){
        PATTERN_INSTRS &instrs = *pattern_it;
        F_SIZE pattern_first_instr = instrs.front();
        F_SIZE indirect_jump_offset = instrs.back();

        //should be more precise: control flow analysis        
        INSTR_MAP_ITERATOR pattern_start_instr_it = _instr_maps.find(pattern_first_instr);
        UINT8 table_base_reg = pattern_start_instr_it->second->get_sib_base_reg();
        pattern_start_instr_it--;
        INT32 fault_count = 0;
        INT32 fault_tolerant = 10;
        while(pattern_start_instr_it!=_instr_maps.end()){
            Instruction *instr = pattern_start_instr_it->second;
            F_SIZE table_base_offset = 0;
            if(instr->is_dest_reg(table_base_reg)){
                if(instr->is_lea_rip(table_base_reg, table_base_offset)){/*
                    BLUE("Switch Jump Table[0x%lx]: (%s)\n", table_base_offset, get_path().c_str());
                    while(instrs_first_it!=instrs.end()){
                        get_instr_by_off(*instrs_first_it)->dump_file_inst();
                        instrs_first_it++;
                    }*/
                    //check jump table: there is 4byte data in one entry! Also check entry is instruction aligned
                    F_SIZE entry_offset = table_base_offset;
                    INT32 entry_data = read_4byte_data_in_off(entry_offset);
                    F_SIZE first_entry_instr = entry_data + table_base_offset;
                    if(!is_instr_entry_in_off(first_entry_instr))//makae sure that is real jump table!
                        break;
                    //calculate jump table size
                    do{
                        //record jump targets
                        insert_br_target(table_base_offset+entry_data, indirect_jump_offset);
                        entry_offset += 4;
                        entry_data = read_4byte_data_in_off(entry_offset);
                    }while(entry_data!=0);
                    
                    JUMPIN_MAP_ITER iter = _indirect_jump_maps.find(indirect_jump_offset);
                    JUMPIN_INFO &info = iter->second;
                    info.type = SWITCH_CASE;
                    info.table_offset = table_base_offset;
                    info.table_size = entry_offset - table_base_offset;
                    
                    break;
                }else if(fault_count<fault_tolerant)
                    fault_count++;
                else
                    break;
            }
            pattern_start_instr_it--;
        }
        
        pattern_it++;    
    }
}

void Module::analysis_all_modules_indirect_jump_targets()
{
   MODULE_MAP_ITERATOR it = _all_module_maps.begin();
   for(; it!=_all_module_maps.end(); it++){
        it->second->analysis_indirect_jump_targets();
   }
}

void Module::dump_all_bbls_in_va(const P_ADDRX load_base)
{
    MODULE_MAP_ITERATOR it = _all_module_maps.begin();
    for(; it!=_all_module_maps.end(); it++){
         it->second->dump_bbl_in_va(load_base);
    }
}

void Module::dump_all_bbls_in_off()
{
    MODULE_MAP_ITERATOR it = _all_module_maps.begin();
    for(; it!=_all_module_maps.end(); it++){
         it->second->dump_bbl_in_off();
    }
}

void Module::dump_bbl_in_va(const P_ADDRX load_base) const
{
    BBL_MAP_ITERATOR it = _bbl_maps.begin();
    for(; it!=_bbl_maps.end(); it++){
        it->second->dump_in_va(load_base);
    }
}

void Module::dump_bbl_in_off() const
{
    BBL_MAP_ITERATOR it = _bbl_maps.begin();
    for(; it!=_bbl_maps.end(); it++){
        it->second->dump_in_off();
    }
}

BasicBlock *Module::find_bbl_by_instr(Instruction *instr) const
{
    BBL_MAP_ITERATOR bbl_it;
    INSTR_MAP_ITERATOR instr_it = _instr_maps.find(instr->get_instr_offset());
    while((bbl_it = _bbl_maps.find(instr_it->first)) == _bbl_maps.end()){
        instr_it--;
    }
    return bbl_it->second;
}

Instruction *Module::find_instr_by_off(F_SIZE offset) const
{
    INSTR_MAP_ITERATOR instr_it = _instr_maps.find(offset);
    while(instr_it == _instr_maps.end()){
        offset--;
    }
    return instr_it->second;
}