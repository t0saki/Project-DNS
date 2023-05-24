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
    record->type = atoi(token);

    token = strtok(NULL, " ");
    record->rdata = (char *)malloc(strlen(token) + 1);
    strcpy(record->rdata, token);

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

dns_rr *find_rr(dns_rr ** records, int countr, char *name, int type) {
    for (int i = 0; i < countr; i++) {
        if (strcmp(records[i]->name, name) == 0 && records[i]->type == type) {
            return records[i];
        }
    }
    // else find best match
    int best_match = 0;
    int best_match_index = -1;
    for (int i = 0; i < countr; i++) {
        if (strcmp(records[i]->name, name) > best_match) {
            best_match = strcmp(records[i]->name, name);
            best_match_index = i;
        }
    }
    if (best_match_index == -1) {
        return NULL;
    }
    return records[best_match_index];
}