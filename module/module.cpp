#include "module.h"
#include "elf-parser.h"
#include "instruction.h"
#include "basic-block.h"
#include "code_variant_manager.h"

Module::MODULE_MAP Module::_all_module_maps;
const std::string Module::func_type_name[Module::FUNC_TYPE_NUM] = 
{
    "CALL_TARGET", "PROLOG_MATCH", "SYM_RECORD",
};

Module::Module(ElfParser *elf): _elf(elf), _real_load_base(0)
{
    _elf->get_plt_range(_plt_start, _plt_end);
    _elf->search_function_from_sym_table(_func_info_maps);
    _elf->search_plt_info(_plt_info_map);
    
    _setjmp_plt = 0;
    for(PLT_INFO_MAP::iterator iter = _plt_info_map.begin(); iter!=_plt_info_map.end(); iter++)
        if(iter->second.plt_name.find("setjmp")!=std::string::npos)
            _setjmp_plt = iter->first;
        
    _elf->search_rela_x_section(_rela_targets);
    _all_module_maps.insert(make_pair(_elf->get_elf_name(), this));
}

Module::~Module()
{
    NOT_IMPLEMENTED(wangzhe);
}

std::string Module::get_sym_func_name(F_SIZE offset) const
{
    SYM_FUNC_INFO_MAP::const_iterator iter = _func_info_maps.find(offset);
    ASSERT(iter!=_func_info_maps.end());
    return iter->second.func_name;
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

std::set<F_SIZE> Module::get_indirect_jump_targets(F_SIZE jumpin_offset, BOOL &is_memset) const
{
    JUMPIN_MAP_CONST_ITER it = _indirect_jump_maps.find(jumpin_offset);
    if(it!=_indirect_jump_maps.end()){
        is_memset = it->second.type==MEMSET_JMP ? true : false;
        return it->second.targets;
    }else{
        is_memset = false;
        return std::set<F_SIZE>();
    }
}

BasicBlock *Module::get_bbl_by_va(const P_ADDRX addr) const
{
    ASSERTM(_real_load_base!=0, "Forgot setting real load base of module(%s)\n", get_name().c_str());
    return get_bbl_by_off(addr - _real_load_base);
}

BOOL Module::is_memset_jmpin(const F_SIZE offset) const
{
    JUMPIN_MAP::const_iterator it = _indirect_jump_maps.find(offset);
    if(it!=_indirect_jump_maps.end()){
        return it->second.type==MEMSET_JMP ? true : false;
    }else
        return false;
}

BOOL Module::is_switch_case_main_jmpin(const F_SIZE offset) const
{
    JUMPIN_MAP::const_iterator it = _indirect_jump_maps.find(offset);
    if(it!=_indirect_jump_maps.end()){
        return it->second.type==SWITCH_CASE_ABSOLUTE ? true : false;
    }else
        return false;
}

BOOL Module::is_switch_case_so_jmpin(const F_SIZE offset) const
{
    JUMPIN_MAP::const_iterator it = _indirect_jump_maps.find(offset);
    if(it!=_indirect_jump_maps.end()){
        return it->second.type==SWITCH_CASE_OFFSET ? true : false;
    }else
        return false;
}

BOOL Module::is_instr_entry_in_off(const F_SIZE target_offset, BOOL consider_prefix) const
{
    INSTR_MAP_ITERATOR ret = _instr_maps.find(target_offset);
    if(ret!=_instr_maps.end())
        return true;
    else{
        if(consider_prefix){
            ret = _instr_maps.find(target_offset-1);
            if(ret!=_instr_maps.end())
                return ret->second->has_lock_and_repeat_prefix();
            else
                return false;
        }else
            return false;
    }
}

BOOL Module::is_bbl_entry_in_off(const F_SIZE target_offset, BOOL consider_prefix) const
{
    BBL_MAP_ITERATOR ret = _bbl_maps.find(target_offset);
    if(ret != _bbl_maps.end())
        return true;
    else{
        if(consider_prefix){
            ret = _bbl_maps.find(target_offset-1);
            if(ret != _bbl_maps.end())
                return ret->second->has_lock_and_repeat_prefix();
            else
                return false;
        }else
            return false;
    }
}

BOOL Module::is_in_plt_in_off(const F_SIZE offset) const
{
    if(offset>=_plt_start && offset<_plt_end)
        return true;
    else
        return false;
}

BOOL Module::is_in_plt_in_va(const P_ADDRX addr) const
{
    return is_in_plt_in_off(addr - _real_load_base);
}

BOOL Module::is_instr_entry_in_va(const P_ADDRX addr, BOOL consider_prefix) const
{
    return is_instr_entry_in_off(addr - _real_load_base, consider_prefix);
}

BOOL Module::is_bbl_entry_in_va(const P_ADDRX addr, BOOL consider_prefix) const
{
    return is_bbl_entry_in_off(addr - _real_load_base, consider_prefix);
}

BOOL Module::is_br_target(const F_SIZE target_offset) const
{
    return _br_targets.find(target_offset)!=_br_targets.end();
}

BOOL Module::is_call_target(const F_SIZE offset) const
{
    return _call_targets.find(offset)!=_call_targets.end();
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
    //erase gs segmentation
    if(instr->has_gs_seg())
        erase_gs_seg(instr_offset);
    delete instr;
    //TODO:pattern should be maintance!
}

void Module::erase_gs_seg(F_SIZE instr_offset)
{
    ASSERT(_gs_set.find(instr_offset)!=_gs_set.end());
    _gs_set.erase(instr_offset);
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

void Module::insert_call_target(const F_SIZE target)
{
    _call_targets.insert(target);
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
            
        while(!is_instr_entry_in_off(guess_start_instr, false))
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

void Module::init_cvm_from_modules()
{
    MODULE_MAP_ITERATOR it = _all_module_maps.begin();
    for(; it!=_all_module_maps.end(); it++){
        Module *module = it->second;
        CodeVariantManager *cvm = new CodeVariantManager(module->get_path());
        it->second->set_cvm(cvm);
    }
}

void Module::generate_relocation_block()
{
    //position fixed bbl
    for(BBL_SET::const_iterator iter = _pos_fixed_bbls.begin(); iter!=_pos_fixed_bbls.end(); iter++){
        BasicBlock *bbl = *iter;
        std::vector<BBL_RELA> rela_info;
        F_SIZE second;
        F_SIZE bbl_offset = bbl->get_bbl_offset(second);
        F_SIZE bbl_end = bbl_offset + bbl->get_bbl_size();
        BOOL has_lock_and_repeat_prefix = bbl->has_lock_and_repeat_prefix();
        BOOL has_fallthrough_bbl = bbl->has_fallthrough_bbl();
        std::string bbl_template = bbl->generate_code_template(rela_info);
        RandomBBL *rbbl = new RandomBBL(bbl_offset, bbl_end, has_lock_and_repeat_prefix, has_fallthrough_bbl, \
            rela_info, bbl_template);
        rela_info.clear();
        _cvm->insert_fixed_random_bbl(bbl_offset, rbbl);
    }
    //position movable bbl
    for(BBL_SET::const_iterator iter = _pos_movable_bbls.begin(); iter!=_pos_movable_bbls.end(); iter++){
        BasicBlock *bbl = *iter;
        std::vector<BBL_RELA> rela_info;
        F_SIZE second;
        F_SIZE bbl_offset = bbl->get_bbl_offset(second);
        F_SIZE bbl_end = bbl_offset + bbl->get_bbl_size();
        BOOL has_lock_and_repeat_prefix = bbl->has_lock_and_repeat_prefix();
        BOOL has_fallthrough_bbl = bbl->has_fallthrough_bbl();
        std::string bbl_template = bbl->generate_code_template(rela_info);
        RandomBBL *rbbl = new RandomBBL(bbl_offset, bbl_end, has_lock_and_repeat_prefix, has_fallthrough_bbl, \
            rela_info, bbl_template);
        rela_info.clear();
        _cvm->insert_movable_random_bbl(bbl_offset, rbbl);
    }
    // recognized indirect jump targets should have trampoline
    for(JUMPIN_MAP_ITER iter = _indirect_jump_maps.begin(); iter!=_indirect_jump_maps.end(); iter++){
        JUMPIN_INFO &info = iter->second;
        F_SIZE jump_offset = iter->first;
        BasicBlock *src_bbl = find_bbl_cover_offset(jump_offset);
        F_SIZE second;
        F_SIZE src_bbl_offset = src_bbl->get_bbl_offset(second);
        //TODO:memset use hash!!!!
        if(info.type==SWITCH_CASE_OFFSET || info.type==SWITCH_CASE_ABSOLUTE)
            _cvm->insert_switch_case_jmpin_rbbl(src_bbl_offset, info.targets);
    }
}

void Module::generate_all_relocation_block()
{
    MODULE_MAP_ITERATOR it = _all_module_maps.begin();
    for(; it!=_all_module_maps.end(); it++){
        Module *module = it->second;
        module->generate_relocation_block();
    }
}

void Module::insert_bbl(BasicBlock *bbl)
{
    F_SIZE second_offset = 0;
    _bbl_maps.insert(std::make_pair(bbl->get_bbl_offset(second_offset), bbl));
}

void Module::insert_align_entry(F_SIZE offset)
{
    _align_entries.insert(offset);
}

BOOL Module::is_align_entry(F_SIZE offset) const
{
    return _align_entries.find(offset)!=_align_entries.end();
}

BOOL Module::is_maybe_func_entry(F_SIZE offset) const
{
    BasicBlock *bb = find_bbl_by_offset(offset, true);
    if(!bb)
        return false;
    else
        return _maybe_func_entry.find(bb)!=_maybe_func_entry.end();
}

BOOL Module::is_maybe_func_entry(BasicBlock *bb) const
{
    return _maybe_func_entry.find(bb)!=_maybe_func_entry.end();
}

void Module::insert_indirect_jump(F_SIZE offset)
{
    JUMPIN_INFO info;
    info.type = UNKNOW;
    info.table_offset = 0;
    info.table_size = 0;
    _indirect_jump_maps.insert(std::make_pair(offset, info));
}

void Module::insert_gs_instr_offset(F_SIZE offset)
{
    _gs_set.insert(offset);
}

void Module::insert_fixed_bbl(BasicBlock *bbl)
{
    _pos_fixed_bbls.insert(bbl);
}

void Module::insert_movable_bbl(BasicBlock *bbl)
{
    _pos_movable_bbls.insert(bbl);
}

BOOL Module::is_gs_used()
{
    return _gs_set.size()!=0;
}

void Module::set_cvm(CodeVariantManager *cvm)
{
    _cvm = cvm;
}

void Module::check_br_targets()
{
    for(BR_TARGETS_ITERATOR it = _br_targets.begin(); it!=_br_targets.end(); it++){
        F_SIZE target_offset = it->first;
        FATAL(!is_instr_entry_in_off(target_offset, true), \
            "find one br target (offset:0x%lx, src:0x%lx) is not in module (%s) instruction list!\n", \
            target_offset, *(it->second.begin()), get_path().c_str());
    }
}

BasicBlock *Module::construct_bbl(const Module::INSTR_MAP &instr_maps, BOOL is_call_proceeded, \
    BOOL is_call_setjmp_proceeded)
{
    const Instruction *first_instr = instr_maps.begin()->second;
    F_SIZE bbl_start = first_instr->get_instr_offset();
    const Instruction *last_instr = instr_maps.rbegin()->second;
    F_SIZE bbl_end = last_instr->get_next_offset();
    SIZE bbl_size = bbl_end - bbl_start;

    BOOL has_lock_and_repeat_prefix = first_instr->has_lock_and_repeat_prefix();    
    BasicBlock *generated_bbl = NULL;

    if(last_instr->is_sequence() || last_instr->is_sys() || last_instr->is_int() || last_instr->is_cmov())
        generated_bbl = new SequenceBBL(bbl_start, bbl_size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps);
    else if(last_instr->is_direct_call())
        generated_bbl = new DirectCallBBL(bbl_start, bbl_size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps);
    else if(last_instr->is_indirect_call())
        generated_bbl = new IndirectCallBBL(bbl_start, bbl_size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps);
    else if(last_instr->is_direct_jump())
        generated_bbl = new DirectJumpBBL(bbl_start, bbl_size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps);
    else if(last_instr->is_indirect_jump())
        generated_bbl = new IndirectJumpBBL(bbl_start, bbl_size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps);
    else if(last_instr->is_condition_branch())
        generated_bbl = new ConditionBrBBL(bbl_start, bbl_size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps);
    else if(last_instr->is_ret())
        generated_bbl = new RetBBL(bbl_start, bbl_size, is_call_proceeded, has_lock_and_repeat_prefix, instr_maps);
    else
        ASSERTM(generated_bbl, "unkown instruction list!\n");
    //judge is function entry or not, ignore plt
    if(is_in_plt_in_off(bbl_start)){
        insert_fixed_bbl(generated_bbl);
    }else if(is_call_setjmp_proceeded){//setjmp proceeded must be fixed
        ASSERT(is_call_proceeded);
        _maybe_func_entry.insert(std::make_pair(generated_bbl, RELA_TARGET));
        insert_fixed_bbl(generated_bbl);
    }else if(is_sym_func_entry(bbl_start)){
        _maybe_func_entry.insert(std::make_pair(generated_bbl, SYM_RECORD));
        insert_fixed_bbl(generated_bbl);
    }else if(is_call_target(bbl_start)){
        _maybe_func_entry.insert(std::make_pair(generated_bbl, CALL_TARGET));
        insert_fixed_bbl(generated_bbl);
    }else if(is_rela_target(bbl_start)){
        _maybe_func_entry.insert(std::make_pair(generated_bbl, RELA_TARGET));
        insert_fixed_bbl(generated_bbl);
    }else{
        INT32 push_reg_instr_num = 0;
        INT32 decrease_rsp_instr_num = 0;
        BOOL increase_rsp = false;
        BOOL has_pop_before_push = false;
        //judge maybe function entry bb or not
        for(Module::INSTR_MAP_ITERATOR iter = instr_maps.begin(); iter!=instr_maps.end(); iter++){
            Instruction *instr = iter->second;
            if(push_reg_instr_num==0 && instr->is_pop_reg()){
                has_pop_before_push = true;
                break;
            }
            if(instr->is_push_reg())
                push_reg_instr_num++;
            if(!increase_rsp && instr->is_decrease_rsp(increase_rsp))
                decrease_rsp_instr_num++;
        }

        if(!has_pop_before_push && !increase_rsp && (push_reg_instr_num>0 || decrease_rsp_instr_num>0)){
            _maybe_func_entry.insert(std::make_pair(generated_bbl, PROLOG_MATCH));
            insert_fixed_bbl(generated_bbl);
        }
    }

    return generated_bbl;
}

void Module::split_bbl()
{
    // 1. Analysis indirect jump instruction to find more br targets!!!!
    analysis_indirect_jump_targets();
    // 2. init bbl range
    INSTR_MAP_ITERATOR bbl_entry = _instr_maps.end();
    INSTR_MAP_ITERATOR bbl_exit = _instr_maps.end();
    // 3. record last instruction information
    F_SIZE last_next_instr_offset = 0;
    INSTR_MAP_ITERATOR last_iterator = _instr_maps.end();
    BOOL last_bbl_is_call = false;
    BOOL last_bbl_is_call_setjmp = false;
    // 4. scan all instructions to build bbl 
    for(INSTR_MAP_ITERATOR it = _instr_maps.begin(); it!=_instr_maps.end(); it++){
        // 4.1 get current instruction information
        Instruction *instr = it->second;
        F_SIZE curr_instr_offset = instr->get_instr_offset();
        F_SIZE next_instr_offset = instr->get_next_offset();
        BOOL is_aligned = is_align_entry(curr_instr_offset);
        // 4.2 judge
        if(last_next_instr_offset==curr_instr_offset){
            /*these two instructions are sequence */
            // 1. find bbl entry (consider prefix instruction and align entries)
            if(is_rela_target(curr_instr_offset) || is_sym_func_entry(curr_instr_offset)|| is_br_target(curr_instr_offset) \
                || is_aligned || (instr->has_lock_and_repeat_prefix() && (is_br_target(curr_instr_offset+1) \
                || is_align_entry(curr_instr_offset+1)))){
                /* curr instruction is bbl entry, so last instr is bbl exit */
                // <bbl_entry, last_iterator> is a bbl
                if(bbl_exit!=last_iterator){
                    bbl_exit = last_iterator;
                    BasicBlock *new_bbl = construct_bbl(INSTR_MAP(bbl_entry, ++bbl_exit), last_bbl_is_call, \
                        last_bbl_is_call_setjmp);
                    bbl_exit--;
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
                BasicBlock *new_bbl = construct_bbl(INSTR_MAP(bbl_entry, ++bbl_exit), last_bbl_is_call, \
                    last_bbl_is_call_setjmp);
                bbl_exit--;
                insert_bbl(new_bbl);
                // set next bbl entry
                bbl_entry = ++it;
                it--;
                // judge next bbl is call proceeded
                last_bbl_is_call = instr->is_call() ? true : false;
                if(instr->is_direct_call()){
                    F_SIZE call_target_offset = instr->get_target_offset();
                    ASSERT(call_target_offset!=0);
                    if(call_target_offset==_setjmp_plt)
                        last_bbl_is_call_setjmp = true;
                    else
                        last_bbl_is_call_setjmp = false;
                }else
                    last_bbl_is_call_setjmp = false;
            }
        }else if (last_next_instr_offset<curr_instr_offset){
            /*these two instructions are not sequence (maybe data in it)*/
            if((last_next_instr_offset!=0) && (bbl_exit!=last_iterator)){
                //data in it, last instruction is bbl exit
                bbl_exit = last_iterator;
                BasicBlock *new_bbl = construct_bbl(INSTR_MAP(bbl_entry, ++bbl_exit), last_bbl_is_call, \
                    last_bbl_is_call_setjmp);
                bbl_exit--;
                insert_bbl(new_bbl);
            }
            //set new bbl entry, first instruction or data in it
            bbl_entry = it;
        }else
            ASSERTM(0, "%s instr map list is not ordered: Last(0x%lx), Curr(0x%lx)\n", \
                get_path().c_str(), last_next_instr_offset, curr_instr_offset);
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
        it->second->examine_bbls();
   }
}

void Module::examine_bbls()
{
    for(BBL_MAP_ITERATOR iter = _bbl_maps.begin(); iter!=_bbl_maps.end(); iter++){
        BasicBlock *bbl = iter->second;
        F_SIZE fallthrough_offset = bbl->get_fallthrough_offset();
        if(bbl->is_sequence()){
            if(bbl->has_ud2() || bbl->has_hlt())
                bbl->set_no_fallthrough();
            else if(!find_bbl_by_offset(fallthrough_offset, false))
                bbl->set_no_fallthrough();
        }else if(bbl->is_ret() || bbl->is_direct_jump() || bbl->is_indirect_jump())
            bbl->set_no_fallthrough();
        else if(bbl->is_indirect_call()){
            if(!find_bbl_by_offset(fallthrough_offset, false))
                bbl->set_no_fallthrough();
        }
    }
}

BOOL Module::analysis_memset_jump(F_SIZE jump_offset, std::set<F_SIZE> &targets)
{
    UINT8 jump_dest_reg = R_NONE;
    F_SIZE memset_target = 0;
    UINT64 disp = 0;
    F_SIZE table_target = 0;
    UINT8 src_reg = R_NONE;
    BOOL lea_dest_src_matched = false, movsx_matched = false, lea_src_imm_matched = false;
    INSTR_MAP_ITERATOR scan_iter = _instr_maps.find(jump_offset);
    while(scan_iter!=_instr_maps.end()){
        Instruction *instr = scan_iter->second;
        if(jump_dest_reg == R_NONE)//indirect jump instruction
            if(instr->is_jump_reg())
                jump_dest_reg = instr->get_dest_reg();
            else
                return false;
        else{
            if(!lea_dest_src_matched){
                if(instr->is_dest_reg(jump_dest_reg)){
                    if(instr->is_add_two_regs_1(jump_dest_reg, src_reg)){
                        lea_dest_src_matched = true;
                    }else
                        return false;
                }
            }else{
                UINT8 movsx_dest = R_NONE, movsx_src = R_NONE;
                if(!movsx_matched){
                    if(instr->is_dest_reg(src_reg)){
                        if(instr->is_movsx_sib_to_reg(movsx_src, movsx_dest)){
                            if(movsx_src==src_reg && movsx_src==movsx_dest)
                                movsx_matched = true;
                            else
                                return false;
                        }else
                            return false;
                    }
                }else{
                    if(!lea_src_imm_matched){
                        if(instr->is_lea_rip(src_reg, disp)){
                            table_target = convert_pt_addr_to_offset(instr->get_next_paddr(get_pt_x_load_base())+disp);
                            lea_src_imm_matched = true;
                        }else
                            return false;
                    }else{
                        if(instr->is_lea_rip(jump_dest_reg, disp)){
                            memset_target = convert_pt_addr_to_offset(instr->get_next_paddr(get_pt_x_load_base())+disp);
                            break;
                        }else
                            return false;
                    }
                }
            }
        }
        scan_iter--;
    }
    //find target list
    F_SIZE entry_offset = table_target;
    INT16 entry_data = read_2byte_data_in_off(entry_offset);
    do{
        //record jump targets
        F_SIZE memset_entry_target = memset_target + entry_data;
        if(is_instr_entry_in_off(memset_entry_target, true)){
            Instruction *inst = find_instr_by_off(memset_entry_target, true);
            if(!inst->is_ret()){
                insert_br_target(memset_entry_target, jump_offset);
                targets.insert(memset_entry_target);
            }
        }else
            break;
        
        entry_offset += 2;
        entry_data = read_2byte_data_in_off(entry_offset);
    }while(entry_data!=0);
    
    return true;
}

/*  from maybe function entry to use control flow to find out all movable bbls, the left bbls are fixed
    (include fucntion entry and plt) 
*/
void Module::separate_movable_bbls()
{ 
    // 1.find all maybe function CFG
    std::vector<BasicBlock*> fixed_aligned_bbl;
    for(FUNC_ENTRY_BBL_MAP::iterator iter = _maybe_func_entry.begin(); iter!=_maybe_func_entry.end(); iter++){
        BasicBlock *bbl = iter->first;
        //curr bbl must be fixed
        ASSERT(is_fixed_bbl(bbl));
        recursive_to_find_movable_bbls(bbl);
    }
    // 2.the left aligned bbl maybe function entry
    for(ALIGN_ENTRY::iterator iter = _align_entries.begin(); iter!=_align_entries.end(); iter++){
        BasicBlock *bbl = find_bbl_by_offset(*iter, false);
        F_SIZE second_entry;
        F_SIZE bbl_entry = bbl->get_bbl_offset(second_entry);
        if(!is_movable_bbl(bbl) && !is_fixed_bbl(bbl) && ((bbl_entry&0xf)==0)){
            insert_fixed_bbl(bbl);
            //find maybe aligned function
            _maybe_func_entry.insert(std::make_pair(bbl, ALIGNED_ENTRY));
            fixed_aligned_bbl.push_back(bbl);
        }
    }
    // 3.find all aligned funciton CFG
    for(std::vector<BasicBlock*>::iterator iter = fixed_aligned_bbl.begin(); iter!=fixed_aligned_bbl.end(); iter++)
        recursive_to_find_movable_bbls(*iter);
    // 4.left aligned funciton 
    for(BBL_MAP_ITERATOR iter = _bbl_maps.begin(); iter!=_bbl_maps.end(); iter++){
        BasicBlock *bbl = iter->second;
        if(!is_movable_bbl(bbl) && !is_fixed_bbl(bbl)){
            if(bbl->is_nop())
                insert_movable_bbl(bbl);
            else{
                insert_fixed_bbl(bbl); 
            }
        }
    }
}

void Module::recursive_to_find_movable_bbls(BasicBlock *bbl)
{
    F_SIZE target_offset = bbl->get_target_offset();
    F_SIZE fallthrough_offset = bbl->get_fallthrough_offset();

    if(bbl->is_sequence() || bbl->is_indirect_call() || bbl->is_direct_call()){
        //find fallthrough movable bbls
        BasicBlock *fallthrough_bbl = find_bbl_by_offset(fallthrough_offset, true);
        if(!fallthrough_bbl){
            ASSERT(read_1byte_code_in_off(fallthrough_offset)==0);
            return ;
        }
        if(!is_movable_bbl(fallthrough_bbl) && !is_fixed_bbl(fallthrough_bbl)){
            insert_movable_bbl(fallthrough_bbl);
            recursive_to_find_movable_bbls(fallthrough_bbl);
        }
    }else if(bbl->is_condition_branch()){
        //find target movable bbls
        BasicBlock *target_bbl = find_bbl_by_offset(target_offset, true);
        ASSERT(target_bbl);
        if(!is_movable_bbl(target_bbl) && !is_fixed_bbl(target_bbl)){
            insert_movable_bbl(target_bbl);
            recursive_to_find_movable_bbls(target_bbl);
        }
        //find fallthrough movable bbls
        BasicBlock *fallthrough_bbl = find_bbl_by_offset(fallthrough_offset, true);
        if(!fallthrough_bbl){
            ASSERT(read_1byte_code_in_off(fallthrough_offset)==0);
            return ;
        }
        if(!is_movable_bbl(fallthrough_bbl) && !is_fixed_bbl(fallthrough_bbl)){
            insert_movable_bbl(fallthrough_bbl);
            recursive_to_find_movable_bbls(fallthrough_bbl);
        }
    }else if(bbl->is_direct_jump()){
        //find target movable bbls
        BasicBlock *target_bbl = find_bbl_by_offset(target_offset, true);
        ASSERT(target_bbl);
        if(!is_movable_bbl(target_bbl) && !is_fixed_bbl(target_bbl)){
            insert_movable_bbl(target_bbl);
            recursive_to_find_movable_bbls(target_bbl);
        }
    }else if(bbl->is_indirect_jump()){
        //find target movable bbls
        JUMPIN_MAP::iterator iter = _indirect_jump_maps.find(bbl->get_last_instr_offset());
        ASSERT(iter!=_indirect_jump_maps.end());
        JUMPIN_INFO &info = iter->second;
        
        if(info.type==SWITCH_CASE_OFFSET || info.type==SWITCH_CASE_ABSOLUTE || info.type==MEMSET_JMP){
            BOOL has_movable_bbl = false;
            for(std::set<F_SIZE>::iterator target_iter = info.targets.begin(); target_iter!=info.targets.end(); target_iter++){
                BasicBlock *target_bbl = find_bbl_by_offset(*target_iter, false);
                ASSERT(target_bbl);
                if(!is_fixed_bbl(target_bbl))
                    has_movable_bbl = true;
                
                if(is_movable_bbl(target_bbl) || is_fixed_bbl(target_bbl))
                    continue ;
                else{
                    insert_movable_bbl(target_bbl);
                    recursive_to_find_movable_bbls(target_bbl);
                }
            }

            if(!has_movable_bbl){//the indirect jump is tail-call!
                info.type = UNKNOW;
                info.table_offset = 0;
                info.table_size = 0;
                info.targets.clear();
            }
        }else
            ASSERT(info.type!=PLT_JMP);
    }
    
    return ;
}

BOOL Module::is_movable_bbl(BasicBlock *bbl)
{
    return _pos_movable_bbls.find(bbl)!=_pos_movable_bbls.end();
}

BOOL Module::is_fixed_bbl(BasicBlock *bbl)
{
    return _pos_fixed_bbls.find(bbl)!=_pos_fixed_bbls.end();
}

BOOL Module::analysis_jump_table_in_main(F_SIZE jump_offset, F_SIZE &table_base, SIZE &table_size, std::set<F_SIZE> &targets)
{
    INSTR_MAP_ITERATOR iter = _instr_maps.find(jump_offset); 
    Instruction *instr = iter->second;
    UINT8 base, index, scale;
    UINT64 disp = 0;
    if(instr->is_jump_mem()){
        instr->get_sib(0, base, index, scale, disp);
        if(base!=R_NONE || index==R_NONE || scale!=8)
            return false;
    }else if(instr->is_jump_smem()){
        disp = instr->get_disp();
    }else{//
        ASSERT(instr->is_jump_reg());
        UINT8 jump_base_reg = instr->get_dest_reg();
        iter--;
        while(iter!=_instr_maps.end()){
            Instruction *inst = iter->second;
            UINT8 base_reg, dest_reg;
            if(inst->is_dest_reg(jump_base_reg)){
                if(inst->is_mov_sib_to_reg64(base_reg, dest_reg, disp) && base_reg==R_NONE && dest_reg==jump_base_reg)
                    break;
                else
                    return false;
            }
            iter--;
        }
    }
    F_SIZE entry_offset = convert_pt_addr_to_offset(disp);
    table_base = entry_offset;
    INT64 entry_data = read_8byte_data_in_off(entry_offset);
    do{
        F_SIZE target_instr = entry_data - get_pt_load_base();
        //record jump targets
        if(is_instr_entry_in_off(target_instr, false)){//makae sure that is real jump target!
            Instruction *inst = find_instr_by_off(target_instr, false);
            if(!inst->is_ret()){
                insert_br_target(target_instr, jump_offset);
                targets.insert(target_instr);
            }
        }else{
            entry_offset -=8;
            break;
        }
        entry_offset += 8;
        entry_data = read_8byte_data_in_off(entry_offset);
    }while((entry_data&0xffffffff)!=0);
    table_size = entry_offset - table_base;
    return true;
}


void Module::analysis_indirect_jump_targets()
{
    for(JUMPIN_MAP_ITER iter = _indirect_jump_maps.begin(); iter!=_indirect_jump_maps.end(); iter++){
        JUMPIN_INFO &info = iter->second;
        F_SIZE jump_offset = iter->first;
        F_SIZE table_base, table_size;
        if(info.type==UNKNOW){//analysis other
            if(is_in_plt_in_off(jump_offset)){//jmpq *%rip... in plt section
                info.type = PLT_JMP;
                info.table_offset = 0;
                info.table_size = 0;
                info.targets.clear();
            }else if(is_shared_object() && analysis_jump_table_in_so(jump_offset, table_base, table_size, info.targets)){
                info.type = SWITCH_CASE_OFFSET;
                info.table_offset = table_base;
                info.table_size = table_size;
            }else if(!is_shared_object() && analysis_jump_table_in_main(jump_offset, table_base, table_size, info.targets)){
                info.type = SWITCH_CASE_ABSOLUTE;
                info.table_offset = table_base;
                info.table_size = table_size;
            }else if(analysis_memset_jump(jump_offset, info.targets)){//assembly code in library
                info.type = MEMSET_JMP;
                info.table_offset = 0;
                info.table_size = 0;
            }
        }
    }
}

BOOL Module::analysis_jump_table_in_so(F_SIZE jump_offset, F_SIZE &table_base, SIZE &table_size, std::set<F_SIZE> &targets)
{
    INSTR_MAP_ITERATOR iter = _instr_maps.find(jump_offset); 
    Instruction *instr = iter->second;
    if(!instr->is_jump_reg())
        return false;
    UINT8 jump_reg = instr->get_dest_reg();
    iter--;

    UINT64 disp;
    UINT8 base_or_entry_reg1 = R_NONE, base_or_entry_reg2 = R_NONE;
    BOOL find_reg1_base = false, find_reg2_base = false;
    F_SIZE reg1_base = 0, reg2_base = 0;
    
    UINT8 movsxd_dest_reg = R_NONE, movsxd_sib_base_reg = R_NONE;
    BOOL find_movsxd_sib_base = false;
    F_SIZE movsxd_sib_base = 0;
    BOOL add_instr_is_matched = false, movsxd_instr_is_matched = false;

    UINT8 add_base_reg = R_NONE;
    BOOL find_add_base_reg = false;
    F_SIZE add_base = 0;

    INT32 fault_count = 0;
    INT32 fault_tolerant = 10;
    
    while(iter!=_instr_maps.end()){
        instr = iter->second;
        if(!add_instr_is_matched){
            if(instr->is_dest_reg(jump_reg)){
                if(instr->is_add_two_regs_1(jump_reg, base_or_entry_reg2)){
                    base_or_entry_reg1 = jump_reg;
                    add_instr_is_matched = true;
                }else
                    return false;//wrong
            }
        }else{//add instr is matched
            if(!movsxd_instr_is_matched){
                if(!find_reg1_base && instr->is_dest_reg(base_or_entry_reg1)){
                    if(instr->is_lea_rip(base_or_entry_reg1, disp)){
                        reg1_base = convert_pt_addr_to_offset(instr->get_next_paddr(get_pt_x_load_base())+disp);
                        find_reg1_base = true;
                        add_base_reg = base_or_entry_reg1;
                        find_add_base_reg = true;
                        add_base = reg1_base;
                    }else if(instr->is_movsxd_sib_to_reg(movsxd_sib_base_reg, movsxd_dest_reg) \
                        && movsxd_dest_reg==base_or_entry_reg1){
                        movsxd_instr_is_matched = true;
                        add_base_reg = base_or_entry_reg2;
                        find_add_base_reg = find_reg2_base;
                        add_base = reg2_base;
                    }else
                        return false;
                }else if(!find_reg2_base && instr->is_dest_reg(base_or_entry_reg2)){
                    if(instr->is_lea_rip(base_or_entry_reg2, disp)){
                        reg2_base = convert_pt_addr_to_offset(instr->get_next_paddr(get_pt_x_load_base())+disp);
                        find_reg2_base = true;
                        add_base_reg = base_or_entry_reg2;
                        find_add_base_reg = true;
                        add_base = reg2_base;
                    }else if(instr->is_movsxd_sib_to_reg(movsxd_sib_base_reg, movsxd_dest_reg) \
                        && movsxd_dest_reg==base_or_entry_reg2){
                        movsxd_instr_is_matched = true;
                        add_base_reg = base_or_entry_reg1;
                        find_add_base_reg = find_reg1_base;
                        add_base = reg1_base;
                    }else
                        return false;
                }

                if(find_reg1_base && find_reg2_base)
                    return false;
            }else{
                if(!find_add_base_reg){
                    if(instr->is_dest_reg(add_base_reg)){
                        if(instr->is_lea_rip(add_base_reg, disp)){
                            add_base = convert_pt_addr_to_offset(instr->get_next_paddr(get_pt_x_load_base())+disp);
                            find_add_base_reg = true;
                        }else if(fault_count<fault_tolerant)
                            fault_count++;
                        else
                            return false;
                    }
                }
                if(!find_movsxd_sib_base){
                    if(instr->is_dest_reg(movsxd_sib_base_reg)){
                        if(instr->is_lea_rip(movsxd_sib_base_reg, disp)){
                            movsxd_sib_base = convert_pt_addr_to_offset(instr->get_next_paddr(get_pt_x_load_base())+disp);
                            find_movsxd_sib_base = true;
                        }else if(fault_count<fault_tolerant)
                            fault_count++;
                        else
                            return false;
                    }
                }
                if(find_add_base_reg && find_movsxd_sib_base){
                    if(add_base==movsxd_sib_base){
                        table_base = movsxd_sib_base;
                        break;
                    }else
                        return false;
                }
            }
        }
        iter--;
    }
    
    if(iter==_instr_maps.end())
        return false;
    else{
        //check jump table: there is 4byte data in one entry! Also check entry is instruction aligned
        F_SIZE entry_offset = table_base;
        INT32 entry_data = read_4byte_data_in_off(entry_offset);
        F_SIZE first_entry_instr = entry_data + table_base;
        if(!is_instr_entry_in_off(first_entry_instr, false))//makae sure that is real jump table!
            return false;
        //calculate jump table size
        do{
            //record jump targets
            F_SIZE target_offset = table_base+entry_data;
            if(is_instr_entry_in_off(target_offset, false)){
                Instruction *inst = find_instr_by_off(target_offset, false);
                if(!inst->is_ret()){
                    insert_br_target(target_offset, jump_offset);
                    targets.insert(target_offset);
                }
            }else{
                entry_offset -= 4;//Warnning 
                break;
            }
            entry_offset += 4;
            entry_data = read_4byte_data_in_off(entry_offset);
        }while(entry_data!=0);
        
        table_size = entry_offset - table_base;
        
        return true;
    }
}

void Module::analysis_all_modules_indirect_jump_targets()
{
   MODULE_MAP_ITERATOR it = _all_module_maps.begin();
   for(; it!=_all_module_maps.end(); it++){
        it->second->analysis_indirect_jump_targets();
   }
}

void Module::separate_movable_bbls_from_all_modules()
{
   MODULE_MAP_ITERATOR it = _all_module_maps.begin();
   for(; it!=_all_module_maps.end(); it++){
        it->second->separate_movable_bbls();
   }
}

void Module::dump_all_bbls_in_va(const P_ADDRX load_base)
{
    MODULE_MAP_ITERATOR it = _all_module_maps.begin();
    for(; it!=_all_module_maps.end(); it++){
         it->second->dump_bbl_in_va(load_base);
    }
}

void Module::dump_all_indirect_jump_result()
{
    BLUE("Dump all indirect jump info:\n");
    MODULE_MAP_ITERATOR it = _all_module_maps.begin();
    for(; it!=_all_module_maps.end(); it++){
         it->second->dump_indirect_jump_result();
    }
}

void Module::dump_indirect_jump_result()
{
    INT32 switch_case_off_count = 0, switch_case_absolute_count = 0;
    INT32 plt_jmp_count = 0;
    INT32 memset_jmp_count = 0;
    INT32 sum = _indirect_jump_maps.size();
    
    JUMPIN_MAP_ITER it = _indirect_jump_maps.begin();
    for(; it!=_indirect_jump_maps.end(); it++){
        switch(it->second.type){
            case SWITCH_CASE_OFFSET:switch_case_off_count++; break;
            case PLT_JMP: plt_jmp_count++; break;
            case SWITCH_CASE_ABSOLUTE: switch_case_absolute_count++; break;
            case MEMSET_JMP: memset_jmp_count++; break;
            default:
                ;//find_instr_by_off(it->first)->dump_file_inst();
        }
        
    }
    
    PRINT("Indirect Jump Types (%4d in %20s): %2d%%(%4d switch case offset), %2d%%(%4d switch case absolute), %2d%%(%4d plt), %2d%%(%4d memset)\n",\
        sum, get_name().c_str(), 100*switch_case_off_count/sum, switch_case_off_count, 100*switch_case_absolute_count/sum,\
        switch_case_absolute_count, 100*plt_jmp_count/sum, plt_jmp_count, 100*memset_jmp_count/sum, memset_jmp_count);
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

void Module::dump_bbl_movable_info() const
{
    INT32 gadget_num = 0;
    INT32 little_gadget = 0;
    INT32 plt_num = 0;
    for(BBL_SET::const_iterator iter = _pos_fixed_bbls.begin(); iter!=_pos_fixed_bbls.end(); iter++){
        F_SIZE second;
        F_SIZE target = (*iter)->get_bbl_offset(second);
        if(is_in_plt_in_off(target))
            plt_num++;
        
        if((*iter)->is_indirect_call() || (*iter)->is_indirect_jump() || (*iter)->is_ret()){
            if(!is_in_plt_in_off(target)){
                gadget_num++;
                if(!is_maybe_func_entry(*iter) && !(*iter)->is_ret())//ret is protected by shadow stack
                    little_gadget++;
            }
        }
    }
    
    INT32 movable_bbl_num = (INT32)_pos_movable_bbls.size();
    INT32 fixed_bbl_num = (INT32)_pos_fixed_bbls.size();
    INT32 sum = fixed_bbl_num + movable_bbl_num;
    INT32 fixed_bbl_wo_plt = fixed_bbl_num - plt_num;
    PRINT("%20s: Fixed bbls [without plt] (No.P: %2d%%) Sum: %4d Gadget:%4d, UsableGadget:%4d[isNotFuncEntry && isNotRet])\n", 
            get_name().c_str(), 100*fixed_bbl_wo_plt/sum, fixed_bbl_wo_plt, gadget_num, little_gadget);
}

void Module::dump_all_bbl_movable_info()
{   
    BLUE("Dump bbl movable info:\n");
    MODULE_MAP_ITERATOR it = _all_module_maps.begin();
    for(; it!=_all_module_maps.end(); it++){
         it->second->dump_bbl_movable_info();
    }
}

BasicBlock *Module::find_bbl_by_offset(F_SIZE offset, BOOL consider_prefix) const
{
    BBL_MAP_ITERATOR it = _bbl_maps.find(offset);
    if(it!=_bbl_maps.end())
        return it->second;
    else{
        if(consider_prefix){
            it = _bbl_maps.find(offset-1);
            if(it!=_bbl_maps.end() && it->second->has_lock_and_repeat_prefix())
                return it->second;
            else
                return NULL;
        }else
            return NULL;
    }
}

BasicBlock *Module::find_bbl_cover_offset(F_SIZE offset) const
{
    BBL_MAP_ITERATOR iter = _bbl_maps.upper_bound(offset);
    if(iter!=_bbl_maps.end() && (--iter)->second->is_in_bbl(offset))
        return iter->second;
    else
        return NULL;
}

BasicBlock *Module::find_bbl_cover_instr(Instruction *instr) const
{
    F_SIZE instr_offset = instr->get_instr_offset();
    BBL_MAP_ITERATOR iter = _bbl_maps.upper_bound(instr_offset);
    if(iter!=_bbl_maps.end() && (--iter)->second->is_in_bbl(instr_offset))
        return iter->second;
    else
        return NULL;
}

Instruction *Module::find_instr_cover_offset(F_SIZE offset) const
{
    INSTR_MAP_ITERATOR iter = _instr_maps.upper_bound(offset);
    if(iter!=_instr_maps.end() && (--iter)->second->is_in_instr(offset))
        return iter->second;
    else
        return NULL;
}

Instruction *Module::find_instr_by_off(F_SIZE offset, BOOL consider_prefix) const
{
    INSTR_MAP_ITERATOR instr_it = _instr_maps.find(offset);
    if(instr_it == _instr_maps.end()){
        if(consider_prefix){
            instr_it = _instr_maps.find(offset-1);
            if(instr_it!=_instr_maps.end() && instr_it->second->has_lock_and_repeat_prefix())
                return instr_it->second;
            else
                return NULL;
        }else
            return NULL;
    }else
        return instr_it->second;
}
