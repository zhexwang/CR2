#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/mutex.h>

#include "lkm-utility.h"
#include "lkm-netlink.h"
#include "lkm-monitor.h"

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

void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;
	char *mesg = NULL;
	char *app_name = NULL;
	char monitor_idx = 0;
	char app_slot_idx = 0;
	volatile char *start_flag = NULL;
	MESG_BAG wrong_msg = {WRONG_APP, 0, 0, 0, 0, 0, 0, "\0", "Wrong app!"};
    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid; /*pid of sending process */
	mesg = ((MESG_BAG*)nlmsg_data(nlh))->mesg;
	app_name = ((MESG_BAG*)nlmsg_data(nlh))->app_name;
	strcpy(wrong_msg.app_name, app_name);
	PRINTK("Recieve mesg from user(%d): %s\n", pid, mesg);
	monitor_idx = is_monitor_app(app_name);
	if(monitor_idx!=0){
		switch(((MESG_BAG*)nlmsg_data(nlh))->connect){
			case CONNECT:
				insert_shuffle_info(monitor_idx, pid);
				break;
			case DISCONNECT:
				free_one_shuffle_info(monitor_idx, pid);
				break;
			case CV1_IS_READY: case CV2_IS_READY:
				app_slot_idx = get_app_slot_idx_from_shuffle_config(monitor_idx, pid);
				start_flag = get_start_flag(app_slot_idx);
				set_shuffle_pc(app_slot_idx, ((MESG_BAG*)nlmsg_data(nlh))->new_ip);
				*start_flag = 0;
				break;
			default:
				PRINTK("Unkown connect type %d sent by shuffle process\n", ((MESG_BAG*)nlmsg_data(nlh))->connect);
		}
	}else{
		PRINTK("%s is not our monitor app!\n", app_name);
		nl_send_msg(pid, wrong_msg);
	}

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

LKM_SS_TYPE global_ss_type;
void set_ss_type(LKM_SS_TYPE ss_type)
{
	global_ss_type = ss_type;
}

int need_set_gs(void)
{
	return global_ss_type!=LKM_OFFSET_SS_TYPE;
}

