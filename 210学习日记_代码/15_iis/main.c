#include "command.h"
#include "clock.h"
#include "led.h"
#include "uart.h"
#include "lib.h"
#include "nand.h"
#include "i2c.h"
#include "iis.h"

#define	CFG_PROMPT		"WY_BOOT # "	/* Monitor Command Prompt	*/
#define	CFG_CBSIZE		256		/* Console I/O Buffer Size	*/
#define   WM8960_DEVICE_ADDR		0x34

int offset = 0x2E;				/* wav文件头部的大小 */
short *p = (short *)0x23000000;

char *argv[10];

void WM8960_init(void)
{
	/* 复位，让其他所有的寄存器恢复到默认值 */ 
	wm8960_write(WM8960_DEVICE_ADDR, 0xf, 0x0);

	/* 打开电源，使用fast start-up模式 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x19, 1<<8 | 1<<7 | 1<<6);
	/* 任然是打开电源 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x1a, 1<<8 | 1<<7 | 1<<6 | 1<<5 | 1<<4 | 1<<3);
	/* 左右声道输出使能 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x2F, 1<<3 | 1<<2);
	
	/* 设置时钟，使用的都是默认值 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x4, 0x0);

	/* 关键是将R5寄存器的bit[3]清零，关闭静音功能 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x5, 0x0);
	
	/* 设置通信协议方式:如数据是24位，即IIS，左右声道时钟电平是否反转 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x7, 0x2);
		
	/* 设置左右声道输出的音量 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x2, 0xFF | 0x100);/* 控制左声道的 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x3, 0xFF | 0x100);/* 控制右声道的 */
	wm8960_write(WM8960_DEVICE_ADDR, 0xa, 0xFF | 0x100);/* 控制左声道的 */
	wm8960_write(WM8960_DEVICE_ADDR, 0xb, 0xFF | 0x100);/* 控制右声道的 */
	
	/* 使能通道，否则会静音 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x22, 1<<8 | 1<<7);/* 控制左声道的 */
	wm8960_write(WM8960_DEVICE_ADDR, 0x25, 1<<8 | 1<<7);	/* 控制右声道的 */
	
	return;
}

int readline (const char *const prompt)
{
	char console_buffer[CFG_CBSIZE];		/* console I/O buffer	*/
	char *buf = console_buffer;
	int argc = 0;
	int state = 0;

	//puts(prompt);
	wy_printf("%s",prompt);
	gets(console_buffer);
	
	while (*buf)
	{
		if (*buf != ' ' && state == 0)
		{
			argv[argc++] = buf;
			state = 1;
		}
		
		if (*buf == ' ' && state == 1)
		{
			*buf = '\0';
			state = 0;
		}
		
		buf++;	
	}
	
	return argc;
}

void message(void)
{
	wy_printf("\nThis bootloader support some command to test peripheral:\n");
	wy_printf("Such as: LCD, IIS, BUZZER \n");
	wy_printf("Try 'help' to learn them \n\n");	
}

int main(void)
{
	char buf[6];
	int argc = 0;
	int i = 0;

	led_init(); /* 设置对应管脚为输出 */
	uart_init(); /* 初始化UART0 */
	nand_read_id(buf);

	i2c_init(); /* 初始化IIC */
	WM8960_init();
	IIS_init();
	
	wy_printf("\n**********************************************************\n");
	wy_printf("                     wy_bootloader\n");
	wy_printf("                     vars: %d \n",2012);
	wy_printf("                     nand id:");
	putchar_hex(buf[0]);
	putchar_hex(buf[1]);
	putchar_hex(buf[2]);
	putchar_hex(buf[3]);
	putchar_hex(buf[4]);
	wy_printf("\n**********************************************************\n");
	while (1)
	{
		argc = readline (CFG_PROMPT);
		if(argc == 0 && i ==0)
		{
			message();
			i=1;
		}
		run_command(argc, argv);
	}	
	
	return 0;
}

