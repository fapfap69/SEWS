#!/bin/bash

# Nome del file di output
OUTPUT_FILE="metrics.dat"

# Funzione per ottenere la percentuale di CPU utilizzata
get_cpu_usage() {
    # Ottiene la percentuale di CPU idle e la converte in percentuale utilizzata
    if [[ "$(uname)" == "Darwin" ]]; then
        # macOS
        local idle=$(top -l 1 | grep "CPU usage" | awk '{print $7}' | tr -d '%')
        echo "scale=2; 100 - $idle" | bc
    else
        # Linux
        local idle=$(top -bn1 | grep "Cpu(s)" | awk '{print $8}')
        echo "scale=2; 100 - $idle" | bc
    fi
}

# Funzione per ottenere la memoria utilizzata in MB
get_memory_usage() {
    if [[ "$(uname)" == "Darwin" ]]; then
        # macOS
        local used=$(top -l 1 | grep PhysMem | awk '{print $2}' | tr -d 'M')
        echo $used
    else
        # Linux
        local used=$(free -m | grep Mem | awk '{print $3}')
        echo $used
    fi
}

# Funzione per ottenere lo spazio su disco utilizzato in GB
get_disk_usage() {
    local used=$(df -h / | tail -1 | awk '{print $3}' | tr -d 'G')
    # Converti in numero con decimali se contiene un punto
    if [[ $used == *"."* ]]; then
        echo $used
    else
        echo "$used.0"
    fi
}

# Funzione per ottenere il carico di sistema (load average)
get_load_average() {
    if [[ "$(uname)" == "Darwin" ]]; then
        # macOS
        local load=$(sysctl -n vm.loadavg | awk '{print $2}')
        echo $load
    else
        # Linux
        local load=$(cat /proc/loadavg | awk '{print $1}')
        echo $load
    fi
}

# Funzione per ottenere il numero di processi
get_process_count() {
    local count=$(ps aux | wc -l)
    # Sottrai 1 per l'intestazione
    echo $(($count - 1))
}

# Ottieni le metriche
CPU_USAGE=$(get_cpu_usage)
MEMORY_USAGE=$(get_memory_usage)
DISK_USAGE=$(get_disk_usage)
LOAD_AVERAGE=$(get_load_average)
PROCESS_COUNT=$(get_process_count)

# Crea il file di metriche
cat > $OUTPUT_FILE << EOF
# Metriche di sistema
# Generato il $(date)

# Utilizzo CPU (percentuale)
cpu=$CPU_USAGE[%]

# Memoria utilizzata (MB)
memory=$MEMORY_USAGE[MB]

# Spazio su disco utilizzato (GB)
disk=$DISK_USAGE[GB]

# Carico di sistema (load average)
load=$LOAD_AVERAGE

# Numero di processi
processes=$PROCESS_COUNT
EOF

echo "Metriche salvate in $OUTPUT_FILE"

# Mostra il contenuto del file
cat $OUTPUT_FILE
