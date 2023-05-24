#pragma once
#include "dns_message.h"

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
    // free(labels);
}

void pack_dns_response(const dns_question *question, const dns_rr *answer,
                       uint8_t *response_buffer, int *len) {
    // Pack DNS header
    memset(response_buffer, 0, DNS_MAX_MESSAGE_SIZE);
    dns_header header = {0};
    header.id = htons(0x1234);
    header.flags = htons(DNS_RESPONSE | DNS_RECURSION_DESIRED);
    header.qdcount = htons(1);
    header.ancount = htons(1);
    memcpy(response_buffer, &header, sizeof(header));

    // Pack DNS question
    int offset = sizeof(header);
    memcpy(response_buffer + offset, question->qname, question->qname_len + 2);
    offset += question->qname_len + 2;
    uint16_t qtype = htons(question->qtype);
    memcpy(response_buffer + offset, &qtype, sizeof(qtype));
    offset += sizeof(qtype);
    uint16_t qclass = htons(question->qclass);
    memcpy(response_buffer + offset, &qclass, sizeof(qclass));
    offset += sizeof(qclass);

    // Pack DNS answer
    memcpy(response_buffer + offset, answer->name, strlen(answer->name) + 2);
    offset += strlen(answer->name) + 2;
    uint16_t type = htons(answer->type);
    memcpy(response_buffer + offset, &type, sizeof(type));
    offset += sizeof(type);
    uint16_t classt = htons(answer->classt);
    memcpy(response_buffer + offset, &classt, sizeof(classt));
    offset += sizeof(classt);
    uint32_t ttl = htonl(answer->ttl);
    memcpy(response_buffer + offset, &ttl, sizeof(ttl));
    offset += sizeof(ttl);
    uint16_t rdlength = htons(answer->rdlength);
    memcpy(response_buffer + offset, &rdlength, sizeof(rdlength));
    offset += sizeof(rdlength);
    memcpy(response_buffer + offset, answer->rdata, answer->rdlength);
    offset += answer->rdlength;
    *len = offset;
}

// Function to parse DNS message
void parse_dns_message(const uint8_t *buffer, int len, dns_rr *answers,
                       int *answers_count, dns_question *questions) {
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
    // free(domain);
    printf("\n");

    dns_question q = {0};
    q.qname = domain;
    q.qname_len = strlen(domain);
    q.qtype = qtype;
    q.qclass = qclass;
    *questions = q;

    // Parse DNS question
    // if (header.qdcount > 0 && questions != NULL) {
    //     printf("Questions:\n");
    //     unpack_dns_question(buffer, offset, len, questions);
    // }

    // Parse DNS answer
    if (header.ancount > 0 && answers != NULL) {
        printf("Answers:\n");
        unpack_dns_answer(buffer, offset, len, header.ancount, answers,
                          answers_count);
    }
}

// Parse DNS question
void unpack_dns_question(const uint8_t *buffer, int offset, int len,
                         dns_question *question) {
    dns_question q = {0};

    // Parse name field
    char *name = labels_to_domain(buffer + offset, len - offset);
    q.qname = name;
    q.qname_len = strlen(name);
    offset += strlen(name) + 2;

    // Parse type field
    memcpy(&q.qtype, buffer + offset, sizeof(q.qtype));
    q.qtype = ntohs(q.qtype);
    offset += sizeof(q.qtype);

    // Parse class field
    memcpy(&q.qclass, buffer + offset, sizeof(q.qclass));
    q.qclass = ntohs(q.qclass);
    offset += sizeof(q.qclass);

    printf("    QNAME: %s\n", q.qname);
    printf("    QTYPE: %d\n", q.qtype);
    printf("    QCLASS: %d\n", q.qclass);
    printf("\n");
}

void unpack_dns_answer(const uint8_t *buffer, int offset, int len, int ancount,
                       dns_rr *answers, int *answers_count) {
    *answers_count = ancount;

    // Iterate over each answer
    for (int i = 0; i < ancount; i++) {
        // Parse DNS answer
        dns_rr answer = {0};

        char *name;

        // if compression
        if ((buffer[offset] & 0xc0) == 0xc0) {
            // Parse name field
            int nameoffset = buffer[offset + 1];
            name = labels_to_domain(buffer + nameoffset, len - nameoffset);
            answer.name = name;
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
        answer.rdata = (char *)malloc(answer.rdlength + 1);
        if (answer.type == DNS_TYPE_A) {
            // Parse IPv4 address
            char ipv4[32];
            sprintf(ipv4, "%d.%d.%d.%d", buffer[offset], buffer[offset + 1],
                    buffer[offset + 2], buffer[offset + 3]);
            // asprintf((char**)&ipv4, "%d.%d.%d.%d", buffer[offset],
            // buffer[offset + 1],
            //          buffer[offset + 2], buffer[offset + 3]);
            strcpy(answer.rdata, ipv4);
        } else if (answer.type == DNS_TYPE_CNAME) {
            // Parse CNAME
            char *cname = labels_to_domain(buffer + offset, len - offset);
            strcpy(answer.rdata, cname);
        } else if (answer.type == DNS_TYPE_MX) {
            // Parse MX
            uint16_t preference;
            memcpy(&preference, buffer + offset, sizeof(preference));
            preference = ntohs(preference);
            char *mx = labels_to_domain(buffer + offset + 2, len - offset - 2);
            char *mx_record;
            sprintf(mx_record, "%d %s", preference, mx);
            strcpy(answer.rdata, mx_record);
        } else if (answer.type == DNS_TYPE_NS) {
            // Parse NS
            char *ns = labels_to_domain(buffer + offset, len - offset);
            strcpy(answer.rdata, ns);
        } else if (answer.type == DNS_TYPE_SOA) {
            // Parse SOA
            char *mname = labels_to_domain(buffer + offset, len - offset);
            char *rname = labels_to_domain(buffer + offset + strlen(mname) + 1,
                                           len - offset - strlen(mname) - 1);
            uint32_t serial;
            memcpy(&serial,
                   buffer + offset + strlen(mname) + 1 + strlen(rname) + 1 + 20,
                   sizeof(serial));
            serial = ntohl(serial);
            uint32_t refresh;
            memcpy(&refresh,
                   buffer + offset + strlen(mname) + 1 + strlen(rname) + 1 +
                       20 + 4,
                   sizeof(refresh));
            refresh = ntohl(refresh);
            uint32_t retry;
            memcpy(&retry,
                   buffer + offset + strlen(mname) + 1 + strlen(rname) + 1 +
                       20 + 4 + 4,
                   sizeof(retry));
            retry = ntohl(retry);
            uint32_t expire;
            memcpy(&expire,
                   buffer + offset + strlen(mname) + 1 + strlen(rname) + 1 +
                       20 + 4 + 4 + 4,
                   sizeof(expire));
            expire = ntohl(expire);
            uint32_t minimum;
            memcpy(&minimum,
                   buffer + offset + strlen(mname) + 1 + strlen(rname) + 1 +
                       20 + 4 + 4 + 4 + 4,
                   sizeof(minimum));
            minimum = ntohl(minimum);
            char *soa_record;
            sprintf(soa_record, "%s %s %d %d %d %d %d", mname, rname, serial,
                    refresh, retry, expire, minimum);
            strcpy(answer.rdata, soa_record);
        } else {
            // Parse unknown type
            answer.rdata = (char *)malloc(answer.rdlength + 1);
            memcpy(answer.rdata, buffer + offset, answer.rdlength);
            answer.rdata[answer.rdlength] = '\0';
        }
        offset += answer.rdlength;

        // Add answer to the list
        answers[i] = answer;

        // Print the parsed answer
        printf("    Answer %d:\n", i + 1);
        printf("    Name: %s\n", answer.name);
        printf("    Type: %d\n", answer.type);
        printf("    Class: %d\n", answer.classt);
        printf("    TTL: %d\n", answer.ttl);
        printf("    RDLength: %d\n", answer.rdlength);
        printf("    RData: %s\n", answer.rdata);

        // Move to the next answer
        printf("\n");
    }
}

void solve_answers(char *domain, dns_rr *answers, int answers_count,
                   char **ns_ips, int *ns_ips_count) {
    // Iterate over each answer
    for (int i = 0; i < answers_count; i++) {
        // Parse DNS answer
        dns_rr answer = answers[i];

        if (answer.type == DNS_TYPE_A) {
            if (strcmp(answer.name, domain) == 0) {
                ns_ips[*ns_ips_count] =
                    (char *)malloc(strlen(answer.rdata) + 1);
                strcpy(ns_ips[*ns_ips_count], answer.rdata);
                printf("    NS IP: %s\n", answer.rdata);
                (*ns_ips_count)++;
            }
        } else if (answer.type == DNS_TYPE_CNAME) {
            char *cname = answer.rdata;
            strcpy(domain, cname);
        } else if (answer.type == DNS_TYPE_MX) {
            char *mx = answer.rdata;
            char *mx_domain = mx + 2;
            strcpy(domain, mx_domain);
        } else if (answer.type == DNS_TYPE_NS) {
            char *ns = answer.rdata;
            strcpy(domain, ns);
        } else if (answer.type == DNS_TYPE_SOA) {
            char *soa = answer.rdata;
            char *mname = strtok(soa, " ");
            char *rname = strtok(NULL, " ");
            char *soa_domain = rname + 1;
            strcpy(domain, soa_domain);
        } else {
            // Parse unknown type
            printf("    Unknown type: %d\n", answer.type);
        }
    }
}