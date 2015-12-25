#pragma once

#include <elf.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>

#include "type.h"
#include "utility.h"
 
//typdef function
typedef struct func_info{
	F_SIZE range_start;
	F_SIZE range_end;
	std::string func_name;//if has name in symbol table
}FUNC_INFO;
typedef std::map<F_SIZE, FUNC_INFO> SYM_FUNC_INFO_MAP;

/* sizeof(plt_item) = 16
ff 25 7a 5b 29 00	   jmpq   *0x295b7a(%rip)		 
68 00 00 00 00		   pushq  $0x0
e9 e0 ff ff ff		   jmpq   402470 <_init+0x20>	
*/	
typedef struct plt_item{
	F_SIZE plt_start;
	F_SIZE plt_end;
	std::string plt_name;	
}PLT_ITEM;
typedef std::map<F_SIZE, PLT_ITEM> PLT_INFO_MAP;

typedef std::set<F_SIZE> RELA_X_TARGETS;
class ElfParser
{
public:
	//typedef
	typedef struct{
		F_SIZE start;
		F_SIZE end;
		std::string section_name;
	}SECTION_REGION;
	typedef std::vector<SECTION_REGION>::const_iterator SECTION_ITERATOR;
	typedef std::vector<ElfParser*>::const_iterator DEPENDENCE_ELF_ITERATOR;
	typedef std::map<std::string, ElfParser*>::const_iterator PARSED_ELF_ITERATOR;
	// static values, mapping table (elf_name ==> ElfParser*)
	static std::map<std::string, ElfParser*> _all_parsed_elfs;
private:
	INT32 _elf_fd;
	std::string _elf_path;
	F_SIZE _elf_size;
	S_ADDRX _map_start;
	BOOL _is_so;
	//section information
	// 1.symbol
	Elf64_Sym *_sym_table;
	INT32 _symt_num;
	// 2.dynamic symbol
	Elf64_Sym *_dynsym_table;
	INT32 _dynsymt_num;
	// 3.relocation
	Elf64_Rela *_rela_dyn;
	INT32 _rela_dyn_num;
	Elf64_Rela *_rela_plt;
	INT32 _rela_plt_num;
	// 4.string
	char *_str_table;
	char *_dynstr_table;
	// 5.X sections
	std::vector<SECTION_REGION> _x_sections;
	// 6.load
	P_ADDRX _pt_load_base;
	// 7.dependence elf
	std::vector<ElfParser*> _dependence_elfs;
	//legal args
	static UINT32 _magic;
	static UINT16 _machine;
protected:
	BOOL is_x64_elf()
	{
		return (*(UINT32 *)_map_start==_magic)&&(((Elf64_Ehdr*)_map_start)->e_machine==_machine);
	}
	void map_elf();
	void parse_dependence_lib();
	static void add_elf_parser(ElfParser *elf)
	{
		if(!is_parsed(elf->get_elf_name()))
			_all_parsed_elfs.insert(make_pair(elf->get_elf_name(), elf));
	}
	void find_function_from_sym_table(const Elf64_Sym *sym_table, const INT32 sym_num,\
		const char *str, SYM_FUNC_INFO_MAP &func_info_map);
public:
	ElfParser(const char *elf_path);
	~ElfParser();
	static void init()
	{
		_magic = 0x464c457f;
		_machine = EM_X86_64;
	}
	//parse elf
	static ElfParser *parse_elf(const char *elf_path);
	//judge functions
	static BOOL is_parsed(const std::string elf_path)
	{
		PARSED_ELF_ITERATOR it = _all_parsed_elfs.find(get_name(elf_path));
		return it!=_all_parsed_elfs.end();
	}
	BOOL is_shared_object() const
	{
		return _is_so;
	}
	void get_plt_range(F_SIZE &start, F_SIZE &end) const
	{
		SECTION_ITERATOR it = _x_sections.begin();
		for(;it!=_x_sections.end();it++){
			const SECTION_REGION &sr = *it;
			if(sr.section_name.find(".plt")!=std::string::npos){
				start = sr.start;
				end = sr.end;
				return ;
			}
		}
		start = 0;
		end = 0;
		return ;
	}
	BOOL is_in_x_section_file(const F_SIZE off) const
	{
		SECTION_ITERATOR it = _x_sections.begin();
		for(;it!=_x_sections.end();it++){
			if((off>=(*it).start)&&(off<(*it).end))
				return true;
		}
		return false;
	}
	//get functions
	const std::string get_elf_path() const
	{
		return _elf_path;
	}
	const std::string get_elf_name() const
	{
		return get_name(_elf_path);
	}
	SIZE get_x_section_num() const
	{
		return _x_sections.size();
	}
	void get_x_region(const UINT32 idx, S_ADDRX &region_start, S_ADDRX &region_end) const
	{
		ASSERTM(idx<(UINT32)_x_sections.size(), \
			"Executable section num (%d) is overflow (%d)!\n", idx, (INT32)_x_sections.size());
		region_start = _x_sections[idx].start + _map_start;
		region_end   = _x_sections[idx].end + _map_start;
	}
	static std::string get_name(const std::string &path)
	{
		return path.substr(path.find_last_of('/') + 1);
	}
	static ElfParser* get_elf_parser(const std::string elf_path)
	{
		ASSERTM(is_parsed(elf_path), "%s is not parsed!\n", elf_path.c_str());
		PARSED_ELF_ITERATOR it = _all_parsed_elfs.find(get_name(elf_path));
		return it->second;
	}
	P_ADDRX get_pt_load_base() const
	{
		return _pt_load_base;
	}
	UINT8 *get_code_offset_ptr(const F_SIZE off) const
	{
		ASSERTM(is_in_x_section_file(off), "%lx should in executable sections\n", off);
		return (UINT8 *)(_map_start + off);
	}
	/*  @Return: return false if relocation reflect the disassmbler, otherwise return true;
		@Arguments: none;   
		@Introduction: main executable maybe relocate instructions at load-time, we disassemble the binary offline.
					We must handle this carefully! Now we do not handle it and only report the check result! 
	 */
	void check_relocation() const;
	//read bytes
	UINT8 read_1byte_code_in_off(F_SIZE offset) const
	{
		ASSERT(is_in_x_section_file(offset));
		return *(UINT8*)(offset+_map_start);
	}
	INT16 read_2byte_data_in_off(F_SIZE offset) const
	{
		ASSERT(offset<_elf_size);
		return *(INT16*)(offset+_map_start);
	}
	INT32 read_4byte_data_in_off(F_SIZE offset) const
	{
		ASSERT(offset<_elf_size);
		return *(INT32*)(offset+_map_start);
	}
	INT64 read_8byte_data_in_off(F_SIZE offset) const
	{
		ASSERT(offset<_elf_size);
		return *(INT64*)(offset+_map_start);
	}
	void search_plt_info(PLT_INFO_MAP &plt_map);
	void search_function_from_sym_table(SYM_FUNC_INFO_MAP &func_info_map);
	void search_rela_x_section(RELA_X_TARGETS &rela_targets);
	//dump functions
	void dump_x_sections() const;
	void dump_dependence() const;
	static void dump_all_parsed_elfs();
	void dump_dynsymt() const;
	void dump_symt() const;
	void dump_rela_dyn() const;
	void dump_rela_plt() const;
};

