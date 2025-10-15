/**
 * DETECTRA Gateway v2.0 - Web Interface HTML
 *
 * Embedded web pages for device management and monitoring
 */

#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

// ==================== MAIN DASHBOARD PAGE ====================

const char* HTML_DASHBOARD = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DETECTRA Gateway - Dashboard</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #e0e0e0;
            padding: 20px;
            min-height: 100vh;
        }

        .container {
            max-width: 1400px;
            margin: 0 auto;
        }

        header {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            padding: 20px 30px;
            border-radius: 15px;
            margin-bottom: 30px;
            border: 1px solid rgba(0, 212, 255, 0.3);
        }

        h1 {
            color: #00d4ff;
            font-size: 2em;
            margin-bottom: 10px;
        }

        .gateway-info {
            display: flex;
            gap: 30px;
            flex-wrap: wrap;
            margin-top: 15px;
        }

        .info-item {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .status-dot {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            animation: pulse 2s infinite;
        }

        .status-online { background: #00ff88; }
        .status-offline { background: #ff4444; }
        .status-warning { background: #ffaa00; }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }

        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }

        .stat-card {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            padding: 20px;
            border-radius: 12px;
            border: 1px solid rgba(0, 212, 255, 0.2);
            transition: transform 0.3s, border-color 0.3s;
        }

        .stat-card:hover {
            transform: translateY(-5px);
            border-color: rgba(0, 212, 255, 0.5);
        }

        .stat-label {
            color: #888;
            font-size: 0.9em;
            margin-bottom: 8px;
        }

        .stat-value {
            color: #00d4ff;
            font-size: 2em;
            font-weight: bold;
        }

        .section {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            padding: 25px;
            border-radius: 15px;
            margin-bottom: 25px;
            border: 1px solid rgba(0, 212, 255, 0.2);
        }

        h2 {
            color: #00d4ff;
            margin-bottom: 20px;
            font-size: 1.5em;
            border-bottom: 2px solid rgba(0, 212, 255, 0.3);
            padding-bottom: 10px;
        }

        .button-group {
            display: flex;
            gap: 15px;
            margin-bottom: 20px;
            flex-wrap: wrap;
        }

        button {
            background: linear-gradient(135deg, #00d4ff 0%, #0099cc 100%);
            color: white;
            border: none;
            padding: 12px 25px;
            border-radius: 8px;
            cursor: pointer;
            font-size: 1em;
            font-weight: 600;
            transition: all 0.3s;
            box-shadow: 0 4px 15px rgba(0, 212, 255, 0.3);
        }

        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(0, 212, 255, 0.5);
        }

        button:active {
            transform: translateY(0);
        }

        button.secondary {
            background: linear-gradient(135deg, #666 0%, #444 100%);
            box-shadow: 0 4px 15px rgba(100, 100, 100, 0.3);
        }

        button.danger {
            background: linear-gradient(135deg, #ff4444 0%, #cc0000 100%);
            box-shadow: 0 4px 15px rgba(255, 68, 68, 0.3);
        }

        .devices-table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 15px;
        }

        .devices-table th {
            background: rgba(0, 212, 255, 0.2);
            padding: 12px;
            text-align: left;
            color: #00d4ff;
            font-weight: 600;
        }

        .devices-table td {
            padding: 12px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        }

        .devices-table tr:hover {
            background: rgba(0, 212, 255, 0.1);
        }

        .device-status {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 0.9em;
            font-weight: 600;
        }

        .device-status.online {
            background: rgba(0, 255, 136, 0.2);
            color: #00ff88;
        }

        .device-status.offline {
            background: rgba(255, 68, 68, 0.2);
            color: #ff4444;
        }

        .device-status.polling {
            background: rgba(255, 170, 0, 0.2);
            color: #ffaa00;
        }

        .modal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0, 0, 0, 0.8);
            backdrop-filter: blur(5px);
            z-index: 1000;
            align-items: center;
            justify-content: center;
        }

        .modal.active {
            display: flex;
        }

        .modal-content {
            background: #1a1a2e;
            padding: 30px;
            border-radius: 15px;
            max-width: 500px;
            width: 90%;
            border: 2px solid #00d4ff;
            box-shadow: 0 10px 50px rgba(0, 212, 255, 0.3);
        }

        .form-group {
            margin-bottom: 20px;
        }

        label {
            display: block;
            margin-bottom: 8px;
            color: #00d4ff;
            font-weight: 600;
        }

        input[type="text"] {
            width: 100%;
            padding: 12px;
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(0, 212, 255, 0.3);
            border-radius: 8px;
            color: white;
            font-size: 1em;
        }

        input[type="text"]:focus {
            outline: none;
            border-color: #00d4ff;
            box-shadow: 0 0 10px rgba(0, 212, 255, 0.3);
        }

        .progress-bar {
            width: 100%;
            height: 30px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            overflow: hidden;
            margin-top: 20px;
        }

        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #00d4ff 0%, #00ff88 100%);
            width: 0%;
            transition: width 0.5s;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: 600;
        }

        .log-container {
            background: rgba(0, 0, 0, 0.3);
            padding: 15px;
            border-radius: 8px;
            max-height: 300px;
            overflow-y: auto;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
        }

        .log-entry {
            padding: 5px 0;
            border-bottom: 1px solid rgba(255, 255, 255, 0.05);
        }

        .log-timestamp {
            color: #888;
            margin-right: 10px;
        }

        .no-devices {
            text-align: center;
            padding: 40px;
            color: #888;
            font-size: 1.1em;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>DETECTRA Gateway Dashboard</h1>
            <div class="gateway-info">
                <div class="info-item">
                    <span class="status-dot status-online" id="statusDot"></span>
                    <span><strong>Gateway:</strong> <span id="gatewayId">GW-XXXXX</span></span>
                </div>
                <div class="info-item">
                    <span><strong>IP:</strong> <span id="ipAddress">---.---.---.---</span></span>
                </div>
                <div class="info-item">
                    <span><strong>Uptime:</strong> <span id="uptime">--</span></span>
                </div>
            </div>
        </header>

        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-label">Paired Devices</div>
                <div class="stat-value" id="deviceCount">0</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Online Devices</div>
                <div class="stat-value" id="onlineCount">0</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Total Messages</div>
                <div class="stat-value" id="messageCount">0</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Success Rate</div>
                <div class="stat-value" id="successRate">0%</div>
            </div>
        </div>

        <div class="section">
            <h2>Quick Actions</h2>
            <div class="button-group">
                <button onclick="startPolling()">▶ Start Polling</button>
                <button onclick="showAddDeviceModal()" class="secondary">➕ Add Device</button>
                <button onclick="refreshStatus()" class="secondary">🔄 Refresh</button>
            </div>

            <div id="pollingProgress" style="display: none;">
                <div class="progress-bar">
                    <div class="progress-fill" id="progressFill">0%</div>
                </div>
                <p style="margin-top: 10px; color: #888;">
                    <span id="pollingStatus">Polling in progress...</span>
                </p>
            </div>
        </div>

        <div class="section">
            <h2>Paired Devices (<span id="deviceTableCount">0</span>)</h2>

            <div id="devicesList">
                <table class="devices-table" id="devicesTable">
                    <thead>
                        <tr>
                            <th>Device ID</th>
                            <th>Status</th>
                            <th>Table Left</th>
                            <th>Table Right</th>
                            <th>Battery</th>
                            <th>RSSI</th>
                            <th>Last Seen</th>
                            <th>Actions</th>
                        </tr>
                    </thead>
                    <tbody id="devicesTableBody">
                        <tr>
                            <td colspan="8" class="no-devices">No devices paired. Click "Add Device" to get started.</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>

        <div class="section">
            <h2>System Log</h2>
            <div class="log-container" id="logContainer">
                <div class="log-entry">
                    <span class="log-timestamp">[--:--:--]</span>
                    <span>Waiting for updates...</span>
                </div>
            </div>
        </div>
    </div>

    <!-- Add Device Modal -->
    <div class="modal" id="addDeviceModal">
        <div class="modal-content">
            <h2>Add New Device</h2>
            <form id="addDeviceForm" onsubmit="pairDevice(event)">
                <div class="form-group">
                    <label for="deviceId">Device ID *</label>
                    <input type="text" id="deviceId" name="deviceId"
                           placeholder="EDy-XXXXX (e.g., ED1-A3F2B)"
                           pattern="ED[0-9]-[0-9A-Fa-f]{5}"
                           title="Format: EDy-XXXXX where y=0-9, X=hex (0-F)"
                           required>
                </div>

                <div class="form-group">
                    <label for="tableLeft">Table Left ID</label>
                    <input type="text" id="tableLeft" name="tableLeft"
                           placeholder="e.g., BLR-13-IL-02">
                </div>

                <div class="form-group">
                    <label for="tableRight">Table Right ID</label>
                    <input type="text" id="tableRight" name="tableRight"
                           placeholder="e.g., BLR-13-IL-01">
                </div>

                <div class="button-group" style="margin-top: 25px;">
                    <button type="submit">Pair Device</button>
                    <button type="button" onclick="closeModal()" class="secondary">Cancel</button>
                </div>

                <div id="pairStatus" style="margin-top: 15px; display: none;">
                    <p id="pairStatusText"></p>
                </div>
            </form>
        </div>
    </div>

    <script>
        let ws = null;
        let pollingActive = false;

        // Connect to WebSocket
        function connectWebSocket() {
            ws = new WebSocket('ws://' + location.host + '/ws');

            ws.onopen = function() {
                console.log('WebSocket connected');
                addLog('WebSocket connected');
                refreshStatus();
            };

            ws.onmessage = function(event) {
                try {
                    const data = JSON.parse(event.data);
                    handleWebSocketMessage(data);
                } catch(e) {
                    console.error('WebSocket message error:', e);
                }
            };

            ws.onclose = function() {
                console.log('WebSocket disconnected');
                addLog('WebSocket disconnected - reconnecting...');
                setTimeout(connectWebSocket, 3000);
            };

            ws.onerror = function(error) {
                console.error('WebSocket error:', error);
            };
        }

        function handleWebSocketMessage(data) {
            if (data.polling_active !== undefined) {
                updatePollingStatus(data);
            }

            if (data.devices) {
                updateDevicesList(data.devices);
            }

            if (data.stats) {
                updateStats(data.stats);
            }

            if (data.log) {
                addLog(data.log);
            }
        }

        function updatePollingStatus(data) {
            pollingActive = data.polling_active;

            if (pollingActive) {
                document.getElementById('pollingProgress').style.display = 'block';
                const progress = (data.current_device_index / data.total_devices) * 100;
                document.getElementById('progressFill').style.width = progress + '%';
                document.getElementById('progressFill').textContent = Math.round(progress) + '%';

                let statusText = `Polling device ${data.current_device_index + 1}/${data.total_devices}`;
                if (data.current_device_id) {
                    statusText += ` (${data.current_device_id})`;
                }
                if (data.current_phase) {
                    statusText += ` - ${data.current_phase}`;
                }
                document.getElementById('pollingStatus').textContent = statusText;
            } else {
                document.getElementById('pollingProgress').style.display = 'none';
            }
        }

        function updateDevicesList(devices) {
            const tbody = document.getElementById('devicesTableBody');

            if (devices.length === 0) {
                tbody.innerHTML = '<tr><td colspan="8" class="no-devices">No devices paired.</td></tr>';
                document.getElementById('deviceTableCount').textContent = '0';
                return;
            }

            tbody.innerHTML = '';
            document.getElementById('deviceTableCount').textContent = devices.length;

            devices.forEach(device => {
                const row = tbody.insertRow();

                // Device ID
                row.insertCell().textContent = device.device_id || '--';

                // Status
                const statusCell = row.insertCell();
                let statusClass = device.online ? 'online' : 'offline';
                if (pollingActive && device.polling) statusClass = 'polling';
                statusCell.innerHTML = `<span class="device-status ${statusClass}">${statusClass.toUpperCase()}</span>`;

                // Table Left
                row.insertCell().textContent = device.table_left || '--';

                // Table Right
                row.insertCell().textContent = device.table_right || '--';

                // Battery
                row.insertCell().textContent = device.battery >= 0 ? device.battery + '%' : '--';

                // RSSI
                row.insertCell().textContent = device.rssi ? device.rssi + ' dBm' : '--';

                // Last Seen
                const lastSeenCell = row.insertCell();
                if (device.last_contact) {
                    const seconds = Math.floor((Date.now() - device.last_contact) / 1000);
                    lastSeenCell.textContent = formatTimeSince(seconds);
                } else {
                    lastSeenCell.textContent = 'Never';
                }

                // Actions
                const actionsCell = row.insertCell();
                actionsCell.innerHTML = `
                    <button onclick="pollDevice('${device.device_id}')" style="padding: 5px 10px; font-size: 0.9em; margin-right: 5px;">📡 POLL</button>
                    <button onclick="removeDevice('${device.device_id}')" class="danger" style="padding: 5px 10px; font-size: 0.9em;">Remove</button>
                `;
            });
        }

        function updateStats(stats) {
            document.getElementById('deviceCount').textContent = stats.total_devices || 0;
            document.getElementById('onlineCount').textContent = stats.online_devices || 0;
            document.getElementById('messageCount').textContent = stats.total_messages || 0;

            const successRate = stats.success_rate || 0;
            document.getElementById('successRate').textContent = successRate.toFixed(1) + '%';

            if (stats.gateway_id) {
                document.getElementById('gatewayId').textContent = stats.gateway_id;
            }

            if (stats.ip_address) {
                document.getElementById('ipAddress').textContent = stats.ip_address;
            }

            if (stats.uptime_ms) {
                document.getElementById('uptime').textContent = formatUptime(stats.uptime_ms);
            }
        }

        function addLog(message) {
            const container = document.getElementById('logContainer');
            const entry = document.createElement('div');
            entry.className = 'log-entry';

            const timestamp = new Date().toLocaleTimeString();
            entry.innerHTML = `<span class="log-timestamp">[${timestamp}]</span><span>${message}</span>`;

            container.insertBefore(entry, container.firstChild);

            // Keep only last 50 entries
            while (container.children.length > 50) {
                container.removeChild(container.lastChild);
            }
        }

        function startPolling() {
            fetch('/api/poll/start', { method: 'POST' })
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'started') {
                        addLog('Polling cycle started');
                    } else if (data.error) {
                        addLog('Error: ' + data.error);
                    }
                })
                .catch(error => {
                    addLog('Failed to start polling: ' + error);
                });
        }

        function pollDevice(deviceId) {
            fetch('/api/poll/device', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ device_id: deviceId })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    addLog(`POLL sent to ${deviceId}`);
                } else {
                    addLog('Error: ' + (data.error || 'Unknown error'));
                    alert('Failed to poll device: ' + (data.error || 'Unknown error'));
                }
            })
            .catch(error => {
                addLog('Failed to poll device: ' + error);
                alert('Network error: ' + error);
            });
        }

        function refreshStatus() {
            fetch('/api/polling')
                .then(response => response.json())
                .then(data => {
                    handleWebSocketMessage(data);
                    addLog('Status refreshed');
                })
                .catch(error => {
                    console.error('Refresh failed:', error);
                });

            fetch('/api/devices')
                .then(response => response.json())
                .then(data => {
                    if (data.devices) {
                        updateDevicesList(data.devices);
                    }
                })
                .catch(error => {
                    console.error('Failed to fetch devices:', error);
                });
        }

        function showAddDeviceModal() {
            document.getElementById('addDeviceModal').classList.add('active');
        }

        function closeModal() {
            document.getElementById('addDeviceModal').classList.remove('active');
            document.getElementById('addDeviceForm').reset();
            document.getElementById('pairStatus').style.display = 'none';
        }

        function pairDevice(event) {
            event.preventDefault();

            const formData = new FormData(event.target);
            const deviceData = {
                device_id: formData.get('deviceId'),
                table_left: formData.get('tableLeft'),
                table_right: formData.get('tableRight')
            };

            document.getElementById('pairStatus').style.display = 'block';
            document.getElementById('pairStatusText').textContent = 'Pairing device...';

            fetch('/api/device/pair', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(deviceData)
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    document.getElementById('pairStatusText').textContent = '✓ Device paired successfully!';
                    document.getElementById('pairStatusText').style.color = '#00ff88';
                    addLog(`Device ${deviceData.device_id} paired`);
                    setTimeout(() => {
                        closeModal();
                        refreshStatus();
                    }, 2000);
                } else {
                    document.getElementById('pairStatusText').textContent = '✗ Pairing failed: ' + (data.error || 'Unknown error');
                    document.getElementById('pairStatusText').style.color = '#ff4444';
                }
            })
            .catch(error => {
                document.getElementById('pairStatusText').textContent = '✗ Network error: ' + error;
                document.getElementById('pairStatusText').style.color = '#ff4444';
            });
        }

        function removeDevice(deviceId) {
            if (!confirm(`Remove device ${deviceId}?`)) return;

            fetch('/api/device/remove', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ device_id: deviceId })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    addLog(`Device ${deviceId} removed`);
                    refreshStatus();
                } else {
                    alert('Failed to remove device: ' + (data.error || 'Unknown error'));
                }
            })
            .catch(error => {
                alert('Network error: ' + error);
            });
        }

        function formatTimeSince(seconds) {
            if (seconds < 60) return seconds + 's ago';
            if (seconds < 3600) return Math.floor(seconds / 60) + 'm ago';
            if (seconds < 86400) return Math.floor(seconds / 3600) + 'h ago';
            return Math.floor(seconds / 86400) + 'd ago';
        }

        function formatUptime(ms) {
            const seconds = Math.floor(ms / 1000);
            const hours = Math.floor(seconds / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            return hours + 'h ' + minutes + 'm';
        }

        // Initialize
        connectWebSocket();
        refreshStatus();
        setInterval(refreshStatus, 10000); // Refresh every 10 seconds
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_INTERFACE_H
