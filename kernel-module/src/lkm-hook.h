#ifndef _LKM_HOOK_H_
#define _LKM_HOOK_H_

#include <asm/page.h>
#include <linux/socket.h>
#include <linux/signal.h>

// ---------- hack function typdef------------- //
typedef asmlinkage long (*MMAP_FUNC_TYPE)(ulong addr, ulong len, ulong prot, ulong flags, ulong fd, ulong pgoff);
typedef asmlinkage long (*MUNMAP_FUNC_TYPE)(ulong addr, ulong len);
typedef asmlinkage long (*OPEN_FUNC_TYPE)(const char __user *filename, int flags, umode_t mode);
typedef asmlinkage long (*EXECVE_FUNC_TYPE)(const char __user* filename, const char __user* const __user* argv,\
                    const char __user* const __user* envp);
typedef asmlinkage long (*FTRUNCATE_FUNC_TYPE)(unsigned int fd, unsigned long length);
typedef asmlinkage long (*ARCH_PRCTL_FUNC_TYPE)(int code, unsigned long addr);
typedef asmlinkage long (*CLOSE_FUNC_TYPE)(unsigned int fd);
typedef asmlinkage long (*EXIT_GROUP_FUNC_TYPE)(ulong error);
typedef asmlinkage long (*MPROTECT_FUNC_TYPE)(unsigned long start, size_t len, unsigned long prot);
typedef asmlinkage long (*MUNMAP_FUNC_TYPE)(unsigned long addr, size_t len);

extern MMAP_FUNC_TYPE orig_mmap;
extern OPEN_FUNC_TYPE orig_open;
extern EXECVE_FUNC_TYPE orig_execve;
extern FTRUNCATE_FUNC_TYPE orig_ftruncate;
extern ARCH_PRCTL_FUNC_TYPE orig_arch_prctl;
extern void *orig_stub_execve;
extern CLOSE_FUNC_TYPE orig_close;
extern EXIT_GROUP_FUNC_TYPE orig_exit_group;
extern MPROTECT_FUNC_TYPE orig_mprotect;
extern MUNMAP_FUNC_TYPE orig_munmap;
//
typedef asmlinkage long (*UMASK_FUNC_TYPE)(int mask);
extern UMASK_FUNC_TYPE orig_umask;

// ---------------IO pair syscall-----------------------//
typedef asmlinkage long (*READ_FUNC_TYPE)(unsigned int fd, char __user *buf, size_t count);
typedef asmlinkage long (*WRITE_FUNC_TYPE)(unsigned int fd, const char __user *buf, size_t count);

extern READ_FUNC_TYPE orig_read;
extern WRITE_FUNC_TYPE orig_write;

typedef asmlinkage long (*PREAD64_FUNC_TYPE)(unsigned int fd, char __user *buf, size_t count, loff_t pos);
typedef asmlinkage long (*PWRITE64_FUNC_TYPE)(unsigned int fd, const char __user *buf, size_t count, loff_t pos);

extern PREAD64_FUNC_TYPE orig_pread64;
extern PWRITE64_FUNC_TYPE orig_pwrite64;

typedef asmlinkage long (*READV_FUNC_TYPE)(unsigned long fd, const struct iovec __user *vec, unsigned long vlen);
typedef asmlinkage long (*WRITEV_FUNC_TYPE)(unsigned long fd, const struct iovec __user *vec, unsigned long vlen);

extern READV_FUNC_TYPE orig_readv;
extern WRITEV_FUNC_TYPE orig_writev;

typedef asmlinkage long (*RECVFROM_FUNC_TYPE)(int, void __user *, size_t, unsigned, struct sockaddr __user *, int __user *);
typedef asmlinkage long (*SENDTO_FUNC_TYPE)(int, void __user *, size_t, unsigned, struct sockaddr __user *, int);

extern RECVFROM_FUNC_TYPE orig_recvfrom;
extern SENDTO_FUNC_TYPE orig_sendto;

typedef asmlinkage long (*RECVMSG_FUNC_TYPE)(int fd, struct msghdr __user *msg, unsigned flags);
typedef asmlinkage long (*SENDMSG_FUNC_TYPE)(int fd, struct msghdr __user *msg, unsigned flags);

extern RECVMSG_FUNC_TYPE orig_recvmsg;
extern SENDMSG_FUNC_TYPE orig_sendmsg;

typedef asmlinkage long (*PREADV_FUNC_TYPE)(unsigned long fd, const struct iovec __user *vec, unsigned long vlen, \
	unsigned long pos_l, unsigned long pos_h);
typedef asmlinkage long (*PWRITEV_FUNC_TYPE)(unsigned long fd, const struct iovec __user *vec, unsigned long vlen, unsigned long pos_l, \
	unsigned long pos_h);

extern PREADV_FUNC_TYPE orig_preadv;
extern PWRITEV_FUNC_TYPE orig_pwritev;

typedef asmlinkage long (*MQTIMEDRECEIVE_FUNC_TYPE)(mqd_t mqdes, char __user *msg_ptr, size_t msg_len, unsigned int __user *msg_prio, \
	const struct timespec __user *abs_timeout);
typedef asmlinkage long (*MQTIMEDSEND_FUNC_TYPE)(mqd_t mqdes, const char __user *msg_ptr, size_t msg_len, unsigned int msg_prio, \
	const struct timespec __user *abs_timeout);

extern MQTIMEDRECEIVE_FUNC_TYPE orig_mq_timedreceive;
extern MQTIMEDSEND_FUNC_TYPE orig_mq_timedsend;


// ---------------Signal syscall-----------------------//

typedef asmlinkage long (*RTSIGACTION_FUNC_TYPE)(int sig, const struct sigaction __user *act, struct sigaction __user *oact, size_t sigsetsize); 
#ifdef _C10
typedef asmlinkage long (*SIGALTSTACK_FUNC_TYPE)(const stack_t __user *uss, stack_t __user *uoss, long arg2, long arg3, long arg4, long arg5, \
	long arg6, long arg7, struct pt_regs regs);
#else
typedef asmlinkage long (*SIGALTSTACK_FUNC_TYPE)(const struct sigaltstack __user *uss, struct sigaltstack __user *uoss);
#endif

extern RTSIGACTION_FUNC_TYPE orig_rt_sigaction;
extern SIGALTSTACK_FUNC_TYPE orig_sigaltstack;

//new kernel versioin does not use signal system call
//typedef asmlinkage long (*SIGNAL_FUNC_TYPE)(int sig, __sighandler_t handler);
//extern SIGNAL_FUNC_TYPE orig_signal;

// ---------------fork/vfork/clone syscall-----------------------//

#ifdef _C10
typedef asmlinkage long (*CLONE_FUNC_TYPE)(unsigned long clone_flags, unsigned long newsp, int __user * parent_tidptr,
	int __user * child_tidptr, unsigned long tls_val);
#else
typedef asmlinkage long (*CLONE_FUNC_TYPE)(unsigned long clone_flags, unsigned long newsp, int __user * parent_tidptr,
	int __user * child_tidptr, int tls_val);
#endif
extern CLONE_FUNC_TYPE orig_clone;

//--------------------setsid syscall-----------------------------//
typedef asmlinkage long (*SETSID_FUNC_TYPE)(void);
extern SETSID_FUNC_TYPE orig_setsid;


extern void hook_systable(void);
extern void stop_hook(void);
extern void init_orig_syscall(void);
#endif
