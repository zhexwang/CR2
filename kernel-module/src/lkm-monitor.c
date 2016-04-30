#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/mman.h>
#include <linux/spinlock_types.h>

#include "lkm-config.h"
#include "lkm-utility.h"
#include "lkm-file.h"
#include "lkm-hook.h"
#include "lkm-netlink.h"
#include "lkm-monitor.h"

char* monitor_app_list[MAX_APP_LIST_NUM];

//line index is MAX_APP_LIST_NUM
typedef struct{
	char app_slot_idx;
	int shuffle_pid;
}S_CONFIG;

S_CONFIG shuffle_config_list[MAX_APP_LIST_NUM][MAX_SHUFFLE_NUM_FOR_ONE_APP];

spinlock_t shuffle_config_lock;
spinlock_t app_slot_lock;

void lock_init(void)
{
	spin_lock_init(&shuffle_config_lock);
	spin_lock_init(&app_slot_lock);
}

void init_shuffle_config_list(void)
{
	int index, index_app;
	for(index_app = 0; index_app < MAX_APP_LIST_NUM; index_app++)
		for(index = 0; index < MAX_SHUFFLE_NUM_FOR_ONE_APP; index++){
			shuffle_config_list[index_app][index].shuffle_pid = 0;
			shuffle_config_list[index_app][index].app_slot_idx = -1;
		}
}

void insert_shuffle_info(char monitor_list_idx, int shuffle_pid)
{
	int index;
	spin_lock(&shuffle_config_lock);
	for(index = 0; index < MAX_SHUFFLE_NUM_FOR_ONE_APP; index++){
		//find a free
		if(shuffle_config_list[(int)monitor_list_idx][index].shuffle_pid==0){
			shuffle_config_list[(int)monitor_list_idx][index].shuffle_pid = shuffle_pid;
			spin_unlock(&shuffle_config_lock);
			return ;
		}
	}
	PRINTK("find no free shuffle config list %s\n", __FUNCTION__);
	spin_unlock(&shuffle_config_lock);
}

void free_one_shuffle_info(char monitor_list_idx, int shuffle_pid)
{
	int index;
	spin_lock(&shuffle_config_lock);
	for(index = 0; index < MAX_SHUFFLE_NUM_FOR_ONE_APP; index++){
		//find a free
		if(shuffle_config_list[(int)monitor_list_idx][index].shuffle_pid==shuffle_pid){
			shuffle_config_list[(int)monitor_list_idx][index].shuffle_pid = 0;
			shuffle_config_list[(int)monitor_list_idx][index].app_slot_idx = -1;
			spin_unlock(&shuffle_config_lock);
			return ;
		}
	}
	PRINTK("find no free shuffle config list %s\n", __FUNCTION__);
	spin_unlock(&shuffle_config_lock);
}

int connect_one_shuffle(char monitor_list_idx, char app_slot_idx)
{
	int index;

	spin_lock(&shuffle_config_lock);
	for(index = 0; index < MAX_SHUFFLE_NUM_FOR_ONE_APP; index++){
		//find a free
		if(shuffle_config_list[(int)monitor_list_idx][index].app_slot_idx==-1 && \
			shuffle_config_list[(int)monitor_list_idx][index].shuffle_pid!=0){
			shuffle_config_list[(int)monitor_list_idx][index].app_slot_idx = app_slot_idx;
			spin_unlock(&shuffle_config_lock);
			return shuffle_config_list[(int)monitor_list_idx][index].shuffle_pid;
		}
	}
	PRINTK("find no free shuffle config list %s\n", __FUNCTION__);
	spin_unlock(&shuffle_config_lock);
	
	return 0;
}

int has_free_shuffle(char monitor_list_idx, char app_slot_idx)
{
	int index;

	spin_lock(&shuffle_config_lock);
	for(index = 0; index < MAX_SHUFFLE_NUM_FOR_ONE_APP; index++){
		//find a free
		if(shuffle_config_list[(int)monitor_list_idx][index].app_slot_idx==-1 && \
			shuffle_config_list[(int)monitor_list_idx][index].shuffle_pid!=0){
			spin_unlock(&shuffle_config_lock);
			return 1;
		}
	}
	spin_unlock(&shuffle_config_lock);
	return 0;
}

char get_app_slot_idx_from_shuffle_config(char monitor_list_idx, int shuffle_pid)
{
	int index;
	spin_lock(&shuffle_config_lock);
	for(index = 0; index < MAX_SHUFFLE_NUM_FOR_ONE_APP; index++){
		if(shuffle_config_list[(int)monitor_list_idx][index].shuffle_pid==shuffle_pid){			
			spin_unlock(&shuffle_config_lock);
			return shuffle_config_list[(int)monitor_list_idx][index].app_slot_idx;
		}
	}
	PRINTK("find no free shuffle config list %s\n", __FUNCTION__);
	spin_unlock(&shuffle_config_lock);
	return -1;
}

/**
 * checking if the intercepted app is the one we want to monitor
 **/
char is_monitor_app(const char *name)
{
	ulong ret = 0, i = 0;
	for(i = 1; i < MAX_APP_LIST_NUM; i++){
		 /* search the application name in the monitor list
		 */
		if(monitor_app_list[i] == NULL) break;
		// if orig name is too long (the struct->comm's len is 16)
		// thus, we can not use strcmp
		if(!strcmp(name, monitor_app_list[i])){
			ret = i; break;
		}
		if(strlen(name) >= 15 && !strncmp(name, monitor_app_list[i], strlen(name))){
			ret = i; break;
		}
	}
	return ret;
}

void init_monitor_app_list(void)
{
	lock_init();
	monitor_app_list[0] = "reserved.cr2";
	//----SPEC CPU2006 INT------//
	//c language
	monitor_app_list[1] = "perlbench_base.cr2";//400
	monitor_app_list[2] = "bzip2_base.cr2";//401
	monitor_app_list[3] = "gcc_base.cr2";//403
	monitor_app_list[4] = "mcf_base.cr2";//429
	monitor_app_list[5] = "gobmk_base.cr2";//445
	monitor_app_list[6] = "hmmer_base.cr2";//456
	monitor_app_list[7] = "sjeng_base.cr2";//458
	monitor_app_list[8] = "libquantum_base.cr2";//462
	monitor_app_list[9] = "h264ref_base.cr2";//464
	monitor_app_list[10] = "specrand_base.cr2";//999--998
	//c++ language
	monitor_app_list[11] = "omnetpp_base.cr2";//471
	monitor_app_list[12] = "astar_base.cr2";//473
	monitor_app_list[13] = "Xalan_base.cr2";//483
	//----SPEC CPU2006 FP------//
	//fortran language
	monitor_app_list[14] = "bwaves_base.cr2";//410
	monitor_app_list[15] = "gamess_base.cr2";//416
	monitor_app_list[16] = "zeusmp_base.cr2";//434
	monitor_app_list[17] = "cactusADM_base.cr2";//436
	monitor_app_list[18] = "leslie3d_base.cr2";//437
	monitor_app_list[19] = "calculix_base.cr2";//454
	monitor_app_list[20] = "GemsFDTD_base.cr2";//459
	monitor_app_list[21] = "tonto_base.cr2";//465
	monitor_app_list[22] = "wrf_base.cr2";//481
	//c language
	monitor_app_list[23] = "milc_base.cr2";//433
	monitor_app_list[24] = "gromacs_base.cr2";//435
	monitor_app_list[25] = "lbm_base.cr2";//470
	monitor_app_list[26] = "sphinx_livepretend_base.cr2";//482
	//c++ language
	monitor_app_list[27] = "namd_base.cr2";//444
	monitor_app_list[28] = "soplex_base.cr2";//450
	monitor_app_list[29] = "dealII_base.cr2";//447
	monitor_app_list[30] = "povray_base.cr2";//453
	//-------------------nginx-----------------------//
	monitor_app_list[31] = "nginx";
	//--------------parsec benchmarks----------------//
	//apps
	monitor_app_list[32] = "blackscholes";
	monitor_app_list[33] = "bodytrack";
	monitor_app_list[34] = "facesim";
	monitor_app_list[35] = "ferret";
	monitor_app_list[36] = "fluidanimate";
	monitor_app_list[37] = "freqmine";
	monitor_app_list[38] = "rtview";
	monitor_app_list[39] = "swaptions";
	monitor_app_list[40] = "vips";//unable to map in _VM
	monitor_app_list[41] = "x264";
	//kernel
	monitor_app_list[42] = "canneal";
	monitor_app_list[43] = "dedup";
	monitor_app_list[44] = "streamcluster";
	//---------------------Apache--------------------//
	monitor_app_list[45] = "httpd";//we only support multi-thread mode now, you must use ./httpd -X
	//---------------------test----------------------//
	monitor_app_list[46] = "a.out";
}

//-------------APP slot----------------------//

typedef struct{
	long cc_start;
	long cc_end;
	char shfile[256];
}X_REGION;

typedef struct{
	long ss_region_start;
	long ss_region_end;
	int belong_pid;
	char shfile[256];
}STACK_REGION;

typedef struct{
	int pid;
	volatile char start_flag;
}START_FLAG;

typedef struct{
	int pgid[MAX_PGID_NUM];//setsid
	int shuffle_pid;
	int process_sum;
	int ss_number;
	int cc_id;//current cc used index
	ulong pc;//send by shuffle process
	//volatile char start_flag;
	START_FLAG start_flags[MAX_FLAG_NUM];
	long program_entry;
	char executed_start;
	char app_name[256];
	char start_instr_encode[7];
	char has_rerandomization;
	ulong rerand_times;
	size_t accumulated_output_data_size; 
	char has_send_stop;
	int stopped_pid[MAX_STOP_NUM];//multi-threads and multi-processes need to stop all processes/threads, when rerandomization
	char has_output_syscall;
	X_REGION xr[MAX_X_NUM];
	STACK_REGION sr[MAX_STACK_NUM];	
}APP_SLOT;

APP_SLOT app_slot_list[MAX_APP_SLOT_LIST_NUM];

int is_pgid_matched(char app_slot_idx, int pgid)
{
	int index = 0;
	for(index=0; index<MAX_PGID_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].pgid[index]==pgid)
			return 1;
	}
	return 0;
}

int is_pgid_empty(char app_slot_idx)
{
	int index = 0;
	for(index=0; index<MAX_PGID_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].pgid[index]!=0)
			return 0;
	}
	return 1;
}

void add_a_new_pgid(char app_slot_idx, int pgid)
{
	int index = 0;
	for(index=0; index<MAX_PGID_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].pgid[index]==0){
			app_slot_list[(int)app_slot_idx].pgid[index] = pgid;
			return ;
		}
	}
}

int get_cc_id(struct task_struct *ts)
{
	int index = 0;
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts))))
			return app_slot_list[index].cc_id;
	}
	return -1;
}

int get_process_sum(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			spin_unlock(&app_slot_lock);
			return app_slot_list[index].process_sum;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
	return 0;
}

void create_new_sid(struct task_struct *ts, int old_pgid)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, old_pgid)){
			add_a_new_pgid(index, pid_vnr(task_pgrp(ts)));
			spin_unlock(&app_slot_lock);
			return ;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
	return ;
}

int increase_process_sum(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].process_sum++;
			spin_unlock(&app_slot_lock);
			return app_slot_list[index].process_sum;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
	return -1;
}

int decrease_process_sum(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].process_sum--;
			spin_unlock(&app_slot_lock);
			return app_slot_list[index].process_sum;
		}
	}
	PRINTK("[%d, %d]find no app slot in %s\n", ts->pid, pid_vnr(task_pgrp(ts)), __FUNCTION__);
	spin_unlock(&app_slot_lock);
	return -1;
}

char init_app_slot(struct task_struct *ts)
{
	int index = 0;
	PRINTK("execve %s\n", ts->comm);
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_empty(index)){
			add_a_new_pgid(index, pid_vnr(task_pgrp(ts)));
			app_slot_list[index].shuffle_pid = 0;
			app_slot_list[index].ss_number = 0;
			app_slot_list[index].process_sum = 1;
			app_slot_list[index].cc_id = 0;
			app_slot_list[index].pc = 0;
			//app_slot_list[index].start_flag = 0;
			app_slot_list[index].executed_start = 0;
			app_slot_list[index].program_entry = 0;
			app_slot_list[index].has_rerandomization = 0;
			app_slot_list[index].has_send_stop = 0;
			app_slot_list[index].rerand_times = 0;
		 	app_slot_list[index].accumulated_output_data_size = 0; 
			app_slot_list[index].has_output_syscall = 0;
			strcpy(app_slot_list[index].app_name, ts->comm);
			memset((void*)&(app_slot_list[index].start_flags[0]), 0, MAX_FLAG_NUM*sizeof(START_FLAG));
			memset((void*)&(app_slot_list[index].stopped_pid[0]), 0, MAX_STOP_NUM*sizeof(int));
			memset((void*)&(app_slot_list[index].start_instr_encode), 11, sizeof(char));
			memset((void*)&(app_slot_list[index].xr[0]), 0, MAX_X_NUM*sizeof(X_REGION));
			memset((void*)&(app_slot_list[index].sr[0]), 0, MAX_STACK_NUM*sizeof(STACK_REGION));
			return index;
		}
	}
	return -1;
}
void free_app_slot(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	//find the slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			PRINTK("%s has rerand %ld times!\n", ts->comm, app_slot_list[index].rerand_times);
			memset((void*)(&app_slot_list[index]), 0, sizeof(APP_SLOT));
			spin_unlock(&app_slot_lock);
			return ;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
}

void clear_io_out(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	//
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].has_output_syscall = 0; 
			spin_unlock(&app_slot_lock); 
			return ;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
}

void set_io_out(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	// 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].has_output_syscall = 1; 
			spin_unlock(&app_slot_lock); 
			return ;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
}

char has_io_out(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	// 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			spin_unlock(&app_slot_lock); 
			return app_slot_list[index].has_output_syscall;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return 0;
}

char need_rerandomization(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	// 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			spin_unlock(&app_slot_lock); 
			return (app_slot_list[index].has_output_syscall==1) && (app_slot_list[index].accumulated_output_data_size>=TRANSFER_THRESHOLD);
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return 0;
}

void set_output_syscall(struct task_struct *ts, size_t len)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].has_output_syscall = 1; 
			app_slot_list[index].accumulated_output_data_size += len;
			spin_unlock(&app_slot_lock); 
			return ;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
}

void clear_rerand_record(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].accumulated_output_data_size = 0; 
			app_slot_list[index].has_output_syscall = 0; 
			spin_unlock(&app_slot_lock); 
			return ;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
}

char has_enough_data_size(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	// 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			spin_unlock(&app_slot_lock); 
			return app_slot_list[index].accumulated_output_data_size>=TRANSFER_THRESHOLD;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return 0;
}

void increase_data_size(struct task_struct *ts, size_t len)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].accumulated_output_data_size += len; 
			spin_unlock(&app_slot_lock); 
			return ;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
}

void clear_data_size(struct task_struct *ts)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].accumulated_output_data_size = 0; 
			spin_unlock(&app_slot_lock); 
			return ;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
}

char *get_start_encode_and_set_entry(struct task_struct *ts, long program_entry)
{
	int index = 0;
	spin_lock(&app_slot_lock); 
	// 1.find the  encode
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].program_entry = program_entry;
			spin_unlock(&app_slot_lock); 
			return app_slot_list[index].start_instr_encode;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return NULL;
}

char *is_checkpoint(struct task_struct *ts, char *app_slot_idx)
{
	int index = 0;
	long curr_rip = task_pt_regs(ts)->ip;
	spin_lock(&app_slot_lock); 
	// 1.find the  encode
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			*app_slot_idx = index;
			if(app_slot_list[index].program_entry == (curr_rip-CHECK_ENCODE_LEN)){
				spin_unlock(&app_slot_lock); 
				return app_slot_list[index].start_instr_encode;
			}else{
				spin_unlock(&app_slot_lock); 
				return NULL;
			}
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	*app_slot_idx = -1;
	spin_unlock(&app_slot_lock); 
	return NULL;
}

char get_app_slot_idx(int pgid)
{
	int index = 0;
	spin_lock(&app_slot_lock); 
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++)
		if(is_pgid_matched(index, pgid)){
			spin_unlock(&app_slot_lock); 
			return index;
		}
	spin_unlock(&app_slot_lock); 		
	return -1;
}

void set_shuffle_pid(char app_slot_idx, int shuffle_pid)
{
	spin_lock(&app_slot_lock); 
	app_slot_list[(int)app_slot_idx].shuffle_pid = shuffle_pid;
	spin_unlock(&app_slot_lock); 
}

void set_shuffle_pc(char app_slot_idx, ulong pc)
{
	spin_lock(&app_slot_lock); 
	app_slot_list[(int)app_slot_idx].pc = pc;
	spin_unlock(&app_slot_lock); 
}

void set_additional_pc(char app_slot_idx, long ips[MAX_STOP_NUM])
{
	int index;
	struct task_struct *task;
	spin_lock(&app_slot_lock); 
	for(index = 0; index<MAX_STOP_NUM; index++){
		task = pid_task(find_get_pid(app_slot_list[(int)app_slot_idx].stopped_pid[index]), PIDTYPE_PID);
		if(task)
			task_pt_regs(task)->ip = ips[index];
	}
	spin_unlock(&app_slot_lock); 
}

int get_shuffle_pid(char app_slot_idx)
{
	return app_slot_list[(int)app_slot_idx].shuffle_pid;
}

volatile char *get_start_flag(char app_slot_idx, int pid)
{
	int index;
	spin_lock(&app_slot_lock);
	for(index=0; index<MAX_FLAG_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].start_flags[index].pid==pid){
			spin_unlock(&app_slot_lock);
			return &app_slot_list[(int)app_slot_idx].start_flags[index].start_flag;
		}
	}
	PRINTK("Find no matched pid(%d) %s\n", pid, __FUNCTION__);
	spin_unlock(&app_slot_lock);
	return NULL;	
}

volatile char *req_a_start_flag(char app_slot_idx, int pid)
{
	int index;
	spin_lock(&app_slot_lock);
	for(index=0; index<MAX_FLAG_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].start_flags[index].pid==0){
			app_slot_list[(int)app_slot_idx].start_flags[index].pid = pid;
			PRINTK("%s pid(%d)\n", __FUNCTION__, pid);
			spin_unlock(&app_slot_lock);
			return &app_slot_list[(int)app_slot_idx].start_flags[index].start_flag;
		}
	}
	PRINTK("There is no free start flag in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
	return NULL;
}

void free_a_start_flag(char app_slot_idx, int pid)
{
	int index;
	spin_lock(&app_slot_lock);
	for(index=0; index<MAX_FLAG_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].start_flags[index].pid==pid){
			app_slot_list[(int)app_slot_idx].start_flags[index].pid = 0;
			app_slot_list[(int)app_slot_idx].start_flags[index].start_flag = 0;
			PRINTK("%s pid(%d)\n", __FUNCTION__, pid);
			spin_unlock(&app_slot_lock);
			return ;
		}
	}
	PRINTK("Find no matched pid in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
	return ;
}

ulong get_shuffle_pc(char app_slot_idx)
{
	return app_slot_list[(int)app_slot_idx].pc;
}

long set_app_start(struct task_struct *ts)
{
	int index;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].executed_start = 1;
			spin_unlock(&app_slot_lock); 
			return app_slot_list[index].program_entry;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return 0;
}

char is_app_start(struct task_struct *ts)
{
	int index;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			spin_unlock(&app_slot_lock); 
			return app_slot_list[index].executed_start;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return 0;
}

char free_x_info(struct task_struct *ts, long start, long end, char *shm_path)
{
	int index;
	int internal_index = 0;
	long free_len = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			for(internal_index=0; internal_index<MAX_X_NUM; internal_index++){
				if(app_slot_list[index].xr[internal_index].cc_start!=0){
					//PRINTK("(%d) %lx-%lx\n", internal_index, 
					//	app_slot_list[index].xr[internal_index].cc_start, app_slot_list[index].xr[internal_index].cc_end);
					if(app_slot_list[index].xr[internal_index].cc_start==start && app_slot_list[index].xr[internal_index].cc_end==end){
						//need this code cache
						free_len = app_slot_list[index].xr[internal_index].cc_end - app_slot_list[index].xr[internal_index].cc_start;
						orig_munmap(app_slot_list[index].xr[internal_index].cc_start, free_len);
						strcpy(shm_path, app_slot_list[index].xr[internal_index].shfile);
						memset((void*)&(app_slot_list[index].xr[internal_index]), 0, sizeof(X_REGION));
						spin_unlock(&app_slot_lock); 
						return index;
					}
				}				
			}
			//PRINTK("find no match x info!\n");
			spin_unlock(&app_slot_lock); 
			return -1;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return -1;
}

char insert_x_info(struct task_struct *ts, long cc_start, long cc_end, const char *file)
{
	int index;
	int internal_index = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			for(internal_index=0; internal_index<MAX_X_NUM; internal_index++){
				if(app_slot_list[index].xr[internal_index].cc_start==0){
					//PRINTK("insert x info (%s): %lx-%lx\n", file, cc_start, cc_end);
					app_slot_list[index].xr[internal_index].cc_start = cc_start;
					app_slot_list[index].xr[internal_index].cc_end = cc_end;
					strcpy(app_slot_list[index].xr[internal_index].shfile, file);
					spin_unlock(&app_slot_lock); 
					return index;
				}
			}
			PRINTK("find no free x info!\n");
			spin_unlock(&app_slot_lock); 
			return -1;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return -1;
}

char insert_stack_info(struct task_struct *ts, long ss_start, long ss_end, const char *file)
{
	int index;
	int internal_index = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			app_slot_list[index].ss_number++;
			for(internal_index=0; internal_index<MAX_STACK_NUM; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid==0){
					app_slot_list[index].sr[internal_index].ss_region_start = ss_start;
					app_slot_list[index].sr[internal_index].ss_region_end = ss_end;
					app_slot_list[index].sr[internal_index].belong_pid = ts->pid;
					strcpy(app_slot_list[index].sr[internal_index].shfile, file);
					spin_unlock(&app_slot_lock); 
					return index;
				}
			}
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return -1;
}

char *get_stack_shm_info(struct task_struct *ts, long *stack_len)
{
	int index;
	int internal_index = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			for(internal_index=0; internal_index<MAX_STACK_NUM; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid==ts->pid){
					*stack_len = app_slot_list[index].sr[internal_index].ss_region_end - \
						app_slot_list[index].sr[internal_index].ss_region_start;
					spin_unlock(&app_slot_lock);
					return app_slot_list[index].sr[internal_index].shfile;
				}
			}
		}
	}
	PRINTK("[%d, %d]find no app slot in %s\n", ts->pid, pid_vnr(task_pgrp(ts)), __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return NULL;
}

void lock_app_slot(void)
{
	spin_lock(&app_slot_lock);
}

void unlock_app_slot(void)
{
	spin_unlock(&app_slot_lock);
}

void modify_stack_belong(struct task_struct *ts, int old_pid, int new_pid)
{
	int index;
	int internal_index = 0;
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			for(internal_index=0; internal_index<MAX_STACK_NUM; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid==old_pid){
					if(app_slot_list[index].sr[internal_index].ss_region_start==0)
						PRINTK("%s modify stack belong error (old pid: %d, new pid %d): find no shadow stack!\n", ts->comm, old_pid, new_pid);
					else
						app_slot_list[index].sr[internal_index].belong_pid = new_pid; 
					return;
				}
			}
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	return ;
}

int get_stack_number(struct task_struct *ts)
{
	int index;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			spin_unlock(&app_slot_lock);
			return app_slot_list[index].ss_number;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return -1;
}

char get_ss_info(struct task_struct *ts, long *stack_start, long *stack_end)
{
	int index;
	int internal_index = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			for(internal_index=0; internal_index<MAX_STACK_NUM; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid==ts->pid){
					*stack_start = app_slot_list[index].sr[internal_index].ss_region_start;
					*stack_end = app_slot_list[index].sr[internal_index].ss_region_end;
					spin_unlock(&app_slot_lock); 
					return index;
				}
			}
		}
	}
	PRINTK("[%d, %d] find no app slot in %s\n", ts->pid, pid_vnr(task_pgrp(ts)), __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return -1;
}

void free_stack_info(int pgid, int pid)
{
	int index;
	int internal_index = 0;
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pgid)){
			for(internal_index=0; internal_index<MAX_STACK_NUM; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid==pid){
					if(app_slot_list[index].sr[internal_index].ss_region_start==0)
						PRINTK("%s free stack error): find no shadow stack!\n", current->comm);
					else
						memset((void*)&app_slot_list[index].sr[internal_index], 0, sizeof(STACK_REGION)); 
					return;
				}
			}
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	return ;
}

int is_process_exist(int pid, char *comm)
{
	struct task_struct *task;  
	for_each_process(task)  
	{
		if(task->pid==pid && !strcmp(task->comm, comm))
			return 1;
	}
	
	//PRINTK("%s(%d) is not exist\n", comm, pid);
	return 0;
}

typedef struct {
	int pid;
	int stack_len;
	int stack_idx;
	char *shm_file;
}SAVED_STACK_INFO;
extern void send_ss_free_mesg_to_shuffle_process(struct task_struct *ts, char app_slot_idx, \
	int shuffle_pid, long stack_len, char *shm_file);

void free_dead_stack(struct task_struct *ts)
{
	int index;
	int internal_index = 0;
	SAVED_STACK_INFO ss_array[MAX_STACK_NUM];
	int array_sum = 0;
	int array_idx = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(is_pgid_matched(index, pid_vnr(task_pgrp(ts)))){
			//scan all pid
			for(internal_index=0; internal_index<MAX_STACK_NUM; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid!=0){
					ss_array[array_sum++] = (SAVED_STACK_INFO){app_slot_list[index].sr[internal_index].belong_pid, \
						app_slot_list[index].sr[internal_index].ss_region_end - app_slot_list[index].sr[internal_index].ss_region_start, \
						internal_index, app_slot_list[index].sr[internal_index].shfile};
					//PRINTK("find dead stack belong to %d\n", app_slot_list[index].sr[internal_index].belong_pid);
				}
			}
			spin_unlock(&app_slot_lock); 
			//judge the pid is exist or not
			for(array_idx = 0; array_idx<array_sum; array_idx++){
				if(!is_process_exist(ss_array[array_idx].pid, ts->comm)){
					//send message
					send_ss_free_mesg_to_shuffle_process(ts, index, app_slot_list[index].shuffle_pid, ss_array[array_idx].stack_len, \
						ss_array[array_idx].shm_file);
					//free
					memset((void*)&app_slot_list[index].sr[ss_array[array_idx].stack_idx], 0, sizeof(STACK_REGION)); 
				}
			}
			return ;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return ;
}


void init_stopped_ips_from_app_slot(char app_slot_idx, long ips[MAX_STOP_NUM])
{
	int index;
	struct task_struct *task;
	for(index = 0; index<MAX_STOP_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].stopped_pid[index]!=0){
			task = pid_task(find_get_pid(app_slot_list[(int)app_slot_idx].stopped_pid[index]), PIDTYPE_PID);
			ips[index] = task ? task_pt_regs(task)->ip : 0;
		}else
			ips[index] = 0;
	}
}

/**************************rerandomization and communication with shuffle process**************************/

void send_rerandomization_mesg_to_shuffle_process(struct task_struct *ts, int curr_cc_id, char app_slot_idx)
{
	int connect = curr_cc_id==0 ? CURR_IS_CV1_NEED_CV2 : CURR_IS_CV2_NEED_CV1;
	struct pt_regs *regs = task_pt_regs(ts);
	int shuffle_pid = get_shuffle_pid(app_slot_idx);
	volatile char *start_flag = req_a_start_flag(app_slot_idx, ts->pid);
	MESG_BAG msg = {connect, ts->pid, regs->ip, {0}, CC_OFFSET, SS_OFFSET, GS_BASE, global_ss_type, "\0", "need rerandomization!"};
	strcpy(msg.app_name, ts->comm);

	init_stopped_ips_from_app_slot(app_slot_idx, msg.additional_ips);
	
	if(shuffle_pid!=0){
		nl_send_msg(shuffle_pid, msg);
		*start_flag = 1;
		while(*start_flag){
			schedule();
		}
		regs->ip = get_shuffle_pc(app_slot_idx);
		
		free_a_start_flag(app_slot_idx, ts->pid);
	}
}

void insert_stopped_pid(char app_slot_idx, int pid)
{
	int index;
	spin_lock(&app_slot_lock); 
	for(index = 0; index<MAX_STOP_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].stopped_pid[index]==0){
			app_slot_list[(int)app_slot_idx].stopped_pid[index] = pid;			
			spin_unlock(&app_slot_lock); 
			return ;
		}
	}
	PRINTK("%s find no free stopped task_struct\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
}

void wait_for_all_stopped(char app_slot_idx)
{
	struct task_struct *task;
	int index;
	for(index = 0; index<MAX_STOP_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].stopped_pid[index]!=0){
			task = pid_task(find_get_pid(app_slot_list[(int)app_slot_idx].stopped_pid[index]), PIDTYPE_PID);
			if(task)
				PRINTK("[%d] wait %d (state: %ld) to be stopped, pc=0x%lx!\n", current->pid, task->pid, task->state, task_pt_regs(task)->ip);
			
			do{
				task = pid_task(find_get_pid(app_slot_list[(int)app_slot_idx].stopped_pid[index]), PIDTYPE_PID);//should get new task from pid, some process or threads maybe exist
				schedule();
			}while(task && task->state!=TASK_STOPPED);
			
			if(task)
				PRINTK("[%d] %d has stopped, pc=0x%lx!\n", current->pid, task->pid, task_pt_regs(task)->ip);
		}
	}
}

char make_sure_all_stopped(char app_slot_idx)
{
	struct task_struct *task;
	int index;
	for(index = 0; index<MAX_STOP_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].stopped_pid[index]!=0){
			task = pid_task(find_get_pid(app_slot_list[(int)app_slot_idx].stopped_pid[index]), PIDTYPE_PID);
			if(task && task->state!=TASK_STOPPED){
				PRINTK("%d should not be waked!\n", task->pid);
				return 0;
			}
		}
	}
	PRINTK("Make sure that all are stopped!\n");
	return 1;
}

void set_send_stop(char app_slot_idx)
{
	spin_lock(&app_slot_lock); 
	app_slot_list[(int)app_slot_idx].has_send_stop = 1;
	spin_unlock(&app_slot_lock); 	
}

void stop_all_processes(char app_slot_idx, struct task_struct *ts)
{
	struct task_struct *task = ts;
	// 1.send stop signal to all related processes}
	do{
		if(task->pid!=ts->pid){
			insert_stopped_pid(app_slot_idx, task->pid);
			PRINTK("[%d] send %d to be stopped, pc=0x%lx!\n", ts->pid, task->pid, task_pt_regs(task)->ip);
			kill_pid(task_pid(task), SIGSTOP, 1);
		}
	}while_each_thread(ts, task);
	// 2.set stop signal flag
	set_send_stop(app_slot_idx);
	// 3.wait for stopped
	wait_for_all_stopped(app_slot_idx);
	// 4.make sure all stopped
	make_sure_all_stopped(app_slot_idx);

	return ;
}

void wake_all_processes(char app_slot_idx)
{
	int index;
	struct task_struct *task;
	spin_lock(&app_slot_lock); 
	for(index = 0; index<MAX_STOP_NUM; index++){
		if(app_slot_list[(int)app_slot_idx].stopped_pid[index]!=0){
			task = pid_task(find_get_pid(app_slot_list[(int)app_slot_idx].stopped_pid[index]), PIDTYPE_PID);
			if(task && task->state==TASK_STOPPED){
				PRINTK("[%d] wake %d\n", current->pid, task->pid);
				kill_pid(task_pid(task), SIGCONT, 1);
			}
			app_slot_list[(int)app_slot_idx].stopped_pid[index] = 0;
		}
	}
	PRINTK("[%d] finish waking all\n", current->pid);
	spin_unlock(&app_slot_lock); 	
}

void clear_rerandomization_and_send_stop(char app_slot_idx)
{
	spin_lock(&app_slot_lock); 
	app_slot_list[(int)app_slot_idx].has_rerandomization = 0;
	app_slot_list[(int)app_slot_idx].has_send_stop = 0;
	spin_unlock(&app_slot_lock); 
}

int has_rerandomization(char app_slot_idx)
{
	spin_lock(&app_slot_lock);
	if(app_slot_list[(int)app_slot_idx].has_rerandomization==1){
		spin_unlock(&app_slot_lock);
		PRINTK("Had threads to wait!\n");
		//wait for stopped
		while(app_slot_list[(int)app_slot_idx].has_send_stop==0)
			schedule();
		return 1;
	}else{
		//set rerandomization flag
		app_slot_list[(int)app_slot_idx].has_rerandomization = 1;
		spin_unlock(&app_slot_lock);
		return 0;
	}
}

void close_rerandomization(struct task_struct *ts)
{
	char index = get_app_slot_idx(pid_vnr(task_pgrp(ts)));
	spin_lock(&app_slot_lock);
	app_slot_list[(int)index].has_rerandomization = 1;
	app_slot_list[(int)index].has_send_stop = 1;
	spin_unlock(&app_slot_lock);
}

void increase_rerand_times(char app_slot_idx)
{
	spin_lock(&app_slot_lock);
	app_slot_list[(int)app_slot_idx].rerand_times++;
	spin_unlock(&app_slot_lock);
}

void rerandomization(struct task_struct *ts)
{
	//return ;
	int internal_index = 0;
	int shm_fd = 0;
	long cc_start = 0;
	long cc_end = 0;
	int curr_cc_id = 0;
	long shm_off = 0;
	long mmap_ret = 0;
	int shuffle_pid = 0;
	char index = get_app_slot_idx(pid_vnr(task_pgrp(ts)));
	// 1.judge has a process/thread has rerandomization or not
	if(has_rerandomization(index))
		return ;
	// increase rerand times
	increase_rerand_times(index);
	// 2.stop all related processes(multi-threads and multi-processes)
	stop_all_processes(index, ts);
	// 3.remap the code cache
	spin_lock(&app_slot_lock); 
	curr_cc_id = app_slot_list[(int)index].cc_id;
	shuffle_pid = app_slot_list[(int)index].shuffle_pid;
	app_slot_list[(int)index].cc_id = curr_cc_id==0 ? 1 : 0;
	for(internal_index = 0; internal_index<MAX_X_NUM; internal_index++){
		cc_start = app_slot_list[(int)index].xr[internal_index].cc_start;
		cc_end = app_slot_list[(int)index].xr[internal_index].cc_end;
		if(cc_start!=0){
			shm_off = curr_cc_id==0 ? (cc_end-cc_start) : 0;
			shm_fd = open_shm_file(app_slot_list[(int)index].xr[internal_index].shfile);
			mmap_ret = orig_mmap(cc_start, cc_end-cc_start, PROT_READ|PROT_EXEC, MAP_SHARED|MAP_FIXED, shm_fd, shm_off);
			//PRINTK("remap %s %lx (fd=%d, ret=%ld) \n", app_slot_list[(int)index].xr[internal_index].shfile, shm_off, shm_fd, mmap_ret);
			close_shm_file(shm_fd);
		}
	}
	spin_unlock(&app_slot_lock); 
	// 4.send msg to shuffle process
	send_rerandomization_mesg_to_shuffle_process(ts, curr_cc_id, index);
	// 5.clear all flags
	clear_rerandomization_and_send_stop(index);
	// 6.wake all related processes
	wake_all_processes(index);
}

/*************************Below functions are used to stop and wakeup processes******************************************/

long saved_state = 0;

void stop_all(void)
{
	struct task_struct *task;  
	for_each_process(task)  
	{
		if(!strcmp(task->comm, "b.out")){
			saved_state = task->state;
			set_task_state(task, TASK_STOPPED);//TASK_INTERRUPTIBLE
		}
	} 
	return ;
}

void wake_all(void)
{
	struct task_struct *task;  
	//struct pt_regs *regs;
	for_each_process(task)  
	{
		if(!strcmp(task->comm, "b.out")){
			//regs = task_pt_regs(task);
			wake_up_process(task);
			set_task_state(task, saved_state);
		}
	} 
	return ;
}

