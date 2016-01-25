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
	//movl disp32(%rsp), $imm32
	static std::string gen_movl_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &disp32_pos, INT32 disp32);
	//movl (%rsp), $imm32
	static std::string gen_movl_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32);
	//movl disp8(%rsp), $imm32
	static std::string gen_movl_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &disp8_pos, INT8 disp8);
	//movq disp32(%rsp), $imm32
	static std::string gen_movq_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &disp32_pos, INT32 disp32);
	//jmpq disp32(%rsp)
	static std::string gen_jmpq_rsp_smem(UINT16 &disp32_pos, INT32 disp32);
	//jmp rel32
	static std::string gen_jump_rel32_instr(UINT16 &rel32_pos, INT32 rel32);
	//jmp rel8
	static std::string gen_jump_rel8_instr(UINT16 &rel8_pos, INT8 rel8);
	//callnext
	static std::string gen_call_next();
	//addq %rsp, $imm8
	static std::string gen_addq_imm8_to_rsp_instr(UINT16 &imm8_pos, INT8 imm8);
	//pushq imm32
	static std::string gen_pushq_imm32_instr(UINT16 &imm32_pos, INT32 imm32);
	//cmp reg32, imm32
	static std::string gen_cmp_reg32_imm32_instr(UINT8 reg_index, UINT16 &imm32_pos, INT32 imm32);
	//je rel32
	static std::string gen_je_rel32_instr(UINT16 &rel32_pos, INT32 rel32);
	//convert functions
	static std::string convert_jumpin_mem_to_cmpl_imm32(const UINT8 *instcode, UINT32 instsize, UINT16 &imm32_pos, INT32 imm32);
	static std::string convert_jumpin_mem_to_push_mem(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_jumpin_reg_to_push_reg(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_callin_reg_to_push_reg(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_callin_mem_to_push_mem(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_cond_br_relx_to_rel32(const UINT8 *instcode, UINT32 inst_size, UINT16 &rel32_pos, INT32 rel32);
	static std::string convert_cond_br_relx_to_rel8(const UINT8 *instcode, UINT32 inst_size, UINT16 &rel8_pos, INT8 rel8);
	//modify functions
	static std::string modify_disp_of_pushq_rsp_mem(std::string src_template, INT8 addend);
};
