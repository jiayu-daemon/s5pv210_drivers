#include "clock.h"
#include "uart.h"
#include "lib.h"
#include "nand.h"
#include "lcd.h"

int main(void)
{
	int i;
	
	uart_init(); /* ³õÊ¼»¯UART0 */
	wy_printf("LCD initialize ...\n");
	lcd_init();
	
	while (1)
	{
		wy_printf("display red\n");
		lcd_clear_screen(0xff0000);
		for(i=50;i>0;i--)
			delay();
		wy_printf("display green\n");
		lcd_clear_screen(0x00ff00);
		for(i=50;i>0;i--)
			delay();
		wy_printf("display blue\n");
		lcd_clear_screen(0x0000ff);
		for(i=50;i>0;i--)
			delay();
		nand_read((unsigned char *)0x3fc00000, 0xC00000, 0x300000);
		wy_printf("display girl\n");
		lcd_draw_bmp(0x3fc00000);
		for(i=150;i>0;i--)
			delay();
	}

	return 0;
}

