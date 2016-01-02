#include "instr_generator.h"

std::string InstrGenerator::gen_invalid_instr()
{
    UINT8 array = 0xd6;
    return std::string((const INT8 *)&array, 1);
}

std::string InstrGenerator::gen_addq_imm32_to_rsp_mem_instr(UINT16 &imm_pos, INT32 imm32)
{
    UINT8 array[8] = {0x48, 0x81, 0x04, 0x24, imm32&0xff, (imm32>>8)&0xff, (imm32>>16)&0xff, (imm32>>24)&0xff};
    imm_pos = 4;
    return std::string((const INT8*)array, 8);
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
    UINT8 push_mem_ModRM = instcode[jmpin_index++]|0x10;
    push_template.append((const INT8*)&push_mem_ModRM, 1);
    // 4.copy SIB | Displacement of jmpin_inst
    while(jmpin_index<instsize){
        push_template.append((const INT8*)(instcode+jmpin_index), 1);
        jmpin_index++;
    }
    
    return push_template;
}

std::string InstrGenerator::gen_retq_instr()
{
    UINT8 array = 0xc3;
    return std::string((const INT8 *)&array, 1);
}
