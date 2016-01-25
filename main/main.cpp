#include "elf-parser.h"
#include "disassembler.h"
#include "module.h"
#include "pin-profile.h"
#include "option.h"
#include "code_variant_manager.h"
#include "netlink.h"

int main(int argc, char **argv)
{
    Options::parse(argc, argv);

    if(Options::_static_analysis){
        // 1. parse elf
        ElfParser::init();
        ElfParser::parse_elf(Options::_elf_path.c_str());
        // 2. disassemble all modules into instructions
        Disassembler::init();
        Disassembler::disassemble_all_modules();
        // 3. split bbls
        Module::split_all_modules_into_bbls();
        // 4. classified bbls
        Module::separate_movable_bbls_from_all_modules();
        // 5. check the static analysis's result
        if(Options::_need_check_static_analysis){
            PinProfile *profile = new PinProfile(Options::_check_file.c_str());
            profile->check_bbl_safe();
            profile->check_func_safe();
        }
        //Module::dump_all_indirect_jump_result();
        //Module::dump_all_bbl_movable_info();
        // 6. generate bbl template
        Module::init_cvm_from_modules();
        Module::generate_all_relocation_block();
        // 7. output static analysis dbs    
    }

    if(Options::_dynamic_shuffle){
        if(!Options::_static_analysis){
            //read input relocation dbs
            NOT_IMPLEMENTED(wangzhe);
        }
        NetLink::connect_with_lkm();
        PID proc_id = 0;
        S_ADDRX curr_pc = 0;
        SIZE cc_offset = 0, ss_offset = 0;
        NetLink::recv_mesg(proc_id, curr_pc, cc_offset, ss_offset);
        //generate code variant
        CodeVariantManager::init_protected_proc_info(proc_id, cc_offset, ss_offset);
        CodeVariantManager::generate_all_code_variant();
        MESG_BAG msg_content = {1, 0, (long)curr_pc, (long)cc_offset, (long)ss_offset, "Generate the code variant!"};
        NetLink::send_mesg(msg_content);
        NetLink::disconnect_with_lkm();
    }
    
    
    return 0;
}
