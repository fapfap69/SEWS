// main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include "server.h"

static volatile int running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        running = 0;
    }
}

// Funzione per il parsing dei parametri da riga di comando
void parse_command_line(int argc, char* argv[]) {
    int opt;
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"max-clients", required_argument, 0, 'c'},
        {"buffer-size", required_argument, 0, 'b'},
        {"www-root", required_argument, 0, 'w'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "p:c:b:w:vh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                server_config.port = atoi(optarg);
                break;
            case 'c':
                server_config.max_clients = atoi(optarg);
                break;
            case 'b':
                server_config.buffer_size = atoi(optarg);
                break;
            case 'w':
                strncpy(server_config.www_root, optarg, sizeof(server_config.www_root) - 1);
                server_config.www_root[sizeof(server_config.www_root) - 1] = '\0';
                break;
            case 'v':
                server_config.verbose = true;
                break;
            case 'h':
                printf("Uso: %s [OPZIONI]\n", argv[0]);
                printf("Opzioni:\n");
                printf("  -p, --port=PORTA           Porta su cui ascoltare (default: %d)\n", DEFAULT_PORT);
                printf("  -c, --max-clients=NUM      Numero massimo di client (default: %d)\n", DEFAULT_MAX_CLIENTS);
                printf("  -b, --buffer-size=SIZE     Dimensione del buffer (default: %d)\n", DEFAULT_BUFFER_SIZE);
                printf("  -w, --www-root=PATH        Directory radice per i file statici (default: %s)\n", DEFAULT_WWW_ROOT);
                printf("  -v, --verbose              Abilita i messaggi di log dettagliati\n");
                printf("  -h, --help                 Mostra questo messaggio di aiuto\n");
                exit(0);
                break;
            default:
                fprintf(stderr, "Uso: %s -h per l'aiuto\n", argv[0]);
                exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parsing dei parametri da riga di comando
    parse_command_line(argc, argv);
    
    printf("Starting SEWS (Simple Embedded Web Server)\n");
    
    // Avvia il server in un thread separato
    pthread_t server_thread;
    if (pthread_create(&server_thread, NULL, start_server, NULL) != 0) {
        perror("Errore nella creazione del thread del server");
        exit(1);
    }
    
    // Attendi un momento per permettere al server di avviarsi
    sleep(1);
    
    // Loop principale per l'aggiornamento delle metriche
    int counter1 = 0;
    int counter2 = 100;
    
    while (running) {
        // Simula l'aggiornamento delle metriche
        counter1 = (counter1 + 1) % 1000;
        counter2 = 100 + (counter1 % 50);
        
        // Aggiorna e invia le metriche
        update_metrics(counter1, counter2);
        if (server_config.verbose) {
            printf("Metriche aggiornate: value1=%d, value2=%d\n", counter1, counter2);
        }
        
        // Attendi un secondo prima del prossimo aggiornamento
        sleep(1);
    }
    
    printf("\nShutting down...\n");
    return 0;
}
