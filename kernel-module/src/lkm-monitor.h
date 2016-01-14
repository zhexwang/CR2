#ifndef _LKM_MONITOR_H_
#define _LKM_MONITOR_H_

#include <linux/module.h>

extern ulong is_monitor_app(const char *name);
extern void init_monitor_app_list(void);

extern void init_app_slot(struct task_struct *ts);
extern void free_app_slot(struct task_struct *ts);
extern void set_io_in(struct task_struct *ts, int sys_no, char value);
extern char has_io_in(struct task_struct *ts, int sys_no);
extern char *get_start_encode_and_set_entry(struct task_struct *ts, long program_entry);
extern char *is_checkpoint(struct task_struct *ts);
extern long set_app_start(struct task_struct *ts);
extern void insert_x_info(struct task_struct *ts, long cc_start, long cc_end, const char *file);
extern void insert_stack_info(struct task_struct *ts, long ss_start, long ss_end, const char *file);

extern void rerandomization(struct task_struct *ts);
#endif