#include "rgbled.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>

#define NUM_DEVICES 1
#define FIRSTMINOR 0

MODULE_AUTHOR("flopezb03");
MODULE_LICENSE("Dual BSD/GPL");

struct rgbled_dev rgbled_device;
dev_t rgbled_devnum;


int rgbled_open(struct inode *inode, struct file *filp){
    return 0;
}
int rgbled_release(struct inode *inode, struct file *filp){
    return 0;
}
ssize_t rgbled_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
    return 0;
}
ssize_t rgbled_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
    return 0;
}



struct file_operations rgbled_fops = {
    .owner = THIS_MODULE,
    .open = rgbled_open,
    .release = rgbled_release,
    .read = rgbled_read,
    .write = rgbled_write,
};



static int __init rgbled_init(void){
    int res;

    res = alloc_chrdev_region(&rgbled_devnum, FIRSTMINOR, NUM_DEVICES, "rgbled");
    if(res < 0)
        return res;

    memset(&rgbled_device, 0, sizeof(struct rgbled_dev));


    cdev_init(&rgbled_device.cdev, &rgbled_fops);
    rgbled_device.cdev.owner = THIS_MODULE;
    rgbled_device.cdev.ops = &rgbled_fops;
    res = cdev_add(&rgbled_device.cdev, rgbled_devnum, NUM_DEVICES);
    if(res < 0){
        unregister_chrdev_region(rgbled_devnum, NUM_DEVICES);
        return res;
    }
    
    return 0;
}

static void __exit rgbled_cleanup(void){
    cdev_del(&rgbled_device.cdev);

    
    unregister_chrdev_region(rgbled_devnum, NUM_DEVICES);
}

module_init(rgbled_init);
module_exit(rgbled_cleanup);
