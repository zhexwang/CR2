#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include "code_variant_manager.h"
#include "instr_generator.h"

CodeVariantManager::CVM_MAPS CodeVariantManager::_all_cvm_maps;
std::string CodeVariantManager::_code_variant_img_path;
SIZE CodeVariantManager::_cc_offset = 0;
SIZE CodeVariantManager::_ss_offset = 0;
P_ADDRX CodeVariantManager::_org_stack_load_base = 0;
P_ADDRX CodeVariantManager::_ss_load_base = 0;
P_SIZE CodeVariantManager::_ss_load_size = 0;
std::string CodeVariantManager::_ss_shm_path;
INT32 CodeVariantManager::_ss_fd = -1;

static std::string get_real_path(const char *file_path)
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
    
    return std::string(path[CURR_IDX]); 
}

static std::string get_real_name_from_path(std::string path)
{
    std::string real_path = get_real_path(path.c_str());
    UINT32 found = real_path.find_last_of("/");
    std::string name;
    if(found==std::string::npos)
        name = real_path;
    else
        name = real_path.substr(found+1);

    return name;
}

CodeVariantManager::CodeVariantManager(std::string module_path)
{
    _elf_real_name = get_real_name_from_path(get_real_path(module_path.c_str()));
    add_cvm(this);
}

static INT32 map_shm_file(std::string shm_path, S_ADDRX &addr, F_SIZE &file_sz)
{
    //1.open shm file
    INT32 fd = shm_open(shm_path.c_str(), O_RDWR, 0644);
    ASSERTM(fd!=-1, "open shm file failed! %s", strerror(errno));
    //2.calculate file size
    struct stat statbuf;
    INT32 ret = fstat(fd, &statbuf);
    FATAL(ret!=0, "fstat failed %s!", strerror(errno));
    F_SIZE file_size = statbuf.st_size;
    //3.mmap file
    void *map_ret = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    PERROR(map_ret!=MAP_FAILED, "mmap failed!");

    //return 
    addr = (S_ADDRX)map_ret;
    file_sz = file_size;
    return fd;
}

static void close_shm_file(INT32 fd)
{
    close(fd);
}

CodeVariantManager::~CodeVariantManager()
{
    close_shm_file(_cc_fd);
    //close_shm_file(_ss_fd);
    munmap((void*)_cc1_base, _cc_load_size*2);
    //munmap((void*)_ss_base, _ss_load_size);
}

void CodeVariantManager::init_cc()
{
    // 1.map cc
    S_SIZE map_size = 0;
    _cc_fd = map_shm_file(_cc_shm_path, _cc1_base, map_size);
    ASSERT(map_size==(_cc_load_size*2));
    _cc2_base = _cc1_base+map_size/2;  
    // 2.set curr cc
    _curr_is_first_cc = true;
}

void CodeVariantManager::init_cc_and_ss()
{
    // map cc
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++)
        iter->second->init_cc();
    // map ss
    S_SIZE map_size;
    S_ADDRX map_start;
    _ss_fd = map_shm_file(_ss_shm_path, map_start, map_size);
    _ss_load_base = map_start + map_size;
    ASSERT(map_size==_ss_load_size);
}

typedef struct mapsRow{
	P_ADDRX start;
	P_ADDRX end;
	SIZE offset;
	char perms[8];
	char dev[8];
	INT32 inode;
	char pathname[128];
}MapsFileItem;

inline BOOL is_executable(const MapsFileItem *item_ptr)
{
	if(strstr(item_ptr->perms, "x"))
		return true;
	else
		return false;
}

inline BOOL is_shared(const MapsFileItem *item_ptr)
{
	if(strstr(item_ptr->perms, "s"))
		return true;
	else
		return false;
}

void CodeVariantManager::parse_proc_maps(PID protected_pid)
{
    //1.open maps file
    char maps_path[100];
    sprintf(maps_path, "/proc/%d/maps", protected_pid);
    FILE *maps_file = fopen(maps_path, "r"); 
    ASSERTM(maps_file!=NULL, "Open %s failed!\n", maps_path);
    //2.find strings
    char shared_prefix_str[256];
    sprintf(shared_prefix_str, "%d-", protected_pid);
    std::string shared_prefix = std::string(shared_prefix_str);
    std::string cc_sufix = ".cc";
    std::string ss_sufix = ".ss";
    //3.read maps file, line by line
    #define mapsRowMaxNum 100
    MapsFileItem mapsArray[mapsRowMaxNum] = {{0}};
    INT32 mapsRowNum = 0;
    char row_buffer[256];
    char *ret = fgets(row_buffer, 256, maps_file);
    FATAL(!ret, "fgets wrong!\n");
    while(!feof(maps_file)){
        MapsFileItem *currentRow = mapsArray+mapsRowNum;
        //read one row
        currentRow->pathname[0] = '\0';
        sscanf(row_buffer, "%lx-%lx %s %lx %s %d %s", &(currentRow->start), &(currentRow->end), \
            currentRow->perms, &(currentRow->offset), currentRow->dev, &(currentRow->inode), currentRow->pathname);
        //x and code cache
        if(is_executable(currentRow) && !strstr(currentRow->pathname, "[vdso]") && !strstr(currentRow->pathname, "[vsyscall]")){
            std::string maps_record_name = get_real_name_from_path(get_real_path(currentRow->pathname));
            if(is_shared(currentRow)){
                SIZE prefix_pos = maps_record_name.find(shared_prefix);
                ASSERT(prefix_pos!=std::string::npos);
                prefix_pos += shared_prefix.length();
                SIZE cc_pos = maps_record_name.find(cc_sufix);
                ASSERT(cc_pos!=std::string::npos);
                //find cvm
                CVM_MAPS::iterator iter = _all_cvm_maps.find(maps_record_name.substr(prefix_pos, cc_pos-prefix_pos));
                ASSERT(iter!=_all_cvm_maps.end());
                iter->second->set_cc_load_info(currentRow->start, currentRow->end-currentRow->start, maps_record_name);
            }else{
                CVM_MAPS::iterator iter = _all_cvm_maps.find(maps_record_name);
                ASSERT(iter!=_all_cvm_maps.end());
                iter->second->set_x_load_base(currentRow->start);
            }
        }
        //shadow stack and stack
        if(!is_executable(currentRow)){
            std::string maps_record_name = get_real_name_from_path(get_real_path(currentRow->pathname));
            if(is_shared(currentRow)){//shadow stack
                ASSERT(maps_record_name.find(ss_sufix)!=std::string::npos);
                set_ss_load_info(currentRow->end, currentRow->end-currentRow->start, maps_record_name);
            }else{
                if(strstr(currentRow->pathname, "[stack]"))//stack
                    set_stack_load_base(currentRow->end);
            }
        }
        
        //calculate the row number
        mapsRowNum++;
        ASSERT(mapsRowNum < mapsRowMaxNum);
        //read next row
        ret = fgets(row_buffer, 256, maps_file);
    }

    fclose(maps_file);
    return ;
}

#define JMP32_LEN 0x5
#define JMP8_LEN 0x2
#define OVERLAP_JMP32_LEN 0x4
#define JMP8_OPCODE 0xeb
#define JMP32_OPCODE 0xe9

inline CC_LAYOUT_PAIR place_invalid_boundary(S_ADDRX invalid_addr, CC_LAYOUT &cc_layout)
{
    std::string invalid_template = InstrGenerator::gen_invalid_instr();
    invalid_template.copy((char*)invalid_addr, invalid_template.length());
    return cc_layout.insert(std::make_pair(Range<S_ADDRX>(invalid_addr, invalid_addr+invalid_template.length()-1), BOUNDARY_PTR));
}

inline CC_LAYOUT_PAIR place_trampoline8(S_ADDRX tramp8_addr, INT8 offset8, CC_LAYOUT &cc_layout)
{
    //gen jmp rel8 instruction
    UINT16 pos = 0;
    std::string jmp_rel8_template = InstrGenerator::gen_jump_rel8_instr(pos, offset8);
    jmp_rel8_template.copy((char*)tramp8_addr, jmp_rel8_template.length());
    //insert trampoline8 into cc layout
    return cc_layout.insert(std::make_pair(Range<S_ADDRX>(tramp8_addr, tramp8_addr+JMP8_LEN-1), TRAMP_JMP8_PTR));
}
inline CC_LAYOUT_PAIR place_trampoline32(S_ADDRX tramp32_addr, INT32 offset32, CC_LAYOUT &cc_layout)
{
    //gen jmp rel32 instruction
    UINT16 pos = 0;
    std::string jmp_rel32_template = InstrGenerator::gen_jump_rel32_instr(pos, offset32);
    jmp_rel32_template.copy((char*)tramp32_addr, jmp_rel32_template.length());
    //insert trampoline8 into cc layout
    return cc_layout.insert(std::make_pair(Range<S_ADDRX>(tramp32_addr, tramp32_addr+JMP32_LEN-1), TRAMP_JMP32_PTR));
}

inline CC_LAYOUT_PAIR place_overlap_trampoline32(S_ADDRX tramp32_addr, INT32 offset32, CC_LAYOUT &cc_layout)
{
    //gen jmp rel32 instruction
    UINT16 pos = 0;
    std::string jmp_rel32_template = InstrGenerator::gen_jump_rel32_instr(pos, offset32);
    jmp_rel32_template.copy((char*)tramp32_addr, jmp_rel32_template.length());
    //insert trampoline8 into cc layout
    return cc_layout.insert(std::make_pair(Range<S_ADDRX>(tramp32_addr, tramp32_addr+OVERLAP_JMP32_LEN-1), TRAMP_OVERLAP_JMP32_PTR));
}

//this function is only used to place trampoline8 with trampoline32 
S_ADDRX front_to_place_overlap_trampoline32(S_ADDRX overlap_tramp_addr, UINT8 overlap_byte, CC_LAYOUT &cc_layout)
{
    // 1. set boundary to start searching
    std::pair<CC_LAYOUT_ITER, BOOL> boundary = cc_layout.insert(std::make_pair(Range<S_ADDRX>(overlap_tramp_addr, \
        overlap_tramp_addr+OVERLAP_JMP32_LEN-1), TRAMP_OVERLAP_JMP32_PTR));
    CC_LAYOUT_ITER curr_iter = boundary.first, prev_iter = --boundary.first;
    ASSERT(curr_iter!=cc_layout.begin());
    S_ADDRX trampoline32_addr = 0;
    //loop to find the space to place the tramp32
    while(prev_iter!=cc_layout.end()){
        S_SIZE left_space = curr_iter->first - prev_iter->first;
        if(left_space>=JMP32_LEN){
            trampoline32_addr = curr_iter->first.low() - JMP32_LEN;
            S_ADDRX end = prev_iter->first.high() + 1;
            while(trampoline32_addr!=end){
                INT32 offset32 = trampoline32_addr - overlap_tramp_addr - JMP32_LEN;
                if(((offset32>>24)&0xff)==overlap_byte){
                    CC_LAYOUT_PAIR ret = place_overlap_trampoline32(overlap_tramp_addr, offset32, cc_layout);
                    FATAL(ret.second, "place overlap trampoline32 wrong!\n");
                    return trampoline32_addr;
                }
                trampoline32_addr--;
            }
        }
    
        curr_iter--;
        prev_iter--;
    }
    ASSERT(prev_iter!=cc_layout.end());
    return trampoline32_addr;
}

S_ADDRX front_to_place_trampoline32(S_ADDRX fixed_trampoline_addr, CC_LAYOUT &cc_layout)
{
    // 1. set boundary to start searching
    std::pair<CC_LAYOUT_ITER, BOOL> boundary = cc_layout.insert(std::make_pair(Range<S_ADDRX>(fixed_trampoline_addr, \
        fixed_trampoline_addr+JMP8_LEN-1), TRAMP_JMP8_PTR));
    ASSERT(boundary.second);
    // 2. init scanner
    CC_LAYOUT_ITER curr_iter = boundary.first, prev_iter = --boundary.first;
    ASSERT(curr_iter!=cc_layout.begin());
    // 3. variables used to store the results
    S_ADDRX trampoline32_addr = 0;
    S_ADDRX trampoline8_base = fixed_trampoline_addr;
    S_ADDRX last_tramp8_addr = 0;
    // 4. loop to search
    while(prev_iter!=cc_layout.begin()){
        S_SIZE space = curr_iter->first - prev_iter->first;
        ASSERT(space>=0);
        // assumpe the dest can place tramp32
        trampoline32_addr = curr_iter->first.low() - JMP32_LEN;
        INT32 dest_offset8 = trampoline32_addr - trampoline8_base - JMP8_LEN;
        // 4.1 judge can place trampoline32
        if((space>=JMP32_LEN) && (dest_offset8>=SCHAR_MIN)){
            CC_LAYOUT_PAIR ret = place_trampoline8(trampoline8_base, dest_offset8, cc_layout);
            FATAL((trampoline8_base!=fixed_trampoline_addr) && !ret.second, " place trampoline8 wrong!\n");
            break;
        }
        // assumpe the dest can place tramp8
        S_ADDRX tramp8_addr = curr_iter->first.low() - JMP8_LEN;
        INT32 relay_offset8 = tramp8_addr - trampoline8_base - JMP8_LEN;
        // 4.2 judge is over 8 relative offset
        if(relay_offset8>=SCHAR_MIN)
            last_tramp8_addr = space>=JMP8_LEN ? tramp8_addr : last_tramp8_addr;
        else{//need relay
            ASSERT(last_tramp8_addr!=0);
            INT32 last_offset8 = last_tramp8_addr - trampoline8_base - JMP8_LEN;
            ASSERT(last_offset8>=SCHAR_MIN);
            //place the internal jmp8 rel8 template
            CC_LAYOUT_PAIR ret = place_trampoline8(trampoline8_base, last_offset8, cc_layout);
            FATAL((trampoline8_base!=fixed_trampoline_addr) && !ret.second, " place trampoline8 wrong!\n");
            //clear last
            trampoline8_base = last_tramp8_addr;
            last_tramp8_addr = 0;
        }

        curr_iter--;
        prev_iter--;
    }
    ASSERT(prev_iter!=cc_layout.begin());
    
    return trampoline32_addr;
}

S_ADDRX CodeVariantManager::arrange_cc_layout(S_ADDRX cc_base, CC_LAYOUT &cc_layout, \
    RBBL_CC_MAPS &rbbl_maps, JMPIN_CC_OFFSET &jmpin_rbbl_offsets)
{
    S_ADDRX trampoline32_addr = 0;
    S_ADDRX used_cc_base = 0;
    // 1.place fixed rbbl's trampoline  
    CC_LAYOUT_PAIR invalid_ret = place_invalid_boundary(cc_base, cc_layout);
    FATAL(!invalid_ret.second, " place invalid boundary wrong!\n");
    for(RAND_BBL_MAPS::iterator iter = _postion_fixed_rbbl_maps.begin(); iter!=_postion_fixed_rbbl_maps.end(); iter++){
        RAND_BBL_MAPS::iterator iter_bk = iter;
        F_SIZE curr_bbl_offset = iter->first;
        F_SIZE next_bbl_offset = (++iter)!=_postion_fixed_rbbl_maps.end() ? iter->first : curr_bbl_offset+JMP32_LEN;
        S_SIZE left_space = next_bbl_offset - curr_bbl_offset;
        iter = iter_bk;
        //place trampoline
        if(left_space>=JMP32_LEN){
            trampoline32_addr = curr_bbl_offset + cc_base;
            used_cc_base = trampoline32_addr + JMP32_LEN;
        }else{
            ASSERT(left_space>=JMP8_LEN);
            //search lower address to find space to place the jmp rel32 trampoline
            trampoline32_addr = front_to_place_trampoline32(curr_bbl_offset + cc_base, cc_layout);
            used_cc_base = next_bbl_offset + cc_base;
        }
        //place tramp32
        CC_LAYOUT_PAIR ret = place_trampoline32(trampoline32_addr, curr_bbl_offset, cc_layout);
        FATAL(!ret.second, " place trampoline32 wrong!\n");
    }
    // 2.place switch-case trampolines
    #define TRAMP_GAP 0x100
    S_ADDRX new_cc_base = used_cc_base + TRAMP_GAP;
    // get full jmpin targets
    TARGET_SET merge_set;
    for(JMPIN_ITERATOR iter = _switch_case_jmpin_rbbl_maps.begin(); iter!= _switch_case_jmpin_rbbl_maps.end(); iter++){
        F_SIZE jmpin_rbbl_offset = iter->first;
        P_SIZE jmpin_target_offset = _cc_offset + new_cc_base - cc_base;
        jmpin_rbbl_offsets.insert(std::make_pair(jmpin_rbbl_offset, jmpin_target_offset));
        merge_set.insert(iter->second.begin(), iter->second.end());
    }
    // 3.place jmpin targets trampoline
    for(TARGET_ITERATOR it = merge_set.begin(); it!=merge_set.end(); it++){
        TARGET_ITERATOR it_bk = it;
        F_SIZE curr_bbl_offset = *it;
        F_SIZE next_bbl_offset = (++it)!=merge_set.end() ? *it : curr_bbl_offset+JMP32_LEN;
        S_SIZE left_space = next_bbl_offset - curr_bbl_offset;
        it = it_bk;
        //can place trampoline
        if(left_space>=JMP32_LEN){
            trampoline32_addr = curr_bbl_offset + new_cc_base;
            used_cc_base = trampoline32_addr + JMP32_LEN;
        }else{
            ASSERT(left_space>=JMP8_LEN);
            //search lower address to find space to place the jmp rel32 trampoline
            trampoline32_addr = front_to_place_trampoline32(curr_bbl_offset + new_cc_base, cc_layout);
            used_cc_base = next_bbl_offset + new_cc_base;
        }
        //place tramp32
        CC_LAYOUT_PAIR ret = place_trampoline32(trampoline32_addr, curr_bbl_offset, cc_layout);
        FATAL(!ret.second, " place trampoline32 wrong!\n");
    }
    // 4.place fixed and movable rbbls
    for(RAND_BBL_MAPS::iterator iter = _postion_fixed_rbbl_maps.begin(); iter!=_postion_fixed_rbbl_maps.end(); iter++){
        RandomBBL *rbbl = iter->second;
        S_SIZE rbbl_size = rbbl->get_template_size();
        rbbl_maps.insert(std::make_pair(iter->first, used_cc_base));
        if(rbbl->has_lock_and_repeat_prefix())//consider prefix
            rbbl_maps.insert(std::make_pair(iter->first+1, used_cc_base+1));
        cc_layout.insert(std::make_pair(Range<S_ADDRX>(used_cc_base, used_cc_base+rbbl_size-1), (S_ADDRX)rbbl));
        used_cc_base += rbbl_size;
    }

    for(RAND_BBL_MAPS::iterator iter = _movable_rbbl_maps.begin(); iter!=_movable_rbbl_maps.end(); iter++){
        RandomBBL *rbbl = iter->second;
        S_SIZE rbbl_size = rbbl->get_template_size();
        rbbl_maps.insert(std::make_pair(iter->first, used_cc_base));
        if(rbbl->has_lock_and_repeat_prefix())//consider prefix
            rbbl_maps.insert(std::make_pair(iter->first+1, used_cc_base+1));
        cc_layout.insert(std::make_pair(Range<S_ADDRX>(used_cc_base, used_cc_base+rbbl_size-1), (S_ADDRX)rbbl));
        used_cc_base += rbbl_size;
    }

    //judge used cc size
    ASSERT((used_cc_base - cc_base)<=_cc_load_size);
    return used_cc_base;
}

void CodeVariantManager::relocate_rbbls_and_tramps(CC_LAYOUT &cc_layout, S_ADDRX cc_base, \
    RBBL_CC_MAPS &rbbl_maps, JMPIN_CC_OFFSET &jmpin_rbbl_offsets)
{
    for(CC_LAYOUT::iterator iter = cc_layout.begin(); iter!=cc_layout.end(); iter++){
        S_ADDRX range_base_addr = iter->first.low();
        S_SIZE range_size = iter->first.high() - range_base_addr + 1;
        switch(iter->second){
            case BOUNDARY_PTR: break;
            case TRAMP_JMP8_PTR: ASSERT(range_size>=JMP8_LEN); break;//has already relocated, when generate the jmp rel8
            case TRAMP_OVERLAP_JMP32_PTR: ASSERT(0); break;
            case TRAMP_JMP32_PTR://need relocate the trampolines
                {
                    ASSERT(range_size>=JMP32_LEN);
                    S_ADDRX relocate_addr = range_base_addr + 0x1;//opcode
                    S_ADDRX curr_pc = range_base_addr + JMP32_LEN;
                    F_SIZE target_rbbl_offset = (F_SIZE)(*(INT32*)relocate_addr);
                    RBBL_CC_MAPS::iterator ret = rbbl_maps.find(target_rbbl_offset);
                    ASSERT(ret!=rbbl_maps.end());
                    S_ADDRX target_rbbl_addr = ret->second;
                    INT64 offset64 = target_rbbl_addr - curr_pc;
                    ASSERT((offset64 > 0 ? offset64 : -offset64) < 0x7fffffff);
                    *(INT32*)relocate_addr = (INT32)offset64;
                }
                break;
            default://rbbl
                {
                    RandomBBL *rbbl = (RandomBBL*)iter->second;
                    F_SIZE rbbl_offset = rbbl->get_rbbl_offset();
                    JMPIN_CC_OFFSET::iterator ret = jmpin_rbbl_offsets.find(rbbl_offset);
                    P_SIZE jmpin_offset = ret!=jmpin_rbbl_offsets.end() ? ret->second : 0;//judge is switch case or not
                    rbbl->gen_code(cc_base, range_base_addr, range_size, _org_x_load_base, _cc_offset, _ss_offset, \
                        rbbl_maps, jmpin_offset);
                }
        }
    }
}

void CodeVariantManager::generate_code_variant(BOOL is_first_cc)
{
    S_ADDRX cc_base = is_first_cc ? _cc1_base : _cc2_base;
    CC_LAYOUT &cc_layout = is_first_cc ? _cc_layout1 : _cc_layout2;
    RBBL_CC_MAPS &rbbl_maps = is_first_cc ? _rbbl_maps1 : _rbbl_maps2;
    JMPIN_CC_OFFSET &jmpin_rbbl_offsets = is_first_cc ? _jmpin_rbbl_offsets1 : _jmpin_rbbl_offsets2;
    // 1.arrange the code layout
    arrange_cc_layout(cc_base, cc_layout, rbbl_maps, jmpin_rbbl_offsets);
    // 2.generate the code
    relocate_rbbls_and_tramps(cc_layout, cc_base, rbbl_maps, jmpin_rbbl_offsets);

    return ;
}

void CodeVariantManager::generate_all_code_variant()
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        iter->second->generate_code_variant(true);
        //iter->second->generate_code_variant(false);
    }
    
    return ;
}
