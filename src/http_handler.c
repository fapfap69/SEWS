// http_handler.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>     // Per send()
#include <netinet/in.h>
#include <arpa/inet.h>
#include "http_handler.h"
#include "server.h"
#include "utils.h"

extern ServerConfig server_config;
#define MAX_PATH 1024

// Mappa delle estensioni MIME
static struct {
    const char* ext;
    const char* mime;
} mime_types[] = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".json", "application/json"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif",  "image/gif"},
    {".ico",  "image/x-icon"},
    {NULL,    "text/plain"}
};

const char* get_mime_type(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (ext) {
        for (int i = 0; mime_types[i].ext != NULL; i++) {
            if (strcasecmp(ext, mime_types[i].ext) == 0) {
                return mime_types[i].mime;
            }
        }
    }
    return "text/plain";
}

// Invia un errore 404 se il file non esiste
static void send_404(int client_socket) {
    const char* response = "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "<html><body><h1>404 Not Found</h1></body></html>";
    send(client_socket, response, strlen(response), 0);
}
// Invia un errore 500 se c'è un problema con il server
static void send_500(int client_socket) {
    const char* response = "HTTP/1.1 500 Internal Server Error\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "<html><body><h1>500 Internal Server Error</h1></body></html>";
    send(client_socket, response, strlen(response), 0);
}
// Invia un errore 403 se l'accesso è vietato
static void send_403(int client_socket) {
    const char* response = "HTTP/1.1 403 Forbidden\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "<html><body><h1>403 Forbidden</h1></body></html>";
    send(client_socket, response, strlen(response), 0);
}
// Invia un errore 400 se la richiesta è malformata
static void send_400(int client_socket) {
    const char* response = "HTTP/1.1 400 Bad Request\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "<html><body><h1>400 Bad Request</h1></body></html>";
    send(client_socket, response, strlen(response), 0);
}
// Invia un errore 401 se l'autenticazione è richiesta
static void send_401(int client_socket) {
    const char* response = "HTTP/1.1 401 Unauthorized\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "<html><body><h1>401 Unauthorized</h1></body></html>";
    send(client_socket, response, strlen(response), 0);
}
// Invia un errore 408 se la richiesta è scaduta
static void send_408(int client_socket) {
    const char* response = "HTTP/1.1 408 Request Timeout\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "<html><body><h1>408 Request Timeout</h1></body></html>";
    send(client_socket, response, strlen(response), 0);
}
// Invia un errore 429 se ci sono troppe richieste
static void send_429(int client_socket) {
    const char* response = "HTTP/1.1 429 Too Many Requests\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "<html><body><h1>429 Too Many Requests</h1></body></html>";
    send(client_socket, response, strlen(response), 0);
}

static void send_file(int client_socket, const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        send_404(client_socket);
        return;
    }

    // Ottieni dimensione file
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Invia headers
    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n",
             get_mime_type(filepath), size);
    send(client_socket, header, strlen(header), 0);

    // Invia il file
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes, 0);
    }

    fclose(file);
}

void handle_http_request(int client_socket, char* buffer) {
    // Estrai il percorso dalla richiesta
    char* path_start = strchr(buffer, ' ') + 1;
    char* path_end = strchr(path_start, ' ');
    size_t path_length = path_end - path_start;

    char filepath[MAX_PATH];
    strncpy(filepath, server_config.www_root, sizeof(filepath));
    strncat(filepath, path_start, path_length);

    // Se la richiesta è per la root, serve index.html
    if (strcmp(path_start, "/") == 0 || strcmp(path_start, "/index.html") == 0) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", server_config.www_root);
    }

    // Se è un file HTML, analizza le metriche richieste
    if (strstr(filepath, ".html") != NULL) {
        FILE* file = fopen(filepath, "r");
        if (!file) {
            send_404(client_socket);
            return;
        }
        
        // Leggi il contenuto del file
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char* content = malloc(size + 1);
        fread(content, 1, size, file);
        content[size] = '\0';
        fclose(file);
        
        // Cerca il meta tag con le metriche
        char* metrics_list = NULL;
        char* meta_tag = strstr(content, "<meta name=\"sews-metrics\" content=\"");
        
        if (meta_tag) {
            meta_tag += 34; // Lunghezza di "<meta name=\"sews-metrics\" content=\""
            char* end = strchr(meta_tag, '\"');
            
            if (end) {
                size_t metrics_length = end - meta_tag;
                metrics_list = malloc(metrics_length + 1);
                strncpy(metrics_list, meta_tag, metrics_length);
                metrics_list[metrics_length] = '\0';
            }
        }
        
        // Se non ci sono metriche specificate, usa una lista vuota
        if (!metrics_list) {
            metrics_list = strdup("");
        }
        
        // Genera un token di sicurezza unico per questa richiesta
        char token[64];
        generate_random_token(token, sizeof(token));
        
        // Crea il tag script con il token di sicurezza
        char script_tag[512];
        snprintf(script_tag, sizeof(script_tag),
                 "<script>\n"
                 "window.SEWS_CONFIG = {\n"
                 "  securityToken: \"%s\"\n"
                 "};\n"
                 "</script>",
                 token);
        
        // Cerca il tag </head> per inserire lo script
        char* head_end = strstr(content, "</head>");
        if (head_end) {
            // Calcola la posizione di inserimento
            size_t pos = head_end - content;
            
            // Crea il nuovo contenuto con lo script inserito
            char* new_content = malloc(size + strlen(script_tag) + 1);
            strncpy(new_content, content, pos);
            strcpy(new_content + pos, script_tag);
            strcpy(new_content + pos + strlen(script_tag), head_end);
            
            // Invia la risposta HTTP con il contenuto modificato
            char header[512];
            snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     strlen(new_content));
            
            send(client_socket, header, strlen(header), 0);
            send(client_socket, new_content, strlen(new_content), 0);
            
            free(new_content);
        } else {
            // Se non trova </head>, invia il contenuto originale
            send_file(client_socket, filepath);
        }
        
        // Memorizza l'associazione tra token e metriche autorizzate
        store_token_metrics(token, metrics_list);
        
        free(content);
        free(metrics_list);
    } else {
        // Per i file non HTML, usa la funzione esistente
        send_file(client_socket, filepath);
    }
}
