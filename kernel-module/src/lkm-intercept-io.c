#include <linux/unistd.h>
#include <linux/sched.h>

#include "lkm-utility.h"
#include "lkm-monitor.h"
#include "lkm-hook.h"



#define INTERCEPT_FUNC_DEF(name) void intercept_##name (void)

//read/write
INTERCEPT_FUNC_DEF(read)
{
	if(is_monitor_app(current->comm)){
		//PRINTK("[%d, %d] read syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_read, 1);
	}

	return ;
}

INTERCEPT_FUNC_DEF(write)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_read)==1){
			printk(KERN_ERR "[%d, %d] write syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] write syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_read, 0);
	}

	return ;
}
//pread64/pwrite64
INTERCEPT_FUNC_DEF(pread64)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" pread64 syscall\n");
		set_io_in(current, __NR_pread64, 1);
	}

	return ;
}

INTERCEPT_FUNC_DEF(pwrite64)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_pread64)==1){
			printk(KERN_ERR "[%d, %d] pwrite64 syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] pwrite64 syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_pread64, 0);
	}
	
	return ;
}
//readv/writev
INTERCEPT_FUNC_DEF(readv)
{
	if(is_monitor_app(current->comm)){
		//PRINTK("readv syscall\n");
		set_io_in(current, __NR_readv, 1);
	}

	return ;
}

INTERCEPT_FUNC_DEF(writev)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_readv)==1){
			printk(KERN_ERR "[%d, %d] writev syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK([%d, %d] writev syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_readv, 0);
	}
	
	return ;
}
//recvfrom/sendto
INTERCEPT_FUNC_DEF(recvfrom)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" recvfrom syscall\n");
		set_io_in(current, __NR_recvfrom, 1);
	}

	return ;
}

INTERCEPT_FUNC_DEF(sendto)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_recvfrom)==1){
			printk(KERN_ERR "[%d, %d] sendto syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] sendto syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_recvfrom, 0);
	}

	return ;
}
//recvmsg/sendmsg
INTERCEPT_FUNC_DEF(recvmsg)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" recvmsg syscall\n");
		set_io_in(current, __NR_recvmsg, 1);
	}

	return ;
}

INTERCEPT_FUNC_DEF(sendmsg)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_recvmsg)==1){
			printk(KERN_ERR "[%d, %d] sendmsg syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] sendmsg syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_recvmsg, 0);
	}

	return ;
}
//preadv/pwritev
INTERCEPT_FUNC_DEF(preadv)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" preadv syscall\n");
		set_io_in(current, __NR_preadv, 1);
	}

	return ;
}

INTERCEPT_FUNC_DEF(pwritev)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_preadv)==1){
			printk(KERN_ERR "[%d, %d] pwritev syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] pwritev syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_preadv, 0);
	}

	return ;
}
//mq_timedreceive/mq_timedsend
INTERCEPT_FUNC_DEF(mq_timedreceive)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" mq_timedreceive syscall\n");
		set_io_in(current, __NR_mq_timedreceive, 1);
	}

	return ;
}

INTERCEPT_FUNC_DEF(mq_timedsend)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_mq_timedreceive)==1){
			printk(KERN_ERR "[%d, %d] mq_timedsend syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] mq_timedsend syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_mq_timedreceive, 0);
	}
	
	return ;
}

