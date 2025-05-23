document.addEventListener('DOMContentLoaded', function() {
    const metricsContainer = document.getElementById('metrics-container');
    const statusElement = document.getElementById('connection-status');
    let ws = null;
    let reconnectAttempts = 0;
    const maxReconnectAttempts = 5;
    
    // Ottieni le metriche dal meta tag
    const metricsMeta = document.querySelector('meta[name="swsws-metrics"]');
    const authorizedMetrics = metricsMeta ? metricsMeta.getAttribute('content').split(',') : [];
   
    // Ottieni le soglie dal meta tag
    const thresholdsMeta = document.querySelector('meta[name="swsws-thresholds"]');
    const thresholdsStr = thresholdsMeta ? thresholdsMeta.getAttribute('content') : '';
 
    // Parsing delle soglie
    const thresholds = {};
    if (thresholdsStr) {
        thresholdsStr.split(',').forEach(item => {
            const [metric, warning, critical] = item.split(':');
            thresholds[metric] = {
                warning: parseFloat(warning),
                critical: parseFloat(critical)
            };
        });
    }


    // Ottieni il token di sicurezza inserito dal server
    const config = window.SWSWS_CONFIG || {};
    const securityToken = config.securityToken || '';

    // Funzione per determinare lo stato di allarme
    function getAlertState(name, value) {
        if (!thresholds[name]) return 'normal';
        
        if (value >= thresholds[name].critical) {
            return 'critical';
        } else if (value >= thresholds[name].warning) {
            return 'warning';
        }
        
        return 'normal';
    }

    function updateTimestamp(timestamp) {
        let timestampElement = document.getElementById('last-update');
    
        // Se non esiste, crealo
        if (!timestampElement) {
            const statusDiv = document.querySelector('.status');
        
            // Crea un elemento per il timestamp
            const timestampDiv = document.createElement('div');
            timestampDiv.className = 'timestamp';
            timestampDiv.innerHTML = 'Ultimo aggiornamento: <span id="last-update"></span>';
        
            // Inseriscilo prima dell'elemento status
            statusDiv.parentNode.insertBefore(timestampDiv, statusDiv);
        
            // Ottieni il riferimento all'elemento span
            timestampElement = document.getElementById('last-update');
        }
    
        // Aggiorna il timestamp
        timestampElement.textContent = timestamp;
    }

    // Funzione per creare o aggiornare una metrica
    function updateMetric(name, data) {
        // Verifica se questa metrica è autorizzata
        if (authorizedMetrics.length > 0 && !authorizedMetrics.includes(name)) {
            return; // Ignora metriche non autorizzate
        }
    
        // Estrai valore e unità
        const value = data.value;
        const unit = data.unit || '';
    
        // Determina lo stato di allarme
        const alertState = getAlertState(name, value);

        // Cerca se esiste già un elemento per questa metrica
        let metricElement = document.getElementById('metric-' + name);
        let unitElement = document.getElementById('unit-' + name);
        let alertElement = document.getElementById('alert-' + name);
    
        // Se non esiste, crealo
        if (!metricElement) {
           const metricCard = document.createElement('div');
            metricCard.className = 'metric-card';
        
            const metricTitle = document.createElement('h2');
            metricTitle.textContent = name;
        
            const metricValue = document.createElement('div');
            metricValue.className = 'metric-value';
            metricValue.id = 'metric-' + name;
            metricValue.textContent = value;
        
            const metricUnit = document.createElement('div');
            metricUnit.className = 'metric-unit';
            metricUnit.id = 'unit-' + name;
            metricUnit.textContent = unit;
        
            const alertIndicator = document.createElement('div');
            alertIndicator.className = 'alert-indicator ' + alertState;
            alertIndicator.id = 'alert-' + name;

            metricCard.appendChild(alertIndicator);
            metricCard.appendChild(metricTitle);
            metricCard.appendChild(metricValue);
            metricCard.appendChild(metricUnit);
            metricsContainer.appendChild(metricCard);
        } else {
            // Altrimenti aggiorna solo il valore, l'unità e lo stato di allarme
            metricElement.textContent = value;
            metricElement.className = 'metric-value ' + alertState;
            
            if (unitElement) {
                unitElement.textContent = unit;
            }
            
            if (alertElement) {
                alertElement.className = 'alert-indicator ' + alertState;
            }
        }
    }

    function connect() {
        // Usa il protocollo corretto (ws o wss)
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        // Includi il token di sicurezza nella connessione
        const wsUrl = `${protocol}//${window.location.host}?token=${securityToken}`;
        
        console.log('Tentativo di connessione a:', wsUrl);
        
        try {
            ws = new WebSocket(wsUrl);
        } catch (e) {
            console.error('Errore nella creazione del WebSocket:', e);
            statusElement.textContent = 'Errore di connessione';
            statusElement.className = 'disconnected';
            return;
        }

        ws.onopen = function() {
            console.log('WebSocket connesso');
            statusElement.textContent = 'Connesso';
            statusElement.className = 'connected';
            reconnectAttempts = 0;
        };

        ws.onclose = function() {
            console.log('WebSocket disconnesso');
            statusElement.textContent = 'Disconnesso';
            statusElement.className = 'disconnected';
            
            // Gestione riconnessione
            if (reconnectAttempts < maxReconnectAttempts) {
                reconnectAttempts++;
                console.log(`Tentativo di riconnessione ${reconnectAttempts}/${maxReconnectAttempts}`);
                setTimeout(connect, 5000);
            }
        };

        ws.onmessage = function(event) {
            console.log('Messaggio ricevuto:', event.data);
            try {
                const data = JSON.parse(event.data);
        
                // Aggiorna il timestamp se presente
                if (data.timestamp) {
                    updateTimestamp(data.timestamp);
                }
        
                // Aggiorna tutte le metriche ricevute
                for (const [name, metricData] of Object.entries(data)) {
                    // Salta il campo timestamp che non è una metrica
                    if (name === 'timestamp') continue;
                    updateMetric(name, metricData);
                }
            } catch (e) {
                console.error('Errore nel parsing dei dati:', e);
            }
        };

        ws.onerror = function(error) {
            console.error('Errore WebSocket:', error);
        };
    }

    // Avvia la connessione WebSocket
    connect();
});
