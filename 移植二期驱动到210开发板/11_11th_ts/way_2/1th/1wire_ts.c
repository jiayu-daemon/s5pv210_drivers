#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/param.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/io.h>

#include <asm/system.h>
#include <asm/leds.h>
#include <asm/mach-types.h>
#include <asm/irq.h>
#include <asm/mach/time.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <mach/map.h>
#include <mach/regs-irq.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>

#include <plat/regs-timer.h>	 
#include <plat/gpio-cfg.h>
#include <plat/clock.h>
#include <plat/cpu.h>

static DECLARE_WAIT_QUEUE_HEAD(ts_waitq);

#define MHZ (1000*1000)
#define PRINT_MHZ(m) 			((m) / MHZ), ((m / 1000) % 1000)
#define TOUCH_DEVICE_NAME	"tiny_ts"
#define SAMPLE_BPS 9600

#define SLOW_LOOP_FEQ 25
#define FAST_LOOP_FEQ 60

#define REQ_TS   0x40U
#define REQ_INFO 0x60U

enum {
	IDLE,
	START,
	REQUEST,
	WAITING,
	RESPONSE,
	STOPING,
} one_wire_status = IDLE;

struct timer_regs {
	unsigned long tcfg0;
	unsigned long tcfg1;
	unsigned long tcon;
	unsigned long reserved1[9];
	unsigned long tcntb3;
	unsigned long tcmpb3;
	unsigned long tcnto3;
	unsigned long reserved2[2];
	unsigned long tint_cstat;	
};

static struct timer_regs *timer_regs;
static unsigned long *gph1con;
static unsigned long *gph1dat;
static int ts_ready;
static unsigned ts_status;
static unsigned lcd_type, firmware_ver;
static unsigned total_received, total_error;
static unsigned last_req, last_res;
static unsigned long TCNT_FOR_SAMPLE_BIT;
static volatile unsigned int io_bit_count;
static volatile unsigned int io_data;
static volatile unsigned char one_wire_request;
static int exitting;
static struct timer_list ts_timer;

static inline void notify_ts_data(unsigned x, unsigned y, unsigned down)
{
	if (!down && !(ts_status &(1U << 31))) {
		// up repeat, give it up
		return;
	}

	ts_status = ((x << 16) | (y)) | (down << 31);
	ts_ready = 1;
	wake_up_interruptible(&ts_waitq);
}

static ssize_t ts_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	unsigned long err;

	if (!ts_ready) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event_interruptible(ts_waitq, ts_ready);
	}

	ts_ready = 0;

	if (count < sizeof ts_status) {
		return -EINVAL;
	} else {
		count = sizeof ts_status;
	}

	err = copy_to_user((void *)buffer, (const void *)(&ts_status), sizeof ts_status);
	return err ? -EFAULT : sizeof ts_status;
}

static unsigned int ts_poll( struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &ts_waitq, wait);

	if (ts_ready)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static struct file_operations ts_fops = {
	owner:		THIS_MODULE,
	read:		ts_read,	
	poll:   	ts_poll,
};

static struct miscdevice ts_misc = {
	.minor		= 181,
	.name		= TOUCH_DEVICE_NAME,
	.fops		= &ts_fops,
};

static inline void notify_info_data(unsigned char _lcd_type,
		unsigned char ver_year, unsigned char week)
{
	if (_lcd_type != 0xFF) {
		lcd_type = _lcd_type;
		firmware_ver = ver_year * 100 + week;
	}
}

static inline void set_pin_value(int v)
{
	if (v) {
		*gph1dat |= 1<<2;
	} else {
		*gph1dat &= ~(1<<2);
	}
}

static inline int get_pin_value(void)
{
	if(*gph1dat & (1<<2))
		return 1;
	else
		return 0;
}

// CRC
static const unsigned char crc8_tab[] = {
0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3,
};

#define crc8_init(crc) ((crc) = 0XACU)
#define crc8(crc, v) ( (crc) = crc8_tab[(crc) ^(v)])

// once a session complete
static void one_wire_session_complete(unsigned char req, unsigned int res)
{
	unsigned char crc;
	const unsigned char *p = (const unsigned char*)&res;
	total_received ++;

	last_res = res;

	crc8_init(crc);
	crc8(crc, p[3]);
	crc8(crc, p[2]);
	crc8(crc, p[1]);

	if (crc != p[0]) {
		// CRC dismatch
		if (total_received > 100) {
			total_error++;
		}
		return;
	}

	switch(req) {
		case REQ_TS:
			{
				unsigned short x,y;
				unsigned pressed;
				x =  ((p[3] >>   4U) << 8U) + p[2];
				y =  ((p[3] &  0xFU) << 8U) + p[1];
				pressed = (x != 0xFFFU) && (y != 0xFFFU); 
				notify_ts_data(x, y, pressed);
			}
			break;

		case REQ_INFO:
			notify_info_data(p[3], p[2], p[1]);
			break;
		default:
			break;
	}
}

// one-wire protocol core
static int init_timer3_for_ts(void)
{
	struct clk *clk;
	unsigned long pclk;
	unsigned long prescaler;
	unsigned long prescale1_value;

	/* 获得时钟，并使能时钟 */
	clk = clk_get(NULL, "timers");
	if (IS_ERR(clk)) {
		printk("ERROR to get PCLK\n");
		return -EIO;
	}
	clk_enable(clk);
	pclk = clk_get_rate(clk);
	printk("Timer3 clock source : %ld.%03ld Mhz\n", PRINT_MHZ(pclk));

	/* 获得定时器的预定标器的值,目的是为了不修改该值，因为定时器2，3，4共享它 */
	prescaler = timer_regs->tcfg0;
	prescale1_value = (prescaler >> 8) & 0xFF;

	// calc the TCNT_FOR_SAMPLE_BIT, that is one of the goal
	TCNT_FOR_SAMPLE_BIT = pclk / (prescale1_value + 1) / SAMPLE_BPS - 1;

	/* 定时器3的分频系数为1 */
	timer_regs->tcfg1 &= (0xf<<12); 

	printk("TCNT_FOR_SAMPLE_BIT = %d\n", TCNT_FOR_SAMPLE_BIT);
	return 0;
}

static inline void stop_timer_for_1wire(void)
{
	timer_regs->tcon &= ~(1<<16);/* 停止定时器3 */
}

static irqreturn_t timer_for_1wire_interrupt(int irq, void *dev_id)
{
	timer_regs->tint_cstat |= 0x100; /* 清中断 */
  
	io_bit_count--;
	switch(one_wire_status) {
	case START:
		if (io_bit_count == 0) {
			io_bit_count = 16;
			one_wire_status = REQUEST;
		}
		break;

	case REQUEST:
		// Send a bit
		set_pin_value(io_data & (1U << 31));
		io_data <<= 1;
		if (io_bit_count == 0) {
			io_bit_count = 2;
			one_wire_status = WAITING;
		}
		break;
		
	case WAITING:
		if (io_bit_count == 0) {
			io_bit_count = 32;
			one_wire_status = RESPONSE;
		}
		if (io_bit_count == 1) {
			*gph1con &= ~(1<<8);/* 设置为输入 */
			set_pin_value(1);
		}
		break;
		
	case RESPONSE:
		// Get a bit
		io_data = (io_data << 1) | get_pin_value();
		if (io_bit_count == 0) {
			io_bit_count = 2;
			one_wire_status = STOPING;
			set_pin_value(1);
			*gph1con |= 1<<8;/* 设置为输出 */
			one_wire_session_complete(one_wire_request, io_data);
		}
		break;

	case STOPING:
		if (io_bit_count == 0) {
			one_wire_status = IDLE;
			stop_timer_for_1wire();
		}
		break;
		
	default:
		stop_timer_for_1wire();
	}
	return IRQ_HANDLED;
}

static struct irqaction timer_for_1wire_irq = {
	.name    = "1-wire Timer Tick",
	.flags   = IRQF_DISABLED | IRQF_IRQPOLL,
	.handler = timer_for_1wire_interrupt,
	.dev_id  = &timer_for_1wire_irq,
};

static void start_one_wire_session(unsigned char req)
{
	unsigned long flags;

	if (one_wire_status != IDLE) {
		return;
	}

	one_wire_status = START;

	set_pin_value(1);
	*gph1con |= 1<<8; /* 设置为输出 */
	/*  IDLE to START */
	{
		unsigned char crc;
		crc8_init(crc);
		crc8(crc, req);
		io_data = (req << 8) + crc;
		io_data <<= 16;
	}
	last_req = (io_data >> 16);
	one_wire_request = req;
	io_bit_count = 1;
	*gph1con |= 1<<8; /* 设置为输出 */

	timer_regs->tcntb3 = TCNT_FOR_SAMPLE_BIT;
	/* 不手动装载，不电平反转，不自动装载 */
	timer_regs->tcon &=~(0xF << 16); 
	timer_regs->tcon |= 1<<17;/* 手动转载数据 */

	local_irq_save(flags);
	timer_regs->tcon |= 1<<16;/* 开启定时器3，并开启自动装载 */
	timer_regs->tcon |= 1<<19;
	timer_regs->tcon &= ~(1<<17);/* 关闭手动装载 */
	set_pin_value(0);
	local_irq_restore(flags);
}

void one_wire_timer_proc(unsigned long v)
{
	unsigned char req;

	if (exitting) {
		return;
	}

	/* 设置查询周期为20ms,并启动内核定时器 */
	ts_timer.expires = jiffies + HZ / 50;
	add_timer(&ts_timer);

	if (lcd_type == 0) {
		req = REQ_INFO;
	} else {
		req = REQ_TS;
	}

	start_one_wire_session(req);
}

static int read_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len;
	len = sprintf(buf, "%u %u %u %u %04X %08X\n",
			lcd_type, firmware_ver, total_received, total_error, last_req, last_res);
	*eof = 1;
	return len;
}

static int dev_init(void)
{
	int ret;

	ret = misc_register(&ts_misc);
	if (ret == 0) {
		create_proc_read_entry("driver/one-wire-info", 0, NULL, read_proc, NULL);
		printk (TOUCH_DEVICE_NAME"\tinitialized\n");
	}
	
	timer_regs = ioremap(0xE2500000, sizeof(struct timer_regs));
	gph1con = ioremap(0xE0200C20, 8);
	gph1dat = gph1con + 1;

	/* 设置GPH1_2为输出,并输出1 */
	*gph1con |= 1<<8;
	*gph1dat |= 1<<2;

	setup_irq(IRQ_TIMER3, &timer_for_1wire_irq);
	init_timer3_for_ts();
	/* 初始化内核的定时器 */
	init_timer(&ts_timer);
	ts_timer.function = one_wire_timer_proc;
	one_wire_timer_proc(0);

	/* 使能定时器3中断，清中断 */
	timer_regs->tint_cstat |= 0x108; 
	return ret;
}

static void dev_exit(void)
{
	exitting = 1;
	remove_proc_entry("driver/one-wire-info", NULL);
	misc_deregister(&ts_misc);
	del_timer_sync(&ts_timer);
	free_irq(IRQ_TIMER3, &timer_for_1wire_irq);
	iounmap(timer_regs);
	iounmap(gph1con);
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");

