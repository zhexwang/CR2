
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "elf-parser.h"

std::map<std::string, ElfParser*> ElfParser::_all_parsed_elfs;
UINT32 ElfParser::_magic;
UINT16 ElfParser::_machine;

ElfParser::ElfParser(const char *elf_path): _sym_table(NULL), _symt_num(0), _dynsym_table(NULL), \
    _dynsymt_num(0), _rela_dyn(NULL), _rela_dyn_num(0), _rela_plt(NULL), _rela_plt_num(0), \
    _str_table(NULL), _dynstr_table(NULL), _pt_load_base(0)
{
    _elf_path = std::string(elf_path);
    ASSERT(!is_parsed(_elf_path));
    // 1.map elf to virtual memory
	map_elf();
	ASSERT(is_x64_elf());
	// 2.judge is so
	Elf64_Ehdr *elf_header = (Elf64_Ehdr*)_map_start;
	Elf64_Half elf_type = elf_header->e_type;
	if(elf_type==ET_EXEC){
		_is_so = false;
	}else if(elf_type==ET_DYN){
		_is_so = true;
	}else
		ASSERTM(0, "unkown elf type %d\n", elf_type);
    // 3.read program header
    Elf64_Phdr *Phdr = (Elf64_Phdr*)(_map_start + elf_header->e_phoff);
    for(UINT16 idx=0; idx<elf_header->e_phnum; idx++){
        if((Phdr[idx].p_type==PT_LOAD)&&(Phdr[idx].p_offset==0)){
            _pt_load_base = Phdr[idx].p_vaddr;
        }
    }
	// 4.read sections
	Elf64_Shdr *SecHdr = (Elf64_Shdr *)(_map_start + elf_header->e_shoff);
	char *SecHdrStrTab = (char *)(_map_start + SecHdr[elf_header->e_shstrndx].sh_offset);
	//search each section to find symtable and string table
	for (INT32 idx=0; idx<elf_header->e_shnum; idx++) {
		Elf64_Shdr *currentSec = SecHdr + idx;
		char *secName = SecHdrStrTab + currentSec->sh_name;
		if(strcmp(secName, ".symtab")==0) {
			_sym_table = (Elf64_Sym *)(_map_start + currentSec->sh_offset);
			_symt_num = currentSec->sh_size/sizeof(Elf64_Sym);
		}else if(strcmp(secName, ".strtab")==0) 
			_str_table = (char*)(_map_start + currentSec->sh_offset);
		else if(strcmp(secName, ".dynsym")==0){
			_dynsym_table = (Elf64_Sym *)(_map_start + currentSec->sh_offset);
			_dynsymt_num = currentSec->sh_size/sizeof(Elf64_Sym);		
		}else if (strcmp(secName, ".dynstr")==0){
			_dynstr_table = (char*)(_map_start + currentSec->sh_offset);
		}else if(strcmp(secName, ".rela.dyn")==0){
			_rela_dyn= (Elf64_Rela *)(_map_start + currentSec->sh_offset); 
			_rela_dyn_num = currentSec->sh_size/sizeof(Elf64_Rela);
        }else if(strcmp(secName, ".rela.plt")==0){
			_rela_plt= (Elf64_Rela *)(_map_start + currentSec->sh_offset); 
			_rela_plt_num = currentSec->sh_size/sizeof(Elf64_Rela);
        }else if(BITS_ARE_SET(currentSec->sh_flags, SHF_EXECINSTR)){
            SECTION_REGION element = {currentSec->sh_offset, currentSec->sh_offset+currentSec->sh_size, \
                std::string(secName)};
            _x_sections.push_back(element);
        }
    }
    // 5.add parser
    add_elf_parser(this);
    // 6.parse dependence libraries
    parse_dependence_lib();
    // 7.check if we can disassemble binary safely
    check_relocation();
}

void ElfParser::map_elf()
{
    //1.open elf file
    _elf_fd = open(_elf_path.c_str(), O_RDONLY);
    PERROR(_elf_fd!=-1, "open elf failed!");
    //2.calculate elf size
    struct stat statbuf;
    INT32 ret = fstat(_elf_fd, &statbuf);
    PERROR(ret==0, "fstat failed!");
    _elf_size = statbuf.st_size;
    //3.mmap elf
    void *map_ret = mmap(NULL, _elf_size, PROT_READ, MAP_PRIVATE, _elf_fd, 0);
    PERROR(map_ret!=MAP_FAILED, "mmap failed!");
    _map_start = (S_ADDRX)map_ret;
}

void ElfParser::parse_dependence_lib()
{
    // get the path of all dependence libraries
    // 1.generate command string
    char command[128];
    sprintf(command, "/usr/bin/ldd %s", _elf_path.c_str());
    char type[3] = "r";
    // 2.get stream of command's output
    FILE *p_stream = popen(command, type);
    ASSERTM(p_stream, "popen() error!\n");
    // 4.parse the stream to get dependence libraries
    SIZE len = 500;
    char *line_buf = new char[len];
    while(getline(&line_buf, &len, p_stream)!=-1) {  
        char name[100];
        char path[100];
        char *pos = NULL;
        P_ADDRX addr;
        // get name and path of images
        if((pos = strstr(line_buf, "ld-linux-x86-64"))!=NULL){
            sscanf(line_buf, "%s (0x%lx)\n", path, &addr);
            sscanf(pos, "%s (0x%lx)", name, &addr);
        }else if((pos = strstr(line_buf, "linux-vdso"))!=NULL){
            sscanf(line_buf, "%s => (0x%lx)\n", name, &addr);
            path[0] = '\0';
        }else if((pos = strstr(line_buf, "statically"))!=NULL){
            path[0] = '\0';
            name[0] = '\0';
        }else{    
            sscanf(line_buf, "%s => %s (0x%lx)\n", name, path, &addr);
            ASSERTM(strstr(path, name), "Name(%s) is not in Path(%s)\n", name, path);
        }
        //record parsed elf
        if((path[0]!='\0')){
            ElfParser *elf = NULL;
            if(is_parsed(path))
                elf = get_elf_parser(path);
            else 
                elf = new ElfParser(path);

            _dependence_elfs.push_back(elf);
        }
    }
    free(line_buf);
    // 5.close stream
    pclose(p_stream);
}

ElfParser::~ElfParser()
{
	INT32 ret = close(_elf_fd);
	PERROR(ret==0, "close failed!");
	ret = munmap((void*)_map_start, _elf_size);
	PERROR(ret==0, "munmap failed!");
}

ElfParser *ElfParser::parse_elf(const char *elf_path)
{
    return new ElfParser(elf_path);
}

void ElfParser::check_relocation() const
{
    // 1.check .rela.plt
    for(INT32 idx = 0; idx<_rela_plt_num; idx++){
        F_SIZE off = _rela_plt[idx].r_offset;
        FATAL(is_in_x_section_file(off), ".rela.plt is in x setions!\n");
    }
    // 2.check .rela.dyn
    for(INT32 idx = 0; idx<_rela_dyn_num; idx++){
        F_SIZE off = _rela_dyn[idx].r_offset;
        FATAL(is_in_x_section_file(off), ".rela.dyn is in x sections!\n");
    }
}

void ElfParser::find_function_from_sym_table(const Elf64_Sym *sym_table, const INT32 sym_num, \
    const char *str, FUNC_INFO_MAP &func_info_map)
{
    F_SIZE plt_start, plt_end;
    get_plt_range(plt_start, plt_end);
    
    for(INT32 idx = 0; idx<sym_num; idx++){
        Elf64_Sym sym = sym_table[idx];
        if(ELF64_ST_TYPE(sym.st_info)==STT_FUNC){
            if(sym.st_value==0)
                continue;
            
            F_SIZE func_start = _is_so ? sym.st_value : sym.st_value-_pt_load_base;
            F_SIZE func_end = sym.st_size + func_start;
            
            if(func_start==0 || (func_start>=plt_start && func_start<plt_end))
                continue;

            FUNC_INFO_MAP::iterator iter = func_info_map.find(func_start);
            if(iter==func_info_map.end()){
                FUNC_INFO info = {func_start, func_end, SYM_FUNC, std::string(str + sym.st_name)};    
                func_info_map.insert(std::make_pair(func_start, info));
            }else{
                FUNC_INFO &info = iter->second;                
                ASSERT(func_start==info.range_start);
                info.range_end = func_end;
                info.type = SYM_FUNC;
                info.func_name = std::string(str+sym.st_name);
            }
        }
    }

}

void ElfParser::search_function_from_sym_table(FUNC_INFO_MAP &func_info_map)
{
    // 1.scan dynamic symbol table
    find_function_from_sym_table(_dynsym_table, _dynsymt_num, _dynstr_table, func_info_map);
    // 2.scan symbol table
    find_function_from_sym_table(_sym_table, _symt_num, _str_table, func_info_map);
}

static const char *bind_name[] = {
    TO_STRING_INTERNAL(LOCAL),  /* Local symbol */
    TO_STRING_INTERNAL(GLOBAL), /* Global symbol */
    TO_STRING_INTERNAL(WEAK),   /* Weak symbol */    
    TO_STRING_INTERNAL(NUM),    /* Number of defined types.*/
};

static const char *type_name[] = {
    TO_STRING_INTERNAL(NOTYPE), /* Symbol type is unspecified */
    TO_STRING_INTERNAL(OBJECT), /* Symbol is a data object */
    TO_STRING_INTERNAL(FUNC),   /* Symbol is a code object */
    TO_STRING_INTERNAL(SECTION),/* Symbol associated with a section */
    TO_STRING_INTERNAL(FILE),   /* Symbol's name is file name */           
    TO_STRING_INTERNAL(COMMON), /* Symbol is a common data object */         
    TO_STRING_INTERNAL(TLS),    /* Symbol is thread-local data object*/
    TO_STRING_INTERNAL(NUM),    /* Number of defined types.*/            
};

static const char *vis_name[] = {
    TO_STRING_INTERNAL(DEFAULT),  /* Default symbol visibility rules */
    TO_STRING_INTERNAL(INTERNAL), /* Processor specific hidden class */
    TO_STRING_INTERNAL(HIDDEN),   /* Sym unavailable in other modules */   
    TO_STRING_INTERNAL(PROTECTED),/* Not preemptible, not exported */
};

static void get_section_index(const INT32 sec_idx, char *NdxBuf)
{
    switch(sec_idx){
        case SHN_UNDEF:  sprintf(NdxBuf, "UND"); break;
        case SHN_BEFORE: sprintf(NdxBuf, "BSO"); break; 
        case SHN_AFTER:  sprintf(NdxBuf, "ESO"); break; 
        case SHN_HIPROC: sprintf(NdxBuf, "EPR"); break; 
        case SHN_LOOS:   sprintf(NdxBuf, "SOS"); break; 
        case SHN_HIOS:   sprintf(NdxBuf, "EOS"); break; 
        case SHN_XINDEX: sprintf(NdxBuf, "EXT"); break;   
        case SHN_COMMON: sprintf(NdxBuf, "COM"); break;
        case SHN_ABS:    sprintf(NdxBuf, "ABS"); break;
        default:
            sprintf(NdxBuf, "%3d", sec_idx);
    }
}

static void dump_sym_table(const Elf64_Sym *sym_table, const INT32 sym_num, const char *str)
{
    PRINT("   Num:    Value          Size Type    Bind   Vis      Ndx Name\n");
    for(INT32 idx = 0; idx<sym_num; idx++){
        Elf64_Sym sym = sym_table[idx];
        //get ndx
        char NdxBuf[4];
        get_section_index(sym.st_shndx, NdxBuf);
        //print
        PRINT("%6d: %016lx %5d %-7s %-6s %-9s%3s %s\n", idx, sym.st_value, (INT32)sym.st_size, \
              type_name[ELF64_ST_TYPE(sym.st_info)], bind_name[ELF64_ST_BIND(sym.st_info)], \
              vis_name[ELF64_ST_VISIBILITY(sym.st_other)], NdxBuf, str + sym.st_name);
    }

}

void ElfParser::dump_dynsymt() const
{
    BLUE("====== Symbol table '.dynsym' contains %d entries ======\n", _dynsymt_num);
    dump_sym_table(_dynsym_table, _dynsymt_num, _dynstr_table);
}

void ElfParser::dump_symt() const
{
    BLUE("====== Symbol table '.symtab' contains %d entries ======\n", _symt_num);
    dump_sym_table(_sym_table, _symt_num, _str_table);
}

static const char *relocation_name[] = {
    TO_STRING_INTERNAL(R_X86_64_NONE),
    TO_STRING_INTERNAL(R_X86_64_64),
    TO_STRING_INTERNAL(R_X86_64_PC32),
    TO_STRING_INTERNAL(R_X86_64_GOT32),
    TO_STRING_INTERNAL(R_X86_64_PLT32),             
    TO_STRING_INTERNAL(R_X86_64_COPY),          
    TO_STRING_INTERNAL(R_X86_64_GLOB_DAT),
    TO_STRING_INTERNAL(R_X86_64_JUMP_SLOT),         
    TO_STRING_INTERNAL(R_X86_64_RELATIVE),          
    TO_STRING_INTERNAL(R_X86_64_GOTPCREL),          
    TO_STRING_INTERNAL(R_X86_64_32),      
    TO_STRING_INTERNAL(R_X86_64_32S),         
    TO_STRING_INTERNAL(R_X86_64_16),          
    TO_STRING_INTERNAL(R_X86_64_PC16),
    TO_STRING_INTERNAL(R_X86_64_8),
    TO_STRING_INTERNAL(R_X86_64_PC8),
    TO_STRING_INTERNAL(R_X86_64_DTPMOD64),
    TO_STRING_INTERNAL(R_X86_64_DTPOFF64),
    TO_STRING_INTERNAL(R_X86_64_TPOFF64),
    TO_STRING_INTERNAL(R_X86_64_TLSGD),
    TO_STRING_INTERNAL(R_X86_64_TLSLD),
    TO_STRING_INTERNAL(R_X86_64_DTPOFF32),
    TO_STRING_INTERNAL(R_X86_64_GOTTPOFF),
    TO_STRING_INTERNAL(R_X86_64_TPOFF32),
    TO_STRING_INTERNAL(R_X86_64_PC64),
    TO_STRING_INTERNAL(R_X86_64_GOTOFF64),
    TO_STRING_INTERNAL(R_X86_64_GOTPC32),
    TO_STRING_INTERNAL(R_X86_64_GOT64),
    TO_STRING_INTERNAL(R_X86_64_GOTPCREL64),
    TO_STRING_INTERNAL(R_X86_64_GOTPC64),
    TO_STRING_INTERNAL(R_X86_64_GOTPLT64),
    TO_STRING_INTERNAL(R_X86_64_PLTOFF64),
    TO_STRING_INTERNAL(R_X86_64_SIZE32),
    TO_STRING_INTERNAL(R_X86_64_SIZE64),
    TO_STRING_INTERNAL(R_X86_64_GOTPC32_TLSDESC),
    TO_STRING_INTERNAL(R_X86_64_TLSDESC_CALL),
    TO_STRING_INTERNAL(R_X86_64_TLSDESC),
    TO_STRING_INTERNAL(R_X86_64_IRELATIVE),
    TO_STRING_INTERNAL(R_X86_64_NUM),
};

static void dump_rela(const Elf64_Rela *rela_array, const INT32 rela_num, \
        const char *dynstr, const Elf64_Sym *dynsym_table)
{
    PRINT("  Offset          Info           Type           Sym. Value    Sym. Name + Addend\n");
    for(INT32 idx = 0; idx<rela_num; idx++){
        Elf64_Rela rela = rela_array[idx];
        INT32 sym_idx =  ELF64_R_SYM(rela.r_info);
        INT32 type_idx = ELF64_R_TYPE(rela.r_info);
        Elf64_Sym sym = dynsym_table[sym_idx];
        PRINT("%012lx  %012lx %-18s %016lx %s + %lx\n", rela.r_offset, rela.r_info, \
            relocation_name[type_idx], sym.st_value, dynstr + sym.st_name, rela.r_addend);
    }

}
void ElfParser::dump_rela_dyn() const
{
    BLUE("====== Relocation section '.rela.dyn' contains %d entries ======\n", _rela_dyn_num);
    dump_rela(_rela_dyn, _rela_dyn_num, _dynstr_table, _dynsym_table);
}
void ElfParser::dump_rela_plt() const
{
    BLUE("====== Relocation section '.rela.plt' contains %d entries ======\n", _rela_plt_num);
    dump_rela(_rela_plt, _rela_plt_num, _dynstr_table, _dynsym_table);
}

void ElfParser::dump_dependence() const
{
    BLUE("====== %s's dependence libraries %d (%s) ======\n", get_elf_name().c_str(), (INT32)_dependence_elfs.size(), _elf_path.c_str());
    DEPENDENCE_ELF_ITERATOR it = _dependence_elfs.begin();
    for(;it!=_dependence_elfs.end();it++){
        PRINT("%-25s ==> %s\n", (*it)->get_elf_name().c_str(), (*it)->get_elf_path().c_str());
    }
}

void ElfParser::dump_all_parsed_elfs()
{
    PARSED_ELF_ITERATOR it = _all_parsed_elfs.begin();
    BLUE("====== Dump all parsed elfs (%d) ======\n", (INT32)_all_parsed_elfs.size());
    for(;it!=_all_parsed_elfs.end();it++){
        PRINT("%-25s ==> %s\n", it->second->get_elf_name().c_str(), it->second->get_elf_path().c_str());
    }
}

void ElfParser::dump_x_sections() const
{
    BLUE("====== Dump %s's executable sections (%d) ======\n", get_elf_name().c_str(), (INT32)_x_sections.size());
    PRINT("  Start       End     Name\n");
    SECTION_ITERATOR it = _x_sections.begin();
    for(;it!=_x_sections.end();it++){
        PRINT("%08lx - %08lx  %s\n", (*it).start, (*it).end, (*it).section_name.c_str());
    }
}

