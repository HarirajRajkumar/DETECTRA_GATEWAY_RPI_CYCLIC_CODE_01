# DETECTRA Gateway v2.0 - Sequential Polling Architecture

## Overview

Gateway v2.0 implements **sequential master-slave polling** for RPi Zero edge devices using a custom LoRa protocol with HMAC-SHA256 authentication.

### Architecture Change from v1.7/v1.8

**v1.7/v1.8 (Broadcast):**
```
Edge Devices → Gateway (receives broadcasts) → MQTT → PC
```

**v2.0 (Sequential Polling):**
```
Gateway (Master) ⟷ Device 1 (4-phase protocol) ⟶ MQTT → PC
                 ⟷ Device 2 (4-phase protocol) ⟶ MQTT → PC
                 ⟷ Device 3 (4-phase protocol) ⟶ MQTT → PC
                 ... (up to 15 devices)
```

---

## Key Features

### ✅ Sequential Polling Protocol
- Gateway initiates all communication
- One device polled at a time
- 4-phase protocol per device:
  1. **Health Check** (POLL → ACK:ONLINE)
  2. **Start Inference** (START_INFER → ACK:INFERRING)
  3. **Data Collection** (5× DATA → ACK)
  4. **Finalize** (FINALIZE → ACK:FINALIZED, SLEEP → ACK:SLEEPING)

### ✅ HMAC-SHA256 Authentication
- Every message authenticated with HMAC
- Device-specific shared secrets
- Prevents message spoofing
- Timestamp validation (±60 seconds)

### ✅ Dual LoRa Modules
- LoRa Module 1: Devices 1-15 (GPIO 43/44)
- LoRa Module 2: Devices 16-30 (GPIO 17/18) - Future expansion
- Load balancing across modules

### ✅ Device Pairing System
- QR code generation for easy device configuration
- Shared secret storage in FFat filesystem
- Automatic device discovery

### ✅ Retry Logic
- Up to 3 retries per phase
- Exponential backoff (2s, 4s, 8s)
- Graceful handling of offline devices

### ✅ Web Interface
- Real-time polling progress via WebSocket
- Device status dashboard
- Manual polling trigger
- Report downloads

### ✅ MQTT Publishing
- Gateway status (every 30s)
- Polling progress updates
- Individual device data
- Cycle completion reports

### ✅ FFat Storage
- Device pairing secrets
- CSV reports per cycle
- Configuration persistence

---

## Communication Protocol

### Message Format

```
SENDER_ID:COMMAND:TARGET_ID:SEQUENCE:TIMESTAMP:PAYLOAD:HMAC
```

**Example:**
```
GW01:POLL:D1:001:1728567890:null:a3f2b1c4d5e6f7a8
```

### Commands

**Gateway → Device:**
- `POLL` - Health check request
- `START_INFER` - Begin inference
- `ACK` - Acknowledge data received
- `FINALIZE` - Complete polling cycle
- `SLEEP` - Enter RX listening mode

**Device → Gateway:**
- `ACK` - Response with status (ONLINE, INFERRING, FINALIZED, SLEEPING)
- `DATA` - Inference results

---

## 4-Phase Protocol Flow

### Example: Complete Polling Cycle for Device 1

```
Time    Direction    Message
──────────────────────────────────────────────────────────────────────────────
00:00   GW→D1        GW01:POLL:D1:001:1728567890:null:a3f2b1c4d5e6f7a8
00:02   D1→GW        D1:ACK:GW01:ONLINE:001:1728567892:bat_95:rssi_-45:snr_8:b4c3a2f1

00:03   GW→D1        GW01:START_INFER:D1:002:1728567893:null:c5d4e3f2a1b0
00:04   D1→GW        D1:ACK:GW01:INFERRING:002:1728567894:null:d6e5f4a3b2c1

        [Device captures image and runs YOLOv9 inference - ~20 seconds per position]

00:24   D1→GW        D1:DATA:003:1/5:1728567914:BLR-13-IL-01:left:motherboard:40%,led_on:50%:e7f6a5b4c3d2
00:25   GW→D1        GW01:ACK:D1:003:1728567915:1/5:f8a7b6c5d4e3

00:44   D1→GW        D1:DATA:004:2/5:1728567934:BLR-13-IL-01:left_center:motherboard:42%:a1b2c3d4e5f6
00:45   GW→D1        GW01:ACK:D1:004:1728567935:2/5:b2c3d4e5f6a7

01:04   D1→GW        D1:DATA:005:3/5:1728567954:BLR-13-IL-02:center:motherboard:45%,led_on:55%:c3d4e5f6a7b8
01:05   GW→D1        GW01:ACK:D1:005:1728567955:3/5:d4e5f6a7b8c9

01:24   D1→GW        D1:DATA:006:4/5:1728567974:BLR-13-IL-02:center_right:motherboard:43%:e5f6a7b8c9d0
01:25   GW→D1        GW01:ACK:D1:006:1728567975:4/5:f6a7b8c9d0e1

01:44   D1→GW        D1:DATA:007:5/5:1728567994:BLR-13-IL-02:right:motherboard:38%:COMPLETE:a7b8c9d0e1f2
01:45   GW→D1        GW01:ACK:D1:007:1728567995:5/5:b8c9d0e1f2a3

01:47   GW→D1        GW01:FINALIZE:D1:008:1728567997:null:c9d0e1f2a3b4
01:48   D1→GW        D1:ACK:GW01:FINALIZED:008:1728567998:null:d0e1f2a3b4c5

01:49   GW→D1        GW01:SLEEP:D1:009:1728567999:null:e1f2a3b4c5d6
01:50   D1→GW        D1:ACK:GW01:SLEEPING:009:1728568000:null:f2a3b4c5d6e7
```

**Total Time per Device:** ~2.4 minutes (144 seconds)
**Full Cycle (15 devices):** ~36 minutes

---

## Hardware Requirements

### ESP32-S3 Development Board
- ESP32-S3-WROOM-1-N16R8
- Dual-core, 16MB Flash, 8MB PSRAM

### LoRa Modules (Dual RAK3172)
- 2× RAK3172 (UART interface)
- Module 1: GPIO 43 (TX), 44 (RX)
- Module 2: GPIO 17 (TX), 18 (RX)
- Configuration: 868 MHz, SF9, BW125, P2P mode

### OLED Display
- SSD1306 128×32 (I2C)
- SDA: GPIO 38
- SCL: GPIO 37

### Status Indicators
- NeoPixel LED: GPIO 8
- Buzzer: GPIO 5

### Pin Configuration Summary
```cpp
LORA1_TX      43
LORA1_RX      44
LORA2_TX      17
LORA2_RX      18
OLED_SDA      38
OLED_SCL      37
NEOPIXEL_PIN  8
BUZZER_PIN    5
```

---

## Software Requirements

### Arduino IDE Setup

1. **Install ESP32 Board Support**
   - File → Preferences
   - Additional Board URLs: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Tools → Board → Boards Manager → "ESP32" → Install

2. **Required Libraries**

   Via Arduino Library Manager (Tools → Manage Libraries):
   - `PubSubClient` 2.8.0 (MQTT)
   - `Adafruit SSD1306` 2.5.7 (OLED)
   - `Adafruit GFX Library` 1.11.5 (Graphics)
   - `Adafruit NeoPixel` 1.11.0 (LED)
   - `ArduinoJson` 6.21.3 (JSON)
   - `ESPAsyncWebServer` (via GitHub)
   - `AsyncTCP` (via GitHub)

3. **Board Settings**
   ```
   Board: "ESP32S3 Dev Module"
   Upload Speed: 921600
   USB Mode: "Hardware CDC and JTAG"
   USB CDC On Boot: "Enabled"
   CPU Frequency: "240MHz (WiFi)"
   Flash Mode: "QIO 80MHz"
   Flash Size: "16MB (128Mb)"
   Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"
   PSRAM: "QSPI PSRAM"
   Arduino Runs On: "Core 1"
   Events Run On: "Core 1"
   ```

---

## Configuration

### 1. WiFi Credentials

Edit lines 66-67:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 2. MQTT Broker

**⚠️ IMPORTANT:** Set to your PC's IP address (NOT localhost)

Edit lines 70-74:
```cpp
const char* mqtt_server = "192.168.1.100";   // Your PC's IP
const int mqtt_port = 1883;
const char* mqtt_client_id = "DETECTRA_GW01";
const char* mqtt_username = "";               // Optional
const char* mqtt_password = "";               // Optional
```

**Find your PC's IP:**
- Windows: `ipconfig`
- Linux: `ip addr show`
- macOS: `ifconfig`

### 3. Gateway ID and Location

Edit via NVS Preferences or web interface:
```cpp
// Stored in NVS
gateway_id: "GW01"
building: "BLR"
floor: "13"
lab: "Innovation Lab"
polling_interval: 60  // minutes
```

### 4. Web Server Credentials

Edit lines 79-80:
```cpp
const char* web_username = "rnd";
const char* web_password = "rnd";
```

---

## Device Pairing

### Step 1: Generate Device Secret (on PC)

```python
import hashlib
import json

device_id = "D1"
shared_secret = hashlib.sha256(f"DETECTRA_{device_id}".encode()).hexdigest()[:32]

print(f"Device ID: {device_id}")
print(f"Shared Secret: {shared_secret}")
```

### Step 2: Create device_secrets.json

Upload to gateway FFat filesystem at `/device_secrets.json`:

```json
{
  "gateway_id": "GW01",
  "devices": [
    {
      "device_id": "D1",
      "shared_secret": "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6",
      "table_left": "BLR-13-IL-02",
      "table_right": "BLR-13-IL-01"
    },
    {
      "device_id": "D2",
      "shared_secret": "b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7",
      "table_left": "BLR-13-IL-04",
      "table_right": "BLR-13-IL-03"
    }
  ]
}
```

### Step 3: Configure RPi Zero Device

On RPi Zero, create `/home/pi/detectra_config.json`:

```json
{
  "device_id": "D1",
  "gateway_id": "GW01",
  "shared_secret": "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6",
  "table_left": "BLR-13-IL-02",
  "table_right": "BLR-13-IL-01",
  "lora_freq": "868000000",
  "lora_sf": "9",
  "lora_bw": "125"
}
```

---

## Installation

### Step 1: Upload Firmware

1. Connect ESP32-S3 via USB
2. Open `gateway_v2_0_sequential_polling.ino` in Arduino IDE
3. Update WiFi and MQTT settings
4. Select correct board and port
5. Click **Upload** (→ button)

### Step 2: Upload FFat Files

1. Install **ESP32 Sketch Data Upload** plugin
2. Create `data/` folder in sketch directory
3. Add `device_secrets.json`
4. Tools → ESP32 Sketch Data Upload

### Step 3: Verify Operation

**Expected Serial Output:**
```
==========================================
 DETECTRA GATEWAY v2.0
 Sequential Polling Architecture
==========================================

[INIT] Initializing hardware...
[INIT] OLED initialized
[INIT] Hardware ready
[LORA] Initializing LoRa modules...
[LORA1] Configuring P2P mode...
[LORA1] LoRa Module 1 configured
[WIFI] Connecting to YOUR_SSID.......... Connected! IP: 192.168.1.150
[MQTT] Configured for 192.168.1.100:1883
[MQTT] Connecting... Connected!
[WEB] Initializing web server...
[WEB] Server started on port 80
[WEB] Access: http://192.168.1.150
[FFAT] Initializing filesystem...
[FFAT] Filesystem mounted
[CONFIG] Configuration loaded
  Gateway ID: GW01
  Location: BLR-13-Innovation Lab
  Devices: 3
[CONFIG] Loaded device: D1
[CONFIG] Loaded device: D2
[CONFIG] Loaded device: D3
[LORA TASK] Started on Core 0
[POLLING TASK] Started on Core 1

==========================================
 Gateway Initialized Successfully
 Gateway ID: GW01
 Devices Paired: 3
 Polling Interval: 60 minutes
==========================================

==========================================
 Starting Polling Cycle
 Devices: 3
==========================================

>>> Polling Device: D1 (1/3)
[LORA1] TX: GW01:POLL:D1:001:1728567890:null:a3f2b1c4d5e6f7a8
[LORA1] RX: +EVT:RXP2P:RSSI -52:SNR 8:D1:ACK:GW01:ONLINE:001:1728567892:bat_95:rssi_-45:snr_8:b4c3a2f1
[PROTOCOL] ✓ HMAC verified for D1
[PROTOCOL] ✓ Device ONLINE: D1
  Battery: 95%
  RSSI: -45 dBm
  SNR: 8 dB
[POLLING] → START_INFERENCE
...
```

---

## Operation

### Polling Cycle

1. **Automatic Polling**
   - Starts automatically on boot
   - Repeats every N minutes (configurable)

2. **Manual Polling**
   - Via web interface: `POST /api/poll/start`
   - Via WebSocket: `{"command": "start_polling"}`

3. **Cycle Completion**
   - CSV report generated in FFat
   - MQTT message published
   - WebSocket notification sent

### LED Status Codes

| Color | Meaning |
|-------|---------|
| Blue | Initializing |
| Green | Device online / successful |
| Cyan | WiFi + MQTT connected |
| Yellow | WiFi OK, MQTT failed |
| Red | Device offline / error |

### Buzzer Alerts

- **Short beep (100ms):** Startup
- **Long beep (500ms):** Device offline detected

---

## MQTT Message Formats

### Topic: `detectra/GW01/status` (Every 30 seconds)

```json
{
  "gateway_id": "GW01",
  "wifi_connected": true,
  "mqtt_connected": true,
  "polling_active": true,
  "devices_paired": 3,
  "total_messages": 142,
  "successful_polls": 38,
  "failed_polls": 2,
  "uptime_ms": 3600000,
  "ip_address": "192.168.1.150",
  "location": {
    "building": "BLR",
    "floor": "13",
    "lab": "Innovation Lab"
  }
}
```

### Topic: `detectra/GW01/polling` (During polling)

```json
{
  "polling_active": true,
  "current_device_index": 0,
  "total_devices": 3,
  "elapsed_ms": 45230,
  "current_device_id": "D1",
  "current_phase": "DATA_COLLECTION"
}
```

### Topic: `detectra/GW01/device/D1` (Per device)

```json
{
  "gateway_id": "GW01",
  "device_id": "D1",
  "online": true,
  "battery": 95,
  "rssi": -45,
  "snr": 8,
  "last_position": "right",
  "last_table_id": "BLR-13-IL-01",
  "last_detections": "motherboard:38%",
  "positions_received": 5,
  "total_polls": 12,
  "successful_polls": 11,
  "failed_polls": 1,
  "last_contact": 1728568000,
  "timestamp": 1728568050
}
```

### Topic: `detectra/GW01/data` (Cycle complete)

```json
{
  "gateway_id": "GW01",
  "cycle_complete": true,
  "duration_ms": 432000,
  "devices_polled": 3,
  "successful": 3,
  "failed": 0,
  "timestamp": 1728568432
}
```

---

## Web Interface

### Access

```
http://<gateway-ip>/
Username: rnd
Password: rnd
```

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Dashboard (HTML) |
| `/api/devices` | GET | Get device list (JSON) |
| `/api/polling` | GET | Get polling status (JSON) |
| `/api/poll/start` | POST | Start manual polling |

### WebSocket Updates

Connect to `ws://<gateway-ip>/ws` for real-time polling progress.

**Message format:**
```json
{
  "polling_active": true,
  "current_device_id": "D2",
  "current_phase": "DATA_COLLECTION",
  "progress_percent": 66
}
```

---

## Troubleshooting

### Issue 1: LoRa Module Not Responding

**Symptoms:**
- No `[LORA1] RX:` messages
- Polling timeouts

**Solutions:**
1. Check UART connections (TX/RX swap?)
2. Verify RAK3172 is powered
3. Test manually:
   ```cpp
   LoRa1.println("AT");
   delay(100);
   while (LoRa1.available()) {
     Serial.write(LoRa1.read());
   }
   ```
4. Check LoRa configuration matches device

### Issue 2: HMAC Verification Failed

**Symptoms:**
- `[PROTOCOL] ✗ HMAC verification failed!`

**Solutions:**
1. Verify shared secret matches on gateway and device
2. Check timestamp synchronization (±60 seconds tolerance)
3. Ensure message format is correct

### Issue 3: Device Always Timeout

**Symptoms:**
- `[POLLING] ⚠ Timeout in phase: HEALTH_CHECK`
- `[POLLING] ✗ Max retries reached`

**Solutions:**
1. Check device is powered and LoRa configured
2. Verify device is listening (RX mode)
3. Check LoRa frequency/SF/BW match
4. Increase timeout values:
   ```cpp
   #define TIMEOUT_HEALTH_CHECK  30000  // Increase to 30s
   ```

### Issue 4: FFat Mount Failed

**Symptoms:**
- `[FFAT] Failed to mount FFat!`

**Solutions:**
1. Use correct partition scheme: "16M Flash (3MB APP/9.9MB FATFS)"
2. Format FFat via code:
   ```cpp
   FFat.format();
   FFat.begin(true);  // true = format if needed
   ```

---

## Performance Optimization

### Reduce Polling Time

**Option 1: Fewer positions (3 instead of 5)**
- Modify device firmware to send only left/center/right
- Update `device.positionsReceived >= 3` check

**Option 2: Parallel inference (future)**
- Use multiple gateways
- Assign device groups to each gateway

### Increase Reliability

**Option 1: Adjust retry strategy**
```cpp
#define MAX_RETRIES           5
#define RETRY_DELAY_BASE      1000  // Faster retries
```

**Option 2: Increase LoRa power**
```cpp
#define LORA_PWR      "22"  // Maximum 22 dBm
```

---

## Development Roadmap

### Phase 1 (Current)
- ✅ Basic sequential polling
- ✅ HMAC authentication
- ✅ Single LoRa module (15 devices)
- ✅ MQTT publishing

### Phase 2 (Next)
- ⏳ Dual LoRa module support (30 devices)
- ⏳ Complete web dashboard with live updates
- ⏳ QR code device pairing wizard
- ⏳ RPi Zero device firmware

### Phase 3 (Future)
- ⏳ NTP time synchronization
- ⏳ OTA firmware updates
- ⏳ Advanced diagnostics
- ⏳ Cloud integration

---

## Differences from v1.7/v1.8

| Feature | v1.7/v1.8 | v2.0 |
|---------|-----------|------|
| Protocol | Broadcast | Sequential Polling |
| Security | None | HMAC-SHA256 |
| Device Control | Passive | Active (Master-Slave) |
| Scalability | Limited | 15-30 devices |
| Device Status | Inferred | Explicit (4-phase) |
| Reliability | Lower | Higher (retry logic) |
| Power Management | Always on | Controlled RX/TX |

---

## Support

### Resources
- [LoRa Protocol Guide](../../../LORA_gateway_system_guide_V0_01.md)
- [System Architecture](../../../SYSTEM_ARCHITECTURE_V_0_01.md)
- [RPi Zero Device Firmware](../../../RPI_ZERO_DETECTRA/)

### Common Commands

```bash
# Monitor MQTT traffic
mosquitto_sub -h localhost -t "detectra/GW01/#" -v

# Test MQTT connection
mosquitto_pub -h localhost -t "detectra/test" -m "hello"

# Check ESP32 port
ls /dev/ttyUSB* /dev/ttyACM*  # Linux
# Device Manager → Ports (COM & LPT)  # Windows

# Monitor serial output
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

---

## License

DETECTRA System - Industrial IoT Object Detection Platform
© 2025 All Rights Reserved
