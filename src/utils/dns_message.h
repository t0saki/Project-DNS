#pragma once
#include "utils.c"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int DNS_PORT = 53;
int ID = 11451;
#define DNS_TYPE_A 1
#define DNS_TYPE_CNAME 5
#define DNS_TYPE_MX 15
#define DNS_TYPE_NS 2
#define DNS_TYPE_SOA 6
#define DNS_TYPE_TXT 16
#define DNS_TYPE_AAAA 28
#define DNS_TYPE_PTR 12
#define DNS_MAX_MESSAGE_SIZE 512
#define DNS_RESPONSE 33152
#define DNS_REQUEST 0
#define DNS_RECURSION_DESIRED 1
#define DNS_AUTHORITATIVE 32
// 0x0100
#define DNS_QUERY 256

#define DNS_CLASS_IN 1

typedef struct dns_message {
    char* name;
    char* type;
    char* classt;
    char* ttl;
    char* rdlength;
    char* rdata;
    struct dns_message* next;
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
    char* qname;
    uint16_t qtype;
    uint16_t qclass;
    int qname_len;
} dns_question;

typedef struct dns_rr {
    char* name;
    uint16_t type;
    uint16_t classt;
    uint32_t ttl;
    uint16_t rdlength;
    uint16_t preference;
    char* rdata;
} dns_rr;

typedef struct dns_query {
    dns_header* header;
    dns_question* question;
    dns_rr* answer;
    dns_rr* authority;
    dns_rr* additional;
} dns_query;

typedef struct dns_cache {
    dns_question* question;
    dns_rr* answer;
    int last_used;
} dns_cache;

void pack_dns_message(const char* domain, uint16_t type, uint16_t classt,
    uint8_t* buffer, int* len);

void pack_dns_response(const dns_question* question, const dns_rr* answer,
    uint8_t* response_buffer, int* len);

// Function to parse DNS message
void parse_dns_message(const uint8_t* buffer, int len, dns_rr* answers,
    int* answers_count, dns_question* questions);

void unpack_dns_answer(const uint8_t* buffer, int offset, int len, int ancount,
    dns_rr* answers, int* answers_count);

void unpack_dns_question(const uint8_t* buffer, int offset, int len,
    dns_question* questions);

void solve_answers(char* domain, dns_rr* answers, int answers_count,
    char** ns_ips, int* ns_ips_count);
