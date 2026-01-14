#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>

#define UNIX_SOCKET_NAME "/tmp/telemetry-client.socket"
#define TCP_SOCKET_IP "192.168.1.43"
#define TCP_SOCKET_PORT 3333

struct measures_data_t{
    double temperature;
    double pressure;
    time_t timestamp;
};

static void closeall(int unix_server_fd, int unix_client_fd, int tcp_server_fd);
static void set_signals();
static void term_handler(int);
static void error_message(char* msg);

static int init_server_unix_socket(int* unix_server_fd, int* unix_client_fd);
static int accept_unix_client(int unix_server_fd, int* unix_client_fd);
static int read_from_client(int unix_client_fd, struct measures_data_t* md);

static int init_client_tcp_socket(int* tcp_server_fd);
static int write_to_tcp_server(int tcp_server_fd, struct measures_data_t* measures_data_p);


static int daemon_mode = 0;
static int end = 0;




int main(int argc, char** argv){
    int unix_server_fd = -1;
    int unix_client_fd = -1;
    int tcp_server_fd = -1;
    struct measures_data_t measures_data;

    if (argc == 2 && strcmp(argv[1], "-d") == 0){
        daemon_mode = 1;
        openlog(NULL,0,LOG_USER);
        daemon(0,0);
    }
    set_signals();

    //  Init Unix Socket as Server
    if(!init_server_unix_socket(&unix_server_fd, &unix_client_fd)){
        closeall(unix_server_fd, unix_client_fd, tcp_server_fd);
        exit(EXIT_FAILURE);
    }

    // Init TCP Socket as Client 
    if(!init_client_tcp_socket(&tcp_server_fd)){
        closeall(unix_server_fd, unix_client_fd, tcp_server_fd);
        exit(EXIT_FAILURE);
    }

    while(!end){

        //  Read from client
        if(!read_from_client(unix_client_fd, &measures_data)){
            if(end)
                continue;

            error_message("Client disconnected");
            close(unix_client_fd);

            if(!accept_unix_client(unix_server_fd, &unix_client_fd)){
                closeall(unix_server_fd, unix_client_fd, tcp_server_fd);
                exit(EXIT_FAILURE);
            }
            continue;
        }

        //Try reconnect server
        if(tcp_server_fd == -1){
            if(!init_client_tcp_socket(&tcp_server_fd)){
                closeall(unix_server_fd, unix_client_fd, tcp_server_fd);
                exit(EXIT_FAILURE);
            }
        }

        //  Write to server
        if(tcp_server_fd != -1){
            if(!write_to_tcp_server(tcp_server_fd, &measures_data)){
                close(tcp_server_fd);
                tcp_server_fd = -1;
            }
        }

        
    }

    closeall(unix_server_fd, unix_client_fd, tcp_server_fd);
    exit(EXIT_SUCCESS);
}

static void closeall(int unix_server_fd, int unix_client_fd, int tcp_server_fd){
    if(tcp_server_fd != -1)
        close(tcp_server_fd);
    if(unix_client_fd != -1)
        close(unix_client_fd);
    if(unix_server_fd != -1)
        close(unix_server_fd);
    unlink(UNIX_SOCKET_NAME);

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




static int init_server_unix_socket(int* unix_server_fd, int* unix_client_fd){
    struct sockaddr_un unix_server_addr;

    unlink(UNIX_SOCKET_NAME);

    if((*unix_server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        error_message("Unix Socket Creation");
        return 0;
    }

    unix_server_addr.sun_family = AF_UNIX;
    strncpy(unix_server_addr.sun_path, UNIX_SOCKET_NAME, sizeof(unix_server_addr.sun_path) - 1);

    if(bind(*unix_server_fd, (const struct sockaddr*) &unix_server_addr, sizeof(unix_server_addr)) == -1){
        error_message("Bind Unix Server");
        return 0;
    }

    if(listen(*unix_server_fd, 1) == -1){
        error_message("Listen Unix Server");
        return 0;
    }
    
    if(!accept_unix_client(*unix_server_fd, unix_client_fd))
        return 0;

    return 1;
}
static int accept_unix_client(int unix_server_fd, int* unix_client_fd){
    if((*unix_client_fd = accept(unix_server_fd, NULL, NULL)) == -1){
        error_message("Accept Unix Server");
        return 0;
    }

    return 1;
}

static int read_from_client(int unix_client_fd, struct measures_data_t* md){
    return recv(unix_client_fd, md, sizeof(struct measures_data_t), 0) > 0;
}


static int init_client_tcp_socket(int* tcp_server_fd){
    struct sockaddr_in tcp_server_addr;

    memset(&tcp_server_addr, 0, sizeof(tcp_server_addr));
    tcp_server_addr.sin_family = AF_INET; 
    inet_pton(AF_INET, TCP_SOCKET_IP, &(tcp_server_addr.sin_addr));
    tcp_server_addr.sin_port = htons(TCP_SOCKET_PORT);
    
    if((*tcp_server_fd = socket(tcp_server_addr.sin_family, SOCK_STREAM, 0)) == -1){
        error_message("TCP Socket Creation");
        return 0;
    }
    
    if(connect(*tcp_server_fd, (const struct sockaddr*)&tcp_server_addr, sizeof(tcp_server_addr)) == -1){
        error_message("Connection to the TCP Socket");
        close(*tcp_server_fd);
        *tcp_server_fd = -1;
    }
    
    return 1;
}

static int write_to_tcp_server(int tcp_server_fd, struct measures_data_t* measures_data_p){
    if(send(tcp_server_fd, measures_data_p, sizeof(struct measures_data_t), MSG_NOSIGNAL) <= 0){
        error_message("Writing to TCP Server");
        return 0;
    }
    return 1;
}