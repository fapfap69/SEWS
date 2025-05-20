# SEWS - Simple Embedded Web Server

SEWS is a lightweight embedded web server written in C, designed to display metrics and numerical indicators in real-time via WebSocket.

Author : A.Franco - INFN Sezione di BARI - ITALY
Version : 1.0 - 20/05/2025

## History 

20/05/2025 - Release 1.0


## Features

- **Lightweight HTTP server** (< 30KB)
- **WebSocket support** for real-time updates
- **Responsive dashboard** that adapts to any device
- **Metrics filtering** based on page
- **Configurable alarm thresholds**
- **Multiple data sources** (file, commands, simulation)
- **Security** with authentication tokens
- **Configurable** via command line parameters

## Building

```bash
make           # Standard compilation
make tiny      # Size-optimized compilation
make compress  # Compress executable with UPX
```

## Running

```bash
./sews [options]
```

### Options

```
  -p, --port=PORT            Port to listen on (default: 8080)
  -c, --max-clients=NUM      Maximum number of clients (default: 10)
  -b, --buffer-size=SIZE     Buffer size (default: 4096)
  -w, --www-root=PATH        Root directory for static files (default: ./www)
  -m, --metrics-source=SRC   Metrics source (default: sim:1:100)
                             Formats: sim:inc:base, file:path, cmd:command
  -v, --verbose              Enable detailed log messages
  -h, --help                 Show this help message
```

## Usage Examples

### Metrics Simulation

```bash
./sews --metrics-source="sim:1:100"
```

This starts the server with simulated metrics that increment by 1 from a base of 100.

### Reading from File

```bash
./sews --metrics-source="file:/path/to/metrics.txt"
```

The metrics file must have the format:

```
# Comment
cpu=75[%]
memory=512[MB]
disk=250[GB]
network=1024[KB/s]
```

### Reading from Command

```bash
./sews --metrics-source="cmd:./get_metrics.sh"
```

The command must produce output in the metrics format.

### Example Script for System Metrics

```bash
#!/bin/bash
# get_metrics.sh - Generate system metrics

# CPU
cpu=$(top -bn1 | grep "Cpu(s)" | awk '{print 100 - $8}')
# Memory
mem=$(free -m | grep Mem | awk '{print $3}')
# Disk
disk=$(df -h / | tail -1 | awk '{print $5}' | tr -d '%')
# Load
load=$(cat /proc/loadavg | awk '{print $1}')

echo "cpu=$cpu[%]"
echo "memory=$mem[MB]"
echo "disk=$disk[%]"
echo "load=$load"
```

## Creating Custom Dashboards

To create a custom dashboard, create an HTML file with meta tags to specify the metrics to display:

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="sews-metrics" content="cpu,memory,disk,load"> <!-- Requested metrics -->
    <meta name="sews-thresholds" content="cpu:80:90,memory:70:90"> <!-- Warning:critical thresholds -->
    <title>System Dashboard</title>
    <link rel="stylesheet" href="css/style.css">
</head>
<body>
    <div class="container">
        <h1>System Dashboard</h1>
        <div class="metrics" id="metrics-container">
            <!-- Metrics will be inserted here dynamically -->
        </div>
        <div class="timestamp">
            Last update: <span id="last-update">-</span>
        </div>
        <div class="status">
            Connection status: <span id="connection-status">Disconnected</span>
        </div>
        <div class="alert-legend">
            <div class="legend-item">
                <div class="legend-indicator normal"></div>
                <span>Normal</span>
            </div>
            <div class="legend-item">
                <div class="legend-indicator warning"></div>
                <span>Warning</span>
            </div>
            <div class="legend-item">
                <div class="legend-indicator critical"></div>
                <span>Critical</span>
            </div>
        </div>
    </div>
    <script src="js/script.js"></script>
</body>
</html>
```

## Project Structure

```
SEWS/
├── src/                # Source code
│   ├── main.c          # Entry point
│   ├── server.c        # Server core
│   ├── websocket.c     # WebSocket handling
│   ├── http_handler.c  # HTTP handling
│   ├── metrics.c       # Metrics management
│   └── utils.c         # Utility functions
├── www/                # Static files
│   ├── index.html      # Main dashboard
│   ├── css/            # Stylesheets
│   └── js/             # JavaScript scripts
└── Makefile            # Build script
```

## Security

SEWS implements several security measures:
- Authentication tokens for WebSocket connections
- Page-based metrics filtering
- Protection against directory traversal
- Limitation of simultaneous requests
- Request timeouts

## Performance

SEWS is designed to be lightweight and fast:
- Executable size < 30KB
- Low memory consumption
- Support for hundreds of simultaneous connections
- Real-time updates with minimal latency

## System Requirements

- Operating System: Linux, macOS
- Libraries: pthread, OpenSSL

## License

This project is released under the MIT license.