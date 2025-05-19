// www/script.js
document.addEventListener('DOMContentLoaded', function() {
    const value1Element = document.getElementById('value1');
    const value2Element = document.getElementById('value2');
    const statusElement = document.getElementById('connection-status');
    let ws = null;
    let reconnectAttempts = 0;
    const maxReconnectAttempts = 5;

    function connect() {
        // Usa il protocollo corretto (ws o wss) e la porta corretta
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        // Usa la stessa porta del server HTTP
        const wsUrl = `${protocol}//${window.location.host}`;
        
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
                if (data.value1 !== undefined) {
                    value1Element.textContent = data.value1;
                }
                if (data.value2 !== undefined) {
                    value2Element.textContent = data.value2;
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

