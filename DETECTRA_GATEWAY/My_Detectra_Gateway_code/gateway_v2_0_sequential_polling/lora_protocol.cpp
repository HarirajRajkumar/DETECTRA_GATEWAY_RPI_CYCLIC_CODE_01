/**
 * DETECTRA Gateway v2.0 - LoRa Protocol Implementation
 */

#include "lora_protocol.h"

// ==================== TIMESTAMP MANAGEMENT ====================

static unsigned long timestampOffset = 0;

void initTimestamp() {
  // Use millis() as timestamp base (seconds)
  timestampOffset = millis() / 1000;
}

unsigned long getCurrentTimestamp() {
  return timestampOffset + (millis() / 1000);
}

bool validateTimestamp(unsigned long messageTimestamp, unsigned long currentTimestamp) {
  long diff = (long)currentTimestamp - (long)messageTimestamp;
  return (abs(diff) <= TIMESTAMP_TOLERANCE);
}

// ==================== HMAC CALCULATION ====================

String calculateHMAC(const String& message, const String& secret) {
  byte hmacResult[32];

  // Calculate HMAC-SHA256
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret.c_str(), secret.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)message.c_str(), message.length());
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);

  // Convert first 8 bytes to hex string (16 characters)
  String hmac = "";
  for (int i = 0; i < 8; i++) {
    char hex[3];
    sprintf(hex, "%02x", hmacResult[i]);
    hmac += hex;
  }

  return hmac;
}

bool verifyHMAC(const String& message, const String& secret) {
  // Split message and HMAC
  int lastColon = message.lastIndexOf(':');
  if (lastColon == -1) return false;

  String messageWithoutHMAC = message.substring(0, lastColon);
  String receivedHMAC = message.substring(lastColon + 1);

  // Calculate expected HMAC
  String expectedHMAC = calculateHMAC(messageWithoutHMAC, secret);

  // Compare (case-insensitive)
  receivedHMAC.toLowerCase();
  expectedHMAC.toLowerCase();

  return (receivedHMAC == expectedHMAC);
}

// ==================== MESSAGE BUILDING ====================

String buildMessage(
  const String& senderId,
  const String& command,
  const String& targetId,
  const String& sequence,
  const String& payload,
  const String& secret
) {
  unsigned long timestamp = getCurrentTimestamp();

  // Build message without HMAC
  String message = senderId + ":" + command + ":" + targetId + ":" +
                   sequence + ":" + String(timestamp) + ":" + payload;

  // Calculate HMAC
  String hmac = calculateHMAC(message, secret);

  // Append HMAC
  return message + ":" + hmac;
}

// ==================== MESSAGE PARSING ====================

LoRaMessage parseMessage(const String& rawMessage) {
  LoRaMessage msg;
  msg.valid = false;

  // Count colons (should be at least 5: sender:cmd:target:seq:time:payload) - SIMPLIFIED PROTOCOL
  int colonCount = 0;
  for (unsigned int i = 0; i < rawMessage.length(); i++) {
    if (rawMessage.charAt(i) == ':') colonCount++;
  }

  if (colonCount < 5) {
    Serial.println("[PROTOCOL] Invalid message format (expected at least 6 fields, got " + String(colonCount + 1) + ")");
    return msg;
  }

  // Parse fields
  int idx = 0;
  int lastIdx = 0;
  String fields[6];

  for (int i = 0; i < 6; i++) {
    idx = rawMessage.indexOf(':', lastIdx);
    if (idx == -1 && i < 5) {
      Serial.println("[PROTOCOL] Parsing error at field " + String(i));
      return msg;
    }

    if (i == 5) {
      // Last field (payload)
      fields[i] = rawMessage.substring(lastIdx);
    } else {
      fields[i] = rawMessage.substring(lastIdx, idx);
      lastIdx = idx + 1;
    }
  }

  // Populate structure
  msg.senderId = fields[0];
  msg.command = fields[1];
  msg.targetId = fields[2];
  msg.sequence = fields[3];
  msg.timestamp = fields[4].toInt();
  msg.payload = fields[5];
  msg.hmac = "";  // No HMAC in simplified protocol

  // Skip timestamp validation for now (devices may not have synced time)
  // if (!validateTimestamp(msg.timestamp, getCurrentTimestamp())) {
  //   Serial.println("[PROTOCOL] Timestamp validation failed");
  //   Serial.println("  Message time: " + String(msg.timestamp));
  //   Serial.println("  Current time: " + String(getCurrentTimestamp()));
  //   Serial.println("  Difference: " + String((long)getCurrentTimestamp() - (long)msg.timestamp) + "s");
  //   return msg;
  // }

  msg.valid = true;
  return msg;
}

void parseDataPayload(LoRaMessage& msg) {
  // Expected format: "BLR-13-IL-01:left:motherboard:40%,led_on:50%"
  // Or with position index: "1/5" embedded

  if (msg.payload == "null" || msg.payload.isEmpty()) {
    return;
  }

  String payload = msg.payload;
  int colonCount = 0;
  int positions[4] = {-1, -1, -1, -1};

  // Find colon positions
  for (unsigned int i = 0; i < payload.length(); i++) {
    if (payload.charAt(i) == ':') {
      if (colonCount < 4) {
        positions[colonCount] = i;
      }
      colonCount++;
    }
  }

  if (colonCount >= 2) {
    // Extract table ID
    msg.data.tableId = payload.substring(0, positions[0]);

    // Extract position
    msg.data.position = payload.substring(positions[0] + 1, positions[1]);

    // Extract detections (rest of the string)
    if (colonCount >= 3) {
      msg.data.detections = payload.substring(positions[1] + 1, positions[2]);

      // Check if there's position index info (e.g., "COMPLETE" or position count)
      if (colonCount >= 3) {
        String extra = payload.substring(positions[2] + 1);
        if (extra.indexOf('/') != -1) {
          // Format: "1/5"
          int slashIdx = extra.indexOf('/');
          msg.data.positionIndex = extra.substring(0, slashIdx).toInt();
          msg.data.totalPositions = extra.substring(slashIdx + 1).toInt();
        }
      }
    } else {
      msg.data.detections = payload.substring(positions[1] + 1);
    }
  }
}

void parseHealthPayload(LoRaMessage& msg) {
  // Expected format: "bat_95:rssi_-45:snr_8"
  // Or just: "bat_95" or combination

  if (msg.payload == "null" || msg.payload.isEmpty()) {
    msg.health.battery = -1;
    msg.health.rssi = -999;
    msg.health.snr = -999;
    return;
  }

  msg.health.battery = -1;
  msg.health.rssi = -999;
  msg.health.snr = -999;

  String payload = msg.payload;
  int startIdx = 0;

  while (startIdx < (int)payload.length()) {
    int colonIdx = payload.indexOf(':', startIdx);
    String field;

    if (colonIdx == -1) {
      field = payload.substring(startIdx);
      startIdx = payload.length();
    } else {
      field = payload.substring(startIdx, colonIdx);
      startIdx = colonIdx + 1;
    }

    // Parse field
    if (field.startsWith("bat_")) {
      msg.health.battery = field.substring(4).toInt();
    } else if (field.startsWith("rssi_")) {
      msg.health.rssi = field.substring(5).toInt();
    } else if (field.startsWith("snr_")) {
      msg.health.snr = field.substring(4).toInt();
    }
  }
}

// ==================== UTILITY FUNCTIONS ====================

String phaseToString(PollingPhase phase) {
  switch (phase) {
    case PHASE_IDLE:              return "IDLE";
    case PHASE_HEALTH_CHECK:      return "HEALTH_CHECK";
    case PHASE_START_INFERENCE:   return "START_INFERENCE";
    case PHASE_DATA_COLLECTION:   return "DATA_COLLECTION";
    case PHASE_FINALIZE:          return "FINALIZE";
    case PHASE_COMPLETE:          return "COMPLETE";
    case PHASE_ERROR:             return "ERROR";
    default:                      return "UNKNOWN";
  }
}

String generateSequence(int& counter) {
  counter++;
  if (counter > 999) counter = 1;

  char seq[4];
  sprintf(seq, "%03d", counter);
  return String(seq);
}
