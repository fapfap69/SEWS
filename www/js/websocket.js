document.addEventListener('DOMContentLoaded', function() {
    const ws = new WebSocket(`ws://${window.location.host}/ws`);
    const statusElement = document.getElementById('connection-status');

    ws.onopen = function() {
        statusElement.textContent = 'Connected';
        statusElement.className = 'connected';
    };

    ws.onclose = function() {
        statusElement.textContent = 'Disconnected';
        statusElement.className = 'disconnected';
    };

    ws.onmessage = function(event) {
        const data = JSON.parse(event.data);
        Object.keys(data).forEach(key => {
            const element = document.getElementById(key);
            if (element) {
                element.textContent = data[key];
            }
        });
    };
});
