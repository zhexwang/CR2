
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>

#include "string.h"
#include "netlink.h"
#include "utility.h"

#define NETLINK_USER 31


struct sockaddr_nl NetLink::src_addr = {0};
struct sockaddr_nl NetLink::dest_addr = {0};
struct nlmsghdr *NetLink::nlh = NULL;
struct iovec NetLink::iov = {0};
int NetLink::sock_fd = -1;
struct msghdr NetLink::msg = {0};

extern std::string get_real_name_from_path(std::string path);

void NetLink::connect_with_lkm(std::string elf_path)
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
    // 6.connect with lkm
    std::string name = get_real_name_from_path(elf_path);
    MESG_BAG mesg = {CONNECT, 0, 0, 0, 0, 0, LKM_OFFSET_SS_TYPE, "\0", "Connect with LKM"};
    ASSERT(name.length()<=256);
    strcpy(mesg.app_name, name.c_str());
    NetLink::send_mesg(mesg);
}

void NetLink::send_mesg(MESG_BAG mesg)
{
    memcpy(NLMSG_DATA(nlh), &mesg, sizeof(MESG_BAG));
    nlh->nlmsg_pid = getpid();
    BLUE("Sending message to kernel: %s\n", mesg.mesg);
    sendmsg(sock_fd, &msg, 0);
}

void NetLink::send_cv_ready_mesg(BOOL is_cv1, long new_pc, std::string elf_path)
{
    std::string name = get_real_name_from_path(elf_path);
    int cvn_ready = is_cv1 ? CV1_IS_READY : CV2_IS_READY;
    MESG_BAG msg_content = {cvn_ready, 0, new_pc, 0, 0, 0, LKM_OFFSET_SS_TYPE, "\0", "Code variant is ready!"};
    strcpy(msg_content.app_name, name.c_str());
    send_mesg(msg_content);
}

void NetLink::send_sigaction_handled_mesg(long new_pc, std::string elf_path)
{
    std::string name = get_real_name_from_path(elf_path);
    MESG_BAG msg_content = {SIGACTION_HANDLED, 0, new_pc, 0, 0, 0, LKM_OFFSET_SS_TYPE, "\0", "Sigaction is handled!"};
    strcpy(msg_content.app_name, name.c_str());
    send_mesg(msg_content);
}

void NetLink::send_ss_handled_mesg(long new_pc, std::string elf_path)
{
    std::string name = get_real_name_from_path(elf_path);
    MESG_BAG msg_content = {SS_HANDLED, 0, new_pc, 0, 0, 0, LKM_OFFSET_SS_TYPE, "\0", "Shadow Stack is handled!"};
    strcpy(msg_content.app_name, name.c_str());
    send_mesg(msg_content);
}

void NetLink::send_dlopen_handled_mesg(long new_pc, std::string elf_path)
{
    std::string name = get_real_name_from_path(elf_path);
    MESG_BAG msg_content = {DLOPEN_HANDLED, 0, new_pc, 0, 0, 0, LKM_OFFSET_SS_TYPE, "\0", "Dlopen is handled!"};
    strcpy(msg_content.app_name, name.c_str());
    send_mesg(msg_content);
}

MESG_BAG NetLink::recv_mesg()
{
	BLUE("Waiting for message from kernel: ");
    recvmsg(sock_fd, &msg, 0);
    BLUE("%s\n", (*(MESG_BAG*)NLMSG_DATA(nlh)).mesg);
    return *(MESG_BAG*)NLMSG_DATA(nlh);
}

void NetLink::disconnect_with_lkm(std::string elf_path)
{
    std::string app_name = get_real_name_from_path(elf_path);
    MESG_BAG out_mesg = {DISCONNECT, 0, 0, 0, 0, 0, LKM_OFFSET_SS_TYPE, "\0", "Shuffle process is exit!"};
    strcpy(out_mesg.app_name, app_name.c_str());
    send_mesg(out_mesg);
	free(nlh);
    close(sock_fd);
}

