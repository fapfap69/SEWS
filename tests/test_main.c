// tests/test_main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/http_handler.h"
#include "../src/websocket.h"

void test_mime_types() {
    assert(strcmp(get_mime_type("test.html"), "text/html") == 0);
    assert(strcmp(get_mime_type("style.css"), "text/css") == 0);
    assert(strcmp(get_mime_type("script.js"), "application/javascript") == 0);
    printf("MIME type tests passed\n");
}

int main() {
    test_mime_types();
    // Aggiungi altri test qui
    printf("All tests passed!\n");
    return 0;
}
