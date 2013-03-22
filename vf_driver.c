#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/config.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/ioport.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/uaccess.h>

#define VF_MAJOR 200

static struct file_operations vf_fops = {
	open	= vf_open,
	read	= vf_read,
	write	= vf_write,
	release	= vf_close,
};

static void vf_hardware_init (void) {}
static void control () {}
static int vf_open (struct inode *inode, struct file *file) {
	printk(KERN_INFO "vf opened\n\r");
	return 0;
}

static int vf_close (struct inode *inode, struct file *file) {
	printk(KERN_INFO "vf closed\n\r");
	return 0;
}
static int vf_read (struct file *file, char *buf, size_t count, loff_t *ppos) {
	struct vf_dev *dev = file->private_data;
	struct vf_qset *dptr;  // the first listitem
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize= quantum * qset;  // how many bytes in the listitem
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (*ppos >= dev->size)
		goto out;
	if (*ppos + count > dev->size)
		count = dev->size - *ppos;

//find listitem, qset index, and offset in the quantum
	item = (long)*ppos/itemsize;
	rest = (long)*ppos%itemsize;
	s_pos = rest/quantum; q_pos = rest%quantum;

//follow the list up to the right position (defined elsewhere)
	dptr = vf_follow(dev, item);

	if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
		goto out;  // don't fill holes

// read only up to the end of this quantum
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos]+q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}

	*ppos += count;
	retval = count;

	out:
		up(&dev->sem);
		return retval;
}

static int vf_write (struct file *file, const char *buf, size_t count, loff_t *ppos) {
	struct vf_dev *dev = file->private_data;
	struct vf_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize= quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM;  //value used in "goto out" statements

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

// find listitem, qset index & offset in the quantum
	item = (long)*ppos/itemsize;
	rest = (long)*ppos%itemsize;

	s_pos= rest/quantum; q_pos= rest%quantum;

//follow the list up to the right position
	dptr = vf_follow(dev, item);
	if (dptr == NULL)
		goto out;
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if(!dprt->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	if(!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if(!dptr->data[s_pos])
			goto out;
	}

//write only up to the end of this quantum
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if(copy_from_user(dptr->data[s_pos]+q_pos, buf, count)) {
		retval = -EFAULT;
		goto out;
	}

	*ppos += count;
	retval = count;

//update the size
	if (def->size < *ppos)
		def_size = *ppos;

	out:
		up(&dev->sem);
		return retval;
}

static int __init vf_init (void) {
	vf_hardware_init();
	printk(KERN_INFO "Registering the vf\n\r");
	if(register_chrdev(VF_MAJOR, "vfdriver", &vf_fops)){
		printk(KERN_INFO "register the vf-driver error\n\r");
		goto fail_register_chrdev;
	}
	printk(KERN_INFO "vf-driver was registered\n\r");
	return 0;

	fail_register_chrdev:
		printk(KERN_INFO "FAIL to register vf-driver\n\r");
		return 0;
}

static void __exit vf_cleanup (void) {
	unregister_chrdev(VF_MAJOR, "vfdriver");
	return;
}

module_init(vf_init);
module_exit(vf_cleanup);
