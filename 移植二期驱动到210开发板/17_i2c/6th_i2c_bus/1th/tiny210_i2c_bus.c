#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <asm/irq.h>

#include <plat/regs-iic.h>
#include <plat/iic.h>

static u32 s5pv210_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}

static int s5pv210_i2c_xfer(struct i2c_adapter *adap,
			struct i2c_msg *msgs, int num)
{
	return -EREMOTEIO;
}

static const struct i2c_algorithm s5pv210_i2c_algorithm = {
	.master_xfer		= s5pv210_i2c_xfer,
	.functionality		= s5pv210_i2c_func,
};

/*1.分配设置一个i2c_adapter*/
static struct i2c_adapter s5pv210_adapter = {
	.owner		= THIS_MODULE,
	.algo		= &s5pv210_i2c_algorithm,
	.name             = "tiny210",
};

static int tiny_i2c_bus_init(void)
{
	/* 2. 注册i2c_adapter */
	i2c_add_adapter(&s5pv210_adapter);
	return 0;
}

static int tiny_i2c_bus_exit(void)
{
	i2c_del_adapter(&s5pv210_adapter);
}

module_init(tiny_i2c_bus_init);
module_exit(tiny_i2c_bus_exit);

MODULE_LICENSE("GPL");

