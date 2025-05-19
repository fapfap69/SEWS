// metrics.h
#ifndef METRICS_H
#define METRICS_H

#include <pthread.h>
#include <stdbool.h>

#define MAX_METRICS 20  // Numero massimo di metriche supportate


// Struttura per una singola metrica
typedef struct {
    char name[64];
    int value;
    char unit[16];  // Unità di misura (%, MB, GB, KB/s, ecc.)
} Metric;

// Struttura per le metriche
typedef struct {
    Metric metrics[MAX_METRICS];
    int count;
} Metrics;

// Struttura per memorizzare i token e le metriche associate
typedef struct {
    char token[64];
    char* metrics;
    time_t expiry;
} TokenMetrics;

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
void metrics_set(const char* name, int value);
void metrics_set_with_unit(const char* name, int value, const char* unit);

// Token metrics functions
void generate_random_token(char* token, size_t length);
void store_token_metrics(const char* token, const char* metrics);
bool get_token_metrics(const char* token, char** metrics);
void cleanup_expired_tokens();

#endif