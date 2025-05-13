#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

int init_server(int port);
void handle_connections(void);
void cleanup_server(void);

#endif
