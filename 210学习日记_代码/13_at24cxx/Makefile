uart.bin:start.s main.c uart.c clock.c led.c lib.c command.c nand.c mem_setup.S irq.c i2c.c
	arm-linux-gcc -nostdlib -c start.s -o start.o
	arm-linux-gcc -nostdlib -c main.c -o main.o
	arm-linux-gcc -nostdlib -c uart.c -o uart.o
	arm-linux-gcc -nostdlib -c lib.c -o lib.o
	arm-linux-gcc -nostdlib -c clock.c -o clock.o	
	arm-linux-gcc -nostdlib -c led.c -o led.o	
	arm-linux-gcc -nostdlib -c command.c -o command.o	
	arm-linux-gcc -nostdlib -c nand.c -o nand.o	
	arm-linux-gcc -nostdlib -c irq.c -o irq.o	
	arm-linux-gcc -nostdlib -c i2c.c -o i2c.o	
	arm-linux-gcc -nostdlib -c mem_setup.S -o mem_setup.o	
	arm-linux-ld -T bootloader.lds start.o main.o uart.o lib.o clock.o led.o command.o nand.o mem_setup.o irq.o i2c.o -o uart_elf
	arm-linux-objcopy -O binary -S uart_elf uart.bin

clean:
	rm -rf *.o *.bin uart_elf *.dis
