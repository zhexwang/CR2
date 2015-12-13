#pragma once

#include <set>
#include <map>
#include <string>
#include <fstream>

#include "type.h"
#include "utility.h"
#include "utility.h"

class Module;
class PinProfile
{
private:
    //record path
    const std::string _path;
    //images
    std::string *_img_name;
    INT32 _img_num;
    //map image index to module
    Module **_module_maps;
    //record image branch targets
    std::set<F_SIZE> *_img_branch_targets;
    //record indirect branch instructions
    typedef struct{
        F_SIZE inst_offset;
        INT32 image_index;
    }INST_POS;
    std::multimap<INST_POS, INST_POS> _indirect_call_maps;
    std::multimap<INST_POS, INST_POS> _indirect_jump_maps;
    std::multimap<INST_POS, INST_POS> _ret_maps;
protected:
    void read_image_info(std::ifstream &ifs);
    void read_indirect_branch_info(std::ifstream &ifs, std::multimap<INST_POS, INST_POS> &maps);
public:
    PinProfile(const char *profile_path);
    ~PinProfile();
    void map_modules();
    void dump_profile_image_info() const;
};

