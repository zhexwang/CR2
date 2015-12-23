#include "function.h"
#include "basic-block.h"
#include "module.h"

const std::string Function::func_type_name[Function::FUNC_TYPE_NUM] = 
{
    "CALL_TARGET", "PROLOG_MATCH", "SYM_RECORD",
};

Function::Function(const std::map<F_SIZE, BasicBlock*> &bbl_maps, const Function::FUNC_TYPE type)
    :_bbl_maps(bbl_maps), _type(type)
{
    _start = bbl_maps.begin()->first;
    _size = bbl_maps.rbegin()->second->get_bbl_end_offset() - _start;
    _entry_bbl_set.insert(bbl_maps.begin()->second);
}

Function::~Function()
{
    ;
}

void Function::add_new_entry(BasicBlock * bbl)
{
    ASSERT(_entry_bbl_set.find(bbl)==_entry_bbl_set.end());
    _entry_bbl_set.insert(bbl);
}

const Module *Function::get_module() 
{
    ASSERT(_entry_bbl_set.size()!=0); 
    return (*(_entry_bbl_set.begin()))->get_module();
}


void Function::dump_in_off()
{
    std::string name;
    switch(_type){
        case CALL_TARGET:
            name = "call_target";
            break;
        case PROLOG_MATCH:
            name = "prolog_match";
            break;
        case SYM_RECORD:
            name = get_module()->get_sym_func_name(_start);
            break;
        default:
            ASSERT(0);
            
    }
    BLUE("[%8lx, %8lx) %15s %s<Path:%s>\n", _start, _start+_size, func_type_name[_type].c_str(), \
        name.c_str(), get_module()->get_path().c_str());
}

