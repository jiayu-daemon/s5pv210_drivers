uart.bin:start.s main.c uart.c clock.c lib.c nand.c lcd.c mem_setup.S
	arm-linux-gcc -nostdlib -c start.s -o start.o
	arm-linux-gcc -nostdlib -c main.c -o main.o
	arm-linux-gcc -nostdlib -c uart.c -o uart.o
	arm-linux-gcc -nostdlib -c lib.c -o lib.o
	arm-linux-gcc -nostdlib -c clock.c -o clock.o	
	arm-linux-gcc -nostdlib -c lcd.c -o lcd.o	
	arm-linux-gcc -nostdlib -c nand.c -o nand.o	
	arm-linux-gcc -nostdlib -c mem_setup.S -o mem_setup.o	
	arm-linux-ld -T bootloader.lds start.o main.o uart.o lib.o clock.o lcd.o nand.o mem_setup.o -o uart_elf
	arm-linux-objcopy -O binary -S uart_elf uart.bin

clean:
	rm -rf *.o *.bin uart_elf *.dis
