#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include <signal.h>

#define UNIX_SOCKET_NAME "/tmp/telemetry-client.socket"

struct measures_data_t{
    double temperature;
    double pressure;
    time_t timestamp;
};

static void closeall();
static void set_signals();
static void term_handler(int);
static void error_message(char* msg);
static void init_server_unix_socket();
static void accept_unix_client();
static int read_from_client(struct measures_data_t* md);


static daemon_mode = 0;
static end = 0;
static int unix_server_fd = -1;
static int unix_client_fd = -1;



int main(int argc, char** argv){
    struct measures_data_t measures_data;

    if (argc == 2 && strcmp(argv[1], "-d") == 0){
        daemon_mode = 1;
        openlog(NULL,0,LOG_USER);
        daemon(0,0);
    }
    set_signals();

    //  Init Unix Socket as Server
    init_server_unix_socket();

    while(!end){

        //  Read from client
        if(!read_from_client(&measures_data)){
            if(end)
                continue;
            error_message("Client disconnected");
            close(unix_client_fd);
            accept_unix_client();
            continue;
        }

        //  Write to server
        fprintf(stderr, "Temp: %.2lf | Pres: %.2lf | Time: %d\n", measures_data.temperature, measures_data.pressure, measures_data.timestamp);

        
    }

    closeall();
    exit(EXIT_SUCCESS);
}

static void closeall(){
    if(unix_client_fd != -1)
        close(unix_client_fd);
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




static void init_server_unix_socket(){
    struct sockaddr_un unix_server_addr;

    if((unix_server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        error_message("Unix Socket Creation");
        closeall();
        exit(EXIT_FAILURE);
    }

    unix_server_addr.sun_family = AF_UNIX;
    strncpy(unix_server_addr.sun_path, UNIX_SOCKET_NAME, sizeof(unix_server_addr.sun_path) - 1);

    if(bind(unix_server_fd, (const struct sockaddr*) &unix_server_addr, sizeof(unix_server_addr)) == -1){
        error_message("Bind Unix Server");
        closeall();
        exit(EXIT_FAILURE);
    }

    if(listen(unix_server_fd, 1) == -1){
        error_message("Listen Unix Server");
        closeall();
        exit(EXIT_FAILURE);
    }
    
    accept_unix_client();
}
static void accept_unix_client(){
    if((unix_client_fd = accept(unix_server_fd, NULL, NULL)) == -1){
        error_message("Accept Unix Server");
        closeall();
        exit(EXIT_FAILURE);
    }
}

static int read_from_client(struct measures_data_t* md){
    return recv(unix_client_fd, md, sizeof(struct measures_data_t), 0) > 0;
}

