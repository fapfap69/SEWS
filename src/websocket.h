// websocket.h
#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stddef.h>

int handle_websocket_handshake(int client_socket, char* buffer);
void handle_websocket_frame(int client_socket, unsigned char* buffer, size_t length);
void broadcast_metrics(const char* message);

#endif


