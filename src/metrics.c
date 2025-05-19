// metrics.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "metrics.h"

// Dati delle metriche
static Metrics current_metrics = {0, 0};
static pthread_mutex_t metrics_mutex = PTHREAD_MUTEX_INITIALIZER;

// Callback per le notifiche di aggiornamento
static metrics_callback_t update_callback = NULL;

// Thread di acquisizione
static pthread_t collection_thread;
static volatile bool collection_running = false;
static char collection_source[256] = "";

// Inizializza il sistema di metriche
void metrics_init(void) {
    pthread_mutex_lock(&metrics_mutex);
    current_metrics.value1 = 0;
    current_metrics.value2 = 0;
    pthread_mutex_unlock(&metrics_mutex);
}

// Registra un callback per l'aggiornamento delle metriche
void metrics_register_callback(metrics_callback_t callback) {
    update_callback = callback;
}

// Aggiorna le metriche con nuovi valori
void metrics_update(int value1, int value2) {
    pthread_mutex_lock(&metrics_mutex);
    current_metrics.value1 = value1;
    current_metrics.value2 = value2;
    
    // Notifica tramite callback se registrato
    if (update_callback) {
        update_callback(&current_metrics);
    }
    
    pthread_mutex_unlock(&metrics_mutex);
}

// Ottieni i valori correnti delle metriche
void metrics_get(Metrics* metrics) {
    pthread_mutex_lock(&metrics_mutex);
    *metrics = current_metrics;
    pthread_mutex_unlock(&metrics_mutex);
}

// Funzione per leggere le metriche da un file
static bool read_metrics_from_file(const char* filename, int* value1, int* value2) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return false;
    }
    
    int result = fscanf(file, "%d %d", value1, value2);
    fclose(file);
    
    return (result == 2);
}

// Funzione per leggere le metriche da una pipe
static bool read_metrics_from_pipe(const char* command, int* value1, int* value2) {
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        return false;
    }
    
    int result = fscanf(pipe, "%d %d", value1, value2);
    pclose(pipe);
    
    return (result == 2);
}

// Thread di acquisizione delle metriche
static void* metrics_collection_thread(void* arg) {
    const char* source = (const char*)arg;
    
    while (collection_running) {
        int value1 = 0, value2 = 0;
        bool success = false;
        
        // Determina il tipo di fonte
        if (strncmp(source, "file:", 5) == 0) {
            // Leggi da file
            success = read_metrics_from_file(source + 5, &value1, &value2);
        } else if (strncmp(source, "cmd:", 4) == 0) {
            // Leggi da comando
            success = read_metrics_from_pipe(source + 4, &value1, &value2);
        } else if (strncmp(source, "sim:", 4) == 0) {
            // Simulazione (formato: "sim:incremento:base")
            int increment = 1, base = 100;
            sscanf(source + 4, "%d:%d", &increment, &base);
            
            static int counter = 0;
            counter = (counter + increment) % 1000;
            value1 = counter;
            value2 = base + (counter % 50);
            success = true;
        }
        
        if (success) {
            metrics_update(value1, value2);
        }
        
        // Attendi prima del prossimo aggiornamento
        sleep(1);
    }
    
    return NULL;
}

// Avvia il thread di acquisizione delle metriche
bool metrics_start_collection(const char* source) {
    if (collection_running) {
        return false;  // Già in esecuzione
    }
    
    strncpy(collection_source, source, sizeof(collection_source) - 1);
    collection_source[sizeof(collection_source) - 1] = '\0';
    
    collection_running = true;
    
    if (pthread_create(&collection_thread, NULL, metrics_collection_thread, collection_source) != 0) {
        collection_running = false;
        return false;
    }
    
    return true;
}

// Ferma il thread di acquisizione delle metriche
void metrics_stop_collection(void) {
    if (!collection_running) {
        return;
    }
    
    collection_running = false;
    pthread_join(collection_thread, NULL);
}