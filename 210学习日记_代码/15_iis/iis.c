// GPIO
#define GPICON  	(*(volatile unsigned int *)0xE0200220)	//IIS Signals

// IIS
#define IISCON  	(*(volatile unsigned int *)0xEEE30000)	//IIS Control
#define IISMOD  	(*(volatile unsigned int *)0xEEE30004)	//IIS Mode
#define IISFIC  	(*(volatile unsigned int *)0xEEE30008)	//IIS FIFO Control
#define IISPSR  	(*(volatile unsigned int *)0xEEE3000C)	//IIS Prescaler
#define IISTXD		(*(volatile unsigned int *)0xEEE30010)	//IIS TXD DATA
#define IISRXD 		(*(volatile unsigned int *)0xEEE30014)	//IIS RXD DATA
#define IISFICS  	(*(volatile unsigned int *)0xEEE30018)	//IIS FIFO Control

//CLCK
#define EPLL_CON0  	(*(volatile unsigned int *)0xe0100110)
#define EPLL_CON1  	(*(volatile unsigned int *)0xe0100114)
#define CLK_SRC0  	(*(volatile unsigned int *)0xE0100200)		
#define CLK_CON  	(*(volatile unsigned int *)0xEEE10000)	

void IIS_init(void)
{
	/* 设置对应GPIO用于IIS */
	GPICON = 0x22222222;

	/* 设置锁相环
	 * SDIV [2:0]  : SDIV = 0x3
	 * PDIV [13:8] : PDIV = 0x3
	 * MDIV [24:16]: MDIV = 0x43
	 * LOCKED  [29]: 1 = 使能锁
	 * ENABLE  [31]: 1 = 使能锁相环
	 *
	 * Fout = (0x43+0.7)*24M / (3 * 2^3) = 67.7*24M/24 = 67.7Mhz
	 */
	EPLL_CON0 = 0xa8430303;	/* MPLL_FOUT = 67.7Mhz */
	EPLL_CON1 = 0xbcee;          /* K = 0xbcee */

	/* 时钟源的设置
	 * APLL_SEL[0] :1 = FOUTAPLL
	 * MPLL_SEL[4] :1 = FOUTMPLL
	 * EPLL_SEL[8] :1 = FOUTEPLL
	 * VPLL_SEL[12]:1 = FOUTVPLL
	 * MUX_MSYS_SEL[16]:0 = SCLKAPLL
	 * MUX_DSYS_SEL[20]:0 = SCLKMPLL
	 * MUX_PSYS_SEL[24]:0 = SCLKMPLL
	 * ONENAND_SEL [28]:1 = HCLK_DSYS
	 */	
	CLK_SRC0 = 0x10001111;

	/* 时钟源的进一步设置(AUDIO SUBSYSTEMCLK SRC)
	 * bit[3:2]: 00 = MUXi2s_a_out来源于Main CLK
	 * bit[0]  : 1 = Main CLK来源于FOUT_EPLL
	 */
	CLK_CON = 0x1;
	/* 由于AUDIO SUBSYSTEMCLK DIV寄存器使用的是默认值，故分频系数为1 */
			
	// IISCDCLK  11.289Mhz = 44.1K * 256fs 
	// IISSCLK    1.4112Mhz = 44.1K * 32fs
	// IISLRCLK   44.1Khz
	/* 预分频值
	 * bit[13:8] : N = 5
	 * bit[15]   : 使能预分频
	 */
	IISPSR = 1<<15 | 5<<8;
	
	/* 设置IIS控制器
	 * bit[0]: 1 = 使能IIS
	 */
	IISCON |= 1<<0 | (unsigned)1<<31;
	
	/* 设置各个时钟输出
	 * bit[2:1]:IISSCLK(位时钟)  44.1K * 32fs = 1.4112Mhz
	 * bit[3:4]:IISCDCLK(系统时钟) 44.1K * 256fs = 11.289Mhz
	 * bit[9:8]:10 = 既可以发送又可以接收
	 * bit[10] :0 = PCLK is internal source clock for IIS 
	 */
	IISMOD = 1<<9 | 0<<8 | 1<<10;
}
