#include <linux/unistd.h>
#include <linux/module.h>

#include "lkm-config.h"
#include "lkm-hook.h"
#include "lkm-intercept.h"
#include "lkm-utility.h"

MMAP_FUNC_TYPE orig_mmap = NULL;
OPEN_FUNC_TYPE orig_open = NULL;
EXECVE_FUNC_TYPE orig_execve = NULL;
FTRUNCATE_FUNC_TYPE orig_ftruncate = NULL;
ARCH_PRCTL_FUNC_TYPE orig_arch_prctl = NULL;
void *orig_stub_execve = NULL;
CLOSE_FUNC_TYPE orig_close = NULL;
EXIT_GROUP_FUNC_TYPE orig_exit_group = NULL;
MPROTECT_FUNC_TYPE orig_mprotect = NULL;
MUNMAP_FUNC_TYPE orig_munmap = NULL;

//real IO pair syscall
READ_FUNC_TYPE orig_read = NULL;
WRITE_FUNC_TYPE orig_write = NULL;

PREAD64_FUNC_TYPE orig_pread64 = NULL;
PWRITE64_FUNC_TYPE orig_pwrite64 = NULL;

READV_FUNC_TYPE orig_readv = NULL;
WRITEV_FUNC_TYPE orig_writev = NULL;

RECVFROM_FUNC_TYPE orig_recvfrom = NULL;
SENDTO_FUNC_TYPE orig_sendto = NULL;

RECVMSG_FUNC_TYPE orig_recvmsg = NULL;
SENDMSG_FUNC_TYPE orig_sendmsg = NULL;

PREADV_FUNC_TYPE orig_preadv = NULL;
PWRITEV_FUNC_TYPE orig_pwritev = NULL;

MQTIMEDRECEIVE_FUNC_TYPE orig_mq_timedreceive = NULL;
MQTIMEDSEND_FUNC_TYPE orig_mq_timedsend = NULL;

//signal
RTSIGACTION_FUNC_TYPE orig_rt_sigaction = NULL;
void *orig_stub_sigaltstack = NULL;
SIGALTSTACK_FUNC_TYPE orig_sigaltstack = NULL;

/***********************System call table operation*************************/
// ---------- write to read-only memory with CR0-WP bit manipulation
static inline unsigned long readcr0(void) {
	unsigned long val;
	asm volatile("mov %%cr0,%0 \n\t" : "=r" (val));
	return val;
}

static inline void writecr0(unsigned long val) {
	asm volatile("mov %0,%%cr0 \n\t" : : "r" (val));
}

void rewrite_systable_entry(ulong index, void *handler_addr)
{
	unsigned long cr0;
	ulong *table = (ulong*)orig_sys_call_table;

	cr0 = readcr0();
	writecr0(cr0 & ~X86_CR0_WP);

	/* replace the syscall handler to the new one */
	//ensure 64bit mov with only one instruction
	asm volatile("movq %1, %0 \n\t" :"=m"(*(table + index)) :"r"(handler_addr), "m"(*(table + index)));

	// change back, to enforce read-only
	writecr0(cr0); 
	return ;
}

static inline void *get_orig_syscall_addr(ulong index)
{
	ulong *table = (ulong*)orig_sys_call_table;
	return (void*)(*(table + index));
}
//-----------Sys stub sigaltstack special handling----------------//

static long ss_call_proceeded_addr = 0;
static long ss_internal_call_addr = 0;
#ifdef _C10
	int ss_call_proceded_encode = 0x000028e9;
	int ss_internal_call_proceded_encode = 0x24548d48;
#endif

static void *get_orig_sigaltstack_from_stub_sigaltstack(void *stub_sigaltstack) 
{
	int call_offset = 0, internal_call_offset = 0;
	
	char *pos = (char*)stub_sigaltstack;
	long ss_internal_call_proceeded_addr = 0;
	//int count = 0;//(use to dump)
	while(*(int*)pos!=ss_call_proceded_encode){
		if(*(int*)pos==ss_internal_call_proceded_encode)
			ss_internal_call_proceeded_addr = (long)pos;
		pos++;
		//count++;
		//if(count==8){
		//	PRINTK("0x%lx\n", *(long*)pos);
		//	count=0;
		//}
	}
	//PRINTK("0x%lx, %d\n", *(long*)((long)pos-count), count);
	ss_call_proceeded_addr = (long)pos;
	call_offset = *(int*)(ss_call_proceeded_addr-4);
	internal_call_offset = *(int*)(ss_internal_call_proceeded_addr-4);
	
	ss_internal_call_addr = ss_internal_call_proceeded_addr+(long)internal_call_offset;
	//PRINTK("internal call addr : %lx\n", ss_internal_call_addr);
	return (void*)(ss_call_proceeded_addr+(long)call_offset);
}

#ifdef _C10
static long intercept_sigaltstack_addr = (long)intercept_sigaltstack;
#else
//the first instruction of hook_execve is callnext, we should cross this instruction!
static long intercept_sigaltstack_addr = (long)intercept_sigaltstack+5;
#endif
extern void intercept_stub_sigaltstack(void); 
void hook_sigaltstack_template(void)
{
	asm volatile(
			".globl intercept_stub_sigaltstack \n"
			"intercept_stub_sigaltstack:"
			"subq    $0x30, %%rsp  \n\t"
			"callq   *%0 \n\t"
			"lea     0x8(%%rsp),%%rdx \n\t"
			"callq   *%1 \n\t"
			"jmpq    *%2 \n\t"
			:
			:"m"(ss_internal_call_addr), "m"(intercept_sigaltstack_addr), "m"(ss_call_proceeded_addr)
		   );

	return ;
}

//-----------Sys stub execve special handling---------------------//
static long se_call_proceeded_addr = 0;
#ifdef _C10
	int se_call_proceded_encode = 0x249c8b4c;//mov 0x98(%rsp),%r11
#else
	int se_call_proceded_encode = 0x24448948;//movq %rax, RAX(%rsp)
#endif

static void *get_orig_execve_from_stub_execve(void *stub_execve) 
{
	int call_offset = 0;
	
	char *pos = (char*)stub_execve;
	//int count = 0;//(use to dump stub_execve)
	//PRINTK("0x%lx\n", *(long*)pos);
	while(*(int*)pos!=se_call_proceded_encode){
		pos++;
		//count++;
		//if(count==8){
		//	PRINTK("0x%lx\n", *(long*)pos);
		//	count=0;
		//}
	}
	//PRINTK("0x%lx, %d\n", *(long*)((long)pos-count), count);
	se_call_proceeded_addr = (long)pos;
	call_offset = *(int*)(se_call_proceeded_addr-4);
	
	return (void*)(se_call_proceeded_addr+(long)call_offset);
}

#ifdef _C10
static long intercept_execve_addr = (long)intercept_execve;
#else
//the first instruction of hook_execve is callnext, we should cross this instruction!
static long intercept_execve_addr = (long)intercept_execve+5;
#endif
extern void intercept_stub_execve(void); 
void hook_execve_template(void)
{
	asm volatile(
			".globl intercept_stub_execve \n"
			"intercept_stub_execve:"
			"addq    $0x8, %%rsp \n\t"
			"subq    $0x30, %%rsp  \n\t"
			"movq    %%rbx,0x28(%%rsp) \n\t"
			"movq    %%rbp,0x20(%%rsp) \n\t"
			"movq	 %%r12,0x18(%%rsp) \n\t"
			"movq    %%r13,0x10(%%rsp) \n\t"
			"movq    %%r14,0x8(%%rsp) \n\t"
			"movq    %%r15,(%%rsp) \n\t"
#ifdef _C10
			"movq    %%gs:0xbf00,%%r11 \n\t"
#else
			"movq    %%gs:0xbfc0,%%r11 \n\t"
#endif			
			"movq    %%r11,0x98(%%rsp) \n\t"
			"movq    $0x2b,0xa0(%%rsp) \n\t"
			"movq    $0x33,0x88(%%rsp) \n\t"
			"movq    $0xffffffffffffffff,0x58(%%rsp) \n\t"
			"movq    0x30(%%rsp),%%r11 \n\t"
			"movq    %%r11,0x90(%%rsp) \n\t"
#ifdef _C10
			"movq    %%rsp,%%rcx \n\t"
#endif
			"callq   *%0 \n\t"
			"jmpq    *%1 \n\t"
			:
			:"m"(intercept_execve_addr), "m"(se_call_proceeded_addr)
		   );

	return ;
}

//--------------------IO pair syscall hook---------------------//
#define ASM_HOOK_SYSCALL(name) \
	asm volatile( \
		".globl hook_"#name " \n" \
		"hook_"#name ":" \
		"pushq %%rax \n\t" \
		"pushq %%rbx \n\t" \
		"pushq %%rcx \n\t" \
		"pushq %%rdx \n\t" \
		"pushq %%rsi \n\t" \
		"pushq %%rdi \n\t" \
		"pushq %%r8 \n\t" \
		"pushq %%r9 \n\t" \
		"pushq %%r10 \n\t" \
		"pushq %%r11 \n\t" \
		"pushq %%r12 \n\t" \
		"pushq %%r13 \n\t" \
		"pushq %%r14 \n\t" \
		"pushq %%r15 \n\t" \
		"callq *%0 \n\t" \
		"popq %%r15 \n\t" \
		"popq %%r14 \n\t" \
		"popq %%r13 \n\t" \
		"popq %%r12 \n\t" \
		"popq %%r11 \n\t" \
		"popq %%r10 \n\t" \
		"popq %%r9 \n\t" \
		"popq %%r8 \n\t" \
		"popq %%rdi \n\t" \
		"popq %%rsi \n\t" \
		"popq %%rdx \n\t" \
		"popq %%rcx \n\t" \
		"popq %%rbx \n\t" \
		"popq %%rax \n\t" \
		"jmpq *%1 \n\t" \
		: \
		:"m"(new_##name), "m"(orig_##name) \
	)

#define HOOK_FUNC_DEC(name) extern void hook_##name (void); static long new_##name = (long)intercept_##name

HOOK_FUNC_DEC(read);
HOOK_FUNC_DEC(write);
HOOK_FUNC_DEC(pread64);
HOOK_FUNC_DEC(pwrite64);
HOOK_FUNC_DEC(readv);
HOOK_FUNC_DEC(writev);
HOOK_FUNC_DEC(recvfrom);
HOOK_FUNC_DEC(sendto);
HOOK_FUNC_DEC(recvmsg);
HOOK_FUNC_DEC(sendmsg);
HOOK_FUNC_DEC(preadv);
HOOK_FUNC_DEC(pwritev);
HOOK_FUNC_DEC(mq_timedreceive);
HOOK_FUNC_DEC(mq_timedsend);

void asm_code(void)
{
	ASM_HOOK_SYSCALL(read);
	ASM_HOOK_SYSCALL(write);
	ASM_HOOK_SYSCALL(pread64);	
	ASM_HOOK_SYSCALL(pwrite64);
	ASM_HOOK_SYSCALL(readv);
	ASM_HOOK_SYSCALL(writev);
	ASM_HOOK_SYSCALL(recvfrom);
	ASM_HOOK_SYSCALL(sendto);
	ASM_HOOK_SYSCALL(recvmsg);
	ASM_HOOK_SYSCALL(sendmsg);
	ASM_HOOK_SYSCALL(preadv);
	ASM_HOOK_SYSCALL(pwritev);
	ASM_HOOK_SYSCALL(mq_timedreceive);
	ASM_HOOK_SYSCALL(mq_timedsend);
}

//-------------------------------------------------------------//
void init_orig_syscall(void)
{
	//record original syscall addr
	orig_mmap = get_orig_syscall_addr(__NR_mmap);
	orig_open = get_orig_syscall_addr(__NR_open);
	orig_stub_execve = get_orig_syscall_addr(__NR_execve);
	orig_execve = get_orig_execve_from_stub_execve(orig_stub_execve);
	orig_ftruncate = get_orig_syscall_addr(__NR_ftruncate);
	orig_arch_prctl = get_orig_syscall_addr(__NR_arch_prctl);
	orig_close = get_orig_syscall_addr(__NR_close);
	orig_exit_group = get_orig_syscall_addr(__NR_exit_group);
	orig_mprotect = get_orig_syscall_addr(__NR_mprotect);
	orig_munmap = get_orig_syscall_addr(__NR_munmap);
	//IO pair
	orig_read = get_orig_syscall_addr(__NR_read);	
	orig_write = get_orig_syscall_addr(__NR_write);
	orig_pread64 = get_orig_syscall_addr(__NR_pread64);
	orig_pwrite64 = get_orig_syscall_addr(__NR_pwrite64);
	orig_readv = get_orig_syscall_addr(__NR_readv);
	orig_writev = get_orig_syscall_addr(__NR_writev);
	orig_recvfrom = get_orig_syscall_addr(__NR_recvfrom);
	orig_sendto = get_orig_syscall_addr(__NR_sendto);
	orig_recvmsg = get_orig_syscall_addr(__NR_recvmsg);
	orig_sendmsg = get_orig_syscall_addr(__NR_sendmsg);
	orig_preadv = get_orig_syscall_addr(__NR_preadv);
	orig_pwritev = get_orig_syscall_addr(__NR_pwritev);
	orig_mq_timedreceive = get_orig_syscall_addr(__NR_mq_timedreceive);
	orig_mq_timedsend = get_orig_syscall_addr(__NR_mq_timedsend);
	//signal
	orig_rt_sigaction = get_orig_syscall_addr(__NR_rt_sigaction);
	orig_stub_sigaltstack = get_orig_syscall_addr(__NR_sigaltstack);
	orig_sigaltstack = get_orig_sigaltstack_from_stub_sigaltstack(orig_stub_sigaltstack);
	/*
    PRINTK("Origin SyS_mmap: %p\n", orig_mmap);	
	PRINTK("Origin SyS_munmap: %p\n", orig_munmap);	
    PRINTK("Origin SyS_open: %p\n", orig_open);
    PRINTK("Origin stub_execve: %p\n", orig_stub_execve);
	PRINTK("Origin SyS_execve: %p\n", orig_execve);	
	PRINTK("Origin SyS_close: %p\n", orig_close);	
	PRINTK("Origin SyS_exit_group: %p\n", orig_exit_group);	
	PRINTK("Origin SyS_mprotect: %p\n", orig_mprotect);	
	PRINTK("Origin SyS_ftruncate: %p\n", orig_ftruncate);
	PRINTK("Origin SyS_arch_prctl: %p\n", orig_arch_prctl);
    PRINTK("Origin SyS_read: %p\n", orig_read);	
	PRINTK("Origin SyS_write: %p\n", orig_write); 
	PRINTK("Origin SyS_pread64: %p\n", orig_pread64);	
	PRINTK("Origin SyS_pwrite64: %p\n", orig_pwrite64);	
	PRINTK("Origin SyS_readv: %p\n", orig_readv);	
	PRINTK("Origin SyS_writev: %p\n", orig_writev);	
	PRINTK("Origin SyS_ecvfrom: %p\n", orig_recvfrom);	
	PRINTK("Origin SyS_sendto: %p\n", orig_sendto);	
	PRINTK("Origin SyS_recvmsg: %p\n", orig_recvmsg);	
	PRINTK("Origin SyS_sendmsg: %p\n", orig_sendmsg);	
	PRINTK("Origin SyS_preadv: %p\n", orig_preadv);	
	PRINTK("Origin SyS_pwritev: %p\n", orig_pwritev);	
	PRINTK("Origin SyS_mq_timedreceive: %p\n", orig_mq_timedreceive);	
	PRINTK("Origin SyS_mq_timedsend: %p\n", orig_mq_timedsend);	*/
	PRINTK("Origin SyS_rt_sigaction: %p\n", orig_rt_sigaction);	
	PRINTK("Origin SyS_stub_sigaltstack: %p\n", orig_stub_sigaltstack);	
	PRINTK("Origin SyS_sigaltstack: %p\n", orig_sigaltstack);	
}

void hook_systable(void)
{	
	rewrite_systable_entry(__NR_mmap, (void*)intercept_mmap);
	rewrite_systable_entry(__NR_execve, (void*)intercept_stub_execve);
	rewrite_systable_entry(__NR_exit_group, (void*)intercept_exit_group);
	//hacking io pair
	rewrite_systable_entry(__NR_read, (void*)hook_read);
	rewrite_systable_entry(__NR_write, (void*)hook_write);
	rewrite_systable_entry(__NR_pread64, (void*)hook_pread64);
	rewrite_systable_entry(__NR_pwrite64, (void*)hook_pwrite64);
	rewrite_systable_entry(__NR_readv, (void*)hook_readv);
	rewrite_systable_entry(__NR_writev, (void*)hook_writev);
	rewrite_systable_entry(__NR_recvfrom, (void*)hook_recvfrom);
	rewrite_systable_entry(__NR_sendto, (void*)hook_sendto);
	rewrite_systable_entry(__NR_recvmsg, (void*)hook_recvmsg);
	rewrite_systable_entry(__NR_sendmsg, (void*)hook_sendmsg);
	rewrite_systable_entry(__NR_preadv, (void*)hook_preadv);
	rewrite_systable_entry(__NR_pwritev, (void*)hook_pwritev);	
	rewrite_systable_entry(__NR_mq_timedreceive, (void*)hook_mq_timedreceive);	
	rewrite_systable_entry(__NR_mq_timedsend, (void*)hook_mq_timedsend);	
	//hack signal
	rewrite_systable_entry(__NR_sigaltstack, (void*)intercept_stub_sigaltstack);
										
	return ;
}

void stop_hook(void)
{
	//write back the original syscall addr to syscall table
	rewrite_systable_entry(__NR_mmap, (void*)orig_mmap);
	rewrite_systable_entry(__NR_execve, (void*)orig_stub_execve);
	rewrite_systable_entry(__NR_exit_group, (void*)orig_exit_group);
	//write back io pair
	rewrite_systable_entry(__NR_read, (void*)orig_read);
	rewrite_systable_entry(__NR_write, (void*)orig_write);
	rewrite_systable_entry(__NR_pread64, (void*)orig_pread64);
	rewrite_systable_entry(__NR_pwrite64, (void*)orig_pwrite64);
	rewrite_systable_entry(__NR_readv, (void*)orig_readv);
	rewrite_systable_entry(__NR_writev, (void*)orig_writev);
	rewrite_systable_entry(__NR_recvfrom, (void*)orig_recvfrom);
	rewrite_systable_entry(__NR_sendto, (void*)orig_sendto);
	rewrite_systable_entry(__NR_recvmsg, (void*)orig_recvmsg);
	rewrite_systable_entry(__NR_sendmsg, (void*)orig_sendmsg);
	rewrite_systable_entry(__NR_preadv, (void*)orig_preadv);
	rewrite_systable_entry(__NR_pwritev, (void*)orig_pwritev);	
	rewrite_systable_entry(__NR_mq_timedreceive, (void*)orig_mq_timedreceive);	
	rewrite_systable_entry(__NR_mq_timedsend, (void*)orig_mq_timedsend);
	//write back signal
	rewrite_systable_entry(__NR_sigaltstack, (void*)orig_stub_sigaltstack);
	return ;	
}

