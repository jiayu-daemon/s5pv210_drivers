#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>

#include <plat/regs-nand.h>
#include <plat/nand.h>

struct nand_regs {
	unsigned long nfconf;
	unsigned long nfcont;
	unsigned long nfcmmd;
	unsigned long nfaddr;
	unsigned long nfdata;
	unsigned long nfmeccd0;
	unsigned long nfmeccd1;
	unsigned long nfseccd;
	unsigned long nfsblk;
	unsigned long nfeblk;
	unsigned long nfstat;
	unsigned long nfeccerr0;
	unsigned long nfeccerr1;
};

static struct nand_regs *nand_regs;
static struct nand_chip *tiny_nand_chip;
static struct mtd_info *tiny_nand_mtd;

static void tiny_nand_select_chip(struct mtd_info *mtd, int chipnr)
{
	if(chipnr == -1)
	{
		/*取消选择*/
		nand_regs->nfcont |= (1<<1);
	}
	else
	{
		/*选中芯片*/
		nand_regs->nfcont &= ~(1<<1);
	}
}

static void tiny_nand_cmd_ctrl(struct mtd_info *mtd, int dat,
				unsigned int ctrl)
{

	if (ctrl & NAND_CLE)
	{
		/*发命令*/
		nand_regs->nfcmmd = dat;
	}
	else
	{
		/*发地址*/
		nand_regs->nfaddr = dat;
	}
}

static int tiny_nand_dev_ready(struct mtd_info *mtd, struct nand_chip *chip)
{
	/*等待命令的操作完成*/
	return (nand_regs->nfstat & (1<<0));
}

static int tiny_nand_init(void)
{
	struct clk *nand_clk;
	/*1.分配一个nand_chip结构体*/
	tiny_nand_chip = kzalloc(sizeof(struct nand_chip),GFP_KERNEL);
	nand_regs = ioremap(0xB0E00000,sizeof(struct nand_regs));

	/*2.设置*/
	/*
	 * 初始化nand_chip结构体中的函数指针
	 * 提供选中芯片，发命令，发地址，读数据，写数据，等待等操作
	 */
	tiny_nand_chip->select_chip    = tiny_nand_select_chip;
	tiny_nand_chip->cmd_ctrl        = tiny_nand_cmd_ctrl;
	tiny_nand_chip->IO_ADDR_R   = &nand_regs->nfdata;
	tiny_nand_chip->IO_ADDR_W  = &nand_regs->nfdata;
	tiny_nand_chip->dev_ready     = tiny_nand_dev_ready;
	tiny_nand_chip->ecc.mode      = NAND_ECC_SOFT;
	/*3.硬件相关*/
	/*使能时钟*/
	nand_clk = clk_get(NULL, "nand");
	clk_enable(nand_clk);

	/*
	 * AddrCycle[1]:1 = 发送地址需要5个周期
	 */
	nand_regs->nfconf |= 1<<1;
#define TWRPH1    1
#define TWRPH0    1
#define TACLS        1
	nand_regs->nfconf |= (TACLS<<12) | (TWRPH0<<8) | (TWRPH1<<4);
	/*
	 * MODE[0]:1     = 使能Nand Flash控制器
	 * Reg_nCE0[1]:1 = 取消片选
	 */
	nand_regs->nfcont |= (1<<1)|(1<<0);
	/*4.使用*/
	tiny_nand_mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	tiny_nand_mtd->owner = THIS_MODULE;
	tiny_nand_mtd->priv = tiny_nand_chip;

	nand_scan(tiny_nand_mtd, 1);
}

static void tiny_nand_exit(void)
{
	kfree(tiny_nand_mtd);
	iounmap(nand_regs);
	kfree(tiny_nand_chip);
}

module_init(tiny_nand_init);
module_exit(tiny_nand_exit);

MODULE_LICENSE("GPL");

