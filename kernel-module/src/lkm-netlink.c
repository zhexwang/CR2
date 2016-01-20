#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include "lkm-utility.h"
#include "lkm-netlink.h"

#define NETLINK_USER 31

struct sock *nl_sk = NULL;

void nl_send_msg(int target_pid, MESG_BAG mesg_bag)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb_out;
    int res;

    skb_out = nlmsg_new(sizeof(MESG_BAG), 0);
    if (!skb_out) {
        PRINTK("Failed to allocate new skb in %s\n", __FUNCTION__);
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, sizeof(MESG_BAG), 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	memcpy(nlmsg_data(nlh), &mesg_bag, sizeof(MESG_BAG));

    res = nlmsg_unicast(nl_sk, skb_out, target_pid);
    if (res < 0)
        PRINTK("Error while sending to user in %s\n", __FUNCTION__);
	else
		PRINTK("Send mesg to user(%d): %s\n", target_pid, mesg_bag.mesg);
}

extern char start_flag;
long connect_with_shuffle_process = 0;
int shuffle_process_pid = 0;
long new_ip = 0;

void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;
    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid; /*pid of sending process */
	shuffle_process_pid = pid;
	PRINTK("Recieve mesg from user(%d): %s\n", pid, ((MESG_BAG*)nlmsg_data(nlh))->mesg);
	start_flag = 0;
	new_ip = ((MESG_BAG*)nlmsg_data(nlh))->new_ip;
	connect_with_shuffle_process = ((MESG_BAG*)nlmsg_data(nlh))->connect;

	return ;
}

void init_netlink(void)
{
#ifdef _C10
	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, 0, nl_recv_msg, NULL, THIS_MODULE);
#else
	struct netlink_kernel_cfg cfg = { .input = nl_recv_msg};
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
#endif
    if (!nl_sk) {
        PRINTK("Error creating socket in %s\n", __FUNCTION__);
        return ;
    }
	
    return;
}

void exit_netlink(void)
{
    netlink_kernel_release(nl_sk);
}

