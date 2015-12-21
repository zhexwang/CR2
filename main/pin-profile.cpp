#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "pin-profile.h"
#include "module.h"
#include "basic-block.h"
using namespace std;

const string PinProfile::type_name[PinProfile::TYPE_SUM] = {"IndirectCall", "IndirectJump", "Ret"};

PinProfile::PinProfile(const char *path) : _path(string(path))
{
    //1. init ifstream
    ifstream ifs(_path.c_str(), ifstream::in);
    //2. read image info
    read_image_info(ifs);
    map_modules();
    //3. read indirect call info
    read_indirect_branch_info(ifs, _indirect_call_maps, INDIRECT_CALL);
    //4. read indirect jump info
    read_indirect_branch_info(ifs, _indirect_jump_maps, INDIRECT_JUMP);
    //5. read ret info
    read_indirect_branch_info(ifs, _ret_maps, RET);
    //6. read shadow stack unmatched info
    read_ss_unmatched_info(ifs);
}

PinProfile::~PinProfile()
{
    delete []_module_maps;
    delete []_img_name;
    delete []_img_branch_targets;
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
        ASSERT(index==idx);
        ASSERT(image_path.find_last_of("/") == string::npos);
        _img_name[idx] = image_path;
    }
}

BOOL operator< (const PinProfile::INST_POS left, const PinProfile::INST_POS right)
{
    if((left.instr_offset<right.instr_offset)&&(left.image_index<right.image_index))
        return true;
    else 
        return false;
}

void PinProfile::read_indirect_branch_info(ifstream &ifs, multimap<INST_POS, INST_POS> &maps, INST_TYPE type)
{
    string padding;
    UINT8 c;
    INT32 instr_num = 0;
    ifs>>hex>>padding>>instr_num;
    ASSERTM(padding.find(PinProfile::type_name[type])!=string::npos, "type name unmatched!\n");        
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

void PinProfile::read_ss_unmatched_info(std::ifstream &ifs)
{
    string padding;
    UINT8 c;
    INT32 instr_num = 0;
    ifs>>hex>>padding>>instr_num;
    for(INT32 idx=0; idx<instr_num; idx++){
        //record ret information
        F_SIZE src_offset, target_offset;
        INT32 src_image_index, target_image_index;
        ifs>>hex>>src_offset>>c>>src_image_index>>padding>>target_offset>>c>>target_image_index>>c;
        INST_POS src = {src_offset, src_image_index};
        INST_POS target = {target_offset, target_image_index};
        _unmatched_ret.insert(make_pair(src, target));
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

void PinProfile::map_modules()
{
    Module::MODULE_MAP_ITERATOR it = Module::_all_module_maps.begin();
    for(; it!=Module::_all_module_maps.end(); it++){
        // get real path
        string str_path = get_real_path(it->second->get_path().c_str());
        // get name
        UINT32 found = str_path.find_last_of("/");
        string name;
        if(found==string::npos)
            name = str_path;
        else
            name = str_path.substr(found+1);
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
            
            BOOL is_bbl_entry = module->is_bbl_entry_in_off(target_offset);
            if(!is_bbl_entry){
                //test prefix
                BasicBlock *bb = module->get_bbl_by_off(target_offset-1);
                is_bbl_entry = bb&&bb->has_prefix() ? true : false;
            }
            if(!is_bbl_entry){
                Instruction *instr = module->find_instr_by_off(target_offset);
                BasicBlock *bbl = module->find_bbl_by_instr(instr);
                bbl->dump_in_off();
                FATAL(!is_bbl_entry, "check one indirect branch target (%s:0x%lx) is not bbl entry!\n", \
                    _module_maps[idx]->get_path().c_str(), target_offset);
            }
        }
    }    
}

void PinProfile::check_func_safe() const
{
    NOT_IMPLEMENTED(wangzhe);
}
