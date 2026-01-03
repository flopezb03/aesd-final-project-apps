#ifndef BMP280_H

#define BMP280_H

#include <stdint.h>
#include <time.h>

// I2C Address
#define BMP280_ADDR1 0x76
#define BMP280_ADDR2 0x77


struct bmp280_readout_t{
    double temperature;
    double pressure;
    time_t timestamp;
};



int init_bmp280(int fd);
struct bmp280_readout_t bmp280_measurement(int fd);



#endif