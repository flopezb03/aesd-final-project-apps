#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define TCP_SOCKET_IP "192.168.1.43"
#define TCP_SOCKET_PORT 3333

struct measures_data_t{
    double temperature;
    double pressure;
    time_t timestamp;
};

static void closeall(int tcp_server_fd, int tcp_client_fd);
static void set_signals();
static void term_handler(int);
static void error_message(char* msg);

static int init_server_tcp_socket(int* tcp_server_fd, int* tcp_client_fd);
static int accept_tcp_client(int tcp_server_fd, int* tcp_client_fd);
static int read_from_client(int tcp_client_fd, struct measures_data_t* md);


static int daemon_mode = 0;
static int end = 0;




int main(int argc, char** argv){
    int tcp_server_fd = -1;
    int tcp_client_fd = -1;
    struct measures_data_t measures_data;

    if (argc == 2 && strcmp(argv[1], "-d") == 0){
        daemon_mode = 1;
        openlog(NULL,0,LOG_USER);
        daemon(0,0);
    }
    set_signals();

    //  Init TCP Socket as Server
    if(!init_server_tcp_socket(&tcp_server_fd, &tcp_client_fd)){
        closeall(tcp_server_fd, tcp_client_fd);
        exit(EXIT_FAILURE);
    }

    while(!end){

        //  Read from client
        if(!read_from_client(tcp_client_fd, &measures_data)){
            if(end)
                continue;

            error_message("Client disconnected");
            close(tcp_client_fd);

            if(!accept_tcp_client(tcp_server_fd, &tcp_client_fd)){
                closeall(tcp_server_fd, tcp_client_fd);
                exit(EXIT_FAILURE);
            }
            continue;
        }

        //  Process data
        fprintf(stderr, "Temp: %.2lf | Pres: %.2lf | Time: %ld\n", measures_data.temperature, measures_data.pressure, measures_data.timestamp);
        syslog(LOG_USER, "Temp: %.2lf | Pres: %.2lf | Time: %ld\n", measures_data.temperature, measures_data.pressure, measures_data.timestamp);
        
    }

    closeall(tcp_server_fd, tcp_client_fd);
    exit(EXIT_SUCCESS);
}

static void closeall(int tcp_server_fd, int tcp_client_fd){
    if(tcp_client_fd != -1)
        close(tcp_client_fd);
    if(tcp_server_fd != -1)
        close(tcp_server_fd);

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




static int init_server_tcp_socket(int* tcp_server_fd, int* tcp_client_fd){
    struct sockaddr_in tcp_server_addr;

    memset(&tcp_server_addr, 0, sizeof(tcp_server_addr));
    tcp_server_addr.sin_family = AF_INET; 
    inet_pton(AF_INET, TCP_SOCKET_IP, &(tcp_server_addr.sin_addr));
    tcp_server_addr.sin_port = htons(TCP_SOCKET_PORT);

    if((*tcp_server_fd = socket(tcp_server_addr.sin_family, SOCK_STREAM, 0)) == -1){
        error_message("TCP Socket Creation");
        return 0;
    }

    if(bind(*tcp_server_fd, (const struct sockaddr*) &tcp_server_addr, sizeof(tcp_server_addr)) == -1){
        error_message("Bind TCP Server");
        return 0;
    }

    if(listen(*tcp_server_fd, 1) == -1){
        error_message("Listen TCP Server");
        return 0;
    }
    
    if(!accept_tcp_client(*tcp_server_fd, tcp_client_fd))
        return 0;

    return 1;
}
static int accept_tcp_client(int tcp_server_fd, int* tcp_client_fd){
    if((*tcp_client_fd = accept(tcp_server_fd, NULL, NULL)) == -1){
        error_message("Accept TCP Server");
        return 0;
    }

    return 1;
}

static int read_from_client(int tcp_client_fd, struct measures_data_t* md){
    return recv(tcp_client_fd, md, sizeof(struct measures_data_t), 0) > 0;
}

