#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "pin-profile.h"
#include "module.h"
#include "basic-block.h"
using namespace std;

const string PinProfile::type_name[PinProfile::TYPE_SUM] = {"IndirectCall", "IndirectJump", "Ret"};
const string PinProfile::unmatched_ss_name = "ShadowStackUnmatched";
const string PinProfile::unaligned_ret_name = "UnalignedRet";

PinProfile::PinProfile(const char *path) : _path(string(path))
{
    //1. init ifstream
    ifstream ifs(_path.c_str(), ifstream::in);
    //2. read image info
    read_image_info(ifs);
    map_modules();
    string name;
    INT32 instr_num = 0;
    //3. read indirect call info
    ifs>>hex>>name>>instr_num;     
    ASSERTM(name.find(type_name[INDIRECT_CALL])!=string::npos, "type name unmatched!\n");   
    read_indirect_branch_info(ifs, _indirect_call_maps, instr_num);
    //4. read indirect jump info
    ifs>>hex>>name>>instr_num;     
    ASSERTM(name.find(type_name[INDIRECT_JUMP])!=string::npos, "type name unmatched!\n");   
    read_indirect_branch_info(ifs, _indirect_jump_maps, instr_num);
    //5. read ret info
    ifs>>hex>>name>>instr_num;     
    ASSERTM(name.find(type_name[RET])!=string::npos, "type name unmatched!\n");  
    read_indirect_branch_info(ifs, _ret_maps, instr_num);
    //6. read shadow stack unmatched info
    ifs>>hex>>name>>instr_num;     
    ASSERTM(name.find(unmatched_ss_name)!=string::npos, "type name unmatched!\n");  
    read_indirect_branch_info(ifs, _unmatched_ret, instr_num);
    //7. read rsp is not 8bytes aligned
    ifs>>hex>>name>>instr_num;     
    ASSERTM(name.find(unaligned_ret_name)!=string::npos, "type name unmatched!\n");  
    read_indirect_branch_info(ifs, _unaligned_ret, instr_num);
}

PinProfile::~PinProfile()
{
    delete []_module_maps;
    delete []_img_name;
    delete []_img_branch_targets;
}

static string get_real_path(const char *file_path)
{
    #define PATH_LEN 1024
    #define INCREASE_IDX ++idx;idx%=2
    #define OTHER_IDX (idx==0 ? 1 : 0)
    #define CURR_IDX idx
    
    char path[2][PATH_LEN];
    for(INT32 i=0; i<2; i++)
        memset(path[i], '\0', PATH_LEN);
    INT32 idx = 0;
    INT32 ret = 0;
    struct stat statbuf;
    //init
    strcpy(path[CURR_IDX], file_path);
    //loop to find real path
    while(1){
        ret = lstat(path[CURR_IDX], &statbuf);
        if(ret!=0)//lstat failed
            break;
        if(S_ISLNK(statbuf.st_mode)){
            ret = readlink(path[CURR_IDX], path[OTHER_IDX], PATH_LEN);
            PERROR(ret>0, "readlink error!\n");
            INCREASE_IDX;
        }else
            break;
    }
    
    return string(path[CURR_IDX]); 
}

static string get_real_name_from_path(string path)
{
    string real_path = get_real_path(path.c_str());
    UINT32 found = real_path.find_last_of("/");
    string name;
    if(found==string::npos)
        name = real_path;
    else
        name = real_path.substr(found+1);

    return name;
}

void PinProfile::read_image_info(ifstream &ifs)
{
    //read image title
    INT32 index = 0;
    string image_path;
    ifs>>hex>>image_path>>_img_num;
    //init image
    _img_name = new string[_img_num]();
    _img_branch_targets = new set<F_SIZE>[_img_num]();
    _module_maps = new Module*[_img_num]();
    //read image list
    for(INT32 idx=0; idx<_img_num; idx++){
        ifs>>hex>>index>>image_path;
        string real_name = get_real_name_from_path(image_path);
        ASSERT(index==idx);
        _img_name[idx] = real_name;
    }
}

BOOL operator< (const PinProfile::INST_POS left, const PinProfile::INST_POS right)
{
    if((left.instr_offset<right.instr_offset)&&(left.image_index<right.image_index))
        return true;
    else 
        return false;
}

void PinProfile::read_indirect_branch_info(ifstream &ifs, multimap<INST_POS, INST_POS> &maps, INT32 instr_num)
{
    string padding;
    UINT8 c;
    for(INT32 idx=0; idx<instr_num; idx++){
        //record branch information
        F_SIZE src_offset, target_offset;
        INT32 src_image_index, target_image_index;
        ifs>>hex>>src_offset>>c>>src_image_index>>padding>>target_offset>>c>>target_image_index>>c;
        INST_POS src = {src_offset, src_image_index};
        INST_POS target = {target_offset, target_image_index};
        maps.insert(make_pair(src, target));
        //insert branch targets
        _img_branch_targets[target_image_index].insert(target_offset);
    }
}

void PinProfile::dump_profile_image_info() const
{
    for(INT32 idx = 0; idx<_img_num; idx++){
        PRINT("%3d %s\n", idx, _img_name[idx].c_str());
    }
}

INT32 PinProfile::get_img_index_by_name(string name) const
{
    for(INT32 idx=0; idx<_img_num; idx++){
        if(_img_name[idx]==name)
            return idx;
    }
    return -1;
}

void PinProfile::map_modules()
{
    Module::MODULE_MAP_ITERATOR it = Module::_all_module_maps.begin();
    for(; it!=Module::_all_module_maps.end(); it++){
        // get real path
        string real_path = get_real_path(it->second->get_path().c_str());
        // get name
        string name = get_real_name_from_path(real_path);
        // match _all_modules to _module_maps**;    
        INT32 index = get_img_index_by_name(name);
        FATAL(index==-1, "map failed!\n");
        _module_maps[index] = it->second;
    }
}

void PinProfile::check_bbl_safe() const 
{
    for(INT32 idx = 1; idx<_img_num; idx++){
        Module *module = _module_maps[idx];
        set<F_SIZE>::const_iterator it = _img_branch_targets[idx].begin();
        for(;it!=_img_branch_targets[idx].end(); it++){
            F_SIZE target_offset = *it;
            BOOL is_bbl_entry = module->is_bbl_entry_in_off(target_offset, true);

            if(!is_bbl_entry){
                Instruction *instr = module->find_instr_by_off(target_offset, true);
                ASSERT(instr);
                BasicBlock *bbl = module->find_bbl_cover_instr(instr);
                bbl->dump_in_off();
                FATAL(!is_bbl_entry, "check one indirect branch target (%s:0x%lx) is not bbl entry!\n", \
                    _module_maps[idx]->get_path().c_str(), target_offset);
            }
        }
    }    
}

void PinProfile::check_func_safe() const
{
    //check indirect call
    for(INDIRECT_BRANCH_INFO::const_iterator iter = _indirect_call_maps.begin(); iter!=_indirect_call_maps.end(); iter++){
        INST_POS src = iter->first;
        Module *src_module = _module_maps[src.image_index];
        F_SIZE src_offset = src.instr_offset;
        INST_POS target = iter->second;
        Module *target_module = _module_maps[target.image_index];
        F_SIZE target_offset = target.instr_offset;
        BasicBlock *src_bbl = src_module->find_bbl_cover_offset(src_offset);
        BasicBlock *target_bbl = target_module->find_bbl_by_offset(target_offset, false);
        ASSERT(src_bbl && target_bbl);
        BOOL is_fixed  = target_module->is_fixed_bbl(target_bbl);
        if(!is_fixed){
            ERR("check one indirect call target bbl (%s:0x%lx) is not fixed!\n ", \
                target_module->get_path().c_str(), target_offset);
            ERR("SrcBBL:\n");
            src_bbl->dump_in_off();
            ERR("TargetBBL:\n");
            target_bbl->dump_in_off();
        }
    }
    //check indirect jump
    for(INDIRECT_BRANCH_INFO::const_iterator iter = _indirect_jump_maps.begin(); iter!=_indirect_jump_maps.end(); iter++){
        INST_POS src = iter->first;
        Module *src_module = _module_maps[src.image_index];
        F_SIZE src_offset = src.instr_offset;
        INST_POS target = iter->second;
        Module *target_module = _module_maps[target.image_index];
        F_SIZE target_offset = target.instr_offset;
        BasicBlock *src_bbl = src_module->find_bbl_cover_offset(src_offset);
        BasicBlock *target_bbl = target_module->find_bbl_by_offset(target_offset, false);
        ASSERT(src_bbl && target_bbl);
        
        if(src_module->is_in_plt_in_off(src_offset)){
            ASSERT(src_module->is_fixed_bbl(src_bbl));
            continue;
        }
        
        if(src_module==target_module){//we only recognize the indirect jump in curr modules
            Module::JUMPIN_MAP_ITER it =  src_module->_indirect_jump_maps.find(src_offset);
            ASSERT(it!=src_module->_indirect_jump_maps.end());
            Module::JUMPIN_INFO &info = it->second;
            if(info.type==Module::SWITCH_CASE_ABSOLUTE || info.type==Module::SWITCH_CASE_OFFSET || \
                info.type==Module::MEMSET_JMP){
                ASSERT(info.targets.find(target_offset)!=info.targets.end());
                continue;
            }
        }//left jumpin must be fixed
        BOOL is_fixed  = target_module->is_fixed_bbl(target_bbl);
        if(!is_fixed){
            ERR("check one indirect jump target (%s:0x%lx) is not fixed!\n", \
                target_module->get_path().c_str(), target_offset);
            ERR("SrcBBL:\n");
            src_bbl->dump_in_off();
            ERR("TargetBBL:\n");
            target_bbl->dump_in_off();
        }
    }
}
