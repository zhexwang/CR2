#include "pin-profile.h"
using namespace std;

PinProfile::PinProfile(const char *path) : _path(string(path))
{
    //1. init ifstream
    ifstream ifs(_path.c_str(), ifstream::in);
    //2. read image info
    read_image_info(ifs);
    //3. read indirect call info
    read_indirect_branch_info(ifs, _indirect_call_maps);
    //4. read indirect jump info
    read_indirect_branch_info(ifs, _indirect_jump_maps);
    //5. read ret info
    read_indirect_branch_info(ifs, _ret_maps);
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
    ifs>>image_path>>_img_num;
    //init image
    _img_name = new string[_img_num]();
    _img_branch_targets = new set<F_SIZE>[_img_num]();
    _module_maps = new Module*[_img_num]();
    //read image list
    for(INT32 idx=0; idx<_img_num; idx++){
            ifs>>index>>image_path;
            ASSERT(index==idx);
            UINT32 found = image_path.find_last_of("/");
            if(found==string::npos)
                _img_name[idx] = image_path;
            else
                _img_name[idx] = image_path.substr(found+1);
    }
}

void PinProfile::read_indirect_branch_info(ifstream &ifs, multimap<INST_POS, INST_POS> &maps)
{
    ;
}

void PinProfile::dump_profile_image_info() const
{
    for(INT32 idx = 0; idx<_img_num; idx++){
            PRINT("%3d %s\n", idx, _img_name[idx].c_str());
    }
}

void PinProfile::map_to_modules()
{
    MODULE_MAP_ITERATOR it = _all_module_maps.begin();
    for(; it!=_all_module_maps.end(); it++){
         it->second->analysis_indirect_jump_targets();
         char real_path[1024] = "\0";
         UINT32 ret = readlink(it->second->get_path().c_str(), real_path, 1024);
         ASSERTM(ret<=0, "readlink error!\n");
         string str_path(real_path);
         UINT32 found = str_path.find_last_of("/");
         if(found==string::npos)
    }
}
