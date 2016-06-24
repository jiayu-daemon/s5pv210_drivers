/* linux/arch/arm/mach-s5pv210/mach-smdkv210.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/dm9000.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/clk.h>
#include <linux/delay.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>

#include <plat/regs-serial.h>
#include <plat/s5pv210.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/gpio-cfg.h>

#define S5P_MEMREG(x)		(S5P_VA_SROMC + (x))

#define S5P_SROM_BW		S5P_MEMREG(0x00)
#define S5P_SROM_BC0		S5P_MEMREG(0x04)
#define S5P_SROM_BC1		S5P_MEMREG(0x08)
#define S5P_SROM_BC2		S5P_MEMREG(0x0C)
#define S5P_SROM_BC3		S5P_MEMREG(0x10)
#define S5P_SROM_BC4		S5P_MEMREG(0x14)
#define S5P_SROM_BC5		S5P_MEMREG(0x18)
#define S5PV210_PA_USB_EHCI	(0xEC200000)
#define S5P_PA_USB_EHCI		S5PV210_PA_USB_EHCI
#define S5P_SZ_USB_EHCI     	SZ_1M

#define S5PV210_PA_USB_OHCI	(0xEC300000)
#define S5P_PA_USB_OHCI		S5PV210_PA_USB_OHCI
#define S5P_SZ_USB_OHCI     	SZ_1M
/* USB2.0 OTG Controller register */
unsigned long *uphypwr;
unsigned long *uphyclk;
unsigned long *urstcon;

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define S5PV210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define S5PV210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define S5PV210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

/* physical address for dm9000a ...kgene.kim@samsung.com */
#define S5PV210_PA_DM9000_A     (0x88001000)
#define S5PV210_PA_DM9000_F     (S5PV210_PA_DM9000_A + 0x300C)

static struct resource dm9000_resources[] = {
	[0] = {
		.start	= S5PV210_PA_DM9000_A,
		.end	= S5PV210_PA_DM9000_A + SZ_1K*4 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= S5PV210_PA_DM9000_F,
		.end	= S5PV210_PA_DM9000_F + SZ_1K*4 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= IRQ_EINT(7),
		.end	= IRQ_EINT(7),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct s3c2410_uartcfg smdkv210_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
};

static struct dm9000_plat_data dm9000_platdata = {
	.flags		= DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM,
	.dev_addr	= { 0x08, 0x90, 0x00, 0xa0, 0x02, 0x10 },
};

struct platform_device tiny210_device_dm9000 = {
	.name		= "dm9000",
	.id			= -1,
	.num_resources	= ARRAY_SIZE(dm9000_resources),
	.resource	= dm9000_resources,
	.dev		= {
		.platform_data	= &dm9000_platdata,
	},
};

static void __init tiny210_dm9000_set(void)
{
	unsigned int tmp;

	tmp = ((0<<28)|(0<<24)|(5<<16)|(0<<12)|(0<<8)|(0<<4)|(0<<0));
	__raw_writel(tmp, (S5P_SROM_BW+0x08));

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= ~(0xf << 4);
	tmp |= (0x1 << 4); /* dm9000 16bit */
	__raw_writel(tmp, S5P_SROM_BW);

	gpio_request(S5PV210_MP01(1), "nCS1");
	s3c_gpio_cfgpin(S5PV210_MP01(1), S3C_GPIO_SFN(2));
	gpio_free(S5PV210_MP01(1));
}

static struct platform_device *smdkv210_devices[] __initdata = {
	&s5pv210_device_iis0,
	&s5pv210_device_ac97,
	&s3c_device_adc,
	&s3c_device_ts,
	&s3c_device_wdt,
	&tiny210_device_dm9000,
};

static struct s3c2410_ts_mach_info s3c_ts_platform __initdata = {
	.delay			= 10000,
	.presc			= 49,
	.oversampling_shift	= 2,
};

static void __init smdkv210_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdkv210_uartcfgs, ARRAY_SIZE(smdkv210_uartcfgs));
}

static void __init smdkv210_machine_init(void)
{
	s3c24xx_ts_set_platdata(&s3c_ts_platform);
	platform_add_devices(smdkv210_devices, ARRAY_SIZE(smdkv210_devices));
	tiny210_dm9000_set();
}

MACHINE_START(SMDKV210, "SMDKV210")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= s5pv210_init_irq,
	.map_io		= smdkv210_map_io,
	.init_machine	= smdkv210_machine_init,
	.timer		= &s3c24xx_timer,
MACHINE_END
