#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
// #include "dns_message.h"

int udp_send(char *server_address, char *domain, uint16_t port,
             uint16_t query_type);

void udp_recv(int sockfd, int *response_len, uint8_t *response_buffer);

int tcp_send(char *server_address, char *domain, uint16_t port,
             uint16_t query_type);

void tcp_recv(int sockfd, int *response_len, uint8_t *response_buffer);