# DETECTRA Gateway v2.0 - Web Interface Guide

## Access

**URL:** `http://<gateway-ip>/`

**Credentials:**
- Username: `rnd`
- Password: `rnd`

---

## Dashboard Features

### 1. Gateway Status Header
- **Gateway ID:** Current gateway identifier (e.g., GW01)
- **IP Address:** Gateway's local IP
- **Uptime:** Time since boot

### 2. Statistics Cards
- **Paired Devices:** Total number of configured devices
- **Online Devices:** Currently responsive devices
- **Total Messages:** LoRa messages received
- **Success Rate:** Percentage of successful polls

### 3. Quick Actions
- **▶ Start Polling:** Manually trigger a polling cycle
- **➕ Add Device:** Open device pairing modal
- **🔄 Refresh:** Update all data

### 4. Paired Devices Table
Displays all configured devices with:
- Device ID (EDy-XXXXX format)
- Status (Online/Offline/Polling)
- Table Left ID
- Table Right ID
- Battery percentage
- RSSI (signal strength)
- Last seen timestamp
- Remove button

### 5. System Log
Real-time log of gateway activities

---

## Adding a New Device

### Step 1: Click "Add Device" Button

### Step 2: Fill Device Information

**Device ID** (Required)
- Format: `EDy-XXXXX`
- `y` = single digit (0-9)
- `XXXXX` = 5 hex characters (0-9, A-F)
- Example: `ED1-A3F2B`, `ED2-C4D1E`

**Table Left ID** (Optional)
- Left table identifier
- Example: `BLR-13-IL-02`

**Table Right ID** (Optional)
- Right table identifier
- Example: `BLR-13-IL-01`

### Step 3: Click "Pair Device"

The gateway will:
1. Validate device ID format
2. Check if device already paired
3. Send `PAIR` command via LoRa
4. Wait for device response (LED changes color + beep)
5. Save pairing to FFat storage

---

## Device Pairing Process

### Gateway Side:
```
1. User enters device ID in web interface
2. Gateway validates format (EDy-XXXXX)
3. Gateway generates temporary secret
4. Gateway sends LoRa message: "GW01:PAIR:ED1-A3F2B:000:timestamp:null"
5. Gateway adds device to list
6. Gateway saves to /device_secrets.json
```

### Device Side (RPi Zero):
```
1. RPi receives PAIR command via LoRa
2. RPi changes NeoPixel to BLUE
3. RPi beeps buzzer
4. RPi responds: "ED1-A3F2B:PAIR_ACK:GW01:timestamp"
5. RPi saves gateway ID to config
```

### Completion:
```
- Gateway receives PAIR_ACK
- Gateway marks device as "paired"
- Device appears in dashboard table
- Device ready for polling
```

---

## API Endpoints

### GET /api/devices
Returns list of all paired devices

**Response:**
```json
{
  "devices": [
    {
      "device_id": "ED1-A3F2B",
      "paired": true,
      "online": true,
      "battery": 95,
      "rssi": -45,
      "phase": "IDLE"
    }
  ]
}
```

### GET /api/polling
Returns current polling status

**Response:**
```json
{
  "polling_active": true,
  "current_device_index": 2,
  "total_devices": 5,
  "elapsed_ms": 125000,
  "current_device_id": "ED3-F7A2C",
  "current_phase": "DATA_COLLECTION"
}
```

### POST /api/poll/start
Manually start a polling cycle

**Response:**
```json
{
  "status": "started"
}
```

**Error (if already polling):**
```json
{
  "error": "polling already active"
}
```

### POST /api/device/pair
Pair a new device

**Request Body:**
```json
{
  "device_id": "ED1-A3F2B",
  "table_left": "BLR-13-IL-02",
  "table_right": "BLR-13-IL-01"
}
```

**Success Response:**
```json
{
  "success": true,
  "message": "Pairing command sent"
}
```

**Error Responses:**
```json
{
  "success": false,
  "error": "Invalid device ID format"
}
```

```json
{
  "success": false,
  "error": "Device already paired"
}
```

```json
{
  "success": false,
  "error": "Maximum devices reached (15)"
}
```

### POST /api/device/remove
Remove a paired device

**Request Body:**
```json
{
  "device_id": "ED1-A3F2B"
}
```

**Success Response:**
```json
{
  "success": true,
  "message": "Device removed"
}
```

---

## WebSocket Real-Time Updates

**Endpoint:** `ws://<gateway-ip>/ws`

### Messages Sent to Client:

**Polling Status Update:**
```json
{
  "polling_active": true,
  "current_device_index": 1,
  "total_devices": 5,
  "current_device_id": "ED2-C4D1E",
  "current_phase": "HEALTH_CHECK"
}
```

**Device List Update:**
```json
{
  "devices": [...]
}
```

**System Log:**
```json
{
  "log": "Device ED1-A3F2B paired successfully"
}
```

### Commands from Client:

**Start Polling:**
```json
{
  "command": "start_polling"
}
```

**Get Status:**
```json
{
  "command": "get_status"
}
```

---

##Device ID Format Rules

### Valid Format: `EDy-XXXXX`

**Components:**
- `ED` - Fixed prefix for "Edge Device"
- `y` - Single digit (0-9) for device number
- `-` - Separator
- `XXXXX` - 5 hexadecimal characters (0-9, A-F)

### Examples:

✅ **Valid:**
- `ED1-A3F2B`
- `ED2-C4D1E`
- `ED3-0F7A8`
- `ED9-FFFFF`

❌ **Invalid:**
- `ED10-A3F2B` (y must be single digit)
- `ED1-A3F` (too few hex characters)
- `ED1-G3F2B` (G is not valid hex)
- `D1-A3F2B` (missing ED prefix)

---

## Color Coding

### LED Status:
- **Blue:** Initializing
- **Green:** WiFi connected
- **Cyan:** WiFi + MQTT connected (normal operation)
- **Yellow:** WiFi OK, MQTT failed
- **Red:** Device offline detected

### Device Status in Table:
- **🟢 ONLINE:** Device responsive
- **🔴 OFFLINE:** Device not responding
- **🟡 POLLING:** Currently being polled

---

## Troubleshooting

### Cannot Access Web Interface

**Symptoms:**
- Browser shows "Connection refused"

**Solutions:**
1. Check gateway is powered on
2. Verify you're on the same network
3. Check serial monitor for IP address
4. Try: `http://<IP>` (not `https://`)

### Device Pairing Fails

**Symptoms:**
- "Pairing command sent" but device not added

**Solutions:**
1. Check device is powered and LoRa configured
2. Verify device ID format is correct
3. Check LoRa module receiving (serial monitor)
4. Ensure device is within LoRa range

### WebSocket Disconnects

**Symptoms:**
- Real-time updates stop working

**Solutions:**
1. Refresh browser page
2. Check network stability
3. Monitor serial for WebSocket errors

---

## File Storage

### NVS (Non-Volatile Storage)
Stores gateway configuration:
- `gateway_id`
- `building`, `floor`, `lab`
- `poll_interval`
- `num_devices`

### FFat Filesystem
Stores device data:
- `/device_secrets.json` - Device pairings and secrets
- `/report_TIMESTAMP.csv` - Polling cycle reports

---

## Security Notes

### Current Setup:
- HTTP Basic Authentication (username/password)
- Not HTTPS (unencrypted)

### For Production:
Consider adding:
- HTTPS with SSL certificates
- Token-based authentication
- CORS restrictions
- Rate limiting

---

## Browser Compatibility

✅ **Tested:**
- Chrome 90+
- Firefox 88+
- Edge 90+
- Safari 14+

⚠️ **Note:** Requires JavaScript enabled

---

## Development Tips

### Debugging WebSocket:
Open browser console (F12) and check:
```javascript
ws.readyState
// 0 = CONNECTING
// 1 = OPEN
// 2 = CLOSING
// 3 = CLOSED
```

### Testing API Endpoints:
```bash
# Get device list
curl -u rnd:rnd http://192.168.1.150/api/devices

# Start polling
curl -u rnd:rnd -X POST http://192.168.1.150/api/poll/start

# Pair device
curl -u rnd:rnd -X POST http://192.168.1.150/api/device/pair \
  -H "Content-Type: application/json" \
  -d '{"device_id":"ED1-A3F2B","table_left":"BLR-13-IL-02","table_right":"BLR-13-IL-01"}'
```

---

## Future Enhancements

Planned features:
- ⏳ Device configuration editor
- ⏳ Historical data charts
- ⏳ CSV report download from web UI
- ⏳ Gateway settings page
- ⏳ Firmware OTA updates
- ⏳ Multi-gateway support

---

© 2025 DETECTRA System
