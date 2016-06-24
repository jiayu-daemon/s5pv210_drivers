#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

static struct class *seconddrv_class;

volatile unsigned long *gph2con;
volatile unsigned long *gph2dat;

volatile unsigned long *gph3con;
volatile unsigned long *gph3dat;

static int second_drv_open(struct inode *inode, struct file *file)
{
	/* 配置GPH2_3为输入引脚 */
	*gph2con &= ~(0xf<<(3*4));

	/* 配置GPH3_0,GPH3_1,GPH3_2,GPH3_3为输入引脚 */
	*gph3con &= ~((0xf<<(0*4)) | (0xf<<(1*4)) | (0xf<<(2*4)) | (0xf<<(3*4)));

	return 0;
}

ssize_t second_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	/* 返回5个引脚的电平 */
	unsigned char key_vals[5];
	int regval;

	if (size != sizeof(key_vals))
		return -EINVAL;

	/* 读GPH2_3 */
	regval = *gph2dat;
	key_vals[0] = (regval & (1<<3)) ? 1 : 0;	

	/* 读GPH3_0,GPH3_1,GPH3_2,GPH3_3 */
	regval = *gph3dat;
	key_vals[1] = (regval & (1<<0)) ? 1 : 0;
	key_vals[2] = (regval & (1<<1)) ? 1 : 0;
	key_vals[3] = (regval & (1<<2)) ? 1 : 0;
	key_vals[4] = (regval & (1<<3)) ? 1 : 0;
	
	copy_to_user(buf, key_vals, sizeof(key_vals));
	
	return sizeof(key_vals);
}


static struct file_operations sencod_drv_fops = {
	.owner  =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
	.open   =   second_drv_open,     
	.read	=	second_drv_read,	   
};


int major;
static int second_drv_init(void)
{
	major = register_chrdev(0, "second_drv", &sencod_drv_fops);

	seconddrv_class = class_create(THIS_MODULE, "second_drv");

	device_create(seconddrv_class, NULL, MKDEV(major, 0), NULL, "buttons"); /* /dev/buttons */

	gph2con = (volatile unsigned long *)ioremap(0xe0200c40, 16);
	gph2dat = gph2con + 1;

	gph3con = (volatile unsigned long *)ioremap(0xE0200C60, 16);
	gph3dat = gph3con + 1;

	return 0;
}

static void second_drv_exit(void)
{
	unregister_chrdev(major, "second_drv");
	device_destroy(seconddrv_class, MKDEV(major, 0));
	class_destroy(seconddrv_class);
	iounmap(gph2con);
	iounmap(gph3con);
	return 0;
}


module_init(second_drv_init);

module_exit(second_drv_exit);

MODULE_LICENSE("GPL");

