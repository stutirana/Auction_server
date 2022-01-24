#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "../include/resources.h"
#define BUFFER_SIZE 1024
#define SA struct sockaddr

unsigned int establish_port(int input_size, char * requested_port[],List_t* auctions);
void run_server(int server_port);
int server_init(int server_port);
void *process_client(void* clientfd_ptr);

#endif
