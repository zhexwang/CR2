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
	//test
	monitor_app_list[31] = "a.out";
}

//-------------APP slot----------------------//

typedef struct{
	char has_write;
	char has_pwrite64;
	char has_writev;
	char has_sendto;
	char has_sendmsg;
	char has_pwritev;
	char has_mq_timedsend;
}IO_MONITOR;

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
	int pgid;
	int shuffle_pid;
	int cc_id;//current cc used index
	ulong pc;//send by shuffle process
	volatile char start_flag;
	long program_entry;
	char executed_start;
	char app_name[256];
	char start_instr_encode[7];
	IO_MONITOR im;//use to monitor  
	X_REGION xr[15];
	STACK_REGION sr[15];	
}APP_SLOT;

APP_SLOT app_slot_list[MAX_APP_SLOT_LIST_NUM];

int get_cc_id(struct task_struct *ts)
{
	int index = 0;
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts)))
			return app_slot_list[index].cc_id;
	}
	return -1;
}

char init_app_slot(struct task_struct *ts)
{
	int index = 0;
	PRINTK("execve %s\n", ts->comm);
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==0){
			app_slot_list[index].pgid = pid_vnr(task_pgrp(ts));
			app_slot_list[index].shuffle_pid = 0;
			app_slot_list[index].cc_id = 0;
			app_slot_list[index].pc = 0;
			app_slot_list[index].start_flag = 0;
			app_slot_list[index].executed_start = 0;
			app_slot_list[index].program_entry = 0;
			strcpy(app_slot_list[index].app_name, ts->comm);
			memset((void*)&(app_slot_list[index].start_instr_encode), 11, sizeof(char));
			memset((void*)&(app_slot_list[index].im), 0, sizeof(IO_MONITOR));
			memset((void*)&(app_slot_list[index].xr[0]), 0, 15*sizeof(X_REGION));
			memset((void*)&(app_slot_list[index].sr[0]), 0, 15*sizeof(STACK_REGION));
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
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			memset((void*)(&app_slot_list[index]), 0, sizeof(APP_SLOT));
			spin_unlock(&app_slot_lock);
			return ;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
}

void set_io_in(struct task_struct *ts, int sys_no, char value)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			switch(sys_no){
				case __NR_write: app_slot_list[index].im.has_write = value; spin_unlock(&app_slot_lock); return;
				case __NR_pwrite64: app_slot_list[index].im.has_pwrite64 = value; spin_unlock(&app_slot_lock); return;
				case __NR_writev: app_slot_list[index].im.has_writev = value; spin_unlock(&app_slot_lock); return;
				case __NR_sendto: app_slot_list[index].im.has_sendto = value; spin_unlock(&app_slot_lock); return;
				case __NR_sendmsg: app_slot_list[index].im.has_sendmsg = value; spin_unlock(&app_slot_lock); return;
				case __NR_pwritev: app_slot_list[index].im.has_pwritev = value; spin_unlock(&app_slot_lock); return;
				case __NR_mq_timedsend: app_slot_list[index].im.has_mq_timedsend = value; spin_unlock(&app_slot_lock); return;
				default:
					break;
			}
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock);
}

char has_io_in(struct task_struct *ts, int sys_no)
{
	int index = 0;
	spin_lock(&app_slot_lock);
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			switch(sys_no){
				case __NR_write: spin_unlock(&app_slot_lock); return app_slot_list[index].im.has_write;
				case __NR_pwrite64: spin_unlock(&app_slot_lock); return app_slot_list[index].im.has_pwrite64;
				case __NR_writev: spin_unlock(&app_slot_lock); return app_slot_list[index].im.has_writev;
				case __NR_sendto: spin_unlock(&app_slot_lock); return app_slot_list[index].im.has_sendto;
				case __NR_sendmsg: spin_unlock(&app_slot_lock); return app_slot_list[index].im.has_sendmsg;
				case __NR_pwritev: spin_unlock(&app_slot_lock); return app_slot_list[index].im.has_pwritev;
				case __NR_mq_timedsend: spin_unlock(&app_slot_lock); return app_slot_list[index].im.has_mq_timedsend;
				default:
					break;
			}
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return 0;
}

char *get_start_encode_and_set_entry(struct task_struct *ts, long program_entry)
{
	int index = 0;
	spin_lock(&app_slot_lock); 
	// 1.find the  encode
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
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
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
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
		if(app_slot_list[index].pgid==pgid){
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

int get_shuffle_pid(char app_slot_idx)
{
	return app_slot_list[(int)app_slot_idx].shuffle_pid;
}

volatile char *get_start_flag(char app_slot_idx)
{
	return &app_slot_list[(int)app_slot_idx].start_flag;
}

ulong get_shuffle_pc(char app_slot_idx)
{
	return app_slot_list[(int)app_slot_idx].pc;
}

long set_app_start(struct task_struct *ts){
	int index;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			app_slot_list[index].executed_start = 0;
			spin_unlock(&app_slot_lock); 
			return app_slot_list[index].program_entry;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return 0;
}

void insert_x_info(struct task_struct *ts, long cc_start, long cc_end, const char *file)
{
	int index;
	int internal_index = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			for(internal_index=0; internal_index<15; internal_index++){
				if(app_slot_list[index].xr[internal_index].cc_start==0){
					app_slot_list[index].xr[internal_index].cc_start = cc_start;
					app_slot_list[index].xr[internal_index].cc_end = cc_end;
					strcpy(app_slot_list[index].xr[internal_index].shfile, file);
					spin_unlock(&app_slot_lock); 
					return;
				}
			}
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return ;
}

void insert_stack_info(struct task_struct *ts, long ss_start, long ss_end, const char *file)
{
	int index;
	int internal_index = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			for(internal_index=0; internal_index<15; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid==0){
					app_slot_list[index].sr[internal_index].ss_region_start = ss_start;
					app_slot_list[index].sr[internal_index].ss_region_end = ss_end;
					app_slot_list[index].sr[internal_index].belong_pid = ts->pid;
					strcpy(app_slot_list[index].sr[internal_index].shfile, file);
					spin_unlock(&app_slot_lock); 
					return;
				}
			}
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	spin_unlock(&app_slot_lock); 
	return ;
}

char *get_stack_shm_info(struct task_struct *ts, long *stack_len)
{
	int index;
	int internal_index = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			for(internal_index=0; internal_index<15; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid==ts->pid){
					spin_unlock(&app_slot_lock); 
					*stack_len = app_slot_list[index].sr[internal_index].ss_region_end - \
						app_slot_list[index].sr[internal_index].ss_region_start;
					return app_slot_list[index].sr[internal_index].shfile;
				}
			}
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
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
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			for(internal_index=0; internal_index<15; internal_index++){
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

int get_stack_sum(struct task_struct *ts)
{
	int index;
	int internal_index = 0;
	int sum = 0;
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			for(internal_index=0; internal_index<15; internal_index++){
				if(app_slot_list[index].sr[internal_index].ss_region_start!=0){
					sum++;
					spin_unlock(&app_slot_lock); 
					return sum;
				}
			}
			spin_unlock(&app_slot_lock);
			return 0;
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
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			for(internal_index=0; internal_index<15; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid==ts->pid){
					*stack_start = app_slot_list[index].sr[internal_index].ss_region_start;
					*stack_end = app_slot_list[index].sr[internal_index].ss_region_end;
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

void free_stack_info(struct task_struct *ts)
{
	int index;
	int internal_index = 0;
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			for(internal_index=0; internal_index<15; internal_index++){
				if(app_slot_list[index].sr[internal_index].belong_pid==ts->pid){
					if(app_slot_list[index].sr[internal_index].ss_region_start==0)
						PRINTK("%s free stack error): find no shadow stack!\n", ts->comm);
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

/**************************rerandomization and communication with shuffle process**************************/

void send_rerandomization_mesg_to_shuffle_process(struct task_struct *ts, int curr_cc_id, char app_slot_idx)
{
	int connect = curr_cc_id==0 ? CURR_IS_CV1_NEED_CV2 : CURR_IS_CV2_NEED_CV1;
	struct pt_regs *regs = task_pt_regs(ts);
	int shuffle_pid = get_shuffle_pid(app_slot_idx);
	volatile char *start_flag = get_start_flag(app_slot_idx);
	MESG_BAG msg = {connect, pid_vnr(task_pgrp(ts)), regs->ip, CC_OFFSET, SS_OFFSET, GS_BASE, global_ss_type, "\0", "need rerandomization!"};
	strcpy(msg.app_name, ts->comm);
	
	if(shuffle_pid!=0){
		nl_send_msg(shuffle_pid, msg);
		*start_flag = 1;
		while(*start_flag){
			schedule();
		}
		regs->ip = get_shuffle_pc(app_slot_idx);
	}
}

void rerandomization(struct task_struct *ts)
{
	int index = 0;
	int internal_index = 0;
	int shm_fd = 0;
	long cc_start = 0;
	long cc_end = 0;
	int curr_cc_id = 0;
	long shm_off = 0;
	int shuffle_pid = 0;
	//struct pt_regs *regs = task_pt_regs(ts);
	// 1.remap the code cache
	spin_lock(&app_slot_lock); 
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].pgid==pid_vnr(task_pgrp(ts))){
			curr_cc_id = app_slot_list[index].cc_id;
			shuffle_pid = app_slot_list[index].shuffle_pid;
			app_slot_list[index].cc_id = curr_cc_id==0 ? 1 : 0;
			for(internal_index = 0; internal_index<15; internal_index++){
				cc_start = app_slot_list[index].xr[internal_index].cc_start;
				cc_end = app_slot_list[index].xr[internal_index].cc_end;
				if(cc_start!=0){
					shm_off = curr_cc_id==0 ? (cc_end-cc_start) : 0;
					shm_fd = open_shm_file(app_slot_list[index].xr[internal_index].shfile);
					orig_mmap(cc_start, cc_end-cc_start, PROT_READ|PROT_EXEC, MAP_SHARED|MAP_FIXED, shm_fd, shm_off);
					PRINTK("remap %s %lx\n", app_slot_list[index].xr[internal_index].shfile, shm_off);
					close_shm_file(shm_fd);
				}
			}
			break;
		}
	}
	spin_unlock(&app_slot_lock); 
	// 2.send msg to shuffle process
	send_rerandomization_mesg_to_shuffle_process(ts, curr_cc_id, index);
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

