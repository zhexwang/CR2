#ifndef _LKM_CONFIG_H_
#define _LKM_CONFIG_H_

#include <linux/kernel.h>
/***************CR2 args******************/
#define CC_OFFSET (1ul<<30)
#define CC_MULTIPULE (4)
#define SS_OFFSET (1ul<<30)
#define SS_MULTIPULE (15)

/*********When you change machine, you should modify the below info ***********/
const static ulong orig_sys_call_table = (0xffffffff81801400);		// the system call table

#define LD_NAME "ld-2.19.so"

#define MAX_APP_LIST_NUM 10
//we only support run less than 4 same protected application at the same time
#define MAX_APP_SLOT_LIST_NUM  MAX_APP_LIST_NUM*4

#endif
