#pragma once

#include <elf.h>
#include <iostream>
#include <vector>
#include <map>

#include "type.h"
#include "utility.h"

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
	BOOL is_in_x_section_va(const S_ADDRX addr) const
	{
		SECTION_ITERATOR it = _x_sections.begin();
		for(;it!=_x_sections.end();it++){
			S_ADDRX region_start = (*it).start + _map_start;
			S_ADDRX region_end = (*it).end + _map_start;
			if((addr>=region_start)&&(addr<region_end))
				return true;
		}
		return false;
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
	INT32 read_4byte_data_in_off(F_SIZE offset) const
	{
		ASSERT(offset<_elf_size);
		return *(INT32*)(offset+_map_start);
	}
	//dump functions
	void dump_x_sections() const;
	void dump_dependence() const;
	static void dump_all_parsed_elfs();
	void dump_dynsymt() const;
	void dump_symt() const;
	void dump_rela_dyn() const;
	void dump_rela_plt() const;
};

