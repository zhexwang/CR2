#pragma once

#include <sys/socket.h>
#include <linux/netlink.h>

typedef struct{
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
	static void init();
	static void send_mesg(MESG_BAG mesg);
	static MESG_BAG recv_mesg();
	static void exit();
};

