#include "bmp280.h"

#include <unistd.h>
#include <stdint.h>
#include <time.h>

// Types
#define BMP280_S32_t int32_t
#define BMP280_U32_t uint32_t
#define BMP280_S64_t int64_t

// Registers
#define BMP280_CTRL_MEAS    0xF4
#define BMP280_CONFIG       0xF5
#define BMP280_PRESS_MSB     0xF7
#define BMP280_PRESS_LSB     0xF8
#define BMP280_PRESS_XLSB    0xF9
#define BMP280_TEMP_MSB     0xFA
#define BMP280_TEMP_LSB     0xFB
#define BMP280_TEMP_XLSB    0xFC

// Compensation parameter registers
#define BMP280_CALIB00      0x88
#define BMP280_CALIB01      0x89
#define BMP280_CALIB02      0x8A
#define BMP280_CALIB03      0x8B
#define BMP280_CALIB04      0x8C
#define BMP280_CALIB05      0x8D
#define BMP280_CALIB06      0x8E
#define BMP280_CALIB07      0x8F
#define BMP280_CALIB08      0x90
#define BMP280_CALIB09      0x91
#define BMP280_CALIB10      0x92
#define BMP280_CALIB11      0x93
#define BMP280_CALIB12      0x94
#define BMP280_CALIB13      0x95
#define BMP280_CALIB14      0x96
#define BMP280_CALIB15      0x97
#define BMP280_CALIB16      0x98
#define BMP280_CALIB17      0x99
#define BMP280_CALIB18      0x9A
#define BMP280_CALIB19      0x9B
#define BMP280_CALIB20      0x9C
#define BMP280_CALIB21      0x9D
#define BMP280_CALIB22      0x9E
#define BMP280_CALIB23      0x9F
#define BMP280_CALIB24      0xA0
#define BMP280_CALIB25      0xA1


// Compensation parameters
static BMP280_S32_t t_fine;
static uint16_t dig_T1, dig_P1;
static int16_t dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

// Register ctrl_meas
static uint8_t osrs_p = 0b001;      // x1 Pressure oversampling
static uint8_t osrs_t = 0b001;      // x1 Temperature oversampling
static uint8_t mode = 0b01;         // Forced mode
static uint8_t reg_ctrl_meas;


// Register config
static uint8_t t_sb = 0b000;        // Inactive duration (Not used, only normal mode)
static uint8_t filter = 0b000;      // Filter Off
static uint8_t spi3w_en = 0b0;      // Not used 3 pin SPI
static uint8_t reg_config;


static int bmp280_write(int fd, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return write(fd, buf, 2);
}

static int bmp280_read(int fd, uint8_t reg, uint8_t *buf, int len)
{
    write(fd, &reg, 1);
    return read(fd, buf, len);
}

static uint16_t comp_param_read(int fd, uint8_t reg){
    uint8_t buf[2];
    bmp280_read(fd, reg, &buf, 2);
    
    return ((uint16_t)buf[1] << 8) | buf[0];
}

static BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T){
    BMP280_S32_t var1, var2, T;
    
    var1 = ((((adc_T>>3) - ((BMP280_S32_t)dig_T1<<1))) * ((BMP280_S32_t)dig_T2)) >> 11;
    var2 = (((((adc_T>>4) - ((BMP280_S32_t)dig_T1)) * ((adc_T>>4) - ((BMP280_S32_t)dig_T1))) >> 12) * ((BMP280_S32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;

    return T;
}

static BMP280_U32_t bmp280_compensate_P_int64(BMP280_S32_t adc_P){
    BMP280_S64_t var1, var2, p;

    var1 = ((BMP280_S64_t)t_fine) - 128000;
    var2 = var1 * var1 * (BMP280_S64_t)dig_P6;
    var2 = var2 + ((var1*(BMP280_S64_t)dig_P5)<<17);
    var2 = var2 + (((BMP280_S64_t)dig_P4)<<35);
    var1 = ((var1 * var1 * (BMP280_S64_t)dig_P3)>>8) + ((var1 * (BMP280_S64_t)dig_P2)<<12);
    var1 = (((((BMP280_S64_t)1)<<47)+var1))*((BMP280_S64_t)dig_P1)>>33;

    if (var1 == 0)
        return 0;   // avoid exception caused by division by zero

    p = 1048576-adc_P;
    p = (((p<<31)-var2)*3125)/var1;
    var1 = (((BMP280_S64_t)dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((BMP280_S64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((BMP280_S64_t)dig_P7)<<4);

    return (BMP280_U32_t)p;
}


int init_bmp280(int fd){

    // Read compensation parameters
    dig_T1 = comp_param_read(fd, BMP280_CALIB00);
    dig_T2 = (int16_t)comp_param_read(fd, BMP280_CALIB02);
    dig_T3 = (int16_t)comp_param_read(fd, BMP280_CALIB04);

    dig_P1 = comp_param_read(fd, BMP280_CALIB06);
    dig_P2 = (int16_t)comp_param_read(fd, BMP280_CALIB08);
    dig_P3 = (int16_t)comp_param_read(fd, BMP280_CALIB10);
    dig_P4 = (int16_t)comp_param_read(fd, BMP280_CALIB12);
    dig_P5 = (int16_t)comp_param_read(fd, BMP280_CALIB14);
    dig_P6 = (int16_t)comp_param_read(fd, BMP280_CALIB16);
    dig_P7 = (int16_t)comp_param_read(fd, BMP280_CALIB18);
    dig_P8 = (int16_t)comp_param_read(fd, BMP280_CALIB20);
    dig_P9 = (int16_t)comp_param_read(fd, BMP280_CALIB22);


    // Set register config
    reg_config = (t_sb << 5) | (filter << 2) | (spi3w_en);
    bmp280_write(fd, BMP280_CONFIG, reg_config);

    // Prepare register ctrl_meas
    reg_ctrl_meas = (osrs_p << 5) | (osrs_t << 2) | (mode);

}



struct bmp280_readout_t bmp280_measurement(int fd){
    struct bmp280_readout_t readout;
    BMP280_S32_t temperature;
    BMP280_U32_t pressure;

    // Set chip in forced mode, measure and then go to sleep mode again
    bmp280_write(fd, BMP280_CTRL_MEAS, reg_ctrl_meas);

    // Read temperature registers
    uint8_t raw_temp_buf[3];
    bmp280_read(fd, BMP280_TEMP_MSB, &raw_temp_buf, 3);

    // Read pressure registers
    uint8_t raw_press_buf[3];
    bmp280_read(fd, BMP280_PRESS_MSB, &raw_press_buf, 3);

    // Timestamp
    time(&readout.timestamp);

    // Format temperature
    BMP280_S32_t raw_temp = (int32_t)((uint32_t)raw_temp_buf[0] << 12) | ((uint32_t)raw_temp_buf[1] << 4) | ((uint32_t)raw_temp_buf[2] >> 4);
    temperature = bmp280_compensate_T_int32(raw_temp);
    readout.temperature = (double) temperature / 100;

    // Format temperature
    BMP280_S32_t raw_press = (int32_t)((uint32_t)raw_press_buf[0] << 12) | ((uint32_t)raw_press_buf[1] << 4) | ((uint32_t)raw_press_buf[2] >> 4);
    pressure = bmp280_compensate_P_int64(raw_press);
    readout.pressure = (double) pressure / 256;


    return readout;
}