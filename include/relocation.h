#pragma once

#include <vector>
#include <string>
#include "type.h"

enum INSTR_RELA_TYPE{
	RIP_RELA_TYPE = 0,//RIP_RELATIVE instruction
	BRANCH_RELA_TYPE, //direct call/jump/conditional branch	
	SS_RELA_TYPE,     //when shadow stack use offset with main stack(without using gs segmentation)
	INSTR_RELA_TYPE_NUM,
}__attribute((packed));

typedef struct{
	INSTR_RELA_TYPE r_type;
	UINT16 r_byte_pos; //need relocate position
	UINT16 r_byte_size;//relocation bytes num
	UINT16 r_base_pos; //base postion (pc)
	INT64 r_info;     // 1.rip_rela_type: displacement; 2. branch_rela_type: branch target
}INSTR_RELA;         //relocation the instruction template

typedef struct{

}BBL_RELA;


class RandomBBL{
protected:
	std::vector<BBL_RELA> _reloc_info;
	std::string _random_template;
public:
	RandomBBL(std::vector<BBL_RELA> reloc_info, std::string random_template);
	~RandomBBL();
	SIZE get_block_size()const {return _random_template.length();}
	/* @Args: random_offset represents the BBL's postion relative to the fixed postion in code cache
	 *        cc_offset represents the offset between the code cache with origin code region
	 *        ss_offset represents the offset between the shadow stack with main stack
	 *	      relocate_pos represents output the relocation code into relocate_pos address.
	 */
	void relocate(SIZE random_offset, SIZE cc_offset, SIZE ss_offset, S_ADDRX relocate_pos, SIZE relocate_size);
};

