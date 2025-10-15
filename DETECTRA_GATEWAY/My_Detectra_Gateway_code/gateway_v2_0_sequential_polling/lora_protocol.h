/**
 * DETECTRA Gateway v2.0 - LoRa Protocol Library
 *
 * Communication Protocol for Gateway ↔ RPi Zero Edge Devices
 *
 * Message Format: SENDER_ID:COMMAND:TARGET_ID:SEQUENCE:TIMESTAMP:PAYLOAD:HMAC
 * Example: GW01:POLL:D1:001:1728567890:null:a3f2b1c4d5e6f7a8
 *
 * Security: HMAC-SHA256 (first 8 bytes = 16 hex chars)
 */

#ifndef LORA_PROTOCOL_H
#define LORA_PROTOCOL_H

#include <Arduino.h>
#include <mbedtls/md.h>

// ==================== PROTOCOL CONSTANTS ====================

// Commands - Gateway → Device
#define CMD_POLL         "POLL"           // Health check
#define CMD_START_INFER  "START_INFER"    // Begin inference
#define CMD_ACK          "ACK"            // Acknowledge
#define CMD_FINALIZE     "FINALIZE"       // Complete cycle
#define CMD_SLEEP        "SLEEP"          // Enter listening mode

// Commands - Device → Gateway
#define CMD_DATA         "DATA"           // Inference data

// Response Status
#define STATUS_ONLINE      "ONLINE"       // Device responding
#define STATUS_INFERRING   "INFERRING"    // Device processing
#define STATUS_FINALIZED   "FINALIZED"    // Cycle completed
#define STATUS_SLEEPING    "SLEEPING"     // Entering RX mode

// Timeouts (milliseconds)
#define TIMEOUT_HEALTH_CHECK  15000       // 15 seconds
#define TIMEOUT_START_INFER   5000        // 5 seconds
#define TIMEOUT_DATA_COLLECT  120000      // 120 seconds (2 minutes)
#define TIMEOUT_FINALIZE      10000       // 10 seconds

// Retry Configuration
#define MAX_RETRIES           3           // Maximum retry attempts
#define RETRY_DELAY_BASE      2000        // Base delay: 2 seconds
                                          // Exponential backoff: 2s, 4s, 8s

// Message Validation
#define TIMESTAMP_TOLERANCE   60          // ±60 seconds allowed
#define HMAC_LENGTH           16          // 16 hex characters (8 bytes)

// ==================== DATA STRUCTURES ====================

/**
 * Parsed LoRa Message Structure
 */
struct LoRaMessage {
  String senderId;      // e.g., "GW01", "D1"
  String command;       // e.g., "POLL", "ACK", "DATA"
  String targetId;      // e.g., "D1", "GW01"
  String sequence;      // e.g., "001"
  unsigned long timestamp;
  String payload;       // Command-specific data
  String hmac;          // 16-character hex string

  // Parsed payload (for DATA messages)
  struct {
    String tableId;         // e.g., "BLR-13-IL-01"
    String position;        // "left", "center", "right", etc.
    String detections;      // "motherboard:40%,led_on:50%"
    int positionIndex;      // 1-5
    int totalPositions;     // 5
  } data;

  // Parsed payload (for ONLINE messages)
  struct {
    int battery;            // Battery percentage
    int rssi;               // Signal strength
    int snr;                // Signal-to-noise ratio
  } health;

  bool valid;               // Message validation status
};

/**
 * Device Polling State
 */
enum PollingPhase {
  PHASE_IDLE,
  PHASE_HEALTH_CHECK,
  PHASE_START_INFERENCE,
  PHASE_DATA_COLLECTION,
  PHASE_FINALIZE,
  PHASE_COMPLETE,
  PHASE_ERROR
};

/**
 * Device Information
 */
struct DeviceInfo {
  String deviceId;          // e.g., "D1", "D2"
  String sharedSecret;      // 32-character hex string
  bool paired;              // Device paired status
  String tableLeft;         // Table left ID (e.g., "BLR-13-IL-02")
  String tableRight;        // Table right ID (e.g., "BLR-13-IL-01")

  // Current state
  PollingPhase phase;
  int retryCount;
  unsigned long lastContact;
  bool commandSent;         // Flag to prevent re-sending commands

  // Health data
  int battery;
  int rssi;
  int snr;
  bool online;

  // Data collection progress
  int positionsReceived;    // 0-5
  String lastPosition;
  String lastTableId;
  String lastDetections;

  // Statistics
  unsigned long totalPolls;
  unsigned long successfulPolls;
  unsigned long failedPolls;
};

// ==================== PROTOCOL FUNCTIONS ====================

/**
 * Calculate HMAC-SHA256 for message authentication
 *
 * @param message The message to authenticate (without HMAC)
 * @param secret Shared secret key (32-character hex)
 * @return First 8 bytes of HMAC as 16-character hex string
 */
String calculateHMAC(const String& message, const String& secret);

/**
 * Verify HMAC-SHA256 of received message
 *
 * @param message Complete message with HMAC
 * @param secret Shared secret key
 * @return true if HMAC valid, false otherwise
 */
bool verifyHMAC(const String& message, const String& secret);

/**
 * Build secure LoRa message with HMAC
 *
 * @param senderId Gateway/Device ID
 * @param command Command string
 * @param targetId Target Device/Gateway ID
 * @param sequence Sequence number (e.g., "001")
 * @param payload Payload data (use "null" if empty)
 * @param secret Shared secret for HMAC
 * @return Complete message with HMAC
 */
String buildMessage(
  const String& senderId,
  const String& command,
  const String& targetId,
  const String& sequence,
  const String& payload,
  const String& secret
);

/**
 * Parse incoming LoRa message
 *
 * @param rawMessage Raw message string
 * @return Parsed LoRaMessage structure
 */
LoRaMessage parseMessage(const String& rawMessage);

/**
 * Parse DATA payload into components
 *
 * Example: "BLR-13-IL-01:left:motherboard:40%,led_on:50%"
 *
 * @param msg LoRaMessage with payload
 */
void parseDataPayload(LoRaMessage& msg);

/**
 * Parse ONLINE/ACK payload with health data
 *
 * Example: "bat_95:rssi_-45:snr_8"
 *
 * @param msg LoRaMessage with payload
 */
void parseHealthPayload(LoRaMessage& msg);

/**
 * Validate message timestamp (must be within ±60 seconds)
 *
 * @param messageTimestamp Timestamp from message
 * @param currentTimestamp Current system timestamp
 * @return true if within tolerance, false otherwise
 */
bool validateTimestamp(unsigned long messageTimestamp, unsigned long currentTimestamp);

/**
 * Get current Unix timestamp (seconds since epoch)
 * Uses millis() as reference (not real time)
 *
 * @return Current timestamp
 */
unsigned long getCurrentTimestamp();

/**
 * Initialize timestamp reference
 * Call this in setup() after NTP sync or use millis-based time
 */
void initTimestamp();

/**
 * Convert PollingPhase enum to string for logging
 *
 * @param phase PollingPhase enum value
 * @return Human-readable string
 */
String phaseToString(PollingPhase phase);

/**
 * Generate sequence number (3-digit format)
 *
 * @param counter Sequence counter (rolls over at 999)
 * @return Formatted sequence string (e.g., "001", "042", "999")
 */
String generateSequence(int& counter);

#endif // LORA_PROTOCOL_H
