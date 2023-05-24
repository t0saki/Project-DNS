#include "server.h"

const int MAX_TIER = 6;
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
            DNS_PORT = port;
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

    create_log_file_server();

    // Start local DNS server thread
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, root_server, (void *)server_address);

    // Start local DNS server thread
    pthread_t tier_server_threads[MAX_TIER];
    for (int i = 0; i < MAX_TIER; i++) {
        // int *arg = (int *)malloc(sizeof(*arg));
        // *arg = i;
        pthread_create(&tier_server_threads[i], NULL, tier_server, (void *)i);
    }

    while (1) {
        sleep(10000);
    }

    return 0;
}

void *root_server(void *args) {
    char *server_address = (char *)args;

    char *msg = (char *)malloc(100);
    sprintf(msg, "Starting root DNS server at %s:%d\n", server_address,
            DNS_PORT);
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

        // Parse the DNS query
        dns_question question = {0};
        parse_dns_message((uint8_t *)buffer, len, (dns_rr *)NULL, (int *)NULL,
                          &question);
        msg = (char *)malloc(100);
        sprintf(msg, "Received DNS query: %s\n", question.qname);
        write_log(msg);

        char *dot_name = add_dot(question.qname);

        // // Add a dot to the end of the domain name
        // question.qname = add_dot(question.qname);

        // // Pack to new buffer to add a dot
        // uint8_t dns_buffer[DNS_MAX_MESSAGE_SIZE];
        // memset(dns_buffer, 0, DNS_MAX_MESSAGE_SIZE);
        // int dns_buffer_len = 0;
        // pack_dns_message(question.qname, question.qtype, DNS_CLASS_IN,
        //                  dns_buffer, &dns_buffer_len);

        // buffer = (char *)malloc(1024);
        // memcpy(buffer, dns_buffer, dns_buffer_len);

        char *next_dns_server_address = TIER_SERVERS[0];
        while (1) {
            // Send the DNS query to the next DNS server with TCP
            int sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_tcp < 0) {
                write_log("Failed to create socket.\n");
                exit(EXIT_FAILURE);
            }

            struct sockaddr_in server_addr_tcp = {0};
            server_addr_tcp.sin_family = AF_INET;
            server_addr_tcp.sin_port = htons(DNS_PORT);
            server_addr_tcp.sin_addr.s_addr =
                inet_addr(next_dns_server_address);

            if (connect(sockfd_tcp, (struct sockaddr *)&server_addr_tcp,
                        sizeof(server_addr_tcp)) < 0) {
                char *msg = (char *)malloc(100);
                sprintf(msg, "Failed to connect to %s:%d.\n",
                        next_dns_server_address, DNS_PORT);
                write_log(msg);
                exit(EXIT_FAILURE);
            }

            // // Add 2 bytes of length to the beginning of the buffer
            // uint16_t len = htons(len);
            // memcpy(buffer, &len, 2);

            // Send the DNS query
            if (send(sockfd_tcp, buffer, len, 0) < 0) {
                write_log("Failed to send DNS query.\n");
                exit(EXIT_FAILURE);
            }

            // Receive the DNS response
            char *buffer_tcp = (char *)malloc(1024);
            int len_tcp = recv(sockfd_tcp, buffer_tcp, 1024, 0);
            if (len_tcp < 0) {
                write_log("Failed to receive DNS response.\n");
                exit(EXIT_FAILURE);
            }

            // // Delete the first 2 bytes of length
            // len_tcp -= 2;
            // memmove(buffer_tcp, buffer_tcp + 2, len_tcp);

            // Parse the DNS response
            dns_rr *answers = (dns_rr *)malloc(100 * sizeof(dns_rr));
            int answer_count = 0;
            dns_question question = {0};
            parse_dns_message((uint8_t *)buffer_tcp, len_tcp, answers,
                              &answer_count, &question);

            // Get the next DNS server address
            if (answer_count > 0) {
                next_dns_server_address = answers[0].rdata;
            }

            // If totally same as the query, then break
            answers[0].name[1] = question.qname[0];
            if (strcmp(dot_name, answers[0].name) == 0) {
                printf("RData: %s\n", answers[0].rdata);
                msg = (char *)malloc(100);
                sprintf(msg, "Found the IP address: %s\n", answers[0].rdata);
                write_log(msg);
                break;
            }

            printf("Next iter\n");
        }
    }
    // Close the socket
    close(sockfd);

    while (1) {
        sleep(10000);
    }
}

void *tier_server(void *args) {
    int tier = (long long)args;
    char *server_address = TIER_SERVERS[tier];
    char *rr_file = TIER_RR_FILES[tier];

    char *msg = (char *)malloc(100);
    sprintf(msg, "Starting DNS server at %s:%d\n", server_address, DNS_PORT);
    write_log(msg);

    // Read the resource records from the file,dns_rr **read_rr_all(char
    // *filename, int *countr)
    int rr_count = 0;
    dns_rr **rrs = read_rr_all(rr_file, &rr_count);

    // Start receiving TCP DNS queries
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    printf("Started listening at %s:%d\n", server_address, DNS_PORT);

    if (listen(sockfd, 5) < 0) {
        write_log("Failed to listen.\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accept a TCP connection
        struct sockaddr_in client_addr = {0};
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sockfd =
            accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sockfd < 0) {
            write_log("Failed to accept connection.\n");
            exit(EXIT_FAILURE);
        }

        // Receive the DNS query
        char *buffer = (char *)malloc(1024);
        int len = recv(client_sockfd, buffer, 1024, 0);
        if (len < 0) {
            write_log("Failed to receive DNS query.\n");
            exit(EXIT_FAILURE);
        }

        // // Delete the first 2 bytes of length
        // len -= 2;
        // memmove(buffer, buffer + 2, len);

        printf("Received DNS query: %d\n", len);

        // Parse the DNS query
        dns_question question = {0};

        parse_dns_message((uint8_t *)buffer, (int)len, (dns_rr *)NULL,
                          (int *)NULL, (dns_question *)&question);

        // Find the resource record
        // dns_rr *rr = find_rr(rrs, rr_count, add_dot(question.qname),
        //                      question.qtype);

        char *query_domain = add_dot(question.qname);

        int rr_response_index =
            find_rr(rrs, rr_count, query_domain, question.qtype);

        dns_rr rr_response = {0};

        // If has full A record or CNAME
        if (rr_response_index == -1) {
            rr_response_index =
                find_rr(rrs, rr_count, query_domain, DNS_TYPE_A);
            if (rr_response_index != -1)
                rr_response = *rrs[rr_response_index];
        }

        if (rr_response_index == -1) {
            rr_response_index =
                find_rr(rrs, rr_count, query_domain, DNS_TYPE_CNAME);
            // If found CNAME
            if (rr_response_index != -1) {
                // Find the A record for the CNAME
                int cname_a_rr_index = find_rr(
                    rrs, rr_count, rrs[rr_response_index]->rdata, DNS_TYPE_A);
                rr_response = *rrs[rr_response_index];
                rr_response.type = DNS_TYPE_A;
                if (cname_a_rr_index != -1)
                    rr_response.rdata = rrs[cname_a_rr_index]->rdata;
            }
        }

        if (rr_response_index == -1) {
            // If not found, find the NS record
            // Find ns first
            int ns_rr_index = find_rr(rrs, rr_count, query_domain, DNS_TYPE_NS);

            char *ns_domain = rrs[ns_rr_index]->rdata;

            // Find the A record for the NS
            int ns_a_rr_index = find_rr(rrs, rr_count, ns_domain, DNS_TYPE_A);

            rr_response = *rrs[ns_a_rr_index];
            rr_response.name = rrs[ns_rr_index]->name;
            rr_response.type = DNS_TYPE_A;
            rr_response.rdata = rrs[ns_a_rr_index]->rdata;
        }

        // Create the DNS response
        uint8_t response[1024];
        int response_len = 0;
        pack_dns_response(&question, &rr_response, response, &response_len);

        // // Add 2 bytes of length to the beginning of the buffer
        // uint16_t len2 = htons(response_len);
        // memcpy(buffer, &len2, 2);

        // Send the DNS response
        if (send(client_sockfd, (char *)response, response_len, 0) < 0) {
            write_log("Failed to send DNS response.\n");
            exit(EXIT_FAILURE);
        }

        // Close the client socket
        close(client_sockfd);
    }

    while (1) {
        sleep(10000);
    }
}