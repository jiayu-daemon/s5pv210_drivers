#include "lib.h"

#define VIC0INTSELECT		(*(volatile unsigned int *)0xF200000C)
#define VIC1INTSELECT		(*(volatile unsigned int *)0xF210000C)
#define VIC2INTSELECT		(*(volatile unsigned int *)0xF220000C)
#define VIC3INTSELECT		(*(volatile unsigned int *)0xF230000C)

#define VIC0INTENABLE		(*(volatile unsigned int *)0xF2000010)

#define VIC0INTENCLEAR		(*(volatile unsigned int *)0xF2000014)
#define VIC1INTENCLEAR		(*(volatile unsigned int *)0xF2100014)
#define VIC2INTENCLEAR		(*(volatile unsigned int *)0xF2200014)
#define VIC3INTENCLEAR		(*(volatile unsigned int *)0xF2300014)

#define VIC0ADDRESS  		(*(volatile unsigned int *)0xF2000F00)
#define VIC1ADDRESS  		(*(volatile unsigned int *)0xF2100F00)
#define VIC2ADDRESS  		(*(volatile unsigned int *)0xF2200F00)
#define VIC3ADDRESS  		(*(volatile unsigned int *)0xF2300F00)

#define VIC1VECTADDR14  	(*(volatile unsigned int *)0xF2100138)
#define VIC1INTENABLE     	(*(volatile unsigned int *)0xF2100010)

extern void key_IRQ(void);
void irq_init(void)
{
	/* 设置为IRQ中断 */
	VIC1INTSELECT &= ~(1<<14);
	/* 使能中断(中断控制器里面的) */
	VIC1INTENABLE |= 1<<14;
	/* 设置中断向量 */
	VIC1VECTADDR14 = (int)key_IRQ;
}

