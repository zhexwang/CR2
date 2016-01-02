#pragma once

#include <string>
#include "type.h"
#include "utility.h"

class InstrGenerator
{
public:
	//bad
	static std::string gen_invalid_instr();
	//addq (%rsp), $imm32
	static std::string gen_addq_imm32_to_rsp_mem_instr(UINT16 &imm_pos, INT32 imm32);
	//retq
	static std::string gen_retq_instr();
	//convert functions
	static std::string convert_jumpin_mem_to_push_mem(const UINT8 *instcode, UINT32 instsize);
	
	
};
