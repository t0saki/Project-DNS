#include "client.h"

int main(int argc, char *argv[]) {
    char *server_address = NULL;
    char *domain = NULL;
    uint16_t port = DNS_PORT;
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
            DNS_PORT = port;
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

    // Send DNS query
    int sockfd = udp_send(server_address, domain, port, query_type);

    printf("Sent DNS query to %s:%d\n", server_address, port);

    // Receive DNS response
    uint8_t response_buffer[DNS_MAX_MESSAGE_SIZE];
    int response_len = 0;
    udp_recv(sockfd, &response_len, response_buffer);

    // Parse DNS response
    dns_rr response[128] = {0};
    int response_count;
    dns_question question;

    parse_dns_message(response_buffer, response_len, (dns_rr *)&response, &response_count, &question);

    printf("IP addresses:\n");
    char* ns_ips[128] = {0};
    int ns_count = 0;
    solve_answers(domain, response, response_count, ns_ips, &ns_count);
    for (int i = 0; i < ns_count; i++) {
        printf("%s\n", ns_ips[i]);
    }

    // Close the socket
    close(sockfd);

    // Write log
    create_log_file_client();
    char *msg = (char *)malloc(100);
    sprintf(msg, "Sent DNS query to %s:%d, received answer: %s\n", server_address, port, ns_ips[0]);
    write_log(msg);

    return 0;
}
