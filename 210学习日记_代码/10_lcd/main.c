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
		wy_printf("draw rectangle\n");
		Glib_Rectangle(33,33,555,400,0xff0000);
		Glib_Rectangle(55,55,666,422,0x00ff00);
		Glib_Rectangle(77,77,777,444,0xffffff);
		for(i=50;i>0;i--)
			delay();
	}

	return 0;
}

