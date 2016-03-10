#pragma once

#include <string>
#include "type.h"
#include "utility.h"

class InstrGenerator
{
public:
	//debug
	static std::string gen_movl_imm32_to_mem32_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &mem32_pos, INT32 mem32);
	static std::string gen_addq_imm32_to_mem32_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &mem32_pos, INT32 mem32);
	static std::string gen_xchg_rax_mem32_instr(UINT16 &smem_pos, INT32 mem32);
	static std::string gen_xchg_rsp_mem32_instr(UINT16 &smem_pos, INT32 mem32);
	static std::string gen_addq_imm8_to_rax_instr(UINT16 &imm8_pos, INT8 imm8);
	static std::string gen_movq_imm32_to_rax_smem_instr(UINT16 &imm32_pos, INT32 imm32);
	static std::string gen_movq_imm32_to_smem32_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &mem32_pos, INT32 mem32);
	static std::string gen_addq_imm8_to_smem32_instr(UINT16 &imm8_pos, INT8 imm8, UINT16 &smem32_pos, INT32 mem32);
	//bad
	static std::string gen_invalid_instr();
	//eflag
	static std::string gen_pushfq();
	static std::string gen_popfq();
	static std::string gen_pushfw();
	static std::string gen_popfw();
	//xchg %rax, disp32(%rsp)
	static std::string gen_xchg_rax_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32);
	//xchg %rax, gs:disp32(%rsp)
	static std::string gen_xchg_rax_gs_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32);
	//movq %rax, disp32(%rsp)
	static std::string gen_movq_rax_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32);
	//movq disp32(%rsp), %rax
	static std::string gen_movq_rsp_smem_rax_instr(UINT16 &disp32_pos, INT32 disp32);
	//movq gs:disp32(%rsp), %rax
	static std::string gen_movq_gs_rsp_smem_rax_instr(UINT16 &disp32_pos, INT32 disp32);
	//nop
	static std::string gen_nop_instr();
	//movq %%rax, reg64
	static std::string gen_movq_reg_to_rax_instr(UINT8 reg_index);
	//addq 
	static std::string gen_addq_reg_imm32_instr(UINT8 reg_index, UINT16 &imm32_pos, INT32 imm32);
	//addq %rax, $imm32
	static std::string gen_addq_rax_imm32_instr(UINT16 &imm32_pos, INT32 imm32);
	//addq (%rsp), $imm32
	static std::string gen_addq_imm32_to_rsp_mem_instr(UINT16 &imm_pos, INT32 imm32);
	//addq disp8(%rsp), $imm32
	static std::string gen_addq_imm32_to_rsp_smem_disp8_instr(UINT16 &imm_pos, INT32 imm32, UINT16 &disp8_pos, INT8 disp8);
	//retq
	static std::string gen_retq_instr();
	//movl disp32(%rsp), $imm32
	static std::string gen_movl_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &disp32_pos, INT32 disp32);
	//movl gs:disp32(%rsp), $imm32
	static std::string gen_movl_imm32_to_gs_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &disp32_pos, INT32 disp32);
	//movl (%rsp), $imm32
	static std::string gen_movl_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32);
	//movl disp8(%rsp), $imm32
	static std::string gen_movl_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &disp8_pos, INT8 disp8);
	//movq disp32(%rsp), $imm32
	static std::string gen_movq_imm32_to_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &disp32_pos, INT32 disp32);
	//movq gs:disp32(%rsp), $imm32
	static std::string gen_movq_imm32_to_gs_rsp_smem_instr(UINT16 &imm32_pos, INT32 imm32, UINT16 &disp32_pos, INT32 disp32);
	//pushq disp32(%rsp)
	static std::string gen_pushq_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32);
	//pushq gs:disp32(%rsp)
	static std::string gen_pushq_gs_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32);
	//popq disp32(%rsp)
	static std::string gen_popq_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32);
	//popq gs:disp32(%rsp)
	static std::string gen_popq_gs_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32);
	//jmpq * %reg64
	static std::string gen_jmpq_reg(UINT8 reg_index);
	//jmpq disp32(%rsp)
	static std::string gen_jmpq_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32);
	//jmpq gs:disp32(%rsp)
	static std::string gen_jmpq_gs_rsp_smem_instr(UINT16 &disp32_pos, INT32 disp32);
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
	//cmp reg64, imm8
	static std::string gen_cmp_reg64_imm8_instr(UINT8 reg_index, UINT16 &imm8_pos, INT8 imm8);
	//je rel32
	static std::string gen_je_rel32_instr(UINT16 &rel32_pos, INT32 rel32);
	//jl rel8
	static std::string gen_jl_rel8_instr(UINT16 &rel8_pos, INT8 rel8, BOOL is_taken = true);
	//jns rel8
	static std::string gen_jns_rel8_instr(UINT16 &rel8_pos, INT8 rel8, BOOL is_taken = true);
	//convert functions
	static std::string convert_jumpin_reg_to_movq_rax_reg(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_callin_mem_to_movq_rax_mem(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_jumpin_mem_to_movq_rax_mem(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_jumpin_mem_to_movq_reg_mem(const UINT8 *instcode, UINT32 instsize, UINT8 reg_index);
	static std::string convert_jmpin_mem_to_cmp_mem_imm8(UINT8 *instcode, UINT16 instsize, UINT16 &imm8_pos, INT8 imm8);
	static std::string convert_jmpin_reg64_to_cmp_reg64_imm8(UINT8 *instcode, UINT16 instsize, UINT16 &imm8_pos, INT8 imm8);
	static std::string convert_jumpin_mem_to_cmpl_imm32(const UINT8 *instcode, UINT32 instsize, UINT16 &imm32_pos, INT32 imm32);
	static std::string convert_jumpin_mem_to_push_mem(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_jumpin_reg_to_push_reg(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_callin_reg_to_push_reg(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_callin_mem_to_push_mem(const UINT8 *instcode, UINT32 instsize);
	static std::string convert_cond_br_relx_to_rel32(const UINT8 *instcode, UINT32 inst_size, UINT16 &rel32_pos, INT32 rel32);
	static std::string convert_cond_br_relx_to_rel8(const UINT8 *instcode, UINT32 inst_size, UINT16 &rel8_pos, INT8 rel8);
	//modify functions
	static std::string modify_disp_of_pushq_rsp_mem(std::string src_template, INT8 addend);
	static std::string modify_disp_of_movq_rsp_mem(std::string src_template, INT8 addend);
};
