

#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>


#define UNIX_SOCKET_NAME "/tmp/telemetry-client.socket"

void init_server_unix_socket();

int unix_server_fd;
int unix_client_fd;



int main(int argc, char** argv){
    double readdata;

    fprintf(stderr, "Starting connection...\n");
    init_server_unix_socket();

    while(1){
        read(unix_client_fd, &readdata, sizeof(double));

        fprintf(stderr, "Received: %lf", readdata);
    }


}






void init_server_unix_socket(){
    struct sockaddr_un unix_server_addr;

    unix_server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    unix_server_addr.sun_family = AF_UNIX;
    strncpy(unix_server_addr.sun_path, UNIX_SOCKET_NAME, sizeof(unix_server_addr.sun_path) - 1);

    if(bind(unix_server_fd, (const struct sockaddr*) &unix_server_addr, sizeof(unix_server_addr)) == -1){
        fprintf(stderr, "Error bind unix server\n");
        exit(EXIT_FAILURE);
    }

    listen(unix_server_fd, 1);

    if((unix_client_fd = accept(unix_server_fd, NULL, NULL)) == -1){
        fprintf(stderr, "Error accept unix server\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Connection accepted\n");
}



