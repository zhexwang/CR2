#include <string.h>

#include "disassembler.h"
#include "elf-parser.h"
#include "instruction.h"
#include "module.h"

_DInst Disassembler::_dInst;
_CodeInfo Disassembler::_ci;
_DecodedInst Disassembler::_decodedInst;

void Disassembler::init()
{
    _ci.codeLen = 0x1000;
    _ci.dt = Decode64Bits;
    _ci.features = DF_NONE;
}

static BOOL is_0h_align(F_SIZE offset)
{
    return (offset&(0xfull))==0;
}

static BOOL objdump_error_correction(Module *module, P_ADDRX instr_addr)
{
    if(module->get_name()=="dedup"){
        if((instr_addr>=0x407933) && (instr_addr<0x407980)){//this region is data, we should skip it 
            //ERR("%lx\n", instr_addr);
            return false;
        }else
            return true;
    }else
        return true;
}

void Disassembler::disassemble_module(Module *module)
{
    /*Use objdump tools to split the code and data*/
    // 1.generate command string
    char command[500];
    sprintf(command, "objdump -d %s | sed '/>:/d' | sed '/Disassembly/d' | sed '/elf64-x86-64/d' | grep \":\"", \
        module->get_path().c_str());
    char type[3] = "r";
    // 2.get stream of command's output
    FILE *p_stream = popen(command, type);
    ASSERTM(p_stream, "popen() error!\n");
    // 4.parse the stream to get address of instructions
    SIZE len = 5000;
    char *line_buf = new char[len];
    Instruction *instr = NULL;
    BOOL has_nop_instrs = false;
    char *pos = line_buf;
    // objdump error : there are numbers of 0h after ret and jmp instructions, used for align! 
    std::set<Instruction *> maybe_0h_align_start_instrs;
    while(getline(&line_buf, &len, p_stream)!=-1) {
        // 4.1 get instruction offset
        P_ADDRX instr_addr;
        sscanf(line_buf, "%lx\n", &instr_addr);
        // skip data region
        if(!objdump_error_correction(module, instr_addr))
            continue;
        /* 4.2 skip! objdump result may contain two addresses which maps to one instruction
                  1642a2:       64 48 c7 45 00 00 00    movq   $0x0,%fs:0x0(%rbp)
                  1642a9:       00 00    
        */
        char *push_pos = strstr(line_buf, "push");
        if(push_pos){
            if(pos<=push_pos){
                if(!strstr(line_buf, "<"))
                    pos = push_pos;
            }else
                continue;
        }else{
            if(pos>=(line_buf+strlen(line_buf)))                                                                                                                        
                continue;
        }

        F_SIZE instr_off = module->convert_pt_addr_to_offset(instr_addr);
        // 4.3 disassemble instruction
        instr = disassemble_instruction(instr_off, module, line_buf);
        // 4.4 record instruction
        if(instr){
            module->insert_instr(instr);
            // 4.4.1 record branch targets
            if(instr->is_direct_call() || instr->is_condition_branch() || instr->is_direct_jump())
                module->insert_br_target(instr->get_target_offset(), instr_off);
            // 4.4.2 record maybe 0h aligned entries
            if(instr->is_ret() || instr->is_jump() || instr->is_ud2() || instr->is_hlt()){
                F_SIZE next_offset = instr->get_next_offset();
                if(module->is_in_x_section_in_off(next_offset) && module->read_1byte_code_in_off(next_offset)==0)//align 0h
                    maybe_0h_align_start_instrs.insert(instr);
            }
            // 4.4.3 record nop aligned entries
            if(instr->is_nop())
                has_nop_instrs = true;
            else{
                if(has_nop_instrs)// && is_0h_align(instr_off))
                    module->insert_align_entry(instr_off);
                has_nop_instrs = false;
            }
            // 4.4.4 record indirect jump
            if(instr->is_indirect_jump())
                module->insert_indirect_jump(instr->get_instr_offset());
            // 4.4.5 record direct call target(maybe function)
            if(instr->is_direct_call())
                module->insert_call_target(instr->get_target_offset());
            // 4.4.6 record gs segmentation
            if(instr->has_gs_seg()){
                module->insert_gs_instr_offset(instr_off);
            }
        }
    }
    free(line_buf);
    // 5.close stream
    pclose(p_stream);
    // there maybe some instructions that are not disassembled!
    //special_handle_vminstr(module);
    // 6. fix objdump error1 : due to [10h 20h 30h ...] align. 
    // Warnning: there is a likey bug (erase recognized switch jump)
    std::set<Instruction *>::iterator pick_one;
    std::vector<Instruction *> new_generated_instrs;
    std::vector<F_SIZE> new_generated_align_instrs;
    // 6.1 search all likely 0h aligned entries
    while((pick_one = maybe_0h_align_start_instrs.begin())!=maybe_0h_align_start_instrs.end()){
        Instruction *instr = *pick_one;
        F_SIZE erase_start = instr->get_next_offset();
        // 6.1.1 calculate 0h aligned padding range    
        F_SIZE padding_begin = erase_start;
        F_SIZE padding_end = padding_begin;
        while(module->read_1byte_code_in_off(padding_end)==0){
            padding_end++;
        };
        // 6.1.2 judge is aligned instruction or not
        if(!is_0h_align(padding_end)){//aligned instruction is found
            maybe_0h_align_start_instrs.erase(pick_one);
            continue;
        }
        F_SIZE erase_end = padding_end;
        // 6.1.3 record aligned entries
        module->insert_align_entry(erase_end);
        // 6.1.4 regenerated new instructions to subtitute the objdump results    
        Instruction *new_instr = NULL;
        while(!module->is_instr_entry_in_off(erase_end, false)){
            new_instr = disassemble_instruction(erase_end, module);
            ASSERT(new_instr);
            new_generated_instrs.push_back(new_instr);
            erase_end = new_instr->get_next_offset();
            // record nop aligned entries
            if(new_instr->is_nop())
                has_nop_instrs = true;
            else{
                if(has_nop_instrs)// && is_0h_align(instr_off))
                    new_generated_align_instrs.push_back(new_instr->get_instr_offset());
                has_nop_instrs = false;
            }
            // record gs segmentation
            if(instr->has_gs_seg()){
                module->insert_gs_instr_offset(erase_end);
                ASSERTM(0, "find the instruction (%lx) with gs segmenation in module %s\n", erase_end, module->get_path().c_str());
            }
        };
        if(has_nop_instrs){
            new_generated_align_instrs.push_back(new_instr->get_next_offset());
            has_nop_instrs = false;
        }
        // 6.1.5 erase instructions
        std::vector<Instruction*> erased_instrs;
        module->erase_instr_range(erase_start, erase_end, erased_instrs);
        //if erased instructions are in align_start_instrs, erase them!
        maybe_0h_align_start_instrs.erase(pick_one);
        for(std::vector<Instruction*>::iterator it = erased_instrs.begin(); it!=erased_instrs.end(); it++)
            maybe_0h_align_start_instrs.erase(*it);
    }
    // 6.2 record new generated instructions
    for(std::vector<Instruction*>::iterator it = new_generated_instrs.begin();\
        it!=new_generated_instrs.end(); it++){
        Instruction *inst = *it;
        module->insert_instr(inst);
        // record branch targets
        if(inst->is_direct_call() || inst->is_condition_branch() || inst->is_direct_jump())
            module->insert_br_target(inst->get_target_offset(), inst->get_instr_offset());
    }
    // 6.3 record new aligned instructions
    for(std::vector<F_SIZE>::iterator it = new_generated_align_instrs.begin();\
        it!=new_generated_align_instrs.end(); it++)
        module->insert_align_entry(*it);
    // 7. fix objdump error2 : due to br instructions' targets are not disassemble instruction aligned!
#if 0 // check br targets are true now, if check_br_targets failed, you should open these codes    
fix_again: 
    F_SIZE br_target = 0;
    for(Module::BR_TARGETS_ITERATOR it = module->_br_targets.begin(); it!=module->_br_targets.end(); it++){
        if((*it)!=br_target)//multiset handling
            br_target = *it;
        else
            continue;
            
        F_SIZE target_offset = *it;
        if(!module->is_instr_entry_in_off(target_offset, false)){
            //judge is prefix. instruction or not 
            target_offset -= 1;
            Instruction *instr = module->get_instr_by_off(target_offset);
            
            if(!(instr && instr->has_lock_and_repeat_prefix())){
                target_offset ++;
                // fix objdump error!
                std::vector<Instruction *> new_generated_instr;
                Instruction *target_instr = NULL;
                F_SIZE next_offset_of_target_instr = target_offset;
                do{
                    target_instr = disassemble_instruction(next_offset_of_target_instr, module);
                    ASSERT(target_instr);
                    target_instr->dump_file_inst();
                    new_generated_instr.push_back(target_instr);
                    next_offset_of_target_instr = target_instr->get_next_offset();
                }while(!module->is_instr_entry_in_off(next_offset_of_target_instr, false));
                //erase
                std::vector<Instruction*> erased_instrs;
                module->erase_instr_range(target_offset, next_offset_of_target_instr, erased_instrs);
                //record instructions    
                BOOL br_target_is_changed = false;
                for(std::vector<Instruction*>::iterator it = new_generated_instr.begin();\
                    it!=new_generated_instr.end(); it++){
                    Instruction *instruction = *it;
                    module->insert_instr(instruction);
                    // record branch targets
                    if(instruction->is_direct_call() || instruction->is_condition_branch() || instruction->is_direct_jump()){
                        br_target_is_changed = true;
                        module->insert_br_target(instruction->get_target_offset(), instruction->get_instr_offset());
                    }
                }
                if(br_target_is_changed)
                    goto fix_again;
            }
        }
    }
#endif    
}

void Disassembler::disassemble_all_modules()
{
    ElfParser::PARSED_ELF_ITERATOR it = ElfParser::_all_parsed_elfs.begin();
    for(;it!=ElfParser::_all_parsed_elfs.end();it++){
        ElfParser *elf = it->second;
        Module *module = new Module(elf);
        disassemble_module(module);
        if(module->get_name()=="freqmine"){
            Instruction *instr = module->find_instr_by_off(0x1ed08, false);
            module->erase_br_target(instr->get_target_offset(), 0x1ed08);
            module->erase_instr(instr);
        }
        module->check_br_targets();
    }
}

Instruction *Disassembler::disassemble_instruction(const F_SIZE instr_off, const Module *module, \
    const char *objdump_line_buf)
{
    //init
    UINT8 *code = module->get_code_offset_ptr(instr_off);
    _ci.code = code;
    _ci.codeOffset = instr_off;
    //decompose the instruction
    UINT32 dinstcount = 0;
    if(code[0]==0xdf && code[1]==0xc0){//FFREEP ST0
        _dInst.size = 2;
        _dInst.opcode = I_UNDEFINED;
        _dInst.flags = 0;
        _dInst.meta &= (~0x7);//FC_NONE        
        _dInst.addr = instr_off;
        dinstcount = 1;
    }else if(code[0]==0xc7 && code[1]==0xf8 && (*(INT32*)&code[2])==0){//xbeginq
        _dInst.size = 6;
        _dInst.opcode = I_UNDEFINED;
        _dInst.flags = 0;
        _dInst.meta &= (~0x7);//FC_NONE        
        _dInst.addr = instr_off;
        dinstcount = 1;
    }else if(code[0]==0xc6 && code[1]==0xf8){//xabort
        _dInst.size = 3;
        _dInst.opcode = I_UNDEFINED;
        _dInst.flags = 0;
        _dInst.meta &= (~0x7);//FC_NONE        
        _dInst.addr = instr_off;
        dinstcount = 1;
    }else if(code[0]==0x0f && code[1]==0x1 && code[2]==0xd5){//xend
        _dInst.size = 3;
        _dInst.opcode = I_UNDEFINED;
        _dInst.flags = 0;
        _dInst.meta &= (~0x7);//FC_NONE        
        _dInst.addr = instr_off;
        dinstcount = 1;
    }else//special handling vfmaddsd
        distorm_decompose(&_ci, &_dInst, 1, &dinstcount);
    ASSERT(_dInst.addr == instr_off);

    if(dinstcount==1){
		switch(META_GET_FC(_dInst.meta)){
            case FC_NONE:
                return new SequenceInstr(_dInst, module);
			case FC_CALL:
                {
                    if(_dInst.ops[0].type==O_PC)//direct call
                        return new DirectCallInstr(_dInst, module);
                    else
                        return new IndirectCallInstr(_dInst, module);
                }
			case FC_RET:
                return new RetInstr(_dInst, module);
            case FC_SYS:
                return new SysInstr(_dInst, module);
			case FC_UNC_BRANCH:
                {
                    if(_dInst.ops[0].type==O_PC)//direct jump
                        return new DirectJumpInstr(_dInst, module);
                    else
                        return new IndirectJumpInstr(_dInst, module);
                }
			case FC_CND_BRANCH:
				return new ConditionBrInstr(_dInst, module);
            case FC_INT:
                return new IntInstr(_dInst, module);
			case FC_CMOV:
				return new CmovInstr(_dInst, module);
            default:
                ASSERTM(0, "unkown type!\n");
            return NULL;
		}
    }else{
        //failed diasm instructions
        const static std::string failed_disasm_instrs[4] = {//only for vector instructions
            "vfnmaddsd",
            "vfmaddsd",
            "vfmaddss",
            "vfmsubsd",
        };
        if(!objdump_line_buf)
            return NULL;
        //find failed instructions, if found generate it!
        for(INT32 idx = 0; idx<4; idx++){
            if(strstr(objdump_line_buf, failed_disasm_instrs[idx].c_str())){
                BOOL search_rip_relative_instr = strstr(objdump_line_buf, "rip") ? true : false;
                BOOL search_sib_relative_instr = strstr(objdump_line_buf, ")") ? true : false;
                BOOL search_rsp_sib_relative_instr = strstr(objdump_line_buf, "rsp") ? true : false;
                //generate new instruction
                _dInst.opcode = I_UNDEFINED;  
                _dInst.addr = instr_off;
                _dInst.meta &= (~0x7);//FC_NONE      
                if(search_rip_relative_instr){
                    _dInst.size = 10;
                    _dInst.flags = FLAG_RIP_RELATIVE;
                    _dInst.ops[0].type = O_SMEM;
                    const char *disp = strstr(objdump_line_buf, "0x");
                    ASSERT(disp);
                    char *stopstring;
                    INT64 disp64 = strtol(disp, &stopstring, 0);
                    ASSERT(((disp64 > 0 ? disp64 : -disp64) > 0x7f));//dispSize==32
                    _dInst.dispSize = 32;
                    _dInst.disp = (UINT64)disp64;
                }else{
                    _dInst.size = search_sib_relative_instr ? (search_rsp_sib_relative_instr ? 8 : 7) : 6;
                    _dInst.flags = 0;
                }
                return new SequenceInstr(_dInst, module);
            }
        }

        return NULL;
    }
}

void Disassembler::dump_pinst(const Instruction *instr, const P_ADDRX load_base)
{
    ASSERTM(instr, "instruction * cannot be NULL!\n");
    _dInst = instr->_dInst;
    _dInst.addr += load_base;
    const UINT8 *inst_code = instr->_encode;
    if(_dInst.opcode==I_UNDEFINED){
        PRINT("%12lx (%02d) FAILED DISASM INSTR\n", _dInst.addr, _dInst.size);
    }else{
        UINT32 decodedInstsCount = 0;
        distorm_decode(_dInst.addr, inst_code, _dInst.size, Decode64Bits, &_decodedInst, _dInst.size, &decodedInstsCount);
        ASSERT(decodedInstsCount==1);
        PRINT("%12lx (%02d) %-24s  %s%s%s\n", _decodedInst.offset, _decodedInst.size, _decodedInst.instructionHex.p,\
            (char*)_decodedInst.mnemonic.p, _decodedInst.operands.length != 0 ? " " : "", (char*)_decodedInst.operands.p);
    }
}

void Disassembler::dump_file_inst(const Instruction *instr)
{
    dump_pinst(instr, 0);
}

void Disassembler::dump_string(const std::string str, P_ADDRX load_base)
{
    SIZE disasm_block_size = str.length();
    _DecodedInst  *disassembled = new _DecodedInst[disasm_block_size];
    UINT32 decodedInstsCount = 0;
    distorm_decode(load_base, (const UINT8*)str.c_str(), disasm_block_size, Decode64Bits, \
        disassembled, disasm_block_size, &decodedInstsCount);
    for(UINT32 i=0; i<decodedInstsCount; i++){
     PRINT("%12lx (%02d) %-24s  %s%s%s\n", disassembled[i].offset, disassembled[i].size, disassembled[i].instructionHex.p,\
         (char*)disassembled[i].mnemonic.p, disassembled[i].operands.length != 0 ? " " : "", (char*)disassembled[i].operands.p);
    }
    delete [] disassembled;
}

