#ifndef BMP280_H

#define BMP280_H


#include <time.h>


struct bmp280_readout_t{
    double temperature;
    double pressure;
    time_t timestamp;
};



int init_bmp280();
int bmp280_measurement(struct bmp280_readout_t* readout);
void close_bmp280();



#endif