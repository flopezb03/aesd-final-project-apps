#ifndef BMP280_H

#define BMP280_H


#include <time.h>


struct bmp280_readout_t{
    double temperature;
    double pressure;
    time_t timestamp;
};



int init_bmp280(int fd);
int bmp280_measurement(int fd, struct bmp280_readout_t* readout);



#endif