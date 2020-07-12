//
// Server side C program to demonstrate Socket programming
//
// https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#include "libctemplate/ctemplate.h"
#include "config.h"
#include "lib.h"

void *client_handler(void *sock)
{
    printf("\n\nIN THREAD\n\n");
    int n;
    int new_socket = *((int *) sock);

    char raw_request[30000] = {0x0};
    n = read(new_socket , raw_request, 30000);
    if (n < 0)
    {
        perror("ERROR reading from socket!\n");
        close(new_socket);
        pthread_exit(NULL);
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
        pthread_exit(NULL);
    }
    printf("------------------Response sent-------------------\n");
    printf("%s", response);

    close(new_socket);
    free(response);
    pthread_exit(NULL);
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
    pthread_t interrupt[MAX_THREADS + 10];
    while(1)
    {
        printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        int re = pthread_create(&interrupt[i++], NULL, client_handler, &new_socket);
        if(re)
        {
            printf("ERROR return code from the pthread_create() is %d\n", re);
        }
        if( i >= MAX_THREADS)
        {
            i = 0;
            while(i < MAX_THREADS)
            {
                pthread_join(interrupt[i++], NULL);
                printf("\n\nJOINING THREAD\n\n");
            }
            i = 0;
        }
    }
    return 0;
}
