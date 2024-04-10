#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

static int __init lkm1_init(void) {
    printk(KERN_INFO "lkm1: %s", __FUNCTION__);
    return 0;
}

static void __exit lkm1_exit(void) {
    printk(KERN_INFO "lkm1: exit\n");
}

module_init(lkm1_init);
module_exit(lkm1_exit);

MODULE_AUTHOR("kp7742");
MODULE_DESCRIPTION("lkm");
MODULE_LICENSE("GPL");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif