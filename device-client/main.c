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

static void closeall(int unix_server_fd, int unix_client_fd);
static void set_signals();
static void term_handler(int);
static void error_message(char* msg);
static int init_server_unix_socket(int* unix_server_fd, int* unix_client_fd);
static int accept_unix_client(int unix_server_fd, int* unix_client_fd);
static int read_from_client(int unix_client_fd, struct measures_data_t* md);


static daemon_mode = 0;
static end = 0;




int main(int argc, char** argv){
    int unix_server_fd = -1;
    int unix_client_fd = -1;
    struct measures_data_t measures_data;

    if (argc == 2 && strcmp(argv[1], "-d") == 0){
        daemon_mode = 1;
        openlog(NULL,0,LOG_USER);
        daemon(0,0);
    }
    set_signals();

    //  Init Unix Socket as Server
    if(!init_server_unix_socket(&unix_server_fd, &unix_client_fd)){
        closeall(unix_server_fd, unix_client_fd);
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
                closeall(unix_server_fd, unix_client_fd);
                exit(EXIT_FAILURE);
            }
            continue;
        }

        //  Write to server
        fprintf(stderr, "Temp: %.2lf | Pres: %.2lf | Time: %d\n", measures_data.temperature, measures_data.pressure, measures_data.timestamp);

        
    }

    closeall(unix_server_fd, unix_client_fd);
    exit(EXIT_SUCCESS);
}

static void closeall(int unix_server_fd, int unix_client_fd){
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




static int init_server_unix_socket(int* unix_server_fd, int* unix_client_fd){
    struct sockaddr_un unix_server_addr;

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

