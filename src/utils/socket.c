#pragma once
#include "socket.h"

int udp_send(char *server_address, char *domain, uint16_t port,
             uint16_t query_type) {
    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (inet_pton(AF_INET, server_address, &server.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address.\n");
        exit(EXIT_FAILURE);
    }

    // Pack DNS query message
    uint8_t dns_buffer[DNS_MAX_MESSAGE_SIZE];
    memset(dns_buffer, 0, DNS_MAX_MESSAGE_SIZE);
    int dns_buffer_len = 0;
    pack_dns_message(domain, query_type, DNS_CLASS_IN, dns_buffer,
                     &dns_buffer_len);

    // Send DNS query
    if (sendto(sockfd, dns_buffer, dns_buffer_len, 0,
               (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void udp_recv(int sockfd, int *response_len, uint8_t *response_buffer) {
    // Receive DNS response
    *response_len =
        recvfrom(sockfd, response_buffer, DNS_MAX_MESSAGE_SIZE, 0, NULL, NULL);
    if (*response_len < 0) {
        perror("recvfrom failed");
        exit(EXIT_FAILURE);
    }
}

int tcp_send(char *server_address, char *domain, uint16_t port,
             uint16_t query_type) {
    // Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (inet_pton(AF_INET, server_address, &server.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address.\n");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Pack DNS query message
    uint8_t dns_buffer[DNS_MAX_MESSAGE_SIZE];
    memset(dns_buffer, 0, DNS_MAX_MESSAGE_SIZE);
    int dns_buffer_len = 0;
    pack_dns_message(domain, query_type, DNS_CLASS_IN, dns_buffer,
                     &dns_buffer_len);

    // Send DNS query
    if (send(sockfd, dns_buffer, dns_buffer_len, 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void tcp_recv(int sockfd, int *response_len, uint8_t *response_buffer) {
    // Receive DNS response
    *response_len = recv(sockfd, response_buffer, DNS_MAX_MESSAGE_SIZE, 0);
    if (*response_len < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }
}