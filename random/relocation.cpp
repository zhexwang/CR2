#include "relocation.h"
#include "utility.h"
#include "disassembler.h"

RandomBBL::RandomBBL(F_SIZE origin_start, F_SIZE origin_end, BOOL has_lock_and_repeat_prefix, BOOL has_fallthrough_bbl, \
    std::vector<BBL_RELA> reloc_info, std::string random_template)
    : _origin_bbl_start(origin_start), _origin_bbl_end(origin_end), _has_lock_and_repeat_prefix(has_lock_and_repeat_prefix), \
        _has_fallthrough_bbl(has_fallthrough_bbl), _reloc_table(reloc_info), _random_template(random_template)
{
    ;
}

RandomBBL::~RandomBBL()
{
    ;
}

static inline S_ADDRX get_saddr_from_offset(F_SIZE offset, RBBL_CC_MAPS &rbbl_maps)
{
    RBBL_CC_MAPS::iterator ret = rbbl_maps.find(offset);
    ASSERTM(ret!=rbbl_maps.end(), "rbbl(offset: %lx) find failed!\n", offset);
    return ret->second;
}

void RandomBBL::gen_code(S_ADDRX cc_base, S_ADDRX gen_addr, S_SIZE gen_size, P_ADDRX orig_x_load_base, \
    P_SIZE cc_offset, P_SIZE ss_offset, RBBL_CC_MAPS &rbbl_maps, P_SIZE jmpin_offset)
{
    //code cache address in protected process
    P_ADDRX cc_load_base = orig_x_load_base + cc_offset;
    //the address of rbbl in protected process
    P_ADDRX curr_rbbl_in_prot = gen_addr - cc_base + cc_load_base;
    //the load address of origin bbl in protected process
    P_ADDRX curr_bbl_in_prot = orig_x_load_base + _origin_bbl_start;
    //1. copy template to target address
    SIZE template_size = _random_template.size();
    ASSERT(gen_size>=template_size);
    _random_template.copy((INT8 *)gen_addr, template_size);
    //2. relocate the rbbl
    for(BBL_RELA_VEC_ITER iter = _reloc_table.begin(); iter!=_reloc_table.end(); iter++){
        BBL_RELA &rela = *iter;
        S_ADDRX reloc_addr = rela.r_byte_pos + gen_addr;
        switch(rela.r_type){
            case RIP_RELA_TYPE:  
                {
                     //Disp2 = Disp1 + old_bbl_addr - new_bbl_addr + Offset1 - Offset2
                     //Offset1 - Offset2 = r_addend, Disp1 = r_value
                     ASSERTM(rela.r_byte_size==4, "we only handle 32bits displacement!\n");
                     INT32 new_disp32 = (INT32)rela.r_value + curr_bbl_in_prot - curr_rbbl_in_prot + rela.r_addend;
                     *(INT32*)reloc_addr = new_disp32;
                }
                break;
            case BRANCH_RELA_TYPE:
                {
                    //REL32 = bbl_target_addr - bbl_src_addr - Offset
                    //r_addend    = -Offset, r_value = target_addr
                    ASSERTM(rela.r_byte_size==4, "we only handle 32bits branch(loop loopnz... is not considered)!\n");
                    //find target rbbl in protected process
                    F_SIZE target_bbl_offset = rela.r_value;
                    S_ADDRX target_rbbl_in_shuf = get_saddr_from_offset(target_bbl_offset, rbbl_maps);
                    P_ADDRX target_rbbl_in_prot = target_rbbl_in_shuf - cc_base + cc_load_base;
                    //obtain the offset
                    INT32 new_rel32 = target_rbbl_in_prot - curr_rbbl_in_prot + rela.r_addend;
                    *(INT32*)reloc_addr = new_rel32;
                }
                break;
            case SS_RELA_TYPE:
                {
                    ASSERTM(rela.r_byte_size==4, "we only handle 32bits displacement!\n");
                    INT32 new_disp32 = (INT32)(rela.r_addend - ss_offset);
                    *(INT32*)reloc_addr = new_disp32;
                }
                break;
            case CC_RELA_TYPE:
                {
                    ASSERTM(rela.r_byte_size==4, "we only handle 32bits imm!\n");
                    INT32 new_imm32 = (INT32)(cc_offset + rela.r_addend);
                    *(INT32*)reloc_addr = new_imm32;
                }
                break;
            case HIGH32_CC_RELA_TYPE:
                {
                    //find target rbbl in protected process
                    F_SIZE target_bbl_offset = rela.r_value;
                    //if has no fallthrough instruction insert invalid instruction 
                    S_ADDRX target_rbbl_in_shuf = _has_fallthrough_bbl ? get_saddr_from_offset(target_bbl_offset, rbbl_maps) : cc_base;
                    P_ADDRX target_rbbl_in_prot = target_rbbl_in_shuf - cc_base + cc_load_base;
                    //relocate
                    *(INT32*)reloc_addr = (INT32)(target_rbbl_in_prot>>32);
                }
                break;
            case LOW32_CC_RELA_TYPE:
                {
                    //find target rbbl in protected process
                    F_SIZE target_bbl_offset = rela.r_value;
                    //if has no fallthrough instruction insert invalid instruction 
                    S_ADDRX target_rbbl_in_shuf = _has_fallthrough_bbl ? get_saddr_from_offset(target_bbl_offset, rbbl_maps) : cc_base;
                    P_ADDRX target_rbbl_in_prot = target_rbbl_in_shuf - cc_base + cc_load_base;
                    //relocate
                    *(INT32*)reloc_addr = (INT32)target_rbbl_in_prot;
                }
                break;
            case HIGH32_ORG_RELA_TYPE:
                {
                    //find target rbbl in protected process
                    F_SIZE target_bbl_offset = rela.r_value;
                    P_ADDRX target_bbl_in_prot = target_bbl_offset + orig_x_load_base;
                    //relocate
                    *(INT32*)reloc_addr = (INT32)(target_bbl_in_prot>>32);
                }
                break;
            case LOW32_ORG_RELA_TYPE:
                {
                    //find target rbbl in protected process
                    F_SIZE target_bbl_offset = rela.r_value;
                    P_ADDRX target_bbl_in_prot = target_bbl_offset + orig_x_load_base;
                    //relocate
                    *(INT32*)reloc_addr = (INT32)target_bbl_in_prot;
                }
                break;
            case TRAMPOLINE_RELA_TYPE:
                {
                    ASSERTM(rela.r_byte_size==4, "we only handle 32bits imm!\n");
                    ASSERTM(jmpin_offset!=0, "should have jmpin offset!\n");
                    INT32 new_imm32 = (INT32)(jmpin_offset + rela.r_addend);
                    *(INT32*)reloc_addr = new_imm32;
                }
                break;
            case DEBUG_HIGH32_RELA_TYPE:
                {
                    *(INT32*)reloc_addr = (INT32)(curr_rbbl_in_prot>>32);
                }
                break;
            case DEBUG_LOW32_RELA_TYPE:
                {
                    *(INT32*)reloc_addr = (INT32)curr_rbbl_in_prot;
                }
                break;
            default:
                ASSERT(0);
        }
    }
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
    TO_STRING_INTERNAL(DEBUG_LOW32_RELA_TYPE),  
    TO_STRING_INTERNAL(DEBUG_HIGH32_RELA_TYPE),
};

void RandomBBL::dump_relocation()
{
    INT32 entry_num = (INT32)_reloc_table.size();
    ERR(" No          Type           RelaPos RelaSize   Addend            Value\n");
    for(INT32 idx = 0; idx!=entry_num; idx++){
        BBL_RELA &rela = _reloc_table[idx];
        PRINT("%3d %22s %8x %8d %8x %16llx\n", idx, type_to_string[rela.r_type].c_str(), rela.r_byte_pos, rela.r_byte_size, \
            rela.r_addend, rela.r_value);
    }
}


