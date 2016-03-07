#pragma once

#include <map>
#include <set>

#include "type.h"
#include "utility.h"
#include "relocation.h"
#include "range.h"

//if range is randomBBL, void* is not 0 and 1, if range is jmp8 trampoline, void* is 0, 
//if range is jmp32 tramp, void* is 1; 
#define TRAMP_JMP8_PTR 0
#define TRAMP_JMP32_PTR 1
#define TRAMP_OVERLAP_JMP32_PTR 2
#define INV_TRAMP_PTR 3
#define BOUNDARY_PTR 4
#define RBBL_PTR_MIN 5

typedef std::map<Range<S_ADDRX>, S_ADDRX> CC_LAYOUT;
typedef CC_LAYOUT::iterator CC_LAYOUT_ITER;
typedef std::pair<CC_LAYOUT_ITER, BOOL> CC_LAYOUT_PAIR;

class CodeVariantManager
{
public:
	typedef std::map<F_SIZE, RandomBBL*> RAND_BBL_MAPS;
	typedef std::set<RandomBBL*> RAND_BBL_SET;
	typedef std::set<F_SIZE> TARGET_SET;
	typedef TARGET_SET::iterator TARGET_ITERATOR;
	typedef std::map<F_SIZE, TARGET_SET> JMPIN_TARGETS_MAPS;//first is src_bbl
	typedef JMPIN_TARGETS_MAPS::iterator JMPIN_ITERATOR;
	typedef std::map<std::string, CodeVariantManager*> CVM_MAPS;
	typedef struct{
		P_ADDRX sighandler;
		P_ADDRX sigreturn;
		BOOL cv1_handled;
		BOOL cv2_handled;
	}SIG_INFO;
	typedef std::map<P_ADDRX, SIG_INFO> SIG_HANDLERS;
	typedef struct{
		S_ADDRX ss_base;
		S_SIZE ss_size;
		INT32 ss_fd;
		std::string shm_file;
	}SS_INFO;
	typedef std::map<std::string, SS_INFO> SS_MAPS;
protected:
	RAND_BBL_MAPS _postion_fixed_rbbl_maps;
	RAND_BBL_MAPS _movable_rbbl_maps;
	JMPIN_TARGETS_MAPS _switch_case_jmpin_rbbl_maps;
	std::string _elf_real_name;
	LKM_SS_TYPE _ss_type;
	/********generate code information********/
    CC_LAYOUT _cc_layout1;
	CC_LAYOUT _cc_layout2;
	S_ADDRX _cc1_used_base;
	S_ADDRX _cc2_used_base;
    //store the mapping which maps from binary offset to cc address
    RBBL_CC_MAPS _rbbl_maps1;
	RBBL_CC_MAPS _rbbl_maps2;
    //store the switch-case/memset jmpin offset
    JMPIN_CC_OFFSET _jmpin_rbbl_offsets1;
	JMPIN_CC_OFFSET _jmpin_rbbl_offsets2;
	//static vars
	static BOOL _is_cv1_ready;
	static BOOL _is_cv2_ready;
	/*****************************************/
	//shuffle process information
	//code cache
	S_ADDRX _cc1_base;
	S_ADDRX _cc2_base;
	std::string _cc_shm_path;
	INT32 _cc_fd;
	//protected process information
	//x region and code cache
	P_ADDRX _org_x_load_base;
	P_SIZE _org_x_load_size;
	P_ADDRX _cc_load_base;
	P_SIZE _cc_load_size;
	//main stack and shadow stack
	static P_ADDRX _org_stack_load_base;
	//shadow stack
	static SS_MAPS _ss_maps;
	//static vars
	static CVM_MAPS _all_cvm_maps;
	static std::string _code_variant_img_path;
	static SIZE _cc_offset;
	static SIZE _ss_offset;
	static P_ADDRX _gs_base;
	static BOOL _has_init;
	//signal related
	static SIG_HANDLERS _sig_handlers;
public:
	//get functions
	CodeVariantManager(std::string module_path);
	~CodeVariantManager();
	static BOOL is_added(const std::string elf_path);
	static void recycle();
	static void init_from_db(std::string elf_path, std::string db_path, LKM_SS_TYPE ss_type);
	static RandomBBL *find_rbbl_from_all_paddrx(P_ADDRX p_addr, BOOL is_first_cc);
	static RandomBBL *find_rbbl_from_all_saddrx(S_ADDRX s_addr, BOOL is_first_cc);
	static P_ADDRX find_cc_paddrx_from_all_orig(P_ADDRX orig_p_addrx, BOOL is_first_cc);
	static S_ADDRX find_cc_saddrx_from_all_orig(P_ADDRX orig_p_addrx, BOOL is_first_cc);
	static P_ADDRX find_cc_paddrx_from_all_rbbls(RandomBBL *rbbl, BOOL is_first_cc);
	static void start_gen_code_variants();
	static void stop_gen_code_variants();
	static P_ADDRX get_new_pc_from_old_all(P_ADDRX old_pc, BOOL first_cc_is_new);
	static void modify_new_ra_in_ss(BOOL first_cc_is_new);
	static void init_protected_proc_info(PID protected_pid, SIZE cc_offset, SIZE ss_offset, P_ADDRX gs_base, LKM_SS_TYPE ss_type)
	{
		FATAL(ss_type==LKM_SEG_SS_PP_TYPE, "Current version do not support shadow stack++!\n");
	    for(CVM_MAPS::iterator iter = _all_cvm_maps.begin(); iter!=_all_cvm_maps.end(); iter++)
	        FATAL(iter->second->_ss_type!=ss_type, "LKM shadow stack type is unconsistent with CVM!\n");
		
		_cc_offset = cc_offset;
		_ss_offset = ss_offset;
		_gs_base = gs_base;
		parse_proc_maps(protected_pid);//has already create shadow stack
		init_all_cc();
		_has_init = true;
	}
	static BOOL is_code_variant_ready(BOOL is_first_cc)
	{
		return is_first_cc ? _is_cv1_ready : _is_cv2_ready;
	}
	static void wait_for_code_variant_ready(BOOL is_first_cc);
	static void consume_cv(BOOL is_first_cc);
	static void clear_all_cv(BOOL is_first_cc);
	static void store_into_db(std::string db_path);	
	static void handle_dlopen(P_ADDRX orig_x_base, P_ADDRX orig_x_end, P_SIZE cc_size, std::string db_path, LKM_SS_TYPE ss_type, \
		std::string lib_path, std::string shm_path);
	static P_ADDRX handle_sigaction(P_ADDRX orig_sighandler_addr, P_ADDRX orig_sigreturn_addr, P_ADDRX old_pc);
	//insert functions
	void insert_fixed_random_bbl(F_SIZE bbl_offset, RandomBBL *rand_bbl)
	{
		_postion_fixed_rbbl_maps.insert(std::make_pair(bbl_offset, rand_bbl));
		
	}
	void insert_movable_random_bbl(F_SIZE bbl_offset, RandomBBL *rand_bbl)
	{
		_movable_rbbl_maps.insert(std::make_pair(bbl_offset, rand_bbl));
		
	}
	void insert_switch_case_jmpin_rbbl(F_SIZE src_bbl_offset, F_SIZE target_bbl_offset)
	{
		JMPIN_ITERATOR iter = _switch_case_jmpin_rbbl_maps.find(src_bbl_offset);
		if(iter!=_switch_case_jmpin_rbbl_maps.end()){
			TARGET_SET &targets = iter->second;
			targets.insert(target_bbl_offset);
		}else{
			TARGET_SET targets;
			_switch_case_jmpin_rbbl_maps.insert(std::make_pair(src_bbl_offset, targets));
		}
	}
	void insert_switch_case_jmpin_rbbl(F_SIZE src_bbl_offset, TARGET_SET targets)
	{
		_switch_case_jmpin_rbbl_maps.insert(std::make_pair(src_bbl_offset, targets));
	}
	void set_ss_type(LKM_SS_TYPE ss_type)
	{
		_ss_type = ss_type;
	}
	static void create_ss(P_SIZE ss_size, std::string ss_shm_path);
	static void free_ss(P_SIZE ss_size, std::string ss_shm_path);
protected:	
	void read_db_files(std::string db_path, LKM_SS_TYPE ss_type);
	void patch_sigaction_entry(BOOL is_first_cc, P_ADDRX handler_paddrx, P_ADDRX sigreturn_paddrx);
	static void patch_all_sigaction_entry(BOOL is_first_cc);
	static void add_cvm(CodeVariantManager *cvm)
	{
		_all_cvm_maps.insert(std::make_pair(cvm->get_name(), cvm));
	}
	static void pause_gen_code_variants();
	static void continue_gen_code_variants();
	//set functions
	static void parse_proc_maps(PID protected_pid);
	static void generate_all_code_variant(BOOL is_first_cc);
	static void *generate_code_variant_concurrently(void *arg);
	static BOOL sighandler_is_registered(P_ADDRX orig_sighandler_addr, P_ADDRX orig_sigreturn_addr)
	{
		SIG_HANDLERS::iterator iter = _sig_handlers.find(orig_sighandler_addr);
		if(iter!=_sig_handlers.end()){
			FATAL(orig_sigreturn_addr!=iter->second.sigreturn, "register two different sigreturns: %lx(old) %lx(new)\n", iter->second.sigreturn, orig_sigreturn_addr);
			return true;
		}else
			return false;
	}
	void clear_cv(BOOL is_first_cc);
	void clear_sighandler(BOOL is_first_cc);
	P_ADDRX get_new_pc_from_old(P_ADDRX old_pc, BOOL first_cc_is_new);
	P_ADDRX find_cc_paddrx_from_rbbl(RandomBBL *rbbl, BOOL is_first_cc);
	P_ADDRX find_cc_paddrx_from_orig(P_ADDRX orig_p_addrx, BOOL is_first_cc);
	S_ADDRX find_cc_saddrx_from_orig(P_ADDRX orig_p_addrx, BOOL is_first_cc);
	RandomBBL *find_rbbl_from_paddrx(P_ADDRX p_addr, BOOL is_first_cc);
	RandomBBL *find_rbbl_from_saddrx(S_ADDRX s_addr, BOOL is_first_cc);
	S_ADDRX arrange_cc_layout(S_ADDRX cc_base, CC_LAYOUT &cc_layout, RBBL_CC_MAPS &rbbl_maps, JMPIN_CC_OFFSET &jmpin_rbbl_offsets);
	void generate_code_variant(BOOL is_first_cc);
	void clean_cc(BOOL is_first_cc);
	void relocate_rbbls_and_tramps(CC_LAYOUT &cc_layout, S_ADDRX cc_base, RBBL_CC_MAPS &rbbl_maps, JMPIN_CC_OFFSET &jmpin_rbbl_offsets);
	//init cc and ss
	void init_cc();
	static void init_all_cc();
	void set_x_load_base(P_ADDRX load_base, P_SIZE load_size)
	{
		_org_x_load_base = load_base;
		_org_x_load_size = load_size;
	}
	void set_cc_load_info(P_ADDRX cc_load_base, P_SIZE cc_size, std::string cc_shm_path)
	{
		_cc_load_base = cc_load_base;
		_cc_load_size = cc_size;
		_cc_shm_path = cc_shm_path;
		ASSERT(_cc_load_base==(_org_x_load_base+_cc_offset));
	}
	static void set_stack_load_base(P_ADDRX stack_load_base)
	{
		_org_stack_load_base = stack_load_base;
	}
	//get functions
	std::string get_name()
	{
		return _elf_real_name;
	}
};

