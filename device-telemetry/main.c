#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

int main(){
    int file;

    fprintf(stdout, "INIT");

    file = open("/dev/i2c-1", O_RDWR);
    if(file < 0){
        fprintf(stderr, "Error: Openning /dev/i2c-1");
        exit(1);
    }

    fprintf(stdout, "OPEN");

    int addr = 0x027;
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        fprintf(stderr, "Error: ioctl I2C_SLAVE");
        exit(1);
    }

    fprintf(stdout, "IOCTL");

    __u8 reg = 0x10; /* Device register to access */
    __s32 res;
    char buf[10];

    buf[0] = reg;
    buf[1] = 0x43;
    buf[2] = 0x65;
    if (write(file, buf, 3) != 3) {
        fprintf(stderr, "Error: write to i2c");
    }

    fprintf(stdout, "DONE");

}