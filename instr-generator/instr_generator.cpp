#include "instr_generator.h"
#include "disasm_common.h"

std::string InstrGenerator::gen_invalid_instr()
{
    UINT8 array = 0xd6;
    return std::string((const INT8 *)&array, 1);
}

std::string InstrGenerator::gen_addq_imm32_to_rsp_mem_instr(UINT16 &imm_pos, INT32 imm32)
{
    UINT8 array[8] = {0x48, 0x81, 0x04, 0x24, (UINT8)(imm32&0xff), (UINT8)((imm32>>8)&0xff), \
        (UINT8)((imm32>>16)&0xff), (UINT8)((imm32>>24)&0xff)};
    imm_pos = 4;
    return std::string((const INT8*)array, 8);
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

std::string InstrGenerator::gen_je_rel32_instr(UINT16 &rel32_pos, INT32 rel32)
{
    UINT8 array[6] = {0x0f, 0x84, (UINT8)(rel32&0xff), (UINT8)((rel32>>8)&0xff), (UINT8)((rel32>>16)&0xff), \
        (UINT8)((rel32>>24)&0xff)};
    rel32_pos = 2;
    return std::string((const INT8 *)array, 6);
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

std::string InstrGenerator::gen_jmpq_rsp_smem(UINT16 &disp32_pos, INT32 disp32)
{
    UINT8 array[7] = {0xff, 0xa4, 0x24, (UINT8)(disp32&0xff), (UINT8)((disp32>>8)&0xff), (UINT8)((disp32>>16)&0xff), \
        (UINT8)((disp32>>24)&0xff)};
    disp32_pos = 3;
    return std::string((const INT8 *)array, 7);
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
    UINT8 opcode_pos;

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
                INT64 disp = 0;
                for(INT32 idx=0; idx<4; idx++)
                    disp |= src_template[pushq_index+idx+2]<<(idx*8);// 2 is skip the ModRM and SIB  
                INT64 fixed_disp = disp + (INT64)addend;
                ASSERT((fixed_disp > 0 ? fixed_disp : -fixed_disp) < 0x7fffffff);
                //copy ModRM
                dest_template.append(1, src_template[pushq_index++]);
                //copy SIB
                dest_template.append(1, src_template[pushq_index++]);
                //set displacement
                dest_template.append((const INT8*)&fixed_disp, 4);
            }
            break;
        default:
            ASSERTM(0, "Unkown ModRM(%x) in pushq mem with rsp register!\n", src_template[pushq_index]);
    }

    return dest_template;
}


