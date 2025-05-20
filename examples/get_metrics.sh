#!/bin/bash

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

# Funzione per ottenere il numero di utenti
get_user_count() {
    local count=$(who | wc -l)
    echo $count
}

# Funzione per otterere informazioni sulla rete
get_network_stats() {
    local interface=""
    local network_stats=""

    if [[ "$(uname)" == "Darwin" ]]; then
        # macOS: ottieni l'interfaccia di default
        interface=$(route -n get default | grep interface | awk '{print $2}')
        if [ -z "$interface" ]; then
            interface="en0"  # Fallback all'interfaccia comune
        fi
        # Ottieni i byte ricevuti
        network_stats=$(netstat -I $interface -b | tail -1 | awk '{print $7}')
    else
        # Linux: ottieni l'interfaccia di default
        interface=$(ip route | grep default | awk '{print $5}')
        if [ -z "$interface" ]; then
            interface="eth0"  # Fallback all'interfaccia comune
        fi
        # Ottieni i byte ricevuti
        network_stats=$(cat /proc/net/dev | grep $interface | awk '{print $2}')
    fi

    # Converti in KB
    network_stats=$(echo "scale=2; $network_stats / 1024" | bc)

    echo $network_stats
}

# Ottieni le metriche
CPU_USAGE=$(get_cpu_usage)
MEMORY_USAGE=$(get_memory_usage)
DISK_USAGE=$(get_disk_usage)
LOAD_AVERAGE=$(get_load_average)
PROCESS_COUNT=$(get_process_count)
USER_COUNT=$(get_user_count)
NETWORK_STATS=$(get_network_stats)

# Stampa le metriche nel formato richiesto
echo "# Metriche di sistema"
echo "# Generato il $(date)"
echo "disk=$DISK_USAGE[GB]"
echo "network=$NETWORK_STATS[KB]"
echo "cpu=$CPU_USAGE[%]"
echo "memory=$MEMORY_USAGE[MB]"
echo "load=$LOAD_AVERAGE"
echo "processes=$PROCESS_COUNT"
echo "users=$USER_COUNT"
