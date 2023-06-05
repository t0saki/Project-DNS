#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dns_message.c"
#include "utils.c"

char* read_line(FILE* file);

dns_rr* read_rr(FILE* file);

dns_rr** read_rr_all(char* filename, int* countr);

int find_rr(dns_rr** records, int countr, char* name, int type);