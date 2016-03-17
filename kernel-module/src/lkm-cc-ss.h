#ifndef _LKM_CC_SS_H_
#define _LKM_CC_SS_H_

extern void free_mem(unsigned long addr, size_t len);
extern long allocate_trace_debug_buffer(ulong buffer_base, ulong buffer_size);
extern long allocate_cc(long orig_x_size, const char *orig_name);
extern long allocate_cc_fixed(long orig_x_start, long orig_x_end, const char *orig_name);
extern long allocate_ss_fixed(long orig_stack_start, long orig_stack_end);
extern long reallocate_ss(long ss_start, long ss_end);

#endif
