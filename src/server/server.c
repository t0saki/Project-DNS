#include "server.h"

const int MAX_TIER = 4;
char *ROOT_SERVER = "127.1.1.1";
char *TIER_SERVERS[] = {"127.2.2.1", "127.3.3.1", "127.4.4.1",
                        "127.5.5.1", "127.6.6.1", "127.7.7.1"};
char *TIER_RR_FILES[] = {"data/rr1.txt", "data/rr2.txt", "data/rr3.txt",
                         "data/rr4.txt", "data/rr5.txt", "data/rr6.txt"};

int main(int argc, char *argv[]) {
    char *server_address = ROOT_SERVER;
    uint16_t port = DNS_PORT;

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

    // for (int i = 0; i < MAX_TIER; i++) {
    //     // Start tier 1 DNS server thread
    //     pthread_t tier1_thread;
    //     pthread_create(&tier1_thread, NULL, server, (void *)TIER_SERVERS[i]);
    // }

    while (1) {
        sleep(10000);
    }

    return 0;
}

void *start_server(void *args) {
    printf("Starting server\n");
    char *server_address = (char *)args;

    char *msg = (char *)malloc(100);
    sprintf(msg, "Starting DNS server at %s:%d\n", server_address, DNS_PORT);
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
    server_addr.sin_port = htons(DNS_PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_address);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        char *msg = (char *)malloc(100);
        sprintf(msg, "Failed to bind to port %d.\n", DNS_PORT);
        write_log(msg);
        exit(EXIT_FAILURE);
    }

    msg = (char *)malloc(100);
    sprintf(msg, "Started DNS server at %s:%d\n", server_address, DNS_PORT);
    write_log(msg);

    int count = 0;

    while (1) {
        char *msg = (char *)malloc(100);
        sprintf(msg, "Waiting for DNS query %d\n", count++);
        write_log(msg);

        // Receive a DNS query
        struct sockaddr_in client_addr = {0};
        socklen_t client_addr_len = sizeof(client_addr);
        char *buffer = (char *)malloc(1024);
        int len = recvfrom(sockfd, buffer, 1024, 0,
                           (struct sockaddr *)&client_addr, &client_addr_len);
        if (len < 0) {
            write_log("Failed to receive DNS query.\n");
            exit(EXIT_FAILURE);
        }

        printf("Received DNS query: %d\n", count);
    }
    // Close the socket
    close(sockfd);
}

void *server(void *args) {
    int i = 0;
    return 0;
}