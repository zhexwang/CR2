#pragma once

#include <map>
#include "type.h"
#include "utility.h"

#include "relocation.h"

class CodeVariantManager
{
public:
	typedef std::map<F_SIZE, RandomBBL*> RAND_BBL_MAPS;
	typedef std::map<std::string, CodeVariantManager*> CVM_MAPS;
protected:
	RAND_BBL_MAPS _postion_fixed_maps;
	RAND_BBL_MAPS _movable_maps;
	std::string _elf_path;
	S_ADDRX _cc_start1;
	S_ADDRX _cc_start2;
	SIZE _cc_size;
	//static vars
	static CVM_MAPS _all_cvm_maps;
	static std::string _code_variant_img_path;
	static SIZE _cc_offset;
	static SIZE _ss_offset;
public:
	static void init_protected_proc_info(SIZE cc_offset, SIZE ss_offset)
	{
		FATAL(ss_offset==0, "Current version only support shadow stack based on offset without gs segmentation!\n");
		_cc_offset = cc_offset;
		_ss_offset = ss_offset;
	}
	static void init_code_variant_image(std::string variant_img_path)
	{
		_code_variant_img_path = variant_img_path;
	}
	CodeVariantManager(std::string module_path, SIZE cc_size);
	~CodeVariantManager();
	//insert functions
	void insert_fixed_random_bbl(F_SIZE bbl_offset, RandomBBL *rand_bbl)
	{
		_postion_fixed_maps.insert(std::make_pair(bbl_offset, rand_bbl));
	}
	void insert_movable_random_bbl(F_SIZE bbl_offset, RandomBBL *rand_bbl)
	{
		_movable_maps.insert(std::make_pair(bbl_offset, rand_bbl));
	}
};

