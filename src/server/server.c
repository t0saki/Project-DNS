#include "server.h"

const int MAX_TIER = 6;
char *TIER_SERVERS[] = {"127.1.1.1", "127.2.2.1", "127.3.3.1",
                        "127.4.4.1", "127.5.5.1", "127.6.6.1"};

int main(int argc, char *argv[]) {
    char *server_address = TIER_SERVERS[0];
    uint16_t port = 53;

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "s:p:")) != -1) {
        switch (opt) {
        case 's':
            server_address = strtrim(optarg);
            break;
        case 'p':
            port = atoi(strtrim(optarg));
            break;
        default:
            fprintf(stderr, "Invalid arguments.\n");
            exit(EXIT_FAILURE);
        }
    }

    // // Check if all required arguments are provided
    // if (server_address == NULL) {
    //     fprintf(stderr, "Missing arguments.\n");
    //     exit(EXIT_FAILURE);
    // }

    create_log_file();

    // Start local DNS server thread
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, start_server, (void *)server_address);

    for (int i = 0; i < MAX_TIER; i++) {
        // Start tier 1 DNS server thread
        pthread_t tier1_thread;
        pthread_create(&tier1_thread, NULL, server,
                       (void *)TIER_SERVERS[i]);
    }

    return 0;
}

void *start_server(void *args) {
    printf("Starting server\n");
    char *server_address = (char *)args;

    char *msg = (char *)malloc(100);
    sprintf(msg, "Starting DNS server at %s:%d\n", server_address, 53);
    write_log(msg);

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        write_log("Failed to create socket.\n");
        exit(EXIT_FAILURE);
    }

    // Bind to the specified port
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);
    server_addr.sin_addr.s_addr = inet_addr(server_address);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        char *msg = (char *)malloc(100);
        sprintf(msg, "Failed to bind to port %d.\n", 53);
        write_log(msg);
        exit(EXIT_FAILURE);
    }

    msg = (char *)malloc(100);
    sprintf(msg, "Started DNS server at %s:%d\n", server_address, 53);
    write_log(msg);

    while (1) {
        // Receive DNS query
        uint8_t query_buffer[DNS_MAX_MESSAGE_SIZE];
        int query_len = 0;
        struct sockaddr_in client_addr = {0};
        int client_addr_len = sizeof(client_addr);
        query_len = recvfrom(sockfd, query_buffer, sizeof(query_buffer), 0,
                             (struct sockaddr *)&client_addr,
                             (socklen_t *)&client_addr_len);
        if (query_len < 0) {
            write_log("Failed to receive DNS query.\n");
            exit(EXIT_FAILURE);
        }

        // Parse DNS query
        // dns_rr response[128] = {0};
        // int response_count;

        // parse_dns_message(response_buffer, response_len, (dns_rr *)&response,
        //                   &response_count);

        // char *ns_ips[128] = {0};
        // int ns_count = 0;
        // solve_answers(domain, response, response_count, ns_ips, &ns_count);
        // for (int i = 0; i < ns_count; i++) {
        //     printf("%s\n", ns_ips[i]);
        // }

        // Send DNS response
        uint8_t response_buffer[DNS_MAX_MESSAGE_SIZE];
        int response_len = 0;
        // udp_send(sockfd, client_addr, response_buffer, response_len);
    }
    // Close the socket
    close(sockfd);
}

void *server(void *args) {
    int i = 0;
}