#include "relocation.h"
#include "utility.h"
#include "disassembler.h"

RandomBBL::RandomBBL(F_SIZE origin_start, F_SIZE origin_end, BOOL has_lock_and_repeat_prefix, \
    std::vector<BBL_RELA> reloc_info, std::string random_template)
    : _origin_bbl_start(origin_start), _origin_bbl_end(origin_end), _has_lock_and_repeat_prefix(has_lock_and_repeat_prefix), \
        _reloc_table(reloc_info), _random_template(random_template)
{
    ;
}

RandomBBL::~RandomBBL()
{
    ;
}

void RandomBBL::relocate(S_ADDRX gen_addr, SIZE cc_offset, SIZE ss_offset, RBBL_CC_MAPS &rbbl_maps, JMPIN_CC_OFFSET &jmpin_offsets)
{
    NOT_IMPLEMENTED(wangzhe);
}

void RandomBBL::dump_template(P_ADDRX relocation_base)
{
    ERR("RandomBBL: origin_bbl_range[%lx, %lx) template_size(%d) relocation_num(%d)\n", \
        _origin_bbl_start, _origin_bbl_end, (INT32)_random_template.length(), (INT32)_reloc_table.size());
    Disassembler::dump_string(_random_template, relocation_base); 
}

static std::string type_to_string[] = {
	TO_STRING_INTERNAL(RIP_RELA_TYPE),      //RIP_RELATIVE instruction
	TO_STRING_INTERNAL(BRANCH_RELA_TYPE),   //direct call/jump/conditional branch/fallthrough	
	TO_STRING_INTERNAL(SS_RELA_TYPE),       //when shadow stack use offset with main stack(without using gs segmentation)
	TO_STRING_INTERNAL(CC_RELA_TYPE),       //indirect jump/indirect call instructions will jump to code cache (+offset)
	TO_STRING_INTERNAL(HIGH32_CC_RELA_TYPE),//the direct addreess of the high 32 bits in code cache
	TO_STRING_INTERNAL(LOW32_CC_RELA_TYPE), //the direct address of the low 32 bits in code cache
    TO_STRING_INTERNAL(HIGH32_ORG_RELA_TYPE),//the direct address of the high 32 bits in orign code region
    TO_STRING_INTERNAL(LOW32_ORG_RELA_TYPE), //the direct address of the low 32 bits in origin code region
    TO_STRING_INTERNAL(TRAMPOLINE_RELA_TYPE),//recognized jmpin instructions will jump to their own trampolines 

};
void RandomBBL::dump_relocation()
{
    INT32 entry_num = (INT32)_reloc_table.size();
    ERR(" No          Type         RelaPos RelaSize   Addend            Value\n");
    for(INT32 idx = 0; idx!=entry_num; idx++){
        BBL_RELA &rela = _reloc_table[idx];
        PRINT("%3d %20s %8x %8d %8x %16llx\n", idx, type_to_string[rela.r_type].c_str(), rela.r_byte_pos, rela.r_byte_size, \
            rela.r_addend, rela.r_value);
    }
}


