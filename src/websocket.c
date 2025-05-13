// websocket.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>     // Per send() e altre funzioni socket
#include <netinet/in.h>     // Per strutture di rete
#include <arpa/inet.h>      // Per funzioni di conversione indirizzi
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "websocket.h"
#include "server.h"

// Costanti per i frame WebSocket
#define WS_FIN 0x80
#define WS_OPCODE_TEXT 0x01
#define WS_OPCODE_CLOSE 0x08
#define WS_MASK 0x80

// Funzione per generare la chiave di accettazione WebSocket
static char* generate_websocket_key(const char* client_key) {
    const char* magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char concat_key[128];
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    
    snprintf(concat_key, sizeof(concat_key), "%s%s", client_key, magic);
    
    SHA1((unsigned char*)concat_key, strlen(concat_key), sha1_hash);
    
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, sha1_hash, SHA_DIGEST_LENGTH);
    BIO_flush(b64);
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    
    char* base64_key = malloc(bptr->length);
    memcpy(base64_key, bptr->data, bptr->length - 1);
    base64_key[bptr->length - 1] = '\0';
    
    BIO_free_all(b64);
    return base64_key;
}

// Funzione per l'handshake WebSocket
int handle_websocket_handshake(int client_socket, char* buffer) {
    char* key_start = strstr(buffer, "Sec-WebSocket-Key: ") + 19;
    char* key_end = strstr(key_start, "\r\n");
    size_t key_length = key_end - key_start;
    
    char client_key[25];
    strncpy(client_key, key_start, key_length);
    client_key[key_length] = '\0';
    
    char* accept_key = generate_websocket_key(client_key);
    
    char response[256];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             accept_key);
    
    free(accept_key);
    
    return send(client_socket, response, strlen(response), 0);
}

// Funzione per inviare un frame WebSocket
static int send_websocket_frame(int client_socket, const char* message, size_t length) {
    unsigned char* frame = malloc(10 + length); // Header max 10 bytes + payload
    size_t frame_size;
    
    frame[0] = WS_FIN | WS_OPCODE_TEXT;
    
    if (length <= 125) {
        frame[1] = length;
        frame_size = 2;
    } else if (length <= 65535) {
        frame[1] = 126;
        frame[2] = (length >> 8) & 0xFF;
        frame[3] = length & 0xFF;
        frame_size = 4;
    } else {
        frame[1] = 127;
        for (int i = 0; i < 8; i++) {
            frame[2 + i] = (length >> ((7 - i) * 8)) & 0xFF;
        }
        frame_size = 10;
    }
    
    memcpy(frame + frame_size, message, length);
    frame_size += length;
    
    int result = send(client_socket, frame, frame_size, 0);
    free(frame);
    return result;
}

// Funzione per gestire i frame WebSocket in arrivo
void handle_websocket_frame(int client_socket, unsigned char* buffer, size_t length) {
    if (length < 2) return;
    
    unsigned char opcode = buffer[0] & 0x0F;
    unsigned char masked = buffer[1] & WS_MASK;
    size_t payload_length = buffer[1] & 0x7F;
    size_t header_length = 2;
    
    if (payload_length == 126) {
        payload_length = (buffer[2] << 8) | buffer[3];
        header_length += 2;
    } else if (payload_length == 127) {
        payload_length = 0;
        for (int i = 0; i < 8; i++) {
            payload_length = (payload_length << 8) | buffer[2 + i];
        }
        header_length += 8;
    }
    
    if (masked) {
        unsigned char* mask = &buffer[header_length];
        unsigned char* payload = &buffer[header_length + 4];
        
        for (size_t i = 0; i < payload_length; i++) {
            payload[i] ^= mask[i % 4];
        }
        
        if (opcode == WS_OPCODE_CLOSE) {
            // Gestione chiusura connessione
            unsigned char close_frame[] = {0x88, 0x00};
            send(client_socket, close_frame, 2, 0);
        }
    }
}

// Funzione per inviare aggiornamenti a tutti i client
void broadcast_metrics(const char* message) {
    // Nota: questa funzione dovrà essere implementata per accedere alla lista dei client
    // e inviare il messaggio a ciascuno di essi usando send_websocket_frame
    // La lista dei client dovrebbe essere gestita nel server.c
    
    // Per ora è solo un placeholder
    printf("Broadcasting: %s\n", message);
}
// In un'applicazione reale, dovresti implementare una lista di client e inviare il messaggio a ciascuno di essi
// usando send_websocket_frame(client_socket, message, length);
// La lista dei client dovrebbe essere gestita nel server.c
// In un'applicazione reale, dovresti implementare una lista di client e inviare il messaggio a ciascuno di essi
// usando send_websocket_frame(client_socket, message, length);
// La lista dei client dovrebbe essere gestita nel server.c
// In un'applicazione reale, dovresti implementare una lista di client e inviare il messaggio a ciascuno di essi
// usando send_websocket_frame(client_socket, message, length);
// La lista dei client dovrebbe essere gestita nel server.c     