// src/utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// Funzioni di logging e gestione errori
void log_message(const char* level, const char* message);
void fatal_error(const char* message);

// Funzioni di gestione memoria
void* safe_malloc(size_t size);
char* safe_strdup(const char* str);

// Funzioni di gestione file
char* read_file(const char* filename);
const char* get_file_extension(const char* filename);



//const char* get_mime_type(const char* filename);

// Funzioni di rete
//int create_server_socket(int port);

#endif
