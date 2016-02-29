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
        if(Options::_has_output_db_file){
            // 6. generate bbl template
            Module::init_cvm_from_modules();
            Module::generate_all_relocation_block(LKM_OFFSET_SS_TYPE);
            // 7. output static analysis dbs    
            CodeVariantManager::store_into_db(Options::_output_db_file_path);
        }
    }

    if(Options::_dynamic_shuffle){
        //init code variant manager
        if(!Options::_static_analysis){
            //read input relocation dbs
            CodeVariantManager::init_from_db(Options::_elf_path.c_str(), Options::_input_db_file_path, LKM_OFFSET_SS_TYPE);
        }else if(!Options::_has_output_db_file){
            // generate bbl template
            Module::init_cvm_from_modules();
            Module::generate_all_relocation_block(LKM_OFFSET_SS_TYPE);
        }
        // 1.init netlink and get protected process's information
        NetLink::connect_with_lkm(Options::_elf_path);
        MESG_BAG mesg = NetLink::recv_mesg();
        if(mesg.connect!=P_PROCESS_IS_IN){
            return -1;
        }
        // 2.generate the first code variant
        CodeVariantManager::init_protected_proc_info(mesg.proctected_procid, mesg.cc_offset, mesg.ss_offset, mesg.gs_base, mesg.lkm_ss_type);
        CodeVariantManager::start_gen_code_variants();
        CodeVariantManager::wait_for_code_variant_ready(true);
        P_ADDRX new_pc = CodeVariantManager::find_cc_paddrx_from_all_orig(mesg.new_ip, true);
        ASSERT(new_pc!=0);
        // 3.send message to switch to the new generated code variant
        NetLink::send_cv_ready_mesg(true, new_pc, Options::_elf_path);
        // 4.loop to listen for rereandomization and exit
        while(1){
            // block to recv message from kernel module
            MESG_BAG mesg = NetLink::recv_mesg();
            
            if(mesg.connect==P_PROCESS_IS_OUT)
                break;
            else if(mesg.connect==CURR_IS_CV1_NEED_CV2 || mesg.connect==CURR_IS_CV2_NEED_CV1){
                //handle rerandomization
                BOOL need_cv1 = mesg.connect==CURR_IS_CV2_NEED_CV1;
                CodeVariantManager::wait_for_code_variant_ready(need_cv1);
                new_pc = CodeVariantManager::get_new_pc_from_old_all(mesg.new_ip, need_cv1);
                ASSERT(new_pc!=0);
                CodeVariantManager::modify_new_ra_in_ss(need_cv1);
                NetLink::send_cv_ready_mesg(need_cv1, new_pc, Options::_elf_path);
                CodeVariantManager::consume_cv(need_cv1 ? false : true);
            }else if(mesg.connect==SIGACTION_DETECTED){
                //handle sigaction
                P_ADDRX sighandler_addr = mesg.cc_offset;
                P_ADDRX sigreturn_addr = mesg.ss_offset;
                //ERR("Register handler: %lx, sigreturn: %lx\n", sighandler_addr, sigreturn_addr);
                new_pc = CodeVariantManager::handle_sigaction(sighandler_addr, sigreturn_addr, mesg.new_ip);
                NetLink::send_sigaction_handled_mesg(new_pc, Options::_elf_path);
            }else
                ASSERTM(0, "Unkwon message type %d!\n", mesg.connect);
        };
        // 5.stop gen code variants
        CodeVariantManager::stop_gen_code_variants();
        // 6.recycle 
        CodeVariantManager::recycle();
        // 7.disconnect
        NetLink::disconnect_with_lkm(Options::_elf_path);
    }
    
    return 0;
}
