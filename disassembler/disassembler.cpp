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

void Disassembler::disassemble_module(Module *module)
{
    /*Use objdump tools to split the code and data*/
    // 1.generate command string
    char command[500];
    sprintf(command, "objdump -d %s | sed '/>:/d' | sed '/section/d' | sed '/format/d' | grep \":\" | cut -d ':' -f 1", \
        module->get_path().c_str());
    char type[3] = "r";
    // 2.get stream of command's output
    FILE *p_stream = popen(command, type);
    ASSERTM(p_stream, "popen() error!\n");
    // 4.parse the stream to get address of instructions
    SIZE len = 50;
    char *line_buf = new char[len];
    Instruction *instr = NULL;
    // objdump error : there are zero after ret and jmp instructions, used for align! 
    std::set<Instruction *> align_start_instrs;
    while(getline(&line_buf, &len, p_stream)!=-1) {
        // 4.1 get instruction offset
        P_ADDRX instr_addr;
        sscanf(line_buf, "%lx\n", &instr_addr);
        F_SIZE instr_off = instr_addr - module->get_pt_load_base();
        /* 4.2 skip! objdump result may contain two addresses which maps to one instruction
                  1642a2:       64 48 c7 45 00 00 00    movq   $0x0,%fs:0x0(%rbp)
                  1642a9:       00 00    
        */
        if(instr && (instr->get_next_offset() > instr_off))
            continue;
        // 4.3 disassemble instruction
        instr = disassemble_instruction(instr_off, module);
        // 4.4 record instruction
        if(instr){
            module->insert_instr(instr);
            // record branch targets
            if(instr->is_direct_call() || instr->is_condition_branch() || instr->is_direct_jump())
                module->insert_br_target(instr->get_target_offset());

            if(instr->is_ret() || instr->is_jump()){
                F_SIZE next_offset = instr->get_next_offset();
                if(module->is_in_x_section_in_off(next_offset) && module->read_x_byte_in_off(next_offset)==0)//align 0h
                    align_start_instrs.insert(instr);
            }
        }
    }
    free(line_buf);
    // 5.close stream
    pclose(p_stream);
    // 6. fix objdump errors
    // 6.1 fix align 0h 
    std::set<Instruction *>::iterator pick_one;
    std::vector<Instruction *> new_generated_instrs;
    while((pick_one = align_start_instrs.begin())!=align_start_instrs.end()){
        // padding 0h calculate!
        Instruction *instr = *pick_one;
        F_SIZE erase_start = instr->get_next_offset();
        ASSERT(instr);
        // calculate padding range    
        F_SIZE padding_begin = erase_start;
        F_SIZE padding_end = padding_begin;
        while(module->read_x_byte_in_off(padding_end)==0){
            padding_end++;
        };
        if(!is_0h_align(padding_end)){//aligned instruction is found
            align_start_instrs.erase(pick_one);
            continue;
        }
        F_SIZE erase_end = padding_end;
        //generated new instructions    
        while(!module->is_instr_entry_in_off(erase_end)){
            Instruction *new_instr = disassemble_instruction(erase_end, module);
            ASSERT(new_instr);
            new_generated_instrs.push_back(new_instr);
            erase_end = new_instr->get_next_offset();
        };
        //erase instructions
        std::vector<Instruction*> erased_instrs;
        module->erase_instr_range(erase_start, erase_end, erased_instrs);
        //if erased instructions are in align_start_instrs, erase them!
        align_start_instrs.erase(pick_one);
        for(std::vector<Instruction*>::iterator it = erased_instrs.begin(); it!=erased_instrs.end(); it++)
            align_start_instrs.erase(*it);
    }
    //record instructions
    for(std::vector<Instruction*>::iterator it = new_generated_instrs.begin();\
        it!=new_generated_instrs.end(); it++){
        Instruction *inst = *it;
        module->insert_instr(inst);
        // record branch targets
        if(inst->is_direct_call() || inst->is_condition_branch() || inst->is_direct_jump())
            module->insert_br_target(inst->get_target_offset());
    }
    // 6.2 fix br instructions' target is not aligned!
#if 0 // check br targets are true now    
fix_again: 
    F_SIZE br_target = 0;
    for(Module::BR_TARGETS_ITERATOR it = module->_br_targets.begin(); it!=module->_br_targets.end(); it++){
        if((*it)!=br_target)//multiset handling
            br_target = *it;
        else
            continue;
            
        F_SIZE target_offset = *it;
        if(!module->is_instr_entry_in_off(target_offset)){
            //judge is prefix. instruction or not 
            target_offset -= 1;
            Instruction *instr = module->get_instr_by_off(target_offset);
            
            if(!(instr && instr->has_prefix())){
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
                }while(!module->is_instr_entry_in_off(next_offset_of_target_instr));
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
                        module->insert_br_target(instruction->get_target_offset());
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
        module->check_br_targets();
    }
}

Instruction *Disassembler::disassemble_instruction(const F_SIZE instr_off, const Module *module)
{
    //init
    UINT8 *code = module->get_code_offset_ptr(instr_off);
    _ci.code = code;
    _ci.codeOffset = instr_off;
    //decompose the instruction
    UINT32 dinstcount = 0;
    if(code[0]==0xdf && code[1]==0xc0){//special handling
        _dInst.size = 2;
        _dInst.opcode = 0;
        _dInst.flags = 0;
        _dInst.meta &= (~0x7);//FC_NONE        
        _dInst.addr = instr_off;
        dinstcount = 1;
    }else
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
    }else
        return NULL;
}

void Disassembler::dump_pinst(const Instruction *instr, const P_ADDRX load_base)
{
    ASSERTM(instr, "instruction * cannot be NULL!\n");
    _dInst = instr->_dInst;
    _dInst.addr += load_base;
    const UINT8 *inst_code = instr->_encode;
    if(_dInst.size==2 && inst_code[0]==0xdf && inst_code[1]==0xc0){
        PRINT("%12lx (02) dfc0 %-20s FFREEP ST0\n", _dInst.addr, " ");
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
