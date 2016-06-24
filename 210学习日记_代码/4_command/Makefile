uart.bin:start.s main.c uart.c clock.c led.c lib.c command.c
	arm-linux-gcc -nostdlib -c start.s -o start.o
	arm-linux-gcc -nostdlib -c main.c -o main.o
	arm-linux-gcc -nostdlib -c uart.c -o uart.o
	arm-linux-gcc -nostdlib -c lib.c -o lib.o
	arm-linux-gcc -nostdlib -c clock.c -o clock.o	
	arm-linux-gcc -nostdlib -c led.c -o led.o	
	arm-linux-gcc -nostdlib -c command.c -o command.o	
	arm-linux-ld -Ttext 0xD0020010 start.o main.o uart.o lib.o clock.o led.o command.o -o uart_elf
	arm-linux-objcopy -O binary -S uart_elf uart.bin

clean:
	rm -rf *.o *.bin uart_elf *.dis
