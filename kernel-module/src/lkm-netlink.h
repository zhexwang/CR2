#ifndef __LKM_NETLINK_H__
#define __LKM_NETLINK_H__


/*
 * When lkm_ss_type==LKM_OFFSET_SS_TYPE, ss_offset represents the offset between shadow stack and original stack!
 *                  gs base = 0
 * When lkm_ss_type==LKM_SEG_SS_TYPE, ss_offset represents the offset between shadow stack and original stack!
 *                  gs base !=0 
 * When lkm_ss_type==LKM_SEG_SS_PP_TYPE, ss_offset represents the offset between shadow stack and original stack!
 *                  gs base !=0
 */
// Front section is return addresses and the lower section is index of return addresses

typedef enum LKM_SS_TYPE{
	LKM_OFFSET_SS_TYPE = 0,
	LKM_SEG_SS_TYPE,
	LKM_SEG_SS_PP_TYPE,
	LKM_SS_TYPE_NUM,
}LKM_SS_TYPE;

typedef struct{
	int connect;
	int proctected_procid;
	long new_ip;
	long cc_offset;
	long ss_offset;
	long gs_base;
	LKM_SS_TYPE lkm_ss_type;
	char app_name[256];
	char mesg[256];
}MESG_BAG;

#define DISCONNECT            0 //send by shuffle process
#define CONNECT               1 //send by shuffle process
#define CV1_IS_READY          2 //send by shuffle process
#define CV2_IS_READY          3 //send by shuffle process
#define SIGACTION_HANDLED     4 //send by shuffle process
#define SS_HANDLED            5 //send by shuffle process
#define CURR_IS_CV2_NEED_CV1  6 //send by kernel module
#define CURR_IS_CV1_NEED_CV2  7 //send by kernel module
#define P_PROCESS_IS_IN       8 //send by kernel module
#define P_PROCESS_IS_OUT      9 //send by kernel module
#define SIGACTION_DETECTED    10//send by kernel module
#define CREATE_SS             11//send by kernel module
#define FREE_SS               12//send by kernel module
#define WRONG_APP             13//send by kernel module

extern void nl_send_msg(int target_pid, MESG_BAG mesg_bag);
extern void init_netlink(void);
extern void exit_netlink(void);
extern LKM_SS_TYPE global_ss_type;
extern void set_ss_type(LKM_SS_TYPE ss_type);
extern int need_set_gs(void);
#endif
