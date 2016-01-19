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
		if(has_io_in(current, __NR_write)==1){
			PRINTK("[%d, %d] read syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			//PRINTK("[%d, %d] read syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_write, 0);
	}

	return ;
}

INTERCEPT_FUNC_DEF(write)
{
	if(is_monitor_app(current->comm)){
		//PRINTK("[%d, %d] write syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_write, 1);
	}

	return ;

}
//pread64/pwrite64
INTERCEPT_FUNC_DEF(pread64)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_pwrite64)==1){
			PRINTK("[%d, %d] pread64 syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] pread64 syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_pwrite64, 0);
	}
	
	return ;
}

INTERCEPT_FUNC_DEF(pwrite64)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" pwrite64 syscall\n");
		set_io_in(current, __NR_pwrite64, 1);
	}
	
	return ;
}
//readv/writev
INTERCEPT_FUNC_DEF(readv)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_writev)==1){
			PRINTK("[%d, %d] readv syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK([%d, %d] readv syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_writev, 0);
	}
	
	return ;
}

INTERCEPT_FUNC_DEF(writev)
{
	if(is_monitor_app(current->comm)){
		//PRINTK("writev syscall\n");
		set_io_in(current, __NR_writev, 1);
	}

	return ;
}
//recvfrom/sendto
INTERCEPT_FUNC_DEF(recvfrom)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_sendto)==1){
			PRINTK("[%d, %d] recvfrom syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] recvfrom syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_sendto, 0);
	}

	return ;
}

INTERCEPT_FUNC_DEF(sendto)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" sendto syscall\n");
		set_io_in(current, __NR_sendto, 1);
	}

	return ;
}
//recvmsg/sendmsg
INTERCEPT_FUNC_DEF(recvmsg)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_sendmsg)==1){
			PRINTK("[%d, %d] recvmsg syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] recvmsg syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_sendmsg, 0);
	}

	return ;
}

INTERCEPT_FUNC_DEF(sendmsg)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" sendmsg syscall\n");
		set_io_in(current, __NR_sendmsg, 1);
	}

	return ;
}
//preadv/pwritev
INTERCEPT_FUNC_DEF(preadv)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_pwritev)==1){
			PRINTK("[%d, %d] preadv syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] preadv syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_pwritev, 0);
	}

	return ;
}

INTERCEPT_FUNC_DEF(pwritev)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" pwritev syscall\n");
		set_io_in(current, __NR_pwritev, 1);
	}

	return ;
}
//mq_timedreceive/mq_timedsend
INTERCEPT_FUNC_DEF(mq_timedreceive)
{
	if(is_monitor_app(current->comm)){
		if(has_io_in(current, __NR_mq_timedsend)==1){
			PRINTK("[%d, %d] mq_timedreceive syscall need rerandomization\n", current->pid, current->tgid);
			rerandomization(current);
		}else
			;//PRINTK("[%d, %d] mq_timedreceive syscall\n", current->pid, current->tgid);
		set_io_in(current, __NR_mq_timedsend, 0);
	}
	
	return ;
}

INTERCEPT_FUNC_DEF(mq_timedsend)
{
	if(is_monitor_app(current->comm)){
		//PRINTK(" mq_timedsend syscall\n");
		set_io_in(current, __NR_mq_timedsend, 1);
	}

	return ;
}

