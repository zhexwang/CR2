#pragma once

#include <map>

#include "type.h"
#include "utility.h"
#include "instruction.h"


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
	P_ADDRX get_bbl_paddr(const P_ADDRX load_base, F_SIZE &second_addrx) const 
	{
		if(_has_prefix)
			second_addrx = _start + load_base +1;
		else
			second_addrx = 0;
		
		return _start + load_base;
	}
	const Module *get_module() const {return _instr_maps.begin()->second->get_module();}
	virtual const std::string get_type() const =0;
	BOOL has_prefix() const {return _has_prefix;}
	//dump functions
	void dump_in_va(const P_ADDRX load_base) const;
	void dump_in_off() const;
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
};

class CallBBL : public BasicBlock
{
protected:
	const static std::string _bbl_type;
public:
	CallBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	const std::string get_type() const {return _bbl_type;}
	~CallBBL();
};

class JumpBBL : public BasicBlock
{
protected:
	const static std::string _bbl_type;
public:
	JumpBBL(const F_SIZE start, const SIZE size, BOOL is_call_proceeded, BOOL has_prefix,\
		BasicBlock::INSTR_MAPS &instr_maps);
	const std::string get_type() const {return _bbl_type;}
	~JumpBBL();
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
};



