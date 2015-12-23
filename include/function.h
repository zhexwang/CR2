#pragma once
#include <set>
#include <map>
#include <string>

#include "type.h"
#include "utility.h"


class BasicBlock;
class Module;

class Function
{
public:
	enum FUNC_TYPE{
		CALL_TARGET = 0,
		PROLOG_MATCH,
		SYM_RECORD,
		FUNC_TYPE_NUM,
	};
protected:	
	static const std::string func_type_name[FUNC_TYPE_NUM];
	F_SIZE _start;
	SIZE _size;
	const std::map<F_SIZE, BasicBlock*> _bbl_maps;
	std::set<BasicBlock*> _entry_bbl_set;
	const FUNC_TYPE _type;
public:
	Function(const std::map<F_SIZE, BasicBlock*> &bbl_maps, const Function::FUNC_TYPE type);
	~Function();
	void add_new_entry(BasicBlock *bbl);
	//get functions
	F_SIZE get_func_offset() const {return _start;}
	F_SIZE get_func_size() const {return _size;}
	P_ADDRX get_func_paddr(P_ADDRX load_base) const {return _start + load_base;}
	const Module *get_module();
	static std::string get_func_type(Function::FUNC_TYPE type){return func_type_name[type];}
	//dump functions
	void dump_in_off();
};
