#include "command.h"
#include "clock.h"
#include "led.h"
#include "uart.h"
#include "lib.h"
#include "nand.h"

#define	CFG_PROMPT		"WY_BOOT # "	/* Monitor Command Prompt	*/
#define	CFG_CBSIZE		256		/* Console I/O Buffer Size	*/

char *argv[10];

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

