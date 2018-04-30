#include "poe.h"

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>

#define MAX_PORT_COUNT 16

struct poe_port_object
{
    struct kobject kobj;
    int port;
    int mode;
    int state;
};
#define to_poe_port_object(x) container_of(x, struct poe_port_object, kobj)

struct poe_port_attribute
{
    struct attribute attr;
    ssize_t (*show)(struct poe_port_object *pobj, struct poe_port_attribute *pattr, char *buf);
    ssize_t (*store)(struct poe_port_object *pobj, struct poe_port_attribute *pattr, const char *buf, size_t count);
};
#define to_poe_port_attribute(x) container_of(x, struct poe_port_attribute, attr)

static ssize_t poe_port_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct poe_port_attribute *pattr;
    struct poe_port_object *pobj;

    pattr = to_poe_port_attribute(attr);
    pobj = to_poe_port_object(kobj);
    if (!pattr->show) return -EIO;
    return pattr->show(pobj, pattr, buf);
}

static ssize_t poe_port_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    struct poe_port_attribute *pattr;
    struct poe_port_object *pobj;

    pattr = to_poe_port_attribute(attr);
    pobj = to_poe_port_object(kobj);
    if (!pattr->store) return -EIO;
    return pattr->store(pobj, pattr, buf, count);
}

static struct sysfs_ops poe_port_sysfs_ops = 
{
    .show = poe_port_show,
    .store = poe_port_store,
};

static void poe_port_release(struct kobject *kobj)
{
    struct poe_port_object *pobj;

    pobj = to_poe_port_object(kobj);
    kfree(pobj);
}

static ssize_t mode_show(struct poe_port_object *pobj, struct poe_port_attribute *pattr, char *buf)
{
    int mode;

    mode = getPortMode(pobj->port);
    if (mode < 0)
    {
        printk(KERN_WARNING "poe-driver: failed to get port mode");
        return mode;
    }
    sprintf(buf, "%d", mode);
    return 1;
}

static ssize_t mode_store(struct poe_port_object *pobj, struct poe_port_attribute *pattr, const char *buf, size_t count)
{
    int mode, error;

    sscanf(buf, "%du", &mode);
    error = setPortMode(pobj->port, mode);
    if (error < 0) return error;
    return count;
}

static struct poe_port_attribute mode_attr = __ATTR_RW(mode);

static ssize_t state_show(struct poe_port_object *pobj, struct poe_port_attribute *pattr, char *buf)
{
    int state;

    state = getPortState(pobj->port);
    if (state < 0) 
    {
        printk(KERN_WARNING "poe-driver: failed to get port state");
        return state;
    }
    sprintf(buf, "%d", state);
    return 1;
}

static ssize_t state_store(struct poe_port_object *pobj, struct poe_port_attribute *pattr, const char *buf, size_t count)
{
    int state, error;

    sscanf(buf, "%du", &state);
    error = setPortState(pobj->port, state);
    if (error < 0) return error;
    return count;
}

static struct poe_port_attribute state_attr = __ATTR_RW(state);

static struct attribute *poe_port_default_attrs[] =
{
    &mode_attr.attr,
    &state_attr.attr,
    NULL,
};

static struct kobj_type poe_ktype =
{
    .sysfs_ops = &poe_port_sysfs_ops,
    .release = poe_port_release,
    .default_attrs = poe_port_default_attrs,
};

static struct kset *poe_kset;

static struct poe_port_object *create_poe_port_object(int port_number)
{
    struct poe_port_object *pobj;
    int error;

    pobj = kzalloc(sizeof(*pobj), GFP_KERNEL);
    if (!pobj) return NULL;
    pobj->kobj.kset = poe_kset;
    error = kobject_init_and_add(&pobj->kobj, &poe_ktype, NULL, "port%d", port_number);
    if (error)
    {
        kobject_put(&pobj->kobj);
        return NULL;
    }

    kobject_uevent(&pobj->kobj, KOBJ_ADD);
    pobj->port = port_number;
    return pobj;
}

static void destroy_poe_port_object(struct poe_port_object *pobj)
{
    kobject_put(&pobj->kobj);
}

static struct poe_port_object *port_objects[MAX_PORT_COUNT] = {NULL};

static int __init poe_init(void)
{
    int i, portCount = 0;
    int devId = getDeviceId();

    if (devId == 0x44) portCount = 4;
    else if (devId < 0)
    {
        printk(KERN_WARNING "poe-driver: Error getting device ID\n");
        return devId;
    }
    else
    {
        printk(KERN_WARNING "poe-driver: Invalid device ID\n");
        return -ENXIO;
    }

    poe_kset = kset_create_and_add("poe", NULL, kernel_kobj);
    if (!poe_kset) return -ENOMEM;

    for (i = 0; i < portCount; ++i)
    {
        port_objects[i] = create_poe_port_object(i);
    }

    return 0;
}

static void __exit poe_exit(void)
{
    int i;
    for (i = 0; i < MAX_PORT_COUNT; ++i)
    {
        if (port_objects[i] != NULL)
            destroy_poe_port_object(port_objects[i]);
    }

    kset_unregister(poe_kset);
}

module_init(poe_init);
module_exit(poe_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Timothy Lassiter");