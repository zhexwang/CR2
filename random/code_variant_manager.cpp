#include "code_variant_manager.h"
#include "utility.h"

CodeVariantManager::CVM_MAPS _all_cvm_maps;
std::string CodeVariantManager::_code_variant_img_path;
SIZE CodeVariantManager::_cc_offset = 0;
SIZE CodeVariantManager::_ss_offset = 0;
pid_t CodeVariantManager::_protected_proc_pid = 0;

CodeVariantManager::CodeVariantManager(std::string module_path, SIZE cc_size)
    : _elf_path(module_path), _cc_size(cc_size)
{
    ;
}

CodeVariantManager::~CodeVariantManager()
{
    ;
}

