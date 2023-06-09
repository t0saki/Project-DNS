#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

char* strtrim(char* str);

char* labels_to_domain(const uint8_t* labels, int len);

char* domain_to_labels(const char* domain);

void create_log_file();

void create_log_file_client();

void write_log(char* log);

char* convert2ptr(char* domain);

int is_valid_domain(char* domain);