void i2c_init(void);
void do_irq(void) ;
void i2c_write(unsigned int slvAddr, unsigned char *buf, int len);
void i2c_read(unsigned int slvAddr, unsigned char *buf, int len);
unsigned char at24cxx_read(unsigned char address);
void at24cxx_write(unsigned char address, unsigned char data);
void wm8960_write(unsigned int slave_addr, int addr, int data);

