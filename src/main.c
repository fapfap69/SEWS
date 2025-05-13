#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "http_server.h"
#include "websocket.h"

#define DEFAULT_PORT 8080

volatile sig_atomic_t keep_running = 1;

void signal_handler(int signum) {
    (void)signum;
    keep_running = 0;
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    
    // Handle command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Setup signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Starting SEWS on port %d...\n", port);
    
    // Initialize server
    if (init_server(port) != 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }

    // Main server loop
    while (keep_running) {
        handle_connections();
    }

    // Cleanup
    cleanup_server();
    printf("\nServer shutdown complete\n");
    
    return 0;
}
