/* 注意:本驱动是给S70的LCD写的驱动，若为其他的LCD，应该修改时钟和时序 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <asm/io.h>
#include <asm/div64.h>

#include <asm/mach/map.h>
#include <mach/regs-gpio.h>

#define MHZ (1000*1000)
#define PRINT_MHZ(m) 			((m) / MHZ), ((m / 1000) % 1000)
/* LCD参数 */
#define VSPW       9  
#define VBPD       13 
#define LINEVAL    479
#define VFPD       21 

#define HSPW       19 
#define HBPD       25 
#define HOZVAL     799
#define HFPD       209

#define LeftTopX     0
#define LeftTopY     0
#define RightBotX   799
#define RightBotY   479

/* LCD控制寄存器 */
static unsigned long *vidcon0;	/* video control 0 */                          
static unsigned long *vidcon1;	/* video control 1 */                          
static unsigned long *vidcon2;	/* video control 2 */                          

static unsigned long *vidtcon0; /* video time control 0 */                   
static unsigned long *vidtcon1; /* video time control 1 */                   
static unsigned long *vidtcon2; /* video time control 2 */

static unsigned long *wincon0;	/* window control 0 */                         

static unsigned long *vidosd0a;	/* video window 0 position control */        
static unsigned long *vidosd0b;	/* video window 0 position control1 */       
static unsigned long *vidosd0c;	/* video window 0 position control */

static unsigned long *vidw00add0b0; 	/* window 0 buffer start address, buffer 0 */
static unsigned long *vidw00add1b0; 	/* window 0 buffer end address, buffer 0 */  
static unsigned long *vidw00add2; 	/* window 0 buffer size */   

static unsigned long *wpalcon;
static unsigned long *shadowcon;

/* 用于LCD的GPIO */
static unsigned long *gpf0con;
static unsigned long *gpf1con;
static unsigned long *gpf2con;
static unsigned long *gpf3con;
static unsigned long *gpd0con;
static unsigned long *gpd0dat;
/*时钟*/
static unsigned long *clk_gate_block;
static unsigned long *display_control;

static struct fb_info *tiny_lcd;
static u32 pseudo_pal[16];

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int tiny_lcdfb_setcolreg(unsigned int regno, unsigned int red,
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info)
{
	unsigned int val;
	
	if (regno > 16)
		return 1;

	/* 用red,green,blue三原色构造出val */
	val  = chan_to_field(red,	&info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue,	&info->var.blue);
	
	//((u32 *)(info->pseudo_palette))[regno] = val;
	pseudo_pal[regno] = val;
	return 0;
}

static struct fb_ops tiny_lcdfb_ops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= tiny_lcdfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static int tiny_lcd_init(void)
{
	struct clk		*tiny_clk;
	/* 1. 分配一个fb_info */
	tiny_lcd = framebuffer_alloc(0, NULL);
	
	/* 2. 设置 */
	/* 2.1 设置固定的参数 */
	strcpy(tiny_lcd->fix.id, "tiny_lcd");
	//tiny_lcd->fix.smem_start = ;  //显存的物理起始地址
	tiny_lcd->fix.smem_len = 800*480*4;
	tiny_lcd->fix.type = FB_TYPE_PACKED_PIXELS;
	tiny_lcd->fix.visual = FB_VISUAL_TRUECOLOR;
	tiny_lcd->fix.line_length = 800*4;

	/* 2.2 设置可变的参数 */
	tiny_lcd->var.xres             = 800;
	tiny_lcd->var.yres             = 480;
	tiny_lcd->var.xres_virtual  = 800;
	tiny_lcd->var.yres_virtual  = 480;
	tiny_lcd->var.bits_per_pixel = 32;
	/*RGB = 8:8:8*/
	tiny_lcd->var.red.offset          = 16;
	tiny_lcd->var.red.length         = 8;
	tiny_lcd->var.green.offset       = 8;
	tiny_lcd->var.green.length      = 8;
	tiny_lcd->var.blue.offset         = 0;
	tiny_lcd->var.blue.length        = 8;
	tiny_lcd->var.activate             = FB_ACTIVATE_NOW;
	
	/* 2.3 设置操作函数 */
	tiny_lcd->fbops = &tiny_lcdfb_ops;
	/* 2.4 其他的设置 */
	//tiny_lcd->screen_base = ; /*显存的虚拟起始地址*/
	tiny_lcd->screen_size = 800*480*4;
	tiny_lcd->pseudo_palette = pseudo_pal;

	/* 3. 硬件相关的操作 */
	/* 3.1 配置GPIO用于LCD */
	gpf0con = ioremap(0xE0200120,4);
	gpf1con = ioremap(0xE0200140,4);
	gpf2con = ioremap(0xE0200160,4);
	gpf3con = ioremap(0xE0200180,4);
	gpd0con = ioremap(0xE02000A0,4);
	gpd0dat = ioremap(0xE02000A4,4);
	*gpf0con = 0x22222222;		// GPF0[7:0]
	*gpf1con = 0x22222222;		// GPF1[7:0]
	*gpf2con = 0x22222222;		// GPF2[7:0]
	*gpf3con = 0x22222222;		// GPF3[7:0]
	*gpd0con |= 1<<4;
	*gpd0dat |= 1<<1;

	/* 3.2 使能时钟 */
	tiny_clk = clk_get(NULL, "lcd");
	if (!tiny_clk || IS_ERR(tiny_clk)) {
		printk(KERN_INFO "failed to get lcd clock source\n");
	}
	clk_enable(tiny_clk);
	printk("Tiny_LCD clock got enabled :: %ld.%03ld Mhz\n", PRINT_MHZ(clk_get_rate(tiny_clk)));

	/* 3.3 根据LCD手册设置LCD控制器, 比如VCLK的频率等 */
	display_control = ioremap(0xe0107008,4);
	vidcon0 = ioremap(0xF8000000,4);
	vidcon1 = ioremap(0xF8000004,4);
	wincon0 = ioremap(0xF8000020,4);
	vidosd0a = ioremap(0xF8000040,4);
	vidosd0b = ioremap(0xF8000044,4);
	vidosd0c = ioremap(0xF8000048,4);
	vidw00add0b0  = ioremap(0xF80000A0,4);
	vidw00add1b0  = ioremap(0xF80000D0,4);
	vidw00add2  = ioremap(0xF8000100,4);
	vidtcon0  = ioremap(0xF8000010,4);
	vidtcon1  = ioremap(0xF8000014,4);
	vidtcon2  = ioremap(0xF8000018,4);
	wpalcon  = ioremap(0xF80001A0,4);
	shadowcon  = ioremap(0xF8000034,4);
	
	/*
	 * Display path selection 
	 * 10: RGB=FIMD I80=FIMD ITU=FIMD
	 */
	*display_control = 2<<0;

	/* 
	 * CLKVAL_F[13:6]:该值需要修改，因为看错手册了
	 * 			     HCLKD=166.75MHz，DCLK(min) = 20ns(50MHz)
	 *                VCLK = 166.75 / (4+1) = 33.35MHz
	 * CLKDIR  [4]:1 = Divided by CLKVAL_F
	 * ENVID   [1]:1 = Enable the video output and the Display control signal. 
	 * ENVID_F [0]:1 = Enable the video output and the Display control signal.  
	 */
	*vidcon0 &= ~((3<<26) | (1<<18) | (0xff<<6)  | (1<<2));     /* RGB I/F, RGB Parallel format,  */
	*vidcon0 |= ((4<<6) | (1<<4) );      /* Divided by CLKVAL_F,vclk== HCLK / (CLKVAL+1) = 166.75/5 = 33.35MHz */

	/* 设置极性(要修改)
	 * IVDEN [4]:0 = Normal
	 * IVSYNC[5]:1 = Inverted
	 * IHSYNC[6]:1 = Inverted
	 * IVCLK [7]:0 = Video data is fetched at VCLK falling edge
	 */
	*vidcon1 &= ~(1<<7);   /* 在vclk的下降沿获取数据 */
	*vidcon1 |= ((1<<6) | (1<<5));  /* HSYNC极性反转, VSYNC极性反转 */
	
	/* 设置时序(需要修改) */
	*vidtcon0 = (VBPD << 16) | (VFPD << 8) | (VSPW << 0);
	*vidtcon1 = (HBPD << 16) | (HFPD << 8) | (HSPW << 0);

	/* 设置屏幕的大小
	 * LINEVAL[21:11]:多少行   = 480
	 * HOZVAL [10:0] :水平大小 = 800
	 */
	*vidtcon2 = (LINEVAL << 11) | (HOZVAL << 0);

	/* WSWP_F   [15] :1    = Swap Enable(为什么要使能)，很关键的一位，能够解决掉重影问题
	 * BPPMODE_F[5:2]:1011 = unpacked 24 BPP (non-palletized R:8-G:8-B:8 )
	 * ENWIN_F  [0]:  1    = Enable the video output and the VIDEO control signal.
	 */
	*wincon0 &= ~(0xf << 2);
	//*wincon0 |= (0xB<<2);
	*wincon0 |= (0xB<<2)|(1<<15);
	
	/* 窗口0，左上角的位置(0,0) */
	/* 窗口0，右下角的位置(0,0) */
	*vidosd0a = (LeftTopX<<11) | (LeftTopY << 0);
	*vidosd0b = (RightBotX<<11) | (RightBotY << 0);
	/* 大小 */
	*vidosd0c = (LINEVAL + 1) * (HOZVAL + 1);

	/* 分配显存 */
	tiny_lcd->screen_base = dma_alloc_writecombine(NULL, tiny_lcd->fix.smem_len, &tiny_lcd->fix.smem_start, GFP_KERNEL);
	*vidw00add0b0 = tiny_lcd->fix.smem_start; //显存的物理起始地址
	*vidw00add1b0 = tiny_lcd->fix.smem_start + tiny_lcd->fix.smem_len; //显存的物理结束地址
	//*vidw00add2 = 800*4;
	//*vidw00add1b0 = (((HOZVAL + 1)*4 + 0) * (LINEVAL + 1)) & (0xffffff); //显存的物理结束地址

	/* C0_EN_F  0  Enables Channel 0. 
	 * 0 = Disables 1 = Enables 
	 */
	*shadowcon = 0x1;

	/* LCD控制器开启 */
	*vidcon0 |= 0x3; /* 开启总控制器 */
	*wincon0 |= 1;     /* 开启窗口0 */

	/*4.注册*/
	register_framebuffer(tiny_lcd);
	return 0;
}

static void tiny_lcd_exit(void)
{
	unregister_framebuffer(tiny_lcd);
	dma_free_writecombine(NULL, tiny_lcd->fix.smem_len, &(tiny_lcd->fix.smem_start), GFP_KERNEL);
	iounmap(gpf0con);
	iounmap(gpf1con);
	iounmap(gpf2con);
	iounmap(gpf3con);
	iounmap(gpd0con);
	iounmap(gpd0dat);
	iounmap(display_control);
	iounmap(vidcon0);
	iounmap(vidcon1);
	iounmap(vidtcon2);
	iounmap(wincon0);
	iounmap(vidosd0a);
	iounmap(vidosd0b);
	iounmap(vidosd0c);
	iounmap(vidw00add0b0);
	iounmap(vidw00add1b0);
	iounmap(vidw00add2);
	iounmap(vidtcon0);
	iounmap(vidtcon1);
	iounmap(shadowcon);
	framebuffer_release(tiny_lcd);
}

module_init(tiny_lcd_init);
module_exit(tiny_lcd_exit);

MODULE_LICENSE("GPL");

