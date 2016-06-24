#define GPJ2CON		(*(volatile unsigned int *)0xe0200280)
#define GPJ2DAT		(*(volatile unsigned int *)0xe0200284)

#define GPH2CON		(*(volatile unsigned int *)0xE0200C40)
#define GPH2DAT		(*(volatile unsigned int *)0xE0200C44)

void delay()
{
	volatile int i = 0x100000;
	while (i--);
}

int main(void)
{
	int i = 0;

	/* ÅäÖÃGPJ2_0ÎªÊä³öÒý½Å */
	GPJ2CON = 0x1111;

	while (1)
	{		
		GPJ2DAT = i;
		i++;
		if (i == 16)
			i = 0;
		delay();
	}

	return 0;
}

