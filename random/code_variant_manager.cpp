#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>

#include "option.h"
#include "code_variant_manager.h"
#include "instr_generator.h"

CodeVariantManager::CVM_MAPS CodeVariantManager::_all_cvm_maps;
std::string CodeVariantManager::_code_variant_img_path;
SIZE CodeVariantManager::_cc_offset = 0;
SIZE CodeVariantManager::_ss_offset = 0;
P_ADDRX CodeVariantManager::_gs_base = 0;
P_ADDRX CodeVariantManager::_org_stack_load_base = 0;
P_ADDRX CodeVariantManager::_ss_load_base = 0;
S_ADDRX CodeVariantManager::_ss_base = 0;
P_SIZE CodeVariantManager::_ss_load_size = 0;
std::string CodeVariantManager::_ss_shm_path;
INT32 CodeVariantManager::_ss_fd = -1;
BOOL CodeVariantManager::_is_cv1_ready = false;
BOOL CodeVariantManager::_is_cv2_ready = false;
CodeVariantManager::SIG_HANDLERS CodeVariantManager::_sig_handlers;

std::string get_real_path(const char *file_path)
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

std::string get_real_name_from_path(std::string path)
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

static void close_shm_file(INT32 fd, std::string shm_path)
{
    std::string full_path = "/dev/shm/"+shm_path;
    INT32 ret = remove(full_path.c_str());
    FATAL(ret==-1, "remove %s failed!\n", shm_path.c_str());
    close(fd);
}

CodeVariantManager::~CodeVariantManager()
{
    close_shm_file(_cc_fd, _cc_shm_path);
    munmap((void*)_cc1_base, _cc_load_size*2);
}

void CodeVariantManager::init_cc()
{
    // 1.map cc
    S_SIZE map_size = 0;
    _cc_fd = map_shm_file(_cc_shm_path, _cc1_base, map_size);
    ASSERT(map_size==(_cc_load_size*2));
    _cc2_base = _cc1_base+map_size/2;  
    _cc1_used_base = _cc1_base;
    _cc2_used_base = _cc2_base;
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
    _ss_base = map_start + map_size;
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

#ifdef TRACE_DEBUG
S_ADDRX trace_debug_buffer_base = 0;
S_ADDRX trace_debug_buffer_end = 0;
void set_tdb_load_info(P_ADDRX start, P_SIZE load_size, std::string name)
{
    S_SIZE map_size = 0;
    int fd = map_shm_file(name, trace_debug_buffer_base, map_size);
    ASSERT(map_size==load_size);
    trace_debug_buffer_end = trace_debug_buffer_base + map_size;
    close_shm_file(fd);
}
#endif

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
                iter->second->set_x_load_base(currentRow->start, currentRow->end - currentRow->start);
            }
        }
        //shadow stack and stack
        if(!is_executable(currentRow)){
            std::string maps_record_name = get_real_name_from_path(get_real_path(currentRow->pathname));
            if(is_shared(currentRow)){//shadow stack
                if(maps_record_name.find(ss_sufix)!=std::string::npos)
                    set_ss_load_info(currentRow->end, currentRow->end-currentRow->start, maps_record_name);
            }else{
                if(strstr(currentRow->pathname, "[stack]"))//stack
                    set_stack_load_base(currentRow->end);
            }
        }

        //debug trace buffer
#ifdef TRACE_DEBUG
        if(strstr(currentRow->pathname, ".tdb")){
            std::string maps_record_name = get_real_name_from_path(get_real_path(currentRow->pathname));
            set_tdb_load_info(currentRow->start, currentRow->end - currentRow->start, maps_record_name);
        }
#endif
        
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
#define OFFSET_POS 0x1
#define JMP8_OPCODE 0xeb
#define JMP32_OPCODE 0xe9

inline CC_LAYOUT_PAIR place_invalid_boundary(S_ADDRX invalid_addr, CC_LAYOUT &cc_layout)
{
    std::string invalid_template = InstrGenerator::gen_invalid_instr();
    invalid_template.copy((char*)invalid_addr, invalid_template.length());
    return cc_layout.insert(std::make_pair(Range<S_ADDRX>(invalid_addr, invalid_addr+invalid_template.length()-1), BOUNDARY_PTR));
}

inline CC_LAYOUT_PAIR place_invalid_trampoline(S_ADDRX invalid_addr, CC_LAYOUT &cc_layout)
{
    std::string invalid_template = InstrGenerator::gen_invalid_instr();
    invalid_template.copy((char*)invalid_addr, invalid_template.length());
    return cc_layout.insert(std::make_pair(Range<S_ADDRX>(invalid_addr, invalid_addr+invalid_template.length()-1), INV_TRAMP_PTR));
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
            if(last_tramp8_addr==0){
                CC_LAYOUT_ITER erase_iter = cc_layout.upper_bound(Range<S_ADDRX>(fixed_trampoline_addr));
                cc_layout.erase(--erase_iter);
                return 0;
            }
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
    ASSERT(trampoline32_addr!=0);
    return trampoline32_addr;
}

S_ADDRX *random(const CodeVariantManager::RAND_BBL_MAPS fixed_rbbls, const CodeVariantManager::RAND_BBL_MAPS movable_rbbls, \
    SIZE &array_num)
{
    array_num = fixed_rbbls.size() + movable_rbbls.size();
    S_ADDRX *rbbl_array = new S_ADDRX[array_num];
    //init array
    SIZE index = 0;
    for(CodeVariantManager::RAND_BBL_MAPS::const_iterator iter = fixed_rbbls.begin(); iter!=fixed_rbbls.end(); iter++, index++)
        rbbl_array[index] = (S_ADDRX)iter->second;
    for(CodeVariantManager::RAND_BBL_MAPS::const_iterator iter = movable_rbbls.begin(); iter!=movable_rbbls.end(); iter++, index++)
        rbbl_array[index] = (S_ADDRX)iter->second;
    //random seed
    srand((INT32)time(NULL));
    for(SIZE idx=array_num-1; idx>0; idx--){
        //swap
        INT32 swap_idx = rand()%idx;
        INT32 temp = rbbl_array[swap_idx];
        rbbl_array[swap_idx] = rbbl_array[idx];
        rbbl_array[idx] = temp;
    }

    return rbbl_array;
}

S_ADDRX *fallthrough_following(const CodeVariantManager::RAND_BBL_MAPS fixed_rbbls, const CodeVariantManager::RAND_BBL_MAPS movable_rbbls, \
    SIZE &array_num)
{
    array_num = fixed_rbbls.size() + movable_rbbls.size();
    S_ADDRX *rbbl_array = new S_ADDRX[array_num];
    //init array
    CodeVariantManager::RAND_BBL_MAPS::const_iterator fixed_iter = fixed_rbbls.begin();
    CodeVariantManager::RAND_BBL_MAPS::const_iterator fixed_end = fixed_rbbls.end();
    CodeVariantManager::RAND_BBL_MAPS::const_iterator movable_iter = movable_rbbls.begin();
    CodeVariantManager::RAND_BBL_MAPS::const_iterator movable_end = movable_rbbls.end();
    
    for(SIZE index=0; index<array_num; index++){
        F_SIZE curr_fixed_offset = fixed_iter!=fixed_end ? fixed_iter->first : 0x7fffffff;
        F_SIZE curr_movable_offset = movable_iter!=movable_end ? movable_iter->first : 0x7fffffff;
        if(curr_fixed_offset<curr_movable_offset){
            rbbl_array[index] = (S_ADDRX)fixed_iter->second;
            fixed_iter++;
        }else if(curr_fixed_offset>curr_movable_offset){
            rbbl_array[index] = (S_ADDRX)movable_iter->second;
            movable_iter++;
        }else
            FATAL(1, "Can not be exist the same offset in both fixed_rbbls and movable_rbbls!\n");
    }

    return rbbl_array;
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
        S_ADDRX inv_trampoline_addr = 0;
        //place trampoline
        if(left_space>=JMP32_LEN){
            trampoline32_addr = curr_bbl_offset + cc_base;
            used_cc_base = trampoline32_addr + JMP32_LEN;
        }else{
            if(left_space<JMP8_LEN){
                ERR("There is no space to place the trampoline (%lx), so we only place the invalid instruction!\n", curr_bbl_offset);
                inv_trampoline_addr = curr_bbl_offset + cc_base;
            }else{
                //search lower address to find space to place the jmp rel32 trampoline
                trampoline32_addr = front_to_place_trampoline32(curr_bbl_offset + cc_base, cc_layout);
                if(trampoline32_addr==0){
                    ERR("There is no space to place the trampoline (%lx), so we only place the invalid instruction!\n", curr_bbl_offset);
                    inv_trampoline_addr = curr_bbl_offset + cc_base;
                }
            }
            
            used_cc_base = next_bbl_offset + cc_base;
        }
        CC_LAYOUT_PAIR ret;
        if(inv_trampoline_addr!=0){
            //place invalid instr
            ret = place_invalid_trampoline(inv_trampoline_addr, cc_layout);
            FATAL(!ret.second, " place inv_trampoline wrong!\n");
            inv_trampoline_addr = 0;
        }else{
            //place tramp32
            ret = place_trampoline32(trampoline32_addr, curr_bbl_offset, cc_layout);
            FATAL(!ret.second, " place trampoline32 wrong!\n");
        }
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
        S_ADDRX inv_trampoline_addr = 0;
        //can place trampoline
        if(left_space>=JMP32_LEN){
            trampoline32_addr = curr_bbl_offset + new_cc_base;
            used_cc_base = trampoline32_addr + JMP32_LEN;
        }else{
            if(left_space<JMP8_LEN){
                ERR("There is no space to place the trampoline (%lx), so we only place the invalid instruction!\n", curr_bbl_offset);
                inv_trampoline_addr = curr_bbl_offset + new_cc_base;
            }else{
                //search lower address to find space to place the jmp rel32 trampoline
                trampoline32_addr = front_to_place_trampoline32(curr_bbl_offset + new_cc_base, cc_layout);
                if(trampoline32_addr==0){
                    ERR("There is no space to place the trampoline (%lx), so we only place the invalid instruction!\n", curr_bbl_offset);
                    inv_trampoline_addr = curr_bbl_offset + new_cc_base;
                }
            }
            
            used_cc_base = next_bbl_offset + new_cc_base;
        }

        CC_LAYOUT_PAIR ret;
        if(inv_trampoline_addr!=0){
            //place invalid instr
            ret = place_invalid_trampoline(inv_trampoline_addr, cc_layout);
            FATAL(!ret.second, " place inv_trampoline wrong!\n");
            inv_trampoline_addr = 0;
        }else{
            //place tramp32
            ret = place_trampoline32(trampoline32_addr, curr_bbl_offset, cc_layout);
            FATAL(!ret.second, " place trampoline32 wrong!\n");
        }
    }
    // 4.place fixed and movable rbbls
    // 4.1 random fixed and movable rbbls
    SIZE rbbl_array_size;
    S_ADDRX *rbbl_array = NULL;
    if(Options::_need_randomize_rbbl)
        rbbl_array = random(_postion_fixed_rbbl_maps, _movable_rbbl_maps, rbbl_array_size);
    else
        rbbl_array = fallthrough_following(_postion_fixed_rbbl_maps, _movable_rbbl_maps, rbbl_array_size);
    // 4.2 place rbbls
    SIZE reduce_num = 0;
    for(SIZE idx = 0; idx<rbbl_array_size; idx++){
        RandomBBL *curr_rbbl = (RandomBBL*)rbbl_array[idx];
        RandomBBL *next_rbbl = idx<(rbbl_array_size-1) ? (RandomBBL*)rbbl_array[idx+1] : NULL;
        S_SIZE place_size = curr_rbbl->get_template_size();
        
        if(next_rbbl){
            F_SIZE next_rbbl_offset = next_rbbl->get_rbbl_offset();
            if(next_rbbl_offset==curr_rbbl->get_last_br_target()){
                place_size -= 5;//JMP_REL32 instruction len
                reduce_num++;
            }
        }
        F_SIZE curr_rbbl_offset = curr_rbbl->get_rbbl_offset();
        rbbl_maps.insert(std::make_pair(curr_rbbl_offset, used_cc_base));
        if(curr_rbbl->has_lock_and_repeat_prefix()){//consider prefix
            ASSERT(place_size>1);
            S_ADDRX prefix_start = used_cc_base + 1;
#ifdef TRACE_DEBUG
            if(_org_x_load_base<0x7fffffff)//main executable 
                prefix_start += 29;
#endif
#ifdef LAST_RBBL_DEBUG
            prefix_start += 22;
#endif
            rbbl_maps.insert(std::make_pair(curr_rbbl_offset+1, prefix_start));      
        }

        if(place_size>0){
            cc_layout.insert(std::make_pair(Range<S_ADDRX>(used_cc_base, used_cc_base+place_size-1), (S_ADDRX)curr_rbbl));
            used_cc_base += place_size;
        }else
            ASSERT(place_size==0);
    }
    // 4.3 free array
    delete []rbbl_array;
    //BLUE("Sum: %d, Reduce: %d(%lf)\n", (INT32)rbbl_array_size, (INT32)reduce_num, ((double)reduce_num*100)/(double)rbbl_array_size);

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
            case INV_TRAMP_PTR: break;
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
                    rbbl->gen_code(cc_base, range_base_addr, range_size, _org_x_load_base, _cc_offset, _ss_offset, _gs_base, \
                        _ss_type, rbbl_maps, jmpin_offset);
                }
        }
    }
}

void CodeVariantManager::clean_cc(BOOL is_first_cc)
{
    S_ADDRX cc_base = is_first_cc ? _cc1_base : _cc2_base;
    S_ADDRX place_addr = cc_base;
    std::string invalid_instr = InstrGenerator::gen_invalid_instr();
    S_SIZE instr_len = invalid_instr.length();
    S_ADDRX cc_end = cc_base + _cc_load_size - instr_len;

    while(place_addr<=cc_end){
        invalid_instr.copy((char*)place_addr, instr_len);
        place_addr += instr_len;
    }
    return ;
}

void CodeVariantManager::generate_code_variant(BOOL is_first_cc)
{
    S_ADDRX cc_base = is_first_cc ? _cc1_base : _cc2_base;
    CC_LAYOUT &cc_layout = is_first_cc ? _cc_layout1 : _cc_layout2;
    RBBL_CC_MAPS &rbbl_maps = is_first_cc ? _rbbl_maps1 : _rbbl_maps2;
    JMPIN_CC_OFFSET &jmpin_rbbl_offsets = is_first_cc ? _jmpin_rbbl_offsets1 : _jmpin_rbbl_offsets2;
    S_ADDRX &cc_used_base = is_first_cc ? _cc1_used_base : _cc2_used_base;
    // 1.clean code cache
    clean_cc(is_first_cc);
    // 2.arrange the code layout
    cc_used_base = arrange_cc_layout(cc_base, cc_layout, rbbl_maps, jmpin_rbbl_offsets);
    // 3.generate the code
    relocate_rbbls_and_tramps(cc_layout, cc_base, rbbl_maps, jmpin_rbbl_offsets);
    
    return ;
}

void CodeVariantManager::generate_all_code_variant(BOOL is_first_cc)
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++)
        iter->second->generate_code_variant(is_first_cc);

    patch_all_sigaction_entry(is_first_cc);
    return ;
}

static BOOL need_stop = false;
static BOOL need_pause = false;
pthread_t child_thread;

void CodeVariantManager::start_gen_code_variants()
{
    need_stop = false;
    pthread_create(&child_thread, NULL, generate_code_variant_concurrently, NULL);
}

void CodeVariantManager::stop_gen_code_variants()
{
    need_stop = true;
    pthread_join(child_thread, NULL);
}

void CodeVariantManager::pause_gen_code_variants()
{
    need_pause = true;
}

void CodeVariantManager::continue_gen_code_variants()
{
    need_pause = false;
}

void* CodeVariantManager::generate_code_variant_concurrently(void *arg)
{
    while(!need_stop){
        if(!_is_cv1_ready){
            generate_all_code_variant(true);
            _is_cv1_ready = true;
        }

        if(!_is_cv2_ready){
            generate_all_code_variant(false);
            _is_cv2_ready = true;
        }
        
        while(need_pause)
            sched_yield();
        
        sched_yield();
    }
    return NULL;
}

void CodeVariantManager::clear_sighandler(BOOL is_first_cc)
{
    for(SIG_HANDLERS::iterator iter = _sig_handlers.begin(); iter!=_sig_handlers.end(); iter++){
        SIG_INFO &sig_info = iter->second;
        BOOL &handled = is_first_cc ? sig_info.cv1_handled : sig_info.cv2_handled;
        handled = false;
    }
}

void CodeVariantManager::clear_cv(BOOL is_first_cc)
{
    CC_LAYOUT &cc_layout = is_first_cc ? _cc_layout1 : _cc_layout2;
    RBBL_CC_MAPS &rbbl_maps = is_first_cc ? _rbbl_maps1 : _rbbl_maps2;
    JMPIN_CC_OFFSET &jmpin_rbbl_offsets = is_first_cc ? _jmpin_rbbl_offsets1 : _jmpin_rbbl_offsets2;
    S_ADDRX &cc_used_base = is_first_cc ? _cc1_used_base : _cc2_used_base;
    S_ADDRX &cc_base = is_first_cc ? _cc1_base : _cc2_base;
    cc_layout.clear();
    rbbl_maps.clear();
    jmpin_rbbl_offsets.clear();
    cc_used_base = cc_base;
    clear_sighandler(is_first_cc);
}

void CodeVariantManager::clear_all_cv(BOOL is_first_cc)
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++)
        iter->second->clear_cv(is_first_cc);
}

void CodeVariantManager::consume_cv(BOOL is_first_cc)
{
    // 1.clear all code variants
    clear_all_cv(is_first_cc);
    // 2.clear ready flags
    if(is_first_cc){
        ASSERT(_is_cv1_ready);
        _is_cv1_ready = false;
    }else{
        ASSERT(_is_cv2_ready);
        
        _is_cv2_ready = false;
    }
}

void CodeVariantManager::wait_for_code_variant_ready(BOOL is_first_cc)
{
    BOOL &is_ready = is_first_cc ? _is_cv1_ready : _is_cv2_ready;
    while(!is_ready)
        sched_yield();
}

RandomBBL *CodeVariantManager::find_rbbl_from_all_paddrx(P_ADDRX p_addr, BOOL is_first_cc)
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        RandomBBL *rbbl = iter->second->find_rbbl_from_paddrx(p_addr, is_first_cc);
        if(rbbl)
            return rbbl;
    }
    return NULL;
}

RandomBBL *CodeVariantManager::find_rbbl_from_all_saddrx(S_ADDRX s_addr, BOOL is_first_cc)
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        RandomBBL *rbbl = iter->second->find_rbbl_from_saddrx(s_addr, is_first_cc);
        if(rbbl)
            return rbbl;
    }
    return NULL;
}

RandomBBL *CodeVariantManager::find_rbbl_from_paddrx(P_ADDRX p_addr, BOOL is_first_cc)
{
    S_ADDRX cc_base = is_first_cc ? _cc1_base : _cc2_base;
    
    if(p_addr>=_cc_load_base && p_addr<(_cc_load_base+_cc_load_size))
        return find_rbbl_from_saddrx(p_addr - _cc_load_base + cc_base, is_first_cc);
    else
        return NULL;
}

RandomBBL *CodeVariantManager::find_rbbl_from_saddrx(S_ADDRX s_addr, BOOL is_first_cc)
{
    S_ADDRX cc_base = is_first_cc ? _cc1_base : _cc2_base;
    CC_LAYOUT &cc_layout = is_first_cc ? _cc_layout1 : _cc_layout2;

    if(s_addr>=cc_base && s_addr<(cc_base+_cc_load_size)){
        CC_LAYOUT_ITER iter = cc_layout.upper_bound(Range<S_ADDRX>(s_addr));
        if(iter!=cc_layout.end() && s_addr<=(--iter)->first.high() && s_addr>=iter->first.low()){
            S_ADDRX ptr = iter->second;
            S_ADDRX start = iter->first.low();
            switch(ptr){
                case BOUNDARY_PTR: return NULL;
                case TRAMP_JMP8_PTR: 
                    {
                        if(start!=s_addr)//s_addr must be trampoline aligned
                            return NULL;
                        ASSERT(*(UINT8*)start==JMP8_OPCODE);    
                        INT8 offset8 = *(INT8*)(start+OFFSET_POS);
                        S_ADDRX target_addr = s_addr + JMP8_LEN + offset8;    
                        return find_rbbl_from_saddrx(target_addr, is_first_cc);
                    }
                case TRAMP_OVERLAP_JMP32_PTR: ASSERT(0); return NULL;
                case TRAMP_JMP32_PTR://need relocate the trampolines
                    {
                        if(start!=s_addr)//s_addr must be trampoline aligned
                            return NULL;
                        ASSERT(*(UINT8*)start==JMP32_OPCODE);    
                        INT32 offset32 = *(INT32*)(start+OFFSET_POS);
                        S_ADDRX target_addr = s_addr + JMP32_LEN + offset32;
                        return find_rbbl_from_saddrx(target_addr, is_first_cc);
                    }
                default://rbbl
                    return (RandomBBL*)ptr;
                }
        }else
            return NULL;
    }else
        return NULL;
}

P_ADDRX CodeVariantManager::find_cc_paddrx_from_all_orig(P_ADDRX orig_p_addrx, BOOL is_first_cc)
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        P_ADDRX ret_addrx = iter->second->find_cc_paddrx_from_orig(orig_p_addrx, is_first_cc);
        if(ret_addrx!=0)
            return ret_addrx;
    }
    return 0;
}

S_ADDRX CodeVariantManager::find_cc_saddrx_from_all_orig(P_ADDRX orig_p_addrx, BOOL is_first_cc)
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        P_ADDRX ret_addrx = iter->second->find_cc_saddrx_from_orig(orig_p_addrx, is_first_cc);
        if(ret_addrx!=0)
            return ret_addrx;
    }
    return 0;
}

S_ADDRX CodeVariantManager::find_cc_saddrx_from_orig(P_ADDRX orig_p_addrx, BOOL is_first_cc)
{
    RBBL_CC_MAPS &rbbl_maps = is_first_cc ? _rbbl_maps1 : _rbbl_maps2;
    F_SIZE pc_size = orig_p_addrx - _org_x_load_base;
    if(pc_size<_org_x_load_size){
        RBBL_CC_MAPS::iterator iter = rbbl_maps.find(pc_size);
        return iter!=rbbl_maps.end() ? iter->second : 0;
    }else
        return 0;
}

P_ADDRX CodeVariantManager::find_cc_paddrx_from_orig(P_ADDRX orig_p_addrx, BOOL is_first_cc)
{
    S_ADDRX cc_base = is_first_cc ? _cc1_base : _cc2_base;
    S_ADDRX ret_addrx = find_cc_saddrx_from_orig(orig_p_addrx, is_first_cc);
    return ret_addrx!=0 ? (ret_addrx - cc_base + _cc_load_base) : 0;
}

P_ADDRX CodeVariantManager::find_cc_paddrx_from_rbbl(RandomBBL *rbbl, BOOL is_first_cc)
{
    RBBL_CC_MAPS &rbbl_maps = is_first_cc ? _rbbl_maps1 : _rbbl_maps2;    
    CC_LAYOUT &cc_layout = is_first_cc ? _cc_layout1 : _cc_layout2;
    F_SIZE rbbl_offset = rbbl->get_rbbl_offset();
    S_ADDRX cc_base = is_first_cc ? _cc1_base : _cc2_base;
    
    RBBL_CC_MAPS::iterator iter = rbbl_maps.find(rbbl_offset);
    if(iter!=rbbl_maps.end()){
        S_ADDRX s_addrx = iter->second;
        CC_LAYOUT_ITER it = cc_layout.lower_bound(Range<S_ADDRX>(s_addrx));
        return (it!=cc_layout.end())&&(it->second==(S_ADDRX)rbbl)? (_cc_load_base + s_addrx - cc_base) : 0; 
    }else
        return 0;
}

P_ADDRX CodeVariantManager::find_cc_paddrx_from_all_rbbls(RandomBBL *rbbl, BOOL is_first_cc)
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        P_ADDRX ret_addrx = iter->second->find_cc_paddrx_from_rbbl(rbbl, is_first_cc);
        if(ret_addrx!=0)
            return ret_addrx;
    }
    return 0;
}

P_ADDRX CodeVariantManager::get_new_pc_from_old(P_ADDRX old_pc, BOOL first_cc_is_new)
{
    RandomBBL *rbbl = find_rbbl_from_paddrx(old_pc, first_cc_is_new ? false : true);
    if(rbbl){
        //get old and new code variant information
        RBBL_CC_MAPS &old_rbbl_maps = first_cc_is_new ? _rbbl_maps2 : _rbbl_maps1;
        RBBL_CC_MAPS &new_rbbl_maps = first_cc_is_new ? _rbbl_maps1 : _rbbl_maps2;
        S_ADDRX old_cc_base = first_cc_is_new ? _cc2_base : _cc1_base;
        S_ADDRX new_cc_base = first_cc_is_new ? _cc1_base : _cc2_base;
        //get old rbbl offset
        RBBL_CC_MAPS::iterator it = old_rbbl_maps.find(rbbl->get_rbbl_offset());
        ASSERT(it!=old_rbbl_maps.end());
        S_ADDRX old_pc_saddrx = old_pc - _cc_load_base + old_cc_base;
        S_SIZE rbbl_internal_offset = old_pc_saddrx - it->second;
        //get new rbbl saddrx
        it = new_rbbl_maps.find(rbbl->get_rbbl_offset());
        ASSERT(it!=new_rbbl_maps.end());
        S_ADDRX new_pc_saddrx = it->second + rbbl_internal_offset;
        return new_pc_saddrx - new_cc_base + _cc_load_base;
    }else
        return 0;
}

P_ADDRX CodeVariantManager::get_new_pc_from_old_all(P_ADDRX old_pc, BOOL first_cc_is_new)
{
    ASSERT(_is_cv1_ready&&_is_cv2_ready);

    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        P_ADDRX new_pc = iter->second->get_new_pc_from_old(old_pc, first_cc_is_new);
        if(new_pc!=0)
            return new_pc;                
    }
    
    return 0;
}

void CodeVariantManager::modify_new_ra_in_ss(BOOL first_cc_is_new)
{
    //TODO: we only handle ordinary shadow stack, not shadow stack++
    ASSERT(_is_cv1_ready && _is_cv2_ready);
    S_ADDRX return_addr_ptr = _ss_base - sizeof(P_ADDRX);
    
    while(return_addr_ptr>=(_ss_base-_ss_load_size)){
        P_ADDRX old_return_addr = *(P_ADDRX *)return_addr_ptr;
        P_ADDRX new_return_addr = get_new_pc_from_old_all(old_return_addr, first_cc_is_new);
        //modify old return address to the new return address
        if(new_return_addr!=0)
            *(P_ADDRX *)return_addr_ptr = new_return_addr;
        
        return_addr_ptr -= sizeof(P_ADDRX);
    }
}

void CodeVariantManager::recycle()
{
    //1. recycle code cache and shm files
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        CodeVariantManager *cvm = iter->second;
        delete cvm;
    }
    //2. recycle shadow stack and shm files
    close_shm_file(_ss_fd, _ss_shm_path);
    munmap((void*)_ss_base, _ss_load_size);
}

/************DB SEG*************/
/* 32BIT SEG TYPE + 32BIT NUM  */
/*******************************/
static UINT32 db_seg_movable = 0;
static UINT32 db_seg_fixed = 1;
static UINT32 db_seg_all_jmpin = 2;
//db_seg_jmpin: the offset of jmpin src, it must be larger than 2

static SIZE read_rbbls(S_ADDRX r_addrx, CodeVariantManager *cvm, BOOL is_movable)
{
    S_ADDRX start_addrx = r_addrx;
    UINT32 *ptr_32 = NULL;
    //1. read seg type
    ptr_32 = (UINT32*)r_addrx;
    INT32 should_type = is_movable ? db_seg_movable : db_seg_fixed;
    INT32 seg_type = *ptr_32++;
    FATAL(should_type!=seg_type, "seg type (%d) is wrong, should be %d!\n", seg_type, should_type);
    //2. read rbbl num
    SIZE rbbl_sum = *ptr_32++;
    r_addrx = (S_ADDRX)ptr_32;
    //3. store all rbbls
    for(SIZE index = 0; index<rbbl_sum; index++){
        //3.1 read rbbl
        SIZE used_size = 0;
        RandomBBL *rbbl = RandomBBL::read_rbbl(r_addrx, used_size);
        ASSERT(used_size!=0 && rbbl);
        r_addrx += used_size;
        //3.2 insert
        if(is_movable)
            cvm->insert_movable_random_bbl(rbbl->get_rbbl_offset(), rbbl);
        else
            cvm->insert_fixed_random_bbl(rbbl->get_rbbl_offset(), rbbl);
    }
    //4. return
    return r_addrx - start_addrx;

}

static SIZE read_switch_case_info(S_ADDRX r_addrx, CodeVariantManager *cvm)
{
    UINT32 *ptr_32 = NULL;
    //1. read seg type
    ptr_32 = (UINT32*)r_addrx;
    UINT32 seg_type = *ptr_32++;
    FATAL(seg_type!=db_seg_all_jmpin, "type unmatched!\n");
    //2. read jmpin sum
    SIZE jmpin_sum = *ptr_32++;
    //3. read all jmpins
    for(SIZE index = 0; index<jmpin_sum; index++){
        //3.1 read jmpin offset
        F_SIZE jmpin_offset = *ptr_32++;
        //3.2 read target sum
        SIZE target_sum = *ptr_32++;
        //3.3 read all targets
        CodeVariantManager::TARGET_SET target_set;
        for(SIZE idx = 0; idx<target_sum; idx++)
            target_set.insert((F_SIZE)*ptr_32++);
        //3.4 insert
        cvm->insert_switch_case_jmpin_rbbl(jmpin_offset, target_set);
        target_set.clear();
    }

    //4. return
    return (S_ADDRX)ptr_32 - r_addrx;
}

static SIZE store_rbbls(S_ADDRX s_addrx, CodeVariantManager::RAND_BBL_MAPS &rbbl_maps, BOOL is_movable)
{
    S_ADDRX start_addrx = s_addrx;
    UINT32 *ptr_32 = NULL;
    //1. store seg type
    ptr_32 = (UINT32*)s_addrx;
    *ptr_32++ = is_movable ? db_seg_movable : db_seg_fixed;
    //2. store rbbl num
    SIZE rbbl_num = rbbl_maps.size();
    ASSERT(rbbl_num<=UINT_MAX);
    *ptr_32++ = (UINT32)rbbl_num;
    s_addrx = (S_ADDRX)ptr_32;
    //3. store all rbbls
    for(CodeVariantManager::RAND_BBL_MAPS::iterator iter = rbbl_maps.begin(); iter!=rbbl_maps.end(); iter++){
        RandomBBL *rbbl = iter->second;
        s_addrx += rbbl->store_rbbl(s_addrx);
    }
    //4. return
    return s_addrx - start_addrx;
}

static SIZE store_switch_case_info(S_ADDRX s_addrx, CodeVariantManager::JMPIN_TARGETS_MAPS &jmpin_maps)
{
    UINT32 *ptr_32 = NULL;
    //1. store seg type
    ptr_32 = (UINT32*)s_addrx;
    *ptr_32++ = db_seg_all_jmpin;
    //2. store jmpin num
    SIZE jmpin_num = jmpin_maps.size();
    *ptr_32++ = (UINT32)jmpin_num;
    //3. store all jmpins
    for(CodeVariantManager::JMPIN_ITERATOR iter = jmpin_maps.begin(); iter!=jmpin_maps.end(); iter++){
        F_SIZE jmpin_offset = iter->first;
        CodeVariantManager::TARGET_SET &target_set = iter->second;
        SIZE target_num = target_set.size();
        //3.1 store jmpin offset
        ASSERT(jmpin_offset<=UINT_MAX);
        *ptr_32++ = (UINT32)jmpin_offset;
        //3.2 store target num
        ASSERT(target_num<=UINT_MAX);
        *ptr_32++ = (UINT32)target_num;
        //3.3 store all targets
        for(CodeVariantManager::TARGET_ITERATOR it = target_set.begin(); it!=target_set.end(); it++){
            F_SIZE target_offset = *it;
            ASSERT(target_offset<=UINT_MAX);
            *ptr_32++ = (UINT32)target_offset;
        }
    }
    //4. return
    return (S_ADDRX)ptr_32 - s_addrx;
}

void handle_directory_path(std::string &db_path)
{
    //judge the directory is exist or not
    struct stat fileStat;
    FATAL(stat(db_path.c_str(), &fileStat)!=0 || !S_ISDIR(fileStat.st_mode), "%s is not legal directory path!\n", db_path.c_str());
    //add '/' to db path
    if(db_path[db_path.length()-1]!='/')
        db_path += '/';
}

static void find_dependence_lib_to_init_cvm(std::string elf_path)
{
    // get the path of all dependence libraries
    // 1.generate command string
    char command[128];
    sprintf(command, "/usr/bin/ldd %s", elf_path.c_str());
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
        //construct the CodeVariantManager
        if((path[0]!='\0'))
            new CodeVariantManager(std::string(path));
    }
    free(line_buf);
    // 5.close stream
    pclose(p_stream);
}

static std::string get_ss_suffix(LKM_SS_TYPE ss_type)
{
    switch(ss_type){
        case LKM_OFFSET_SS_TYPE:
            return std::string(".oss");
        case LKM_SEG_SS_TYPE:
            return std::string(".sss");
        case LKM_SEG_SS_PP_TYPE:
            return std::string(".pss");
        default:
            ASSERTM(0, "Unkown shadow stack type %d!\n", ss_type);
            return std::string("");
    }
}

void CodeVariantManager::init_from_db(std::string elf_path, std::string db_path, LKM_SS_TYPE ss_type)
{
    //1. judge the directory is exist or not
    handle_directory_path(db_path);
    //2. construct all cvms
    new CodeVariantManager(elf_path);
    find_dependence_lib_to_init_cvm(elf_path);
    //3. read db to initialize the code variant manager
     //3.1 prepare shadow stack suffix
    std::string ss_suffix = get_ss_suffix(ss_type);
     //3.2 read db to init all cvms
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        CodeVariantManager *cvm = iter->second;
        //3.2.1 construct the db path
        std::string cvm_name = cvm->get_name();
        std::string cvm_db_path = db_path + cvm_name + ss_suffix;
        //3.2.2 open db file
        INT32 fd = open(cvm_db_path.c_str(), O_RDWR);
        FATAL(fd==-1, "Open db file %s failed!\n", cvm_db_path.c_str());
        //3.2.3 get map size
        struct stat statbuf;
        INT32 ret = fstat(fd, &statbuf);
        FATAL(ret!=0, "Get %s information failed!\n", cvm_db_path.c_str());
        SIZE map_size = X86_PAGE_ALIGN_CEIL(statbuf.st_size);
        //3.2.4 map the file
        void *buf_start = mmap(NULL, map_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
        ASSERT(buf_start!=MAP_FAILED);
        S_ADDRX read_ptr = (S_ADDRX)buf_start;
        
        //3.2.5 read cvm information
        cvm->set_ss_type(ss_type);
         //3.2.5.1 read postion fixed rbbl
        read_ptr += read_rbbls(read_ptr, cvm, false);
         //3.2.5.2 read movable rbbls
        read_ptr += read_rbbls(read_ptr, cvm, true);
         //3.2.5.3 read switch_case jmpin targets
        read_ptr += read_switch_case_info(read_ptr, cvm);
         
        ASSERT(read_ptr==(statbuf.st_size+(S_ADDRX)buf_start));
        //3.2.6 unmap 
        ret = munmap(buf_start, map_size);
        ASSERT(ret==0);
        //3.2.7 close the file
        close(fd);
    }
}

void CodeVariantManager::store_into_db(std::string db_path)
{
    //judge the directory is exist or not
    handle_directory_path(db_path);
    //record all information
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        CodeVariantManager *cvm = iter->second;
        //1. prepare shadow stack suffix
        std::string ss_suffix = get_ss_suffix(cvm->_ss_type);
        //2. store all modules
        #define BUF_SIZE (1ull<<32)
         //2.1 construct the db path for each cvm
        std::string cvm_name = cvm->get_name();
        std::string cvm_db_path = db_path + cvm_name+ss_suffix;
         //2.2 remove the old db 
        remove(cvm_db_path.c_str());
         //2.3 create db
        INT32 fd = creat(cvm_db_path.c_str(), S_IRUSR|S_IWUSR);
        FATAL(fd==-1, "Create %s failed!\n", cvm_db_path.c_str());    
        close(fd);
         //2.4 open db file
        fd = open(cvm_db_path.c_str(), O_RDWR);
        FATAL(fd==-1, "Can not open %s\n", cvm_db_path.c_str());     
         //2.5 enlarge the file      
        struct stat statbuf;
        INT32 ret = fstat(fd, &statbuf);
        FATAL(ret!=0, "Get %s information failed!\n", cvm_db_path.c_str());
        ret = ftruncate(fd, BUF_SIZE);
        FATAL(ret!=0, "Enlarge %s failed!\n", cvm_db_path.c_str());    
         //2.6 map the buffer with the db file
        void *buf_start = mmap(NULL, BUF_SIZE, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
        ASSERT(buf_start!=MAP_FAILED);
        S_ADDRX store_ptr = (S_ADDRX)buf_start;
         //2.7 protect the last page
        ret = mprotect((void*)((S_ADDRX)buf_start+BUF_SIZE-X86_PAGE_SIZE), X86_PAGE_SIZE, PROT_NONE);
        FATAL(ret!=0, "protect the last page error!\n");
        
        //3. store cvm information
         //3.1 store postion fixed rbbl
        store_ptr += store_rbbls(store_ptr, cvm->_postion_fixed_rbbl_maps, false);
         //3.2 store movable rbbls
        store_ptr += store_rbbls(store_ptr, cvm->_movable_rbbl_maps, true);
         //3.3 store switch_case jmpin targets
        store_ptr += store_switch_case_info(store_ptr, cvm->_switch_case_jmpin_rbbl_maps);
        
        //4. unmap and dwindle the file
        ret = munmap(buf_start, BUF_SIZE);
        ASSERT(ret==0);
        SIZE used_size = store_ptr-(S_ADDRX)buf_start;
        ret = ftruncate(fd, used_size);
        FATAL(ret!=0, "Dwindle %s failed!\n", cvm_db_path.c_str());   
        
        //5. close the file
        close(fd);
    }
}

S_ADDRX patch_sigreturn_ss_template(S_ADDRX cc_used_base, LKM_SS_TYPE ss_type, SIZE ss_offset, P_ADDRX gs_base, \
    P_ADDRX sigreturn_paddrx, S_ADDRX handler_saddrx)
{
    //generate template
    P_SIZE virtual_ss_offset = 0;//virtual shadow stack offset is used to relocate the instruction
    switch(ss_type){
        case LKM_OFFSET_SS_TYPE: virtual_ss_offset = ss_offset; break;
        case LKM_SEG_SS_TYPE: virtual_ss_offset = ss_offset + gs_base; ASSERT(gs_base!=0); break;
        case LKM_SEG_SS_PP_TYPE: virtual_ss_offset = ss_offset + gs_base; ASSERT(gs_base!=0); break;
        default:
            ASSERTM(0, "Unkown shadow stack type %d\n", ss_type);
    }
    UINT16 imm32_pos, disp32_pos;
    INT32 high32 = (sigreturn_paddrx>>32)&0xffffffff;
    INT32 low32 = (INT32)sigreturn_paddrx;
    //movl ($ss_offset)(%rsp), 0xffffffff&(sigreturn_paddrx>>32)
    std::string movl_l32_template;
    if(ss_type==LKM_OFFSET_SS_TYPE)
        movl_l32_template = InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(imm32_pos, low32, disp32_pos, (INT32)(-virtual_ss_offset));
    else if(ss_type==LKM_SEG_SS_TYPE)
        movl_l32_template = InstrGenerator::gen_movl_imm32_to_gs_rsp_smem_instr(imm32_pos, low32, disp32_pos, (INT32)(-virtual_ss_offset));
    else
        ASSERT(0);
    S_SIZE instr_len = movl_l32_template.length();
    movl_l32_template.copy((char*)cc_used_base, instr_len);
    cc_used_base += instr_len;
    //movl ($ss_offset+4)(%rsp), 0xffffffff&(sigreturn_paddrx)
    std::string movl_h32_template;
    if(ss_type==LKM_OFFSET_SS_TYPE)
        movl_h32_template = InstrGenerator::gen_movl_imm32_to_rsp_smem_instr(imm32_pos, high32, disp32_pos, (INT32)(4-virtual_ss_offset));
    else if(ss_type==LKM_SEG_SS_TYPE)
        movl_h32_template = InstrGenerator::gen_movl_imm32_to_gs_rsp_smem_instr(imm32_pos, high32, disp32_pos, (INT32)(4-virtual_ss_offset));
    else
        ASSERT(0);
    instr_len = movl_h32_template.length();
    movl_h32_template.copy((char*)cc_used_base, instr_len);
    cc_used_base += instr_len;
    //jmp rel32
    std::string jmp_rel32_template = InstrGenerator::gen_jump_rel32_instr(disp32_pos, 0);
    S_ADDRX relocate_saddrx = disp32_pos + cc_used_base;
    instr_len = jmp_rel32_template.length();
    jmp_rel32_template.copy((char*)cc_used_base, instr_len);
    cc_used_base += instr_len;
    *(INT32*)relocate_saddrx = handler_saddrx - cc_used_base;

    return cc_used_base;
}

void CodeVariantManager::patch_sigaction_entry(BOOL is_first_cc, P_ADDRX handler_paddrx, P_ADDRX sigreturn_paddrx)
{
    CC_LAYOUT &cc_layout = is_first_cc ? _cc_layout1 : _cc_layout2;
    S_ADDRX base_saddrx = is_first_cc ? _cc1_base : _cc2_base;
    RBBL_CC_MAPS &rbbl_maps = is_first_cc ? _rbbl_maps1 : _rbbl_maps2;
    S_ADDRX &cc_used_base = is_first_cc ? _cc1_used_base : _cc2_used_base;
    
    if(handler_paddrx>=_org_x_load_base && handler_paddrx<(_org_x_load_base + _org_x_load_size)){
        //1. get offset 
        F_SIZE handler_offset = handler_paddrx - _org_x_load_base;
        ASSERTM(_postion_fixed_rbbl_maps.find(handler_offset)!=_postion_fixed_rbbl_maps.end(),\
            "signal handler entry basic block must be position fixed!\n");
        //2. get handler addr in code variant
        RBBL_CC_MAPS::iterator handler_iter = rbbl_maps.find(handler_offset);
        ASSERT(handler_iter!=rbbl_maps.end());
        //3. get handler's trampoline addr
        S_ADDRX tramp_saddrx = handler_offset + base_saddrx;
        CC_LAYOUT_ITER cc_iter = cc_layout.upper_bound(Range<S_ADDRX>(tramp_saddrx));
        if(cc_iter!=cc_layout.end() && tramp_saddrx<=(--cc_iter)->first.high() && tramp_saddrx>=cc_iter->first.low()){
            S_ADDRX start = cc_iter->first.low();
            ASSERTM(cc_iter->second==TRAMP_JMP32_PTR && *(UINT8*)start==JMP32_OPCODE, "Must be trampoline32!\n");
            ASSERT(start==tramp_saddrx);
            INT32 offset32 = *(INT32*)(start+OFFSET_POS);
            S_ADDRX target_saddrx = start + JMP32_LEN + offset32;
            ASSERT(target_saddrx==handler_iter->second);                    
            //modify the trampoline32
            *(INT32*)(start+OFFSET_POS) = cc_used_base - JMP32_LEN - start;
            //patch template
            cc_used_base = patch_sigreturn_ss_template(cc_used_base, _ss_type, _ss_offset, _gs_base, sigreturn_paddrx, target_saddrx);
        }else
            ASSERTM(0, "must have trampoline32\n");
    }
}

void CodeVariantManager::patch_all_sigaction_entry(BOOL is_first_cc)
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        CodeVariantManager *cvm = iter->second;
        for(SIG_HANDLERS::iterator it = _sig_handlers.begin(); it!=_sig_handlers.end(); it++){
            SIG_INFO &sig_info = it->second;
            BOOL &handled = is_first_cc ? sig_info.cv1_handled : sig_info.cv2_handled;
            if(!handled){
                P_ADDRX handler_paddrx = sig_info.sighandler;
                P_ADDRX sigreturn_paddrx = find_cc_paddrx_from_all_orig(sig_info.sigreturn, is_first_cc);
                cvm->patch_sigaction_entry(is_first_cc, handler_paddrx, sigreturn_paddrx);
                handled = true;
            }
        }
    }
}

P_ADDRX CodeVariantManager::handle_sigaction(P_ADDRX orig_sighandler_addr, P_ADDRX orig_sigreturn_addr, P_ADDRX old_pc)
{
    if(sighandler_is_registered(orig_sighandler_addr, orig_sigreturn_addr))
        return old_pc;
    else{
        //wait all ready to make sure patch code cache safely
        wait_for_code_variant_ready(true);
        wait_for_code_variant_ready(false);
        //1. pause gen code variants
        pause_gen_code_variants();
        //2. record sighandler and sigreturn
        SIG_INFO sig_info = {orig_sighandler_addr, orig_sigreturn_addr, false, false};
        _sig_handlers.insert(std::make_pair(orig_sighandler_addr, sig_info));
        //3. patch template into sighandler
        patch_all_sigaction_entry(true);
        patch_all_sigaction_entry(false);
        //4. continue to gen code
        continue_gen_code_variants();
        //5. wait code variants
        wait_for_code_variant_ready(true);
        wait_for_code_variant_ready(false);
        return old_pc;
    }       
}

