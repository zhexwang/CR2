#ifndef _LKM_CONFIG_H_
#define _LKM_CONFIG_H_

#include <linux/kernel.h>
/***************CR2 args******************/
#define CC_OFFSET (1ul<<30)
#define CC_MULTIPULE (8)
#define SS_OFFSET (1ul<<30)
#define SS_MULTIPULE (20)

/*********When you change machine, you should modify the below info ***********/
#ifdef _VM
	const static ulong orig_sys_call_table = (0xffffffff81801400);		// the system call table
	#define LD_NAME "ld-2.19.so"
#else
	#ifdef _C10
		const static ulong orig_sys_call_table = (0xffffffff81801300);		// the system call table
		#define LD_NAME "ld-2.15.so"
	#else
		#error unkown os version
	#endif
#endif

#define PAGE_COPY_EXECV (0x25LLU)

#define _START_RAX 0x1c
#define CHECK_ENCODE_LEN 7

#define MAX_APP_LIST_NUM 20
//we only support run less than 4 same protected application at the same time
#define MAX_APP_SLOT_LIST_NUM  MAX_APP_LIST_NUM*4

#define TRACE_DEBUG
#endif
