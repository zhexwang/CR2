#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "code_variant_manager.h"
#include "utility.h"

CodeVariantManager::CVM_MAPS CodeVariantManager::_all_cvm_maps;
std::string CodeVariantManager::_code_variant_img_path;
SIZE CodeVariantManager::_cc_offset = 0;
SIZE CodeVariantManager::_ss_offset = 0;
pid_t CodeVariantManager::_protected_proc_pid = 0;

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

CodeVariantManager::CodeVariantManager(std::string module_path, SIZE cc_size)
    :  _cc_size(cc_size)
{
    _elf_real_name = get_real_name_from_path(get_real_path(module_path.c_str()));
    add_cvm(this);
}

CodeVariantManager::~CodeVariantManager()
{
    ;
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

extern UINT8 CodeCacheSizeMulriple;
void CodeVariantManager::set_all_real_load_base_to_cvms()
{
    //1.open maps file
    char maps_path[100];
    sprintf(maps_path, "/proc/%d/maps", _protected_proc_pid);
    FILE *maps_file = fopen(maps_path, "r"); 
    ASSERT(maps_file!=NULL);
    //2.read maps file, line by line
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

        if(is_executable(currentRow) && !strstr(currentRow->pathname, "[vdso]") && !strstr(currentRow->pathname, "[vsyscall]")){
            std::string maps_record_name = get_real_name_from_path(get_real_path(currentRow->pathname));
            CVM_MAPS::iterator iter = _all_cvm_maps.find(maps_record_name);
            ASSERT(iter!=_all_cvm_maps.end());
            iter->second->set_load_base(currentRow->start);
            ASSERT((currentRow->end - currentRow->start)*CodeCacheSizeMulriple == iter->second->get_cc_size());
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
    
void CodeVariantManager::init_code_variant_image(std::string variant_img_path)
{
    // 1. set shuffle image path 
    _code_variant_img_path = variant_img_path;
    // 2. new the shuffle image file
    
}

