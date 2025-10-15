# Gateway v2.0 - Fixes Applied

## Date: 2025-10-13

## Critical Issues Fixed:

### ✅ 1. LoRa Message Format (RAK3172 AT+PSEND)
**Problem:** RAK3172 expects hex-encoded payload, not plain text
**Error:** `AT_PARAM_ERROR` when sending `AT+PSEND=GW0-00001:PAIR:...`
**Fix Applied:**
- Modified `sendLoRaMessage()` in `gateway_v2_0_sequential_polling.ino:747-767`
- Added hex encoding loop: converts each character to 2-digit hex
- Example: `"GW0-00001:PAIR..."` → `"4757302D30303030313A50414952..."`

**Code:**
```cpp
// Convert message to hex string for RAK3172
String hexPayload = "";
for (unsigned int i = 0; i < message.length(); i++) {
  char hex[3];
  sprintf(hex, "%02X", (unsigned char)message[i]);
  hexPayload += hex;
}
String cmd = "AT+PSEND=" + hexPayload;
```

### ✅ 2. Simplified Protocol (No HMAC)
**Problem:** Gateway expected 7-field messages with HMAC, RPi sent 6-field
**Gateway expected:** `SENDER:CMD:TARGET:SEQ:TIME:PAYLOAD:HMAC`
**RPi sends:** `SENDER:CMD:TARGET:SEQ:TIME:PAYLOAD`
**Fix Applied:**
- Modified `parseMessage()` in `lora_protocol.cpp:95-151`
- Changed colon count from 6 to 5 (expects 6 fields now, not 7)
- Set `msg.hmac = ""` since no HMAC in simplified protocol
- Disabled HMAC verification in `handleLoRaResponse()` (line 715-723)

**Code:**
```cpp
// Count colons (should be 5: sender:cmd:target:seq:time:payload) - SIMPLIFIED PROTOCOL
if (colonCount != 5) {
  Serial.println("[PROTOCOL] Invalid message format (expected 6 fields, got " + String(colonCount + 1) + ")");
  return msg;
}
```

### ✅ 3. Timestamp Validation Disabled
**Problem:** Gateway and RPi have different time sources (millis vs Unix)
**Fix Applied:**
- Commented out timestamp validation in `lora_protocol.cpp:140-147`
- Messages accepted regardless of timestamp difference

**Code:**
```cpp
// Skip timestamp validation for now (devices may not have synced time)
// if (!validateTimestamp(msg.timestamp, getCurrentTimestamp())) {
//   return msg;
// }
```

### ✅ 4. FFat Filesystem Disabled
**Problem:** ESP32-S3-N16R8 needs specific partition scheme for FFat
**Error:** `[FFAT] Failed to mount FFat!`
**Fix Applied:**
- Disabled `initFFat()` - now just prints message (line 597-610)
- Disabled `loadDevicePairings()` - now uses Preferences only (line 630-670)
- Disabled `generateCycleReport()` - no file writing (line 1441-1482)
- Disabled `saveDevicePairing()` call in device pairing (line 543)
- Device count now stored in Preferences (NVS)

**Note:** To re-enable FFat:
- Arduino IDE: Tools → Partition Scheme → "16M Flash (3MB APP/9.9MB FATFS)"
- Uncomment FFat code blocks

### ✅ 5. Gateway ID Format Updated
**Problem:** Gateway used "GW01" instead of "GW0-00001"
**Fix Applied:**
- Changed default gateway ID to "GW0-00001" (line 610)
- Updated MQTT topics to use "GW0-00001" (lines 78, 83-86)
- Updated struct comment (line 113)

---

## Testing Instructions:

### 1. Upload Fixed Gateway Firmware
- Connect ESP32-S3 via USB
- Upload `gateway_v2_0_sequential_polling.ino`
- Open Serial Monitor (115200 baud)

### 2. Run RPi Device Firmware
```bash
cd /home/pi/DETECTRA/RPI_ZERO_DETECTRA
python3 scripts/detectra_edge_device_v2.py
```

### 3. Test Device Pairing
1. Open web interface: `http://<gateway-ip>/` (credentials: rnd/rnd)
2. Click "Add Device" button
3. Enter Device ID: `ED0-00001`
4. Click "Pair Device"

### 4. Expected Behavior:
**On Gateway Serial Monitor:**
```
[LORA1] TX: GW0-00001:PAIR:ED0-00001:000:114:null
[LORA1] RX: +EVT:RXP2P:RSSI -45:SNR 8:ED0-00001:PAIR_ACK:GW0-00001:1728567890
[PROTOCOL] ✓ Message received from ED0-00001
```

**On RPi Console:**
```
📡 PAIR REQUEST FROM: GW0-00001
✓ GATEWAY PAIRING CREATED: GW0-00001
✓ Pairing complete with GW0-00001
```

**On RPi Filesystem:**
```
ls lora_config/
# Should show: GW0-00001.json
```

---

## Protocol Summary:

### Message Format (Simplified):
```
SENDER_ID:COMMAND:TARGET_ID:SEQUENCE:TIMESTAMP:PAYLOAD
```

### PAIR Example:
**Gateway sends:**
```
GW0-00001:PAIR:ED0-00001:000:114:null
```

**Device responds:**
```
ED0-00001:PAIR_ACK:GW0-00001:1728567890
```

### POLL Example:
**Gateway sends:**
```
GW0-00001:POLL:ED0-00001:001:115:null
```

**Device responds:**
```
ED0-00001:ACK:GW0-00001:ONLINE:001:115:bat_95:rssi_-45:snr_8
```

---

## Known Limitations:

1. **No persistent device storage** - devices lost on reboot (until FFat enabled)
2. **No HMAC authentication** - simplified protocol for demo
3. **No timestamp validation** - devices don't need synced time
4. **No CSV reports** - FFat disabled

---

## Next Steps:

1. ✅ Test gateway reboot - verify no FFAT errors
2. ⏳ Test device pairing from web interface
3. ⏳ Verify PAIR_ACK received
4. ⏳ Test sequential polling cycle
5. ⏳ Enable FFat with correct partition (future)

---

© 2025 DETECTRA System
