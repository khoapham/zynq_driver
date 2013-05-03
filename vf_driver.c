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

#include "vfdev.h"

#define BRAM0_START 0x80400000
#define BRAM0_END   0x8040FFFF
#define BRAM1_START 0x80410000
#define BRAM1_END   0x8041FFFF

static void *vf_buf;
static int *bram0_base, *bram1_base, *ctrl, *cnt, *addr, *conf, *src, *stat, *dst;
static int vf_bufsize = 8192;

static int vf_count = 1;
static dev_t vf_dev = MKDEV(202, 128);

static struct cdev vf_cdev;

static int vf_open (struct inode *inode, struct file *file);
static int vf_close (struct inode *inode, struct file *file);
static ssize_t vf_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t vf_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static int vf_mmap(struct file *file, struct vm_area_struct *vma);
static long vf_ioctl(struct file *file, unsigned int ioctl_cmd, unsigned long ioctl_param);

void vf_conf_fir(void);
void vf_start(void);
void vf_get_result(void);

static struct file_operations vf_fops = {
	.open	= vf_open,
	.release= vf_close,
	.read	= vf_read,
	.write	= vf_write,
	.mmap	= vf_mmap,
	.unlocked_ioctl	= vf_ioctl,
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

static int vf_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "vf opened\n\r");
	return 0;
}

static int vf_close(struct inode *inode, struct file *file) {
	printk(KERN_INFO "vf closed\n\r");
	return 0;
}

static int vf_mmap(struct file *file, struct vm_area_struct *vma) {
	int size;
	size = vma->vm_end - vma->vm_start;

	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) return -EAGAIN;

	return 0;
}

static long vf_ioctl(struct file *file, unsigned int ioctl_cmd, unsigned long ioctl_param) {

	switch (ioctl_cmd) {
	case VF_CONF_FIR:  // call for the configuration function
		vf_conf_fir();
		break;
	case VF_START:     // call for start VF processing
		vf_start();
		break;
	case VF_GET_RESULT:// read result from VF
		vf_get_result();
		break;
	default: 
		return -ENOTTY;
	}

	return 0;
}

void vf_conf_fir(void) {
	int i;
//	int bram0_base, bram1_base, ctrl, cnt, addr, conf, src, stat, dst;
	bram0_base = ioremap(BRAM0_START, BRAM0_END - BRAM0_START);  //map the bram0 base
	bram1_base = ioremap(BRAM1_START, BRAM1_END - BRAM1_START);  //map the bram1 base
	printk(KERN_INFO"bram0: %x\n\r", (unsigned int)bram0_base);
	printk(KERN_INFO"bram1: %x\n\r", (unsigned int)bram1_base);

	ctrl = bram0_base;
	cnt  = bram0_base + 0x1;
	addr = bram0_base + 0x2;
	conf = bram0_base + 0x3;
	src  = bram0_base + 0x40;

	stat = bram1_base;
	dst  = bram1_base + 0x40;

	for(i=0;i<40;i++) src[i]=i+0x01;

	iowrite32(0x07b5a000, conf[0]);
	iowrite32(0x00000005, conf[1]);
	iowrite32(0x00000000, conf[2]);
	iowrite32(0x07b5a000, conf[3]);
	iowrite32(0x00000007, conf[4]);
	iowrite32(0x00000000, conf[5]);
	iowrite32(0x07b5e000, conf[6]);
	iowrite32(0x00000005, conf[7]);
	iowrite32(0x00000000, conf[8]);
	iowrite32(0x07b52000, conf[9]);
	iowrite32(0x00000007, conf[10]);
	iowrite32(0x00000000, conf[11]);

	iowrite32(0xffffffff, conf[12]);
	iowrite32(0x00678000, conf[13]);
	iowrite32(0xffffffff, conf[14]);
	iowrite32(0x00678000, conf[15]);
	iowrite32(0x00000345, conf[16]);
	iowrite32(0xffffffff, conf[17]);
	iowrite32(0x00678000, conf[18]);
	iowrite32(0xffffffff, conf[19]);
	iowrite32(0x00678000, conf[20]);
	iowrite32(0xffffffff, conf[21]);
	wmb();
}

void vf_start(void) {
	iowrite32(0x01000100, addr);
	iowrite32(0x0000802d, ctrl);
	wmb();
}

void vf_get_result(void) {
	int status;

	status = ioread32(stat);

	while(status != 0x0) printk(KERN_INFO "vf didn't start\n\r");
	while(status != 0x1) printk(KERN_INFO "vf didn't finish\n\r");

	printk(KERN_INFO"vf finished, ready to read the result\n\r");
	ioread32_rep(dst, vf_buf, 40);
	iowrite32(0x00000000, stat);	
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
	iounmap(bram0_base);
	iounmap(bram1_base);
	return;
}

module_init(vf_init);
module_exit(vf_cleanup);
