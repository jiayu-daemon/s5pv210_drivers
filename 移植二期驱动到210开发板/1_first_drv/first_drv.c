#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

static struct class *firstdrv_class;

volatile unsigned long *gpj2con = NULL;
volatile unsigned long *gpj2dat = NULL;

static int first_drv_open(struct inode *inode, struct file *file)
{
	/* 配置GPJ2_0,GPJ2_1,GPJ2_2,GPJ2_3为输出 */
	*gpj2con &= ~((0xf<<(0*4)) | (0xf<<(1*4)) | (0xf<<(2*4)) | (0xf<<(3*4)));
	*gpj2con |= ((0x1<<(0*4)) | (0x1<<(1*4)) | (0x1<<(2*4)) | (0x1<<(3*4)));
	return 0;
}

static ssize_t first_drv_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	int val;

	copy_from_user(&val, buf, count); //	copy_to_user();

	if (val == 1)
	{
		// 点灯
		*gpj2dat &= ~((1<<0) | (1<<1) | (1<<2) | (1<<3));
	}
	else
	{
		// 灭灯
		*gpj2dat |= (1<<0) | (1<<1) | (1<<2) | (1<<3);
	}
	
	return 0;
}

static struct file_operations first_drv_fops = {
    .owner  =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open   =   first_drv_open,     
	.write	=	first_drv_write,	   
};


int major;
static int first_drv_init(void)
{
	major = register_chrdev(0, "first_drv", &first_drv_fops); // 注册, 告诉内核

	firstdrv_class = class_create(THIS_MODULE, "firstdrv");

	device_create(firstdrv_class, NULL, MKDEV(major, 0), NULL, "xyz"); /* /dev/xyz */

	gpj2con = (volatile unsigned long *)ioremap(0xE0200280, 16);
	gpj2dat = gpj2con + 1;

	return 0;
}

static void first_drv_exit(void)
{
	unregister_chrdev(major, "first_drv"); // 卸载

	device_destroy(firstdrv_class, MKDEV(major, 0));
	class_destroy(firstdrv_class);
	iounmap(gpj2con);
}

module_init(first_drv_init);
module_exit(first_drv_exit);


MODULE_LICENSE("GPL");

