// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <getopt.h>
#include "server.h"
#include "websocket.h"
#include "http_handler.h"
#include "metrics.h"

// Inizializzazione della configurazione con valori predefiniti
ServerConfig server_config = {
    .port = DEFAULT_PORT,
    .max_clients = DEFAULT_MAX_CLIENTS,
    .buffer_size = DEFAULT_BUFFER_SIZE,
    .www_root = DEFAULT_WWW_ROOT,
    .verbose = false
};

static int server_socket;
static int* client_sockets;
static int num_clients = 0;
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Callback per l'aggiornamento delle metriche
void metrics_updated_callback(const Metrics* metrics) {
    // Prepara il messaggio JSON
    char message[1024] = "{";
    char* p = message + 1;
    
    for (int i = 0; i < metrics->count; i++) {
        // Aggiungi virgola se non è il primo elemento
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        
        // Aggiungi "nome": valore
        p += snprintf(p, sizeof(message) - (p - message), 
                     "\"%s\": %d", 
                     metrics->metrics[i].name, metrics->metrics[i].value);
    }
    
    // Chiudi il JSON
    *p++ = '}';
    *p = '\0';
    
    // Invia l'aggiornamento a tutti i client
    broadcast_metrics(message);
}


// Aggiunge un client alla lista
static void add_client(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    if (num_clients < server_config.max_clients) {
        client_sockets[num_clients++] = client_socket;
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Rimuove un client dalla lista
static void remove_client(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (client_sockets[i] == client_socket) {
            // Sposta gli altri client indietro di una posizione
            for (int j = i; j < num_clients - 1; j++) {
                client_sockets[j] = client_sockets[j + 1];
            }
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Funzione per gestire ogni client in un thread separato
static void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    char* buffer = malloc(server_config.buffer_size);
    if (!buffer) {
        close(client_socket);
        return NULL;
    }
    
    // Leggi la richiesta iniziale
    ssize_t bytes_read = recv(client_socket, buffer, server_config.buffer_size - 1, 0);
    if (bytes_read <= 0) {
        free(buffer);
        close(client_socket);
        return NULL;
    }
    buffer[bytes_read] = '\0';

    // Controlla se è una richiesta WebSocket
    if (strstr(buffer, "Upgrade: websocket") != NULL) {
        if (server_config.verbose) {
            printf("Richiesta WebSocket ricevuta\n");
        }
        
        int handshake_result = handle_websocket_handshake(client_socket, buffer);
        
        if (handshake_result >= 0) {
            if (server_config.verbose) {
                printf("Client %d connesso via WebSocket\n", client_socket);
            }
            add_client(client_socket);
            
            // Invia subito le metriche correnti al nuovo client
            Metrics current;
            metrics_get(&current);

            // Crea il messaggio JSON
            char init_message[1024] = "{";
            char* p = init_message + 1;

            for (int i = 0; i < current.count; i++) {
                // Aggiungi virgola se non è il primo elemento
                if (i > 0) {
                    *p++ = ',';
                    *p++ = ' ';
                }
    
                // Aggiungi "nome": valore
                p += snprintf(p, sizeof(init_message) - (p - init_message), 
                             "\"%s\": %d", 
                            current.metrics[i].name, current.metrics[i].value);
            }

            // Chiudi il JSON
            *p++ = '}';
            *p = '\0';
            
            send_websocket_frame(client_socket, init_message, strlen(init_message));
            
            // Loop principale per il client WebSocket
            unsigned char* ws_buffer = malloc(server_config.buffer_size);
            if (ws_buffer) {
                while ((bytes_read = recv(client_socket, ws_buffer, server_config.buffer_size, 0)) > 0) {
                    handle_websocket_frame(client_socket, ws_buffer, bytes_read);
                }
                free(ws_buffer);
            }
            
            if (server_config.verbose) {
                printf("Client %d disconnesso\n", client_socket);
            }
            remove_client(client_socket);
        }
    } else {
        // Gestisci come normale richiesta HTTP
        handle_http_request(client_socket, buffer);
    }

    free(buffer);
    close(client_socket);
    return NULL;
}

void broadcast_to_clients(const char* message) {
    pthread_mutex_lock(&clients_mutex);
    if (server_config.verbose) {
        printf("Broadcasting to %d clients\n", num_clients);
    }
    
    for (int i = 0; i < num_clients; i++) {
        int result = send_websocket_frame(client_sockets[i], message, strlen(message));
        if (result < 0 && server_config.verbose) {
            printf("Errore nell'invio al client %d\n", client_sockets[i]);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}


void update_metrics(int value1, int value2) {
    // Utilizza il modulo metrics per aggiornare i valori
    metrics_update(value1, value2);
}

// Funzione principale del server
void* start_server(void* arg) {

    // Alloca l'array dei client
    client_sockets = malloc(server_config.max_clients * sizeof(int));
    if (!client_sockets) {
        perror("Errore nell'allocazione della memoria per i client");
        exit(1);
    }

    // Registra il callback per le metriche
    metrics_register_callback(metrics_updated_callback);

    struct sockaddr_in server_addr;
    
    // Crea il socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Errore nella creazione del socket");
        exit(1);
    }
    
    // Imposta l'opzione di riutilizzo dell'indirizzo
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Configura l'indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_config.port);
    
    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel binding");
        exit(1);
    }
    
    // Listen
    if (listen(server_socket, 5) < 0) {
        perror("Errore nella listen");
        exit(1);
    }
    
    printf("Server in ascolto sulla porta %d\n", server_config.port);
    printf("Servendo file da: %s\n", server_config.www_root);
    printf("Modalità verbose: %s\n", server_config.verbose ? "attiva" : "disattiva");
    
    // Loop principale del server
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Accetta nuove connessioni
        int* client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (*client_socket < 0) {
            perror("Errore nell'accept");
            free(client_socket);
            continue;
        }
        
        if (server_config.verbose) {
            printf("Nuova connessione accettata: socket %d\n", *client_socket);
        }
        
        // Crea un nuovo thread per gestire il client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_socket) != 0) {
            perror("Errore nella creazione del thread");
            free(client_socket);
            close(*client_socket);
            continue;
        }
        pthread_detach(thread);
    }
    
    // Pulizia (questo codice non viene mai raggiunto in questa implementazione)
    free(client_sockets);
    return NULL;
}

