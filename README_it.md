# SEWS - Simple Embedded Web Server

SEWS è un server web embedded leggero scritto in C, progettato per visualizzare metriche e indicatori numerici in tempo reale tramite WebSocket.

## Caratteristiche

- **Server HTTP leggero** (< 30KB)
- **Supporto WebSocket** per aggiornamenti in tempo reale
- **Dashboard responsive** che si adatta a qualsiasi dispositivo
- **Filtro delle metriche** basato sulla pagina
- **Soglie di allarme** configurabili
- **Multiple fonti di dati** (file, comandi, simulazione)
- **Sicurezza** con token di autenticazione
- **Configurabile** tramite parametri da riga di comando

## Compilazione

```bash
make           # Compilazione standard
make tiny      # Compilazione ottimizzata per dimensioni
make compress  # Compressione dell'eseguibile con UPX
```

## Esecuzione

```bash
./sews [opzioni]
```

### Opzioni

```
  -p, --port=PORTA           Porta su cui ascoltare (default: 8080)
  -c, --max-clients=NUM      Numero massimo di client (default: 10)
  -b, --buffer-size=SIZE     Dimensione del buffer (default: 4096)
  -w, --www-root=PATH        Directory radice per i file statici (default: ./www)
  -m, --metrics-source=SRC   Fonte delle metriche (default: sim:1:100)
                             Formati: sim:inc:base, file:path, cmd:command
  -v, --verbose              Abilita i messaggi di log dettagliati
  -h, --help                 Mostra questo messaggio di aiuto
```

## Esempi di utilizzo

### Simulazione di metriche

```bash
./sews --metrics-source="sim:1:100"
```

Questo avvia il server con metriche simulate che incrementano di 1 a partire da una base di 100.

### Lettura da file

```bash
./sews --metrics-source="file:/path/to/metrics.txt"
```

Il file delle metriche deve avere il formato:

```
# Commento
cpu=75[%]
memory=512[MB]
disk=250[GB]
network=1024[KB/s]
```

### Lettura da comando

```bash
./sews --metrics-source="cmd:./get_metrics.sh"
```

Il comando deve produrre un output nel formato delle metriche.

### Script di esempio per le metriche di sistema

```bash
#!/bin/bash
# get_metrics.sh - Genera metriche di sistema

# CPU
cpu=$(top -bn1 | grep "Cpu(s)" | awk '{print 100 - $8}')
# Memoria
mem=$(free -m | grep Mem | awk '{print $3}')
# Disco
disk=$(df -h / | tail -1 | awk '{print $5}' | tr -d '%')
# Carico
load=$(cat /proc/loadavg | awk '{print $1}')

echo "cpu=$cpu[%]"
echo "memory=$mem[MB]"
echo "disk=$disk[%]"
echo "load=$load"
```

## Creazione di dashboard personalizzate

Per creare una dashboard personalizzata, crea un file HTML con meta tag per specificare le metriche da visualizzare:

```html
<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="sews-metrics" content="cpu,memory,disk,load"> <!-- Metriche richieste -->
    <meta name="sews-thresholds" content="cpu:80:90,memory:70:90"> <!-- Soglie warning:critical -->
    <title>Dashboard Sistema</title>
    <link rel="stylesheet" href="css/style.css">
</head>
<body>
    <div class="container">
        <h1>Dashboard Sistema</h1>
        <div class="metrics" id="metrics-container">
            <!-- Le metriche verranno inserite qui dinamicamente -->
        </div>
        <div class="timestamp">
            Ultimo aggiornamento: <span id="last-update">-</span>
        </div>
        <div class="status">
            Stato connessione: <span id="connection-status">Disconnesso</span>
        </div>
        <div class="alert-legend">
            <div class="legend-item">
                <div class="legend-indicator normal"></div>
                <span>Normale</span>
            </div>
            <div class="legend-item">
                <div class="legend-indicator warning"></div>
                <span>Attenzione</span>
            </div>
            <div class="legend-item">
                <div class="legend-indicator critical"></div>
                <span>Critico</span>
            </div>
        </div>
    </div>
    <script src="js/script.js"></script>
</body>
</html>
```

## Struttura del progetto

```
SEWS/
├── src/                # Codice sorgente
│   ├── main.c          # Punto di ingresso
│   ├── server.c        # Core del server
│   ├── websocket.c     # Gestione WebSocket
│   ├── http_handler.c  # Gestione HTTP
│   ├── metrics.c       # Gestione metriche
│   └── utils.c         # Funzioni di utilità
├── www/                # File statici
│   ├── index.html      # Dashboard principale
│   ├── css/            # Fogli di stile
│   └── js/             # Script JavaScript
└── Makefile            # Script di compilazione
```

## Sicurezza

SEWS implementa diverse misure di sicurezza:
- Token di autenticazione per le connessioni WebSocket
- Filtro delle metriche basato sulla pagina
- Protezione contro directory traversal
- Limitazione delle richieste simultanee
- Timeout delle richieste

## Prestazioni

SEWS è progettato per essere leggero e veloce:
- Dimensione dell'eseguibile < 30KB
- Basso consumo di memoria
- Supporto per centinaia di connessioni simultanee
- Aggiornamenti in tempo reale con latenza minima

## Requisiti di sistema

- Sistema operativo: Linux, macOS
- Librerie: pthread, OpenSSL

## Licenza

Questo progetto è rilasciato sotto licenza MIT.