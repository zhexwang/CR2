
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>

#include "netlink.h"
#include "utility.h"

#define NETLINK_USER 31


struct sockaddr_nl NetLink::src_addr = {0};
struct sockaddr_nl NetLink::dest_addr = {0};
struct nlmsghdr *NetLink::nlh = NULL;
struct iovec NetLink::iov = {0};
int NetLink::sock_fd = -1;
struct msghdr NetLink::msg = {0};

void NetLink::init()
{
	// 1.open socket
    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
	ASSERT(sock_fd>=0);
	// 2.init src_addr and dest_addr
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */
	// 3.bind sock_fd
	bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
	// 4.init hlh
    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(sizeof(MESG_BAG)));
    memset(nlh, 0, NLMSG_SPACE(sizeof(MESG_BAG)));
    nlh->nlmsg_len = NLMSG_SPACE(sizeof(MESG_BAG));
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
	// 5.init msg
	iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
}

void NetLink::send_mesg(MESG_BAG mesg)
{
    memcpy(NLMSG_DATA(nlh), &mesg, sizeof(MESG_BAG));
    BLUE("Sending message to kernel\n");
    sendmsg(sock_fd, &msg, 0);
}

MESG_BAG NetLink::recv_mesg()
{
	BLUE("Waiting for message from kernel\n");
    recvmsg(sock_fd, &msg, 0);
    return *(MESG_BAG*)NLMSG_DATA(nlh);
}

void NetLink::exit()
{
	free(nlh);
    close(sock_fd);
}

