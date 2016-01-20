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
	//shuffle process information
	//code cache
	BOOL _curr_is_first_cc;
	S_ADDRX _cc1_base;
	S_ADDRX _cc2_base;
	std::string _cc_shm_path;
	INT32 _cc_fd;
	//protected process information
	//x region and code cache
	P_ADDRX _org_x_load_base;
	P_ADDRX _cc_load_base;
	P_SIZE _cc_load_size;
	//main stack and shadow stack
	static P_ADDRX _org_stack_load_base;
	static P_ADDRX _ss_load_base;
	static P_SIZE _ss_load_size;
	static std::string _ss_shm_path;
	static INT32 _ss_fd;
	//static vars
	static CVM_MAPS _all_cvm_maps;
	static std::string _code_variant_img_path;
	static SIZE _cc_offset;
	static SIZE _ss_offset;
	//shadow stack
public:
	static void init_protected_proc_info(PID protected_pid, SIZE cc_offset, SIZE ss_offset)
	{
		FATAL(ss_offset==0, "Current version only support shadow stack based on offset without gs segmentation!\n");
		_cc_offset = cc_offset;
		_ss_offset = ss_offset;
		parse_proc_maps(protected_pid);
		init_cc_and_ss();
	}
	static void add_cvm(CodeVariantManager *cvm)
	{
		_all_cvm_maps.insert(std::make_pair(cvm->get_name(), cvm));
	}
	//set functions
	static void parse_proc_maps(PID protected_pid);
	static void generate_all_code_variant();
	void generate_code_variant(BOOL is_first_cc);
	//get functions
	CodeVariantManager(std::string module_path);
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
	//init cc and ss
	void init_cc();
	static void init_cc_and_ss();
	void set_x_load_base(P_ADDRX load_base)
	{
		_org_x_load_base = load_base;
	}
	void set_cc_load_info(P_ADDRX cc_load_base, P_SIZE cc_size, std::string cc_shm_path)
	{
		_cc_load_base = cc_load_base;
		_cc_load_size = cc_size;
		_cc_shm_path = cc_shm_path;
		ASSERT(_cc_load_base==(_org_x_load_base+_cc_offset));
	}
	static void set_stack_load_base(P_ADDRX stack_load_base)
	{
		_org_stack_load_base = stack_load_base;
		ASSERT(_ss_load_base==(_org_stack_load_base-_ss_offset));
	}
	static void set_ss_load_info(P_ADDRX ss_load_base, P_SIZE ss_size, std::string ss_shm_path)
	{
		_ss_load_base = ss_load_base;
		_ss_load_size = ss_size;
		_ss_shm_path = ss_shm_path;
	}
	//get functions
	std::string get_name()
	{
		return _elf_real_name;
	}
};

