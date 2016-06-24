#define GPF0CON		(*(volatile unsigned int *)0xE0200120)
#define GPF1CON		(*(volatile unsigned int *)0xE0200140)
#define GPF2CON		(*(volatile unsigned int *)0xE0200160)
#define GPF3CON		(*(volatile unsigned int *)0xE0200180)

#define GPD0CON		(*(volatile unsigned int *)0xE02000A0)
#define GPD0DAT		(*(volatile unsigned int *)0xE02000A4)

#define CLK_SRC1	(*(volatile unsigned int *)0xe0100204)
#define CLK_DIV1	(*(volatile unsigned int *)0xe0100304)
#define DISPLAY_CONTROL	(*(volatile unsigned int *)0xe0107008)

#define VIDCON0		(*(volatile unsigned int *)0xF8000000)
#define VIDCON1		(*(volatile unsigned int *)0xF8000004)
#define VIDTCON2 	(*(volatile unsigned int *)0xF8000018)
#define VIDTCON3 	(*(volatile unsigned int *)0xF800001c)
#define WINCON0 	(*(volatile unsigned int *)0xF8000020)
#define WINCON2 	(*(volatile unsigned int *)0xF8000028)
#define SHADOWCON 	(*(volatile unsigned int *)0xF8000034)
#define VIDOSD0A 	(*(volatile unsigned int *)0xF8000040)
#define VIDOSD0B 	(*(volatile unsigned int *)0xF8000044)
#define VIDOSD0C 	(*(volatile unsigned int *)0xF8000048)

#define VIDW00ADD0B0 	(*(volatile unsigned int *)0xF80000A0)
#define VIDW00ADD1B0 	(*(volatile unsigned int *)0xF80000D0)
#define VIDW00ADD2   	(*(volatile unsigned int *)0xF8000100)

#define VIDTCON0 	(*(volatile unsigned int *)0xF8000010)
#define VIDTCON1 	(*(volatile unsigned int *)0xF8000014)

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

#define SCR_XSIZE_TFT 	(800)
#define SCR_YSIZE_TFT 	(480)

volatile unsigned long LCD_BUFFER[SCR_YSIZE_TFT][SCR_XSIZE_TFT];

void lcd_init(void)
{
	/* 1. 设置相关GPIO引脚用于LCD */
	GPF0CON = 0x22222222;		// GPF0[7:0]
	GPF1CON = 0x22222222;		// GPF1[7:0]
	GPF2CON = 0x22222222;		// GPF2[7:0]
	GPF3CON = 0x22222222;		// GPF3[7:0]

	/* 使能LCD本身 */
	GPD0CON |= 1<<4;
	GPD0DAT |= 1<<1;

	/* 该寄存器是时钟相关
	 * Display path selection 
	 * 10: RGB=FIMD I80=FIMD ITU=FIMD
	 */
	DISPLAY_CONTROL = 2<<0;

	/* 2. 初始化210的display controller 
	 * 2.1 hsync,vsync,vclk,vden的极性和时间参数
	 * 2.2 行数、列数(分辨率),象素颜色的格式
	 * 2.3 分配显存(frame buffer),写入display controller
	 */
	/* 
	 * CLKVAL_F[13:6]:该值需要根据LCD手册做相应的修改
	 * 			     HCLKD=166.75MHz，DCLK(min) = 20ns(50MHz)
	 *                VCLK = 166.75 / (4+1) = 33.35MHz
	 * CLKDIR  [4]:1 = Divided by CLKVAL_F
	 * ENVID   [1]:1 = Enable the video output and the Display control signal. 
	 * ENVID_F [0]:1 = Enable the video output and the Display control signal.  
	 */
	VIDCON0 &= ~((3<<26) | (1<<18) | (0xff<<6)  | (1<<2));     /* RGB I/F, RGB Parallel format,  */
	VIDCON0 |= ((4<<6) | (1<<4) );

	/* 设置极性(该值需要根据LCD手册做相应的修改)
	 * IVDEN [4]:0 = Normal
	 * IVSYNC[5]:1 = Inverted
	 * IHSYNC[6]:1 = Inverted
	 * IVCLK [7]:0 = Video data is fetched at VCLK falling edge
	 */
	VIDCON1 &= ~(1<<7);   /* 在vclk的下降沿获取数据 */
	VIDCON1 |= ((1<<6) | (1<<5));  /* HSYNC极性反转, VSYNC极性反转 */

	/* 设置时序(需要修改) */
	VIDTCON0 = (VBPD << 16) | (VFPD << 8) | (VSPW << 0);
	VIDTCON1 = (HBPD << 16) | (HFPD << 8) | (HSPW << 0);

	/* 设置屏幕的大小
	 * LINEVAL[21:11]:多少行   = 480
	 * HOZVAL [10:0] :水平大小 = 800
	 */
	VIDTCON2 = (LINEVAL << 11) | (HOZVAL << 0);

	/* WSWP_F   [15] :1    = Swap Enable(为什么要使能)，很关键的一位，能够解决掉重影问题
	 * BPPMODE_F[5:2]:1011 = unpacked 24 BPP (non-palletized R:8-G:8-B:8 )
	 * ENWIN_F  [0]:  1    = Enable the video output and the VIDEO control signal.
	 */
	WINCON0 &= ~(0xf << 2);
	WINCON0 |= (0xB<<2)|(1<<15);

	/* 窗口0，左上角的位置(0,0) */
	/* 窗口0，右下角的位置(800,480) */
	VIDOSD0A = (LeftTopX<<11) | (LeftTopY << 0);
	VIDOSD0B = (RightBotX<<11) | (RightBotY << 0);
	/* 大小 */
	VIDOSD0C = (LINEVAL + 1) * (HOZVAL + 1);

	VIDW00ADD0B0 = LCD_BUFFER;
	/* VBASEL = VBASEU + (LINEWIDTH+OFFSIZE) x (LINEVAL+1) 
	 *        = 0 + (800*4 + 0) * 479
	 *        = 
	 */
	VIDW00ADD1B0 =  (((HOZVAL + 1)*4 + 0) * (LINEVAL + 1)) & (0xffffff);
	//VIDW00ADD1B0 = FRAME_BUFFER + HOZVAL * LINEVAL * 4; /* 新加的，是该这个吗? */

	SHADOWCON = 0x1; /* 使能通道0 */

	/* LCD控制器开启 */
	VIDCON0  |= 0x3; /* 开启总控制器 */
	WINCON0 |= 1;     /* 开启窗口0 */
}

/* 画一个像素点 */
void PutPixel(unsigned long x,unsigned long y, unsigned long c )
{
	if ( (x < SCR_XSIZE_TFT) && (y < SCR_YSIZE_TFT) )
		LCD_BUFFER[(y)][(x)] = c;
}

/* 清屏 */
void lcd_clear_screen( unsigned long c)
{
	unsigned int x,y ;
		
	for( y = 0 ; y < SCR_YSIZE_TFT ; y++ )
	{
		for( x = 0 ; x < SCR_XSIZE_TFT ; x++ )
		{
			LCD_BUFFER[y][x] = c ;
		}
	}
}

/* 用于黄直线:横线和竖线 */
void Glib_Line(int x1,int y1,int x2,int y2,int color)
{
	int dx,dy,e;
	dx=x2-x1; 
	dy=y2-y1;
    
	if(dx>=0)
	{
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 1/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}	
					x1+=1;
					e+=dy;
				}
			}
			else		// 2/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}	
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 8/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}	
					x1+=1;
					e+=dy;
				}
			}
			else		// 7/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}	
					y1-=1;
					e+=dx;
				}
			}
		}	
	}
	else //dx<0
	{
		dx=-dx;		//dx=abs(dx)
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 4/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}	
					x1-=1;
					e+=dy;
				}
			}
			else		// 3/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}	
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 5/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}	
					x1-=1;
					e+=dy;
				}
			}
			else		// 6/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}	
					y1-=1;
					e+=dx;
				}
			}
		}	
	}
}

/* 用于画方框 */
void Glib_Rectangle(int x1,int y1,int x2,int y2,int color)
{
    Glib_Line(x1,y1,x2,y1,color);
    Glib_Line(x2,y1,x2,y2,color);
    Glib_Line(x1,y2,x2,y2,color);
    Glib_Line(x1,y1,x1,y2,color);
}

