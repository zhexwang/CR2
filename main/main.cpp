#include "elf-parser.h"
#include "disassembler.h"
#include "module.h"

int main(int argc, const char **argv)
{
    ElfParser::init();
    ElfParser::parse_elf(argv[1]);
    Disassembler::init();
    Disassembler::disassemble_all_modules();
    Module::split_all_modules_into_bbls();

    return 0;
}
