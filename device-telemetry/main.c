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
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#include "bmp280.h"
#include "lcd.h"
#include "led.h"

#define I2C_DEV "/dev/i2c-1"
#define UNIX_SOCKET_NAME "/tmp/telemetry-client.socket"

struct thread_args{
    int* unix_server_fd_p;
};

static void closeall(int unix_server_fd, pthread_t t_listener);
static void set_signals();
static void term_handler(int);
static void error_message(char* msg);

static int init_client_unix_socket(int* unix_server_fd);
static int write_to_server(int unix_server_fd, struct bmp280_readout_t* bmp280_readout_p);

static void* thread_listener(void* args);

static int end = 0;
static int daemon_mode = 0;


int main(int argc, char** argv)
{
    int unix_server_fd = -1;
    struct bmp280_readout_t bmp280_readout;
    pthread_t t_listener = NULL;
    struct thread_args t_args;
    t_args.unix_server_fd_p = &unix_server_fd;

    


    if (argc == 2 && strcmp(argv[1], "-d") == 0){
        daemon_mode = 1;
        openlog(NULL,0,LOG_USER);
        daemon(0,0);
    }
    set_signals();

    // Init Unix Socket as Client 
    if(!init_client_unix_socket(&unix_server_fd)){
        closeall(unix_server_fd, t_listener);
        exit(EXIT_FAILURE);
    }else{
        //Create Listener Thread
        if(unix_server_fd != -1)
            pthread_create(&t_listener, NULL, thread_listener, &t_args);
    }
    
    // Devices initialization

    if(init_bmp280() == -1){
        error_message("Init BMP280");
        closeall(unix_server_fd, t_listener);
        exit(EXIT_FAILURE);
    }


    if(init_lcd() == -1){
        error_message("Init LCD");
        closeall(unix_server_fd, t_listener);
        exit(EXIT_FAILURE);
    }
    
    if(bmp280_measurement(&bmp280_readout) == -1){
        error_message("BMP280 Measurement");
        closeall(unix_server_fd, t_listener);
        exit(EXIT_FAILURE);
    }
    sleep(2);



    // Loop
    while(!end){
        //Try reconnect server
        if(unix_server_fd == -1){
            if(!init_client_unix_socket(&unix_server_fd)){
                closeall(unix_server_fd, t_listener);
                exit(EXIT_FAILURE);
            }else{
                //Create Listener Thread
                if(unix_server_fd != -1)
                    pthread_create(&t_listener, NULL, thread_listener, &t_args);
            }
        }

        // Take measures
        if(bmp280_measurement(&bmp280_readout) == -1){
            error_message("BMP280 Measurement");
            closeall(unix_server_fd, t_listener);
            exit(EXIT_FAILURE);
        }
        
        // Write to lcd
        char s1[16], s2[16];
        snprintf(s1, sizeof(s1), "T: %.2lf %cC", bmp280_readout.temperature, 0b11011111);
        snprintf(s2, sizeof(s2), "P: %.2lf hPa", bmp280_readout.pressure / 100);
        if(write_lcd(s1, s2) == -1){
            error_message("Write LCD");
            closeall(unix_server_fd, t_listener);
            exit(EXIT_FAILURE);
        }

        // Write to socket
        if(unix_server_fd != -1){
            if(!write_to_server(unix_server_fd, &bmp280_readout)){
                close(unix_server_fd);
                unix_server_fd = -1;
                pthread_join(t_listener, NULL);
                t_listener = NULL;
            }
        }


        sleep(5);
    }

    write_lcd("Closing app", "");
    usleep(750000);
    write_lcd("", "");
    usleep(250000);
    write_lcd("Closing app", "");
    usleep(750000);
    write_lcd("", "");
    usleep(250000);
    write_lcd("Closing app", "");
    usleep(750000);
    write_lcd("", "");

    closeall(unix_server_fd, t_listener);
    exit(EXIT_SUCCESS);
}



static void closeall(int unix_server_fd, pthread_t t_listener){
    if(unix_server_fd != -1){
        shutdown(unix_server_fd, SHUT_RD);
        pthread_join(t_listener, NULL);
    }
    
    close_bmp280();
    close_lcd();

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

    
    if(connect(*unix_server_fd, (const struct sockaddr*)&unix_server_addr, sizeof(unix_server_addr)) == -1){
        error_message("Connection to the Unix Socket");
        close(*unix_server_fd);
        *unix_server_fd = -1;
    }
    
    return 1;
}

static int write_to_server(int unix_server_fd, struct bmp280_readout_t* bmp280_readout_p){
    if(send(unix_server_fd, bmp280_readout_p, sizeof(struct bmp280_readout_t), MSG_NOSIGNAL) <= 0){
        error_message("Writing to server");
        return 0;
    }
    return 1;
}

static void* thread_listener(void* args){
    struct thread_args* t_args = args;
    int countermeasure;
    int rgbled_fd;

    rgbled_fd = open("/dev/rgbled", O_RDWR);
    if (rgbled_fd < 0) {
        error_message("Open rgbled");
        return;
    }

    while(!end){
        if(recv(*(t_args->unix_server_fd_p), &countermeasure, sizeof(countermeasure), 0) <= 0){
            error_message("Listening Unix Server");
            break;
        }

        //  Countermeasure action
        set_rgb_color(rgbled_fd, countermeasure);
    }

    set_rgb_color(rgbled_fd, 0);
    close(rgbled_fd);
}