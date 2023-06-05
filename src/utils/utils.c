#pragma once
#include "utils.h"

char *log_file = NULL;
FILE *log_fp = NULL;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void create_log_file(char *type) {
    if (log_file == NULL) {
        // Check folder logs exists
        struct stat st = {0};
        if (stat("logs", &st) == -1) {
            mkdir("logs", 0700);
        }

        log_file = (char *)malloc(128);
        time_t now;
        time(&now);
        struct tm *local = localtime(&now);
        sprintf(log_file, "logs/dns_%s_%.4d%.2d%.2d%.2d%.2d%.2d.log", type,
                local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
                local->tm_hour, local->tm_min, local->tm_sec);

        log_fp = fopen(log_file, "w");
        fclose(log_fp);

        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("Log file created: %s/%s\n", cwd, log_file);

        char *log = (char *)malloc(128);
        sprintf(log, "Log file created: %s/%s", cwd, log_file);
        write_log(log);

        free(log);
    }
}

void create_log_file_server() { create_log_file("server"); }

void create_log_file_client() { create_log_file("client"); }

void write_log(char *log) {
    // Lock mutex
    while (pthread_mutex_lock(&log_mutex) != 0) {
        // printf("Error locking mutex\n");
    }

    if (log_file == NULL) {
        printf("Log file not created\n");
        return;
    }
    log_fp = fopen(log_file, "a");
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    fprintf(log_fp, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d %s\n", local->tm_year + 1900,
            local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min,
            local->tm_sec, log);
    fclose(log_fp);

    // Unlock mutex
    if (pthread_mutex_unlock(&log_mutex) != 0) {
        printf("Error unlocking mutex\n");
    }
}

void close_log_file() {
    if (log_file != NULL) {
        free(log_file);
        log_file = NULL;
    }
    if (log_fp != NULL) {
        fclose(log_fp);
        log_fp = NULL;
    }
}

char* add_dot(char* domain) {
    char* new_domain = (char*)malloc(1000);
    strcpy(new_domain, domain);
    strcat(new_domain, ".");
    return new_domain;
}

char* convert2ptr(char* domain) {
    // Reverse the IP address and append .in-addr.arpa
    char* new_domain = (char*)malloc(1000);
    int ip[4];
    sscanf(domain, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
    sprintf(new_domain, "%d.%d.%d.%d.in-addr.arpa", ip[3], ip[2], ip[1], ip[0]);
    return new_domain;
}

int is_valid_domain(char* domain) {
    // Check if domain is valid
    int len = strlen(domain);
    if (len > 255) {
        return 0;
    }
    for (int i = 0; i < len; i++) {
        if (domain[i] == '.') {
            if (i == 0 || i == len - 1) {
                return 0;
            }
            if (domain[i - 1] == '.') {
                return 0;
            }
        } else if (domain[i] == '-') {
            if (i == 0 || i == len - 1) {
                return 0;
            }
            if (domain[i - 1] == '.' || domain[i + 1] == '.') {
                return 0;
            }
        } else if (domain[i] == '_') {
            if (i == 0 || i == len - 1) {
                return 0;
            }
            if (domain[i - 1] == '.' || domain[i + 1] == '.') {
                return 0;
            }
        } else if (domain[i] == ' ') {
            return 0;
        }
    }
    return 1;
}