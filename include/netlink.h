#pragma once

#include <sys/socket.h>
#include <linux/netlink.h>

#include "type.h"

typedef struct{
	int connect;
	int proctected_procid;
	long new_ip;
	long cc_offset;
	long ss_offset;
	char mesg[256];
}MESG_BAG;

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
	static MESG_BAG recv_mesg();
	static void recv_mesg(PID &protected_id, S_ADDRX &curr_pc, SIZE &cc_offset, SIZE &ss_offset);
	static void disconnect_with_lkm();
};

