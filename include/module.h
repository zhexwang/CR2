#pragma once
#include <vector>
#include <map>
#include <string>

#include "type.h"
#include "elf-parser.h"

class Instruction;
class BasicBlock;
class Function;

class Module{
public:
	//typedef mapping file_offset to Instruction*/BasicBlock*/Function*'s entry point
	typedef std::map<F_SIZE, Instruction*> INSTR_MAP;
	typedef INSTR_MAP::const_iterator INSTR_MAP_ITERATOR;
	typedef std::map<F_SIZE, BasicBlock*> BBL_MAP;
	typedef BBL_MAP::const_iterator BBL_MAP_ITERATOR;
	typedef std::map<F_SIZE, Function*> FUNC_MAP;
	typedef FUNC_MAP::const_iterator FUNC_MAP_ITERATOR;
	//typedef mapping module name to Module* 
	typedef std::map<std::string, Module*> MODULE_MAP;
	typedef MODULE_MAP::iterator MODULE_MAP_ITERATOR;
protected:
	const ElfParser *_elf;
	INSTR_MAP _instr_maps;//all instructions
	BBL_MAP _bbl_maps;//all basic blocks
	FUNC_MAP _func_maps;//all functions, but ignore multi-entry
	BBL_MAP _func_left_bbl_maps;//out-of function region
	static MODULE_MAP _all_module_maps;
	//real load base
	P_ADDRX _real_load_base;
protected:
	void split_bbl();
	void analysis_indirect_jump_targets();
public:
	Module(const ElfParser *elf);
	~Module();
	//analysis
	static void split_all_modules_into_bbls();
	static void analysis_all_modules_indirect_jump_targets();
	//set functions
	void set_real_load_base(P_ADDRX load_base) {_real_load_base = load_base;}
	//get functions
	std::string get_path() const {return _elf->get_elf_path();}
	std::string get_name() const {return _elf->get_elf_name();}
	P_ADDRX get_pt_load_base() const {return _elf->get_pt_load_base();}
 	UINT8 *get_code_offset_ptr(const F_SIZE off) const {return _elf->get_code_offset_ptr(off);}
	//get functions
	Instruction *get_instr_by_off(const F_SIZE off) const;
	Instruction *get_instr_by_va(const P_ADDRX addr) const;
	BasicBlock *get_bbl_by_off(const F_SIZE off) const;
	BasicBlock *get_bbl_by_va(const P_ADDRX addr) const;
	Function *get_func_by_off(const F_SIZE off) const;
	Function *get_func_by_va(const P_ADDRX addr) const; 
	//insert functions
	void insert_instr(Instruction *instr);
	void insert_bbl(BasicBlock *bbl);
	void insert_func(Function *func);
};
