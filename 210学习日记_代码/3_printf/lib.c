#include "uart.h"

#define UTRSTAT0  	(*(volatile unsigned int *)0xE2900010)
#define UTXH0  		(*(volatile unsigned char *)0xE2900020) 
#define URXH0  		(*(volatile unsigned char *)0xE2900024) 

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
	if(c == '\r')
	{
		while(!(UTRSTAT0&(1<<1)));
		UTXH0 = '\n';
	}
	
	if(c == '\n')
	{
		while(!(UTRSTAT0&(1<<1)));
		UTXH0 = '\r';
	}
	while(!(UTRSTAT0&(1<<1)));
	UTXH0 = c;
}

int puts(const char * s)
{
	while (*s)
		putchar(*s++);
		
	return 0;
}

void putint_hex(int a)
{
	putchar_hex( (a>>24) & 0xFF );
	putchar_hex( (a>>16) & 0xFF );
	putchar_hex( (a>>8) & 0xFF );
	putchar_hex( (a>>0) & 0xFF );
}

char * itoa(int a, char * buf)
{
	int num = a;
	int i = 0;
	int len = 0;
	
	do 
	{
		buf[i++] = num % 10 + '0';
		num /= 10;		
	} while (num);
	buf[i] = '\0';
	
	len = i;
	for (i = 0; i < len/2; i++)
	{
		char tmp;
		tmp = buf[i];
		buf[i] = buf[len-i-1];
		buf[len-i-1] = tmp;
	}
	
	return buf;	
}

typedef int * va_list;
#define va_start(ap, A)		(ap = (int *)&(A) + 1)
#define va_arg(ap, T)		(*(T *)ap++)
#define va_end(ap)		((void)0)

int wy_printf(const char * format, ...)
{
	char c;	
	va_list ap;
		
	va_start(ap, format);
	
	while ((c = *format++) != '\0')
	{
		switch (c)
		{
			case '%':
				c = *format++;
				
				switch (c)
				{
					char ch;
					char * p;
					int a;
					char buf[100];
									
					case 'c':
						ch = va_arg(ap, int);
						putchar(ch);
						break;
					case 's':
						p = va_arg(ap, char *);
						puts(p);
						break;					
					case 'x':
						a = va_arg(ap, int);
						putint_hex(a);
						break;		
					case 'd':
						a = va_arg(ap, int);
						itoa(a, buf);
						puts(buf);
						break;	
					
					default:
						break;
				}				
				break;		
		
			default:
				putchar(c);
				break;
		}
	}
	
	return 0;	
}

