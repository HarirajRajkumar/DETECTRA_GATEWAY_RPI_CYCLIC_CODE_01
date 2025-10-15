# ⚡ RE-UPLOAD GATEWAY FIRMWARE NOW!

## Critical Fixes Applied:

### ✅ 1. Preferences Cache Cleared
**Line 228-233:** Added code to clear old "GW01" config
```cpp
preferences.begin("detectra", false);
preferences.clear();
preferences.end();
```

### ✅ 2. Simplified Protocol (No HMAC)
**All polling functions updated** to build 6-field messages:
```cpp
// OLD (with HMAC):
String message = buildMessage(..., device.sharedSecret);

// NEW (simplified):
String message = config.gatewayId + ":" + CMD_POLL + ":" + device.deviceId + ":" +
                 seq + ":" + timestamp + ":null";
```

### ✅ 3. Hex-Encoded LoRa Transmission
**Line 745-765:** Messages converted to hex for RAK3172

---

## Upload Steps:

### 1. **Compile & Upload Gateway**
- Open `gateway_v2_0_sequential_polling.ino` in Arduino IDE
- Select Board: ESP32-S3-Dev Module
- Select Port: (your COM port)
- Click Upload ▶

### 2. **Monitor Serial Output**
Open Serial Monitor (115200 baud) and look for:
```
[CONFIG] Clearing old preferences...
[CONFIG] Preferences cleared!
[CONFIG] Configuration loaded
  Gateway ID: GW0-00001       ← Should show GW0-00001 now!
  Devices: 0                  ← Should start with 0 devices
```

### 3. **Restart RPi Script**
```bash
python3 scripts/detectra_edge_device_v2.py
```

### 4. **Test Pairing**
1. Open web interface: `http://172.16.131.36/` (rnd/rnd)
2. Click "Add Device"
3. Enter: `ED0-00001`
4. Click "Pair Device"

### 5. **Expected Results**

**Gateway Serial Monitor:**
```
[LORA1] TX: GW0-00001:PAIR:ED0-00001:000:1234:null
[LORA1] RX: OK                     ← Should be OK, not AT_PARAM_ERROR
[LORA1] RX: +EVT:RXP2P:RSSI -45:SNR 8:ED0-00001:PAIR_ACK:GW0-00001:1234567890
[PROTOCOL] ✓ Message received from ED0-00001
```

**RPi Console:**
```
📡 PAIR REQUEST FROM: GW0-00001
✓ GATEWAY PAIRING CREATED: GW0-00001
✓ Pairing complete with GW0-00001
```

**RPi Filesystem:**
```bash
ls lora_config/
# Output: GW0-00001.json
```

---

## After First Successful Boot:

**IMPORTANT:** After confirming GW0-00001 loads correctly, you can remove the preferences.clear() code:

**Comment out lines 228-233:**
```cpp
// FORCE CLEAR OLD CONFIG (remove after first boot with new firmware)
// Serial.println("[CONFIG] Clearing old preferences...");
// preferences.begin("detectra", false);
// preferences.clear();
// preferences.end();
// Serial.println("[CONFIG] Preferences cleared!");
```

This prevents clearing preferences on every reboot.

---

## Troubleshooting:

### Still shows "GW01"?
- Power cycle the ESP32 completely (unplug USB)
- Re-upload firmware
- Check Serial Monitor immediately after boot

### AT_PARAM_ERROR still appears?
- Check LoRa module connections
- Verify RAK3172 is in P2P mode
- Try manual AT command: `AT+PSEND=48454C4C4F` (hex for "HELLO")

### Device not responding?
- Verify RPi script is running
- Check LoRa frequency matches (868 MHz)
- Check spreading factor matches (SF9)

---

## Status Checklist:

- [ ] Gateway uploaded with new firmware
- [ ] Serial shows "GW0-00001" (not "GW01")
- [ ] Serial shows "Devices: 0" initially
- [ ] LoRa TX shows hex-encoded payload
- [ ] No "AT_PARAM_ERROR" on transmission
- [ ] RPi running and waiting for commands
- [ ] Web interface accessible
- [ ] Device pairing test completed
- [ ] `lora_config/GW0-00001.json` created on RPi

---

🚀 **Upload now and test!**
