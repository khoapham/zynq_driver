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

static int *vf_buf, *buf_pos;
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

void vf_initialize(void);
void vf_conf_fir(unsigned long config_bytes);
void vf_data_input(unsigned long data_bytes);
void vf_start(unsigned long data_bytes);
void vf_get_result(unsigned long data_bytes);
void vf_get_cnt(void);
void vf_get_stat(void);

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

//	printk("remaining size: %d\n\r", remaining_size);
	if (remaining_size == 0) { //all read, returning 0 (end of file)
		return 0;
	}

	transfer_size = min_t(int, remaining_size, count);

//	printk("transfer size: %d\n\r", transfer_size);
	if (copy_to_user(buf /*to*/, vf_buf + *ppos /*from*/, transfer_size)) {
		return -EFAULT;
	} else { //increase the position in the open file
		*ppos += transfer_size;
		*buf_pos = *ppos;
//		printk("buffer last postion %x\n\r", *buf_pos);
		return transfer_size;
	}
}

static ssize_t vf_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
	int remaining_bytes;

	remaining_bytes = vf_bufsize - (*ppos); //bytes not written yet in the device
//	printk("ppos: %d\n\r", (int)(*ppos));
//	printk("remaining bytes: %d\n\r", remaining_bytes);
//	if (count>remaining_bytes) {  //change the *ppos
//		*ppos = 0;
//		remaining_bytes = vf_bufsize - (*ppos);
//	}

	if (count>remaining_bytes) { //can't write beyond the end of the device
		return -EIO;
	}

	if (copy_from_user(vf_buf + *ppos /*to*/, buf /*from*/, count)) {
		return -EFAULT;
	} else { //increase the position in the open file
		*ppos += count;
		*buf_pos = *ppos;
//		printk("buffer last postion %x\n\r", *buf_pos);
		return count;
	}
}

static int vf_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "vf opened\n\r");
	vf_initialize();
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
	case VF_DATA_INPUT:  // call for the data input
		vf_data_input(ioctl_param);
		break;
	case VF_CONF_FIR:  // call for the configuration function
		vf_conf_fir(ioctl_param);
		break;
	case VF_START:     // call for start VF processing
		vf_start(ioctl_param);
		break;
	case VF_GET_RESULT:// read result from VF
		vf_get_result(ioctl_param);
		break;
	case VF_GET_CNT:   // read counter value from VF
		vf_get_cnt();
		break;
	case VF_GET_STAT:  // read the VF status
		vf_get_stat();
		break;
	default: 
		return -ENOTTY;
	}

	return 0;
}

void vf_initialize(void) {
        bram0_base = ioremap(BRAM0_START, BRAM0_END - BRAM0_START);  //map the bram0 base
        bram1_base = ioremap(BRAM1_START, BRAM1_END - BRAM1_START);  //map the bram1 base

//      printk("bram0: %x\n\r", (unsigned int)bram0_base);
//      printk("bram1: %x\n\r", (unsigned int)bram1_base);

        ctrl = bram0_base;
        cnt  = bram0_base + 0x1;
        addr = bram0_base + 0x2;
        conf = bram0_base + 0x3;
        src  = bram0_base + 0x40;

        stat = bram1_base;
        dst  = bram1_base + 0x40;

//      printk("control: %x\n\r", (unsigned int)ctrl);
//      printk("address: %x\n\r", (unsigned int)addr);
}

void vf_conf_fir(unsigned long config_bytes) {
//	int i;
	memcpy_toio(conf, (void *)(vf_buf + *buf_pos - config_bytes*4), config_bytes*4);
//	for(i=0;i<22;i++) printk("vf buf[%d]: %x\n\r", i, (unsigned int)(vf_buf[i]));
//	for(i=0;i<22;i++) printk("conf[%d]: %x\n\r", i, (unsigned int)(conf[i]));
}

void vf_data_input(unsigned long data_bytes) {
//	int i;
	memcpy_toio(src, (void *)(vf_buf + *buf_pos - data_bytes*4), data_bytes*4);
//	for(i=22;i<40;i++) printk("vf buf[%d]: %x\n\r", i, (unsigned int)(vf_buf[i]));
//	for(i=0;i<40;i++) printk("src[%d]: %x\n\r", i, (unsigned int)(src[i]));
}

void vf_start(unsigned long data_bytes) {
	int init_ctrl, value_ctrl;
	init_ctrl = 0x0000002d;
	value_ctrl=init_ctrl | ((int)data_bytes << 10);
//	printk("number of data: %d\n\r", (unsigned int)data_bytes);
//	printk("control's value: %x\n\r", value_ctrl);

//	iowrite32(0x00000000, stat);	
	iowrite32(0x01000100, addr);
//	iowrite32(0x0000802d, ctrl);
	iowrite32(value_ctrl, ctrl);
//	wmb();
}

void vf_get_result(unsigned long data_bytes) {
//	int status;
//	int i;

//	status = ioread32((void *)stat);
//	printk("status: %x\n\r", status);
//	while(status != 0x0) printk(KERN_INFO "vf didn't start\n\r");
//	while(status != 0x1) printk(KERN_INFO "vf didn't finish\n\r");

//	printk(KERN_INFO"vf finished, ready to read the result\n\r");
//	for(i=0;i<40;i++) printk("data out[%d]: %x\n\r", i, (unsigned int)dst[i]);
	memcpy_fromio((void *)(vf_buf + *buf_pos), dst, data_bytes*4);
//	for(i=40;i<40;i++) printk("vf buf[%d]: %x\n\r", i, (unsigned int)(vf_buf[i]));
	iowrite32(0x00000000, stat);	
}

void vf_get_cnt(void) {
	memcpy_fromio((void *)(vf_buf + *buf_pos), cnt, 4);
}

void vf_get_stat(void) {
        memcpy_fromio((void *)(vf_buf + *buf_pos), stat, 4);
}

static int __init vf_init (void) {
	int err;

	vf_buf = kmalloc(vf_bufsize, GFP_KERNEL);
	buf_pos= kmalloc(1, GFP_KERNEL);
	if (!vf_buf) {
		err = -ENOMEM;
		goto err_exit;
	}

	printk(KERN_INFO "vf_buf: %x\n\r", (unsigned int)vf_buf);
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
