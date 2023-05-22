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
rr *read_rr(FILE *file) {
    char *line = read_line(file);
    if (line == NULL) {
        return NULL;
    }

    // Parse the line
    char *token = strtok(line, " ");
    rr *record = (rr *)malloc(sizeof(rr));
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
rr **read_rr_all(char *filename) {
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
    rr **records = (rr **)malloc(sizeof(rr *) * count);
    rewind(file);
    for (int i = 0; i < count; i++) {
        records[i] = read_rr(file);
    }

    fclose(file);
    return records;
}