#ifndef __LKM_NETLINK_H__
#define __LKM_NETLINK_H__

typedef struct{
	int connect;
	int proctected_procid;
	long new_ip;
	long cc_offset;
	long ss_offset;
	char mesg[256];
}MESG_BAG;

#define DISCONNECT           0 //send by shuffle process
#define CONNECT              1 //send by shuffle process
#define CV1_IS_READY         2 //send by shuffle process
#define CV2_IS_READY         3 //send by shuffle process
#define CURR_IS_CV2_NEED_CV1 4 //send by kernel module
#define CURR_IS_CV1_NEED_CV2 5 //send by kernel module
#define P_PROCESS_IS_IN      6 //send by kernel module
#define P_PROCESS_IS_OUT     7 //send by kernel module

extern void nl_send_msg(int target_pid, MESG_BAG mesg_bag);
extern void init_netlink(void);
extern void exit_netlink(void);

#endif
