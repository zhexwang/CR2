#ifndef _LKM_MONITOR_H_
#define _LKM_MONITOR_H_

#include <linux/module.h>


extern void init_shuffle_config_list(void);
extern void insert_shuffle_info(char monitor_list_idx,int shuffle_pid);
extern void free_one_shuffle_info(char monitor_list_idx, int shuffle_pid);
extern 	int connect_one_shuffle(char monitor_list_idx, char app_slot_idx);
extern char get_app_slot_idx_from_shuffle_config(char monitor_list_idx, int shuffle_pid);
extern char is_monitor_app(const char *name);
extern void init_monitor_app_list(void);

extern int get_cc_id(struct task_struct *ts);
extern char init_app_slot(struct task_struct *ts);
extern void free_app_slot(struct task_struct *ts);
extern void set_io_in(struct task_struct *ts, int sys_no, char value);
extern char has_io_in(struct task_struct *ts, int sys_no);
extern char *get_start_encode_and_set_entry(struct task_struct *ts, long program_entry);
extern char *is_checkpoint(struct task_struct *ts, char *app_slot_idx);
extern void set_shuffle_pid(char app_slot_idx, int shuffle_pid);
extern char get_app_slot_idx(int tgid);
extern void set_shuffle_pc(char app_slot_idx, ulong pc);
extern ulong get_shuffle_pc(char app_slot_idx);
extern volatile char *get_start_flag(char app_slot_idx);
extern int get_shuffle_pid(char app_slot_idx);
extern long set_app_start(struct task_struct *ts);
extern void insert_x_info(struct task_struct *ts, long cc_start, long cc_end, const char *file);
extern void insert_stack_info(struct task_struct *ts, long ss_start, long ss_end, const char *file);

extern void rerandomization(struct task_struct *ts);
extern void stop_all(void);
extern void wake_all(void);
#endif
