leds_c.bin:start.s main.c
	arm-linux-gcc -nostdlib -c -o start.o start.s
	arm-linux-gcc -nostdlib -c -o main.o main.c
	arm-linux-ld start.o main.o -o led_elf
	arm-linux-objcopy -O binary -S led_elf led_c.bin
	
clean:
	rm -rf led_elf led_c.bin *.o

