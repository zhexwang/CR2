#include <linux/mman.h>
#include <linux/sched.h>

#include "lkm-monitor.h"
#include "lkm-hook.h"
#include "lkm-utility.h"
#include "lkm-config.h"
#include "lkm-file.h"
#include "lkm-netlink.h"

long allocate_trace_debug_buffer(ulong buffer_base, ulong buffer_size)
{
	char shm_path[256];
	int curr_pid = current->pid;//pid_vnr(task_pgrp(current));
	int fd;
	long ret;
	ulong aligned_buffer_size = X86_PAGE_ALIGN_CEIL(buffer_size);
	ulong aligned_buffer_base = X86_PAGE_ALIGN_CEIL(buffer_base);
	
	sprintf(shm_path, "/dev/shm/%d.tdb", curr_pid);
	fd = open_shm_file(shm_path);
	orig_ftruncate(fd, aligned_buffer_size);
	ret = orig_mmap(aligned_buffer_base, aligned_buffer_size, PROT_WRITE|PROT_READ, MAP_SHARED|MAP_FIXED, fd, 0);
	printk(KERN_EMERG  "[CR2:%d]mmap debug page(addr:%lx, len:%lx)\n", current->pid, ret, aligned_buffer_size);
	close_shm_file(fd);
	return ret;
}

static void send_dlopen_mesg_to_shuffle_process(struct task_struct *ts, char app_slot_idx, \
	long orig_x_start, long orig_x_end, long cc_size, const char *lib_name, const char *shm_file)
{
	struct pt_regs *regs = task_pt_regs(ts);
	int shuffle_pid = get_shuffle_pid(app_slot_idx);
	volatile char *start_flag = req_a_start_flag(app_slot_idx, ts->pid);
	MESG_BAG msg = {DLOPEN, ts->pid, regs->ip, {0}, orig_x_start, orig_x_end, cc_size, global_ss_type, "\0", "\0"};
	strcpy(msg.app_name, lib_name);
	strcpy(msg.mesg, shm_file);

	if(shuffle_pid!=0){
		nl_send_msg(shuffle_pid, msg);
		
		*start_flag = 1;
		while(*start_flag){
			schedule();
		}
		regs->ip = get_shuffle_pc(app_slot_idx);
		
		free_a_start_flag(app_slot_idx, ts->pid);
	}
	PRINTK("[%d]dlopen handled! NewPC=0x%lx\n", ts->pid, regs->ip);
	return ;	
}

static void send_dlclose_mesg_to_shuffle_process(struct task_struct *ts, char app_slot_idx, const char *lib_name, const char *shm_file)
{
	struct pt_regs *regs = task_pt_regs(ts);
	int shuffle_pid = get_shuffle_pid(app_slot_idx);
	volatile char *start_flag = req_a_start_flag(app_slot_idx, ts->pid);
	MESG_BAG msg = {DLCLOSE, ts->pid, regs->ip, {0}, 0, 0, 0, global_ss_type, "\0", "\0"};
	strcpy(msg.app_name, lib_name);
	strcpy(msg.mesg, shm_file);

	if(shuffle_pid!=0){
		nl_send_msg(shuffle_pid, msg);
		
		*start_flag = 1;
		while(*start_flag){
			schedule();
		}
		regs->ip = get_shuffle_pc(app_slot_idx);

		free_a_start_flag(app_slot_idx, ts->pid);
	}
	
	return ;	
}

void free_mem(unsigned long addr, size_t len)
{
	long cc_start = addr + CC_OFFSET;
	long cc_end = cc_start + X86_PAGE_ALIGN_CEIL(len)*CC_MULTIPULE;
	int pid = 0;
	char shm_path[256] = "\0";
	char lib_name[256] = "\0";
	char app_slot_idx = free_x_info(current, cc_start, cc_end, shm_path);
	if(app_slot_idx!=-1){
		sscanf(shm_path, "/dev/shm/%d-%s", &pid, lib_name);
		lib_name[strlen(lib_name)-3]='\0';
		PRINTK("[%d]dlclose: %s\n", current->pid, lib_name);
		send_dlclose_mesg_to_shuffle_process(current, app_slot_idx, lib_name, shm_path);
	}
}

long allocate_cc(long orig_x_size, const char *orig_name)
{
	int cc_fd = 0;
	long cc_size = X86_PAGE_ALIGN_CEIL(orig_x_size)*CC_MULTIPULE;
	long cc_ret = 0;
	long x_start = 0;
	char app_slot_idx = 0;
	char shm_path[256];
	int curr_pid = current->pid;//pid_vnr(task_pgrp(current));
	char *file_name = get_filename_from_path(orig_name);
	
	sprintf(shm_path, "/dev/shm/%d-%s.cc", curr_pid, file_name);
	cc_fd = open_shm_file(shm_path);
	orig_ftruncate(cc_fd, cc_size*2);
	cc_ret = orig_mmap(0, cc_size, PROT_EXEC|PROT_READ, MAP_SHARED, cc_fd, 0);

	x_start = cc_ret - CC_OFFSET;
	app_slot_idx = insert_x_info(current, cc_ret, cc_ret+cc_size, shm_path);
	close_shm_file(cc_fd);
	if(is_app_start(current)){//send message to shuffle process, dlopen(need stop all processes/threads, we only handle one process/thread now)
		PRINTK("[%d] dlopen: %s\n", current->pid, orig_name);
		send_dlopen_mesg_to_shuffle_process(current, app_slot_idx, x_start, x_start+orig_x_size, cc_size, orig_name, shm_path);
	}
	//PRINTK("[CR2:%d]mmap_cc(addr:%lx, len:%lx)\n", current->pid, cc_ret, cc_size);
	return cc_ret;
}

long allocate_cc_fixed(long orig_x_start, long orig_x_end, const char *orig_name)
{
	int cc_fd = 0;
	long cc_size = X86_PAGE_ALIGN_CEIL(orig_x_end-orig_x_start)*CC_MULTIPULE;
	long cc_start = orig_x_start+CC_OFFSET;
	long cc_ret = 0;
	char app_slot_idx = 0;
	char shm_path[256];
	int curr_pid = current->pid;//pid_vnr(task_pgrp(current));
	char *file_name = get_filename_from_path(orig_name);
	
	sprintf(shm_path, "/dev/shm/%d-%s.cc", curr_pid, file_name);
	cc_fd = open_shm_file(shm_path);
	orig_ftruncate(cc_fd, cc_size*2);
	cc_ret = orig_mmap(cc_start, cc_size, PROT_EXEC|PROT_READ, MAP_SHARED|MAP_FIXED, cc_fd, 0);

	app_slot_idx = insert_x_info(current, cc_ret, cc_ret+cc_size, shm_path);
	close_shm_file(cc_fd);
	if(is_app_start(current)){//send message to shuffle process, dlopen
		send_dlopen_mesg_to_shuffle_process(current, app_slot_idx, orig_x_start, orig_x_end, cc_size, orig_name, shm_path);
	}
	return cc_ret;
}

extern void send_ss_create_mesg_to_shuffle_process(struct task_struct *ts, char app_slot_idx, int shuffle_pid, long stack_len, char *shm_file);

long allocate_ss_fixed(long orig_stack_start, long orig_stack_end)
{
	int ss_fd = 0;
	long ss_size = (orig_stack_end-orig_stack_start)*SS_MULTIPULE;
	long ss_start = orig_stack_end - SS_OFFSET - ss_size;
	long ss_ret = 0;
	char shm_path[256];
	int curr_sum = get_stack_number(current);
	char *file_name = get_filename_from_path(current->comm);
	int curr_pid = current->pid;
	char app_slot_idx = -1;
	int shuffle_pid = 0;
	
	sprintf(shm_path, "/dev/shm/%d-%d-%s.ss", curr_pid, curr_sum, file_name);
	ss_fd = open_shm_file(shm_path);
	orig_ftruncate(ss_fd, ss_size);
	ss_ret = orig_mmap(ss_start, ss_size, PROT_WRITE|PROT_READ, MAP_SHARED|MAP_FIXED, ss_fd, 0);

	app_slot_idx = insert_stack_info(current, ss_ret, ss_ret+ss_size, shm_path);
	close_shm_file(ss_fd);
	
	if(is_app_start(current)){//send message to shuffle process, dlopen
		shuffle_pid = get_shuffle_pid(app_slot_idx);
		send_ss_create_mesg_to_shuffle_process(current, app_slot_idx, shuffle_pid, ss_size, shm_path);
	}
	return ss_ret;
}

long reallocate_ss(long ss_start, long ss_end)
{
	int ss_fd = 0;
	long ss_size = ss_end - ss_start;
	long ss_ret = 0;
	char shm_path[256];
	int curr_sum = get_stack_number(current);
	char *file_name = get_filename_from_path(current->comm);
	int curr_pid = current->pid;
	
	sprintf(shm_path, "/dev/shm/%d-%d-%s.ss", curr_pid, curr_sum, file_name);
	ss_fd = open_shm_file(shm_path);
	orig_ftruncate(ss_fd, ss_size);
	ss_ret = orig_mmap(ss_start, ss_size, PROT_WRITE|PROT_READ, MAP_SHARED|MAP_FIXED, ss_fd, 0);

	insert_stack_info(current, ss_ret, ss_ret+ss_size, shm_path);
	close_shm_file(ss_fd);
	//TODO: send mesg to shuffle process
	return ss_ret;
}
