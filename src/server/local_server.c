#include "local_server.h"

void* local_server(void* args) {
    char* server_address = (char*)args;
    dns_cache* cache = (dns_cache*)malloc(sizeof(dns_cache)*1000);
    const int cache_size_window = 996;
    int cache_size = 0;

    char* msg = (char*)malloc(1000);
    sprintf(msg, "Starting root DNS server at %s:%d\n", server_address,
        DNS_PORT);
    write_log(msg);

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd<0) {
        write_log("Failed to create socket.\n");
        exit(EXIT_FAILURE);
    }

    // Bind to the specified port
    struct sockaddr_in server_addr = { 0 };
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    server_addr.sin_addr.s_addr = inet_addr(LOCAL_SERVER);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr))<
        0) {
        char* msg = (char*)malloc(1000);
        sprintf(msg, "Failed to bind to port %d.\n", DNS_PORT);
        write_log(msg);
        exit(EXIT_FAILURE);
    }

    int count = 0;

    while (1) {
        char* msg = (char*)malloc(1000);
        sprintf(msg, "Waiting for DNS query %d\n", count++);
        write_log(msg);

        // Receive a DNS query
        struct sockaddr_in client_addr = { 0 };
        socklen_t client_addr_len = sizeof(client_addr);
        char* buffer = (char*)malloc(1024);
        int len = recvfrom(sockfd, buffer, 1024, 0,
            (struct sockaddr*)&client_addr, &client_addr_len);
        if (len<0) {
            write_log("Failed to receive DNS query.\n");
            exit(EXIT_FAILURE);
        }

        // Parse the DNS query
        dns_question question = { 0 };
        parse_dns_message((uint8_t*)buffer, len, (dns_rr*)NULL, (int*)NULL,
            &question);
        msg = (char*)malloc(1000);
        sprintf(msg, "Received DNS query: %s\n", question.qname);
        write_log(msg);

        char* dot_name = add_dot(question.qname);
        dns_question questions = { 0 };
        memset(&questions, 0, sizeof(questions));
        dns_rr final_answer = { 0 };
        memset(&final_answer, 0, sizeof(final_answer));

        // query cache
        int found = 0;
        for (int i = cache_size-1; i>=0; i--) {
            if (strcmp(cache[i%cache_size_window].question->qname,
                question.qname)==0&&
                cache[i%cache_size_window].question->qtype==
                question.qtype) {
                memcpy(&questions, cache[i%cache_size_window].question,
                    sizeof(questions));

                final_answer.name = (char*)malloc(1000);
                strcpy(final_answer.name, cache[i%cache_size_window].answer->name);
                final_answer.type = question.qtype;
                final_answer.classt = DNS_CLASS_IN;
                final_answer.ttl = 0;
                final_answer.rdlength = strlen(cache[i%cache_size_window].answer->rdata);
                final_answer.rdata = (char*)malloc(1000);
                strcpy(final_answer.rdata, cache[i%cache_size_window].answer->rdata);


                msg = (char*)malloc(1000);
                sprintf(msg, "Found in cache: %s\n", question.qname);
                write_log(msg);

                found = 1;
                break;
            }
        }

        // if not in cache
        if (!found) {
            char* next_dns_server_address = TIER_SERVERS[0];
            while (1) {
                memset(&questions, 0, sizeof(questions));
                memset(&final_answer, 0, sizeof(final_answer));
                // Send the DNS query to the next DNS server with TCP
                int sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd_tcp<0) {
                    write_log("Failed to create socket.\n");
                    exit(EXIT_FAILURE);
                }

                struct sockaddr_in server_addr_tcp = { 0 };
                server_addr_tcp.sin_family = AF_INET;
                server_addr_tcp.sin_port = htons(DNS_PORT);
                server_addr_tcp.sin_addr.s_addr =
                    inet_addr(next_dns_server_address);

                if (connect(sockfd_tcp, (struct sockaddr*)&server_addr_tcp,
                    sizeof(server_addr_tcp))<0) {
                    char* msg = (char*)malloc(1000);
                    sprintf(msg, "Failed to connect to %s:%d.\n",
                        next_dns_server_address, DNS_PORT);
                    write_log(msg);
                    exit(EXIT_FAILURE);
                }

                // // Add 2 bytes of length to the beginning of the buffer
                // uint16_t len = htons(len);
                // memcpy(buffer, &len, 2);

                // Send the DNS query
                if (send(sockfd_tcp, udp2tcp(buffer, len), len+2, 0)<0) {
                    write_log("Failed to send DNS query.\n");
                    exit(EXIT_FAILURE);
                }

                // Receive the DNS response
                char* buffer_tcp = (char*)malloc(1024);
                int len_tcp = recv(sockfd_tcp, buffer_tcp, 1024, 0);
                if (len_tcp<0) {
                    write_log("Failed to receive DNS response.\n");
                    exit(EXIT_FAILURE);
                }

                // Parse the DNS response
                dns_rr* answers = (dns_rr*)malloc(10000*sizeof(dns_rr));
                int answer_count = 0;
                parse_dns_message((uint8_t*)tcp2udp(buffer_tcp, len_tcp),
                    len_tcp, answers, &answer_count, &questions);

                // Get the next DNS server address
                if (answer_count>0) {
                    next_dns_server_address = answers[0].rdata;
                }

                // If totally same as the query, or get A record from A and
                // CNAME
                if ((strcmp(question.qname, answers[0].name)==0)) {
                    if ((answers[0].type==DNS_TYPE_A&&
                        question.qtype==DNS_TYPE_A)||
                        (answers[0].type==DNS_TYPE_A&&
                            question.qtype==DNS_TYPE_CNAME)) {
                        printf("RData: %s\n", answers[0].rdata);
                        msg = (char*)malloc(1000);
                        sprintf(msg, "Found the IP address: %s\n",
                            answers[0].rdata);
                        write_log(msg);

                        // Send the DNS response to the client
                        final_answer.name = question.qname;
                        final_answer.type = question.qtype;
                        final_answer.classt = DNS_CLASS_IN;
                        final_answer.ttl = 0;
                        final_answer.rdlength = strlen(answers[0].rdata);
                        final_answer.rdata = answers[0].rdata;

                        break;
                    } else if (question.qtype==DNS_TYPE_MX) {
                        printf("RData: %s\n", answers[0].rdata);
                        msg = (char*)malloc(1000);
                        sprintf(msg, "Found the MX record: %s\n",
                            answers[0].rdata);
                        write_log(msg);

                        // Send the DNS response to the client
                        final_answer.name = question.qname;
                        final_answer.type = question.qtype;
                        final_answer.classt = DNS_CLASS_IN;
                        final_answer.ttl = 0;
                        final_answer.rdlength = strlen(answers[0].rdata);
                        final_answer.rdata = answers[0].rdata;

                        break;
                    } else if (answers[0].type==DNS_TYPE_CNAME) {
                        printf("RData: %s\n", answers[0].rdata);
                        msg = (char*)malloc(1000);
                        sprintf(msg, "Found the CNAME record: %s\n",
                            answers[0].rdata);
                        write_log(msg);

                        // Send the DNS response to the client
                        final_answer.name = question.qname;
                        final_answer.type = question.qtype;
                        final_answer.classt = DNS_CLASS_IN;
                        final_answer.ttl = 0;
                        final_answer.rdlength = strlen(answers[0].rdata);
                        final_answer.rdata = answers[0].rdata;

                        break;
                    } else if (answers[0].type==DNS_TYPE_PTR) {
                        printf("RData: %s\n", answers[0].rdata);
                        msg = (char*)malloc(1000);
                        sprintf(msg, "Found the PTR record: %s\n",
                            answers[0].rdata);
                        write_log(msg);

                        // Send the DNS response to the client
                        final_answer.name = question.qname;
                        final_answer.type = question.qtype;
                        final_answer.classt = DNS_CLASS_IN;
                        final_answer.ttl = 0;
                        final_answer.rdlength = strlen(answers[0].rdata);
                        final_answer.rdata = answers[0].rdata;

                        break;
                    }
                }

                printf("Next iter\n");
            }

            // Add to the cache
            cache_size++;
            int i = cache_size-1;
            cache[i%cache_size_window].answer =
                (dns_rr*)malloc(sizeof(dns_rr));
            cache[i%cache_size_window].question =
                (dns_question*)malloc(sizeof(dns_question));
            cache[i%cache_size_window].answer->name = (char*)malloc(1000);
            strcpy(cache[i%cache_size_window].answer->name, final_answer.name);
            cache[i%cache_size_window].answer->type = question.qtype;
            cache[i%cache_size_window].answer->classt = DNS_CLASS_IN;
            cache[i%cache_size_window].answer->ttl = 0;
            cache[i%cache_size_window].answer->rdlength =
                strlen(final_answer.rdata);
            cache[i%cache_size_window].answer->rdata = (char*)malloc(1000);
            strcpy(cache[i%cache_size_window].answer->rdata,
                final_answer.rdata);

            cache[i%cache_size_window].question->qname = question.qname;
            cache[i%cache_size_window].question->qtype = question.qtype;
            cache[i%cache_size_window].question->qclass = DNS_CLASS_IN;
            cache[i%cache_size_window].question->qname_len =
                question.qname_len;

            msg = (char*)malloc(1000);
            sprintf(msg, "Added to the cache: %s\n", question.qname);
            write_log(msg);
        }

        uint8_t dns_buffer[DNS_MAX_MESSAGE_SIZE];
        memset(dns_buffer, 0, DNS_MAX_MESSAGE_SIZE);
        int dns_buffer_len = 0;
        pack_dns_response(&questions, &final_answer, dns_buffer,
            &dns_buffer_len);

        if (sendto(sockfd, dns_buffer, dns_buffer_len, 0,
            (struct sockaddr*)&client_addr, sizeof(client_addr))<0) {
            write_log("Failed to send DNS response.\n");
            exit(EXIT_FAILURE);
        }

        printf("Sent the DNS response to the client.\n");
    }
    // Close the socket
    close(sockfd);

    while (1) {
        sleep(1000000);
    }
}