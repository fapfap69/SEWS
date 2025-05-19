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

    send_file(client_socket, filepath);
}
