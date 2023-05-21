#pragma once
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "../utils/dns_message.c"
#include "../utils/socket.c"
#include "../utils/utils.c"

void *start_server(void *args);

void *server(void *args);