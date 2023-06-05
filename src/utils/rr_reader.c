#pragma once
#include "rr_reader.h"

/*
rr record would be like this:
cn. 86400 IN NS ns3.servers.com.
ns3.servers.com. 86400 IN A 127.3.3.1
*/

// Read a line from the file
char *read_line(FILE *file) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, file);
    if (read == -1) {
        return NULL;
    }
    return line;
}

// Read a resource record from the file
dns_rr *read_rr(FILE *file) {
    char *line = read_line(file);
    if (line == NULL) {
        return NULL;
    }

    // Parse the line
    char *token = strtok(line, " ");
    dns_rr *record = (dns_rr *)malloc(sizeof(dns_rr));
    record->name = (char *)malloc(strlen(token) + 1);
    strcpy(record->name, token);

    token = strtok(NULL, " ");
    record->ttl = atoi(token);

    token = strtok(NULL, " ");
    if (strcmp(token, "IN") == 0) {
        record->classt = DNS_CLASS_IN;
    }

    token = strtok(NULL, " ");
    if (strcmp(token, "A") == 0) {
        record->type = DNS_TYPE_A;
    } else if (strcmp(token, "CNAME") == 0) {
        record->type = DNS_TYPE_CNAME;
    } else if (strcmp(token, "MX") == 0) {
        record->type = DNS_TYPE_MX;
    } else if (strcmp(token, "NS") == 0) {
        record->type = DNS_TYPE_NS;
    } else if (strcmp(token, "SOA") == 0) {
        record->type = DNS_TYPE_SOA;
    } else if (strcmp(token, "TXT") == 0) {
        record->type = DNS_TYPE_TXT;
    } else if (strcmp(token, "AAAA") == 0) {
        record->type = DNS_TYPE_AAAA;
    } else if (strcmp(token, "PTR") == 0) {
        record->type = DNS_TYPE_PTR;
    }

    token = strtok(NULL, " ");
    record->rdlength = strlen(token);
    record->rdata = (char *)malloc(strlen(token) + 1);
    strcpy(record->rdata, token);

    // Remove \n if it exists
    if (record->rdata[strlen(record->rdata) - 1] == '\n') {
        record->rdata[strlen(record->rdata) - 1] = '\0';
    }

    return record;
}

// Read all resource records from the file
dns_rr **read_rr_all(char *filename, int *countr) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }

    // Count the number of lines in the file
    int count = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, file)) != -1) {
        count++;
    }
    free(line);

    // Read all resource records
    dns_rr **records = (dns_rr **)malloc(sizeof(dns_rr *) * count);
    rewind(file);
    for (int i = 0; i < count; i++) {
        records[i] = read_rr(file);
    }

    fclose(file);
    *countr = count;
    return records;
}

int find_rr(dns_rr ** records, int countr, char *name, int type) {
    for (int i = 0; i < countr; i++) {
        if (strcmp(records[i]->name, name) == 0 && records[i]->type == type) {
            return i;
        }
    }
    // else find best match at the end, such as google.com. and com.
    int best_match = 0;
    int best_match_index = -1;
    size_t name_len = strlen(name);
    for (int i = name_len - 1; i >= 0; i--) {
        char *name_part = name + i;
        for (int j = 0; j < countr; j++) {

            if (strcmp(records[j]->name, name_part) == 0 && records[j]->type == type) {
                if (i > best_match) {
                    best_match = name_len - i;
                    best_match_index = j;
                }
            }
        }
    }
    if (best_match_index != -1) {
        return best_match_index;
    }
    return -1;
}