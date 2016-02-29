#pragma once

#include <sys/socket.h>
#include <linux/netlink.h>
#include <string>

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
	char app_name[256];
	char mesg[256];
}MESG_BAG;

#define DISCONNECT            0 //send by shuffle process
#define CONNECT               1 //send by shuffle process
#define CV1_IS_READY          2 //send by shuffle process
#define CV2_IS_READY          3 //send by shuffle process
#define SIGACTION_HANDLED     4 //send by shuffle process
#define CURR_IS_CV2_NEED_CV1  5 //send by kernel module
#define CURR_IS_CV1_NEED_CV2  6 //send by kernel module
#define P_PROCESS_IS_IN       7 //send by kernel module
#define P_PROCESS_IS_OUT      8 //send by kernel module
#define SIGACTION_DETECTED    9 //send by kernel module
#define WRONG_APP             10 //send by kernel module

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
	static void connect_with_lkm(std::string elf_path);
	static void send_mesg(MESG_BAG mesg);
	static void send_cv_ready_mesg(BOOL is_cv1, long new_pc, std::string elf_path);
	static MESG_BAG recv_mesg();
	static void send_sigaction_handled_mesg(long new_pc, std::string elf_path);
	//if protected process is out, the return value is false
	static void disconnect_with_lkm(std::string elf_path);
};

