#include "poe_driver.h"
#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Timothy Lassiter");

static int mc_poe_driver_init(void)
{
    int result = 0;
    printk( KERN_NOTICE "mc-poe-driver: Initialization started" );

    result = register_device();
    return result;
}

static void mc_poe_driver_exit(void)
{
    printk( KERN_NOTICE "mc-poe-driver: Exiting" );
    unregister_device();
}

module_init(mc_poe_driver_init);
module_exit(mc_poe_driver_exit);

