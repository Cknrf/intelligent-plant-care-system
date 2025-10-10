#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include "../../config.h"

// =============================================================================
// WEBHOOK NOTIFICATION MODULE
// =============================================================================

WiFiSSLClient sslClient;
bool isNotificationSystemReady = false;

// Notification types
enum NotificationType {
  INFO,
  WARNING,
  CRITICAL,
  SUCCESS
};

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
bool initializeNotificationSystem();
bool sendNotification(const String& message, NotificationType type = INFO);
bool sendDiscordWebhook(const String& message, NotificationType type);
String formatNotificationMessage(const String& message, NotificationType type);
String getNotificationEmoji(NotificationType type);
bool ensureWiFiConnection();

// =============================================================================
// SETUP FUNCTION
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 5000); // Wait max 5 seconds for Serial
  
  Serial.println("=== Webhook Notification Module ===");
  
  if (!initializeNotificationSystem()) {
    Serial.println("Failed to initialize notification system!");
    while (true) {
      delay(1000);
    }
  }
  
  Serial.println("Notification system initialized successfully!");
  
  // Send startup notification
  sendNotification("Plant Care System started successfully!", SUCCESS);
}

// =============================================================================
// MAIN LOOP (for testing purposes)
// =============================================================================
void loop() {
  // This is just for testing - in real implementation, 
  // notifications would be triggered by events
  static unsigned long lastTestNotification = 0;
  unsigned long currentTime = millis();
  
  // Send test notification every 5 minutes
  if (currentTime - lastTestNotification >= 300000) {
    sendNotification("System status: All sensors operational", INFO);
    lastTestNotification = currentTime;
  }
  
  delay(10000); // Check every 10 seconds
}

// =============================================================================
// NOTIFICATION SYSTEM FUNCTIONS
// =============================================================================

/**
 * Initialize WiFi connection for notification system
 */
bool initializeNotificationSystem() {
  Serial.println("Initializing notification system...");
  
  if (!ensureWiFiConnection()) {
    return false;
  }
  
  isNotificationSystemReady = true;
  return true;
}

/**
 * Ensure WiFi connection is active
 */
bool ensureWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  
  Serial.println("Connecting to WiFi...");
  unsigned long startTime = millis();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED && 
         (millis() - startTime) < WIFI_TIMEOUT_MS) {
    delay(WIFI_RETRY_DELAY_MS);
    Serial.print(".");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed!");
    return false;
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
}

/**
 * Send notification with retry mechanism
 */
bool sendNotification(const String& message, NotificationType type) {
  if (!isNotificationSystemReady) {
    Serial.println("Notification system not ready");
    return false;
  }
  
  if (!ensureWiFiConnection()) {
    Serial.println("WiFi not available for notification");
    return false;
  }
  
  // Retry mechanism
  for (int attempt = 1; attempt <= NOTIFICATION_RETRY_COUNT; attempt++) {
    Serial.print("Sending notification (attempt ");
    Serial.print(attempt);
    Serial.print("/");
    Serial.print(NOTIFICATION_RETRY_COUNT);
    Serial.println(")...");
    
    if (sendDiscordWebhook(message, type)) {
      Serial.println("Notification sent successfully!");
      return true;
    }
    
    if (attempt < NOTIFICATION_RETRY_COUNT) {
      Serial.print("Retry in ");
      Serial.print(NOTIFICATION_RETRY_DELAY_MS / 1000);
      Serial.println(" seconds...");
      delay(NOTIFICATION_RETRY_DELAY_MS);
    }
  }
  
  Serial.println("Failed to send notification after all retries");
  return false;
}

/**
 * Send Discord webhook notification
 */
bool sendDiscordWebhook(const String& message, NotificationType type) {
  // Force new SSL connection
  sslClient.stop();
  
  HttpClient http(sslClient, DISCORD_HOST, DISCORD_PORT);
  
  String formattedMessage = formatNotificationMessage(message, type);
  String content = "{\"content\":\"" + formattedMessage + "\"}";
  
  // Send HTTP request
  http.beginRequest();
  http.post(DISCORD_WEBHOOK_PATH);
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", content.length());
  http.beginBody();
  http.print(content);
  http.endRequest();
  
  // Get response
  int statusCode = http.responseStatusCode();
  String response = http.responseBody();
  
  if (DEBUG_MODE) {
    Serial.print("HTTP Status: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);
  }
  
  http.stop();
  
  // Check if request was successful
  return (statusCode >= 200 && statusCode < 300);
}

/**
 * Format notification message with emoji and timestamp
 */
String formatNotificationMessage(const String& message, NotificationType type) {
  String emoji = getNotificationEmoji(type);
  String timestamp = String(millis() / 1000); // Simple timestamp in seconds
  
  return emoji + " **Plant Care System** - " + message + " _(t:" + timestamp + ")_";
}

/**
 * Get emoji based on notification type
 */
String getNotificationEmoji(NotificationType type) {
  switch (type) {
    case INFO:     return "ℹ️";
    case WARNING:  return "⚠️";
    case CRITICAL: return "🚨";
    case SUCCESS:  return "✅";
    default:       return "📢";
  }
}

// =============================================================================
// PUBLIC API FUNCTIONS (for use by main system)
// =============================================================================

/**
 * Send info notification
 */
bool notifyInfo(const String& message) {
  return sendNotification(message, INFO);
}

/**
 * Send warning notification
 */
bool notifyWarning(const String& message) {
  return sendNotification(message, WARNING);
}

/**
 * Send critical notification
 */
bool notifyCritical(const String& message) {
  return sendNotification(message, CRITICAL);
}

/**
 * Send success notification
 */
bool notifySuccess(const String& message) {
  return sendNotification(message, SUCCESS);
}

/**
 * Send plant care specific notifications
 */
bool notifyLowMoisture(float moistureLevel) {
  String message = "Low soil moisture detected: " + String(moistureLevel) + "%. Watering needed!";
  return sendNotification(message, WARNING);
}

bool notifyWateringComplete(float moistureLevel) {
  String message = "Watering completed. New moisture level: " + String(moistureLevel) + "%";
  return sendNotification(message, SUCCESS);
}

bool notifyLowLight(int luxLevel) {
  String message = "Low light detected: " + String(luxLevel) + " lux. Consider supplemental lighting.";
  return sendNotification(message, INFO);
}

bool notifySystemError(const String& errorDetails) {
  String message = "System error: " + errorDetails;
  return sendNotification(message, CRITICAL);
}