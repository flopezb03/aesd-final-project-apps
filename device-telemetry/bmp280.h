#ifndef BMP280_H

#define BMP280_H

#include <stdint.h>
#include <time.h>

// I2C Address
#define BMP280_ADDR1 0x76
#define BMP280_ADDR2 0x77

// Types
#define BMP280_S32_t int32_t
#define BMP280_U32_t uint32_t
#define BMP280_S64_t int64_t

struct bmp280_readout_t{
    BMP280_S32_t temperature;
    BMP280_U32_t pressure;
    time_t timestamp;
};



int init_bmp280(int fd);
struct bmp280_readout_t bmp280_measurement(int fd);



#endif