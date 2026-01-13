#define _POSIX_C_SOURCE 200809L

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
#include <sys/socket.h>
#include <sys/un.h>

#include "bmp280.h"
#include "lcd.h"

#define I2C_DEV "/dev/i2c-1"
#define UNIX_SOCKET_NAME "/tmp/telemetry-client.socket"

static void closeall(int i2c_fd, int unix_server_fd);
static void set_signals();
static void term_handler(int);
static void error_message(char* msg);
static int init_client_unix_socket(int* unix_server_fd);
static void write_to_server(int unix_server_fd, struct bmp280_readout_t* bmp280_readout_p);

static int end = 0;
static int daemon_mode = 0;


int main(int argc, char** argv)
{
    int i2c_fd = -1;
    int unix_server_fd = -1;
    struct bmp280_readout_t bmp280_readout;
    


    if (argc == 2 && strcmp(argv[1], "-d") == 0){
        daemon_mode = 1;
        openlog(NULL,0,LOG_USER);
        daemon(0,0);
    }
    set_signals();

    // Init Unix Socket as Client 
    if(!init_client_unix_socket(&unix_server_fd)){
        closeall(i2c_fd, unix_server_fd);
        exit(EXIT_FAILURE);
    }
    
    // I2C Devices initialization
    i2c_fd = open(I2C_DEV, O_RDWR);
    if (i2c_fd < 0) {
        error_message("Open i2c-1");
        closeall(i2c_fd, unix_server_fd);
        exit(EXIT_FAILURE);
    }


    if (ioctl(i2c_fd, I2C_SLAVE, BMP280_ADDR1) < 0) {
        error_message("Ioctl I2C_SLAVE (bmp280) command");
        closeall(i2c_fd, unix_server_fd);
        exit(EXIT_FAILURE);
    }
    if(!init_bmp280(i2c_fd)){
        error_message("Init BMP280");
        closeall(i2c_fd, unix_server_fd);
        exit(EXIT_FAILURE);
    }


    if(ioctl(i2c_fd, I2C_SLAVE, LCD_ADDR) < 0) {
        error_message("Ioctl I2C_SLAVE(lcd) command");
        closeall(i2c_fd, unix_server_fd);
        exit(EXIT_FAILURE);
    }
    init_lcd(i2c_fd);
    


    // First measures not used because of wrong values
    if (ioctl(i2c_fd, I2C_SLAVE, BMP280_ADDR1) < 0) {
        error_message("Ioctl I2C_SLAVE (bmp280) command");
        closeall(i2c_fd, unix_server_fd);
        exit(EXIT_FAILURE);
    }
    bmp280_measurement(i2c_fd, &bmp280_readout);
    sleep(2);



    // Loop
    while(!end){
        //Try reconnect server
        if(unix_server_fd == -1){
            if(!init_client_unix_socket(&unix_server_fd)){
                closeall(i2c_fd, unix_server_fd);
                exit(EXIT_FAILURE);
            }
        }

        // Take measures
        if (ioctl(i2c_fd, I2C_SLAVE, BMP280_ADDR1) < 0) {
            error_message("Ioctl I2C_SLAVE (bmp280) command");
            closeall(i2c_fd, unix_server_fd);
            exit(EXIT_FAILURE);
        }
        if(!bmp280_measurement(i2c_fd, &bmp280_readout))
            continue;
        
        // Write to lcd
        if(ioctl(i2c_fd, I2C_SLAVE, LCD_ADDR) < 0) {
            error_message("Ioctl I2C_SLAVE(lcd) command");
            closeall(i2c_fd, unix_server_fd);
            exit(EXIT_FAILURE);
        }
        char s1[16], s2[16];
        snprintf(s1, sizeof(s1), "T: %.2lf %cC", bmp280_readout.temperature, 0b11011111);
        snprintf(s2, sizeof(s2), "P: %.2lf hPa", bmp280_readout.pressure / 100);
        write_lcd(i2c_fd, s1, s2);

        // Write to socket
        if(unix_server_fd != -1)
            write_to_server(unix_server_fd, &bmp280_readout);


        sleep(60);
    }
    if(ioctl(i2c_fd, I2C_SLAVE, LCD_ADDR) < 0) {
        error_message("Ioctl I2C_SLAVE(lcd) command");
        closeall(i2c_fd, unix_server_fd);
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

    closeall(i2c_fd, unix_server_fd);
    exit(EXIT_SUCCESS);
}



static void closeall(int i2c_fd, int unix_server_fd){
    if(i2c_fd != -1)
        close(i2c_fd);
    if(unix_server_fd != -1)
        close(unix_server_fd);
    
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

static int init_client_unix_socket(int* unix_server_fd){
    struct sockaddr_un unix_server_addr;
    
    if((*unix_server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        error_message("Unix Socket Creation");
        return 0;
    }
    unix_server_addr.sun_family = AF_UNIX;
    strncpy(unix_server_addr.sun_path, UNIX_SOCKET_NAME, sizeof(unix_server_addr.sun_path) - 1);

    
    if(connect(*unix_server_fd, (const struct sockaddr*)&unix_server_addr, sizeof(unix_server_addr)) == -1)
        error_message("Connection to the Unix Socket");
    
    return 1;
}

static void write_to_server(int unix_server_fd, struct bmp280_readout_t* bmp280_readout_p){
    if(send(unix_server_fd, bmp280_readout_p, sizeof(struct bmp280_readout_t), MSG_NOSIGNAL) <= 0)
        error_message("Writing to server");
}