#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "bmp280.h"
#include "lcd.h"

#define I2C_DEV "/dev/i2c-1"

static void closeall();
static void set_signals();
static void term_handler(int);
static void error_message(char* msg);

static int end = 0;
static int i2c_fd = -1;
struct bmp280_readout_t *bmp280_readout;
static int daemon_mode = 0;

int main(int argc, char** argv)
{
    if (argc == 2 && strcmp(argv[1], "-d") == 0){
        daemon_mode = 1;
        openlog(NULL,0,LOG_USER);
        daemon(0,0);
    }
    set_signals();
    
    if((bmp280_readout = malloc(sizeof(bmp280_readout))) == NULL){
        error_message("Malloc");
        closeall();
        exit(EXIT_FAILURE);
    }

    // Open socket 
    
    // I2C Devices initialization
    i2c_fd = open(I2C_DEV, O_RDWR);
    if (i2c_fd < 0) {
        error_message("Open i2c-1");
        closeall();
        exit(EXIT_FAILURE);
    }


    if (ioctl(i2c_fd, I2C_SLAVE, BMP280_ADDR1) < 0) {
        error_message("Ioctl I2C_SLAVE (bmp280) command");
        closeall();
        exit(EXIT_FAILURE);
    }
    if(!init_bmp280(i2c_fd)){
        error_message("Init BMP280");
        closeall();
        exit(EXIT_FAILURE);
    }


    if(ioctl(i2c_fd, I2C_SLAVE, LCD_ADDR) < 0) {
        error_message("Ioctl I2C_SLAVE(lcd) command");
        closeall();
        exit(EXIT_FAILURE);
    }
    init_lcd(i2c_fd);
    


    // First measures not used because of wrong values
    if (ioctl(i2c_fd, I2C_SLAVE, BMP280_ADDR1) < 0) {
        error_message("Ioctl I2C_SLAVE (bmp280) command");
        closeall();
        exit(EXIT_FAILURE);
    }
    bmp280_measurement(i2c_fd, bmp280_readout);
    sleep(2);



    // Loop
    while(!end){
        // Take measures
        if (ioctl(i2c_fd, I2C_SLAVE, BMP280_ADDR1) < 0) {
            error_message("Ioctl I2C_SLAVE (bmp280) command");
            closeall();
            exit(EXIT_FAILURE);
        }
        if(!bmp280_measurement(i2c_fd, bmp280_readout))
            continue;
        
        // Write to lcd
        if(ioctl(i2c_fd, I2C_SLAVE, LCD_ADDR) < 0) {
            error_message("Ioctl I2C_SLAVE(lcd) command");
            closeall();
            exit(EXIT_FAILURE);
        }
        char s1[16], s2[16];
        snprintf(s1, sizeof(s1), "T: %.2lf %cC", bmp280_readout->temperature, 0b11011111);
        snprintf(s2, sizeof(s2), "P: %.2lf hPa", bmp280_readout->pressure / 100);
        write_lcd(i2c_fd, s1, s2);

        // Write to socket

        sleep(60);
    }
    if(ioctl(i2c_fd, I2C_SLAVE, LCD_ADDR) < 0) {
        error_message("Ioctl I2C_SLAVE(lcd) command");
        closeall();
        exit(EXIT_FAILURE);
    }
    write_lcd(i2c_fd, "Closing app", "");
    sleep(2);
    write_lcd(i2c_fd, "", "");
    sleep(1);
    write_lcd(i2c_fd, "Closing app", "");
    sleep(2);
    write_lcd(i2c_fd, "", "");
    sleep(1);
    write_lcd(i2c_fd, "Closing app", "");
    sleep(2);
    write_lcd(i2c_fd, "", "");

    closeall();
    exit(EXIT_SUCCESS);
}



static void closeall(){
    if(bmp280_readout != NULL)
        free(bmp280_readout);
    if(i2c_fd != -1)
        close(i2c_fd);
    
    closelog();
}

static void set_signals(){
    struct sigaction sa;
    sa.sa_handler = term_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
static void term_handler(int){
    end = 1;
}

static void error_message(char* msg){
    if(daemon_mode)
        syslog(LOG_ERR, "Error: %s", msg);
    else
        fprintf(stderr, "Error: %s\n", msg);
}