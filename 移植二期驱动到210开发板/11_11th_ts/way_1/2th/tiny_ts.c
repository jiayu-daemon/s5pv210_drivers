#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/plat-s3c24xx/ts.h>

#include <asm/arch/regs-adc.h>
#include <asm/arch/regs-gpio.h>

#define MHZ (1000*1000)
#define PRINT_MHZ(m) 			((m) / MHZ), ((m / 1000) % 1000)

struct tiny_ts_regs {
	unsigned long tsadccon1;
	unsigned long tscon1;
	unsigned long tsdly1;
	unsigned long tsdatx1;
	unsigned long tsdaty1;
	unsigned long tspenstat1;
	unsigned long clrintadc1;
	unsigned long reserved;
	unsigned long clrintpen1;
};

static struct input_dev *tiny_ts_dev;
static volatile struct tiny_ts_regs *tiny_ts_regs;
static unsigned long *tsadccon0;

static struct timer_list ts_timer;

static void enter_wait_pen_down_mode(void)
{
	tiny_ts_regs->tscon1 = 0xd3;
}

static void enter_wait_pen_up_mode(void)
{
	tiny_ts_regs->tscon1 = 0x1d3;
}

static void enter_measure_xy_mode(void)
{
	tiny_ts_regs->tscon1 = (1<<3)|(1<<2);
}

static void start_adc(void)
{
	tiny_ts_regs->tsadccon1 |= (1<<0);
}

static int tiny_filter_ts(int x[], int y[])
{
#define ERR_LIMIT 10

	int avr_x, avr_y;
	int det_x, det_y;

	avr_x = (x[0] + x[1])/2;
	avr_y = (y[0] + y[1])/2;

	det_x = (x[2] > avr_x) ? (x[2] - avr_x) : (avr_x - x[2]);
	det_y = (y[2] > avr_y) ? (y[2] - avr_y) : (avr_y - y[2]);

	if ((det_x > ERR_LIMIT) || (det_y > ERR_LIMIT))
		return 0;

	avr_x = (x[1] + x[2])/2;
	avr_y = (y[1] + y[2])/2;

	det_x = (x[3] > avr_x) ? (x[3] - avr_x) : (avr_x - x[3]);
	det_y = (y[3] > avr_y) ? (y[3] - avr_y) : (avr_y - y[3]);

	if ((det_x > ERR_LIMIT) || (det_y > ERR_LIMIT))
		return 0;
	
	return 1;
}

static void tiny_ts_timer_function(unsigned long data)
{
	if (tiny_ts_regs->tsdatx1 & (1<<15))
	{
		/* 已经松开 */
		enter_wait_pen_down_mode();
	}
	else
	{
		/* 测量X/Y坐标 */
		enter_measure_xy_mode();
		start_adc();
	}
}

static irqreturn_t pen_down_up_irq(int irq, void *dev_id)
{
	if (tiny_ts_regs->tsdatx1 & (1<<15))
	{
		//printk("pen up\n");
		enter_wait_pen_down_mode();
	}
	else
	{
		//printk("pen down\n");
		//enter_wait_pen_up_mode();
		enter_measure_xy_mode();
		start_adc();
	}
	return IRQ_HANDLED;
}

static irqreturn_t adc_irq(int irq, void *dev_id)
{
	static int cnt = 0;
	static int x[4], y[4];
	int adcdat0, adcdat1;
	
	
	/* 优化措施2: 如果ADC完成时, 发现触摸笔已经松开, 则丢弃此次结果 */
	adcdat0 = tiny_ts_regs->tsdatx1;
	adcdat1 = tiny_ts_regs->tsdaty1;

	if (tiny_ts_regs->tsdatx1 & (1<<15))
	{
		/* 已经松开 */
		cnt = 0;
		enter_wait_pen_down_mode();
	}
	else
	{
		// printk("adc_irq cnt = %d, x = %d, y = %d\n", ++cnt, adcdat0 & 0x3ff, adcdat1 & 0x3ff);
		/* 优化措施3: 多次测量求平均值 */
		x[cnt] = adcdat0 & 0x3ff;
		y[cnt] = adcdat1 & 0x3ff;
		++cnt;
		if (cnt == 4)
		{
			/* 优化措施4: 软件过滤 */
			if (tiny_filter_ts(x, y))
			{			
				printk("x = %d, y = %d\n", (x[0]+x[1]+x[2]+x[3])/4, (y[0]+y[1]+y[2]+y[3])/4);
			}
			cnt = 0;
			enter_wait_pen_up_mode();

			/* 启动定时器处理长按/滑动的情况 */
			mod_timer(&ts_timer, jiffies + HZ/100);
		}
		else
		{
			enter_measure_xy_mode();
			start_adc();
		}		
	}
	
	return IRQ_HANDLED;
}

static int s3c_ts_init(void)
{
	struct clk* clk;
	
	/* 1. 分配一个input_dev结构体 */
	tiny_ts_dev = input_allocate_device();

	/* 2. 设置 */
	/* 2.1 能产生哪类事件 */
	set_bit(EV_KEY, tiny_ts_dev->evbit);
	set_bit(EV_ABS, tiny_ts_dev->evbit);

	/* 2.2 能产生这类事件里的哪些事件 */
	set_bit(BTN_TOUCH, tiny_ts_dev->keybit);

	input_set_abs_params(tiny_ts_dev, ABS_X, 0, 0x3FF, 0, 0);
	input_set_abs_params(tiny_ts_dev, ABS_Y, 0, 0x3FF, 0, 0);
	input_set_abs_params(tiny_ts_dev, ABS_PRESSURE, 0, 1, 0, 0);


	/* 3. 注册 */
	input_register_device(tiny_ts_dev);

	/* 4. 硬件相关的操作 */
	/* 4.1 使能时钟(CLKCON[15]) */
	clk = clk_get(NULL, "adc");
	if (!clk || IS_ERR(clk)) {
		printk(KERN_INFO "failed to get adc clock source\n");
	}
	clk_enable(clk);
	printk("Tiny_TS clock got enabled :: %ld.%03ld Mhz\n", PRINT_MHZ(clk_get_rate(tiny_clk)));

	/* 4.2 设置S3C2440的ADC/TS寄存器 */
	tsadccon0 =  ioremap(0xE1700000, 4);
	tiny_ts_regs = ioremap(0xE1701000, sizeof(struct tiny_ts_regs));

	/* s5pv210有触摸屏接口0，1
	 * Tiny210开发板向外提供接口1
	 * 所以这里选择接口1
	 */
	tsadccon0 |= 1<<17;
	/* bit[14]  : 1-A/D converter prescaler enable
	 * bit[13:6]: A/D converter prescaler value,
	 *            65, ADCCLK=PCLKP/(65+1)=66.7MHz/(65+1)=1MHz
	 * bit[2]: 默认为1，即Standby mode，我们要设置为0，正常模式
	 * bit[0]: A/D conversion starts by enable. 先设为0
	 */
	tiny_ts_regs->tsadccon1 &= ~(1<<2);
	tiny_ts_regs->tsadccon1 |= (1<<14)|(65<<6);

	request_irq(IRQ_TC, pen_down_up_irq, IRQF_SAMPLE_RANDOM, "ts_pen", NULL);
	request_irq(IRQ_ADC, adc_irq, IRQF_SAMPLE_RANDOM, "adc", NULL);

	/* 优化措施1: 
	 * 设置ADCDLY为最大值, 这使得电压稳定后再发出IRQ_TC中断
	 */
	tiny_ts_regs->tsdaty1 = 0xffff;

	/* 优化措施5: 使用定时器处理长按,滑动的情况
	 * 
	 */
	init_timer(&ts_timer);
	ts_timer.function = tiny_ts_timer_function;
	add_timer(&ts_timer);

	enter_wait_pen_down_mode();
	
	return 0;
}

static void s3c_ts_exit(void)
{
	free_irq(IRQ_TC, NULL);
	free_irq(IRQ_ADC, NULL);
	iounmap(tiny_ts_regs);
	iounmap(tsadccon0);
	input_unregister_device(tiny_ts_dev);
	input_free_device(tiny_ts_dev);
	del_timer(&ts_timer);
}

module_init(s3c_ts_init);
module_exit(s3c_ts_exit);

MODULE_LICENSE("GPL");

