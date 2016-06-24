
.global _start
	
_start:
	ldr sp, =0xD0030000  /* 初始化栈，因为后面要调用C函数 */
	bl clock_init              /* 初始化时钟 */
	bl ddr_init                   /* 初始化内存 */
	bl nand_init               /* 初始化NAND */

	ldr r0, =0x36000000   /* 要拷贝到DDR中的位置 */
	ldr r1, =0x0                 /* 从NAND的0地址开始拷贝 */
	ldr r2, =bss_start         /* BSS段的开始地址 */
	sub r2,r2,r0                  /* 要拷贝的大小 */
	bl nand_read              /* 拷贝数据 */

clean_bss:
	ldr r0, =bss_start
	ldr r1, =bss_end
	mov r3, #0
	cmp r0, r1
	ldreq pc, =on_ddr
clean_loop:
	str r3, [r0], #4
	cmp r0, r1	
	bne clean_loop		
	ldr pc, =on_ddr

on_ddr:
	mrs  r0, cpsr
	bic	r0,r0,#0x1f  /* 清M4~M0 */
	orr	r0,r0,#0x12
	msr	cpsr,r0        /* 进入irq */
	ldr sp, =0x3e000000    /* 初始化普通中断模式的栈，指向内存 */

	bl irq_init

	mrs  r0, cpsr
	bic	r0,r0,#0x9f  /* 开总的中断开关,清M4~M0 */
	orr	r0,r0,#0x10
	msr	cpsr,r0      /* 进入user mode */

	ldr sp, =0x3f000000    /* 初始化用户模式的栈，指向内存 */
	ldr pc, =main

halt:
	b halt	

.global key_IRQ

key_IRQ:
	sub lr, lr, #4                   /* 1.计算返回地址 */
	stmdb sp!, {r0-r12, lr}  /* 2.保护现场 */

	/* 3. 处理异常 */
	bl do_irq
	
	/* 4. 恢复现场 */
	ldmia sp!, {r0-r12, pc}^  /* ^表示把spsr恢复到cpsr */

