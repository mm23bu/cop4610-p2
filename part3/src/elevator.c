#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elevator module scaffold");

static int __init elevator_init(void) {
    pr_info("Elevator module scaffold loaded\n");
    return 0;
}

static void __exit elevator_exit(void) {
    pr_info("Elevator module scaffold unloaded\n");
}

module_init(elevator_init);
module_exit(elevator_exit);
