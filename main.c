//
// Server side C program to demonstrate Socket programming
//
// https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa

#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#include "libctemplate/ctemplate.h"
#include "config.h"
#include "lib.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *client_handler(int new_socket)
{
    printf("\n\nIN THREAD\n\n");
    int n;
    char raw_request[30000] = {0x0};
    pthread_mutex_lock(&lock);
    n = read(new_socket , raw_request, 30000);
    if (n < 0)
    {
        perror("ERROR reading from socket!\n");
        close(new_socket);
    }

    printf("%s\n", raw_request );
    char *response = handle_request(raw_request);
    if (!response)
        response = error();

    n = write(new_socket , response , strlen(response));
    if (n < 0)
    {
        perror("ERROR writing to socket!\n");
        close(new_socket);
        free(response);
    }
    pthread_mutex_unlock(&lock);
    printf("------------------Response sent-------------------\n");
    printf("%s", response);

    free(response);
    close(new_socket);
}


int main(int argc, char const *argv[])
{
    int server_fd, new_socket; long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }
    

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    
    memset(address.sin_zero, '\0', sizeof address.sin_zero);
    
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }

    unsigned int i = 0;
    pid_t pid[MAX_THREADS];
    while(1)
    {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        int pid_c = 0;
        if ((pid_c = fork()) == 0)
        {
            client_handler(new_socket);
        }
        else
        {
            pid[i++] = pid_c;
            if (i >= MAX_THREADS - 1)
            {
                i = 0;
                while (i < MAX_THREADS)
                {
                    waitpid(pid[i++], NULL, 0);
                }
                i = 0;
            }
        }
    }
    return 0;
}
