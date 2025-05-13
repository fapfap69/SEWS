// main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "server.h"

static volatile int running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        running = 0;
    }
}

int main(void) {
    // Imposta il gestore dei segnali
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Starting SEWS (Simple Embedded Web Server)\n");
    
    // Avvia il server in un thread separato
    start_server();
    
    // Loop principale per l'aggiornamento delle metriche
    int counter1 = 0;
    int counter2 = 100;
    
    while (running) {
        // Simula l'aggiornamento delle metriche
        counter1 = (counter1 + 1) % 1000;
        counter2 = 100 + (counter1 % 50);
        
        // Aggiorna e invia le metriche
        update_metrics(counter1, counter2);
        
        // Attendi un secondo prima del prossimo aggiornamento
        sleep(1);
    }
    
    printf("\nShutting down...\n");
    return 0;
}
