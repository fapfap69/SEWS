// metrics.h
#ifndef METRICS_H
#define METRICS_H

#include <pthread.h>
#include <stdbool.h>

// Struttura per le metriche
typedef struct {
    int value1;
    int value2;
    // Aggiungi altre metriche secondo necessità
} Metrics;

// Inizializza il sistema di metriche
void metrics_init(void);

// Registra un callback per l'aggiornamento delle metriche
typedef void (*metrics_callback_t)(const Metrics* metrics);
void metrics_register_callback(metrics_callback_t callback);

// Aggiorna le metriche con nuovi valori
void metrics_update(int value1, int value2);

// Ottieni i valori correnti delle metriche
void metrics_get(Metrics* metrics);

// Avvia il thread di acquisizione delle metriche
bool metrics_start_collection(const char* source);

// Ferma il thread di acquisizione delle metriche
void metrics_stop_collection(void);

void metrics_updated_callback(const Metrics* metrics);


#endif