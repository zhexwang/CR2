#pragma once

#include <sys/socket.h>
#include <linux/netlink.h>

#include "type.h"

enum LKM_SS_TYPE{
	LKM_OFFSET_SS_TYPE = 0,
	LKM_SEG_SS_TYPE,
	LKM_SEG_SS_PP_TYPE,
	LKM_SS_TYPE_NUM,
};
/*
 * When lkm_ss_type==LKM_OFFSET_SS_TYPE, ss_offset represents the offset between shadow stack and original stack!
 *                  gs base = 0
 * When lkm_ss_type==LKM_SEG_SS_TYPE, ss_offset represents the offset between shadow stack and original stack!
 *                  gs base !=0 
 * When lkm_ss_type==LKM_SEG_SS_PP_TYPE, ss_offset represents the offset between shadow stack and original stack!
 *                  gs base !=0
 */
// Front section is return addresses and the lower section is index of return addresses
typedef struct{
	int connect;
	int proctected_procid;
	long new_ip;
	long cc_offset;
	long ss_offset;
	long gs_base;
	LKM_SS_TYPE lkm_ss_type;
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

class NetLink
{
protected:
	static struct sockaddr_nl src_addr;
	static struct sockaddr_nl dest_addr;
	static struct nlmsghdr *nlh;
	static struct iovec iov;
	static int sock_fd;
	static struct msghdr msg;
public:
	static void connect_with_lkm();
	static void send_mesg(MESG_BAG mesg);
	static void send_cv_ready_mesg(BOOL is_cv1, long new_pc);
	static MESG_BAG recv_mesg();
	static void recv_init_mesg(PID &protected_id, S_ADDRX &curr_pc, SIZE &cc_offset, SIZE &ss_offset, \
		 P_ADDRX &gs_base, LKM_SS_TYPE &ss_type);
	//if protected process is out, the return value is false
	static BOOL recv_cv_request_mesg(P_ADDRX &curr_pc, BOOL &need_cv1);
	static void disconnect_with_lkm();
};

