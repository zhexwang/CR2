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
		if(need_rerandomization(current)){
			PRINTK("[%d, %d] read syscall need rerandomization\n", current->pid, pid_vnr(task_pgrp(current)));
			rerandomization(current);
			clear_rerand_record(current);
		}else
			;//PRINTK("[%d, %d] read syscall\n", current->pid, pid_vnr(task_pgrp(current)));
	}

	return ;
}

asmlinkage long intercept_write(unsigned int fd, const char __user *buf, size_t count)
{
	if(is_monitor_app(current->comm)){
		PRINTK("[%d, %d] write syscall (size=%ld)\n", current->pid, pid_vnr(task_pgrp(current)), count);
		set_output_syscall(current, count);
	}
	
	return 0;	
}

//pread64/pwrite64
INTERCEPT_FUNC_DEF(pread64)
{
	if(is_monitor_app(current->comm)){
		if(need_rerandomization(current)){
			PRINTK("[%d, %d] pread64 syscall need rerandomization\n", current->pid, pid_vnr(task_pgrp(current)));
			rerandomization(current);
			clear_rerand_record(current);
		}else
			;//PRINTK("[%d, %d] pread64 syscall\n", current->pid, pid_vnr(task_pgrp(current)));
	}
		
	return ;
}

asmlinkage long intercept_pwrite64(unsigned int fd, const char __user *buf, size_t count, loff_t pos)
{
	if(is_monitor_app(current->comm)){
		PRINTK("[%d, %d] pwrite64 syscall (size=%ld)\n", current->pid, pid_vnr(task_pgrp(current)), count);
		set_output_syscall(current, count);
	}
	
	return 0;

}

//readv/writev
INTERCEPT_FUNC_DEF(readv)
{
	if(is_monitor_app(current->comm)){
		if(need_rerandomization(current)){
			PRINTK("[%d, %d] readv syscall need rerandomization\n", current->pid, pid_vnr(task_pgrp(current)));
			rerandomization(current);
			clear_rerand_record(current);
		}else
			;//PRINTK("[%d, %d] readv syscall\n", current->pid, pid_vnr(task_pgrp(current)));
	}
	
	return ;
}

asmlinkage long intercept_writev(unsigned long fd, const struct iovec __user *vec, unsigned long vlen)
{
	long transfer_size = 0;
	int idx = 0;
	if(is_monitor_app(current->comm)){
		for(idx=0; idx<vlen; idx++){
			transfer_size += vec[idx].iov_len;
		}
		PRINTK("[%d, %d] writev syscall (size=%ld)\n", current->pid, pid_vnr(task_pgrp(current)), transfer_size);
		set_output_syscall(current, transfer_size);
	}
	
	return 0;

}

//recvfrom/sendto
INTERCEPT_FUNC_DEF(recvfrom)
{
	if(is_monitor_app(current->comm)){
		if(need_rerandomization(current)){
			PRINTK("[%d, %d] recvfrom syscall need rerandomization\n", current->pid, pid_vnr(task_pgrp(current)));
			rerandomization(current);
			clear_rerand_record(current);
		}else
			;//PRINTK("[%d, %d] recvfrom syscall\n", current->pid, pid_vnr(task_pgrp(current)));
	}
	
	return ;
}

asmlinkage long intercept_sendto(int arg1, void __user * arg2, size_t arg3, unsigned arg4, struct sockaddr __user * arg5, int arg6)
{
	if(is_monitor_app(current->comm)){
		PRINTK("[%d, %d] sendto syscall (size=%ld)\n", current->pid, pid_vnr(task_pgrp(current)), arg3);
		set_output_syscall(current, arg3);
	}

	return 0;

}

//recvmsg/sendmsg
INTERCEPT_FUNC_DEF(recvmsg)
{
	if(is_monitor_app(current->comm)){
		if(need_rerandomization(current)){
			PRINTK("[%d, %d] recvmsg syscall need rerandomization\n", current->pid, pid_vnr(task_pgrp(current)));
			rerandomization(current);
			clear_rerand_record(current);
		}else
			;//PRINTK("[%d, %d] recvmsg syscall\n", current->pid, pid_vnr(task_pgrp(current)));
	}

	return ;
}

asmlinkage long intercept_sendmsg(int fd, struct msghdr __user *msg, unsigned flags)
{
	long transfer_size = 0;
	int idx = 0;

	if(is_monitor_app(current->comm)){
		for(idx=0; idx<msg->msg_iovlen; idx++){
			transfer_size += (msg->msg_iov)[idx].iov_len;
		}
		PRINTK("[%d, %d] sendmsg syscall (size=%ld)\n", current->pid, pid_vnr(task_pgrp(current)), transfer_size);
		set_output_syscall(current, transfer_size);
	}

	return 0;

}

//preadv/pwritev
INTERCEPT_FUNC_DEF(preadv)
{
	if(is_monitor_app(current->comm)){
		if(need_rerandomization(current)){
			PRINTK("[%d, %d] preadv syscall need rerandomization\n", current->pid, pid_vnr(task_pgrp(current)));
			rerandomization(current);
			clear_rerand_record(current);
		}else
			;//PRINTK("[%d, %d] preadv syscall\n", current->pid, pid_vnr(task_pgrp(current)));
	}

	return ;
}

asmlinkage long intercept_pwritev(unsigned long fd, const struct iovec __user *vec, unsigned long vlen, unsigned long pos_l, \
	unsigned long pos_h)
{
	long transfer_size = 0;
	int idx = 0;

	if(is_monitor_app(current->comm)){
		for(idx=0; idx<vlen; idx++){
			transfer_size += vec[idx].iov_len;
		}
		PRINTK("[%d, %d] pwritev syscall (size=%ld)\n", current->pid, pid_vnr(task_pgrp(current)), transfer_size);
		set_output_syscall(current, transfer_size);
	}

	return 0;

}

//mq_timedreceive/mq_timedsend
INTERCEPT_FUNC_DEF(mq_timedreceive)
{
	if(is_monitor_app(current->comm)){
		if(need_rerandomization(current)){
			PRINTK("[%d, %d] mq_timedreceive syscall need rerandomization\n", current->pid, pid_vnr(task_pgrp(current)));
			rerandomization(current);
			clear_rerand_record(current);
		}else
			;//PRINTK("[%d, %d] mq_timedreceive syscall\n", current->pid, pid_vnr(task_pgrp(current)));
	}
	return ;
}

asmlinkage long intercept_mq_timedsend(mqd_t mqdes, const char __user *msg_ptr, size_t msg_len, unsigned int msg_prio, \
	const struct timespec __user *abs_timeout)
{
	if(is_monitor_app(current->comm)){
		PRINTK("[%d, %d] mq_timedsend syscall (size=%ld)\n", current->pid, pid_vnr(task_pgrp(current)), msg_len);
		set_output_syscall(current, msg_len);
	}

	return 0;

}

