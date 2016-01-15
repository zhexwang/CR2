#pragma once

#include <map>
#include <set>

#include "type.h"
#include "utility.h"
#include "relocation.h"

class CodeVariantManager
{
public:
	typedef std::map<F_SIZE, RandomBBL*> RAND_BBL_MAPS;
	typedef std::set<F_SIZE> TARGET_SET;
	typedef TARGET_SET::iterator TARGET_ITERATOR;
	typedef std::map<F_SIZE, TARGET_SET> JMPIN_TARGETS_MAPS;//first is src_bbl
	typedef JMPIN_TARGETS_MAPS::iterator JMPIN_ITERATOR;
	typedef std::map<std::string, CodeVariantManager*> CVM_MAPS;
protected:
	RAND_BBL_MAPS _postion_fixed_rbbl_maps;
	RAND_BBL_MAPS _movable_rbbl_maps;
	JMPIN_TARGETS_MAPS _jmpin_rbbl_maps;
	std::string _elf_real_name;
	S_ADDRX _cc_start1;
	S_ADDRX _cc_start2;
	SIZE _cc_size;
	P_ADDRX _load_base;
	//static vars
	static CVM_MAPS _all_cvm_maps;
	static std::string _code_variant_img_path;
	static SIZE _cc_offset;
	static SIZE _ss_offset;
public:
	static void init_protected_proc_info(SIZE cc_offset, SIZE ss_offset, PID protected_pid)
	{
		FATAL(ss_offset==0, "Current version only support shadow stack based on offset without gs segmentation!\n");
		_cc_offset = cc_offset;
		_ss_offset = ss_offset;
		parse_proc_maps(protected_pid);
	}
	static void add_cvm(CodeVariantManager *cvm)
	{
		_all_cvm_maps.insert(std::make_pair(cvm->get_name(), cvm));
	}
	//set functions
	static void parse_proc_maps(PID protected_pid);
	static S_ADDRX generate_code_variant(S_ADDRX curr_pc);
	//get functions
	CodeVariantManager(std::string module_path, SIZE cc_size);
	~CodeVariantManager();
	//insert functions
	void insert_fixed_random_bbl(F_SIZE bbl_offset, RandomBBL *rand_bbl)
	{
		_postion_fixed_rbbl_maps.insert(std::make_pair(bbl_offset, rand_bbl));
	}
	void insert_movable_random_bbl(F_SIZE bbl_offset, RandomBBL *rand_bbl)
	{
		_movable_rbbl_maps.insert(std::make_pair(bbl_offset, rand_bbl));
	}
	void insert_jmpin_rbbl(F_SIZE src_bbl_offset, F_SIZE target_bbl_offset)
	{
		JMPIN_ITERATOR iter = _jmpin_rbbl_maps.find(src_bbl_offset);
		if(iter!=_jmpin_rbbl_maps.end()){
			TARGET_SET &targets = iter->second;
			targets.insert(target_bbl_offset);
		}else{
			TARGET_SET targets;
			_jmpin_rbbl_maps.insert(std::make_pair(src_bbl_offset, targets));
		}
	}
	void insert_jmpin_rbbl(F_SIZE src_bbl_offset, TARGET_SET targets)
	{
		_jmpin_rbbl_maps.insert(std::make_pair(src_bbl_offset, targets));
	}
	void set_load_base(P_ADDRX load_base)
	{
		_load_base = load_base;
	}
	std::string get_name()
	{
		return _elf_real_name;
	}
	SIZE get_cc_size()
	{
		return _cc_size;
	}
};

