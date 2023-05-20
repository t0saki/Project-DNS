#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
// #include "dns_message.h"

void udp_send(int sockfd, struct sockaddr_in *addr, char *buf, int len);