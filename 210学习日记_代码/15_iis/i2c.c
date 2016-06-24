#include "lib.h"

/* GPIO */
#define GPD1CON		(*(volatile unsigned int *)0xE02000C0)
#define GPD1PUD		(*(volatile unsigned int *)0xE02000C8)

/* IIC */
#define IICCON		(*(volatile unsigned int *)0xE1800000)
#define IICSTAT    	(*(volatile unsigned int *)0xE1800004)
#define IICDS		(*(volatile unsigned int *)0xE180000C)

#define VIC0ADDRESS  		(*(volatile unsigned int *)0xF2000F00)
#define VIC1ADDRESS  		(*(volatile unsigned int *)0xF2100F00)
#define VIC2ADDRESS  		(*(volatile unsigned int *)0xF2200F00)
#define VIC3ADDRESS  		(*(volatile unsigned int *)0xF2300F00)

void Delay(int time);

#define WRDATA      (1)
#define RDDATA      (2)

typedef struct tI2C {
    unsigned char *pData;   /* 数据缓冲区 */
    volatile int DataCount; /* 等待传输的数据长度 */
    volatile int Status;    /* 状态 */
    volatile int Mode;      /* 模式：读/写 */
    volatile int Pt;        /* pData中待传输数据的位置 */
}t210_I2C, *pt210_I2C;

static t210_I2C g_t210_I2C;

void i2c_init(void)
{
	/* 选择引脚功能：GPE15:IICSDA, GPE14:IICSCL */
	GPD1CON |= 0x22;
	GPD1PUD |= 0x5;

	/* bit[7] = 1, 使能ACK
	* bit[6] = 0, IICCLK = PCLK/16
	* bit[5] = 1, 使能中断
	* bit[3:0] = 0xf, Tx clock = IICCLK/16
	* PCLK = 66.7MHz, IICCLK = 4.1MHz
	*/
	IICCON = (1<<7) | (0<<6) | (1<<5) | (0xf);  // 0xaf

	IICSTAT = 0x10;     // I2C串行输出使能(Rx/Tx)
}

/*
 * 主机发送
 * slvAddr : 从机地址，buf : 数据存放的缓冲区，len : 数据长度 
 */
void i2c_write(unsigned int slvAddr, unsigned char *buf, int len)
{
    g_t210_I2C.Mode = WRDATA;   // 写操作
    g_t210_I2C.Pt   = 0;        // 索引值初始为0
    g_t210_I2C.pData = buf;     // 保存缓冲区地址
    g_t210_I2C.DataCount = len; // 传输长度
    
    IICDS   = slvAddr;
    IICSTAT = 0xf0;         // 主机发送，启动
    
    /* 等待直至数据传输完毕 */    
    while (g_t210_I2C.DataCount != -1);
}
        
/*
 * 主机接收
 * slvAddr : 从机地址，buf : 数据存放的缓冲区，len : 数据长度 
 */
void i2c_read(unsigned int slvAddr, unsigned char *buf, int len)
{
    g_t210_I2C.Mode = RDDATA;   // 读操作
    g_t210_I2C.Pt   = -1;       // 索引值初始化为-1，表示第1个中断时不接收数据(地址中断)
    g_t210_I2C.pData = buf;     // 保存缓冲区地址
    g_t210_I2C.DataCount = len; // 传输长度
    
    IICDS        = slvAddr;
    IICSTAT      = 0xb0;    // 主机接收，启动
    
    /* 等待直至数据传输完毕 */    
    while (g_t210_I2C.DataCount != 0);
}

//----------IIC中断服务函数----------
void do_irq(void) 
{
	unsigned int iicSt,i;

	wy_printf("do_irq \n");
	
	iicSt  = IICSTAT; 

	if(iicSt & 0x8){ wy_printf("Bus arbitration failed\n"); }

	switch (g_t210_I2C.Mode)
	{    
		case WRDATA:
		{
			if((g_t210_I2C.DataCount--) == 0)
			{
				// 下面两行用来恢复I2C操作，发出P信号
				IICSTAT = 0xd0;
				IICCON  = 0xaf;
				Delay(10000);  // 等待一段时间以便P信号已经发出
				break;    
			}

			IICDS = g_t210_I2C.pData[g_t210_I2C.Pt++];

			// 将数据写入IICDS后，需要一段时间才能出现在SDA线上
			for (i = 0; i < 10; i++);   

			IICCON = 0xaf;      // 恢复I2C传输
			break;
		}

		case RDDATA:
		{
			if (g_t210_I2C.Pt == -1)
			{
				// 这次中断是发送I2C设备地址后发生的，没有数据
				// 只接收一个数据时，不要发出ACK信号
				g_t210_I2C.Pt = 0;
				if(g_t210_I2C.DataCount == 1)
					IICCON = 0x2f;   // 恢复I2C传输，开始接收数据，接收到数据时不发出ACK
				else 
					IICCON = 0xaf;   // 恢复I2C传输，开始接收数据
				break;
			}

			g_t210_I2C.pData[g_t210_I2C.Pt++] = IICDS;
			g_t210_I2C.DataCount--;

			if (g_t210_I2C.DataCount == 0)
			{

				// 下面两行恢复I2C操作，发出P信号
				IICSTAT = 0x90;
				IICCON  = 0xaf;
				Delay(10000);  // 等待一段时间以便P信号已经发出
				break;    
			}      
			else
			{           
				// 接收最后一个数据时，不要发出ACK信号
				if(g_t210_I2C.DataCount == 1)
					IICCON = 0x2f;   // 恢复I2C传输，接收到下一数据时无ACK
				else 
					IICCON = 0xaf;   // 恢复I2C传输，接收到下一数据时发出ACK
			}
			break;
		}

		default:
		    break;      
	}
	// 清中断向量
	VIC0ADDRESS = 0x0;
	VIC1ADDRESS = 0x0;
	VIC2ADDRESS = 0x0;
	VIC3ADDRESS = 0x0;
} 

/*
 * 延时函数
 */
void Delay(int time)
{
	for (; time > 0; time--);
}

unsigned char at24cxx_read(unsigned char address)
{
	unsigned char val;
	wy_printf("at24cxx_read address = %d\r\n", address);
	i2c_write(0xA0, &address, 1);
	wy_printf("at24cxx_read send address ok\r\n");
	i2c_read(0xA0, (unsigned char *)&val, 1);
	wy_printf("at24cxx_read get data ok\r\n");
	return val;
}

void at24cxx_write(unsigned char address, unsigned char data)
{
	unsigned char val[2];
	val[0] = address;
	val[1] = data;
	i2c_write(0xA0, val, 2);
}

void wm8960_write(unsigned int slave_addr, int addr, int data)
{
	unsigned char val[2];
	val[0] = addr<<1 | ((data>>8) & 0x0001);
	val[1] = (data & 0x00FF);
	i2c_write(slave_addr, val, 2);
}

