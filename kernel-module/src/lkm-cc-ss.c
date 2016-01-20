#include <linux/mman.h>
#include <linux/sched.h>

#include "lkm-monitor.h"
#include "lkm-hook.h"
#include "lkm-utility.h"
#include "lkm-config.h"
#include "lkm-file.h"

long allocate_cc(long orig_x_size, const char *orig_name)
{
	int cc_fd = 0;
	long cc_size = X86_PAGE_ALIGN_CEIL(orig_x_size)*CC_MULTIPULE;
	long cc_ret = 0;
	long x_start = 0;
	char shm_path[256];
	int curr_gid = current->tgid;
	char *file_name = get_filename_from_path(orig_name);
	
	sprintf(shm_path, "/dev/shm/%d-%s.cc", curr_gid, file_name);
	cc_fd = open_shm_file(shm_path);
	orig_ftruncate(cc_fd, cc_size*2);
	cc_ret = orig_mmap(0, cc_size, PROT_EXEC|PROT_READ, MAP_SHARED, cc_fd, 0);

	x_start = cc_ret - CC_OFFSET;
	insert_x_info(current, cc_ret, cc_ret+cc_size, shm_path);
	close_shm_file(cc_fd);
	printk(KERN_EMERG  "[CR2:%d]mmap(addr:%lx, len:%lx)\n", current->pid, cc_ret, cc_size);
	return cc_ret;
}

long allocate_cc_fixed(long orig_x_start, long orig_x_end, const char *orig_name)
{
	int cc_fd = 0;
	long cc_size = X86_PAGE_ALIGN_CEIL(orig_x_end-orig_x_start)*CC_MULTIPULE;
	long cc_start = orig_x_start+CC_OFFSET;
	long cc_ret = 0;
	char shm_path[256];
	int curr_gid = current->tgid;
	char *file_name = get_filename_from_path(orig_name);
	
	sprintf(shm_path, "/dev/shm/%d-%s.cc", curr_gid, file_name);
	cc_fd = open_shm_file(shm_path);
	orig_ftruncate(cc_fd, cc_size*2);
	cc_ret = orig_mmap(cc_start, cc_size, PROT_EXEC|PROT_READ, MAP_SHARED|MAP_FIXED, cc_fd, 0);

	insert_x_info(current, cc_ret, cc_ret+cc_size, shm_path);
	close_shm_file(cc_fd);
	
	return cc_ret;
}

long allocate_ss_fixed(long orig_stack_start, long orig_stack_end)
{
	int ss_fd = 0;
	long ss_size = (orig_stack_end-orig_stack_start)*SS_MULTIPULE;
	long ss_start = orig_stack_end - SS_OFFSET - ss_size;
	long ss_ret = 0;
	char shm_path[256];
	int curr_gid = current->tgid;
	char *file_name = get_filename_from_path(current->comm);
	
	sprintf(shm_path, "/dev/shm/%d-%s.ss", curr_gid, file_name);
	ss_fd = open_shm_file(shm_path);
	orig_ftruncate(ss_fd, ss_size);
	ss_ret = orig_mmap(ss_start, ss_size, PROT_WRITE|PROT_READ, MAP_SHARED|MAP_FIXED, ss_fd, 0);

	insert_stack_info(current, ss_ret, ss_ret+ss_size, shm_path);
	close_shm_file(ss_fd);
	//TODO: send mesg to shuffle process
	return ss_ret;
}


