# DETECTRA_GATEWAY_RPI_CYCLIC_CODE_01

# DETECTRA SYSTEM - Complete Documentation

## 🏢 Project Overview

**Detectra** is an IoT-based object detection and monitoring system designed for warehouse and industrial environments. The system uses edge AI devices (ESP32-S3 or Raspberry Pi Zero) with cameras for object detection, communicating via LoRa to a central gateway, which then reports to a PC application for real-time monitoring and planogram visualization.

---

## 📋 Table of Contents

1. [System Architecture](#system-architecture)
2. [Hardware Components](#hardware-components)
3. [LoRa Communication Protocol](#lora-communication-protocol)
4. [Encryption & Security](#encryption--security)
5. [Sequential Polling Strategy](#sequential-polling-strategy)
6. [Message Formats](#message-formats)
7. [Software Architecture](#software-architecture)
8. [Installation & Setup](#installation--setup)
9. [API Documentation](#api-documentation)
10. [Troubleshooting](#troubleshooting)

---

## 🏗️ System Architecture

### Overall System Topology

```
┌─────────────────────────────────────────────────────────────────┐
│                    DETECTRA SYSTEM ARCHITECTURE                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  LAYER 1: Edge Detection Layer (30 devices per gateway)        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │   Device 1   │  │   Device 2   │  │  Device 30   │         │
│  │              │  │              │  │              │         │
│  │ Camera +     │  │ Camera +     │  │ Camera +     │         │
│  │ ESP32-S3/RPi │  │ ESP32-S3/RPi │  │ ESP32-S3/RPi │         │
│  │ LoRa TX/RX   │  │ LoRa TX/RX   │  │ LoRa TX/RX   │         │
│  │              │  │              │  │              │         │
│  │ Detection:   │  │ Detection:   │  │ Detection:   │         │
│  │ Every 4 hrs  │  │ Every 4 hrs  │  │ Every 4 hrs  │         │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘         │
│         │                  │                  │                 │
│         └──────────────────┴──────────────────┘                 │
│                            │                                    │
│                   LoRa P2P Communication                        │
│                   Frequency: 866 MHz                            │
│                   Range: 1-2 km (urban)                         │
│                   Bandwidth: 125 kHz                            │
│                   Spreading Factor: 9                           │
│                            │                                    │
│                            ▼                                    │
│  ────────────────────────────────────────────────────────────  │
│  LAYER 2: Gateway Layer                                        │
│  ┌────────────────────────────────────────────────────┐        │
│  │          ESP32-S3 Gateway (GW001)                  │        │
│  │  ┌──────────────┐        ┌──────────────┐         │        │
│  │  │   LoRa 1     │        │   LoRa 2     │         │        │
│  │  │  RAK3172     │        │  RAK3172     │         │        │
│  │  │  (RX/TX)     │        │  (RX/TX)     │         │        │
│  │  └──────────────┘        └──────────────┘         │        │
│  │                                                    │        │
│  │  Functions:                                        │        │
│  │  • Poll devices sequentially (1-30)               │        │
│  │  • Decrypt received data                          │        │
│  │  • Validate message integrity                     │        │
│  │  • Aggregate detection results                    │        │
│  │  • Forward to PC via WiFi/MQTT                    │        │
│  │                                                    │        │
│  │  ┌──────────────┐                                 │        │
│  │  │ WiFi Client  │  → Router: 192.168.1.x          │        │
│  │  └──────────────┘                                 │        │
│  └────────────────────┬───────────────────────────────┘        │
│                       │                                        │
│                       │ MQTT Protocol                          │
│                       │ Topic: detectra/GW001/#                │
│                       │                                        │
│                       ▼                                        │
│  ────────────────────────────────────────────────────────────  │
│  LAYER 3: Application Layer                                   │
│  ┌────────────────────────────────────────────────────┐        │
│  │              PC / Server                           │        │
│  │                                                    │        │
│  │  ┌──────────────────┐                             │        │
│  │  │  MQTT Broker     │  (Mosquitto)                │        │
│  │  │  Port: 1883      │                             │        │
│  │  └────────┬─────────┘                             │        │
│  │           │                                        │        │
│  │           ▼                                        │        │
│  │  ┌──────────────────┐                             │        │
│  │  │ Python App       │                             │        │
│  │  │ (Tkinter GUI)    │                             │        │
│  │  │                  │                             │        │
│  │  │ • Planogram View │                             │        │
│  │  │ • Device Status  │                             │        │
│  │  │ • Alerts         │                             │        │
│  │  │ • History        │                             │        │
│  │  │ • Reports        │                             │        │
│  │  └────────┬─────────┘                             │        │
│  │           │                                        │        │
│  │           ▼                                        │        │
│  │  ┌──────────────────┐                             │        │
│  │  │ SQLite Database  │                             │        │
│  │  │ detectra.db      │                             │        │
│  │  └──────────────────┘                             │        │
│  └────────────────────────────────────────────────────┘        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### System Components

| Layer | Component | Quantity | Function |
|-------|-----------|----------|----------|
| **Edge** | ESP32-S3 + Camera / RPi Zero | 30 per gateway | Object detection, LoRa communication |
| **Gateway** | ESP32-S3 with dual LoRa | 1 per 30 devices | LoRa↔WiFi bridge, data aggregation |
| **Application** | PC/Server with MQTT | 1 | Monitoring, visualization, database |

---

## 🔧 Hardware Components

### Edge Detection Device (Two Options)

#### **Option A: ESP32-S3 Based (Lower Power)**

| Component | Part Number | Specification | Purpose |
|-----------|-------------|---------------|---------|
| MCU | ESP32-S3-WROOM-1-N16R8 | 240MHz, 16MB Flash, 8MB PSRAM | Main processor |
| Camera | OV2640 / OV5640 | 2MP / 5MP | Image capture |
| LoRa Module | RAK3172 | 866MHz, EU868 | Communication |
| Power | 18650 Battery + Solar | 3.7V, 3000mAh | Power supply |

**Pros:**
- Low power consumption (200-300mA)
- Compact size
- Lower cost (~$15 per unit)

**Cons:**
- Limited AI capability (96x96 to 416x416 max)
- Lower accuracy (60-75%)
- Requires ESP-DL model training

#### **Option B: Raspberry Pi Zero 2W (Higher Performance)**

| Component | Part Number | Specification | Purpose |
|-----------|-------------|---------------|---------|
| SBC | Raspberry Pi Zero 2W | Quad-core 1GHz, 512MB RAM | Main processor |
| Camera | Pi Camera Module v2 | 8MP | Image capture |
| LoRa | RAK3172 USB Adapter | 866MHz, EU868 | Communication |
| Power | 18650 Battery + Solar | 3.7V, 5000mAh | Power supply |

**Pros:**
- Full YOLOv9 support
- High accuracy (85-95%)
- Easier development
- Proven performance

**Cons:**
- Higher power (500-800mA)
- Larger size
- Higher cost (~$35 per unit)

**Recommendation:** Use **Raspberry Pi Zero 2W** for production, ESP32-S3 for cost-sensitive deployments.

---

### Gateway Device

#### **ESP32-S3 Gateway Specifications**

| Component | Part Number | Specification |
|-----------|-------------|---------------|
| MCU | ESP32-S3-WROOM-1-N16R8 | 240MHz, 16MB Flash, 8MB PSRAM |
| LoRa Module 1 | RAK3172 | UART, 866MHz (for Devices 1-15) |
| LoRa Module 2 | RAK3172 | UART, 866MHz (for Devices 16-30) |
| Display | OLED 0.91" 128x32 | I2C, Status display |
| Buzzer | 12mm Active Buzzer | Status indication |
| NeoPixel | WS2812B | RGB status LED |
| WiFi | Built-in ESP32-S3 | 2.4GHz, connects to router |
| Power | USB-C / DC 5V | Continuous power required |

**Why Dual LoRa Modules?**
- Prevents collision between devices
- Better reliability
- Load balancing (15 devices per module)

---

## 📡 LoRa Communication Protocol

### Communication Strategy: **Sequential Master-Slave Polling**

The gateway acts as **Master**, devices act as **Slaves**. Gateway polls each device one-by-one in sequential order.

### LoRa Configuration

```cpp
// RAK3172 LoRa Settings
AT+NWM=0                              // P2P mode (not LoRaWAN)
AT+P2P=866000000:9:125:1:8:14        // Frequency:SF:BW:CR:Preamble:Power
```

| Parameter | Value | Description |
|-----------|-------|-------------|
| **Frequency** | 866 MHz | EU868 band (adjust for region) |
| **Spreading Factor** | 9 | Balance between range and speed |
| **Bandwidth** | 125 kHz | Standard LoRa bandwidth |
| **Coding Rate** | 4/5 (1) | Error correction |
| **Preamble Length** | 8 | Sync preamble |
| **TX Power** | 14 dBm | ~25mW transmission power |

**Time on Air (ToA) Calculation:**
- 50-byte packet @ SF9, BW125, CR4/5 ≈ **200-300ms**
- 200-byte packet ≈ **800-1000ms**

---

### Sequential Polling Flow

```
┌─────────────────────────────────────────────────────────────┐
│              SEQUENTIAL POLLING CYCLE                       │
│                  (Every 4 hours)                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  T+0:00    Gateway → Device 1:  "POLL" (encrypted)         │
│            Wait for response (timeout: 5 sec)               │
│                                                             │
│  T+0:01    Device 1 → Gateway:  "DATA" (detection result)  │
│                                                             │
│  T+0:02    Gateway → Device 1:  "ACK" (confirmation)       │
│            [Device 1 goes to sleep for 4 hours]            │
│                                                             │
│  T+0:03    Gateway → Device 2:  "POLL"                     │
│            Wait for response...                             │
│                                                             │
│  T+0:04    Device 2 → Gateway:  "DATA"                     │
│                                                             │
│  T+0:05    Gateway → Device 2:  "ACK"                      │
│            [Device 2 goes to sleep]                         │
│                                                             │
│  T+0:06    Gateway → Device 3:  "POLL"                     │
│            ...                                              │
│                                                             │
│  (Continue for all 30 devices)                             │
│                                                             │
│  T+1:30    All devices polled (30 × 3 sec = 90 sec)       │
│                                                             │
│  T+1:35    Gateway aggregates all data                     │
│            Publishes to MQTT broker                         │
│                                                             │
│  T+4:00:00 Next polling cycle begins                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**Timing Per Device:**
- POLL message: 300ms
- Device processing: 500ms
- DATA response: 800ms (larger payload)
- ACK message: 300ms
- Buffer time: 1100ms
- **Total: ~3 seconds per device**

**Total Polling Time:** 30 devices × 3 sec = **90 seconds (1.5 minutes)**

---

## 🔐 Encryption & Security

### Security Requirements

1. **Confidentiality:** Prevent eavesdropping on detection data
2. **Authentication:** Verify message sender identity
3. **Integrity:** Detect tampering or corruption
4. **Replay Protection:** Prevent replay attacks
5. **Forward Secrecy:** Compromise of one key doesn't expose all messages

### Cryptographic Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                 KEY HIERARCHY SYSTEM                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  MASTER KEY (256-bit, stored securely)                     │
│  • Never transmitted                                        │
│  • Stored in encrypted flash                               │
│  • Used only for key derivation                            │
│                                                             │
│            │                                                │
│            ▼ PBKDF2 / HKDF Derivation                      │
│                                                             │
│  DEVICE-SPECIFIC KEYS (per device):                        │
│  ┌────────────────────────────────────────┐                │
│  │ Device 1:                              │                │
│  │   AES Key:  derive(MASTER, "DEV01")    │ 128-bit        │
│  │   HMAC Key: derive(MASTER, "DEV01-H")  │ 256-bit        │
│  └────────────────────────────────────────┘                │
│                                                             │
│  ┌────────────────────────────────────────┐                │
│  │ Device 2:                              │                │
│  │   AES Key:  derive(MASTER, "DEV02")    │                │
│  │   HMAC Key: derive(MASTER, "DEV02-H")  │                │
│  └────────────────────────────────────────┘                │
│                                                             │
│  ... (30 device key pairs)                                 │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Encryption Algorithm: AES-128-CBC + HMAC-SHA256

**Why AES-128-CBC?**
- ✅ Hardware accelerated on ESP32-S3
- ✅ Well-tested and secure
- ✅ Fast encryption/decryption
- ✅ Widely supported

**Why HMAC-SHA256?**
- ✅ Provides message authentication
- ✅ Detects tampering
- ✅ Industry standard

### Key Derivation Example

```cpp
// Pseudo-code for key derivation
uint8_t master_key[32] = {0x00, 0x11, ..., 0xFF};  // Securely stored
uint8_t device_id = 0x05;  // Device 5

// Derive AES key
uint8_t aes_key[16];
char salt_aes[16];
sprintf(salt_aes, "DEV%02d-AES", device_id);
pbkdf2_hmac_sha256(master_key, 32, salt_aes, strlen(salt_aes), 
                   10000, aes_key, 16);

// Derive HMAC key
uint8_t hmac_key[32];
char salt_hmac[16];
sprintf(salt_hmac, "DEV%02d-HMAC", device_id);
pbkdf2_hmac_sha256(master_key, 32, salt_hmac, strlen(salt_hmac), 
                   10000, hmac_key, 32);
```

---

## 📦 Message Formats

### Packet Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    LoRa PACKET FORMAT                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  [HEADER - 6 bytes, UNENCRYPTED]                           │
│  ┌──────┬──────┬──────┬──────┬──────┬──────┐              │
│  │ SYNC │ GW   │ DEV  │ TYPE │ SEQ  │ LEN  │              │
│  │ 0xAA │ ID   │ ID   │      │ NUM  │      │              │
│  │ 0x55 │ 1B   │ 1B   │ 1B   │ 1B   │ 1B   │              │
│  └──────┴──────┴──────┴──────┴──────┴──────┘              │
│                                                             │
│  [IV - 16 bytes]                                           │
│  Initialization Vector for AES-CBC                         │
│  • Generated randomly per message                          │
│  • Ensures same plaintext → different ciphertext          │
│                                                             │
│  [ENCRYPTED PAYLOAD - variable, max 200 bytes]            │
│  ┌────────────────────────────────────────────┐            │
│  │  PLAINTEXT (before encryption):            │            │
│  │  ┌──────┬──────┬──────────┬──────┬──────┐ │            │
│  │  │NONCE │TSTAMP│ PAYLOAD  │BATT │ CRC  │ │            │
│  │  │ 4B   │ 4B   │ var      │ 1B  │ 2B   │ │            │
│  │  └──────┴──────┴──────────┴──────┴──────┘ │            │
│  │                                            │            │
│  │  After AES-128-CBC encryption              │            │
│  │  (padded to 16-byte blocks)                │            │
│  └────────────────────────────────────────────┘            │
│                                                             │
│  [HMAC - 4 bytes]                                          │
│  Truncated HMAC-SHA256 signature                           │
│  • Authenticates: HEADER + IV + CIPHERTEXT                │
│  • Prevents tampering                                      │
│                                                             │
│  TOTAL SIZE: 6 + 16 + (payload) + 4 = 26 + payload bytes  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Message Types

| Type | Value | Direction | Description | Payload Size |
|------|-------|-----------|-------------|--------------|
| **POLL** | 0x20 | Gateway → Device | Request detection data | 12 bytes |
| **DATA** | 0x21 | Device → Gateway | Detection result | 50-180 bytes |
| **ACK** | 0x22 | Gateway → Device | Acknowledge receipt | 8 bytes |
| **ERROR** | 0xE0 | Bidirectional | Error notification | 20 bytes |
| **HELLO** | 0x01 | Device → Gateway | Device announcement | 16 bytes |
| **HELLO_ACK** | 0x02 | Gateway → Device | Gateway response | 16 bytes |

---

### Message Examples

#### 1. POLL Message (Gateway → Device)

```
PLAINTEXT PAYLOAD:
┌────────────────────────────────────────────┐
│ Nonce:     0x12345678  (4 bytes)           │
│ Timestamp: 1704067200  (4 bytes, Unix)     │
│ Timeout:   5           (1 byte, seconds)   │
│ Padding:   0x00...     (to 16-byte block)  │
│ CRC16:     0xABCD      (2 bytes)           │
└────────────────────────────────────────────┘

FULL PACKET (after encryption):
┌────────────────────────────────────────────┐
│ Header:    AA 55 01 05 20 8F 10            │
│            (Sync, GW=1, Dev=5, Type=POLL)  │
│ IV:        3F 8A 9B ... (16 bytes random)  │
│ Encrypted: E4 7D 2A ... (16 bytes)         │
│ HMAC:      9F 2C 3A 1B (4 bytes)           │
│                                            │
│ Total: 42 bytes                            │
└────────────────────────────────────────────┘
```

#### 2. DATA Message (Device → Gateway)

```
PLAINTEXT PAYLOAD:
┌────────────────────────────────────────────┐
│ Nonce:     0x12345678  (4 bytes, echoed)   │
│ Timestamp: 1704067205  (4 bytes)           │
│ Detection: "person:2,box:5,pallet:3"       │
│            (variable, ~30-150 bytes)       │
│ RSSI:      -52         (1 byte, dBm)       │
│ SNR:       8           (1 byte, dB)        │
│ Battery:   85          (1 byte, %)         │
│ CRC16:     0x7F3A      (2 bytes)           │
└────────────────────────────────────────────┘

FULL PACKET:
┌────────────────────────────────────────────┐
│ Header:    AA 55 05 01 21 C2 A0            │
│ IV:        7B 4F 8E ... (16 bytes)         │
│ Encrypted: 9A E3 7C ... (176 bytes)        │
│ HMAC:      2D 8F 1A 4E (4 bytes)           │
│                                            │
│ Total: ~202 bytes                          │
└────────────────────────────────────────────┘
```

#### 3. ACK Message (Gateway → Device)

```
PLAINTEXT PAYLOAD:
┌────────────────────────────────────────────┐
│ Nonce:     0x12345678  (4 bytes, echoed)   │
│ Status:    0x00        (1 byte, OK)        │
│ Sleep:     14400       (2 bytes, seconds)  │
│            (4 hours = 14400 sec)           │
│ CRC16:     0x3D2F      (2 bytes)           │
└────────────────────────────────────────────┘

FULL PACKET:
┌────────────────────────────────────────────┐
│ Header:    AA 55 01 05 22 4A 10            │
│ IV:        A2 6D 9F ... (16 bytes)         │
│ Encrypted: 7E B8 4C ... (16 bytes)         │
│ HMAC:      F3 9A 2C 7D (4 bytes)           │
│                                            │
│ Total: 42 bytes                            │
└────────────────────────────────────────────┘
```

---

## 🔄 Sequential Polling Strategy

### State Machine - Device Side

```
┌──────────────────────────────────────────────────────────┐
│              DEVICE STATE MACHINE                        │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  STATE 1: DEEP SLEEP                                     │
│  • Duration: 4 hours (14,400 seconds)                    │
│  • Power: <5mA (ESP32-S3), <1mA (RPi shutdown)          │
│  • RTC timer set to wake after 4 hours                   │
│  └───► Wake trigger: Timer expires                       │
│                                                          │
│  STATE 2: WAKE & INITIALIZE                              │
│  • Boot system (2-5 seconds)                             │
│  • Initialize LoRa module                                │
│  • Set RX mode: AT+PRECV=65535                          │
│  • Start listening for POLL message                      │
│  └───► Transition: LoRa ready                            │
│                                                          │
│  STATE 3: LISTEN FOR POLL                                │
│  • Wait for POLL message from gateway                    │
│  • Timeout: 30 seconds (if no POLL, go back to sleep)   │
│  • Check message: Is it for my Device ID?                │
│  └───► Trigger: POLL received with my ID                 │
│                                                          │
│  STATE 4: PERFORM DETECTION                              │
│  • Capture image from camera                             │
│  • Run inference (YOLOv9 / ESP-DL)                      │
│  • Format detection results                              │
│  • Read battery level                                    │
│  └───► Transition: Detection complete                    │
│                                                          │
│  STATE 5: SEND DATA                                      │
│  • Encrypt detection data                                │
│  • Calculate HMAC                                        │
│  • Send DATA message to gateway                          │
│  • Start ACK timeout timer (5 seconds)                   │
│  └───► Wait for ACK                                      │
│                                                          │
│  STATE 6: WAIT FOR ACK                                   │
│  ├─ ACK received ──► Go to DEEP SLEEP                   │
│  ├─ Timeout (5sec) ──► Retry (max 3 times)              │
│  └─ Max retries ──► Log error, go to DEEP SLEEP         │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### State Machine - Gateway Side

```
┌──────────────────────────────────────────────────────────┐
│             GATEWAY STATE MACHINE                        │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  STATE 1: IDLE                                           │
│  • WiFi connected                                        │
│  • MQTT connected                                        │
│  • Wait for scheduled poll time (every 4 hours)          │
│  └───► Trigger: Scheduled time reached                   │
│                                                          │
│  STATE 2: INITIALIZE POLLING CYCLE                       │
│  • Clear receive buffers                                 │
│  • Reset device counters                                 │
│  • Initialize both LoRa modules                          │
│  • Set current_device = 1                                │
│  └───► Transition: Start polling                         │
│                                                          │
│  STATE 3: POLL DEVICE [current_device]                   │
│  • Build POLL message for device                         │
│  • Encrypt with device-specific key                      │
│  • Select LoRa module:                                   │
│    - LoRa1 for devices 1-15                              │
│    - LoRa2 for devices 16-30                             │
│  • Send POLL message                                     │
│  • Start response timeout (5 seconds)                    │
│  └───► Wait for DATA response                            │
│                                                          │
│  STATE 4: WAIT FOR DATA                                  │
│  ├─ DATA received ──► PROCESS DATA                       │
│  ├─ Timeout (5sec) ──► RETRY or MARK FAILED             │
│  └─ Invalid data ──► RETRY or MARK FAILED               │
│                                                          │
│  STATE 5: PROCESS DATA                                   │
│  • Decrypt payload                                       │
│  • Verify HMAC                                           │
│  • Validate CRC                                          │
│  • Extract detection results                             │
│  • Store in buffer                                       │
│  • Send ACK to device                                    │
│  └───► Transition: Next device                           │
│                                                          │
│  STATE 6: NEXT DEVICE                                    │
│  • current_device++                                      │
│  • If current_device <= 30:                              │
│    └───► Go to STATE 3 (poll next device)               │
│  • If current_device > 30:                               │
│    └───► Go to STATE 7 (finalize)                        │
│                                                          │
│  STATE 7: FINALIZE POLLING CYCLE                         │
│  • Aggregate all received data                           │
│  • Identify missing/failed devices                       │
│  • Format JSON payload                                   │
│  • Publish to MQTT:                                      │
│    - Topic: detectra/GW001/poll_complete                 │
│    - Payload: All device data                            │
│  • Log statistics                                        │
│  └───► Return to IDLE                                    │
│                                                          │
│  RETRY LOGIC:                                            │
│  • Max retries per device: 3                             │
│  • Backoff: 2 sec, 4 sec, 8 sec                          │
│  • After max retries: Mark device as failed, continue    │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### Collision Avoidance

**Why sequential polling avoids collisions:**
1. Only ONE device transmits at a time
2. Gateway explicitly addresses each device
3. No contention for air


# Software Architecture
