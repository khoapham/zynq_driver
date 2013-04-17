#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/delay.h>
//#include <linux/config.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
//#include <asm/hardware.h>
#include <asm/uaccess.h>

//#define VF_MAJOR 200

static int vf_count = 1;
static dev_t vf_dev = MKDEV(202, 128);

static struct cdev vf_cdev;

static int vf_open (struct inode *inode, struct file *file);
static int vf_close (struct inode *inode, struct file *file);

static struct file_operations vf_fops = {
	.open	= vf_open,
	.release	= vf_close,
};

static int vf_open (struct inode *inode, struct file *file) {
	printk(KERN_INFO "vf opened\n\r");
	return 0;
}

static int vf_close (struct inode *inode, struct file *file) {
	printk(KERN_INFO "vf closed\n\r");
	return 0;
}

static int __init vf_init (void) {
	printk(KERN_INFO "Registering the vf\n\r");
//	if(register_chrdev(VF_MAJOR, "vfdriver", &vf_fops)){
	if (register_chrdev_region(vf_dev, vf_count, "vf-driver")) {
		printk(KERN_INFO "register the vf-driver error\n\r");
		goto fail_register_chrdev;
	}

	cdev_init(&vf_cdev, &vf_fops);

	if (cdev_add(&vf_cdev, vf_dev, vf_count)) {
		printk(KERN_INFO "add the vf-driver character device error\n\r");
	}

	printk(KERN_INFO "vf-driver was registered\n\r");
	return 0;

	fail_register_chrdev:
		printk(KERN_INFO "FAIL to register vf-driver\n\r");
		return 0;
}

static void __exit vf_cleanup (void) {
//	unregister_chrdev(VF_MAJOR, "vfdriver");
	cdev_del(&vf_cdev);
	unregister_chrdev_region(vf_dev, vf_count);
	return;
}

module_init(vf_init);
module_exit(vf_cleanup);
