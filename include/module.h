#pragma once
#include <vector>
#include <map>
#include <string>
#include <set>
#include <list>

#include "type.h"
#include "elf-parser.h"

class CodeVariantManager;
class Instruction;
class BasicBlock;

class Module{
    friend class PinProfile;
	friend class Disassembler;
public:
	//typedef mapping file_offset to Instruction*/BasicBlock*/Function*'s entry point
	typedef std::map<F_SIZE, Instruction*> INSTR_MAP;
	typedef INSTR_MAP::const_iterator INSTR_MAP_ITERATOR;
	typedef std::map<F_SIZE, BasicBlock*> BBL_MAP;
	typedef BBL_MAP::const_iterator BBL_MAP_ITERATOR;
	//typedef branch target list
	typedef std::set<F_SIZE> BR_TARGET_SRCS;
	typedef const std::set<F_SIZE> CONST_BR_TARGET_SRCS;
	typedef BR_TARGET_SRCS::iterator BR_TARGET_SRCS_ITERATOR;
	typedef BR_TARGET_SRCS::const_iterator BR_TARGET_SRCS_CONST_ITER;
	typedef std::map<F_SIZE, BR_TARGET_SRCS> BR_TARGETS;
	typedef BR_TARGETS::iterator BR_TARGETS_ITERATOR;
	typedef BR_TARGETS::const_iterator BR_TARGETS_CONST_ITER;
	typedef std::set<F_SIZE> CALL_TARGETS;
	//typedef aligned entry
	typedef std::set<F_SIZE> ALIGN_ENTRY;
	//typedef likey function entry
	enum FUNC_TYPE{
		CALL_TARGET = 0,
		PROLOG_MATCH,
		SYM_RECORD,
		RELA_TARGET,
		ALIGNED_ENTRY,
		FUNC_TYPE_NUM,
	};
	typedef std::map<BasicBlock*, FUNC_TYPE> FUNC_ENTRY_BBL_MAP;
	//typedef BBL set
	typedef std::set<BasicBlock*> BBL_SET;
	//typedef indirect jump targets
	enum JUMPIN_TYPE{
		UNKNOW = 0,
		SWITCH_CASE_OFFSET,
		SWITCH_CASE_ABSOLUTE,
		PLT_JMP,
		LONG_JMP,
		MEMSET_JMP,
		TYPE_SUM,
	};
	typedef struct indirect_jump_info{
		JUMPIN_TYPE type;
		F_SIZE table_offset;//jump table offset
		SIZE table_size;
		std::set<F_SIZE> targets;
	}JUMPIN_INFO;
	typedef std::map<F_SIZE, JUMPIN_INFO> JUMPIN_MAP;
	typedef JUMPIN_MAP::iterator JUMPIN_MAP_ITER;
	//typedef mapping module name to Module* 
	typedef std::map<std::string, Module*> MODULE_MAP;
	typedef MODULE_MAP::iterator MODULE_MAP_ITERATOR;
protected:
	ElfParser *_elf;
	INSTR_MAP _instr_maps;//all instructions
	BBL_MAP _bbl_maps;//all basic blocks
	BR_TARGETS _br_targets;// direct jump/call, conditional branch and recoginized jump table 
	static MODULE_MAP _all_module_maps;
	//call target
	CALL_TARGETS _call_targets;
	//aligned entry
	ALIGN_ENTRY _align_entries;
	//all indirect jump
	JUMPIN_MAP _indirect_jump_maps;
	//real load base
	P_ADDRX _real_load_base;
	//plt range
	F_SIZE _plt_start, _plt_end;
	//maybe function entry BBL(symbol function, call target and prolog match)
	FUNC_ENTRY_BBL_MAP _maybe_func_entry;
	//symbol function information
	SYM_FUNC_INFO_MAP _func_info_maps;
	//plt information
	PLT_INFO_MAP _plt_info_map;
	//rela targets in x sections
	RELA_X_TARGETS _rela_targets;
	//special handling 
	F_SIZE _setjmp_plt;
	//bbl's entry must be fixed
	BBL_SET _fixed_bbls;
	BBL_SET _movable_bbls;
	static const std::string func_type_name[FUNC_TYPE_NUM];
	//cvm
	CodeVariantManager *_cvm;
protected:
	void split_bbl();
	void analysis_indirect_jump_targets();
	BOOL analysis_jump_table_in_main(F_SIZE jump_offset, F_SIZE &table_base, SIZE &table_size, std::set<F_SIZE> &targets);
	BOOL analysis_jump_table_in_so(F_SIZE jump_offset, F_SIZE &table_base, SIZE &table_size, std::set<F_SIZE> &targets);
	BOOL analysis_memset_jump(F_SIZE jump_offset, std::set<F_SIZE> &targets);
	void separate_movable_bbls();
	void recursive_to_find_movable_bbls(BasicBlock *bbl);
	BasicBlock *construct_bbl(const INSTR_MAP &instr_maps, BOOL is_call_proceeded, BOOL is_call_setjmp_proceeded);
public:
	Module(ElfParser *elf);
	~Module();
	//analysis
	static void split_all_modules_into_bbls();
	static void analysis_all_modules_indirect_jump_targets();
	static void separate_movable_bbls_from_all_modules();
	static void generate_all_relocation_block();
	static void init_cvm_from_modules();
	//check functions
	/*	@Arguments: none
		@Return value: none
		@Introduction: check if all br targets are exist in module instruction list! 
	*/
	void check_br_targets();
	//set functions
	void set_real_load_base(P_ADDRX load_base) {_real_load_base = load_base;}
	void set_cvm(CodeVariantManager *cvm);
	//get functions
	std::string  get_path() const {return _elf->get_elf_path();}
	std::string  get_name() const {return _elf->get_elf_name();}
	std::string  get_sym_func_name(F_SIZE offset) const;
	P_ADDRX      get_pt_load_base() const {return _elf->get_pt_load_base();}
	P_ADDRX      get_pt_x_load_base() const {return _elf->get_pt_x_load_base();}
	F_SIZE       convert_pt_addr_to_offset(const P_ADDRX addr) const {return _elf->convert_pt_addr_to_offset(addr);}
 	UINT8       *get_code_offset_ptr(const F_SIZE off) const {return _elf->get_code_offset_ptr(off);}
	Instruction *get_instr_by_off(const F_SIZE off) const;
	Instruction *get_instr_by_va(const P_ADDRX addr) const;
	BasicBlock  *get_bbl_by_off(const F_SIZE off) const;
	BasicBlock  *get_bbl_by_va(const P_ADDRX addr) const;
	//find function
	Instruction *find_instr_by_off(F_SIZE offset, BOOL consider_prefix) const;
	Instruction *find_instr_cover_offset(F_SIZE offset) const;
	BasicBlock  *find_bbl_by_offset(F_SIZE offset, BOOL consider_prefix) const;
	BasicBlock  *find_bbl_cover_offset(F_SIZE offset) const;
	BasicBlock  *find_bbl_cover_instr(Instruction *instr) const;	
	//judge functions
	BOOL is_instr_entry_in_off(const F_SIZE target_offset, BOOL consider_prefix) const;
	BOOL is_bbl_entry_in_off(const F_SIZE target_offset, BOOL consider_prefix) const;
	BOOL is_instr_entry_in_va(const P_ADDRX addr, BOOL consider_prefix) const;
	BOOL is_bbl_entry_in_va(const P_ADDRX addr, BOOL consider_prefix) const;
	BOOL is_in_plt_in_off(const F_SIZE offset) const;
	BOOL is_in_plt_in_va(const P_ADDRX addr) const;
	BOOL is_br_target(const F_SIZE target_offset) const;
	BOOL is_call_target(const F_SIZE offset) const;
	BOOL is_align_entry(F_SIZE offset) const;
	BOOL is_maybe_func_entry(F_SIZE offset) const;
	BOOL is_maybe_func_entry(BasicBlock *bb) const;
	BOOL is_fixed_bbl(BasicBlock *bbl);
	BOOL is_movable_bbl(BasicBlock *bbl);
	BOOL is_sym_func_entry(F_SIZE offset) const
	{
		return _func_info_maps.find(offset)!=_func_info_maps.end();
	}
	BOOL is_rela_target(F_SIZE offset) const
	{
		return _rela_targets.find(offset)!=_rela_targets.end();
	}
	BOOL is_in_x_section_in_off(const F_SIZE offset) const
	{
		return _elf->is_in_x_section_file(offset);
	}
	BOOL is_shared_object() const
	{
		return _elf->is_shared_object();
	}
	void generate_relocation_block();
	//insert functions
	void insert_br_target(const F_SIZE target, const F_SIZE src);
	void insert_call_target(const F_SIZE target);
	void insert_instr(Instruction *instr);
	void insert_bbl(BasicBlock *bbl);
	void insert_align_entry(F_SIZE offset);
	void insert_indirect_jump(F_SIZE offset);
	//erase functions
	void erase_instr(Instruction *instr);
	void erase_br_target(const F_SIZE target, const F_SIZE src);
	void erase_instr_range(F_SIZE ,F_SIZE ,std::vector<Instruction*> &);    
	//read functions
	UINT8 read_1byte_code_in_off(const F_SIZE read_offset) const
	{
		return _elf->read_1byte_code_in_off(read_offset);
	}
	INT16 read_2byte_data_in_off(const F_SIZE read_offset) const
	{
		return _elf->read_2byte_data_in_off(read_offset);
	}
	INT32 read_4byte_data_in_off(const F_SIZE read_offset) const
	{
		return _elf->read_4byte_data_in_off(read_offset);
	}
	INT64 read_8byte_data_in_off(const F_SIZE read_offset) const
	{
		return _elf->read_8byte_data_in_off(read_offset);
	}
	//dump functions
	static void dump_all_bbls_in_va(P_ADDRX load_base);
	static void dump_all_bbls_in_off();
	static void dump_all_indirect_jump_result();
	static void dump_all_bbl_movable_info();
	void dump_bbl_in_va(P_ADDRX load_base) const;
	void dump_bbl_in_off() const;
	void dump_br_target(const F_SIZE target_offset) const;
	void dump_indirect_jump_result();
	void dump_bbl_movable_info() const;
};
