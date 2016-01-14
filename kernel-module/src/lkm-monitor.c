#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/mman.h>

#include "lkm-config.h"
#include "lkm-utility.h"
#include "lkm-file.h"
#include "lkm-hook.h"
char* monitor_app_list[MAX_APP_LIST_NUM];
/**
 * checking if the intercepted app is the one we want to monitor
 **/
ulong is_monitor_app(const char *name)
{
	ulong ret = 0, i = 0;
	for(i = 0; i < MAX_APP_LIST_NUM; i++){
		 /* search the application name in the monitor list
		 */
		if(monitor_app_list[i] == NULL) break;
		// if orig name is too long (the struct->comm's len is 16)
		// thus, we can not use strcmp
		if(!strcmp(name, monitor_app_list[i])){
			ret = 1; break;
		}
		if(strlen(name) >= 15 && !strncmp(name, monitor_app_list[i], strlen(name))){
			ret = 1; break;
		}
	}
	return ret;
}

void init_monitor_app_list(void)
{
	monitor_app_list[0] = "ls";
}

//-------------APP slot----------------------//

typedef struct{
	char has_read;
	char has_pread64;
	char has_readv;
	char has_recvfrom;
	char has_recvmsg;
	char has_preadv;
	char has_mq_timedreceive;
}IO_MONITOR;

typedef struct{
	long cc_start;
	long cc_end;
	char shfile[256];
	int cc_id;//current cc used index
}X_REGION;

typedef struct{
	long ss_region_start;
	long ss_region_end;
	char shfile[256];
}STACK_REGION;

typedef struct{
	int tgid;	
	long program_entry;
	char executed_start;
	char app_name[256];
	char start_instr_encode[8];
	IO_MONITOR im;//use to monitor  
	X_REGION xr[15];
	STACK_REGION sr[15];	
}APP_SLOT;

APP_SLOT app_slot_list[MAX_APP_SLOT_LIST_NUM];

void init_app_slot(struct task_struct *ts)
{
	int index = 0;
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==0){
			app_slot_list[index].tgid = ts->tgid;
			app_slot_list[index].executed_start = 0;
			app_slot_list[index].program_entry = 0;
			strcpy(app_slot_list[index].app_name, ts->comm);
			memset((void*)&(app_slot_list[index].start_instr_encode), 11, sizeof(char));
			memset((void*)&(app_slot_list[index].im), 0, sizeof(IO_MONITOR));
			memset((void*)&(app_slot_list[index].xr[0]), 0, 15*sizeof(X_REGION));
			memset((void*)&(app_slot_list[index].sr[0]), 0, 15*sizeof(STACK_REGION));
			return ;
		}
	}
}
void free_app_slot(struct task_struct *ts)
{
	int index = 0;
	//find the slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==ts->tgid){
			memset((void*)(&app_slot_list[index]), 0, sizeof(APP_SLOT));
			return ;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
}

void set_io_in(struct task_struct *ts, int sys_no, char value)
{
	int index = 0;
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==ts->tgid){
			switch(sys_no){
				case __NR_read: app_slot_list[index].im.has_read = value; return;
				case __NR_pread64: app_slot_list[index].im.has_pread64 = value; return;
				case __NR_readv: app_slot_list[index].im.has_readv = value; return;
				case __NR_recvfrom: app_slot_list[index].im.has_recvfrom = value; return;
				case __NR_recvmsg: app_slot_list[index].im.has_recvmsg = value; return;
				case __NR_preadv: app_slot_list[index].im.has_preadv = value; return;
				case __NR_mq_timedreceive: app_slot_list[index].im.has_mq_timedreceive = value; return;
				default:
					break;
			}
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
}

char has_io_in(struct task_struct *ts, int sys_no)
{
	int index = 0;
	// 1.find a free slot
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==ts->tgid){
			switch(sys_no){
				case __NR_read: return app_slot_list[index].im.has_read;
				case __NR_pread64: return app_slot_list[index].im.has_pread64;
				case __NR_readv: return app_slot_list[index].im.has_readv;
				case __NR_recvfrom: return app_slot_list[index].im.has_recvfrom;
				case __NR_recvmsg: return app_slot_list[index].im.has_recvmsg;
				case __NR_preadv: return app_slot_list[index].im.has_preadv;
				case __NR_mq_timedreceive: return app_slot_list[index].im.has_mq_timedreceive;
				default:
					break;
			}
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	return 0;
}

char *get_start_encode_and_set_entry(struct task_struct *ts, long program_entry)
{
	int index = 0;
	// 1.find the  encode
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==ts->tgid){
			app_slot_list[index].program_entry = program_entry;
			return app_slot_list[index].start_instr_encode;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	return NULL;
}

char *is_checkpoint(struct task_struct *ts)
{
	int index = 0;
	long curr_rip = task_pt_regs(ts)->ip;
	// 1.find the  encode
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==ts->tgid){
			if(app_slot_list[index].program_entry == (curr_rip-8))
				return app_slot_list[index].start_instr_encode;
			else
				return NULL;
		}
	}

	PRINTK("find no app slot in %s\n", __FUNCTION__);
	return NULL;
}

long set_app_start(struct task_struct *ts){
	int index;
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==ts->tgid){
			app_slot_list[index].executed_start = 0;
			return app_slot_list[index].program_entry;
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	return 0;
}

void insert_x_info(struct task_struct *ts, long cc_start, long cc_end, const char *file)
{
	int index;
	int internal_index = 0;
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==ts->tgid){
			for(internal_index=0; internal_index<15; internal_index++){
				if(app_slot_list[index].xr[internal_index].cc_start==0){
					app_slot_list[index].xr[internal_index].cc_start = cc_start;
					app_slot_list[index].xr[internal_index].cc_end = cc_end;
					strcpy(app_slot_list[index].xr[internal_index].shfile, file);
					app_slot_list[index].xr[internal_index].cc_id = 0;
					return;
				}
			}
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	return ;
}

void insert_stack_info(struct task_struct *ts, long ss_start, long ss_end, const char *file)
{
	int index;
	int internal_index = 0;
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==ts->tgid){
			for(internal_index=0; internal_index<15; internal_index++){
				if(app_slot_list[index].sr[internal_index].ss_region_start==0){
					app_slot_list[index].sr[internal_index].ss_region_start = ss_start;
					app_slot_list[index].sr[internal_index].ss_region_end = ss_end;
					strcpy(app_slot_list[index].sr[internal_index].shfile, file);
					return;
				}
			}
		}
	}
	PRINTK("find no app slot in %s\n", __FUNCTION__);
	return ;
}

/**************************rerandomization and communication with shuffle process**************************/

void rerandomization(struct task_struct *ts)
{
	int index = 0;
	int internal_index = 0;
	int shm_fd = 0;
	long cc_start = 0;
	long cc_end = 0;
	int cc_id = 0;
	long shm_off = 0;
	//struct pt_regs *regs = task_pt_regs(ts);
	// 1.remap the code cache
	for(index=0; index<MAX_APP_SLOT_LIST_NUM; index++){
		if(app_slot_list[index].tgid==ts->tgid){
			for(internal_index = 0; internal_index<15; internal_index++){
				cc_start = app_slot_list[index].xr[internal_index].cc_start;
				cc_end = app_slot_list[index].xr[internal_index].cc_end;
				cc_id = app_slot_list[index].xr[internal_index].cc_id;
				if(cc_start!=0){
					app_slot_list[index].xr[internal_index].cc_id = 1;
					shm_off = cc_id==0 ? (cc_end-cc_start) : 0;
					shm_fd = open_shm_file(app_slot_list[index].xr[internal_index].shfile);
					orig_mmap(cc_start, cc_end-cc_start, PROT_READ|PROT_EXEC, MAP_SHARED|MAP_FIXED, shm_fd, shm_off);
					PRINTK("remap %s %lx\n", app_slot_list[index].xr[internal_index].shfile, shm_off);
					close_shm_file(shm_fd);
				}
			}
		}
	}
	// 2.send msg to shuffle process

	// 3.get msg of shuffle process

}

