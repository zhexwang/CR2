#include <linux/module.h>
#include <linux/mman.h>
#include <asm/uaccess.h>       /* for get_user and put_user */
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>

#include "lkm-config.h"
#include "lkm-utility.h"
#include "lkm-hook.h"
#include "lkm-monitor.h"
#include "lkm-cc-ss.h"
#include "lkm-netlink.h"
#include "lkm-file.h"

char *get_start_encode_and_set_entry(struct task_struct *ts, long program_entry);
volatile char start_flag = 0;
/*
	push %rax
	mov %eax, $231//exit_group
	syscall
*/
char check_instr_encode[8] = {0x50, 0xb8, 0xe7, 0x00, 0x00, 0x00, 0x0f, 0x05};

//set checkpoint to check the program is begin to execute __start function
void set_checkpoint(struct task_struct *ts, long program_entry, long x_region_start, long x_region_end)
{
	int index;
	char *record_encode = get_start_encode_and_set_entry(ts, program_entry);
	char *target_encode = (char*)program_entry;
	orig_mprotect(x_region_start, x_region_end - x_region_start, PROT_READ|PROT_WRITE|PROT_EXEC);
	//record the original encode
	for(index=0; index<8; index++)
		get_user(record_encode[index], target_encode+index);
	//rewrite the checkpoint encode
	for(index=0; index<8; index++)
		put_user(check_instr_encode[index], target_encode+index);
}

extern int shuffle_process_pid;
extern long new_ip;
extern long connect_with_shuffle_process;
long send_mesg_to_shuffle_process(long curr_ip, int curr_pid)
{
	MESG_BAG msg = {1, curr_pid, curr_ip, "set_program_start!"};

	if(connect_with_shuffle_process!=0){
		nl_send_msg(shuffle_process_pid, msg);
		start_flag = 1;
		while(start_flag){
			schedule();
		}
		return new_ip;
	}else
		return curr_ip;
}

//set checkpoint to check the program is begin to execute __start function
long set_program_start(struct task_struct *ts, char *orig_encode)
{
	int index;
	char *target_encode = (char*)set_app_start(ts);
	struct pt_regs *regs = task_pt_regs(ts);
	long *curr_rsp = (long*)regs->sp;
	long orig_rax = 0;
	long curr_ip = regs->ip - 8;
	long newip = 0;
	//rewrite the checkpoint encode
	for(index=0; index<8; index++)
		put_user(orig_encode[index], target_encode+index);
	//pop rax
	regs->sp += 8;	
	get_user(orig_rax, curr_rsp);
	//protect the origin x region

	//send msg to generate the cc and get the pc
	newip = send_mesg_to_shuffle_process(curr_ip, ts->pid);
	//set rip according to the rerandomizaton result
	regs->ip = newip;
	
	return orig_rax;
}

void replace_at_base_in_aux(ulong orig_interp_base, ulong new_interp_base, struct task_struct *ts)
{
	ulong *elf_info = ts->mm->saved_auxv;
	ulong interp_base = 0;
	struct pt_regs *regs = task_pt_regs(ts);
	ulong *auxv_rsp = (long*)regs->sp;
	while(*elf_info!=orig_interp_base){
		elf_info++;
	}
	*elf_info = new_interp_base;

	while(interp_base!=orig_interp_base){
		get_user(interp_base, auxv_rsp);
		auxv_rsp++;
	}
	auxv_rsp--;
	put_user(new_interp_base, auxv_rsp);
}

//when gdb debug the program, the ld.so is loaded at the very high address, there is no space to allocate the memory
//so we remap the interpreter and allocate the code cache for main and ld.so by the way
void remmap_interp_and_allocate_cc(void)
{
	typedef struct {
		ulong region_start;
		ulong region_end;
		ulong prot;
		ulong flags;
		ulong off;
		struct file *file_ptr;
	}LD_REGION;
	#define LD_REGION_MAX 4
	LD_REGION ld_regions[LD_REGION_MAX];
	int ld_region_num = 0;
	int index = 0;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *list = mm->mmap, *ptr = list;
	struct pt_regs *regs = task_pt_regs(current);
	long ld_bss_start = 0;
	long map_addr = 0;
	long cc_ret = 0;
	long ld_offset = 0;//ld-linux.so fixed offset
	long program_entry = 0;
	long *program_entry_addr = NULL;
	int main_file_sec_no = 0;
#ifdef _C10	
	char buf[256];
	char *ld_path = NULL;
	int ld_fd = 0;
#endif

	do{
		struct file *fil = ptr->vm_file;
		if(fil != NULL){
			char* name = fil->f_path.dentry->d_iname;
			if(name && strcmp(name, LD_NAME)==0){
				ld_regions[ld_region_num].region_start = ptr->vm_start;
				ld_regions[ld_region_num].region_end = ptr->vm_end;					

				if(ld_region_num==0)
					ld_regions[ld_region_num].prot = PROT_READ|PROT_EXEC;
				else if(ld_region_num==1)
					ld_regions[ld_region_num].prot = PROT_READ|PROT_WRITE;
				else 
					ld_regions[ld_region_num].prot = 0;
				
				ld_regions[ld_region_num].flags = MAP_PRIVATE|MAP_DENYWRITE|MAP_FIXED;
				ld_regions[ld_region_num].off = ptr->vm_pgoff<<PAGE_SHIFT;
				ld_regions[ld_region_num].file_ptr = fil;
				ld_region_num++;
				
				ld_bss_start = ptr->vm_end;
			}

			if(name && is_monitor_app(name) && ptr->vm_pgoff==0){//main file executable region
				if(main_file_sec_no==0){
					cc_ret = allocate_cc_fixed(ptr->vm_start, ptr->vm_end, name);
					//get main program entry
					program_entry_addr = (long*)((long)&((Elf64_Ehdr *)0)->e_entry + ptr->vm_start);
					get_user(program_entry, program_entry_addr);
					PRINTK("[LKM:%d]entry=0x%lx\n", current->pid, program_entry);
					//set_checkpoint(current, program_entry, ptr->vm_start, ptr->vm_end);
				}
				main_file_sec_no++;
			}
		}

		if(ptr->vm_start==ld_bss_start){
			ld_regions[ld_region_num].region_start = ptr->vm_start;
			ld_regions[ld_region_num].region_end = ptr->vm_end;					
			ld_regions[ld_region_num].prot = PROT_READ|PROT_WRITE;
			
			ld_regions[ld_region_num].flags = MAP_PRIVATE|MAP_DENYWRITE|MAP_FIXED;
			ld_regions[ld_region_num].off = ptr->vm_pgoff<<PAGE_SHIFT;
			ld_regions[ld_region_num].file_ptr = fil;
			ld_region_num++;
		}

		//find stack
		if((ptr->vm_flags&VM_STACK_FLAGS) == VM_STACK_FLAGS){
			map_addr = allocate_ss_fixed(ptr->vm_start, ptr->vm_end);
			//printk(KERN_ERR "shadow stack  = %08lx\n", map_addr);
		} 
		ptr = ptr->vm_next;
		if(ptr == NULL) break;
	} while (ptr != list);
	//unmap all ld.so
	//code cache map
	if(ld_regions[0].prot&PROT_EXEC){
		cc_ret = allocate_cc(ld_regions[0].region_end-ld_regions[0].region_start, LD_NAME);
		ld_offset = ld_regions[0].region_start - (cc_ret-CC_OFFSET);
	}else
		PRINTK("ld.so code cache find failed!\n");
#ifdef _C10	
	//remap the ld.so
	ld_path = dentry_path_raw(ld_regions[index].file_ptr->f_path.dentry, buf, 256);
#endif	
	for(index=0; index<ld_region_num; index++){
#ifdef _C10		
		ld_fd = open_elf(ld_path);
		map_addr = orig_mmap(ld_regions[index].region_start-ld_offset, ld_regions[index].region_end-ld_regions[index].region_start,\
			ld_regions[index].prot, ld_regions[index].flags, ld_fd, ld_regions[index].off);
		close_elf(ld_fd);
		orig_munmap(ld_regions[index].region_start, ld_regions[index].region_end-ld_regions[index].region_start);
#else
		map_addr = vm_mmap(ld_regions[index].file_ptr, ld_regions[index].region_start-ld_offset,
			ld_regions[index].region_end-ld_regions[index].region_start, ld_regions[index].prot,
				ld_regions[index].flags, ld_regions[index].off);
		vm_munmap(ld_regions[index].region_start, ld_regions[index].region_end-ld_regions[index].region_start);
#endif
		
	}
	//scan the aux in the stack and replace the base of interpreter
	replace_at_base_in_aux(ld_regions[0].region_start, ld_regions[index].region_start-ld_offset, current);
	
	regs->ip -= ld_offset;
}

asmlinkage long intercept_mmap(ulong addr, ulong len, ulong prot, ulong flags, ulong fd, ulong pgoff){
	long ret = 0;
	long cc_ret = 0;
	//long ss_ret = 0;
	int procid = current->pid;
	// insert the new element into the mapping table
	if(is_monitor_app(current->comm)){
		if((prot & PROT_EXEC) && ((int)fd)>0 && addr==0){
			// 1.allocate code cache
				// 1.1 get x region file name
			struct file *fil = fget(fd);
			char* name = fil->f_path.dentry->d_iname;			
			cc_ret = allocate_cc(len, name);
			// 2.execute the original mmap, but the start is fixed with CC_OFFSET
			ret = orig_mmap(cc_ret-CC_OFFSET, len, prot, flags|MAP_FIXED, fd, pgoff);
			// 3.judge the code cache is allocate correctly
			if(cc_ret==0 || ret==0){
				PRINTK("[LKM]allocate code cache error!(mmap: cc_ret=%lx, ret=%lx)\n", cc_ret, ret);
				ret = orig_mmap(addr, len, prot, flags, fd, pgoff);
			}else{
			    PRINTK("[ORG:%d]mmap(addr:%lx, len:%lx, prot:%d, flags:%d, fd:%d, off:%lx)= %lx\n", \
			    	procid, addr, len, (int)prot, (int)flags, (int)fd, pgoff, ret);
			}
		}else if((flags&MAP_STACK) && addr==0){
			ret = orig_mmap(addr, len, prot, flags, fd, pgoff);
			//ss_ret = allocate_ss_fixed(ret, ret+len);
			//PRINTK("[LKM]allocate child shadow stack (%lx)\n", ss_ret);
		}else
			ret = orig_mmap(addr, len, prot, flags, fd, pgoff);
	}else
		ret = orig_mmap(addr, len, prot, flags, fd, pgoff);
	
	return ret;
	
}


asmlinkage long intercept_execve(const char __user* filename, const char __user* const __user* argv,\
                    const char __user* const __user* envp)
{
	int ret = 0;
	
	if(is_monitor_app(current->comm)){
		PRINTK("[LKM]execve(%s)\n", current->comm);
		//TODO: send mesg to setup shuffle process
	}
	
	ret = orig_execve(filename, argv, envp);

	if(is_monitor_app(current->comm)){
		init_app_slot(current);
		remmap_interp_and_allocate_cc();
	}
	return ret;
}

asmlinkage long intercept_exit_group(ulong error)
{	
	char *start_encode;
	if(is_monitor_app(current->comm)){
		start_encode = is_checkpoint(current);
		
		if(start_encode){
			return set_program_start(current, start_encode);
		}else{
			PRINTK("[LKM]exit_group(%s)\n", current->comm);
			free_app_slot(current);
		}
	}

	return orig_exit_group(error);
}


