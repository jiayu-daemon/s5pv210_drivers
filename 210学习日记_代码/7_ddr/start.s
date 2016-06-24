
.global _start
	
_start:
	ldr sp, =0xD0030000	@初始化堆栈
	bl clock_init       @初始化时钟
	bl ddr_init         @初始化堆栈
	
	b main

