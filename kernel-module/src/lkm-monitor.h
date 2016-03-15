#ifndef _LKM_MONITOR_H_
#define _LKM_MONITOR_H_

#include <linux/module.h>

extern void init_shuffle_config_list(void);
extern void insert_shuffle_info(char monitor_list_idx,int shuffle_pid);
extern void free_one_shuffle_info(char monitor_list_idx, int shuffle_pid);
extern 	int connect_one_shuffle(char monitor_list_idx, char app_slot_idx);
extern int  has_free_shuffle(char monitor_list_idx, char app_slot_idx);
extern char get_app_slot_idx_from_shuffle_config(char monitor_list_idx, int shuffle_pid);
extern char is_monitor_app(const char *name);
extern void init_monitor_app_list(void);

extern int get_cc_id(struct task_struct *ts);
extern void create_new_sid(struct task_struct *ts, int old_pgid);
extern int get_process_sum(struct task_struct *ts);
extern int increase_process_sum(struct task_struct *ts);
extern int decrease_process_sum(struct task_struct *ts);

extern char init_app_slot(struct task_struct *ts);
extern void free_app_slot(struct task_struct *ts);
extern void set_io_in(struct task_struct *ts, int sys_no, char value);
extern char has_io_in(struct task_struct *ts, int sys_no);
extern char *get_start_encode_and_set_entry(struct task_struct *ts, long program_entry);
extern char *is_checkpoint(struct task_struct *ts, char *app_slot_idx);
extern void set_shuffle_pid(char app_slot_idx, int shuffle_pid);
extern char get_app_slot_idx(int pgid);
extern void set_shuffle_pc(char app_slot_idx, ulong pc);
extern void set_additional_pc(char app_slot_idx, long ips[]);
extern ulong get_shuffle_pc(char app_slot_idx);
extern volatile char *get_start_flag(char app_slot_idx);
extern int get_shuffle_pid(char app_slot_idx);
extern long set_app_start(struct task_struct *ts);
extern char is_app_start(struct task_struct *ts);
extern char insert_x_info(struct task_struct *ts, long cc_start, long cc_end, const char *file);
extern char insert_stack_info(struct task_struct *ts, long ss_start, long ss_end, const char *file);
extern char *get_stack_shm_info(struct task_struct *ts, long *stack_len);
extern int get_stack_number(struct task_struct *ts);
extern void modify_stack_belong(struct task_struct *ts, int old_pid, int new_pid);
extern void free_stack_info(int pgid, int pid);
extern void free_dead_stack(struct task_struct *ts);
extern char get_ss_info(struct task_struct *ts, long *stack_start, long *stack_end);

extern void lock_app_slot(void);
extern void unlock_app_slot(void);

extern void rerandomization(struct task_struct *ts);
extern void stop_all(void);
extern void wake_all(void);
#endif
