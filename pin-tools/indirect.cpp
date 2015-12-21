/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2014 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#include <iostream>
#include <fstream>
#include "pin.H"
#include <vector>
#include <iomanip>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <map>
#include <set>
#include <cassert>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define COLOR_RED "\033[01;31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_HIGH_GREEN "\033[01;32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[01;36m"
#define COLOR_END "\033[0m"

char application_name[1000];

/* 
    Shadow Stack 
*/

class SS{
    // macros and typedef
    #define THREAD_SUM 128
    #define SS_SIZE 0x1000000 // 16M 
    typedef struct{
        ADDRINT stack_base;
        ADDRINT ss_base;
        UINT8 * shadow_stack_array;
    }ShadowStack;
 
protected:
    ShadowStack _ss[THREAD_SUM];
    
public:
    SS(){
        for(UINT32 idx=0; idx<THREAD_SUM; idx++){
            _ss[idx].stack_base = 0x0ull;
            _ss[idx].ss_base = 0x0ull;
            _ss[idx].shadow_stack_array = NULL;
        }
    }
    
    void new_ss(THREADID id, ADDRINT rsp){
        assert(id<THREAD_SUM);
        _ss[id].stack_base = (rsp&~0xfffull)+0x1000;
        if(!_ss[id].shadow_stack_array){
            assert(_ss[id].ss_base==0);
            _ss[id].shadow_stack_array = new UINT8[SS_SIZE];
            _ss[id].ss_base = (ADDRINT)_ss[id].shadow_stack_array + (ADDRINT)SS_SIZE;
        }
    }
    
    void delete_ss(THREADID id){
        assert(id<THREAD_SUM);
        _ss[id].stack_base = 0x0ull;
    }

    ADDRINT get_ss_rsp(THREADID id, ADDRINT rsp){
        assert((id<THREAD_SUM) && (_ss[id].stack_base!=0) && _ss[id].shadow_stack_array && (_ss[id].ss_base!=0));
        ADDRINT off = _ss[id].stack_base - rsp;
        assert(off<SS_SIZE);
        return _ss[id].ss_base - off;
    }
    
    ADDRINT get_ss_value(THREADID id, ADDRINT rsp){
        ADDRINT ss_rsp = get_ss_rsp(id, rsp);
        return *(ADDRINT*)ss_rsp;
    }  
    
    void push(THREADID id, ADDRINT rsp, ADDRINT value){
        ADDRINT ss_rsp = get_ss_rsp(id, rsp);
        *(ADDRINT *)ss_rsp = value;
    }
    
    BOOL check(THREADID id, ADDRINT rsp, ADDRINT value){
        return get_ss_value(id, rsp) == value;
    }

    ~SS(){
        for(UINT32 idx=0; idx<THREAD_SUM; idx++){
            if(_ss[idx].shadow_stack_array){
                assert(_ss[idx].ss_base!=0);
                delete _ss[idx].shadow_stack_array;
            }else
                assert(_ss[idx].ss_base==0);
        }        
    }
};

SS thread_ss;
set<UINT64> ss_unmatched;

/*
    Hash Function to record Indirect Branch instructions' inst_addr and target_address
*/
enum INDIRECT_BRANCH{
    INDIRECT_CALL = 0,
    INDIRECT_JUMP,
    RET,
    INDIRECT_BRANCH_SUM,    
};

const string indirect_branch_type_to_name[INDIRECT_BRANCH_SUM]={"IndirectCall", "IndirectJump", "Ret"};
set<UINT64> indirect_inst_set[INDIRECT_BRANCH_SUM]; 

UINT64 hash(ADDRINT src_addr, UINT32 src_img_id, ADDRINT target_addr, UINT32 target_img_id){
	#define ADDR_MASK 0xffffffull
	#define ID_MASK 0xffull
	#define CAN_HASH(addr, mask) (((~mask)&addr)==0ull)
	assert(CAN_HASH(src_addr, ADDR_MASK) && "src_addr can not hash\n");
	assert(CAN_HASH(target_addr, ADDR_MASK) && "target_addr can not hash\n");
	assert(CAN_HASH(src_img_id, ID_MASK) && "src image id can not hash\n");
	assert(CAN_HASH(target_img_id, ID_MASK) && "target img id can not hash\n");
	return  (((src_addr&ADDR_MASK)|((src_img_id&ID_MASK)<<24))<<32)|(target_addr&ADDR_MASK)|((target_img_id&ID_MASK)<<24);
}

void parse_hash_value(UINT64 hash_value, ADDRINT &src_addr, UINT32 &src_img_id, ADDRINT &target_addr, UINT32 &target_img_id){
	#define ADDR_MASK 0xffffffull
	#define ID_MASK 0xffull
	#define CAN_HASH(addr, mask) (((~mask)&addr)==0ull)
	src_addr = (hash_value>>32)&ADDR_MASK;
	target_addr = hash_value&ADDR_MASK;
	src_img_id = (hash_value>>56)&ID_MASK;
	target_img_id = (hash_value>>24)&ID_MASK;
}

/*
    Image Record
*/
typedef struct image_item{
	string image_name;
	IMG_TYPE type;
	UINT32 valid;
}IMAG_ITEM;
vector<IMAG_ITEM> image_vec;

map<ADDRINT, UINT32> img_map;
typedef map<ADDRINT, UINT32>::iterator img_map_iter;

static inline UINT32 find_img_id(IMG img){
	img_map_iter it = img_map.find(IMG_LowAddress(img));
	assert(it!=img_map.end());
	UINT32 ret = it->second;
	assert(image_vec[ret].valid==1);
	return ret+1;
}

VOID ImageUnload(IMG img, VOID *v)
{
	//cout << "Unload " << IMG_Name(img)<<" "<<IMG_Id(img) << ", Image addr=0x" <<hex<< IMG_LowAddress(img) <<"-0x"<<hex<< IMG_HighAddress (img)<<endl;
	for(UINT32 idx=0; idx!=image_vec.size(); idx++){
		IMAG_ITEM &exist_image = image_vec[idx];
		if((exist_image.image_name==IMG_Name(img)) && (exist_image.type==IMG_Type(img))){
			assert(exist_image.valid==1);
			exist_image.valid = 0;
			img_map.erase(IMG_LowAddress(img));
			return ;
		}
	}
	assert(0);
}

VOID ImageLoad(IMG img, VOID *v)
{
	//cout << "Loading " << IMG_Name(img)<<" "<<IMG_Id(img) << ", Image addr=0x" <<hex<< IMG_LowAddress(img) <<"-0x"<<hex<< IMG_HighAddress (img)<<endl;
 	IMAG_ITEM new_item = {IMG_Name(img), IMG_Type(img), 1};
	for(UINT32 idx=0; idx!=image_vec.size(); idx++){
		IMAG_ITEM &exist_image = image_vec[idx];
		if((exist_image.image_name==new_item.image_name) && (exist_image.type==new_item.type)){
			assert(exist_image.valid==0);
			exist_image.valid = 1;
			img_map.insert(make_pair(IMG_LowAddress(img), idx));
			return ;
		}
	}
	image_vec.push_back(new_item);
	img_map.insert(make_pair(IMG_LowAddress(img), image_vec.size()-1));
}

/*
    Record indirect branch instructions' inst_addr and target address
*/
inline VOID record_target(INDIRECT_BRANCH type, ADDRINT inst_address, ADDRINT inst_target)
{
	IMG src_image = IMG_FindByAddress(inst_address);
	IMG target_image = IMG_FindByAddress(inst_target);
	//fprintf(stderr, "0x%lx->0x%lx\n", inst_address, inst_target);
	if(IMG_Valid(src_image) && IMG_Valid(target_image)){
		UINT32 src_id = find_img_id(src_image);
		UINT32 target_id = find_img_id(target_image);
		ADDRINT src_addr = inst_address - IMG_LowAddress(src_image);
		ADDRINT target_addr = inst_target - IMG_LowAddress(target_image);
		indirect_inst_set[type].insert(hash(src_addr, src_id, target_addr, target_id));
	}
}

VOID record_indirect_call_target(THREADID thread_id, ADDRINT fallthrough_address, ADDRINT rsp, ADDRINT inst_address, ADDRINT inst_target)
{
	PIN_LockClient();
    record_target(INDIRECT_CALL, inst_address, inst_target);
    thread_ss.push(thread_id, rsp-8, fallthrough_address);
	PIN_UnlockClient();
}
VOID record_direct_call_target(THREADID thread_id, ADDRINT fallthrough_address, ADDRINT rsp)
{
	PIN_LockClient();
    thread_ss.push(thread_id, rsp-8, fallthrough_address);
	PIN_UnlockClient();
}
VOID record_indirect_jump_target(ADDRINT inst_address, ADDRINT inst_target)
{
	PIN_LockClient();
    record_target(INDIRECT_JUMP, inst_address, inst_target);
	PIN_UnlockClient();
}
VOID record_ret_target(THREADID thread_id, ADDRINT rsp, ADDRINT inst_address, ADDRINT inst_target)
{
	PIN_LockClient();
    record_target(RET, inst_address, inst_target);
    
    if(!thread_ss.check(thread_id, rsp, inst_target)){        
        IMG src_image = IMG_FindByAddress(inst_address);
        IMG target_image = IMG_FindByAddress(inst_target);
        if(IMG_Valid(src_image) && IMG_Valid(target_image)){
            ADDRINT src_addr = inst_address - IMG_LowAddress(src_image);
            ADDRINT target_addr = inst_target - IMG_LowAddress(target_image);
            UINT32 src_id = find_img_id(src_image);
            UINT32 target_id = find_img_id(target_image);
            ss_unmatched.insert(hash(src_addr, src_id, target_addr, target_id));
            fprintf(stderr, COLOR_RED"[ERROR] Detected shadow stack unmatch: 0x%lx(%s) ==> 0x%lx(%s)\n"COLOR_END, \
                   src_addr, IMG_Name(src_image).c_str(), target_addr, IMG_Name(target_image).c_str());
        }
    }
	PIN_UnlockClient();
}


// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
    // record call instructions
    if(INS_IsCall(ins)){
        ADDRINT next_addr = INS_NextAddress(ins);
        if(INS_IsDirectCall(ins)){
            // direct call
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)record_direct_call_target, IARG_THREAD_ID, IARG_ADDRINT, next_addr,\
                IARG_REG_VALUE, REG_STACK_PTR, IARG_END);
        }else{
            // indirect call
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)record_indirect_call_target, IARG_THREAD_ID, IARG_ADDRINT, next_addr, \
                IARG_REG_VALUE, REG_STACK_PTR, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_END);
        }
    }else if(INS_IsIndirectBranchOrCall(ins)){
        if(INS_IsRet(ins)){
            // ret
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)record_ret_target, IARG_THREAD_ID, IARG_REG_VALUE, REG_STACK_PTR, \
                IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_END);
        }else{
            // indirect jump
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)record_indirect_jump_target, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_END);
        }
    }
}

/*
    Instrument thread start and fini
*/
PIN_LOCK lock;

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PIN_GetLock(&lock, threadid+1);
    thread_ss.new_ss(threadid, PIN_GetContextReg(ctxt, REG_STACK_PTR));
    PIN_ReleaseLock(&lock);
}

VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 flags, VOID *v)
{
    PIN_GetLock(&lock, threadid+1);
    thread_ss.delete_ss(threadid);
    PIN_ReleaseLock(&lock);
}


KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "indirect.out", "specify output file name");

ofstream OutFile;

static string get_real_path(const char *file_path)
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
            if(ret<=0)
                cerr<<"readlink error!"<<endl;

            INCREASE_IDX;
        }else
            break;
    }
    
    return string(path[CURR_IDX]); 
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{	
	// Write to a file since cout and cerr maybe closed by the application
	OutFile.setf(ios::hex, ios::basefield);
	//OutFile.setf(ios::showbase);
	OutFile<<"IMG_NUM= "<<image_vec.size()<<endl;
	for(UINT32 idx=0; idx != image_vec.size(); idx++){
		IMAG_ITEM &item = image_vec[idx];
        string real_path = get_real_path(item.image_name.c_str());
        UINT32 found = real_path.find_last_of("/");
        string image_name;
        if(found==string::npos)
            image_name = real_path;
        else
            image_name = real_path.substr(found+1);
        OutFile<<setw(2)<<idx<<" "<<image_name<<" "<<endl;
	}
    for(UINT32 idx=0; idx != INDIRECT_BRANCH_SUM; idx++){
    	OutFile<<indirect_branch_type_to_name[idx]<<"_NUM= "<<indirect_inst_set[idx].size()<<endl;
    	for(set<UINT64>::iterator iter = indirect_inst_set[idx].begin(); iter!=indirect_inst_set[idx].end(); iter++){
    		ADDRINT src_addr = 0;
    		ADDRINT target_addr = 0;
    		UINT32 src_id = 0;
    		UINT32 target_id = 0;
    		parse_hash_value(*iter, src_addr, src_id, target_addr, target_id);
    		OutFile<<src_addr<<" ( "<<(src_id-1)<<" )--> "<<target_addr<<" ( "<<(target_id-1)<<" )"<<endl;
    	}
    }
    OutFile<<"ShadowStackUnmatchedNum= "<<ss_unmatched.size()<<endl;
    for(set<UINT64>::iterator iter = ss_unmatched.begin(); iter!=ss_unmatched.end(); iter++){
        ADDRINT src_addr = 0;
        ADDRINT target_addr = 0;
        UINT32 src_id = 0;
        UINT32 target_id = 0;
        parse_hash_value(*iter, src_addr, src_id, target_addr, target_id);
        OutFile<<src_addr<<" ( "<<(src_id-1)<<" )--> "<<target_addr<<" ( "<<(target_id-1)<<" )"<<endl;
    }
	OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
	// Initialize pin
	if (PIN_Init(argc, argv)) return Usage();

	char *path_end = strrchr(argv[12],  '/');
	if(!path_end)
		path_end = argv[12];
	else
		path_end++;
	
	sprintf(application_name, "/home/wangzhe/pprofile/%s.indirect.log", path_end);
	OutFile.open(application_name);
	
	// Register Instruction to be called to instrument instructions
	INS_AddInstrumentFunction(Instruction, 0);

	// Register Fini to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);

	// find all image and it's range
	IMG_AddInstrumentFunction(ImageLoad, 0);
	IMG_AddUnloadFunction(ImageUnload, 0);

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
	// Start the program, never returns
	PIN_StartProgram();

	return 0;
}
