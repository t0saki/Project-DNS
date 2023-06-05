#include "server.h"

int main(int argc, char* argv[]) {
    char* server_address = ROOT_SERVER;
    uint16_t port = DNS_PORT;

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "s:p:"))!=-1) {
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
    pthread_create(&server_thread, NULL, local_server, (void*)server_address);

    // Start local DNS server thread
    pthread_t tier_server_threads[MAX_TIER];
    for (int i = 0; i<MAX_TIER; i++) {
        // int *arg = (int *)malloc(sizeof(*arg));
        // *arg = i;
        pthread_create(&tier_server_threads[i], NULL, tier_server, (void*)i);
    }

    while (1) {
        sleep(10000);
    }

    return 0;
}