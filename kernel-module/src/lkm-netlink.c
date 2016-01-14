#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include "lkm-utility.h"

#define NETLINK_USER 31

struct sock *nl_sk = NULL;

typedef struct{
	char mesg[256];
}MESG_BAG;

void nl_send_msg(int target_pid, MESG_BAG mesg_bag)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb_out;
    int res;

    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

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
        PRINTK("Error while sending bak to user in %s\n", __FUNCTION__);
	else
		PRINTK("Send mesg to %d\n", target_pid);

}

void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;

    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid; /*pid of sending process */
    printk(KERN_INFO "Netlink received msg payload from %d\n", pid);

	//return *(MESG_BAG*)nlmsg_data(nlh);
	return ;
}

void init_netlink(void)
{
	struct netlink_kernel_cfg cfg = { .input  = nl_recv_msg};
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        PRINTK("Error creating socket in %s\n", __FUNCTION__);
        return ;
    }
	
    return;
}

void exit_netlink(void)
{
    printk(KERN_INFO "exiting hello module\n");
    netlink_kernel_release(nl_sk);
}

