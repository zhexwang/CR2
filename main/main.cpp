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
        // 1.init netlink and get protected process's information
        NetLink::connect_with_lkm();
        PID proc_id = 0;
        P_ADDRX curr_pc = 0;
        SIZE cc_offset = 0, ss_offset = 0;
        NetLink::recv_init_mesg(proc_id, curr_pc, cc_offset, ss_offset);
        // 2.generate the first code variant
        CodeVariantManager::init_protected_proc_info(proc_id, cc_offset, ss_offset);
        CodeVariantManager::start_gen_code_variants();
        CodeVariantManager::wait_for_code_variant_ready(true);
        P_ADDRX new_pc = CodeVariantManager::find_cc_paddrx_from_all_orig(curr_pc, true);
        ASSERT(new_pc!=0);
        // 3.send message to switch to the new generated code variant
        NetLink::send_cv_ready_mesg(true, new_pc);
        // 4.loop to listen for rereandomization and exit
        BOOL need_cv1 = false;
        while(NetLink::recv_cv_request_mesg(curr_pc, need_cv1)){
            CodeVariantManager::wait_for_code_variant_ready(need_cv1);
            new_pc = CodeVariantManager::get_new_pc_from_old_all(curr_pc, need_cv1);
            ASSERT(new_pc!=0);
            CodeVariantManager::modify_new_ra_in_ss(need_cv1);
            NetLink::send_cv_ready_mesg(need_cv1, new_pc);
            CodeVariantManager::consume_cv(need_cv1 ? false : true);
        };
        // 5.stop gen code variants
        CodeVariantManager::stop_gen_code_variants();
        // 6.disconnect
        NetLink::disconnect_with_lkm();
    }
    
    
    return 0;
}
