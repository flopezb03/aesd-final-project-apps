#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "bmp280.h"

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
        error_message("Ioctl I2C_SLAVE command");
        closeall();
        exit(EXIT_FAILURE);
    }

    if(!init_bmp280(i2c_fd)){
        error_message("Init BMP280");
        closeall();
        exit(EXIT_FAILURE);
    }

    // Loop
    while(!end){
        // Take measures
        if(!bmp280_measurement(i2c_fd, bmp280_readout))
            continue;

        // Write to lcd

        // Write to socket

        sleep(60);
    }

    closeall();
    exit(EXIT_SUCCESS);
}



static void closeall(){
    if(bmp280_readout == NULL)
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