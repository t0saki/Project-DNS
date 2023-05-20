#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* strtrim(char* str) {
    size_t len = strlen(str);
    size_t start = 0;
    size_t end = len - 1;

    while (start <= end && str[start] == ' ') {
        start++;
    }

    while (end >= start && str[end] == ' ') {
        end--;
    }

    size_t trimmed_len = end - start + 1;
    memmove(str, str + start, trimmed_len);
    str[trimmed_len] = '\0';

    return str;
}