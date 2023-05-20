#pragma once
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DNS_TYPE_A 1
#define DNS_TYPE_CNAME 5
#define DNS_TYPE_MX 15
#define DNS_MAX_MESSAGE_SIZE 512
#define DNS_RESPONSE 1
#define DNS_QUERY 0
#define DNS_CLASS_IN 1

typedef struct dns_message {
    char *name;
    char *type;
    char *classt;
    char *ttl;
    char *rdlength;
    char *rdata;
    struct dns_message *next;
} dns_message;

typedef struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header;

typedef struct dns_question {
    char *qname;
    uint16_t qtype;
    uint16_t qclass;
    int qname_len;
} dns_question;

typedef struct dns_rr {
    char *name;
    uint16_t type;
    uint16_t classt;
    uint32_t ttl;
    uint16_t rdlength;
    char *rdata;
} dns_rr;

typedef struct dns_query {
    dns_header *header;
    dns_question *question;
    dns_rr *answer;
    dns_rr *authority;
    dns_rr *additional;
} dns_query;

char *labels_to_domain(const uint8_t *labels, int len);

char *domain_to_labels(const char *domain);

void pack_dns_message(const char *domain, uint16_t type, uint16_t classt,
                      uint8_t *buffer, int *len);

void unpack_dns_answer(const uint8_t *buffer, int offset, int len, int ancount);

void parse_dns_message(const uint8_t *buffer, int len);

