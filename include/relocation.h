#pragma once

#include <vector>
#include <string>
#include <map>
#include "type.h"
#include "netlink.h"

enum RELA_TYPE{
	RIP_RELA_TYPE = 0,   //RIP_RELATIVE instruction
	BRANCH_RELA_TYPE,    //direct call/jump/conditional branch/fallthrough	
	SS_RELA_TYPE,        //when shadow stack use offset with main stack(without using gs segmentation)
	CC_RELA_TYPE,        //indirect jump/indirect call instructions will jump to code cache (+offset)
	HIGH32_CC_RELA_TYPE, //the direct addreess of the high 32 bits in code cache
	LOW32_CC_RELA_TYPE,  //the direct address of the low 32 bits in code cache
	HIGH32_ORG_RELA_TYPE,//the direct address of the high 32 bits in orign code region
	LOW32_ORG_RELA_TYPE, //the direct address of the low 32 bits in origin code region
	TRAMPOLINE_RELA_TYPE,//recognized jmpin instructions will jump to their own trampolines 
	DEBUG_LOW32_RELA_TYPE,  
	DEBUG_HIGH32_RELA_TYPE,
	INSTR_RELA_TYPE_NUM,
}__attribute((packed));

typedef struct{
	RELA_TYPE r_type;
	UINT16    r_byte_pos; //need relocate position
	UINT16    r_byte_size;//relocation bytes num
	UINT16    r_base_pos; //base postion (pc)
	INT64     r_value;    //1.rip_rela_type: displacement; 2. branch_rela_type: branch target; 3.high32/low32 CC/ORG rela type: F_SIZE addr
}INSTR_RELA;             //3. SS rela type: addend; 4.CC rela type: addend; 5. trampoline rela type: callin and jmpin address in elf

typedef struct{
	RELA_TYPE r_type; 
	UINT16    r_byte_pos;   //need relocate position
	UINT16    r_byte_size;  //need relocate size
	INT32     r_addend;     //relocation addend, normalize the instr relocation to bbl start relocation info
	INT64     r_value;      //1.rip_rela_type: displacement; 2. branch_rela_type: branch target; 3.high32/low32 CC/ORG rela type: F_SIZE addr
}BBL_RELA;                //4.Trampoline rela type: callin or jmpin address in elf 

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
typedef std::map<F_SIZE, S_ADDRX> RBBL_CC_MAPS;
typedef std::map<F_SIZE, P_SIZE> JMPIN_CC_OFFSET;

class RandomBBL
{
protected:
	F_SIZE _origin_bbl_start;
	F_SIZE _origin_bbl_end;
	BOOL _has_lock_and_repeat_prefix;
	BOOL _has_fallthrough_bbl;
	BBL_RELA_VEC _reloc_table;
	std::string _random_template;
public:
	RandomBBL(F_SIZE origin_start, F_SIZE origin_end, BOOL has_lock_and_repeat_prefix, BOOL has_fallthrough_bbl, \
		std::vector<BBL_RELA> reloc_info, std::string random_template);
	~RandomBBL();
	BOOL has_lock_and_repeat_prefix() const {return _has_lock_and_repeat_prefix;}
	BOOL has_fallthrough_bbl() const {return _has_fallthrough_bbl;}
	F_SIZE get_rbbl_offset() const {return _origin_bbl_start;}
	SIZE get_template_size()const {return _random_template.length();}
	/* @Args: 
	 *        cc_base represents the allocate address of code cache
	 *		  gen_addr represents the BBL's postion in code cache
	 *		  gen_size represents the space to generate
	 *        orig_x_load_base represents the load address of origin x region
	 *        cc_offset represents the offset between the code cache with origin code region
	 *        ss_offset represents the offset between the shadow stack with main stack
	 */
	void gen_code(S_ADDRX cc_base, S_ADDRX gen_addr, S_SIZE gen_size, P_ADDRX orig_x_load_base, P_SIZE cc_offset, P_SIZE ss_offset, \
		P_ADDRX gs_base, LKM_SS_TYPE ss_type, RBBL_CC_MAPS &rbbl_maps, P_SIZE jmpin_offset);
	//This function is used to judge the last br target is fallthrough rbbl or not, if the fallthrough rbbl is follow by current rbbl,
	//we have the chance to reduce the last jmp rel32 instruction!
	F_SIZE get_last_br_target() const
	{
		#define REL32_LEN 4
		BBL_RELA_VEC::const_reverse_iterator iter = _reloc_table.rbegin();
		if(iter!=_reloc_table.rend()){
			BBL_RELA rela = *iter;
			if((rela.r_type==BRANCH_RELA_TYPE) && (rela.r_byte_pos==(_random_template.length()-REL32_LEN)) && (rela.r_byte_size==4))
				return rela.r_value;
			else
				return 0;
		}else
			return 0;
	}
	SIZE store_rbbl(S_ADDRX s_addrx);
	static RandomBBL *read_rbbl(S_ADDRX r_addrx, SIZE &used_size);
	void dump_template(P_ADDRX relocation_base);
	void dump_relocation();
};

