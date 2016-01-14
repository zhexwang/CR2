
#include <string.h>

#include "utility.h"
#include "instruction.h"
#include "disassembler.h"
#include "module.h"
#include "instr_generator.h"

const std::string SequenceInstr::_type_name = "Sequence Instruction";
const std::string DirectCallInstr::_type_name = "Direct Call Instruction";
const std::string IndirectCallInstr::_type_name = "Indirect Call Instruction";
const std::string DirectJumpInstr::_type_name = "Direct Jump Instruction";
const std::string IndirectJumpInstr::_type_name = "Indirect Jump Instruction";
const std::string SysInstr::_type_name = "Sys Instruction";
const std::string IntInstr::_type_name = "Int Instruction";
const std::string CmovInstr::_type_name = "Cmovxx Instruction";
const std::string RetInstr::_type_name = "Ret Instruction";

Instruction::Instruction(const _DInst &dInst, const Module *module)
    : _dInst(dInst), _module(module)
{
    //copy instruction's encode
    _encode = new UINT8[_dInst.size]();
    void *ret = memcpy(_encode, module->get_code_offset_ptr(_dInst.addr), _dInst.size);
    FATAL(!ret, "memcpy failed!\n");
}

Instruction::~Instruction()
{
    free(_encode);
}

void Instruction::dump_pinst(const P_ADDRX load_base) const
{
    Disassembler::dump_pinst(this, load_base);
}

void Instruction::dump_file_inst() const
{
    Disassembler::dump_file_inst(this);
}

SequenceInstr::SequenceInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

SequenceInstr::~SequenceInstr()
{
    ;
}

static UINT16 find_disp_pos_from_encode(const UINT8 *encode, UINT8 size, INT32 disp32)
{
    INT64 rela_pos = 0;
    BOOL disp_match = false;
    INT32 *scan_start = (INT32*)encode;
    INT32 *disp_scanner = scan_start;
    INT32 *scan_end = (INT32*)((UINT64)encode + size -3);
    while(disp_scanner!=scan_end){
        if((*disp_scanner)==disp32){
            ASSERT(!disp_match);
            rela_pos = (INT64)disp_scanner - (INT64)scan_start;
            disp_match = true;
        }
        INT8 *increase = (INT8*)disp_scanner;//move one byte
        disp_scanner = (INT32*)(++increase);
    }
    FATAL(!disp_match, "displacement find failed!\n");
    return (UINT16)rela_pos;
}
/*
static UINT16 find_disp_pos_from_encode(const UINT8 *encode, UINT8 size, INT8 disp8)
{
    INT64 rela_pos = 0;
    BOOL disp_match = false;
    INT8 *scan_start = (INT8*)encode;
    INT8 *disp_scanner = scan_start;
    INT8 *scan_end = (INT8*)((UINT64)encode + size -1);
    while(disp_scanner!=scan_end){
        if((*disp_scanner)==disp8){
            ASSERT(!disp_match);
            rela_pos = (INT64)disp_scanner - (INT64)scan_start;
            disp_match = true;
        }
        disp_scanner++;
    }
    FATAL(!disp_match, "displacement find failed!\n");
    return (UINT16)rela_pos;
}
*/
std::string SequenceInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    if(is_rip_relative()){
        ASSERTM(_dInst.ops[0].type==O_SMEM || _dInst.ops[0].type==O_MEM  || _dInst.ops[1].type==O_SMEM || _dInst.ops[1].type==O_MEM
            ||_dInst.ops[2].type==O_SMEM || _dInst.ops[2].type==O_MEM, "unknown operand in RIP-operand!\n");
        ASSERTM(_dInst.dispSize==32, "we only handle the situation that the size of displacement=32\n");
        INT64 rela_pos = find_disp_pos_from_encode(_encode, _dInst.size, (INT32)_dInst.disp);
        INSTR_RELA rela_info = {RIP_RELA_TYPE, (UINT16)rela_pos, 4, (UINT16)(_dInst.size), (INT64)_dInst.disp};
        reloc_vec.push_back(rela_info);
    }

    return std::string((const char*)_encode, (SIZE)_dInst.size);
}

DirectCallInstr::DirectCallInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

DirectCallInstr::~DirectCallInstr()
{
    ;
}

std::string DirectCallInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    std::string instr_template;
    F_SIZE fallthrough_addr = get_fallthrough_offset();
    F_SIZE target_addr = get_target_offset();
    if(_module->is_shared_object()){//the offset between code cache and origin code region is lower than 4G
        /* @Shared Object Address is higher than 32bit
            movl ($ss_offset-0x4)(%rsp), fallthrough_addr_in_cc&0xfffffff       //shadow stack push low32 bits real return address
            movl ($ss_offset-0x8)(%rsp), (fallthrough_addr_in_cc>>32)&0xfffffff //shadow stack push high32 bits real return address  
            pushq fallthrough_addr_in_origin_code&0xffffffff                    //push low32 bits real return address 
            movl 0x4(%rsp), (fallthrough_addr_in_origin_code>>32)&0xffffffff    //push high32 bits real return address
            jmp  rel32                                                          //jump to the target function
        */
        UINT16 disp32_rela_pos;
        UINT16 imm32_rela_pos;
        UINT16 curr_pc = instr_template.length();
        //1. first template
          //1.1 generate instruction template
        std::string movl_rra_l32_template = InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(imm32_rela_pos, 0, disp32_rela_pos, 0);
          //1.2 calculate the base pc
        curr_pc += movl_rra_l32_template.length();
        disp32_rela_pos += instr_template.length();
        imm32_rela_pos += instr_template.length();
          //1.3 push relocation information
        INSTR_RELA rra_l32_rela_disp32 = {SS_RELA_TYPE, disp32_rela_pos, 4, curr_pc, -4};
        reloc_vec.push_back(rra_l32_rela_disp32);
        INSTR_RELA rra_l32_rela_imm32 = {LOW32_CC_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(rra_l32_rela_imm32);
          //1.4 merge the template
        instr_template += movl_rra_l32_template;
        //2. second template
          //2.1 generate instruction template
        std::string movl_rra_h32_template = InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(imm32_rela_pos, 0, disp32_rela_pos, 0);
          //2.2 calculate the base pc
        curr_pc += movl_rra_h32_template.length();
        disp32_rela_pos += instr_template.length();
        imm32_rela_pos += instr_template.length();
          //2.3 push relocation information
        INSTR_RELA rra_h32_rela_disp32 = {SS_RELA_TYPE, disp32_rela_pos, 4, curr_pc, -8};
        reloc_vec.push_back(rra_h32_rela_disp32);
        INSTR_RELA rra_h32_rela_imm32 = {HIGH32_CC_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(rra_h32_rela_imm32);
          //2.4 merge
        instr_template += movl_rra_h32_template;
        //3. third template  
          //3.1 generate the template
        std::string pushq_template = InstrGenerator::gen_pushq_imm32_instr(imm32_rela_pos, 0); 
          //3.2 calculate the base pc
        curr_pc += pushq_template.length();
        imm32_rela_pos += instr_template.length();
          //3.3 push relocation information
        INSTR_RELA ora_l32_rela_imm32 = {LOW32_ORG_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(ora_l32_rela_imm32);
          //3.4 merge
        instr_template += pushq_template;
        //4. fouth template  
          //4.1 generate the template
        UINT16 disp8_pos;
        std::string movl_ora_h32_template = InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(imm32_rela_pos, 0, disp8_pos, 4);
          //4.2 calculate the base pc
        curr_pc += movl_ora_h32_template.length();
        imm32_rela_pos += instr_template.length();
          //4.3 push relocation information
        INSTR_RELA ora_h32_rela_imm32 = {HIGH32_ORG_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(ora_h32_rela_imm32);
          //4.4 merge
        instr_template += movl_ora_h32_template;
    }else{
        /* @Executable Address is lower than 32bit, so we can optimze the code
            movq ($ss_offset-0x8)(%rsp), fallthrough_addr_in_cc&0xffffffff   //shadow stack push real return address
            pushq $fallthrough_addr_in_origin                                //push real return address
            jmp rel32                                                        //jump to the target function
        */ 
        UINT16 disp32_rela_pos;
        UINT16 imm32_rela_pos;
        UINT16 curr_pc = instr_template.length();
        //1. first template
          //1.1 generate instruction template
        std::string movq_rra_l32_template = InstrGenerator::gen_movq_imm32_to_rsp_smem_instr(imm32_rela_pos, 0, disp32_rela_pos, 0);
          //1.2 calculate the base pc
        curr_pc += movq_rra_l32_template.length();
        disp32_rela_pos += instr_template.length();
        imm32_rela_pos += instr_template.length();
          //1.3 push relocation information
        INSTR_RELA rra_l32_rela_disp32 = {SS_RELA_TYPE, disp32_rela_pos, 4, curr_pc, -8};
        reloc_vec.push_back(rra_l32_rela_disp32);
        INSTR_RELA rra_l32_rela_imm32 = {LOW32_CC_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(rra_l32_rela_imm32);
          //1.4 merge the template
        instr_template += movq_rra_l32_template;
        //2. second template
          //2.1 generate instruction template
        std::string pushq_template = InstrGenerator::gen_pushq_imm32_instr(imm32_rela_pos, 0);  
          //2.2 calculate the base pc
        curr_pc += pushq_template.length();
        imm32_rela_pos += instr_template.length();
          //2.3 push relocation information
        INSTR_RELA pushq_rela_imm32 = {LOW32_ORG_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(pushq_rela_imm32);
          //2.4 merge the template
        instr_template += pushq_template;
    }
    //jmp rel32 template generation
    UINT16 rel32_rela_pos;
    //1. generate the template
    std::string jmp_rel32_template    = InstrGenerator::gen_jump_rel32_instr(rel32_rela_pos, 0);
    //2. calculate the base pc
    UINT16 base_pc = instr_template.length() + jmp_rel32_template.length();
    rel32_rela_pos += instr_template.length();
    //3. push relocation information
    INSTR_RELA jmp_rel32_rela_rel32 = {BRANCH_RELA_TYPE, rel32_rela_pos, 4, base_pc, (INT64)target_addr}; 
    reloc_vec.push_back(jmp_rel32_rela_rel32);
    //4. merge
    instr_template += jmp_rel32_template;
    
    return instr_template;
}

IndirectCallInstr::IndirectCallInstr(const _DInst &dInst, const Module *module)
     : Instruction(dInst, module)
{
    ;
}

IndirectCallInstr::~IndirectCallInstr()
{
    ;
}

std::string IndirectCallInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    ASSERTM(_dInst.opcode!=I_CALL_FAR, "we only handle call near!\n");
    std::string instr_template;
    F_SIZE fallthrough_addr = get_fallthrough_offset();
    UINT16 curr_pc = 0;
    if(_module->is_shared_object()){//the offset between code cache and origin code region is lower than 4G
        /* @Shared Object Address is higher than 32bit
            movl ($ss_offset-0x4)(%rsp), fallthrough_addr_in_cc&0xfffffff       //shadow stack push low32 bits real return address
            movl ($ss_offset-0x8)(%rsp), (fallthrough_addr_in_cc>>32)&0xfffffff //shadow stack push high32 bits real return address  
            pushq fallthrough_addr_in_origin_code&0xffffffff                    //push low32 bits real return address 
            movl 0x4(%rsp), (fallthrough_addr_in_origin_code>>32)&0xffffffff    //push high32 bits real return address
            pushq mem                                                           (take care of the base register is rsp)
            addq mem, $cc_offset
            ret
        */
        UINT16 disp32_rela_pos;
        UINT16 imm32_rela_pos;
        //1. first template
          //1.1 generate instruction template
        std::string movl_rra_l32_template = InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(imm32_rela_pos, 0, disp32_rela_pos, 0);
          //1.2 calculate the base pc
        curr_pc += movl_rra_l32_template.length();
        disp32_rela_pos += instr_template.length();
        imm32_rela_pos += instr_template.length();
          //1.3 push relocation information
        INSTR_RELA rra_l32_rela_disp32 = {SS_RELA_TYPE, disp32_rela_pos, 4, curr_pc, -4};
        reloc_vec.push_back(rra_l32_rela_disp32);
        INSTR_RELA rra_l32_rela_imm32 = {LOW32_CC_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(rra_l32_rela_imm32);
          //1.4 merge the template
        instr_template += movl_rra_l32_template;
        //2. second template
          //2.1 generate instruction template
        std::string movl_rra_h32_template = InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(imm32_rela_pos, 0, disp32_rela_pos, 0);
          //2.2 calculate the base pc
        curr_pc += movl_rra_h32_template.length();
        disp32_rela_pos += instr_template.length();
        imm32_rela_pos += instr_template.length();
          //2.3 push relocation information
        INSTR_RELA rra_h32_rela_disp32 = {SS_RELA_TYPE, disp32_rela_pos, 4, curr_pc, -8};
        reloc_vec.push_back(rra_h32_rela_disp32);
        INSTR_RELA rra_h32_rela_imm32 = {HIGH32_CC_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(rra_h32_rela_imm32);
          //2.4 merge
        instr_template += movl_rra_h32_template;
        //3. third template  
          //3.1 generate the template
        std::string pushq_template = InstrGenerator::gen_pushq_imm32_instr(imm32_rela_pos, 0); 
          //3.2 calculate the base pc
        curr_pc += pushq_template.length();
        imm32_rela_pos += instr_template.length();
          //3.3 push relocation information
        INSTR_RELA ora_l32_rela_imm32 = {LOW32_ORG_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(ora_l32_rela_imm32);
          //3.4 merge
        instr_template += pushq_template;
        //4. fouth template  
          //4.1 generate the template
        UINT16 disp8_pos;
        std::string movl_ora_h32_template = InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(imm32_rela_pos, 0, disp8_pos, 4);
          //4.2 calculate the base pc
        curr_pc += movl_ora_h32_template.length();
        imm32_rela_pos += instr_template.length();
          //4.3 push relocation information
        INSTR_RELA ora_h32_rela_imm32 = {HIGH32_ORG_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(ora_h32_rela_imm32);
          //4.4 merge
        instr_template += movl_ora_h32_template;
    }else{
        /* @Executable Address is lower than 32bit, so we can optimze the code
            movq ($ss_offset-0x8)(%rsp), fallthrough_addr_in_cc&0xffffffff   //shadow stack push real return address
            pushq $fallthrough_addr_in_origin                                //push real return address
            pushq mem                                                           (take care of the base register is rsp)
            addq mem, $cc_offset
            ret
        */ 
        UINT16 disp32_rela_pos;
        UINT16 imm32_rela_pos;
        //1. first template
          //1.1 generate instruction template
        std::string movq_rra_l32_template = InstrGenerator::gen_movq_imm32_to_rsp_smem_instr(imm32_rela_pos, 0, disp32_rela_pos, 0);
          //1.2 calculate the base pc
        curr_pc += movq_rra_l32_template.length();
        disp32_rela_pos += instr_template.length();
        imm32_rela_pos += instr_template.length();
          //1.3 push relocation information
        INSTR_RELA rra_l32_rela_disp32 = {SS_RELA_TYPE, disp32_rela_pos, 4, curr_pc, -8};
        reloc_vec.push_back(rra_l32_rela_disp32);
        INSTR_RELA rra_l32_rela_imm32 = {LOW32_CC_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(rra_l32_rela_imm32);
          //1.4 merge the template
        instr_template += movq_rra_l32_template;
        //2. second template
          //2.1 generate instruction template
        std::string pushq_template = InstrGenerator::gen_pushq_imm32_instr(imm32_rela_pos, 0);  
          //2.2 calculate the base pc
        curr_pc += pushq_template.length();
        imm32_rela_pos += instr_template.length();
          //2.3 push relocation information
        INSTR_RELA pushq_rela_imm32 = {LOW32_ORG_RELA_TYPE, imm32_rela_pos, 4, curr_pc, (INT64)fallthrough_addr};
        reloc_vec.push_back(pushq_rela_imm32);
          //2.4 merge the template
        instr_template += pushq_template;
    }
    /*
        pushq mem/reg          (take care of the base register is rsp in mem operand, because the rsp has already decrease the rsp)
        addq (%rsp), $cc_offset
        ret
    */
    std::string push_template;
    if(_dInst.ops[0].type==O_REG){
        ASSERT(_dInst.ops[0].index!=R_RSP);//rsp has changed due to push return address onto the main stack, so this convert function is not safe!
        push_template = InstrGenerator::convert_callin_reg_to_push_reg(_encode, _dInst.size);
    }else{
        //convert callin mem to push mem
        push_template = InstrGenerator::convert_callin_mem_to_push_mem(_encode, _dInst.size);
        if(is_rip_relative()){//add relocation information if the instruction is rip relative
            ASSERTM(_dInst.dispSize==32, "we only handle the situation that the size of displacement=32\n");
            UINT16 rela_push_pos = find_disp_pos_from_encode((const UINT8 *)push_template.c_str(), _dInst.size, (INT32)_dInst.disp);
            rela_push_pos += (UINT16)instr_template.length();
            UINT16 push_base_pos = (UINT16)(instr_template.length() + push_template.length());
            INSTR_RELA rela_push = {RIP_RELA_TYPE, rela_push_pos, 4, push_base_pos, (INT64)_dInst.disp};
            reloc_vec.push_back(rela_push);
        }else{//judge if has rsp or not
            UINT8 check_reg = R_NONE;
            if(_dInst.ops[0].type==O_SMEM)
                check_reg = _dInst.ops[0].index;
            else if(_dInst.ops[0].type==O_MEM){
                check_reg = _dInst.base;
                ASSERT(_dInst.ops[0].index!=R_RSP);
            }

            if(check_reg==R_RSP)
                push_template = InstrGenerator::modify_disp_of_pushq_rsp_mem(push_template, 8);
        }
    }

    curr_pc += push_template.length();
    instr_template += push_template;
    //addq 
    UINT16 rela_addq_pos;
    std::string addq_template = InstrGenerator::gen_addq_imm32_to_rsp_mem_instr(rela_addq_pos, 0);
    
    rela_addq_pos += (UINT16)instr_template.length();
    curr_pc += addq_template.length();
    INSTR_RELA rela_addq = {CC_RELA_TYPE, rela_addq_pos, 4, curr_pc, 0};
    
    reloc_vec.push_back(rela_addq);
    instr_template += addq_template;
    //retq
    std::string retq_template = InstrGenerator::gen_retq_instr();
    instr_template += retq_template;
    
    return instr_template;
}

DirectJumpInstr::DirectJumpInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

DirectJumpInstr::~DirectJumpInstr()
{
    ;
}

std::string DirectJumpInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    UINT16 r_byte_pos;
    std::string jmp_rel32_template = InstrGenerator::gen_jump_rel32_instr(r_byte_pos, 0);
    
    UINT16 r_base_pc = jmp_rel32_template.length();
    INT64 r_value = (INT64)get_target_offset();
    
    INSTR_RELA rela = {BRANCH_RELA_TYPE, r_byte_pos, 4, r_base_pc, r_value};
    reloc_vec.push_back(rela);
    return jmp_rel32_template;
}

IndirectJumpInstr::IndirectJumpInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

IndirectJumpInstr::~IndirectJumpInstr()
{
    ;
}

std::string IndirectJumpInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    ASSERTM(_dInst.opcode!=I_JMP_FAR, "we only handle jmp near!\n");
    
    std::string instr_template;
    std::set<F_SIZE> targets = _module->get_indirect_jump_targets(_dInst.addr);
    BOOL has_recognized_targets = targets.size()==0 ? false : true;

    std::string push_template;
    if(_dInst.ops[0].type==O_REG){
        push_template = InstrGenerator::convert_jumpin_reg_to_push_reg(_encode, _dInst.size);
    }else{
        //1. convert jumpin mem to push mem
        push_template = InstrGenerator::convert_jumpin_mem_to_push_mem(_encode, _dInst.size);
        if(is_rip_relative()){//add relocation information if the instruction is rip relative
            ASSERTM(_dInst.dispSize==32, "we only handle the situation that the size of displacement=32\n");
            UINT16 rela_push_pos = find_disp_pos_from_encode((const UINT8 *)push_template.c_str(), _dInst.size, (INT32)_dInst.disp);
            rela_push_pos += (UINT16)instr_template.length();
            UINT16 push_base_pos = (UINT16)(instr_template.length() + push_template.length());
            INSTR_RELA rela_push = {RIP_RELA_TYPE, rela_push_pos, 4, push_base_pos, (INT64)_dInst.disp};
            reloc_vec.push_back(rela_push);
        }
        instr_template += push_template;
        //2. add code cache offset or trampoline offset
        UINT16 rela_addq_pos;
        std::string addq_template = InstrGenerator::gen_addq_imm32_to_rsp_mem_instr(rela_addq_pos, 0);
        rela_addq_pos += (UINT16)instr_template.length();
        UINT16 addq_base_pos = (UINT16)(instr_template.length() + addq_template.length());
        if(has_recognized_targets){//recoginized jmpin
            INSTR_RELA rela_addq = {TRAMPOLINE_RELA_TYPE, rela_addq_pos, 4, addq_base_pos, 0};
            reloc_vec.push_back(rela_addq);
        }else{
            INSTR_RELA rela_addq = {CC_RELA_TYPE, rela_addq_pos, 4, addq_base_pos, 0};
            reloc_vec.push_back(rela_addq);
        }
        instr_template += addq_template;
        //3. gen return instruction
        std::string retq_template = InstrGenerator::gen_retq_instr();
        instr_template += retq_template;
    }

    return instr_template;
}

ConditionBrInstr::ConditionBrInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

ConditionBrInstr::~ConditionBrInstr()
{
    ;
}

std::string ConditionBrInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{  /*            
    * Indicates the instruction is one of:
    * JCXZ(32mode), JECXZ, JRCXZ, JO, JNO, JB, JAE, JZ, JNZ, JBE, JA, JS, JNS, JP, JNP, JL, JGE, JLE, JG, LOOP, LOOPZ, LOOPNZ.                                                            
    */
    F_SIZE target_offset = get_target_offset();
    F_SIZE fallthrough_offset = get_fallthrough_offset();
    std::string instr_template;
    UINT16 curr_pc = 0;
    //1. target
    switch(_dInst.opcode){
        case I_JA: case I_JAE: case I_JB: case I_JBE: case I_JG:
        case I_JGE: case I_JL: case I_JLE: case I_JNO: case I_JNP: case I_JNS: case I_JNZ:
        case I_JO: case I_JP: case I_JS: case I_JZ://can be convert to rel32
            {
                UINT16 cbr_rel32_rela_pos;
                std::string cbr_template = InstrGenerator::convert_cond_br_relx_to_rel32(_encode, _dInst.size, cbr_rel32_rela_pos, 0);

                curr_pc += cbr_template.length();
                cbr_rel32_rela_pos += instr_template.length();
                
                INSTR_RELA cbr_rela = {BRANCH_RELA_TYPE, cbr_rel32_rela_pos, 4, curr_pc, (INT64)target_offset};
                reloc_vec.push_back(cbr_rela);
                
                instr_template += cbr_template;
            }
            break;
        case I_LOOP: case I_LOOPZ: case I_LOOPNZ: case I_JCXZ: case I_JRCXZ://can only be convert to rel8
            {
                UINT16 cbr_rel8_rela_pos;
                std::string cbr_template = InstrGenerator::convert_cond_br_relx_to_rel8(_encode, _dInst.size, cbr_rel8_rela_pos, 0);

                curr_pc += cbr_template.length();
                cbr_rel8_rela_pos += instr_template.length();

                INSTR_RELA cbr_rela = {BRANCH_RELA_TYPE, cbr_rel8_rela_pos, 1, curr_pc, (INT64)target_offset};
                reloc_vec.push_back(cbr_rela);

                instr_template += cbr_template;
            }
            break;
        default:    
            dump_file_inst();
            ASSERTM(0, "unkown condition branch instruction!\n");
           
    }
    //2. fallthrough
    UINT16 jmp_rel32_rela_pos;
    std::string jmp_rel32_template = InstrGenerator::gen_jump_rel32_instr(jmp_rel32_rela_pos, 0);
    
    curr_pc += jmp_rel32_template.length();
    jmp_rel32_rela_pos += instr_template.length();
    
    INSTR_RELA rela = {BRANCH_RELA_TYPE, jmp_rel32_rela_pos, 4, curr_pc, (INT64)fallthrough_offset};
    reloc_vec.push_back(rela);

    instr_template += jmp_rel32_template;
    return instr_template;
}

RetInstr::RetInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

RetInstr::~RetInstr()
{
    ;
}

std::string RetInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    /*  addq %rsp, $0x8               //pop the main stack
        jmpq ($ss_offset-0x8)(%rsp)   //jump to the real return address
    */ 
    std::string instr_template;
    UINT16 curr_pc = 0;
    //addq instruction 
    UINT16 imm8_rela_pos;
    std::string addq_template = InstrGenerator::gen_addq_imm8_to_rsp_instr(imm8_rela_pos, 8);
    curr_pc += addq_template.length();
    instr_template += addq_template;
    //jmpq instruction
    UINT16 disp32_rela_pos;
    std::string jmpq_template = InstrGenerator::gen_jmpq_rsp_smem(disp32_rela_pos, 0);
    disp32_rela_pos += instr_template.length();
    curr_pc += jmpq_template.length();

    INSTR_RELA rela = {SS_RELA_TYPE, disp32_rela_pos, 4, curr_pc, -8};
    reloc_vec.push_back(rela);

    instr_template += jmpq_template;
    return instr_template;
}

SysInstr::SysInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

SysInstr::~SysInstr()
{
    ;
}

std::string SysInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    return std::string((const char*)_encode, (SIZE)_dInst.size);
}

CmovInstr::CmovInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}

CmovInstr::~CmovInstr()
{
    ;
}

std::string CmovInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    if(is_rip_relative()){
        ASSERTM(_dInst.ops[0].type==O_SMEM || _dInst.ops[0].type==O_MEM  || _dInst.ops[1].type==O_SMEM || _dInst.ops[1].type==O_MEM
            ||_dInst.ops[2].type==O_SMEM || _dInst.ops[2].type==O_MEM, "unknown operand in RIP-operand!\n");
        ASSERTM(_dInst.dispSize==32, "we only handle the situation that the size of displacement=32\n");
        INT64 rela_pos = find_disp_pos_from_encode(_encode, _dInst.size, (INT32)_dInst.disp);
        INSTR_RELA rela_info = {RIP_RELA_TYPE, (UINT16)rela_pos, 4, (UINT16)(_dInst.size), (INT64)_dInst.disp};
        reloc_vec.push_back(rela_info);
    }

    return std::string((const char*)_encode, (SIZE)_dInst.size);
}

IntInstr::IntInstr(const _DInst &dInst, const Module *module)
    : Instruction(dInst, module)
{
    ;
}
IntInstr::~IntInstr()
{
    ;
}
	
std::string IntInstr::generate_instr_template(std::vector<INSTR_RELA> &reloc_vec) const
{
    return std::string((const char*)_encode, (SIZE)_dInst.size);
}

