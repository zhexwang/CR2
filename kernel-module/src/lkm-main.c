#include "lkm-utility.h"
#include "lkm-monitor.h"
#include "lkm-hook.h"
#include "lkm-netlink.h"

static int __init cr2_module_init(void)
{
    PRINTK("CR2 Kernel Module launched!\n");
	init_netlink();
	init_monitor_app_list();
	init_shuffle_config_list();
	init_orig_syscall();
	hook_systable();
	set_ss_type(LKM_OFFSET_SS_TYPE);
    return 0;
}

static void __exit cr2_module_exit(void)
{
	exit_netlink();
	stop_hook();
    PRINTK("CR2 Kernel Module Uninstall\n");
}

module_init(cr2_module_init);
module_exit(cr2_module_exit);

MODULE_LICENSE("GPL");

