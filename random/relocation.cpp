#include "relocation.h"
#include "utility.h"

RandomBBL::RandomBBL(std::vector<BBL_RELA> reloc_info, std::string random_template)
    : _reloc_info(reloc_info), _random_template(random_template)
{
    ;
}

RandomBBL::~RandomBBL()
{
    ;
}

void RandomBBL::relocate(SIZE random_offset, SIZE cc_offset, SIZE ss_offset, S_ADDRX relocate_pos, SIZE relocate_size)
{
    NOT_IMPLEMENTED(wangzhe);
}

