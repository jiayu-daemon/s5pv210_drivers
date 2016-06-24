#include "lib.h"
#include "nand.h"
#include "i2c.h"

int help(int argc, char * argv[])
{
	wy_printf("do_command <%s> \n", argv[0]);
	wy_printf("help message: \n");
	wy_printf("md - memory dispaly\n");
	wy_printf("mw - memory write\n");
	wy_printf("nand read - nand read sdram_addr nand_addr size\n");
	wy_printf("nand write - nand write sdram_addr nand_addr size\n");
	return 0;
}

int md(int argc, char * argv[])
{	
	unsigned long *p = (unsigned long *)0;
	int i, j;

	wy_printf("do_command <%s> \n", argv[0]);

	if (argc <= 1) {
		wy_printf ("Usage:\n%s\n", "md address");
		return 1;
	}
	
	if (argc >= 2)
		p = (unsigned long *)atoi(argv[1]);
		
	for (j = 0; j < 16; j++)
	{	
		wy_printf("%x: ", p);
		for (i = 0; i < 4; i++)
			wy_printf("%x ", *p++);	
		wy_printf("\n");
	}
		
	return 0;
}

int mw(int argc, char * argv[])
{	
	unsigned long *p = (unsigned long *)0;
	int v = 0;

	wy_printf("do_command <%s> \n", argv[0]);

	if (argc <= 2) {
		wy_printf ("Usage:\n%s\n", "md address data");
		return 1;
	}
	
	if (argc >= 2)
		p = (unsigned long *)atoi(argv[1]);
		
	if (argc >= 3)
		v = atoi(argv[2]);
		
	*p = v;
	
	return 0;
}

int nand(int argc, char *argv[])
{
	int nand_addr, sdram_addr;
	unsigned int size;
	
	if (argc < 5)
	{
		wy_printf("nand read sdram_addr nand_addr size\n");
		wy_printf("nand write sdram_addr nand_addr size\n");
		return 0;
	}

	sdram_addr = atoi(argv[2]);
	nand_addr = atoi(argv[3]);
	size = atoi(argv[4]);

	wy_printf("do_command <%s> \n", argv[0]);
	wy_printf("sdram 0x%x, nand 0x%x, size 0x%x\n", sdram_addr, nand_addr, size);

	if (strcmp(argv[1], "read") == 0)
		nand_read((unsigned char *)sdram_addr, nand_addr, size);

	if (strcmp(argv[1], "write") == 0)
		nand_write(sdram_addr, nand_addr, size);	

	wy_printf("nand %s finished!\n", argv[1]);
	return 0;
}

int i2c(int argc, char *argv[])
{
	int addr, data;
	int temp;

	addr = atoi(argv[2]);
	data = atoi(argv[3]);

	wy_printf("do_command <%s> \n", argv[0]);
	wy_printf("addr 0x%x, data 0x%x\n", addr, data);

	if (strcmp(argv[1], "read") == 0)
	{
		temp = at24cxx_read(addr);
		wy_printf("addr 0x%x`s data 0x%x\n", addr, temp);
	}

	if (strcmp(argv[1], "write") == 0)
		at24cxx_write(addr, data);

	wy_printf("i2c %s finished!\n", argv[1]);
	return 0;
}

void run_command(int argc, char * argv[])
{
	if (strcmp(argv[0], "help") == 0)
	{
		help(argc, argv);
		return;
	}
	
	if (strcmp(argv[0], "md") == 0)
	{
		md(argc, argv);
		return;
	}
	
	if (strcmp(argv[0], "mw") == 0)
	{
		mw(argc, argv);
		return;
	}

	if (strcmp(argv[0], "i2c") == 0)
	{
		i2c(argc, argv);
		return;
	}

	if (strcmp(argv[0], "nand") == 0)
		nand(argc, argv);

	if(argc >= 1)
		wy_printf("Unknown command '%s' - try 'help' \n",argv[0]);
	return;
}

