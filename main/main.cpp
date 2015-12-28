#include "elf-parser.h"
#include "disassembler.h"
#include "module.h"
#include "pin-profile.h"
#include "option.h"

int main(int argc, const char **argv)
{
    Options::show_system();
    
    ElfParser::init();
    ElfParser::parse_elf(argv[1]);
    
    Disassembler::init();
    Disassembler::disassemble_all_modules();
    
    Module::split_all_modules_into_bbls();
    PinProfile *profile = new PinProfile(argv[2]);

    Module::dump_all_indirect_jump_result();
    profile->check_bbl_safe();
    
    Module::separate_movable_bbls_from_all_modules();
    Module::dump_all_bbl_movable_info();
    profile->check_func_safe();
    
    return 0;
}
