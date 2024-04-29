#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h> 


typedef struct UDP_Header Packet;

int rudp_socket();

int got_ACK(int sockfd, struct sockaddr * from, socklen_t * fromlen);

int handshake_connect(int sockfd, struct sockaddr *serv_addr, socklen_t addrlen, struct sockaddr * respond_server, socklen_t * respond_server_len);

int rudp_send(int sockfd, const void*msg, int len, struct sockaddr *serv_addr, socklen_t addrlen, struct sockaddr * respond_server, socklen_t * respond_server_len);

int rdup_close(int sockfd, struct sockaddr *serv_addr, socklen_t addrlen, struct sockaddr* respond_server, socklen_t * respond_server_len);

int send_ACK(int sockfd, struct sockaddr * dest, socklen_t destlen);

int rudp_receive(int sockfd, struct sockaddr * Sender_adrr, socklen_t * Sender_len);

unsigned short int calculate_checksum(void *data, size_t bytes) ;

int verify_checksum(Packet *buffer, size_t bytes);


