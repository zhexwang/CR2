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
        if(instr)
            module->insert_instr(instr);
    }
    free(line_buf);
    // 5.close stream
    pclose(p_stream);

}

void Disassembler::disassemble_all_modules()
{
    ElfParser::PARSED_ELF_ITERATOR it = ElfParser::_all_parsed_elfs.begin();
    for(;it!=ElfParser::_all_parsed_elfs.end();it++){
        ElfParser *elf = it->second;
        Module *module = new Module(elf);
        disassemble_module(module);
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
