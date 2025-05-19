// server.h
#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include "metrics.h"


// Configurazione predefinita
#define DEFAULT_MAX_CLIENTS 10
#define DEFAULT_PORT 8080
#define DEFAULT_BUFFER_SIZE 4096
#define DEFAULT_WWW_ROOT "./www"

// Struttura di configurazione del server
typedef struct {
    int port;
    int max_clients;
    int buffer_size;
    char www_root[256];
    bool verbose;
} ServerConfig;

// Variabili globali per la configurazione
extern ServerConfig server_config;

// Funzioni principali
void* start_server(void* arg);
void update_metrics(int value1, int value2);
void parse_command_line(int argc, char* argv[]);
void metrics_updated_callback(const Metrics* metrics);

#endif
