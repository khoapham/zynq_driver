#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/uaccess.h>

//#define VF_PHYS 0x12345678

static void *vf_buf;
static int vf_bufsize = 8192;

static int vf_count = 1;
static dev_t vf_dev = MKDEV(202, 128);

static struct cdev vf_cdev;

static int vf_open (struct inode *inode, struct file *file);
static int vf_close (struct inode *inode, struct file *file);
static ssize_t vf_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t vf_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);

static struct file_operations vf_fops = {
	.open	= vf_open,
	.release= vf_close,
	.read	= vf_read,
	.write	= vf_write,
};

static ssize_t vf_read(struct file *file, char __user *buf, size_t count, loff_t *ppos){
	int remaining_size, transfer_size;

	remaining_size = vf_bufsize - (int)(*ppos); //bytes left to transfer

	if (remaining_size == 0) { //all read, returning 0 (end of file)
		return 0;
	}

	transfer_size = min_t(int, remaining_size, count);

	if (copy_to_user(buf /*to*/, vf_buf + *ppos /*from*/, transfer_size)) {
		return -EFAULT;
	} else { //increase the position in the open file
		*ppos += transfer_size;
		return transfer_size;
	}
}

static ssize_t vf_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
	int remaining_bytes;

	remaining_bytes = vf_bufsize - (*ppos); //bytes not written yet in the device

	if (count>remaining_bytes) { //can't write beyond the end of the device
		return -EIO;
	}

	if (copy_from_user(vf_buf + *ppos /*to*/, buf /*from*/, count)) {
		return -EFAULT;
	} else { //increase the position in the open file
		*ppos += count;
		return count;
	}
}

static int vf_open (struct inode *inode, struct file *file) {
	printk(KERN_INFO "vf opened\n\r");
	return 0;
}

static int vf_close (struct inode *inode, struct file *file) {
	printk(KERN_INFO "vf closed\n\r");
	return 0;
}

static int __init vf_init (void) {
	int err;

//	vf_buf = ioremap(VF_PHYS, vf_bufsize);
	vf_buf = kmalloc(vf_bufsize, GFP_KERNEL);
	if (!vf_buf) {
		err = -ENOMEM;
		goto err_exit;
	}

	printk(KERN_INFO "Registering the vf\n\r");
	if (register_chrdev_region(vf_dev, vf_count, "vf-driver")) {
		printk(KERN_INFO "register the vf-driver error\n\r");
		err = -ENODEV;
		goto err_free_buf;
	}

	cdev_init(&vf_cdev, &vf_fops);

	if (cdev_add(&vf_cdev, vf_dev, vf_count)) {
		printk(KERN_INFO "add the vf-driver character device error\n\r");
		err = -ENODEV;
		goto err_dev_unregister;
	}

	printk(KERN_INFO "vf-driver was registered\n\r");
	return 0;

	err_dev_unregister:
		unregister_chrdev_region(vf_dev, vf_count);
	err_free_buf:
		printk(KERN_INFO "FAIL to register vf-driver\n\r");
		iounmap(vf_buf);
	err_exit:
		return err;
}

static void __exit vf_cleanup (void) {
	cdev_del(&vf_cdev);
	unregister_chrdev_region(vf_dev, vf_count);
	iounmap(vf_buf);
	return;
}

module_init(vf_init);
module_exit(vf_cleanup);
