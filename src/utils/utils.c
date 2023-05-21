#pragma once
#include "utils.h"

char *log_file = NULL;

char *strtrim(char *str) {
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

char *labels_to_domain(const uint8_t *labels, int len) {
    char *domain = (char *)malloc(128);
    int offset = 0;
    int domain_offset = 0;
    while (labels[offset] != '\0') {
        int label_len = labels[offset];
        for (int i = 0; i < label_len; i++) {
            domain[domain_offset] = labels[offset + 1 + i];
            domain_offset++;
        }
        domain[domain_offset] = '.';
        domain_offset++;
        offset += label_len + 1;
    }
    domain[domain_offset - 1] = '\0';
    return domain;
}

// test:labels_to_domain((uint8_t *)"\x03www\x06google\x03com\x00", 17)

// Function to convert domain name to labels
char *domain_to_labels(const char *domain) {
    char *labels = (char *)malloc(strlen(domain) + 2);
    int offset = 0;
    int label_len = 0;
    while (domain[offset] != '\0') {
        if (domain[offset] == '.') {
            labels[offset - label_len] = label_len;
            label_len = 0;
        } else {
            labels[offset + 1] = domain[offset];
            label_len++;
        }
        offset++;
    }
    labels[offset + 1] = '\0';
    labels[offset - label_len] = label_len;
    labels[offset + 1] = '\0';
    return labels;
}

void create_log_file() {
    if (log_file == NULL) {
        log_file = (char *)malloc(128);
        time_t now;
        time(&now);
        struct tm *local = localtime(&now);
        sprintf(log_file, "logs/dns_svr_%.4d%.2d%.2d%.2d%.2d%.2d.log",
                local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
                local->tm_hour, local->tm_min, local->tm_sec);

        FILE *fp = fopen(log_file, "w");
        fclose(fp);

        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("Log file created: %s/%s\n", cwd, log_file);

        char *log = (char *)malloc(128);
        sprintf(log, "Log file created: %s/%s", cwd, log_file);
        write_log(log);

        free(log);
    }
}

void create_log_file_client() {
    if (log_file == NULL) {
        log_file = (char *)malloc(128);
        time_t now;
        time(&now);
        struct tm *local = localtime(&now);
        sprintf(log_file, "logs/dns_client_%.4d%.2d%.2d%.2d%.2d%.2d.log",
                local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
                local->tm_hour, local->tm_min, local->tm_sec);

        FILE *fp = fopen(log_file, "w");
        fclose(fp);

        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("Log file created: %s/%s\n", cwd, log_file);

        char *log = (char *)malloc(128);
        sprintf(log, "Log file created: %s/%s", cwd, log_file);
        write_log(log);

        free(log);
    }
}

void write_log(char *log) {
    FILE *fp = fopen(log_file, "a");
    fprintf(fp, "%s\n", log);
    fclose(fp);
}