#include "../utils/dns_message.c"
#include "../utils/socket.c"
#include "../utils/utils.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *server_address = NULL;
    char *domain = NULL;
    uint16_t port = 53;
    uint16_t query_type = 0;

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "s:d:p:t:")) != -1) {
        switch (opt) {
        case 's':
            server_address = strtrim(optarg);
            break;
        case 'd':
            domain = strtrim(optarg);
            break;
        case 'p':
            port = atoi(strtrim(optarg));
            break;
        case 't':
            if (strcmp(strtrim(optarg), "a") == 0) {
                query_type = DNS_TYPE_A;
            } else if (strcmp(strtrim(optarg), "mx") == 0) {
                query_type = DNS_TYPE_MX;
            } else if (strcmp(strtrim(optarg), "cname") == 0) {
                query_type = DNS_TYPE_CNAME;
            }
            break;
        default:
            fprintf(stderr, "Invalid arguments.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Check if all required arguments are provided
    if (server_address == NULL || domain == NULL || query_type == 0) {
        fprintf(stderr, "Missing arguments.\n");
        exit(EXIT_FAILURE);
    }

    int sockfd = udp_send(server_address, domain, port, query_type);

    printf("Sent DNS query to %s:%d\n", server_address, port);

    // Receive DNS response
    uint8_t response_buffer[DNS_MAX_MESSAGE_SIZE];
    int response_len = 0;
    udp_recv(sockfd, &response_len, response_buffer);

    // Parse DNS response
    parse_dns_message(response_buffer, response_len);

    // Close the socket
    close(sockfd);

    return 0;
}
