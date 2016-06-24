#include "lib.h"

int help(int argc, char * argv[])
{
	wy_printf("do_command <%s> \n", argv[0]);
	wy_printf("help message: \n");
	wy_printf("md - memory dispaly\n");
	wy_printf("mw - memory write\n");
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

	if(argc >= 1)
		wy_printf("Unknown command '%s' - try 'help' \n",argv[0]);
	return;
}

