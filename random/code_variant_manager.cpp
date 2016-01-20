#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "code_variant_manager.h"
#include "utility.h"

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

void CodeVariantManager::generate_code_variant(BOOL is_first_cc)
{
    std::map<S_ADDRX, RandomBBL*> rbbl_maps;
    std::map<S_ADDRX, RandomBBL*> trampoline_maps;
    std::map<F_SIZE, P_SIZE> jmpin_offsets;
    S_ADDRX cc_base = is_first_cc ? _cc1_base : _cc2_base;
    S_ADDRX curr_trampoline_addr = 0;
    for(RAND_BBL_MAPS::iterator iter = _postion_fixed_rbbl_maps.begin(); iter!=_postion_fixed_rbbl_maps.end(); iter++){
        F_SIZE curr_bbl_offset = iter->first;
        F_SIZE next_bbl_offset = (++iter)!=_postion_fixed_rbbl_maps.end() ? iter->first : curr_bbl_offset;
        iter--;
        //can place trampoline
        if((next_bbl_offset-curr_bbl_offset)>=0x5){
            curr_trampoline_addr = curr_bbl_offset + cc_base;
            trampoline_maps.insert(std::make_pair(curr_trampoline_addr, iter->second));
        }else{
            ;//ASSERTM(0, "trampoline place error!\n");
            if((next_bbl_offset-curr_bbl_offset)!=0){
                if((next_bbl_offset-curr_bbl_offset)<2)
                    ERR("MIN_SIZE=1 : %lx (%s)\n", curr_bbl_offset, _elf_real_name.c_str());
            }
        }
    }
    
    S_ADDRX rbbl_place_base = curr_trampoline_addr + 0x5;

    for(RAND_BBL_MAPS::iterator iter = _postion_fixed_rbbl_maps.begin(); iter!=_postion_fixed_rbbl_maps.end(); iter++){
        S_SIZE rbbl_size = iter->second->get_template_size();
        rbbl_maps.insert(std::make_pair(rbbl_place_base, iter->second));
        rbbl_place_base += rbbl_size;
    }

    for(RAND_BBL_MAPS::iterator iter = _movable_rbbl_maps.begin(); iter!=_movable_rbbl_maps.end(); iter++){
        S_SIZE rbbl_size = iter->second->get_template_size();
        rbbl_maps.insert(std::make_pair(rbbl_place_base, iter->second));
        rbbl_place_base += rbbl_size;
    }

    TARGET_SET merge_set;

    for(JMPIN_ITERATOR iter = _jmpin_rbbl_maps.begin(); iter!= _jmpin_rbbl_maps.end(); iter++){
        F_SIZE jmpin_offset = iter->first;
        P_SIZE jmpin_target_offset = _cc_offset + rbbl_place_base - cc_base;
        jmpin_offsets.insert(std::make_pair(jmpin_offset, jmpin_target_offset));
        merge_set.insert(iter->second.begin(), iter->second.end());
    }
    
    for(TARGET_ITERATOR it = merge_set.begin(); it!=merge_set.end(); it++){
        F_SIZE curr_bbl_offset = *it;
        F_SIZE next_bbl_offset = (++it)!=merge_set.end() ? *it : curr_bbl_offset;
        it--;
        //can place trampoline
        if((next_bbl_offset-curr_bbl_offset)>=0x5){
            curr_trampoline_addr = curr_bbl_offset + rbbl_place_base;
            RAND_BBL_MAPS::iterator ret = _postion_fixed_rbbl_maps.find(curr_bbl_offset);
            RandomBBL *target_rbbl = NULL;
            
            if(ret==_postion_fixed_rbbl_maps.end()){
                ret = _movable_rbbl_maps.find(curr_bbl_offset);
                ASSERT(ret!=_movable_rbbl_maps.end());
            }

            target_rbbl = ret->second;
            trampoline_maps.insert(std::make_pair(curr_trampoline_addr, target_rbbl));
        }else{
            ;//ASSERTM(0, "trampoline place error!\n");
            if((next_bbl_offset-curr_bbl_offset)!=0){
                if((next_bbl_offset-curr_bbl_offset)<2)
                    ERR("MIN_SIZE=1 : %lx (%s)\n", curr_bbl_offset, _elf_real_name.c_str());
            }
        }
    }
    //judge used cc size
    ASSERT((curr_trampoline_addr + 0x5 - cc_base)<=_cc_load_size);
    return ;
}

void CodeVariantManager::generate_all_code_variant()
{
    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++){
        iter->second->generate_code_variant(true);
        iter->second->generate_code_variant(false);
    }
    
    return ;
}
