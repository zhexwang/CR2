#include "instr_generator.h"
#include "disasm_common.h"

std::string InstrGenerator::gen_addq_rax_imm32_instr(UINT16 &imm32_pos, INT32 imm32)
{
    UINT8 array[6] = {0x48, 0x05, (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), (UINT8)((imm32>>16)&0xff), \
        (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 2;
    return std::string((const INT8*)array, 6);
}

std::string InstrGenerator::gen_movl_imm32_to_mem32_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &mem32_pos, INT32 mem32)
{
    UINT8 array[11] = {0xc7, 0x04, 0x25, (UINT8)(mem32&0xff), (UINT8)((mem32>>8)&0xff), (UINT8)((mem32>>16)&0xff), \
        (UINT8)((mem32>>24)&0xff), (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), (UINT8)((imm32>>16)&0xff), \
        (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 7;
    mem32_pos = 3;
    return std::string((const INT8 *)array, 11);
}

std::string InstrGenerator::gen_addq_imm32_to_mem32_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &mem32_pos, INT32 mem32)
{
    UINT8 array[12] = {0x48, 0x81, 0x04, 0x25, (UINT8)(mem32&0xff), (UINT8)((mem32>>8)&0xff), (UINT8)((mem32>>16)&0xff), \
        (UINT8)((mem32>>24)&0xff), (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), (UINT8)((imm32>>16)&0xff),\
        (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 9;
    mem32_pos = 4;
    return std::string((const INT8*)array, 12);
}

std::string InstrGenerator::gen_xchg_rax_mem32_instr(UINT16 &smem_pos, INT32 mem32)
{
    UINT8 array[8] = {0x48, 0x87, 0x04, 0x25, (UINT8)(mem32&0xff), (UINT8)((mem32>>8)&0xff), (UINT8)((mem32>>16)&0xff), \
        (UINT8)((mem32>>24)&0xff)};
    smem_pos = 4;
    return std::string((const INT8*)array, 8);
}

std::string InstrGenerator::gen_xchg_rsp_mem32_instr(UINT16 &smem_pos, INT32 mem32)
{
    UINT8 array[8] = {0x48, 0x87, 0x24, 0x25, (UINT8)(mem32&0xff), (UINT8)((mem32>>8)&0xff), (UINT8)((mem32>>16)&0xff), \
        (UINT8)((mem32>>24)&0xff)};
    smem_pos = 4;
    return std::string((const INT8*)array, 8);
}

std::string InstrGenerator::gen_addq_imm8_to_rax_instr(UINT16 &imm8_pos, INT8 imm8)
{
    UINT8 array[4] = {0x48, 0x83, 0xc0, (UINT8)imm8};
    imm8_pos = 3;
    return std::string((const INT8*)array, 4);
}

std::string InstrGenerator::gen_movq_imm32_to_rax_smem_instr(UINT16 &imm32_pos, INT32 imm32)
{
    UINT8 array[7] = {0x48, 0xc7, 0x00, (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), (UINT8)((imm32>>16)&0xff),\
        (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 3;
    return std::string((const INT8*)array, 7);
}

std::string InstrGenerator::gen_movq_imm32_to_smem32_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &mem32_pos, INT32 mem32)
{
    UINT8 array[12] = {0x48, 0xc7, 0x04, 0x25, (UINT8)(mem32&0xff), (UINT8)((mem32>>8)&0xff), (UINT8)((mem32>>16)&0xff), \
        (UINT8)((mem32>>24)&0xff), (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), (UINT8)((imm32>>16)&0xff), (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 8;
    mem32_pos = 4;
    return std::string((const INT8*)array, 12);
}

std::string InstrGenerator::gen_addq_imm8_to_smem32_instr(UINT16 &imm8_pos, INT8 imm8, UINT16 &smem32_pos, INT32 mem32)
{
    UINT8 array[9] = {0x48, 0x83, 0x04, 0x25, (UINT8)(mem32&0xff), (UINT8)((mem32>>8)&0xff), (UINT8)((mem32>>16)&0xff), \
        (UINT8)((mem32>>24)&0xff), (UINT8)imm8};
    imm8_pos = 8;
    smem32_pos = 4;
    return std::string((const INT8*)array, 9);
}

std::string InstrGenerator::gen_pushfq()
{
    UINT8 array[1] = {0x9c};
    return std::string((const INT8*)array, 1);
}

std::string InstrGenerator::gen_pushfw()
{
    UINT8 array[2] = {0x66, 0x9c};
    return std::string((const INT8*)array, 2);
}

std::string InstrGenerator::gen_popfw()
{
    UINT8 array[2] = {0x66, 0x9d};
    return std::string((const INT8*)array, 2);
}

std::string InstrGenerator::gen_popfq()
{
    UINT8 array[1] = {0x9d};
    return std::string((const INT8*)array, 1);
}

std::string InstrGenerator::gen_invalid_instr()
{
    UINT8 array[1] = {0xd6};
    return std::string((const INT8 *)array, 1);
}

std::string InstrGenerator::gen_xchg_rax_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[8] = {0x48, 0x87, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 4;
    return std::string((const INT8*)array, 8);
}

std::string InstrGenerator::gen_xchg_rax_gs_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[9] = {0x65, 0x48, 0x87, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 5;
    return std::string((const INT8*)array, 9);
}

std::string InstrGenerator::gen_movq_rax_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[8] = {0x48, 0x8b, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 4;
    return std::string((const INT8*)array, 8);
}

std::string InstrGenerator::gen_movq_rsp_smem_rax_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[8] = {0x48, 0x89, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 4;
    return std::string((const INT8*)array, 8);
}

std::string InstrGenerator::gen_movq_gs_rsp_smem_rax_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[9] = {0x65, 0x48, 0x89, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 5;
    return std::string((const INT8*)array, 9);
}

std::string InstrGenerator::gen_nop_instr()
{
    UINT8 array[1] = {0x90};
    return std::string((const INT8*)array, 1);
}

std::string InstrGenerator::gen_addq_imm32_to_rsp_mem_instr(UINT16 &imm_pos, INT32 imm32)
{
    UINT8 array[8] = {0x48, 0x81, 0x04, 0x24, (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), \
        (UINT8)((imm32>>16)&0xff), (UINT8)((imm32>>24)&0xff)};
    imm_pos = 4;
    return std::string((const INT8*)array, 8);
}

std::string InstrGenerator::gen_addq_imm32_to_rsp_smem_disp8_instr(UINT16 &imm_pos, INT32 imm32, UINT16 &disp8_pos, INT8 disp8)
{
    UINT8 array[9] = {0x48, 0x81, 0x44, 0x24, (UINT8)disp8, (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), \
        (UINT8)((imm32>>16)&0xff), (UINT8)((imm32>>24)&0xff)};
    imm_pos = 5;
    disp8_pos = 4;
    return std::string((const INT8*)array, 9);
}

std::string InstrGenerator::convert_jumpin_mem_to_cmpl_imm32(const UINT8 *instcode, UINT32 instsize, UINT16 &imm32_pos, INT32 imm32)
{
    ASSERT(instsize<=8);
    std::string instr_template;
    UINT32 jmpin_index = 0;
    // 1. copy prefix 
    if((instcode[jmpin_index])!=0xff){//copy prefix
        instr_template.append(1, instcode[jmpin_index++]);//copy prefix
        ASSERT(instcode[jmpin_index]==0xff);
    }
    // 2.set opcode
    instr_template.append(1, 0x81);
    jmpin_index++;
    // 3.calculate cmp ModRM
    instr_template.append(1, instcode[jmpin_index++]|0x38);
    // 4.copy SIB | Displacement of jmpin_inst
    while(jmpin_index<instsize)
        instr_template.append(1, instcode[jmpin_index++]);
    // 5.set imm32
    imm32_pos = instr_template.length();
    instr_template.append((const INT8*)&imm32, sizeof(INT32));
    // 6.ret 

    return instr_template;
}

std::string InstrGenerator::convert_jumpin_mem_to_push_mem(const UINT8 *instcode, UINT32 instsize)
{
    std::string push_template;
    UINT32 jmpin_index = 0;
    UINT8 push_mem_opcode = 0xff;
    // 1. copy prefix 
    if((instcode[jmpin_index])!=0xff){//copy prefix
        push_template.append((const INT8*)(instcode+jmpin_index), 1);
        jmpin_index++;
    }
    ASSERTM(instcode[jmpin_index]==0xff, "jumpin opcode is unkown!\n");
    // 2.set opcode
    push_template.append((const INT8*)&push_mem_opcode, 1);
    jmpin_index++;                                                                                                                                                      
    // 3.calculate cmp ModRM
    ASSERTM((instcode[jmpin_index]&0x38)==0x20, "We only handle jmp r/m64! jmp m16:[16|32|64] is not handled!\n");
    UINT8 push_mem_ModRM = instcode[jmpin_index++]|0x30;
    push_template.append((const INT8*)&push_mem_ModRM, 1);
    // 4.copy SIB | Displacement of jmpin_inst
    while(jmpin_index<instsize){
        push_template.append((const INT8*)(instcode+jmpin_index), 1);
        jmpin_index++;
    }
    
    return push_template;
}

std::string InstrGenerator::convert_jumpin_reg_to_push_reg(const UINT8 *instcode, UINT32 instsize)
{
    std::string push_template;
    UINT16 jumpin_index = 0;
    if(instcode[jumpin_index]!=0xff){//has prefix, copy it
        push_template.append(1, instcode[jumpin_index++]);
        ASSERT(instsize==3);
    }else
        ASSERT(instsize==2);
    ASSERT(instcode[jumpin_index]==0xff);
    //set opcode and reg
    push_template.append(1, 0x50|(instcode[++jumpin_index]&0xf));

    return push_template;
}

std::string InstrGenerator::convert_jumpin_reg_to_movq_rax_reg(const UINT8 *instcode, UINT32 instsize)
{
    std::string movq_template;
    UINT16 jumpin_index = 0;
    //set prefix
    if(instcode[jumpin_index]!=0xff){//has prefix
        movq_template.append(1, 0x4c);
        ASSERT(instcode[jumpin_index]==0x41);
        ASSERT(instsize==3);
        jumpin_index++;
    }else{
        movq_template.append(1, 0x48);
        ASSERT(instsize==2);
    }
    ASSERT(instcode[jumpin_index]==0xff);
    jumpin_index++;
    //set opcode
    movq_template.append(1, 0x89);
    //set modRM
    movq_template.append(1, ((instcode[jumpin_index]<<3)&0x38)|0xc0);

    return movq_template;
}

std::string InstrGenerator::convert_callin_mem_to_movq_rax_mem(const UINT8 *instcode, UINT32 instsize)
{
    std::string movq_template;
    UINT32 callin_index = 0;

    if(instcode[callin_index]==0x64){//has fs prefix
        movq_template.append(1, 0x64);
        callin_index++;
    }
    // 1. set ordinary prefix 
    if(instcode[callin_index]==0xff)
        movq_template.append(1, 0x48);
    else{//has ordninary prefix
        movq_template.append(1, instcode[callin_index++]|0x48);
        ASSERTM(instcode[callin_index]==0xff, "callin opcode is unkown!\n");
    }
    // 2.set opcode
    movq_template.append(1, 0x8b);
    callin_index++;                                                                                                                                                      
    // 3.calculate ModRM
    movq_template.append(1, instcode[callin_index++]&0xc7);
    // 4.copy SIB | Displacement of jmpin_inst
    while(callin_index<instsize){
        movq_template.append((const INT8*)(instcode+callin_index), 1);
        callin_index++;
    }
    
    return movq_template;
}

std::string InstrGenerator::convert_jumpin_mem_to_movq_rax_mem(const UINT8 *instcode, UINT32 instsize)
{
    std::string movq_template;
    UINT32 jmpin_index = 0;
    // 1. set prefix 
    if((instcode[jmpin_index])==0xff)
        movq_template.append(1, 0x48);
    else{//has prefix
        movq_template.append(1, instcode[jmpin_index++]|0x48);
        ASSERTM(instcode[jmpin_index]==0xff, "jumpin opcode is unkown!\n");
    }
    // 2.set opcode
    movq_template.append(1, 0x8b);
    jmpin_index++;                                                                                                                                                      
    // 3.calculate ModRM
    ASSERTM((instcode[jmpin_index]&0x38)==0x20, "We only handle jmp r/m64! jmp m16:[16|32|64] is not handled!\n");
    movq_template.append(1, instcode[jmpin_index++]&0xc7);
    // 4.copy SIB | Displacement of jmpin_inst
    while(jmpin_index<instsize){
        movq_template.append((const INT8*)(instcode+jmpin_index), 1);
        jmpin_index++;
    }
    
    return movq_template;
}

std::string InstrGenerator::convert_jumpin_mem_to_movq_reg_mem(const UINT8 *instcode, UINT32 instsize, UINT8 reg_index)
{
    std::string movq_template;
    UINT32 jmpin_index = 0;
    UINT8 prefix_base = 0;
    UINT8 reg_in_ModRM = 0;
    
    if(reg_index>=R_RAX && reg_index<=R_RDI){
        prefix_base = 0x48;
        reg_in_ModRM = reg_index - R_RAX;
    }else if(reg_index>=R_R8 && reg_index<=R_R15){
        prefix_base = 0x4c;
        reg_in_ModRM = reg_index - R_R8;
    }else
        ASSERT(0);
    
    // 1. set prefix 
    if((instcode[jmpin_index])==0xff)
        movq_template.append(1, prefix_base);
    else{//has prefix
        movq_template.append(1, instcode[jmpin_index++]|prefix_base);
        ASSERTM(instcode[jmpin_index]==0xff, "jumpin opcode is unkown!\n");
    }
    // 2.set opcode
    movq_template.append(1, 0x8b);
    jmpin_index++;                                                                                                                                                      
    // 3.calculate ModRM
    ASSERTM((instcode[jmpin_index]&0x38)==0x20, "We only handle jmp r/m64! jmp m16:[16|32|64] is not handled!\n");
    movq_template.append(1, (instcode[jmpin_index++]&0xc7)|((reg_in_ModRM&0x7)<<3));
    // 4.copy SIB | Displacement of jmpin_inst
    while(jmpin_index<instsize){
        movq_template.append((const INT8*)(instcode+jmpin_index), 1);
        jmpin_index++;
    }
    
    return movq_template;
}

std::string InstrGenerator::convert_callin_reg_to_push_reg(const UINT8 *instcode, UINT32 instsize)
{
    std::string push_template;
    UINT16 callin_index = 0;
    if(instcode[callin_index]!=0xff){//has prefix, copy it
        push_template.append(1, instcode[callin_index++]);
        ASSERT(instsize==3);
    }else
        ASSERT(instsize==2);
    ASSERT(instcode[callin_index]==0xff);
    //set opcode and reg
    push_template.append(1, 0x50|(instcode[++callin_index]&0xf));

    return push_template;
}


std::string InstrGenerator::convert_callin_mem_to_push_mem(const UINT8 *instcode, UINT32 instsize)
{
    std::string push_template;
    UINT32 callin_index = 0;
    UINT8 push_mem_opcode = 0xff;
    // 1. copy prefix 
    if((instcode[callin_index])!=0xff){//copy prefix
        push_template.append((const INT8*)(instcode+callin_index), 1);
        callin_index++;
    }
    ASSERTM(instcode[callin_index]==0xff, "callin opcode is unkown!\n");
    // 2.set opcode
    push_template.append((const INT8*)&push_mem_opcode, 1);
    callin_index++;                                                                                                                                                      
    // 3.calculate cmp ModRM
    ASSERTM((instcode[callin_index]&0x38)==0x10, "We only handle call r/m64! call m16:[16|32|64] is not handled!\n");
    UINT8 push_mem_ModRM = instcode[callin_index++]|0x30;
    push_template.append((const INT8*)&push_mem_ModRM, 1);
    // 4.copy SIB | Displacement of jmpin_inst
    while(callin_index<instsize){
        push_template.append((const INT8*)(instcode+callin_index), 1);
        callin_index++;
    }
    
    return push_template;
}

std::string InstrGenerator::gen_retq_instr()
{
    UINT8 array = 0xc3;
    return std::string((const INT8 *)&array, 1);
}

std::string InstrGenerator::gen_jump_rel32_instr(UINT16 &rel32_pos, INT32 rel32)
{
    UINT8 array[5] = {0xe9, (UINT8)(rel32&0xff), (UINT8)((rel32>>8)&0xff), (UINT8)((rel32>>16)&0xff), (UINT8)((rel32>>24)&0xff)};
    rel32_pos = 1;
    return std::string((const INT8*)array, 5);
}

std::string InstrGenerator::gen_jump_rel8_instr(UINT16 &rel8_pos, INT8 rel8)
{
    UINT8 array[2] = {0xeb, (UINT8)rel8};
    rel8_pos = 1;
    return std::string((const INT8*)array, 2);
}

std::string InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, \
    UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[11] = {0xc7, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), (UINT8)((disp32>>16)&0xff),\
        (UINT8)((disp32>>24)&0xff), (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), (UINT8)((imm32>>16)&0xff), \
        (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 7;
    disp32_pos = 3;

    return std::string((const INT8*)array, 11);
}

std::string InstrGenerator::gen_movl_imm32_to_gs_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, \
    UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[12] = {0x65, 0xc7, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), (UINT8)((disp32>>16)&0xff),\
        (UINT8)((disp32>>24)&0xff), (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), (UINT8)((imm32>>16)&0xff), \
        (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 8;
    disp32_pos = 4;

    return std::string((const INT8*)array, 12);
}

std::string InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, \
    UINT16 &disp8_pos, INT8 disp8)
{
    UINT8 array[8] = {0xc7, 0x44, 0x24, (UINT8)(disp8), (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), \
        (UINT8)((imm32>>16)&0xff), (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 4;
    disp8_pos = 3;
    return std::string((const INT8*)array, 8);
}

std::string InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32)
{
    UINT8 array[7] = {0xc7, 0x04, 0x24, (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), (UINT8)((imm32>>16)&0xff), \
        (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 3;
    return std::string((const INT8*)array, 7);
}

std::string InstrGenerator::gen_movq_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, \
    UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[12] = {0x48, 0xc7, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff), (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff),\
        (UINT8)((imm32>>16)&0xff), (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 8;
    disp32_pos = 4;
    return std::string((const INT8*)array, 12);
}

std::string InstrGenerator::gen_movq_imm32_to_gs_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, \
    UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[13] = {0x65, 0x48, 0xc7, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff), (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff),\
        (UINT8)((imm32>>16)&0xff), (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 9;
    disp32_pos = 5;
    return std::string((const INT8*)array, 13);
}

std::string InstrGenerator::gen_pushq_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[7] = {0xff, 0xb4, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 3;
    return std::string((const INT8 *)array, 7);
}

std::string InstrGenerator::gen_pushq_gs_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[8] = {0x65, 0xff, 0xb4, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 4;
    return std::string((const INT8 *)array, 8);
}

std::string InstrGenerator::gen_popq_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[7] = {0x8f, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 3;
    return std::string((const INT8 *)array, 7);
}

std::string InstrGenerator::gen_popq_gs_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[8] = {0x65, 0x8f, 0x84, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), \
        (UINT8)((disp32>>16)&0xff), (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 4;
    return std::string((const INT8 *)array, 8);
}

std::string InstrGenerator::gen_pushq_imm32_instr(UINT16 &imm32_pos, INT32 imm32)
{
    UINT8 array[5] = {0x68, (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), (UINT8)((imm32>>16)&0xff), (UINT8)((imm32>>24)&0xff)};
    imm32_pos = 1;
    return std::string((const INT8*)array, 5);
}

std::string InstrGenerator::gen_cmp_reg32_imm32_instr(UINT8 reg_index, UINT16 &imm32_pos, INT32 imm32)
{
    std::string instr_template;
    UINT8 modRM_reg = 0;
    //1.prefix
    if(reg_index>=R_R8D && reg_index<=R_R15D){
        instr_template.append(1, 0x41);
        modRM_reg = (reg_index - R_R8D)&0x7;
    }else if(reg_index>=R_EAX && reg_index<=R_EDI)
        modRM_reg = (reg_index - R_EAX)&0x7;
    else
        ASSERT(0);

    instr_template.append(1, 0x81);
    instr_template.append(1, 0xf8|modRM_reg);
    imm32_pos = instr_template.length();
    instr_template.append((const INT8*)&imm32, sizeof(INT32));

    return instr_template;
}

std::string InstrGenerator::gen_cmp_reg64_imm8_instr(UINT8 reg_index, UINT16 &imm8_pos, INT8 imm8)
{
    std::string instr_template;
    UINT8 modRM_reg = 0;
    //1.prefix
    if(reg_index>=R_R8 && reg_index<=R_R15){
        instr_template.append(1, 0x49);
        modRM_reg = (reg_index - R_R8)&0x7;
    }else if(reg_index>=R_RAX && reg_index<=R_RDI){
        instr_template.append(1, 0x48);
        modRM_reg = (reg_index - R_RAX)&0x7;
    }else
        ASSERT(0);
    //2.opcode
    instr_template.append(1, 0x83);
    //3.modRM
    instr_template.append(1, 0xf8|modRM_reg);
    imm8_pos = instr_template.length();
    instr_template.append((const INT8*)&imm8, 1);

    return instr_template;
}

std::string InstrGenerator::gen_movq_reg_to_rax_instr(UINT8 reg_index)
{
    std::string instr_template;
    UINT8 reg_idx_in_modRM = 0;
    if(reg_index>=R_R8 && reg_index<=R_R15){//prefix
        instr_template.append(1, 0x4c);
        reg_idx_in_modRM = reg_index - R_R8;
    }else if(reg_index>=R_RAX && reg_index<=R_RDI){
        instr_template.append(1, 0x48);
        reg_idx_in_modRM = reg_index - R_RAX;
    }else
        ASSERT(0);
    //opcode
    instr_template.append(1, 0x89);
    //modRM
    instr_template.append(1, 0xc0|((reg_idx_in_modRM&0x7)<<3));

    return instr_template;
}
    
std::string InstrGenerator::gen_addq_reg_imm32_instr(UINT8 reg_index, UINT16 &imm32_pos, INT32 imm32)
{
    std::string instr_template;
    if(reg_index>=R_RAX && reg_index<=R_RDI){
        instr_template.append(1, 0x48);
        if(reg_index!=R_RAX){
            instr_template.append(1, 0x81);
            instr_template.append(1, 0xc0+reg_index-R_RAX);
        }else
            instr_template.append(1, 0x05);
    }else if(reg_index>=R_R8 && reg_index<=R_R15){
        instr_template.append(1, 0x49);
        instr_template.append(1, 0x81);
        instr_template.append(1, 0xc0+reg_index-R_R8);
    }else
        ASSERT(0);

    imm32_pos = instr_template.length();
    instr_template.append((const INT8*)&imm32, sizeof(INT32));

    return instr_template;
}

std::string InstrGenerator::gen_jmpq_reg(UINT8 reg_index)
{
    std::string instr_template;

    if(reg_index>=R_R8 && reg_index<=R_R15){
        instr_template.append(1, 0x41);
        instr_template.append(1, 0xff);
        instr_template.append(1, 0xe0+reg_index-R_R8);
    }else{
        ASSERT(reg_index>=R_RAX && reg_index<=R_RDI);
        instr_template.append(1, 0xff);
        instr_template.append(1, 0xe0+reg_index-R_RAX);
    }

    return instr_template;   
}

std::string InstrGenerator::convert_jmpin_reg64_to_cmp_reg64_imm8(UINT8 *instcode, UINT16 instsize, UINT16 &imm8_pos, INT8 imm8)
{
    std::string instr_template;
    UINT16 jmpin_index = 0;
    if(instcode[jmpin_index]!=0xff){//has prefix
        instr_template.append(1, 0x49);
        jmpin_index++;
        ASSERT(instsize=3);
    }else{
        instr_template.append(1, 0x48);
        ASSERT(instsize==2);
    }
    
    ASSERT(instcode[jmpin_index]==0xff);
    instr_template.append(1, 0x83);//set opcode
    jmpin_index++;
    //set modRM
    instr_template.append(1, 0xf8|instcode[jmpin_index]);
    
    imm8_pos = instr_template.length();
    instr_template.append(1, (UINT8)imm8);

    return instr_template;
}

std::string InstrGenerator::convert_jmpin_mem_to_cmp_mem_imm8(UINT8 *instcode, UINT16 instsize, UINT16 &imm8_pos, INT8 imm8)
{
    std::string instr_template;
    UINT32 jmpin_index = 0;
    // 1. calculate prefix 
    if((instcode[jmpin_index])!=0xff){
        instr_template.append(1, instcode[jmpin_index]|0x48);
        jmpin_index++;
    }else
        instr_template.append(1, 0x48);
    ASSERTM(instcode[jmpin_index]==0xff, "jmpin opcode is unkown!\n");
    // 2.set opcode
    instr_template.append(1, 0x83);
    jmpin_index++;                                                                                                                                                      
    // 3.calculate cmp ModRM
    ASSERTM((instcode[jmpin_index]&0x38)==0x20, "We only handle jmp r/m64! jmp m16:[16|32|64] is not handled!\n");
    instr_template.append(1, instcode[jmpin_index++]|0x38);
    // 4.copy SIB | Displacement of jmpin_inst
    while(jmpin_index<instsize)
        instr_template.append(1, instcode[jmpin_index++]);
    // 5.set imm8
    imm8_pos = instr_template.length();
    instr_template.append(1, (UINT8)imm8);
    
    return instr_template;
}

std::string InstrGenerator::gen_je_rel32_instr(UINT16 &rel32_pos, INT32 rel32)
{
    UINT8 array[6] = {0x0f, 0x84, (UINT8)(rel32&0xff), (UINT8)((rel32>>8)&0xff), (UINT8)((rel32>>16)&0xff), \
        (UINT8)((rel32>>24)&0xff)};
    rel32_pos = 2;
    return std::string((const INT8 *)array, 6);
}

std::string InstrGenerator::gen_jl_rel8_instr(UINT16 &rel8_pos, INT8 rel8, BOOL is_taken)
{
    UINT8 array[3] = {is_taken ? (UINT8)0x3e : (UINT8)0x2e, 0x7c, (UINT8)rel8};
    rel8_pos = 2;
    return std::string((const INT8 *)array, 3);
}

std::string InstrGenerator::gen_jns_rel8_instr(UINT16 &rel8_pos, INT8 rel8, BOOL is_taken)
{
    UINT8 array[3] = {is_taken ? (UINT8)0x3e : (UINT8)0x2e, (UINT8)0x79, (UINT8)rel8};
    rel8_pos = 2;
    return std::string((const INT8*)array, 3);
}

std::string InstrGenerator::gen_call_next()
{
    UINT8 array[5] = {0xe8, 0x00, 0x00, 0x00, 0x00};
    return std::string((const INT8 *)array, 5);
}

std::string InstrGenerator::gen_addq_imm8_to_rsp_instr(UINT16 &imm8_pos, INT8 imm8)
{
    UINT8 array[4] = {0x48, 0x83, 0xc4, (UINT8)imm8};
    imm8_pos = 3;
    return std::string((const INT8 *)array, 4);
}

std::string InstrGenerator::gen_jmpq_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[7] = {0xff, 0xa4, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), (UINT8)((disp32>>16)&0xff), \
        (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 3;
    return std::string((const INT8 *)array, 7);
}

std::string InstrGenerator::gen_jmpq_gs_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[8] = {0x65, 0xff, 0xa4, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), (UINT8)((disp32>>16)&0xff), \
        (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 4;
    return std::string((const INT8 *)array, 8);
}

std::string InstrGenerator::convert_cond_br_relx_to_rel32(const UINT8 *instcode, UINT32 inst_size, UINT16 &rel32_pos, INT32 rel32)
{
    std::string instr_template;
    UINT8 opcode_pos = 0;
    
    if(instcode[0]==0x2e || instcode[0]==0x3e){//branch hint prefix
        instr_template.append(1, instcode[0]);
        opcode_pos = 1;
    }
    
    if(instcode[opcode_pos]==0x0f){//condtional br instruction is already rel32
        ASSERT((instcode[opcode_pos+1]&0xf0) == 0x80);//jcc rel32
        instr_template.append((const INT8*)(instcode+opcode_pos), 2);
    }else{//convert rel8 to rel32 opcode
        ASSERT((instcode[opcode_pos]&0xf0) == 0x70);//jcc rel8
        instr_template.append(1, 0x0f);
        instr_template.append(1, instcode[opcode_pos]+0x10);
    }

    rel32_pos = instr_template.length();
    instr_template.append((const INT8 *)&rel32, 4);
    
    return instr_template;
}

std::string InstrGenerator::convert_cond_br_relx_to_rel8(const UINT8 *instcode, UINT32 inst_size, UINT16 &rel8_pos, INT8 rel8)
{
    std::string instr_template;
    UINT8 opcode_pos = 0;

    if(instcode[0]==0x2e || instcode[0]==0x3e){//branch hint prefix
        instr_template.append(1, instcode[0]);
        opcode_pos = 1;
    }

    if(instcode[opcode_pos]!=0x0f){//conditional br instruction is already rel8
        ASSERT(((instcode[opcode_pos]&0xf0)==0x70) || ((instcode[opcode_pos]&0xfc)==0xe0));//jcc or jcxz/jexz/jrcx/loop/loope/loopne
        instr_template.append(1, instcode[opcode_pos]);
    }else{
        ASSERT((instcode[opcode_pos+1]&0xf0)==0x80);//jcc rel32
        instr_template.append(1, instcode[opcode_pos+1]-0x10);
    }

    rel8_pos = instr_template.length();
    instr_template.append((const INT8 *)&rel8, 1);

    return instr_template;
}

std::string InstrGenerator::modify_disp_of_pushq_rsp_mem(std::string src_template, INT8 addend)
{
    std::string dest_template;
    UINT16 pushq_index = 0;
    
    if((UINT8)src_template[pushq_index]!=0xff)//copy prefix
        dest_template.append(1, src_template[pushq_index++]);

    ASSERT((UINT8)src_template[pushq_index]==0xff);//pushq opcode
    dest_template.append(1, src_template[pushq_index++]);//copy opcode
    //judge ModRM
    switch((UINT8)src_template[pushq_index]){
        case 0x34://dispSize=0
            {
                //set dispSize8 ModRM
                dest_template.append(1, 0x74);
                //copy SIB
                dest_template.append(1, src_template[++pushq_index]);
                //set displacement
                dest_template.append(1, addend);
            }
            break;
        case 0x74://dispSize=8
            {
                //get displacement
                INT64 disp = (INT64)src_template[pushq_index+2];
                INT64 fixed_disp = disp + (INT64)addend;
                if((fixed_disp > 0 ? fixed_disp : -fixed_disp) < 0x7f){
                    //can still use dispSize8
                    dest_template.append(1, src_template[pushq_index++]);//copy ModRM
                    dest_template.append(1, src_template[pushq_index++]);//copy SIB
                    dest_template.append(1, (INT8)fixed_disp);//set displacement
                }else{
                    //should use dispSize32
                    dest_template.append(1, 0xb4);//set dispSize32 ModRM
                    dest_template.append(1, src_template[++pushq_index]);//copy SIB
                    //set displacement
                    dest_template.append((const INT8*)&fixed_disp, 4);
                }
            }
            break;
        case 0xb4://dispSize=32
            {
                //copy ModRM
                dest_template.append(1, src_template[pushq_index++]);
                //copy SIB
                dest_template.append(1, src_template[pushq_index++]);
                INT64 disp = *(INT64*)(&src_template[pushq_index]);
                INT64 fixed_disp = disp + (INT64)addend;
                ASSERT((fixed_disp > 0 ? fixed_disp : -fixed_disp) < 0x7fffffff);
                //set displacement
                dest_template.append((const INT8*)&fixed_disp, 4);
            }
            break;
        default:
            ASSERTM(0, "Unkown ModRM(%x) in pushq mem with rsp register!\n", src_template[pushq_index]);
    }

    return dest_template;
}

std::string InstrGenerator::modify_disp_of_movq_rsp_mem(std::string src_template, INT8 addend)
{
    std::string dest_template;
    UINT16 movq_index = 0;
    
    if((UINT8)src_template[movq_index]!=0x8b)//copy prefix
        dest_template.append(1, src_template[movq_index++]);

    ASSERT((UINT8)src_template[movq_index]==0x8b);//movq opcode
    dest_template.append(1, src_template[movq_index++]);//copy opcode
    //judge ModRM
    switch((UINT8)src_template[movq_index]){
        case 0x04://dispSize=0
            {
                //set dispSize8 ModRM
                dest_template.append(1, 0x44);
                //copy SIB
                dest_template.append(1, src_template[++movq_index]);
                //set displacement
                dest_template.append(1, addend);
            }
            break;
        case 0x44://dispSize=8
            {
                //get displacement
                INT64 disp = (INT64)src_template[movq_index+2];
                INT64 fixed_disp = disp + (INT64)addend;
                if((fixed_disp > 0 ? fixed_disp : -fixed_disp) < 0x7f){
                    //can still use dispSize8
                    dest_template.append(1, src_template[movq_index++]);//copy ModRM
                    dest_template.append(1, src_template[movq_index++]);//copy SIB
                    dest_template.append(1, (INT8)fixed_disp);//set displacement
                }else{
                    //should use dispSize32
                    dest_template.append(1, 0x84);//set dispSize32 ModRM
                    dest_template.append(1, src_template[++movq_index]);//copy SIB
                    //set displacement
                    dest_template.append((const INT8*)&fixed_disp, 4);
                }
            }
            break;
        case 0x84://dispSize=32
            {
                //copy ModRM
                dest_template.append(1, src_template[movq_index++]);
                //copy SIB
                dest_template.append(1, src_template[movq_index++]);
                INT64 disp = *(INT64*)(&src_template[movq_index]);
                INT64 fixed_disp = disp + (INT64)addend;
                ASSERT((fixed_disp > 0 ? fixed_disp : -fixed_disp) < 0x7fffffff);
                //set displacement
                dest_template.append((const INT8*)&fixed_disp, 4);
            }
            break;
        default:
            ASSERTM(0, "Unkown ModRM(%x) in movq mem with rsp register!\n", src_template[movq_index]);
    }

    return dest_template;
}


