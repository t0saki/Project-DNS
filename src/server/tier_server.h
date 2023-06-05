#pragma once
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "../utils/dns_message.c"
#include "../utils/socket.c"
#include "../utils/utils.c"
#include "../utils/rr_reader.c"
#include "server_args.h"


void* tier_server(void* args) {
    int tier = (long long)args;
    char* server_address = TIER_SERVERS[tier];
    char* rr_file = TIER_RR_FILES[tier];

    char* msg = (char*)malloc(1000);
    sprintf(msg, "Starting DNS server at %s:%d\n", server_address, DNS_PORT);
    write_log(msg);

    // Read the resource records from the file,dns_rr **read_rr_all(char
    // *filename, int *countr)
    int rr_count = 0;
    dns_rr** rrs = read_rr_all(rr_file, &rr_count);

    // Start receiving TCP DNS queries
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd<0) {
        write_log("Failed to create socket.\n");
        exit(EXIT_FAILURE);
    }

    // Bind to the specified port
    struct sockaddr_in server_addr = { 0 };
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_address);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr))<
        0) {
        char* msg = (char*)malloc(1000);
        sprintf(msg, "Failed to bind to port %d.\n", DNS_PORT);
        write_log(msg);
        exit(EXIT_FAILURE);
    }

    printf("Started listening at %s:%d\n", server_address, DNS_PORT);

    if (listen(sockfd, 5)<0) {
        write_log("Failed to listen.\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        rrs = read_rr_all(rr_file, &rr_count);
        // Accept a TCP connection
        struct sockaddr_in client_addr = { 0 };
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sockfd =
            accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sockfd<0) {
            write_log("Failed to accept connection.\n");
            exit(EXIT_FAILURE);
        }

        // Receive the DNS query
        char* buffer = (char*)malloc(1024);
        int len = recv(client_sockfd, buffer, 1024, 0);
        if (len<0) {
            write_log("Failed to receive DNS query.\n");
            exit(EXIT_FAILURE);
        }

        // // Delete the first 2 bytes of length
        // len -= 2;
        // memmove(buffer, buffer + 2, len);

        printf("Received DNS query: %d\n", len);

        // Parse the DNS query
        dns_question question = { 0 };

        parse_dns_message((uint8_t*)tcp2udp(buffer, len), (int)len,
            (dns_rr*)NULL, (int*)NULL,
            (dns_question*)&question);

        // Find the resource record
        // dns_rr *rr = find_rr(rrs, rr_count, add_dot(question.qname),
        //                      question.qtype);

        char* query_domain = add_dot(question.qname);

        int rr_response_index =
            find_rr(rrs, rr_count, query_domain, question.qtype);

        dns_rr rr_response = { 0 };
        if (rr_response_index!=-1) {
            rr_response = *rrs[rr_response_index];
        }

        // If has full A record or CNAME
        if (rr_response_index==-1) {
            rr_response_index =
                find_rr(rrs, rr_count, query_domain, DNS_TYPE_A);
            if (rr_response_index!=-1)
                rr_response = *rrs[rr_response_index];
        }

        if (rr_response_index==-1) {
            rr_response_index =
                find_rr(rrs, rr_count, query_domain, DNS_TYPE_CNAME);
            // If found CNAME
            if (rr_response_index!=-1) {
                // Find the A record for the CNAME
                int cname_a_rr_index = find_rr(
                    rrs, rr_count, rrs[rr_response_index]->rdata, DNS_TYPE_A);
                rr_response = *rrs[rr_response_index];
                rr_response.type = DNS_TYPE_A;
                if (cname_a_rr_index!=-1)
                    rr_response.rdata = rrs[cname_a_rr_index]->rdata;
            }
        }

        if (rr_response_index==-1) {
            // If not found, find the NS record
            // Find ns first
            int ns_rr_index = find_rr(rrs, rr_count, query_domain, DNS_TYPE_NS);
            if (ns_rr_index!=-1) {
                char* ns_domain = rrs[ns_rr_index]->rdata;

                // Find the A record for the NS
                int ns_a_rr_index =
                    find_rr(rrs, rr_count, ns_domain, DNS_TYPE_A);

                rr_response = *rrs[ns_a_rr_index];
                rr_response.name = rrs[ns_rr_index]->name;
                rr_response.type = DNS_TYPE_A;
                rr_response.rdata = rrs[ns_a_rr_index]->rdata;
            } else {
                // If not found, send not found
                rr_response.name = query_domain;
                if (question.qtype==DNS_TYPE_A) {
                    rr_response.type = DNS_TYPE_A;
                    rr_response.rdata = (char*)malloc(8);
                    sprintf(rr_response.rdata, "0.0.0.0");
                } else if (question.qtype==DNS_TYPE_CNAME||
                    question.qtype==DNS_TYPE_MX) {
                    rr_response.type = DNS_TYPE_CNAME;
                    rr_response.rdata = (char*)malloc(16);
                    sprintf(rr_response.rdata, "not.found");
                }
            }
        }

        // Remove ending dot from rr_response.name
        if (rr_response.name[strlen(rr_response.name)-1]=='.')
            rr_response.name[strlen(rr_response.name)-1] = '\0';

        char* msg = (char*)malloc(1000);
        sprintf(msg, "Received DNS query: %s, sent DNS response: %s->%s\n",
            question.qname, rr_response.name, rr_response.rdata);
        write_log(msg);

        // Create the DNS response
        uint8_t response[1024];
        int response_len = 0;
        pack_dns_response(&question, &rr_response, response, &response_len);

        // // Add 2 bytes of length to the beginning of the buffer
        // uint16_t len2 = htons(response_len);
        // memcpy(buffer, &len2, 2);

        // Send the DNS response
        if (send(client_sockfd, (char*)udp2tcp((char*)response, response_len),
            response_len+2, 0)<0) {
            write_log("Failed to send DNS response.\n");
            exit(EXIT_FAILURE);
        }

        // Close the client socket
        close(client_sockfd);


    }

    while (1) {
        sleep(1000000);
    }
}