// metrics.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "metrics.h"

// Dati delle metriche
static Metrics current_metrics = {.count = 0};  // Inizializza con count = 0

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
    current_metrics.count = 0;  // Inizializza il contatore delle metriche a zero
    pthread_mutex_unlock(&metrics_mutex);
}

// Registra un callback per l'aggiornamento delle metriche
void metrics_register_callback(metrics_callback_t callback) {
    update_callback = callback;
}

// Aggiorna le metriche con nuovi valori (per compatibilità con il vecchio codice)
void metrics_update(int value1, int value2) {
    // Aggiorna le metriche usando i nuovi nomi
    metrics_set("value1", value1);
    metrics_set("value2", value2);
}

// Ottieni i valori correnti delle metriche
void metrics_get(Metrics* metrics) {
    pthread_mutex_lock(&metrics_mutex);
    *metrics = current_metrics;
    pthread_mutex_unlock(&metrics_mutex);
}

// Aggiorna una metrica specifica con unità di misura
void metrics_set_with_unit(const char* name, int value, const char* unit) {
    pthread_mutex_lock(&metrics_mutex);
    
    // Cerca se la metrica esiste già
    for (int i = 0; i < current_metrics.count; i++) {
        if (strcmp(current_metrics.metrics[i].name, name) == 0) {
            current_metrics.metrics[i].value = value;
            
            // Aggiorna l'unità di misura se specificata
            if (unit && *unit) {
                strncpy(current_metrics.metrics[i].unit, unit, sizeof(current_metrics.metrics[i].unit) - 1);
                current_metrics.metrics[i].unit[sizeof(current_metrics.metrics[i].unit) - 1] = '\0';
            }
            
            // Notifica tramite callback se registrato
            if (update_callback) {
                update_callback(&current_metrics);
            }
            
            pthread_mutex_unlock(&metrics_mutex);
            return;
        }
    }
    
    // Se non esiste e c'è spazio, aggiungila
    if (current_metrics.count < MAX_METRICS) {
        strncpy(current_metrics.metrics[current_metrics.count].name, name, sizeof(current_metrics.metrics[0].name) - 1);
        current_metrics.metrics[current_metrics.count].name[sizeof(current_metrics.metrics[0].name) - 1] = '\0';
        current_metrics.metrics[current_metrics.count].value = value;
        
        // Imposta l'unità di misura se specificata
        if (unit && *unit) {
            strncpy(current_metrics.metrics[current_metrics.count].unit, unit, sizeof(current_metrics.metrics[0].unit) - 1);
            current_metrics.metrics[current_metrics.count].unit[sizeof(current_metrics.metrics[0].unit) - 1] = '\0';
        } else {
            current_metrics.metrics[current_metrics.count].unit[0] = '\0';  // Unità vuota
        }
        
        current_metrics.count++;
        
        // Notifica tramite callback se registrato
        if (update_callback) {
            update_callback(&current_metrics);
        }
    }
    
    pthread_mutex_unlock(&metrics_mutex);
}

// Aggiorna una metrica specifica
void metrics_set(const char* name, int value) {
    pthread_mutex_lock(&metrics_mutex);
    
    // Cerca se la metrica esiste già
    for (int i = 0; i < current_metrics.count; i++) {
        if (strcmp(current_metrics.metrics[i].name, name) == 0) {
            current_metrics.metrics[i].value = value;
            
            // Notifica tramite callback se registrato
            if (update_callback) {
                update_callback(&current_metrics);
            }
            
            pthread_mutex_unlock(&metrics_mutex);
            return;
        }
    }
    
    // Se non esiste e c'è spazio, aggiungila
    if (current_metrics.count < MAX_METRICS) {
        strncpy(current_metrics.metrics[current_metrics.count].name, name, sizeof(current_metrics.metrics[0].name) - 1);
        current_metrics.metrics[current_metrics.count].name[sizeof(current_metrics.metrics[0].name) - 1] = '\0';
        current_metrics.metrics[current_metrics.count].value = value;
        current_metrics.count++;
        
        // Notifica tramite callback se registrato
        if (update_callback) {
            update_callback(&current_metrics);
        }
    }
    
    pthread_mutex_unlock(&metrics_mutex);
}
// Funzione per leggere le metriche da un file
static bool read_metrics_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return false;
    }
    
    char line[256];
    bool success = false;
    
    while (fgets(line, sizeof(line), file)) {
        // Rimuovi newline
        char* newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        
        // Salta linee vuote e commenti
        if (line[0] == '\0' || line[0] == '#') continue;
        
        // Cerca il separatore '='
        char* separator = strchr(line, '=');
        if (!separator) continue;
        
        // Dividi nome e valore
        *separator = '\0';
        char* name = line;
        char* value_str = separator + 1;
        
        // Cerca l'unità di misura tra parentesi quadre
        char* unit = NULL;
        char* unit_start = strchr(value_str, '[');
        if (unit_start) {
            *unit_start = '\0';  // Termina il valore prima dell'unità
            unit_start++;  // Salta la parentesi quadra
            
            char* unit_end = strchr(unit_start, ']');
            if (unit_end) {
                *unit_end = '\0';  // Termina l'unità
                unit = unit_start;
            }
        }
        
        // Converti il valore in intero
        int value = atoi(value_str);
        
        // Aggiorna la metrica con l'unità di misura
        metrics_set_with_unit(name, value, unit ? unit : "");
        success = true;
    }
    
    fclose(file);
    return success;
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
        bool success = false;
        
        // Determina il tipo di fonte
        if (strncmp(source, "file:", 5) == 0) {
            // Leggi da file
            success = read_metrics_from_file(source + 5);
        } else if (strncmp(source, "cmd:", 4) == 0) {
            // Leggi da comando
            // Implementazione simile a read_metrics_from_file ma con popen
            // ...
        } else if (strncmp(source, "sim:", 4) == 0) {
            // Simulazione
            static int counter = 0;
            counter = (counter + 1) % 1000;
            
            metrics_set("cpu", counter);
            metrics_set("memory", 100 + (counter % 50));
            metrics_set("disk", 200 + (counter % 30));
            metrics_set("network", 300 + (counter % 70));
            
            success = true;
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

static TokenMetrics* tokens = NULL;
static int num_tokens = 0;
static pthread_mutex_t tokens_mutex = PTHREAD_MUTEX_INITIALIZER;

// Genera un token casuale
void generate_random_token(char* token, size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    for (size_t i = 0; i < length - 1; i++) {
        int index = rand() % (sizeof(charset) - 1);
        token[i] = charset[index];
    }
    token[length - 1] = '\0';
}

// Memorizza l'associazione tra token e metriche
void store_token_metrics(const char* token, const char* metrics) {
    pthread_mutex_lock(&tokens_mutex);
    
    // Aggiungi un nuovo token
    tokens = realloc(tokens, (num_tokens + 1) * sizeof(TokenMetrics));
    strcpy(tokens[num_tokens].token, token);
    tokens[num_tokens].metrics = strdup(metrics);
    tokens[num_tokens].expiry = time(NULL) + 3600; // Scade dopo 1 ora
    num_tokens++;
    
    pthread_mutex_unlock(&tokens_mutex);
}

// Ottieni le metriche associate a un token
bool get_token_metrics(const char* token, char** metrics) {
    pthread_mutex_lock(&tokens_mutex);
    
    bool found = false;
    time_t now = time(NULL);
    
    for (int i = 0; i < num_tokens; i++) {
        if (strcmp(tokens[i].token, token) == 0) {
            if (tokens[i].expiry > now) {
                // Token valido
                *metrics = strdup(tokens[i].metrics);
                found = true;
            }
            break;
        }
    }
    
    pthread_mutex_unlock(&tokens_mutex);
    return found;
}

// Pulizia periodica dei token scaduti
void cleanup_expired_tokens() {
    pthread_mutex_lock(&tokens_mutex);
    
    time_t now = time(NULL);
    int i = 0;
    
    while (i < num_tokens) {
        if (tokens[i].expiry <= now) {
            // Token scaduto, rimuovilo
            free(tokens[i].metrics);
            
            // Sposta l'ultimo token in questa posizione
            if (i < num_tokens - 1) {
                tokens[i] = tokens[num_tokens - 1];
            }
            
            num_tokens--;
        } else {
            i++;
        }
    }
    
    pthread_mutex_unlock(&tokens_mutex);
}
