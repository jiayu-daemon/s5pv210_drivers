#include "clock.h"
#include "led.h"
#include "uart.h"
#include "lib.h"

int main(void)
{
	led_init(); /* 设置对应管脚为输出 */
	clock_init(); /* 初始化时钟 */
	uart_init(); /* 初始化UART0 */

	wy_printf("\n********************************\n");
	wy_printf("                   wy_bootloader\n");
	wy_printf("                   vars: %d \n",2012);
	wy_printf("********************************\n");
	while (1)
	{
		char c = 'x';
		putchar('\n');
		
		c = uart_getchar();
		putchar(c);
		
		for (c = 'a'; c <= 'z'; c++)
			putchar(c);
		break;
	}

	while(1)
	{
		led_water();
	}
	return 0;
}

