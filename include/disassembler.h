#pragma once

#include "type.h"
#include "utility.h"
#include "disasm_common.h"

class Instruction;
class Module;

class Disassembler
{
private:
	//disassemble arguments
	static _CodeInfo _ci;
	static _DInst _dInst;
	static _DecodedInst _decodedInst;
protected:
	/*  @Arguments: Module*
		@Return: None
		@Introduction: This function does following three things:
			1. disassemble x sections in module (should split data and code, now we use objdump)
			2. disassemble x sections and record all Instructions
			3. mapping Addr with Instruction
	*/
	static void disassemble_module(Module *module);
	/*  @Arguments: 
			1. instr_off represents the offset in the module (instruction's offset in module)
			2. code represents the instruction encode in Module
		@Return: None
		@Introduction: This function mainly disassemble the instruction and record it.
	*/
	static Instruction *disassemble_instruction(const F_SIZE instr_off, const Module *module, \
		const char *objdump_line_buf = NULL);
public:
	static void init();
	/*  @Arguments:None
		@Return: None
		@Introduction: disasm all elfparsers and new modules.
	*/
	static void disassemble_all_modules();
	static void dump_pinst(const Instruction *instr, const P_ADDRX load_base);
	static void dump_file_inst(const Instruction *instr);
};

