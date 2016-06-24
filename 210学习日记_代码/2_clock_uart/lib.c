void delay(void)
{
	volatile int i = 0x100000;
	while (i--);
}

void putchar_hex(char c)
{
	char * hex = "0123456789ABCDEF";
	
	uart_putchar(hex[(c>>4) & 0x0F]);
	uart_putchar(hex[(c>>0) & 0x0F]);

	return;
}

int putchar(int c)
{
	if (c == '\n') /* 如果程序里面为\n */
		uart_putchar('\r'); /* 则在终端里面回车，换行 */
		
	uart_putchar(c);

	return 0;
}

