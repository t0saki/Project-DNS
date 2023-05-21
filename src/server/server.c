#include "server.h"

const int MAX_TIER = 5;
char *TIER1_SERVERS[] = {"127.2.2.1", "127.3.3.1", "127.4.4.1", "127.5.5.1", "127.6.6.1");

int main(int argc, char *argv[]) {
    char *server_address = "127.1.1.1";
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

    // Start local DNS server thread
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, start_server, (void *)server_address);

    for (int i = 0; i < MAX_TIER; i++) {
        // Start tier 1 DNS server thread
        pthread_t tier1_thread;
        pthread_create(&tier1_thread, NULL, start_server, (void *)TIER1_SERVERS[i]);
    }


    return 0;
}

void *start_server(void *args) {
    char *server_address = (char *)args;

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // Bind to the specified port
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(53);
    server_addr.sin_addr.s_addr = inet_addr(server_address);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    printf("Listening on %s\n", server_address);

    while (1) {
        // Receive DNS query
        uint8_t query_buffer[DNS_MAX_MESSAGE_SIZE];
        int query_len = 0;
        struct sockaddr_in client_addr = {0};
        int client_addr_len = sizeof(client_addr);
        query_len = recvfrom(sockfd, query_buffer, sizeof(query_buffer), 0, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
        if (query_len < 0) {
            perror("recvfrom()");
            exit(EXIT_FAILURE);
        }

        // Parse DNS query
        parse_dns_message(query_buffer, query_len);

        // Send DNS response
        uint8_t response_buffer[DNS_MAX_MESSAGE_SIZE];
        int response_len = 0;
        udp_sendto(sockfd, (struct sockaddr *)&client_addr, client_addr_len, &response_len, response_buffer);
    }
}

void *server(void *args)