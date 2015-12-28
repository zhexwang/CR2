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
public:
	enum INST_TYPE{
		INDIRECT_CALL = 0,
		INDIRECT_JUMP,
		RET,
		TYPE_SUM,
	};
	static const std::string type_name[TYPE_SUM];
	static const std::string unmatched_ss_name;
	static const std::string unaligned_ret_name;
    typedef struct inst_pos{
        F_SIZE instr_offset;
        INT32 image_index;
    }INST_POS;
	typedef std::multimap<INST_POS, INST_POS> INDIRECT_BRANCH_INFO;
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
    INDIRECT_BRANCH_INFO _indirect_call_maps;
    INDIRECT_BRANCH_INFO _indirect_jump_maps;
    INDIRECT_BRANCH_INFO _ret_maps;
	INDIRECT_BRANCH_INFO _unmatched_ret;//shadow stack
	INDIRECT_BRANCH_INFO _unaligned_ret;//rsp is not 8byte aligned
protected:
    void read_image_info(std::ifstream &ifs);
    void read_indirect_branch_info(std::ifstream &ifs, std::multimap<INST_POS, INST_POS> &maps, INT32 instr_num);
public:
    PinProfile(const char *profile_path);
    ~PinProfile();
	//judge functions
	/*  Arguments: none
		Return : none;
		Introduction: bbl is safe only if all indirect branchs' targets are all bbl(splitted in modules) entry.
	*/
	void check_bbl_safe() const;
	void check_func_safe() const;
	//get functions
	INT32 get_img_index_by_name(std::string) const;
    void map_modules();
    void dump_profile_image_info() const;
};

