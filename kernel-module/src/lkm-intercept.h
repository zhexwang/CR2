#ifndef _LKM_INTERCEPT_H_
#define _LKM_INTERCEPT_H_


//----------------intercept function used by modules----------------------//
extern asmlinkage long intercept_mmap(ulong addr, ulong len, ulong prot, ulong flags, ulong fd, ulong pgoff);
extern asmlinkage long intercept_execve(const char __user* filename, const char __user* const __user* argv,\
                    const char __user* const __user* envp);
extern asmlinkage long intercept_exit_group(ulong error);
extern asmlinkage long intercept_execve(const char __user* filename, const char __user* const __user* argv,\
                    const char __user* const __user* envp);


//-------------IO pair syscall intercept function-------------//
#define EXTERN_INTERCPET_FUNC(name) extern void intercept_##name (void)

EXTERN_INTERCPET_FUNC(read);
EXTERN_INTERCPET_FUNC(write);
EXTERN_INTERCPET_FUNC(pread64);
EXTERN_INTERCPET_FUNC(pwrite64);
EXTERN_INTERCPET_FUNC(readv);
EXTERN_INTERCPET_FUNC(writev);
EXTERN_INTERCPET_FUNC(recvfrom);
EXTERN_INTERCPET_FUNC(sendto);
EXTERN_INTERCPET_FUNC(recvmsg);
EXTERN_INTERCPET_FUNC(sendmsg);
EXTERN_INTERCPET_FUNC(preadv);
EXTERN_INTERCPET_FUNC(pwritev);
EXTERN_INTERCPET_FUNC(mq_timedreceive);
EXTERN_INTERCPET_FUNC(mq_timedsend);

//-------------Signal syscall intercept function-------------//


extern asmlinkage long intercept_sigaltstack(const struct sigaltstack __user *uss, struct sigaltstack __user *uoss);


#endif
