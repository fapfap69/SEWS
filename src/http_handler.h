// http_handler.h
#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

void handle_http_request(int client_socket, char* buffer);
const char* get_mime_type(const char* filename);
void send_http_error(int client_socket, int status_code, const char* status_text);


#endif // HTTP_HANDLER_H
