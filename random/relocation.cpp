#include "relocation.h"
#include "utility.h"
#include "disassembler.h"

RandomBBL::RandomBBL(F_SIZE origin_start, F_SIZE origin_end, BOOL has_lock_and_repeat_prefix, BOOL has_fallthrough_bbl, \
    std::vector<BBL_RELA> reloc_info, std::string random_template)
    :_origin_bbl_start(origin_start), _origin_bbl_end(origin_end), _has_lock_and_repeat_prefix(has_lock_and_repeat_prefix), \
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
    P_SIZE cc_offset, P_SIZE ss_offset, P_ADDRX gs_base, LKM_SS_TYPE ss_type, RBBL_CC_MAPS &rbbl_maps, P_SIZE jmpin_offset)
{
    //code cache address in protected process
    P_ADDRX cc_load_base = orig_x_load_base + cc_offset;
    P_SIZE virtual_ss_offset = 0;//virtual shadow stack offset is used to relocate the instruction
    switch(ss_type){
        case LKM_OFFSET_SS_TYPE: virtual_ss_offset = ss_offset; break;
        case LKM_SEG_SS_TYPE: virtual_ss_offset = ss_offset + gs_base; ASSERT(gs_base!=0); break;
        case LKM_SEG_SS_PP_TYPE: virtual_ss_offset = ss_offset + gs_base; ASSERT(gs_base!=0); break;
        default:
            ASSERTM(0, "Unkown shadow stack type %d\n", ss_type);
    }
    //the address of rbbl in protected process
    P_ADDRX curr_rbbl_in_prot = gen_addr - cc_base + cc_load_base;
    //the load address of origin bbl in protected process
    P_ADDRX curr_bbl_in_prot = orig_x_load_base + _origin_bbl_start;
    //1. copy template to target address
    ASSERT(gen_size>=_random_template.size() || gen_size==(_random_template.size()-5));//optimize
    _random_template.copy((INT8 *)gen_addr, gen_size);
    //2. relocate the rbbl
    for(BBL_RELA_VEC_ITER iter = _reloc_table.begin(); iter!=_reloc_table.end(); iter++){
        BBL_RELA &rela = *iter;
        //last relocation is reduced by optimzation
        if(rela.r_byte_pos>=gen_size){
            ASSERT(rela.r_byte_pos==(gen_size+1));//JMP_REL32
            break;
        }
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
                    INT32 new_disp32 = (INT32)(rela.r_addend - virtual_ss_offset);
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

RandomBBL *RandomBBL::read_rbbl(S_ADDRX r_addrx, SIZE &used_size)
{
    UINT32 *ptr_32 = NULL;
    UINT8 *ptr_8 = NULL;
    UINT16 *ptr_16 = NULL;
    BBL_RELA *ptr_rela = NULL;
    //1. read bbl start
    ptr_32 = (UINT32*)r_addrx;
    F_SIZE origin_bbl_start = (F_SIZE)*ptr_32++;
    //2. read bbl end
    F_SIZE origin_bbl_end = (F_SIZE)*ptr_32++;
    //3. read prefix    
    ptr_8 = (UINT8*)ptr_32;
    BOOL has_lock_and_repeat_prefix = *ptr_8++;
    //4. read fallthrough    
    BOOL has_fallthrough_bbl = *ptr_8++;
    //5. read relocation information
     //5.1 read table size
    ptr_16 = (UINT16*)ptr_8;
    SIZE table_size = *ptr_16++;
    BBL_RELA_VEC reloc_vec;
     //5.2 read reloction
    ptr_rela = (BBL_RELA*)ptr_16;
    for(SIZE index = 0; index<table_size; index++)
        reloc_vec.push_back(*ptr_rela++);
    //6. read template information
     //6.1 read template size
    ptr_16 = (UINT16*)ptr_rela;
    SIZE template_len = (SIZE)*ptr_16++;
     //6.2 read template byte
    std::string random_template = std::string((const INT8 *)ptr_16, template_len);
    //7. set used size
    used_size = (S_ADDRX)ptr_16 + template_len - r_addrx;
    
    return new RandomBBL(origin_bbl_start, origin_bbl_end, has_lock_and_repeat_prefix, has_fallthrough_bbl, reloc_vec, random_template);
}

SIZE RandomBBL::store_rbbl(S_ADDRX s_addrx)
{
    UINT32 *ptr_32 = NULL;
    UINT8 *ptr_8 = NULL;
    UINT16 *ptr_16 = NULL;
    BBL_RELA *ptr_rela = NULL;
    //1. store bbl start
    ptr_32 = (UINT32*)s_addrx;
    *ptr_32++ = (UINT32)_origin_bbl_start;
    //2. store bbl end
    *ptr_32++ = (UINT32)_origin_bbl_end;
    //3. store prefix    
    ptr_8 = (UINT8*)ptr_32;
    *ptr_8++ = (UINT8)_has_lock_and_repeat_prefix;
    //4. store fallthrough    
    *ptr_8++ = (UINT8)_has_fallthrough_bbl;
    //5. store relocation information
    //5.1 store table size
    SIZE table_size = _reloc_table.size();
    ASSERT(table_size<USHRT_MAX);
    ptr_16 = (UINT16*)ptr_8;
    *ptr_16++ = (UINT16)table_size;
    //5.2 store reloction
    ptr_rela = (BBL_RELA*)ptr_16;
    for(BBL_RELA_VEC::iterator iter = _reloc_table.begin(); iter!=_reloc_table.end(); iter++)
        *ptr_rela++ = *iter;
    //6. store template information
    //6.1 store template size
    SIZE template_len = _random_template.length();
    ASSERT(template_len<USHRT_MAX);
    ptr_16 = (UINT16*)ptr_rela;
    *ptr_16++ = (UINT16)template_len;
    //6.2 store template byte
    _random_template.copy((INT8 *)ptr_16, template_len);
    
    return (S_ADDRX)ptr_16 + template_len - s_addrx;
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


