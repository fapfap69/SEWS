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

// Funzione generica per inviare risposte HTTP di errore
void send_http_error(int client_socket, int status_code, const char* status_text) {
    char response[256];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: text/html\r\n"
             "Connection: close\r\n"
             "\r\n"
             "<html><body><h1>%d %s</h1></body></html>",
             status_code, status_text, status_code, status_text);
    
    send(client_socket, response, strlen(response), 0);
}

static void send_file(int client_socket, const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        send_http_error(client_socket, 404, "Not Found");
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

// Funzione per estrarre il contenuto di un meta tag
static char* extract_meta_content(const char* html, const char* meta_name) {
    char search_string[128];
    snprintf(search_string, sizeof(search_string), "<meta name=\"%s\" content=\"", meta_name);
    
    const char* meta_tag = strstr(html, search_string);
    if (!meta_tag) {
        return NULL;
    }
    
    const char* content_start = meta_tag + strlen(search_string);
    const char* content_end = strchr(content_start, '\"');
    if (!content_end) {
        return NULL;
    }
    
    size_t content_length = content_end - content_start;
    char* content = malloc(content_length + 1);
    if (!content) {
        return NULL;
    }
    
    strncpy(content, content_start, content_length);
    content[content_length] = '\0';
    
    return content;
}

void handle_http_request(int client_socket, char* buffer) {
    // Verifica se la richiesta è valida
    if (!strstr(buffer, "GET ") && !strstr(buffer, "HEAD ")) {
        send_http_error(client_socket, 400, "Bad Request");
        return;
    }
    
    // Estrai il percorso dalla richiesta
    char* path_start = strchr(buffer, ' ');
    if (!path_start) {
        send_http_error(client_socket, 400, "Bad Request");
        return;
    }
    
    path_start += 1;
    char* path_end = strchr(path_start, ' ');
    if (!path_end) {
        send_http_error(client_socket, 400, "Bad Request");
        return;
    }
    
    size_t path_length = path_end - path_start;
    
    // Verifica se il percorso è troppo lungo
    if (path_length >= MAX_PATH - strlen(server_config.www_root) - 1) {
        send_http_error(client_socket, 414, "URI Too Long");
        return;
    }
    
    // Verifica se il percorso contiene sequenze di escape per directory traversal
    if (strstr(path_start, "..")) {
        send_http_error(client_socket, 403, "Forbidden");
        return;
    }
    
    char filepath[MAX_PATH];
    strncpy(filepath, server_config.www_root, sizeof(filepath));
    strncat(filepath, path_start, path_length);
    
    // Se la richiesta è per la root, serve index.html
    if (strcmp(path_start, "/") == 0 || strcmp(path_start, "/index.html") == 0) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", server_config.www_root);
    }
    
    // Verifica se il file esiste e può essere letto
    struct stat file_stat;
    if (stat(filepath, &file_stat) != 0) {
        send_http_error(client_socket, 404, "Not Found");
        return;
    }
    
    // Verifica se è una directory
    if (S_ISDIR(file_stat.st_mode)) {
        // Reindirizza alla versione con slash finale se necessario
        if (path_start[path_length - 1] != '/') {
            char redirect_path[MAX_PATH];
            snprintf(redirect_path, sizeof(redirect_path), "%.*s/", (int)path_length, path_start);
            
            char redirect_response[512];
            snprintf(redirect_response, sizeof(redirect_response),
                     "HTTP/1.1 301 Moved Permanently\r\n"
                     "Location: %s\r\n"
                     "Content-Length: 0\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     redirect_path);
            
            send(client_socket, redirect_response, strlen(redirect_response), 0);
            return;
        }
        
        // Prova a servire index.html nella directory
        snprintf(filepath, sizeof(filepath), "%s%.*s/index.html", 
                 server_config.www_root, (int)path_length, path_start);
        
        if (stat(filepath, &file_stat) != 0) {
            send_http_error(client_socket, 404, "Not Found");
            return;
        }
    }
    
    // Verifica i permessi di lettura
    if (access(filepath, R_OK) != 0) {
        send_http_error(client_socket, 403, "Forbidden");
        return;
    }
    
    // Limita il numero di richieste simultanee (esempio)
    static int request_count = 0;
    static pthread_mutex_t request_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    pthread_mutex_lock(&request_mutex);
    request_count++;
    
    if (request_count > 100) {  // Limite arbitrario
        pthread_mutex_unlock(&request_mutex);
        send_http_error(client_socket, 429, "Too Many Requests");
        request_count--;
        return;
    }
    
    pthread_mutex_unlock(&request_mutex);
    
    // Imposta un timeout per la richiesta
    struct timeval timeout;
    timeout.tv_sec = 30;  // 30 secondi
    timeout.tv_usec = 0;
    
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
    }
    
    // Se è un file HTML, analizza le metriche richieste
    if (strstr(filepath, ".html") != NULL) {
        FILE* file = fopen(filepath, "r");
        if (!file) {
            send_http_error(client_socket, 404, "Not Found");
            goto cleanup;
        }
        
        // Leggi il contenuto del file
        fseek(file, 0, SEEK_END);
        size_t size = (size_t)ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char* content = malloc(size + 1);
        if (!content) {
            send_http_error(client_socket, 500, "Internal Server Error");
            fclose(file);
            goto cleanup;
        }
        
        size_t bytes_read = fread(content, 1, size, file);
        if (bytes_read < size) {
            send_http_error(client_socket, 500, "Internal Server Error");
            free(content);
            fclose(file);
            goto cleanup;
        }
        
        content[size] = '\0';
        fclose(file);
        
        // Cerca il meta tag con le metriche
        char* metrics_list = extract_meta_content(content, "swsws-metrics");
        
        // Se non ci sono metriche specificate, usa una lista vuota
        if (!metrics_list) {
            metrics_list = strdup("");
            if (!metrics_list) {
                send_http_error(client_socket, 500, "Internal Server Error");
                free(content);
                goto cleanup;
            }
        }
        
        // Genera un token di sicurezza unico per questa richiesta
        char token[64];
        generate_random_token(token, sizeof(token));
        
        // Crea il tag script con il token di sicurezza
        char script_tag[512];
        snprintf(script_tag, sizeof(script_tag),
                 "<script>\n"
                 "window.SWSWS_CONFIG = {\n"
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
            if (!new_content) {
                send_http_error(client_socket, 500, "Internal Server Error");
                free(content);
                free(metrics_list);
                goto cleanup;
            }
            
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
            
            if (send(client_socket, header, strlen(header), 0) < 0 ||
                send(client_socket, new_content, strlen(new_content), 0) < 0) {
                // Errore di invio, probabilmente il client ha chiuso la connessione
                free(new_content);
                free(content);
                free(metrics_list);
                goto cleanup;
            }
            
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

cleanup:
    // Decrementa il contatore di richieste
    pthread_mutex_lock(&request_mutex);
    request_count--;
    pthread_mutex_unlock(&request_mutex);
}

