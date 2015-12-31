#pragma once

#include <map>

#include "type.h"
#include "utility.h"
#include "instruction.h"
#include "relocation.h"

class BasicBlock
{
public:
	typedef const std::map<F_SIZE, Instruction *> INSTR_MAPS;
	typedef std::map<F_SIZE, Instruction *>::const_iterator INSTR_MAPS_ITERATOR;
protected:
	const F_SIZE _start;
	const SIZE _size;
	const BOOL _is_call_proceeded;
	const BOOL _has_prefix;
	INSTR_MAPS _instr_maps;
	BOOL _is_nop;
	F_SIZE _target;
	F_SIZE _fallthrough;
public:
	BasicBlock(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	virtual ~BasicBlock();
	//get functions
	F_SIZE get_bbl_offset(F_SIZE &second_offset) const 
	{
		if(_has_prefix)
			second_offset = _start+1;
		else
			second_offset = 0;
		
		return _start;
	}
	F_SIZE get_bbl_end_offset() const
	{
		return _start+_size;
	}
	SIZE get_bbl_size() const
	{
		return _size;
	}
	INT32 get_instr_num() const {return (INT32)_instr_maps.size();}
	P_ADDRX get_bbl_paddr(const P_ADDRX load_base, F_SIZE &second_addrx) const 
	{
		if(_has_prefix)
			second_addrx = _start + load_base +1;
		else
			second_addrx = 0;
		
		return _start + load_base;
	}
	F_SIZE get_last_instr_offset() const {return _instr_maps.rbegin()->second->get_instr_offset();}
	const Module *get_module() const {return _instr_maps.begin()->second->get_module();}
	virtual const std::string get_type() const =0;
	F_SIZE get_target_offset() const {return _target;};
	F_SIZE get_fallthrough_offset() const {return _fallthrough;};
	BOOL has_prefix() const {return _has_prefix;}
	//dump functions
	void dump_in_va(const P_ADDRX load_base) const;
	void dump_in_off() const;
	BOOL is_in_bbl(F_SIZE offset) const {return offset>=_start && offset<(_start+_size);}
	BOOL is_in_bbl(Instruction *instr) const 
	{
		F_SIZE instr_offset = instr->get_instr_offset(); 
		return _instr_maps.find(instr_offset)!=_instr_maps.end() ? true : false;
	}
	BOOL is_nop() const {return _is_nop;}
	virtual BOOL is_sequence() const =0;
	virtual BOOL is_direct_call() const =0;
	virtual BOOL is_indirect_call() const =0;
	virtual BOOL is_direct_jump() const =0;
	virtual BOOL is_indirect_jump() const =0;
	virtual BOOL is_condition_branch() const =0;
	virtual BOOL is_ret() const =0;
	virtual std::string generate_code_template(std::vector<BBL_RELA> &rela) const =0;
};

class SequenceBBL : public BasicBlock
{
protected:	
	const static std::string _bbl_type;
public:
	SequenceBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	const std::string get_type() const {return _bbl_type;}
	~SequenceBBL();
	BOOL is_sequence() const {return true;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	std::string generate_code_template(std::vector<BBL_RELA> &reloc_vec) const;
};

class RetBBL : public BasicBlock
{
protected:
	const static std::string _bbl_type;
public:
	RetBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	const std::string get_type() const {return _bbl_type;}
	~RetBBL();
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return true;}
	std::string generate_code_template(std::vector<BBL_RELA> &reloc_vec) const;
};

class DirectCallBBL : public BasicBlock
{
protected:
	const static std::string _bbl_type;
public:
	DirectCallBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	const std::string get_type() const {return _bbl_type;}
	~DirectCallBBL();
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return true;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	std::string generate_code_template(std::vector<BBL_RELA> &reloc_vec) const;
};

class IndirectCallBBL : public BasicBlock
{
protected:
	const static std::string _bbl_type;
public:
	IndirectCallBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	const std::string get_type() const {return _bbl_type;}
	~IndirectCallBBL();
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return true;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	std::string generate_code_template(std::vector<BBL_RELA> &reloc_vec) const;
};

class DirectJumpBBL : public BasicBlock
{
protected:
	const static std::string _bbl_type;
public:
	DirectJumpBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	const std::string get_type() const {return _bbl_type;}
	~DirectJumpBBL();
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return true;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	std::string generate_code_template(std::vector<BBL_RELA> &reloc_vec) const;
};

class IndirectJumpBBL : public BasicBlock
{
protected:
	const static std::string _bbl_type;
public:
	IndirectJumpBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	const std::string get_type() const {return _bbl_type;}
	~IndirectJumpBBL();
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return true;}
	BOOL is_condition_branch() const {return false;}
	BOOL is_ret() const {return false;}
	std::string generate_code_template(std::vector<BBL_RELA> &reloc_vec) const;
};

class ConditionBrBBL : public BasicBlock
{
protected:
	const static std::string _bbl_type;
public:
	ConditionBrBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	const std::string get_type() const {return _bbl_type;}
	~ConditionBrBBL();
	BOOL is_sequence() const {return false;}
	BOOL is_direct_call() const {return false;}
	BOOL is_indirect_call() const {return false;}
	BOOL is_direct_jump() const {return false;}
	BOOL is_indirect_jump() const {return false;}
	BOOL is_condition_branch() const {return true;}
	BOOL is_ret() const {return false;}
	std::string generate_code_template(std::vector<BBL_RELA> &reloc_vec) const;
};



