/**
 * ==========================================
 * DETECTRA GATEWAY v2.0 - Sequential Polling
 * ==========================================
 *
 * Architecture: Gateway (Master) → LoRa → RPi Zero Devices (Slaves)
 *
 * Features:
 * - Sequential polling of up to 15 devices
 * - Dual RAK3172 LoRa modules (UART)
 * - 4-phase communication protocol
 * - HMAC-SHA256 message authentication
 * - MQTT publishing to PC application
 * - Web interface for device management
 * - FFat storage for device pairing and reports
 * - Real-time polling progress via WebSocket
 *
 * Hardware:
 * - ESP32-S3-WROOM-1-N16R8
 * - 2× RAK3172 LoRa modules
 * - SSD1306 OLED display
 * - WS2812B NeoPixel LED
 * - Active buzzer
 *
 * © 2025 DETECTRA System
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <FFat.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include "lora_protocol.h"
#include "web_interface.h"

// ==================== HARDWARE CONFIGURATION ====================

// LoRa Modules (Dual RAK3172)
#define LORA1_TX      43          // LoRa Module 1 (Devices 1-15)
#define LORA1_RX      44
#define LORA2_TX      17          // LoRa Module 2 (Devices 16-30) - Future
#define LORA2_RX      18

// I2C OLED Display
#define OLED_SDA      38
#define OLED_SCL      37
#define OLED_WIDTH    128
#define OLED_HEIGHT   32
#define OLED_ADDR     0x3C

// Status Indicators
#define NEOPIXEL_PIN  8
#define BUZZER_PIN    5

// LoRa Configuration
#define LORA_FREQ     "868000000"    // 868 MHz (EU ISM band)
#define LORA_SF       "9"             // Spreading Factor 9
#define LORA_BW       "125"           // Bandwidth 125 kHz
#define LORA_CR       "1"             // Coding Rate 4/6 (matches RPi)
#define LORA_PREAMBLE "8"             // Preamble length
#define LORA_PWR      "22"            // TX Power 22 dBm

// ==================== NETWORK CONFIGURATION ====================

// WiFi Credentials
const char* ssid = "13FL_03_D3_R&D_2.4GHz";
const char* password = "r&d@1234";

// MQTT Broker
const char* mqtt_server = "172.16.236.158"; //PC's IP address
const int mqtt_port = 1883;
const char* mqtt_client_id = "DETECTRA_GW0-00001";
const char* mqtt_username = "";               // Optional
const char* mqtt_password = "";               // Optional

// MQTT Topics
const char* topic_status = "detectra/GW0-00001/status";
const char* topic_data = "detectra/GW0-00001/data";
const char* topic_device = "detectra/GW0-00001/device/";
const char* topic_polling = "detectra/GW0-00001/polling";

// Web Server Credentials
const char* web_username = "rnd";
const char* web_password = "rnd";

// ==================== GLOBAL OBJECTS ====================

// Hardware
HardwareSerial LoRa1(1);  // UART1
HardwareSerial LoRa2(2);  // UART2
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
Adafruit_NeoPixel led(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Network
WiFiClient espClient;
PubSubClient mqttClient(espClient);
AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

// Storage
Preferences preferences;

// ==================== GLOBAL VARIABLES ====================

// Gateway Configuration
struct GatewayConfig {
  String gatewayId;           // e.g., "GW0-00001"
  String building;
  String floor;
  String lab;
  int pollingIntervalMinutes;
  int numDevices;             // Number of paired devices
} config;

// Device Array (supports 15 devices)
DeviceInfo devices[15];

// Polling State
bool pollingActive = false;
int currentDeviceIndex = 0;
unsigned long pollingStartTime = 0;
unsigned long phaseStartTime = 0;
int sequenceCounter = 0;

// Network Status
bool wifiConnected = false;
bool mqttConnected = false;

// Statistics
unsigned long totalMessages = 0;
unsigned long successfulPolls = 0;
unsigned long failedPolls = 0;

// Watchdog
unsigned long lastLoRaActivity = 0;
unsigned long lastMQTTActivity = 0;

// ==================== FUNCTION PROTOTYPES ====================

// Setup & Initialization
void initHardware();
void initLoRa();
void initWiFi();
void initMQTT();
void initWebServer();
void initFFat();
void loadConfiguration();
void loadDevicePairings();

// LoRa Communication
void loraTask(void* parameter);
void handleLoRaResponse(String response, int loraModule);
bool sendLoRaCommand(const String& command, int loraModule);
bool sendLoRaMessage(const String& message, int loraModule);

// Polling State Machine
void pollingTask(void* parameter);
void startPollingCycle();
void pollNextDevice();
void processPhase(DeviceInfo& device);
void advancePhase(DeviceInfo& device);
void handlePhaseTimeout(DeviceInfo& device);
void handleDeviceOffline(DeviceInfo& device);
void completeDevicePolling(DeviceInfo& device);

// Message Handling
void processIncomingMessage(const LoRaMessage& msg);
void handleAckOnline(const LoRaMessage& msg);
void handleAckInferring(const LoRaMessage& msg);
void handleDataMessage(const LoRaMessage& msg);
void handleAckFinalized(const LoRaMessage& msg);
void handleAckSleeping(const LoRaMessage& msg);

// MQTT Publishing
void publishGatewayStatus();
void publishPollingStatus();
void publishDeviceData(int deviceIndex);
void publishPollingComplete();
void mqttReconnect();

// Web Interface
void setupWebRoutes();
void handleWebSocketMessage(AsyncWebSocketClient* client, char* data, size_t len);
void notifyWebClients(const String& message);
String buildDeviceListJSON();
String buildPollingStatusJSON();

// Display & Indicators
void updateDisplay();
void setLEDColor(uint8_t r, uint8_t g, uint8_t b);
void beepBuzzer(int duration);

// Storage & Reports
void saveConfiguration();
void saveDevicePairing(const DeviceInfo& device);
void generateCycleReport();

// Utilities
String getDeviceSecret(const String& deviceId);
int getDeviceIndexById(const String& deviceId);
String getLoRaModuleForDevice(int deviceIndex);

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("==========================================");
  Serial.println(" DETECTRA GATEWAY v2.0");
  Serial.println(" Sequential Polling Architecture");
  Serial.println("==========================================");
  Serial.println();

  // Initialize hardware
  initHardware();

  // Initialize timestamp
  initTimestamp();

  // FORCE CLEAR OLD CONFIG (remove after first boot with new firmware)
  Serial.println("[CONFIG] Clearing old preferences...");
  preferences.begin("detectra", false);
  preferences.clear();
  preferences.end();
  Serial.println("[CONFIG] Preferences cleared!");

  // Load configuration
  initFFat();
  loadConfiguration();
  loadDevicePairings();

  // Initialize network
  initWiFi();
  initMQTT();
  initWebServer();

  // Initialize LoRa modules
  initLoRa();

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(
    loraTask,
    "LoRaTask",
    10000,
    NULL,
    3,          // High priority
    NULL,
    0           // Core 0
  );

  xTaskCreatePinnedToCore(
    pollingTask,
    "PollingTask",
    10000,
    NULL,
    2,          // Medium priority
    NULL,
    1           // Core 1
  );

  Serial.println("==========================================");
  Serial.println(" Gateway Initialized Successfully");
  Serial.println(" Gateway ID: " + config.gatewayId);
  Serial.println(" Devices Paired: " + String(config.numDevices));
  Serial.println(" Polling Interval: " + String(config.pollingIntervalMinutes) + " minutes");
  Serial.println("==========================================");
  Serial.println();
  Serial.println(">>> Ready! Use web interface to pair devices and start polling.");
  Serial.println();

  // DO NOT auto-start polling - wait for manual trigger from web interface
  // User will pair devices first, then manually click "Start Polling"
}

void loop() {
  // Handle MQTT connection
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();

  // Clean up WebSocket clients
  ws.cleanupClients();

  // Update display periodically
  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 1000) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }

  // Publish status periodically (every 30 seconds)
  static unsigned long lastStatusPublish = 0;
  if (millis() - lastStatusPublish > 30000) {
    publishGatewayStatus();
    lastStatusPublish = millis();
  }

  // Automatic polling trigger (3 minutes for development, 60 minutes for production)
  // Note: This is a backup mechanism. Primary polling is handled in pollingTask()
  static unsigned long lastPollingStart = millis();
  unsigned long pollingIntervalMs = config.pollingIntervalMinutes * 60UL * 1000UL;

  if (!pollingActive && (millis() - lastPollingStart > pollingIntervalMs)) {
    Serial.println("[LOOP] Auto-triggering polling cycle (timer expired)");
    startPollingCycle();
    lastPollingStart = millis();
  }

  delay(10);
}

// ==================== HARDWARE INITIALIZATION ====================

void initHardware() {
  Serial.println("[INIT] Initializing hardware...");

  // NeoPixel LED
  led.begin();
  setLEDColor(0, 0, 255);  // Blue during init
  led.show();

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  beepBuzzer(100);  // Short beep

  // I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  // OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[INIT] OLED initialization failed!");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("DETECTRA GW v2.0");
    display.println("Initializing...");
    display.display();
    Serial.println("[INIT] OLED initialized");
  }

  Serial.println("[INIT] Hardware ready");
}

void initLoRa() {
  Serial.println("[LORA] Initializing LoRa modules...");

  // LoRa Module 1 (Devices 1-15)
  LoRa1.begin(115200, SERIAL_8N1, LORA1_RX, LORA1_TX);
  LoRa1.setRxBufferSize(1024);  // Increase RX buffer to prevent overflow
  delay(500);

  // Basic AT test
  Serial.println("[LORA1] Testing communication...");
  sendLoRaCommand("AT", 1);

  // Disable RX mode first (in case it's already running from previous session)
  Serial.println("[LORA1] Disabling existing RX mode...");
  sendLoRaCommand("AT+PRECV=0", 1);

  // Set P2P mode
  Serial.println("[LORA1] Setting P2P mode...");
  sendLoRaCommand("AT+NWM=0", 1);

  // Configure P2P with single command (matches RPi initialization)
  Serial.println("[LORA1] Configuring P2P parameters...");
  String p2pConfig = "AT+P2P=" + String(LORA_FREQ) + ":" + String(LORA_SF) + ":" +
                     String(LORA_BW) + ":" + String(LORA_CR) + ":" +
                     String(LORA_PREAMBLE) + ":" + String(LORA_PWR);

  Serial.println("[LORA1] " + p2pConfig);
  sendLoRaCommand(p2pConfig, 1);

  // Verify configuration
  Serial.println("[LORA1] Verifying configuration...");
  sendLoRaCommand("AT+P2P?", 1);

  // Start receiving
  Serial.println("[LORA1] Starting RX mode...");
  sendLoRaCommand("AT+PRECV=65533", 1);  // Continuous RX + TX allowed

  Serial.println("[LORA1] LoRa Module 1 configured");
  Serial.println("  Freq: " + String(LORA_FREQ) + " Hz (" + String(atol(LORA_FREQ)/1000000) + " MHz)");
  Serial.println("  SF: " + String(LORA_SF));
  Serial.println("  BW: " + String(LORA_BW) + " kHz");
  Serial.println("  CR: 4/" + String(atoi(LORA_CR) + 5));
  Serial.println("  Preamble: " + String(LORA_PREAMBLE));
  Serial.println("  Power: " + String(LORA_PWR) + " dBm");

  // TODO: Initialize LoRa Module 2 for devices 16-30
  // Currently supporting devices 1-15 only

  setLEDColor(0, 255, 0);  // Green when LoRa ready
  led.show();
}

// ==================== NETWORK INITIALIZATION ====================

void initWiFi() {
  Serial.print("[WIFI] Connecting to " + String(ssid));

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n[WIFI] Connected! IP: " + WiFi.localIP().toString());
    setLEDColor(0, 255, 255);  // Cyan
    led.show();
  } else {
    Serial.println("\n[WIFI] Connection failed!");
    setLEDColor(255, 255, 0);  // Yellow
    led.show();
  }
}

void initMQTT() {
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(4096);  // Large buffer for complex messages
  Serial.println("[MQTT] Configured for " + String(mqtt_server) + ":" + String(mqtt_port));
}

void mqttReconnect() {
  if (!wifiConnected) return;

  static unsigned long lastAttempt = 0;
  if (millis() - lastAttempt < 5000) return;  // Try every 5 seconds
  lastAttempt = millis();

  Serial.print("[MQTT] Connecting...");

  if (mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
    mqttConnected = true;
    Serial.println(" Connected!");
    publishGatewayStatus();
    setLEDColor(0, 255, 255);  // Cyan
    led.show();
  } else {
    mqttConnected = false;
    Serial.println(" Failed (rc=" + String(mqttClient.state()) + ")");
    setLEDColor(255, 255, 0);  // Yellow
    led.show();
  }
}

// ==================== WEB SERVER INITIALIZATION ====================

void initWebServer() {
  Serial.println("[WEB] Initializing web server...");

  // WebSocket handler
  ws.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client,
                AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      Serial.println("[WS] Client connected: " + String(client->id()));
      client->text(buildPollingStatusJSON());
    } else if (type == WS_EVT_DISCONNECT) {
      Serial.println("[WS] Client disconnected: " + String(client->id()));
    } else if (type == WS_EVT_DATA) {
      handleWebSocketMessage(client, (char*)data, len);
    }
  });

  webServer.addHandler(&ws);

  setupWebRoutes();

  webServer.begin();
  Serial.println("[WEB] Server started on port 80");
  Serial.println("[WEB] Access: http://" + WiFi.localIP().toString());
  Serial.println("[WEB] Credentials: " + String(web_username) + " / " + String(web_password));
}

void setupWebRoutes() {
  // Root page - Device dashboard
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(web_username, web_password)) {
      return request->requestAuthentication();
    }
    request->send(200, "text/html", HTML_DASHBOARD);
  });

  // API: Get device list
  webServer.on("/api/devices", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(web_username, web_password)) {
      return request->requestAuthentication();
    }
    request->send(200, "application/json", buildDeviceListJSON());
  });

  // API: Get polling status
  webServer.on("/api/polling", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(web_username, web_password)) {
      return request->requestAuthentication();
    }
    request->send(200, "application/json", buildPollingStatusJSON());
  });

  // API: Start manual polling (all devices)
  webServer.on("/api/poll/start", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!request->authenticate(web_username, web_password)) {
      return request->requestAuthentication();
    }
    if (!pollingActive) {
      startPollingCycle();
      request->send(200, "application/json", "{\"status\":\"started\"}");
    } else {
      request->send(409, "application/json", "{\"error\":\"polling already active\"}");
    }
  });

  // API: Poll single device
  webServer.on("/api/poll/device", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!request->authenticate(web_username, web_password)) {
        return request->requestAuthentication();
      }

      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, data);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      String deviceId = doc["device_id"];
      int deviceIndex = getDeviceIndexById(deviceId);

      if (deviceIndex == -1) {
        request->send(404, "application/json", "{\"success\":false,\"error\":\"Device not found\"}");
        return;
      }

      if (pollingActive) {
        request->send(409, "application/json", "{\"success\":false,\"error\":\"Polling cycle already active\"}");
        return;
      }

      // Send POLL command directly to this device
      DeviceInfo& device = devices[deviceIndex];

      Serial.println("\n>>> Manual POLL to: " + device.deviceId);

      String seq = generateSequence(sequenceCounter);
      String message = config.gatewayId + ":" + String(CMD_POLL) + ":" + device.deviceId + ":" +
                       seq + ":" + String(getCurrentTimestamp()) + ":null";

      sendLoRaMessage(message, 1);

      // Reset device state for this poll
      device.phase = PHASE_HEALTH_CHECK;
      device.retryCount = 0;
      device.commandSent = true;  // Mark as sent
      phaseStartTime = millis();

      request->send(200, "application/json", "{\"success\":true,\"message\":\"POLL sent to " + deviceId + "\"}");
    });

  // API: Pair new device
  webServer.on("/api/device/pair", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!request->authenticate(web_username, web_password)) {
        return request->requestAuthentication();
      }

      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, data);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      String deviceId = doc["device_id"];
      String tableLeft = doc["table_left"] | "";
      String tableRight = doc["table_right"] | "";

      // Validate device ID format (EDy-XXXXX)
      if (!deviceId.startsWith("ED") || deviceId.length() != 9) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid device ID format\"}");
        return;
      }

      // Check if device already paired
      for (int i = 0; i < config.numDevices; i++) {
        if (devices[i].deviceId == deviceId) {
          request->send(409, "application/json", "{\"success\":false,\"error\":\"Device already paired\"}");
          return;
        }
      }

      // Check capacity
      if (config.numDevices >= 15) {
        request->send(507, "application/json", "{\"success\":false,\"error\":\"Maximum devices reached (15)\"}");
        return;
      }

      // Add device
      int idx = config.numDevices;
      devices[idx].deviceId = deviceId;
      devices[idx].sharedSecret = "temp_secret_" + deviceId; // TODO: Generate proper secret
      devices[idx].paired = false; // Will be true after PAIR_ACK
      devices[idx].tableLeft = tableLeft;
      devices[idx].tableRight = tableRight;
      devices[idx].phase = PHASE_IDLE;
      devices[idx].online = false;
      devices[idx].battery = -1;
      devices[idx].rssi = 0;
      devices[idx].snr = 0;
      devices[idx].retryCount = 0;
      devices[idx].commandSent = false;
      devices[idx].positionsReceived = 0;
      devices[idx].totalPolls = 0;
      devices[idx].successfulPolls = 0;
      devices[idx].failedPolls = 0;
      devices[idx].lastContact = 0;

      config.numDevices++;

      // Save to FFat (disabled - will use Preferences)
      // saveDevicePairing(devices[idx]);

      // Save to NVS
      saveConfiguration();

      // Send PAIR command via LoRa with table data (simplified protocol - no HMAC)
      // Format: GWx:PAIR:EDx:000:timestamp:table_left|table_right
      String pairPayload = tableLeft + "|" + tableRight;
      if (pairPayload == "|") pairPayload = "null";  // If both empty, send null
      String pairMessage = config.gatewayId + ":PAIR:" + deviceId + ":000:" + String(getCurrentTimestamp()) + ":" + pairPayload;
      sendLoRaMessage(pairMessage, 1);

      Serial.println("[API] Device pairing initiated: " + deviceId);

      request->send(200, "application/json", "{\"success\":true,\"message\":\"Pairing command sent\"}");
    });

  // API: Remove device
  webServer.on("/api/device/remove", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!request->authenticate(web_username, web_password)) {
        return request->requestAuthentication();
      }

      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, data);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      String deviceId = doc["device_id"];
      int deviceIndex = getDeviceIndexById(deviceId);

      if (deviceIndex == -1) {
        request->send(404, "application/json", "{\"success\":false,\"error\":\"Device not found\"}");
        return;
      }

      // Remove device by shifting array
      for (int i = deviceIndex; i < config.numDevices - 1; i++) {
        devices[i] = devices[i + 1];
      }
      config.numDevices--;

      // Save updated list
      saveConfiguration();

      Serial.println("[API] Device removed: " + deviceId);

      request->send(200, "application/json", "{\"success\":true,\"message\":\"Device removed\"}");
    });
}

// ==================== STORAGE INITIALIZATION ====================

void initFFat() {
  Serial.println("[FFAT] Filesystem disabled (using Preferences only)");

  // FFat disabled - partition issue with ESP32-S3-N16R8
  // To enable: Tools → Partition Scheme → "16M Flash (3MB APP/9.9MB FATFS)"

  // if (!FFat.begin(true)) {
  //   Serial.println("[FFAT] Failed to mount FFat!");
  //   return;
  // }
  // Serial.println("[FFAT] Filesystem mounted");
  // Serial.println("[FFAT] Total: " + String(FFat.totalBytes() / 1024) + " KB");
  // Serial.println("[FFAT] Used: " + String(FFat.usedBytes() / 1024) + " KB");
}

void loadConfiguration() {
  preferences.begin("detectra", true);  // Read-only

  config.gatewayId = preferences.getString("gateway_id", "GW0-00001");
  config.building = preferences.getString("building", "BLR");
  config.floor = preferences.getString("floor", "13");
  config.lab = preferences.getString("lab", "Innovation Lab");
  config.pollingIntervalMinutes = preferences.getInt("poll_interval", 5);  // 5 minutes for development
  config.numDevices = preferences.getInt("num_devices", 0);

  preferences.end();

  Serial.println("[CONFIG] Configuration loaded");
  Serial.println("  Gateway ID: " + config.gatewayId);
  Serial.println("  Location: " + config.building + "-" + config.floor + "-" + config.lab);
  Serial.println("  Devices: " + String(config.numDevices));
}

void loadDevicePairings() {
  // FFat disabled - device pairings stored in config.numDevices via Preferences
  Serial.println("[CONFIG] Device pairings loaded from Preferences (count: " + String(config.numDevices) + ")");

  // FFat-based loading disabled:
  // File file = FFat.open("/device_secrets.json", "r");
  // if (!file) {
  //   Serial.println("[CONFIG] No device pairings found");
  //   return;
  // }
  //
  // StaticJsonDocument<4096> doc;
  // DeserializationError error = deserializeJson(doc, file);
  // file.close();
  //
  // if (error) {
  //   Serial.println("[CONFIG] Failed to parse device_secrets.json");
  //   return;
  // }
  //
  // JsonArray devicesArray = doc["devices"];
  // int count = 0;
  //
  // for (JsonObject deviceObj : devicesArray) {
  //   if (count >= 15) break;
  //
  //   devices[count].deviceId = deviceObj["device_id"].as<String>();
  //   devices[count].sharedSecret = deviceObj["shared_secret"].as<String>();
  //   devices[count].paired = true;
  //   devices[count].phase = PHASE_IDLE;
  //   devices[count].retryCount = 0;
  //   devices[count].online = false;
  //   devices[count].positionsReceived = 0;
  //
  //   Serial.println("[CONFIG] Loaded device: " + devices[count].deviceId);
  //   count++;
  // }
  //
  // config.numDevices = count;
  // Serial.println("[CONFIG] Loaded " + String(count) + " device pairings");
}

// ==================== LORA TASK (Core 0) ====================

void loraTask(void* parameter) {
  Serial.println("[LORA TASK] Started on Core " + String(xPortGetCoreID()));

  String loraBuffer = "";

  while (true) {
    // Read from LoRa Module 1
    while (LoRa1.available()) {
      char c = LoRa1.read();
      if (c == '\n') {
        if (loraBuffer.length() > 0) {
          handleLoRaResponse(loraBuffer, 1);
          loraBuffer = "";
        }
      } else if (c != '\r') {
        loraBuffer += c;
      }
    }

    // TODO: Read from LoRa Module 2

    lastLoRaActivity = millis();
    vTaskDelay(5 / portTICK_PERIOD_MS);  // Reduced from 10ms to 5ms for faster serial reading
  }
}

void handleLoRaResponse(String response, int loraModule) {
  response.trim();

  Serial.println("[LORA" + String(loraModule) + "] RX: " + response);

  // Check if it's a received message (starts with "+EVT:RXP2P:")
  if (response.startsWith("+EVT:RXP2P:")) {
    // Extract message after RSSI/SNR info
    // Format: +EVT:RXP2P:-49:10:4544302D30...
    // (RSSI:-49, SNR:10, then HEX payload)

    // Find the LAST colon - hex payload always comes after it
    int lastColon = response.lastIndexOf(':');

    if (lastColon != -1) {
      String hexMessage = response.substring(lastColon + 1);
      hexMessage.trim();

      Serial.println("[LoRa HEX] " + hexMessage);  // Debug: print hex before decoding

      // Decode hex to ASCII
      String decodedMessage = "";
      for (size_t i = 0; i < hexMessage.length(); i += 2) {
        if (i + 1 < hexMessage.length()) {
          String byteString = hexMessage.substring(i, i + 2);
          char byte = (char)strtol(byteString.c_str(), NULL, 16);
          decodedMessage += byte;
        }
      }

      Serial.println("[LoRa DECODED] ");
      Serial.println(decodedMessage);

      totalMessages++;

      // Parse decoded message (simplified protocol - no HMAC verification)
      LoRaMessage msg = parseMessage(decodedMessage);

      if (msg.valid) {
        Serial.println("[PROTOCOL] ✓ Message received from " + msg.senderId);
        processIncomingMessage(msg);
      } else {
        Serial.println("[PROTOCOL] Invalid message format");
      }
    }
  }
  // Handle hex payload that comes on a separate line (no +EVT: prefix)
  // This happens when RAK3172 splits long RX messages across multiple lines
  else if (response.length() > 16 && response.indexOf("EVT") == -1 && response.indexOf("OK") == -1 && response.indexOf("AT") == -1) {
    // Check if this looks like a hex string (all characters are hex digits)
    bool isHex = true;
    for (size_t i = 0; i < response.length(); i++) {
      char c = response[i];
      if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
        isHex = false;
        break;
      }
    }

    if (isHex) {
      Serial.println("[LoRa] Detected continuation hex payload");

      // Decode hex to ASCII
      String decodedMessage = "";
      for (size_t i = 0; i < response.length(); i += 2) {
        if (i + 1 < response.length()) {
          String byteString = response.substring(i, i + 2);
          char byte = (char)strtol(byteString.c_str(), NULL, 16);
          decodedMessage += byte;
        }
      }

      Serial.println("[LoRa DECODED] " + decodedMessage);

      totalMessages++;

      // Parse decoded message (simplified protocol - no HMAC verification)
      LoRaMessage msg = parseMessage(decodedMessage);

      if (msg.valid) {
        Serial.println("[PROTOCOL] ✓ Message received from " + msg.senderId);
        processIncomingMessage(msg);
      } else {
        Serial.println("[PROTOCOL] Invalid message format");
      }
    }
  }
}

bool sendLoRaCommand(const String& command, int loraModule) {
  if (loraModule == 1) {
    LoRa1.println(command);
  } else {
    LoRa2.println(command);
  }

  delay(200);  // Wait for response
  return true;
}

bool sendLoRaMessage(const String& message, int loraModule) {
  Serial.println("[LORA" + String(loraModule) + "] TX: " + message);

  // Convert message to hex string for RAK3172
  String hexPayload = "";
  for (unsigned int i = 0; i < message.length(); i++) {
    char hex[3];
    sprintf(hex, "%02X", (unsigned char)message[i]);
    hexPayload += hex;
  }

  String cmd = "AT+PSEND=" + hexPayload;

  if (loraModule == 1) {
    LoRa1.println(cmd);
  } else {
    LoRa2.println(cmd);
  }

  return true;
}

// ==================== POLLING TASK (Core 1) ====================

void pollingTask(void* parameter) {
  Serial.println("[POLLING TASK] Started on Core " + String(xPortGetCoreID()));

  while (true) {
    if (pollingActive && currentDeviceIndex < config.numDevices) {
      DeviceInfo& device = devices[currentDeviceIndex];

      // Process current phase
      processPhase(device);

      // Check for timeout
      unsigned long elapsed = millis() - phaseStartTime;
      unsigned long timeout = 0;

      switch (device.phase) {
        case PHASE_HEALTH_CHECK:
          timeout = TIMEOUT_HEALTH_CHECK;
          break;
        case PHASE_START_INFERENCE:
          timeout = TIMEOUT_START_INFER;
          break;
        case PHASE_DATA_COLLECTION:
          timeout = TIMEOUT_DATA_COLLECT;
          break;
        case PHASE_FINALIZE:
          timeout = TIMEOUT_FINALIZE;
          break;
        default:
          timeout = 10000;
      }

      if (elapsed > timeout) {
        handlePhaseTimeout(device);
      }
    } else if (pollingActive && currentDeviceIndex >= config.numDevices) {
      // All devices polled - complete cycle
      publishPollingComplete();
      generateCycleReport();
      pollingActive = false;

      Serial.println("\n==========================================");
      Serial.println(" Polling Cycle Complete");
      Serial.println(" Duration: " + String((millis() - pollingStartTime) / 1000) + " seconds");
      Serial.println(" Next cycle in: " + String(config.pollingIntervalMinutes) + " minutes");
      Serial.println("==========================================\n");

      // Schedule next cycle
      delay(config.pollingIntervalMinutes * 60 * 1000);
      startPollingCycle();
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void startPollingCycle() {
  Serial.println("\n==========================================");
  Serial.println(" Starting Polling Cycle");
  Serial.println(" Devices: " + String(config.numDevices));
  Serial.println("==========================================\n");

  pollingActive = true;
  currentDeviceIndex = 0;
  pollingStartTime = millis();
  sequenceCounter = 0;

  // Reset all devices to IDLE
  for (int i = 0; i < config.numDevices; i++) {
    devices[i].phase = PHASE_IDLE;
    devices[i].retryCount = 0;
    devices[i].positionsReceived = 0;
  }

  publishPollingStatus();
  notifyWebClients(buildPollingStatusJSON());

  // Start first device
  pollNextDevice();
}

void pollNextDevice() {
  if (currentDeviceIndex >= config.numDevices) {
    return;
  }

  DeviceInfo& device = devices[currentDeviceIndex];

  Serial.println("\n>>> Polling Device: " + device.deviceId + " (" +
                 String(currentDeviceIndex + 1) + "/" + String(config.numDevices) + ")");

  device.phase = PHASE_HEALTH_CHECK;
  device.retryCount = 0;
  device.commandSent = false;  // Reset flag when starting new device
  phaseStartTime = millis();

  publishPollingStatus();
  notifyWebClients(buildPollingStatusJSON());
}

void processPhase(DeviceInfo& device) {
  switch (device.phase) {
    case PHASE_HEALTH_CHECK: {
      // Send POLL command only once per phase entry
      if (!device.commandSent) {
        String seq = generateSequence(sequenceCounter);
        String message = config.gatewayId + ":" + String(CMD_POLL) + ":" + device.deviceId + ":" +
                         seq + ":" + String(getCurrentTimestamp()) + ":null";
        sendLoRaMessage(message, 1);
        device.commandSent = true;  // Mark as sent
      }
      // Wait for response (handled in processIncomingMessage)
      break;
    }

    case PHASE_START_INFERENCE: {
      // Send START_INFER command only once per phase entry
      if (!device.commandSent) {
        String seq = generateSequence(sequenceCounter);
        String message = config.gatewayId + ":" + String(CMD_START_INFER) + ":" + device.deviceId + ":" +
                         seq + ":" + String(getCurrentTimestamp()) + ":null";
        sendLoRaMessage(message, 1);
        device.commandSent = true;  // Mark as sent
      }
      break;
    }

    case PHASE_DATA_COLLECTION: {
      // Wait for DATA messages from device
      // Device will send 5 DATA messages
      // Each will be acknowledged in processIncomingMessage
      break;
    }

    case PHASE_FINALIZE: {
      // Send FINALIZE command only once per phase entry
      if (!device.commandSent) {
        String seq = generateSequence(sequenceCounter);
        String message = config.gatewayId + ":" + String(CMD_FINALIZE) + ":" + device.deviceId + ":" +
                         seq + ":" + String(getCurrentTimestamp()) + ":null";
        sendLoRaMessage(message, 1);
        device.commandSent = true;  // Mark as sent
      }
      break;
    }

    case PHASE_COMPLETE: {
      completeDevicePolling(device);
      break;
    }

    case PHASE_ERROR: {
      handleDeviceOffline(device);
      break;
    }

    default:
      break;
  }
}

void advancePhase(DeviceInfo& device) {
  switch (device.phase) {
    case PHASE_HEALTH_CHECK:
      device.phase = PHASE_START_INFERENCE;
      Serial.println("[POLLING] → START_INFERENCE");
      break;

    case PHASE_START_INFERENCE:
      device.phase = PHASE_DATA_COLLECTION;
      Serial.println("[POLLING] → DATA_COLLECTION");
      break;

    case PHASE_DATA_COLLECTION:
      if (device.positionsReceived >= 5) {
        device.phase = PHASE_FINALIZE;
        Serial.println("[POLLING] → FINALIZE");
      }
      break;

    case PHASE_FINALIZE:
      device.phase = PHASE_COMPLETE;
      Serial.println("[POLLING] → COMPLETE");
      break;

    default:
      break;
  }

  phaseStartTime = millis();
  device.retryCount = 0;
  device.commandSent = false;  // Reset flag when entering new phase
}

void handlePhaseTimeout(DeviceInfo& device) {
  Serial.println("[POLLING] ⚠ Timeout in phase: " + phaseToString(device.phase));

  device.retryCount++;

  if (device.retryCount >= MAX_RETRIES) {
    Serial.println("[POLLING] ✗ Max retries reached for " + device.deviceId);
    device.phase = PHASE_ERROR;
    failedPolls++;
  } else {
    // Retry with exponential backoff
    unsigned long backoff = RETRY_DELAY_BASE * (1 << (device.retryCount - 1));
    Serial.println("[POLLING] Retry " + String(device.retryCount) + "/" +
                   String(MAX_RETRIES) + " after " + String(backoff) + "ms");
    delay(backoff);
    device.commandSent = false;  // Reset flag to allow retry transmission
    phaseStartTime = millis();
  }
}

void handleDeviceOffline(DeviceInfo& device) {
  Serial.println("[POLLING] ✗ Device OFFLINE: " + device.deviceId);

  device.online = false;
  publishDeviceData(currentDeviceIndex);

  beepBuzzer(500);  // Alert beep
  setLEDColor(255, 0, 0);  // Red
  led.show();

  currentDeviceIndex++;
  if (currentDeviceIndex < config.numDevices) {
    delay(1000);
    pollNextDevice();
  }
}

void completeDevicePolling(DeviceInfo& device) {
  Serial.println("[POLLING] ✓ Device COMPLETE: " + device.deviceId);

  device.online = true;
  device.lastContact = millis();
  device.totalPolls++;
  device.successfulPolls++;
  successfulPolls++;

  publishDeviceData(currentDeviceIndex);

  setLEDColor(0, 255, 0);  // Green
  led.show();

  currentDeviceIndex++;
  if (currentDeviceIndex < config.numDevices) {
    delay(1000);
    pollNextDevice();
  }
}

// ==================== MESSAGE PROCESSING ====================

void processIncomingMessage(const LoRaMessage& msg) {
  // Handle PAIR_ACK (special case - device may not be fully registered yet)
  if (msg.command == "PAIR_ACK") {
    Serial.println("[PROTOCOL] ✓ PAIR_ACK received from " + msg.senderId);

    int deviceIndex = getDeviceIndexById(msg.senderId);
    if (deviceIndex != -1) {
      devices[deviceIndex].paired = true;
      devices[deviceIndex].online = true;  // Mark as online when pairing succeeds
      devices[deviceIndex].lastContact = millis();
      Serial.println("[PROTOCOL] ✓ Device paired successfully: " + msg.senderId);

      // Update display
      beepBuzzer(200);  // Success beep
      setLEDColor(0, 255, 0);  // Green
      led.show();

      // Notify web clients of device status update
      notifyWebClients(buildDeviceListJSON());
    } else {
      Serial.println("[PROTOCOL] ⚠ PAIR_ACK from unknown device: " + msg.senderId);
    }
    return;
  }

  // Find device index
  int deviceIndex = getDeviceIndexById(msg.senderId);
  if (deviceIndex == -1) {
    Serial.println("[PROTOCOL] Unknown device: " + msg.senderId);
    return;
  }

  DeviceInfo& device = devices[deviceIndex];

  // Handle different commands
  if (msg.command == CMD_ACK) {
    // Parse status from payload or targetId
    String status = msg.targetId;  // Format: ACK:GW01:ONLINE

    if (status == STATUS_ONLINE) {
      handleAckOnline(msg);
    } else if (status == STATUS_INFERRING) {
      handleAckInferring(msg);
    } else if (status == STATUS_FINALIZED) {
      handleAckFinalized(msg);
    } else if (status == STATUS_SLEEPING) {
      handleAckSleeping(msg);
    }
  } else if (msg.command == CMD_DATA) {
    handleDataMessage(msg);
  }
}

void handleAckOnline(const LoRaMessage& msg) {
  int deviceIndex = getDeviceIndexById(msg.senderId);
  if (deviceIndex == -1) return;

  DeviceInfo& device = devices[deviceIndex];

  Serial.println("[PROTOCOL] ✓ Device ONLINE: " + device.deviceId);

  // Parse health data
  LoRaMessage msgCopy = msg;
  parseHealthPayload(msgCopy);

  device.battery = msgCopy.health.battery;
  device.rssi = msgCopy.health.rssi;
  device.snr = msgCopy.health.snr;
  device.online = true;

  Serial.println("  Battery: " + String(device.battery) + "%");
  Serial.println("  RSSI: " + String(device.rssi) + " dBm");
  Serial.println("  SNR: " + String(device.snr) + " dB");

  advancePhase(device);
}

void handleAckInferring(const LoRaMessage& msg) {
  int deviceIndex = getDeviceIndexById(msg.senderId);
  if (deviceIndex == -1) return;

  DeviceInfo& device = devices[deviceIndex];

  Serial.println("[PROTOCOL] ✓ Device INFERRING: " + device.deviceId);

  advancePhase(device);
}

void handleDataMessage(const LoRaMessage& msg) {
  int deviceIndex = getDeviceIndexById(msg.senderId);
  if (deviceIndex == -1) return;

  DeviceInfo& device = devices[deviceIndex];

  // Parse data payload
  LoRaMessage msgCopy = msg;
  parseDataPayload(msgCopy);

  device.positionsReceived++;
  device.lastPosition = msgCopy.data.position;
  device.lastTableId = msgCopy.data.tableId;
  device.lastDetections = msgCopy.data.detections;

  Serial.println("[PROTOCOL] ✓ DATA received (" + String(device.positionsReceived) + "/5)");
  Serial.println("  Table: " + msgCopy.data.tableId);
  Serial.println("  Position: " + msgCopy.data.position);
  Serial.println("  Detections: " + msgCopy.data.detections);

  // Send ACK (simplified protocol - no HMAC)
  String seq = generateSequence(sequenceCounter);
  String ackPayload = String(device.positionsReceived) + "/5";
  String ackMessage = config.gatewayId + ":" + String(CMD_ACK) + ":" + device.deviceId + ":" +
                      seq + ":" + String(getCurrentTimestamp()) + ":" + ackPayload;
  sendLoRaMessage(ackMessage, 1);

  // Check if all positions received
  if (device.positionsReceived >= 5) {
    advancePhase(device);
  }
}

void handleAckFinalized(const LoRaMessage& msg) {
  int deviceIndex = getDeviceIndexById(msg.senderId);
  if (deviceIndex == -1) return;

  DeviceInfo& device = devices[deviceIndex];

  Serial.println("[PROTOCOL] ✓ Device FINALIZED: " + device.deviceId);

  // Send SLEEP command (simplified protocol - no HMAC)
  String seq = generateSequence(sequenceCounter);
  String sleepMessage = config.gatewayId + ":" + String(CMD_SLEEP) + ":" + device.deviceId + ":" +
                        seq + ":" + String(getCurrentTimestamp()) + ":null";
  sendLoRaMessage(sleepMessage, 1);
}

void handleAckSleeping(const LoRaMessage& msg) {
  int deviceIndex = getDeviceIndexById(msg.senderId);
  if (deviceIndex == -1) return;

  DeviceInfo& device = devices[deviceIndex];

  Serial.println("[PROTOCOL] ✓ Device SLEEPING: " + device.deviceId);

  advancePhase(device);
}

// ==================== MQTT PUBLISHING ====================

void publishGatewayStatus() {
  if (!mqttConnected) return;

  StaticJsonDocument<1024> doc;
  doc["gateway_id"] = config.gatewayId;
  doc["wifi_connected"] = wifiConnected;
  doc["mqtt_connected"] = mqttConnected;
  doc["polling_active"] = pollingActive;
  doc["devices_paired"] = config.numDevices;
  doc["total_messages"] = totalMessages;
  doc["successful_polls"] = successfulPolls;
  doc["failed_polls"] = failedPolls;
  doc["uptime_ms"] = millis();
  doc["ip_address"] = WiFi.localIP().toString();

  JsonObject location = doc.createNestedObject("location");
  location["building"] = config.building;
  location["floor"] = config.floor;
  location["lab"] = config.lab;

  char buffer[1024];
  serializeJson(doc, buffer);

  mqttClient.publish(topic_status, buffer, true);  // Retained
}

void publishPollingStatus() {
  if (!mqttConnected) return;

  StaticJsonDocument<512> doc;
  doc["polling_active"] = pollingActive;
  doc["current_device_index"] = currentDeviceIndex;
  doc["total_devices"] = config.numDevices;
  doc["elapsed_ms"] = millis() - pollingStartTime;

  if (currentDeviceIndex < config.numDevices) {
    doc["current_device_id"] = devices[currentDeviceIndex].deviceId;
    doc["current_phase"] = phaseToString(devices[currentDeviceIndex].phase);
  }

  char buffer[512];
  serializeJson(doc, buffer);

  mqttClient.publish(topic_polling, buffer, false);
}

void publishDeviceData(int deviceIndex) {
  if (!mqttConnected || deviceIndex < 0 || deviceIndex >= config.numDevices) return;

  DeviceInfo& device = devices[deviceIndex];

  StaticJsonDocument<1024> doc;
  doc["gateway_id"] = config.gatewayId;
  doc["device_id"] = device.deviceId;
  doc["online"] = device.online;
  doc["battery"] = device.battery;
  doc["rssi"] = device.rssi;
  doc["snr"] = device.snr;
  doc["last_position"] = device.lastPosition;
  doc["last_table_id"] = device.lastTableId;
  doc["last_detections"] = device.lastDetections;
  doc["positions_received"] = device.positionsReceived;
  doc["total_polls"] = device.totalPolls;
  doc["successful_polls"] = device.successfulPolls;
  doc["failed_polls"] = device.failedPolls;
  doc["last_contact"] = device.lastContact;
  doc["timestamp"] = millis();

  char buffer[1024];
  serializeJson(doc, buffer);

  String deviceTopic = String(topic_device) + device.deviceId;
  mqttClient.publish(deviceTopic.c_str(), buffer, false);
}

void publishPollingComplete() {
  if (!mqttConnected) return;

  StaticJsonDocument<512> doc;
  doc["gateway_id"] = config.gatewayId;
  doc["cycle_complete"] = true;
  doc["duration_ms"] = millis() - pollingStartTime;
  doc["devices_polled"] = config.numDevices;
  doc["successful"] = successfulPolls;
  doc["failed"] = failedPolls;
  doc["timestamp"] = millis();

  char buffer[512];
  serializeJson(doc, buffer);

  mqttClient.publish(topic_data, buffer, false);
}

// ==================== WEB INTERFACE ====================

void handleWebSocketMessage(AsyncWebSocketClient* client, char* data, size_t len) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    Serial.println("[WS] Invalid JSON");
    return;
  }

  String command = doc["command"];

  if (command == "start_polling") {
    if (!pollingActive) {
      startPollingCycle();
      client->text("{\"status\":\"started\"}");
    } else {
      client->text("{\"error\":\"already_active\"}");
    }
  } else if (command == "get_status") {
    client->text(buildPollingStatusJSON());
  }
}

void notifyWebClients(const String& message) {
  ws.textAll(message);
}

String buildDeviceListJSON() {
  StaticJsonDocument<4096> doc;

  // Add stats section
  JsonObject stats = doc.createNestedObject("stats");
  stats["gateway_id"] = config.gatewayId;
  stats["ip_address"] = WiFi.localIP().toString();
  stats["uptime_ms"] = millis();
  stats["total_devices"] = config.numDevices;

  // Count online devices
  int onlineCount = 0;
  for (int i = 0; i < config.numDevices; i++) {
    if (devices[i].online) onlineCount++;
  }
  stats["online_devices"] = onlineCount;
  stats["total_messages"] = totalMessages;

  // Calculate success rate
  unsigned long totalAttempts = successfulPolls + failedPolls;
  float successRate = (totalAttempts > 0) ? (successfulPolls * 100.0 / totalAttempts) : 0.0;
  stats["success_rate"] = successRate;

  // Add devices array
  JsonArray devicesArray = doc.createNestedArray("devices");

  for (int i = 0; i < config.numDevices; i++) {
    JsonObject deviceObj = devicesArray.createNestedObject();
    deviceObj["device_id"] = devices[i].deviceId;
    deviceObj["paired"] = devices[i].paired;
    deviceObj["online"] = devices[i].online;
    deviceObj["table_left"] = devices[i].tableLeft;
    deviceObj["table_right"] = devices[i].tableRight;
    deviceObj["battery"] = devices[i].battery;
    deviceObj["rssi"] = devices[i].rssi;
    deviceObj["snr"] = devices[i].snr;
    deviceObj["phase"] = phaseToString(devices[i].phase);
    deviceObj["last_contact"] = devices[i].lastContact;
    deviceObj["positions_received"] = devices[i].positionsReceived;
  }

  char buffer[4096];
  serializeJson(doc, buffer);
  return String(buffer);
}

String buildPollingStatusJSON() {
  StaticJsonDocument<512> doc;
  doc["polling_active"] = pollingActive;
  doc["current_device_index"] = currentDeviceIndex;
  doc["total_devices"] = config.numDevices;
  doc["elapsed_ms"] = millis() - pollingStartTime;

  if (currentDeviceIndex < config.numDevices) {
    doc["current_device_id"] = devices[currentDeviceIndex].deviceId;
    doc["current_phase"] = phaseToString(devices[currentDeviceIndex].phase);
  }

  char buffer[512];
  serializeJson(doc, buffer);
  return String(buffer);
}

// ==================== DISPLAY & INDICATORS ====================

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  //display.print("GW:");
  display.print(config.gatewayId);
  display.print(" ");
  if (wifiConnected) display.print("WiFi:OK ");
  if (mqttConnected) display.print("MQTT:OK");

  display.setCursor(0, 12);
  if (pollingActive) {
    display.print("Polling:");
    display.print(currentDeviceIndex + 1);
    display.print("/");
    display.print(config.numDevices);
    if (currentDeviceIndex < config.numDevices) {
      display.print(" ");
      display.print(devices[currentDeviceIndex].deviceId);
    }
  } else {
    display.print("Idle - Msgs:");
    display.print(totalMessages);
  }

  display.setCursor(0, 24);
  display.print("OK:");
  display.print(successfulPolls);
  display.print(" Fail:");
  display.print(failedPolls);

  display.display();
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  led.setPixelColor(0, led.Color(r, g, b));
  led.show();
}

void beepBuzzer(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

// ==================== STORAGE & REPORTS ====================

void saveConfiguration() {
  preferences.begin("detectra", false);  // Read-write mode

  preferences.putString("gateway_id", config.gatewayId);
  preferences.putString("building", config.building);
  preferences.putString("floor", config.floor);
  preferences.putString("lab", config.lab);
  preferences.putInt("poll_interval", config.pollingIntervalMinutes);
  preferences.putInt("num_devices", config.numDevices);

  preferences.end();

  Serial.println("[CONFIG] Configuration saved to NVS");
}

void saveDevicePairing(const DeviceInfo& device) {
  // Load existing device_secrets.json
  StaticJsonDocument<4096> doc;

  File file = FFat.open("/device_secrets.json", "r");
  if (file) {
    deserializeJson(doc, file);
    file.close();
  }

  // Update gateway ID
  doc["gateway_id"] = config.gatewayId;

  // Get or create devices array
  JsonArray devicesArray;
  if (doc.containsKey("devices")) {
    devicesArray = doc["devices"];
  } else {
    devicesArray = doc.createNestedArray("devices");
  }

  // Check if device already exists
  bool found = false;
  for (JsonObject deviceObj : devicesArray) {
    if (deviceObj["device_id"] == device.deviceId) {
      // Update existing
      deviceObj["shared_secret"] = device.sharedSecret;
      found = true;
      break;
    }
  }

  // Add new device if not found
  if (!found) {
    JsonObject newDevice = devicesArray.createNestedObject();
    newDevice["device_id"] = device.deviceId;
    newDevice["shared_secret"] = device.sharedSecret;
  }

  // Write back to file
  file = FFat.open("/device_secrets.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("[CONFIG] Device pairing saved: " + device.deviceId);
  } else {
    Serial.println("[CONFIG] Failed to save device pairing!");
  }
}

void generateCycleReport() {
  // FFat disabled - report generation skipped
  Serial.println("[REPORT] Cycle complete (FFat report disabled)");

  // FFat-based report disabled:
  // // Generate CSV report of polling cycle
  // String timestamp = String(millis() / 1000);
  // String filename = "/report_" + timestamp + ".csv";
  //
  // File file = FFat.open(filename, "w");
  // if (!file) {
  //   Serial.println("[REPORT] Failed to create report file");
  //   return;
  // }
  //
  // // CSV Header
  // file.println("Device ID,Online,Battery,RSSI,SNR,Positions,Last Table,Last Position,Last Detections");
  //
  // // Device rows
  // for (int i = 0; i < config.numDevices; i++) {
  //   file.print(devices[i].deviceId);
  //   file.print(",");
  //   file.print(devices[i].online ? "1" : "0");
  //   file.print(",");
  //   file.print(devices[i].battery);
  //   file.print(",");
  //   file.print(devices[i].rssi);
  //   file.print(",");
  //   file.print(devices[i].snr);
  //   file.print(",");
  //   file.print(devices[i].positionsReceived);
  //   file.print(",");
  //   file.print(devices[i].lastTableId);
  //   file.print(",");
  //   file.print(devices[i].lastPosition);
  //   file.print(",");
  //   file.println(devices[i].lastDetections);
  // }
  //
  // file.close();
  // Serial.println("[REPORT] Cycle report saved: " + filename);
}

// ==================== UTILITIES ====================

String getDeviceSecret(const String& deviceId) {
  for (int i = 0; i < config.numDevices; i++) {
    if (devices[i].deviceId == deviceId) {
      return devices[i].sharedSecret;
    }
  }
  return "";
}

int getDeviceIndexById(const String& deviceId) {
  for (int i = 0; i < config.numDevices; i++) {
    if (devices[i].deviceId == deviceId) {
      return i;
    }
  }
  return -1;
}
