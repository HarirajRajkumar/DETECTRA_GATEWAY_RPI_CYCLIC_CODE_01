# Critical Fixes for Gateway v2.0

## Issues Found:

### 1. FFat Filesystem Not Mounting ❌
**Problem:** ESP32-S3-N16R8 needs proper partition scheme
**Error:** `[FFAT] Failed to mount FFat!`
**Solution:**
- In Arduino IDE: Tools → Partition Scheme → "16M Flash (3MB APP/9.9MB FATFS)"
- OR use SPIFFS instead of FFat
- FOR NOW: Comment out FFat usage, store in Preferences only

### 2. LoRa Message Format Wrong ❌
**Problem:** `AT+PSEND=` expects hex-encoded payload, not plain text
**Error:** `AT_PARAM_ERROR`
**Current:** `AT+PSEND=GW0-00001:PAIR:ED0-00001:000:114:null`
**Correct:** `AT+PSEND=4757302D30303030313A504149523A4544302D30303030313A...` (hex)

### 3. Gateway Using HMAC Protocol ❌
**Problem:** Gateway expects 7-field messages with HMAC, RPi sends 6-field simplified
**Gateway expects:** `SENDER:CMD:TARGET:SEQ:TIME:PAYLOAD:HMAC`
**RPi sends:** `SENDER:CMD:TARGET:SEQ:TIME:PAYLOAD`
**Solution:** Disable HMAC verification in gateway

### 4. Timestamp Issues ❌
**Problem:** Gateway uses `getCurrentTimestamp()` which returns millis-based time
**Issue:** RPi uses actual Unix timestamp
**Solution:** Use Unix epoch or disable timestamp validation

## Quick Fix Steps:

1. **Disable FFat** (use Preferences for now)
2. **Fix LoRa AT+PSEND** to use hex encoding
3. **Disable HMAC** verification
4. **Simplify protocol** to 6 fields

## Files to Modify:

- `gateway_v2_0_sequential_polling.ino` - Comment out FFat calls
- `gateway_v2_0_sequential_polling.ino` - Fix `sendLoRaMessage()` to hex-encode
- `gateway_v2_0_sequential_polling.ino` - Skip HMAC in `handleLoRaResponse()`
- `lora_protocol.cpp` - Update `parseMessage()` to accept 6 fields

---

## Apply Fixes Now!
