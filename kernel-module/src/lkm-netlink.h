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

extern void nl_send_msg(int target_pid, MESG_BAG mesg_bag);
extern void init_netlink(void);
extern void exit_netlink(void);

#endif
