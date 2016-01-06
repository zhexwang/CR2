#pragma once

#include <vector>
#include <string>
#include "type.h"

enum RELA_TYPE{
	RIP_RELA_TYPE = 0,   //RIP_RELATIVE instruction
	BRANCH_RELA_TYPE,    //direct call/jump/conditional branch/fallthrough	
	SS_RELA_TYPE,        //when shadow stack use offset with main stack(without using gs segmentation)
	CC_RELA_TYPE,        //indirect jump/indirect call instructions will jump to code cache (+offset)
	HIGH32_CC_RELA_TYPE, //the direct addreess of the high 32 bits in code cache
	LOW32_CC_RELA_TYPE,  //the direct address of the low 32 bits in code cache
	LOW32_ORG_RELA_TYPE, //the direct address of the low 32 bits in origin code region
	TRAMPOLINE_RELA_TYPE,//recognized jmpin instructions will jump to their own trampolines 
	INSTR_RELA_TYPE_NUM,
}__attribute((packed));

typedef struct{
	RELA_TYPE r_type;
	UINT16    r_byte_pos; //need relocate position
	UINT16    r_byte_size;//relocation bytes num
	UINT16    r_base_pos; //base postion (pc)
	INT64     r_value;    //1.rip_rela_type: displacement; 2. branch_rela_type: branch target; 3.high32/low32 CC/ORG rela type: F_SIZE addr
}INSTR_RELA;             //3. SS rela type: addend; 4.CC rela type: addend

typedef struct{
	RELA_TYPE r_type; 
	UINT16    r_byte_pos;   //need relocate position
	UINT16    r_byte_size;  //need relocate size
	INT16     r_addend;     //relocation addend, normalize the instr relocation to bbl start relocation info
	INT64     r_value;      //1.rip_rela_type: displacement; 2. branch_rela_type: branch target; 3.high32/low32 CC/ORG rela type: F_SIZE addr
}BBL_RELA;                 

/************RIP Relocation******************
        VA(hight->low)               Relocation 
new_instr_addr  |-Disp2-|
                |       |   Disp2 = Disp1 + old_instr_addr - new_instr_addr
				|Offset2|         
                |       |   Disp2 = Disp1 + old_bbl_addr - new_bbl_addr + Offset1 - Offset2
new_bbl_addr    |-------|   
                  .....     
                            BBL_RELA:    
old_instr_addr  |-Disp1-|        
                |       |       r_addend    = Offset1 - Offset2
				|Offset1|       r_value     = Disp1
                |       |       r_byte_size = 4 (displacement)
old_bbl_addr    |-------|
********************************************/

/************BRANCH Relocation******************
        VA(hight->low)               Relocation 
jump_rel32      |-REL32-|
                |       |   REL32 = bbl_target_addr - jump_rel32_next_pc
				| Offset|         
                |       |   REL32 = bbl_target_addr - bbl_src_addr - Offset
bbl_src_addr    |-------|   
                  .....     
                            BBL_RELA:    
bbl_target_end  |-------|        
                |       |       r_addend    = -Offset
				|       |       r_value     = bbl_target_addr(F_SIZE, not P_ADDRX)
                |       |       r_byte_size = 4 (rel32)
bbl_target_addr |-------|      
********************************************/


typedef std::vector<INSTR_RELA> INSTR_RELA_VEC;
typedef std::vector<INSTR_RELA>::iterator INSTR_RELA_VEC_ITER;
typedef std::vector<BBL_RELA> BBL_RELA_VEC;
typedef std::vector<BBL_RELA>::iterator BBL_RELA_VEC_ITER;

class RandomBBL{
protected:
	F_SIZE _origin_bbl_start;
	F_SIZE _origin_bbl_end;
	BOOL _has_lock_and_repeat_prefix;
	BBL_RELA_VEC _reloc_table;
	std::string _random_template;
public:
	RandomBBL(F_SIZE origin_start, F_SIZE origin_end, BOOL has_lock_and_repeat_prefix, \
		std::vector<BBL_RELA> reloc_info, std::string random_template);
	~RandomBBL();
	SIZE get_block_size()const {return _random_template.length();}
	/* @Args: random_offset represents the BBL's postion relative to the fixed postion in code cache
	 *        cc_offset represents the offset between the code cache with origin code region
	 *        ss_offset represents the offset between the shadow stack with main stack
	 *	      relocate_pos represents output the relocation code into relocate_pos address.
	 */
	void relocate(SIZE random_offset, SIZE cc_offset, SIZE ss_offset, S_ADDRX relocate_pos, SIZE relocate_size);
	void dump_template(P_ADDRX relocation_base);
	void dump_relocation();
};
