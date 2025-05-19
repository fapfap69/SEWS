// src/utils.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"

// Funzione per il logging
void log_message(const char* level, const char* message) {
    time_t now;
    time(&now);
    char* date = ctime(&now);
    date[strlen(date) - 1] = '\0'; // Rimuove il newline
    printf("[%s] [%s] %s\n", date, level, message);
}

// Funzione per gestire gli errori fatali
void fatal_error(const char* message) {
    log_message("ERROR", message);
    exit(1);
}

// Funzione per allocare memoria in modo sicuro
void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fatal_error("Memory allocation failed");
    }
    return ptr;
}

// Funzione per duplicare una stringa in modo sicuro
char* safe_strdup(const char* str) {
    char* dup = strdup(str);
    if (dup == NULL) {
        fatal_error("String duplication failed");
    }
    return dup;
}

// Funzione per leggere un file in memoria
char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = safe_malloc(length + 1);
    if (fread(buffer, 1, length, file) != (size_t)length) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

// Funzione per ottenere l'estensione di un file
const char* get_file_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "";
    }
    return dot + 1;
}


/*
// Funzione per ottenere il MIME type
const char* get_mime_type(const char* filename) {
    const char* ext = get_file_extension(filename);
    
    if (strcasecmp(ext, "html") == 0) return "text/html";
    if (strcasecmp(ext, "css") == 0)  return "text/css";
    if (strcasecmp(ext, "js") == 0)   return "application/javascript";
    if (strcasecmp(ext, "png") == 0)  return "image/png";
    if (strcasecmp(ext, "jpg") == 0)  return "image/jpeg";
    if (strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "gif") == 0)  return "image/gif";
    if (strcasecmp(ext, "ico") == 0)  return "image/x-icon";
    
    return "application/octet-stream";
}

// Funzione per creare un socket TCP
int create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        fatal_error("Socket creation failed");
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        fatal_error("Setsockopt failed");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        fatal_error("Bind failed");
    }

    if (listen(server_fd, 10) < 0) {
        fatal_error("Listen failed");
    }

    return server_fd;
}
*/