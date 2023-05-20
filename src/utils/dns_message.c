#include "dns_message.h"

char *labels_to_domain(const uint8_t *labels, int len) {
    char *domain = (char *)malloc(len + 1);
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

// Function to pack DNS message
void pack_dns_message(const char *domain, uint16_t type, uint16_t classt,
                      uint8_t *buffer, int *len) {
    // Pack DNS header
    memset(buffer, 0, DNS_MAX_MESSAGE_SIZE);
    dns_header header = {0};
    header.id = htons(0x1234);
    header.flags = htons(DNS_QUERY);
    header.qdcount = htons(1);
    memcpy(buffer, &header, sizeof(header));

    // Pack DNS question
    int offset = sizeof(header);
    char *labels = domain_to_labels(domain);
    int domain_len = strlen(domain);
    memcpy(buffer + offset, labels, domain_len + 2);
    offset += domain_len + 2;
    uint16_t qtype = htons(type);
    memcpy(buffer + offset, &qtype, sizeof(qtype));
    offset += sizeof(qtype);
    uint16_t qclass = htons(classt);
    memcpy(buffer + offset, &qclass, sizeof(qclass));
    offset += sizeof(qclass);
    *len = offset;
    free(labels);
}

void unpack_dns_answer(const uint8_t *buffer, int offset, int len, int ancount) {
    // Iterate over each answer
    for (int i = 0; i < ancount; i++) {
        // Parse DNS answer
        dns_rr answer = {0};

        char *name;

        // if compression
        if ((buffer[offset] & 0xc0) == 0xc0) {
            // Parse name field
            name = labels_to_domain(buffer + offset, len - offset);
            answer.name = "repeat";
            offset += 2;
        } else {
            // Parse name field
            name = labels_to_domain(buffer + offset, len - offset);
            answer.name = name;
            offset += strlen(name) + 2;
        }

        // Parse type field
        memcpy(&answer.type, buffer + offset, sizeof(answer.type));
        answer.type = ntohs(answer.type);
        offset += sizeof(answer.type);

        // Parse class field
        memcpy(&answer.classt, buffer + offset, sizeof(answer.classt));
        answer.classt = ntohs(answer.classt);
        offset += sizeof(answer.classt);

        // Parse TTL field
        memcpy(&answer.ttl, buffer + offset, sizeof(answer.ttl));
        answer.ttl = ntohl(answer.ttl);
        offset += sizeof(answer.ttl);

        // Parse rdlength field
        memcpy(&answer.rdlength, buffer + offset, sizeof(answer.rdlength));
        answer.rdlength = ntohs(answer.rdlength);
        offset += sizeof(answer.rdlength);

        // Parse rdata field
        if (answer.type == DNS_TYPE_A) {
            // Parse IPv4 address
            char *ipv4;
            asprintf(&ipv4, "%d.%d.%d.%d", buffer[offset], buffer[offset + 1],
                     buffer[offset + 2], buffer[offset + 3]);
            answer.rdata = ipv4;
        } else if (answer.type == DNS_TYPE_CNAME) {
            // Parse CNAME
            char *cname = labels_to_domain(buffer + offset, len - offset);
            answer.rdata = cname;
        } else if (answer.type == DNS_TYPE_MX) {
            // Parse MX
            uint16_t preference;
            memcpy(&preference, buffer + offset, sizeof(preference));
            preference = ntohs(preference);
            char *mx = labels_to_domain(buffer + offset + 2, len - offset - 2);
            char *mx_record;
            asprintf(&mx_record, "%d %s", preference, mx);
            answer.rdata = mx_record;
        } else {
            // Parse unknown type
            answer.rdata = (char *)malloc(answer.rdlength + 1);
            memcpy(answer.rdata, buffer + offset, answer.rdlength);
            answer.rdata[answer.rdlength] = '\0';
        }
        offset += answer.rdlength;

        // Print the parsed answer
        printf("Answer %d:\n", i + 1);
        printf("Name: %s\n", answer.name);
        printf("Type: %d\n", answer.type);
        printf("Class: %d\n", answer.classt);
        printf("TTL: %d\n", answer.ttl);
        printf("RDLength: %d\n", answer.rdlength);
        printf("RData: %s\n", answer.rdata);

        // Free allocated memory
        free(name);
        free(answer.rdata);

        // Move to the next answer
        printf("\n");
    }
}

// Function to parse DNS message
void parse_dns_message(const uint8_t *buffer, int len) {
    // Parse DNS header
    dns_header header = {0};
    memcpy(&header, buffer, sizeof(header));
    header.id = ntohs(header.id);
    header.flags = ntohs(header.flags);
    header.qdcount = ntohs(header.qdcount);
    header.ancount = ntohs(header.ancount);
    header.nscount = ntohs(header.nscount);
    header.arcount = ntohs(header.arcount);
    printf("ID: %d\n", header.id);
    printf("Flags: %d\n", header.flags);
    printf("QDCOUNT: %d\n", header.qdcount);
    printf("ANCOUNT: %d\n", header.ancount);
    printf("NSCOUNT: %d\n", header.nscount);
    printf("ARCOUNT: %d\n", header.arcount);

    // Parse DNS question
    int offset = sizeof(header);
    char *domain = labels_to_domain(buffer + offset, len - offset);
    printf("QNAME: %s\n", domain);
    offset += strlen(domain) + 2;
    uint16_t qtype = 0;
    memcpy(&qtype, buffer + offset, sizeof(qtype));
    qtype = ntohs(qtype);
    printf("QTYPE: %d\n", qtype);
    offset += sizeof(qtype);
    uint16_t qclass = 0;
    memcpy(&qclass, buffer + offset, sizeof(qclass));
    qclass = ntohs(qclass);
    printf("QCLASS: %d\n", qclass);
    offset += sizeof(qclass);
    free(domain);

    // Parse DNS answer
    if (header.ancount > 0) {
        printf("Answers:\n");
        unpack_dns_answer(buffer, offset, len, header.ancount);
    }
}
