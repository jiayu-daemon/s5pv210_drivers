#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/gpio.h>
#include <linux/poll.h>

static struct class *sixthdrv_class;

volatile unsigned long *gph2con;
volatile unsigned long *gph2dat;

volatile unsigned long *gph3con;
volatile unsigned long *gph3dat;

static struct timer_list buttons_timer;

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

/* 中断事件标志, 中断服务程序将它置1，sixth_drv_read将它清0 */
static volatile int ev_press = 0;

static struct fasync_struct *button_async;

struct pin_desc{
	unsigned int pin;
	unsigned int key_val;
};

/* 键值: 按下时, 0x01, 0x02, 0x03, 0x04, 0x05 */
/* 键值: 松开时, 0x81, 0x82, 0x83, 0x84, 0x85 */
static unsigned char key_val;

struct pin_desc pins_desc[5] = {
	{S5PV210_GPH2(3), 0x01},
	{S5PV210_GPH3(0), 0x02},
	{S5PV210_GPH3(1), 0x03},
	{S5PV210_GPH3(2), 0x04},
	{S5PV210_GPH3(3), 0x05},
};

static struct pin_desc *irq_pd;

//static atomic_t canopen = ATOMIC_INIT(1);     //定义原子变量并初始化为1

static DECLARE_MUTEX(button_lock);     //定义互斥锁

/*
 * 确定按键值
 */
static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	/* 10ms后启动定时器 */
	irq_pd = (struct pin_desc *)dev_id;
	mod_timer(&buttons_timer, jiffies+HZ/100);
	return IRQ_RETVAL(IRQ_HANDLED);
}

static int sixth_drv_open(struct inode *inode, struct file *file)
{
#if 0	
	if (!atomic_dec_and_test(&canopen))
	{
		atomic_inc(&canopen);
		return -EBUSY;
	}
#endif		

	if (file->f_flags & O_NONBLOCK)
	{
		if (down_trylock(&button_lock))
			return -EBUSY;
	}
	else
	{
		/* 获取信号量 */
		down(&button_lock);
	}

	/* 注册中断 */
	request_irq(IRQ_EINT(19),  buttons_irq, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "K4", &pins_desc[0]);
	request_irq(IRQ_EINT(24),  buttons_irq, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "K5", &pins_desc[1]);
	request_irq(IRQ_EINT(25), buttons_irq, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "K6", &pins_desc[2]);
	request_irq(IRQ_EINT(26), buttons_irq, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "K7", &pins_desc[3]);	
	request_irq(IRQ_EINT(27), buttons_irq, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "K8", &pins_desc[4]);	

	return 0;
}

ssize_t sixth_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	if (size != 1)
		return -EINVAL;

	if (file->f_flags & O_NONBLOCK)
	{
		if (!ev_press)
			return -EAGAIN;
	}
	else
	{
		/* 如果没有按键动作, 休眠 */
		wait_event_interruptible(button_waitq, ev_press);
	}

	/* 如果有按键动作, 返回键值 */
	copy_to_user(buf, &key_val, 1);
	ev_press = 0;
	
	return 1;
}


int sixth_drv_close(struct inode *inode, struct file *file)
{
	//atomic_inc(&canopen);
	free_irq(IRQ_EINT(19), &pins_desc[0]);
	free_irq(IRQ_EINT(24), &pins_desc[1]);
	free_irq(IRQ_EINT(25), &pins_desc[2]);
	free_irq(IRQ_EINT(26), &pins_desc[3]);
	free_irq(IRQ_EINT(27), &pins_desc[4]);
	up(&button_lock);
	return 0;
}

static unsigned sixth_drv_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	poll_wait(file, &button_waitq, wait); // 不会立即休眠

	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int sixth_drv_fasync (int fd, struct file *filp, int on)
{
	printk("driver: sixth_drv_fasync\n");
	return fasync_helper (fd, filp, on, &button_async);
}

static struct file_operations sencod_drv_fops = {
	.owner   =  THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
	.open    =  sixth_drv_open,     
	.read	 =	sixth_drv_read,	   
	.release =  sixth_drv_close,
	.poll    =  sixth_drv_poll,
	.fasync	 =  sixth_drv_fasync,
};

int major;

static void buttons_timer_function(unsigned long data)
{
	struct pin_desc * pindesc = irq_pd;
	unsigned int pinval;

	if (!pindesc)
		return;
	
	pinval = gpio_get_value(pindesc->pin);

	if (pinval)
	{
		/* 松开 */
		key_val = 0x80 | pindesc->key_val;
	}
	else
	{
		/* 按下 */
		key_val = pindesc->key_val;
	}

	ev_press = 1;                  /* 表示中断发生了 */
	wake_up_interruptible(&button_waitq);   /* 唤醒休眠的进程 */
	
	kill_fasync (&button_async, SIGIO, POLL_IN);
}

static int sixth_drv_init(void)
{
	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_function;
	//buttons_timer.expires  = 0;
	add_timer(&buttons_timer); 

	major = register_chrdev(0, "sixth_drv", &sencod_drv_fops);

	sixthdrv_class = class_create(THIS_MODULE, "sixth_drv");

	device_create(sixthdrv_class, NULL, MKDEV(major, 0), NULL, "buttons"); /* /dev/buttons */

	gph2con = (volatile unsigned long *)ioremap(0xe0200c40, 16);
	gph2dat = gph2con + 1;

	gph3con = (volatile unsigned long *)ioremap(0xE0200C60, 16);
	gph3dat = gph3con + 1;

	return 0;
}

static void sixth_drv_exit(void)
{
	unregister_chrdev(major, "sixth_drv");
	device_destroy(sixthdrv_class, MKDEV(major, 0));
	class_destroy(sixthdrv_class);
	iounmap(gph2con);
	iounmap(gph3con);
}

module_init(sixth_drv_init);
module_exit(sixth_drv_exit);

MODULE_LICENSE("GPL");

