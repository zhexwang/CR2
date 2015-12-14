#pragma once
#include <vector>
#include <map>
#include <string>
#include <set>

#include "type.h"
#include "elf-parser.h"

class Instruction;
class BasicBlock;
class Function;

class Module{
    friend class PinProfile;
	friend class Disassembler;
public:
	//typedef mapping file_offset to Instruction*/BasicBlock*/Function*'s entry point
	typedef std::map<F_SIZE, Instruction*> INSTR_MAP;
	typedef INSTR_MAP::const_iterator INSTR_MAP_ITERATOR;
	typedef std::map<F_SIZE, BasicBlock*> BBL_MAP;
	typedef BBL_MAP::const_iterator BBL_MAP_ITERATOR;
	typedef std::map<F_SIZE, Function*> FUNC_MAP;
	typedef FUNC_MAP::const_iterator FUNC_MAP_ITERATOR;
	//typedef branch target list
	typedef std::multiset<F_SIZE> BR_TARGETS;
	typedef std::multiset<F_SIZE>::iterator BR_TARGETS_ITERATOR;
	//typedef mapping module name to Module* 
	typedef std::map<std::string, Module*> MODULE_MAP;
	typedef MODULE_MAP::iterator MODULE_MAP_ITERATOR;
protected:
	const ElfParser *_elf;
	INSTR_MAP _instr_maps;//all instructions
	BBL_MAP _bbl_maps;//all basic blocks
	FUNC_MAP _func_maps;//all functions, but ignore multi-entry
	BBL_MAP _func_left_bbl_maps;//out-of function region
	BR_TARGETS _br_targets;
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
	//check functions
	/*	@Arguments: none
		@Return value: none
		@Introduction: check if all br targets are exist in module instruction list! 
	*/
	void check_br_targets() const;
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
	//judge functions
	BOOL is_instr_entry_in_off(const F_SIZE target_offset) const;
	BOOL is_bbl_entry_in_off(const F_SIZE target_offset) const;
	BOOL is_func_entry_in_off(const F_SIZE target_offset) const;
	BOOL is_instr_entry_in_va(const P_ADDRX addr) const;
	BOOL is_bbl_entry_in_va(const P_ADDRX addr) const;
	BOOL is_func_entry_in_va(const P_ADDRX addr) const;
	BOOL is_br_target(const F_SIZE target_offset) const;
	//insert functions
	void insert_br_target(const F_SIZE target_offset);
	void insert_instr(Instruction *instr);
	void insert_bbl(BasicBlock *bbl);
	void insert_func(Function *func);
	//erase functions
	void erase_instr(Instruction *instr);
	void erase_instr_range(F_SIZE ,F_SIZE ,std::vector<Instruction*> &);    
	//read functions
	UINT8 read_x_byte_in_off(const F_SIZE read_offset) const
	{
		return _elf->read_x_byte_in_off(read_offset);
	}
	BOOL is_in_x_section_in_off(const F_SIZE offset) const
	{
		return _elf->is_in_x_section_file(offset);
	}
};
