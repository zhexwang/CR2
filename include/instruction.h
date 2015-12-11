#pragma once

#include <string>

#include "disasm_common.h"
#include "type.h"
#include "utility.h"

class Module;
class Instruction
{
	friend class Disassembler;
protected:
	const _DInst _dInst;
	UINT8 *_encode;
	//contained struct
	const Module *_module;
public:
	Instruction(const _DInst &dInst, const Module *module);
	virtual ~Instruction();
	// get functions
	const Module *get_module() const {return _module;}
	SIZE get_inst_size() const { return _dInst.size; }
	/*  @Introduction: get protected process's addr functions
					if load_base=0, return value represents the offset in elf.
	*/
	P_ADDRX get_inst_paddr(const P_ADDRX base_addr) const
	{
		return _dInst.addr + base_addr;
	}
	P_ADDRX get_next_paddr(const P_ADDRX load_base) const
	{
		return _dInst.addr + _dInst.size + load_base;
	}
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const =0;
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const =0;
	/*  @Introduction: get offset functions
					return value represents the offset of elf
	*/
	F_SIZE get_inst_offset() const { return _dInst.addr; }
	F_SIZE get_next_offset() const { return _dInst.addr + _dInst.size;}
	virtual F_SIZE get_fallthrough_offset() const =0;
	virtual F_SIZE get_target_offset() const =0;
	// judge functions
	virtual BOOL is_sequence() const =0;
	virtual BOOL is_direct_call() const =0;
	virtual BOOL is_indirect_call() const =0;
	virtual BOOL is_direct_jump() const =0;
	virtual BOOL is_indirect_jump() const =0;
	virtual BOOL is_condition_branch() const =0;
	virtual BOOL is_ret() const =0;
	virtual BOOL is_int() const =0;
	virtual BOOL is_sys() const =0;
	virtual BOOL is_cmov() const =0;
	BOOL is_rip_relative() const
	{
		return BITS_ARE_SET(_dInst.flags, FLAG_RIP_RELATIVE) ? true : false;
	}
	// bbl relatived function
	virtual BOOL is_bbl_end() const =0;
	// dump function
	void dump_pinst(const P_ADDRX load_base) const;
	void dump_file_inst() const;
};

class SequenceInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	SequenceInstr(const _DInst &dInst, const Module *module);
	~SequenceInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_paddr(load_base), _type_name.c_str());
		return 0;
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_paddr(load_base), _type_name.c_str());
		return 0;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_offset(), _type_name.c_str());
		return 0;
	}
	virtual F_SIZE get_target_offset() const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_offset(), _type_name.c_str());
		return 0;
	}
	//judge function
	BOOL is_sequence() const {return true;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	BOOL is_int() const {return false;}
	BOOL is_sys() const {return false;}
	BOOL is_cmov() const {return false;}
	BOOL is_bbl_end() const {return false;}
};

class DirectCallInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	DirectCallInstr(const _DInst &dInst, const Module *module);
	~DirectCallInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		return get_next_paddr(load_base);
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		return INSTRUCTION_GET_TARGET(&_dInst) + load_base;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		return get_next_offset();
	}
	virtual F_SIZE get_target_offset() const
	{
		return INSTRUCTION_GET_TARGET(&_dInst);
	}
	//judge function
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return true;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	BOOL is_int() const {return false;}
	BOOL is_sys() const {return false;}
	BOOL is_cmov() const {return false;}
	BOOL is_bbl_end() const {return true;}
};

class IndirectCallInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	IndirectCallInstr(const _DInst &dInst, const Module *module);
	~IndirectCallInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		return get_next_paddr(load_base);
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_paddr(load_base), _type_name.c_str());
		return 0;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		return get_next_offset();
	}
	virtual F_SIZE get_target_offset() const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_offset(), _type_name.c_str());
		return 0;
	}
	//judge function
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return true;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	BOOL is_int() const {return false;}
	BOOL is_sys() const {return false;}
	BOOL is_cmov() const {return false;}
	BOOL is_bbl_end() const {return true;}
};

class DirectJumpInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	DirectJumpInstr(const _DInst &dInst, const Module *module);
	~DirectJumpInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_paddr(load_base), _type_name.c_str());
		return 0;
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		return INSTRUCTION_GET_TARGET(&_dInst) + load_base;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_offset(), _type_name.c_str());
		return 0;
	}
	virtual F_SIZE get_target_offset() const
	{
		return INSTRUCTION_GET_TARGET(&_dInst);
	}
	//judge function
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return true;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	BOOL is_int() const {return false;}
	BOOL is_sys() const {return false;}
	BOOL is_cmov() const {return false;}
	BOOL is_bbl_end() const {return true;}
};

class IndirectJumpInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	IndirectJumpInstr(const _DInst &dInst, const Module *module);
	~IndirectJumpInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_paddr(load_base), _type_name.c_str());
		return 0;
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_paddr(load_base), _type_name.c_str());
		return 0;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_offset(), _type_name.c_str());
		return 0;
	}
	virtual F_SIZE get_target_offset() const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_offset(), _type_name.c_str());
		return 0;
	}
	//judge function
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return true;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	BOOL is_int() const {return false;}
	BOOL is_sys() const {return false;}
	BOOL is_cmov() const {return false;}
	BOOL is_bbl_end() const {return true;}
};

class ConditionBrInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	ConditionBrInstr(const _DInst &dInst, const Module *module);
	~ConditionBrInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		return get_next_paddr(load_base);
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		return INSTRUCTION_GET_TARGET(&_dInst) + load_base;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		return get_next_offset();
	}
	virtual F_SIZE get_target_offset() const
	{
		return INSTRUCTION_GET_TARGET(&_dInst);
	}
	//judge function
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return true;}
	BOOL is_ret() const {return false;}
	BOOL is_int() const {return false;}
	BOOL is_sys() const {return false;}
	BOOL is_cmov() const {return false;}
	BOOL is_bbl_end() const {return true;}
};

class RetInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	RetInstr(const _DInst &dInst, const Module *module);
	~RetInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_paddr(load_base), _type_name.c_str());
		return 0;
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_paddr(load_base), _type_name.c_str());
		return 0;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_offset(), _type_name.c_str());
		return 0;
	}
	virtual F_SIZE get_target_offset() const
	{
		ASSERTM(0, "Addr 0x%lx is %s instruction!\n", get_inst_offset(), _type_name.c_str());
		return 0;
	}
	//judge function
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return true;}
	BOOL is_int() const {return false;}
	BOOL is_sys() const {return false;}
	BOOL is_cmov() const {return false;}
	BOOL is_bbl_end() const {return true;}
};

class SysInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	SysInstr(const _DInst &dInst, const Module *module);
	~SysInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		return get_next_paddr(load_base);
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		return INSTRUCTION_GET_TARGET(&_dInst) + load_base;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		return get_next_offset();
	}
	virtual F_SIZE get_target_offset() const
	{
		return INSTRUCTION_GET_TARGET(&_dInst);
	}
	//judge function
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	BOOL is_int() const {return false;}
	BOOL is_sys() const {return true;}
	BOOL is_cmov() const {return false;}
	BOOL is_bbl_end() const {return false;}
};

class CmovInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	CmovInstr(const _DInst &dInst, const Module *module);
	~CmovInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		return get_next_paddr(load_base);
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		return INSTRUCTION_GET_TARGET(&_dInst) + load_base;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		return get_next_offset();
	}
	virtual F_SIZE get_target_offset() const
	{
		return INSTRUCTION_GET_TARGET(&_dInst);
	}
	//judge function
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	BOOL is_int() const {return false;}
	BOOL is_sys() const {return false;}
	BOOL is_cmov() const {return true;}
	BOOL is_bbl_end() const {return false;}	
};

class IntInstr: public Instruction
{
protected:
	static std::string _type_name;
public:
	IntInstr(const _DInst &dInst, const Module *module);
	~IntInstr();
	//get functions
 	virtual P_ADDRX get_fallthrough_paddr(const P_ADDRX load_base) const
	{
		return get_next_paddr(load_base);
	}
	virtual P_ADDRX get_target_paddr(const P_ADDRX load_base) const 
	{
		return INSTRUCTION_GET_TARGET(&_dInst) + load_base;
	}
	virtual F_SIZE get_fallthrough_offset() const
	{
		return get_next_offset();
	}
	virtual F_SIZE get_target_offset() const
	{
		return INSTRUCTION_GET_TARGET(&_dInst);
	}
	//judge function
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	BOOL is_int() const {return true;}
	BOOL is_sys() const {return false;}
	BOOL is_cmov() const {return false;}
	BOOL is_bbl_end() const {return false;}
};

