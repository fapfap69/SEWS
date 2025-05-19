// http_handler.h
#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

void handle_http_request(int client_socket, char* buffer);
const char* get_mime_type(const char* filename);

//void send_404(int client_socket);
//void send_500(int client_socket);
//void send_403(int client_socket);
//void send_400(int client_socket);
//void send_file(int client_socket, const char* filepath);
//void send_directory_listing(int client_socket, const char* dirpath);

#endif // HTTP_HANDLER_H
