#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Servo.h>
#include "config.h"

struct WeatherData {
  float temperature;
  int humidity;
  String description;
  bool isValid;
  
  // Forecast-specific data (simplified 6-hour window)
  float rainAmount3h;         // Expected rainfall in next 3 hours (mm)
  float rainAmount6h;         // Expected rainfall in next 6 hours (mm)
  unsigned long forecastTime; // When forecast was fetched (millis)
};

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// Sensor readings
int soilMoisture1 = 0;
int soilMoisture2 = 0;
float currentLux = 0;
int rainValue = 1023;
bool rainDetected = false;
bool luxSensorAvailable = false;

// =============================================================================
// ACTUATOR OBJECTS
// =============================================================================
Servo shadeServo;
int currentShadePosition = SHADE_POSITION_OFF;
bool pumpRunning = false;
bool valve1Open = false;
bool valve2Open = false;

// =============================================================================
// NETWORK OBJECTS
// =============================================================================
WiFiSSLClient sslClient;
WeatherData currentWeather;

// =============================================================================
// TIMING VARIABLES
// =============================================================================
unsigned long lastSensorUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastStatusReport = 0;
unsigned long lastWateringTime1 = 0;  // Track last watering for Plant 1
unsigned long lastWateringTime2 = 0;  // Track last watering for Plant 2
unsigned long pumpStartTime = 0;
unsigned long wateringStartTime1 = 0;
unsigned long wateringStartTime2 = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastSensorHealthCheck = 0;

// =============================================================================
// SYSTEM STATE
// =============================================================================
bool systemInitialized = false;
bool wifiConnected = false;
bool currentlyWatering1 = false;
bool currentlyWatering2 = false;
bool offlineMode = false;  // True when weather API is unavailable

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
// Initialization
void initSystem();
bool initWiFi();
bool initSensors();
void initActuators();

// Network maintenance
void checkWiFiConnection();
void checkSensorHealth();

// Sensor functions
void updateAllSensors();
int readSoilMoisture(int pin);
void updateLuxReading();
void updateRainReading();
bool isDry(int plantNum);
bool isWet(int plantNum);
bool isOptimal(int plantNum);
bool isHighLight();
bool isRaining();

// Actuator functions
void deployShade();
void retractShade();
void pumpOn();
void pumpOff();
void openValve(int plantNum);
void closeValve(int plantNum);
void closeAllValves();
void checkPumpSafety();

// Plant care logic
void processPlant(int plantNum);
bool shouldSkipWateringForRain(int plantNum);
void waterPlant(int plantNum);
void stopWatering(int plantNum);
bool canWaterPlant(int plantNum);
void checkWateringCompletion();
int getShadePreference(int plantNum);
void updateShadeBasedOnBothPlants();

// Weather API
bool updateWeather();
WeatherData parseWeatherResponse(const String& response);

// Notifications
bool sendNotification(const String& message, int type);
void sendStatusUpdate();

// Utilities
void printSystemStatus();

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000);  // Give Serial time to initialize

  Serial.println("\n\n===============================================");
  Serial.println("  INTELLIGENT ADAPTIVE PLANT CARE SYSTEM");
  Serial.println("===============================================\n");

  initSystem();

  Serial.println("\n=== System Ready ===\n");
  printSystemStatus();
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void loop() {
  unsigned long currentTime = millis();

  // Update all sensors periodically
  if (currentTime - lastSensorUpdate >= MAIN_LOOP_INTERVAL_MS) {
    updateAllSensors();
    lastSensorUpdate = currentTime;

    // Process each plant independently
    processPlant(1);
    processPlant(2);
    
    // Update shade based on both plants' needs
    updateShadeBasedOnBothPlants();
  }

  // Update weather data periodically (every 1 hour)
  if (currentTime - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL_MS) {
    if (wifiConnected) {
      if (updateWeather()) {
        if (offlineMode) {
          offlineMode = false;  // Weather API recovered!
          Serial.println("✅ Weather API recovered - ONLINE MODE");
          sendNotification("Weather API connection restored!", 3);
        }
      }
    }
    lastWeatherUpdate = currentTime;
  }

  // Send status update periodically (every 30 minutes)
  if (currentTime - lastStatusReport >= STATUS_UPDATE_INTERVAL_MS) {
    if (wifiConnected) {
      sendStatusUpdate();
    }
    lastStatusReport = currentTime;
  }

  // Always check pump safety
  checkPumpSafety();

  // Check watering completion
  checkWateringCompletion();

  // Check WiFi connection periodically
  if (currentTime - lastWiFiCheck >= WIFI_CHECK_INTERVAL_MS) {
    checkWiFiConnection();
    lastWiFiCheck = currentTime;
  }

  // Check sensor health periodically
  if (currentTime - lastSensorHealthCheck >= SENSOR_HEALTH_CHECK_INTERVAL_MS) {
    checkSensorHealth();
    lastSensorHealthCheck = currentTime;
  }

  // No delay needed - non-blocking timers handle everything
  // Your loop runs continuously for responsive safety checks
}

// =============================================================================
// INITIALIZATION FUNCTIONS
// =============================================================================

void initSystem() {
  Serial.println("Initializing system...");

  // Initialize WiFi first
  wifiConnected = initWiFi();

  // Initialize sensors
  if (!initSensors()) {
    Serial.println("WARNING: Some sensors failed to initialize!");
  }

  // Initialize actuators
  initActuators();

  // Get initial weather data
  if (wifiConnected) {
    if (updateWeather()) {
      offlineMode = false;  // Weather API works
      sendNotification("Plant Care System started successfully!", 3);  // SUCCESS
    } else {
      offlineMode = true;   // Weather API failed, use sensors only
      Serial.println("⚠️ Starting in OFFLINE MODE - Weather API unavailable");
      Serial.println("   System will use local sensors only for decisions");
    }
  } else {
    offlineMode = true;     // No WiFi, definitely offline
    Serial.println("⚠️ Starting in OFFLINE MODE - No WiFi connection");
  }

  systemInitialized = true;
  Serial.println("System initialization complete!");
}

bool initWiFi() {
  Serial.println("\n╔════════════════════════════════════════════════════════════╗");
  Serial.println("║                   🌐 WiFi CONNECTION                       ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝");

  Serial.print("📡 SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("🔑 Password: ");
  for (int i = 0; i < strlen(WIFI_PASSWORD); i++) Serial.print("*");
  Serial.println();
  Serial.print("⏱️ Timeout: ");
  Serial.print(WIFI_TIMEOUT_MS / 1000);
  Serial.println(" seconds\n");

  Serial.println("Attempting connection");

  unsigned long startTime = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int dotCount = 0;
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
    dotCount++;
    if (dotCount % 20 == 0) {
      Serial.println();
      Serial.print("Status: ");
      switch (WiFi.status()) {
        case WL_IDLE_STATUS:
          Serial.println("IDLE");
          break;
        case WL_NO_SSID_AVAIL:
          Serial.println("SSID NOT FOUND!");
          Serial.println("⚠️ Check if you are connected to the wifi");
          break;
        case WL_CONNECT_FAILED:
          Serial.println("CONNECTION FAILED!");
          Serial.println("⚠️ Check password is correct");
          break;
        case WL_CONNECTION_LOST:
          Serial.println("CONNECTION LOST");
          break;
        case WL_DISCONNECTED:
          Serial.println("DISCONNECTED - Retrying...");
          break;
        default:
          Serial.print("Unknown (");
          Serial.print(WiFi.status());
          Serial.println(")");
      }
    }
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected Successfully!");
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("📶 Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("🌐 Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.println("════════════════════════════════════════════════════════════\n");
    return true;
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.print("Final Status: ");
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        Serial.println("SSID NOT FOUND");
        Serial.println("\n🔍 TROUBLESHOOTING:");
        Serial.println("   1. Ensure iPhone hotspot is ON");
        Serial.println("   2. Check SSID name is exactly 'iPhone'");
        Serial.println("   3. Move Arduino closer to iPhone");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("AUTHENTICATION FAILED");
        Serial.println("\n🔍 TROUBLESHOOTING:");
        Serial.println("   1. Verify password is 'Mearck123'");
        Serial.println("   2. Check for case sensitivity");
        Serial.println("   3. Re-enter password in config.h");
        break;
      default:
        Serial.println("TIMEOUT");
        Serial.println("\n🔍 TROUBLESHOOTING:");
        Serial.println("   1. Restart iPhone hotspot");
        Serial.println("   2. Check Arduino is within range");
        Serial.println("   3. Try increasing WIFI_TIMEOUT_MS");
    }
    Serial.println("════════════════════════════════════════════════════════════");
    Serial.println("\n⚠️ System will operate WITHOUT WiFi features:");
    Serial.println("   - No weather API integration");
    Serial.println("   - No Discord notifications");
    Serial.println("   - Local sensor-based decisions only\n");
    return false;
  }
}

bool initSensors() {
  Serial.println("\nInitializing sensors...");
  bool allSuccess = true;

  // Soil moisture sensors
  pinMode(SOIL_MOISTURE_PIN_1, INPUT);
  pinMode(SOIL_MOISTURE_PIN_2, INPUT);
  Serial.println("  [OK] Soil moisture sensors");

  // Lux sensor (I2C)
  Serial.print("  Initializing lux sensor...");
  int attempts = 0;
  while (!tsl.begin() && attempts < 5) {
    attempts++;
    delay(200);
  }

  if (attempts < 5) {
    tsl.enableAutoRange(true);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);  // 13MS for outdoor use (consistent)
    luxSensorAvailable = true;
    Serial.println(" [OK] Light sensor");
  } else {
    Serial.println(" [FAILED]");
    allSuccess = false;
  }

  // Rain sensor
  pinMode(RAIN_SENSOR_PIN, INPUT);
  Serial.println("  [OK] Rain sensor");

  // Take initial readings
  updateAllSensors();

  return allSuccess;
}

void initActuators() {
  Serial.println("\nInitializing actuators...");

  // Servo motor
  shadeServo.attach(SERVO_PIN);
  shadeServo.write(SHADE_POSITION_OFF);
  currentShadePosition = SHADE_POSITION_OFF;
  delay(500);
  Serial.println("  [OK] Servo motor (shade)");

  // Water pump
  pinMode(WATER_PUMP_PIN, OUTPUT);
  digitalWrite(WATER_PUMP_PIN, LOW);
  pumpRunning = false;
  Serial.println("  [OK] Water pump");

  // Solenoid valves
  pinMode(SOLENOID_VALVE_1_PIN, OUTPUT);
  pinMode(SOLENOID_VALVE_2_PIN, OUTPUT);
  digitalWrite(SOLENOID_VALVE_1_PIN, LOW);
  digitalWrite(SOLENOID_VALVE_2_PIN, LOW);
  valve1Open = false;
  valve2Open = false;
  Serial.println("  [OK] Solenoid valves");
}

// =============================================================================
// NETWORK MAINTENANCE FUNCTIONS
// =============================================================================

void checkWiFiConnection() {
  // Only check if we think we're connected
  if (wifiConnected && WiFi.status() != WL_CONNECTED) {
    Serial.println("\n⚠️ WiFi connection lost! Attempting reconnection...");
    wifiConnected = false;

    // Simple retry - 3 attempts
    for (int attempt = 1; attempt <= WIFI_RECONNECT_ATTEMPTS; attempt++) {
      Serial.print("Reconnection attempt ");
      Serial.print(attempt);
      Serial.print("/");
      Serial.println(WIFI_RECONNECT_ATTEMPTS);

      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

      unsigned long startTime = millis();
      while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 5000) {  // 5 second timeout per attempt
        delay(500);
        Serial.print(".");
      }
      Serial.println();

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi reconnected successfully!");
        Serial.print("📡 IP Address: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;

        // Send notification about reconnection
        sendNotification("WiFi connection restored!", 0);
        return;
      }
    }

    Serial.println("❌ WiFi reconnection failed. Will retry in 1 minute.");
    Serial.println("   System continues operating on local sensors only.");
  }
}

void checkSensorHealth() {
  // Simple health check for lux sensor
  if (luxSensorAvailable) {
    sensors_event_t event;
    bool reading = tsl.getEvent(&event);

    if (!reading || event.light < 0) {
      Serial.println("⚠️ Lux sensor error detected, attempting recovery...");
      luxSensorAvailable = false;

      // Try to reinitialize
      delay(500);
      if (tsl.begin()) {
        tsl.enableAutoRange(true);
        tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);  // Consistent with initialization
        luxSensorAvailable = true;
        Serial.println("✅ Lux sensor recovered!");

        if (wifiConnected) {
          sendNotification("Lux sensor recovered successfully", 0);
        }
      } else {
        Serial.println("❌ Lux sensor still not responding. System operates without light detection.");
      }
    }
  } else {
    // If sensor was unavailable, try to reconnect every check
    if (tsl.begin()) {
      tsl.enableAutoRange(true);
      tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);  // Consistent with initialization
      luxSensorAvailable = true;
      Serial.println("✅ Lux sensor reconnected!");

      if (wifiConnected) {
        sendNotification("Lux sensor reconnected!", 3);
      }
    }
  }
}

// =============================================================================
// SENSOR FUNCTIONS
// =============================================================================

void updateAllSensors() {
  // Update soil moisture
  soilMoisture1 = readSoilMoisture(SOIL_MOISTURE_PIN_1);
  soilMoisture2 = readSoilMoisture(SOIL_MOISTURE_PIN_2);

  // Update lux
  updateLuxReading();

  // Update rain
  updateRainReading();

  if (DEBUG_MODE) {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║              📊 COMPREHENSIVE SENSOR READINGS              ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");

    // Plant 1 Status
    Serial.print("🌱 Plant 1 Moisture: ");
    Serial.print(soilMoisture1);
    Serial.print("%");
    if (isDry(1)) Serial.print(" [🟤 DRY - NEEDS WATER]");
    else if (isWet(1)) Serial.print(" [💙 WET - ENOUGH WATER]");
    else Serial.print(" [💚 OPTIMAL - PERFECT]");
    if (currentlyWatering1) Serial.print(" 💧 [WATERING ACTIVE]");
    Serial.println();

    // Plant 2 Status
    Serial.print("🌱 Plant 2 Moisture: ");
    Serial.print(soilMoisture2);
    Serial.print("%");
    if (isDry(2)) Serial.print(" [🟤 DRY - NEEDS WATER]");
    else if (isWet(2)) Serial.print(" [💙 WET - ENOUGH WATER]");
    else Serial.print(" [💚 OPTIMAL - PERFECT]");
    if (currentlyWatering2) Serial.print(" 💧 [WATERING ACTIVE]");
    Serial.println();

    // Light Level
    Serial.print("💡 Light Level: ");
    Serial.print(currentLux, 0);
    Serial.print(" lux");
    if (isHighLight()) Serial.print(" [☀️ HIGH LIGHT - SHADE NEEDED]");
    else Serial.print(" [🌤️ Normal light]");
    Serial.println();

    // Rain Status
    Serial.print("🌧️ Rain Sensor: ");
    Serial.print(rainValue);
    Serial.print(" (threshold: ");
    Serial.print(RAIN_THRESHOLD);
    Serial.print(")");
    if (isRaining()) Serial.print(" [🌧️ RAIN DETECTED]");
    else Serial.print(" [☀️ No rain]");
    Serial.println();

    // Weather API Data (Forecast)
    if (currentWeather.isValid) {
      Serial.print("🌡️ Weather Forecast: ");
      Serial.print(currentWeather.temperature, 1);
      Serial.print("°C, ");
      Serial.print(currentWeather.humidity);
      Serial.print("% humidity, ");
      Serial.println(currentWeather.description);
      
      Serial.print("   🌦️ Rain: ");
      Serial.print(currentWeather.rainAmount3h, 1);
      Serial.print("mm (next 3h) | ");
      Serial.print(currentWeather.rainAmount6h, 1);
      Serial.print("mm (next 6h)");
      
      // Age of forecast
      unsigned long forecastAge = (millis() - currentWeather.forecastTime) / 60000;  // minutes
      Serial.print(" | Updated ");
      Serial.print(forecastAge);
      Serial.println(" min ago");
    } else {
      Serial.println("🌡️ Weather Forecast: [Not available yet]");
    }

    // Actuator Status
    Serial.print("🏠 Shade Position: ");
    Serial.print(currentShadePosition);
    Serial.print("° ");
    if (currentShadePosition == SHADE_POSITION_ON) Serial.print("[DEPLOYED - Covering plants]");
    else Serial.print("[RETRACTED - Plants exposed]");
    Serial.println();

    Serial.print("💧 Water Pump: ");
    if (pumpRunning) {
      Serial.print("[RUNNING 🟢] Duration: ");
      Serial.print((millis() - pumpStartTime) / 1000.0, 1);
      Serial.print("s");
    } else {
      Serial.print("[OFF 🔴]");
    }
    Serial.println();

    Serial.print("🔧 Valve 1: ");
    Serial.print(valve1Open ? "[OPEN 🟢]" : "[CLOSED 🔴]");
    Serial.print(" | Valve 2: ");
    Serial.println(valve2Open ? "[OPEN 🟢]" : "[CLOSED 🔴]");

    Serial.println("════════════════════════════════════════════════════════════\n");
  }
}

int readSoilMoisture(int pin) {
  int rawValue = analogRead(pin);
  
  // Sensor validation: soil sensors typically range 100-1023
  if (rawValue < 100 || rawValue > 1023) {
    Serial.print("⚠️ Invalid soil sensor reading on pin ");
    Serial.print(pin);
    Serial.print(": ");
    Serial.println(rawValue);
    // Return a safe default (optimal moisture) rather than invalid data
    return 50;  // Return optimal moisture level to prevent emergency actions
  }
  
  int percentage = map(rawValue, SOIL_SENSOR_DRY_VALUE, SOIL_SENSOR_WET_VALUE, 0, 100);
  return constrain(percentage, 0, 100);
}

void updateLuxReading() {
  if (!luxSensorAvailable) {
    currentLux = 0;
    return;
  }

  sensors_event_t event;
  tsl.getEvent(&event);

  if (event.light > 0) {
    currentLux = event.light;
  }
}

void updateRainReading() {
  rainValue = analogRead(RAIN_SENSOR_PIN);
  rainDetected = (rainValue < RAIN_THRESHOLD);
}

bool isDry(int plantNum) {
  int moisture = (plantNum == 1) ? soilMoisture1 : soilMoisture2;
  return moisture < SOIL_MOISTURE_THRESHOLD_DRY;
}

bool isWet(int plantNum) {
  int moisture = (plantNum == 1) ? soilMoisture1 : soilMoisture2;
  return moisture > SOIL_MOISTURE_THRESHOLD_WET;
}

bool isOptimal(int plantNum) {
  int moisture = (plantNum == 1) ? soilMoisture1 : soilMoisture2;
  return moisture >= SOIL_MOISTURE_THRESHOLD_DRY && moisture <= SOIL_MOISTURE_THRESHOLD_WET;
}

bool isHighLight() {
  return currentLux > LUX_THRESHOLD_HIGH;
}

bool isRaining() {
  return rainDetected;
}

// =============================================================================
// ACTUATOR FUNCTIONS
// =============================================================================

void deployShade() {
  if (currentShadePosition != SHADE_POSITION_ON) {
    Serial.println("[ACTION] Deploying shade...");

    // Smooth movement from 180° down to 90°
    for (int pos = currentShadePosition; pos >= SHADE_POSITION_ON; pos--) {
      shadeServo.write(pos);
      delay(SERVO_SPEED_DELAY_MS);
    }

    currentShadePosition = SHADE_POSITION_ON;
    Serial.println("[ACTION] Shade deployed");
  }
}

void retractShade() {
  if (currentShadePosition != SHADE_POSITION_OFF) {
    Serial.println("[ACTION] Retracting shade...");

    // Smooth movement from 90° up to 180°
    for (int pos = currentShadePosition; pos <= SHADE_POSITION_OFF; pos++) {
      shadeServo.write(pos);
      delay(SERVO_SPEED_DELAY_MS);
    }

    currentShadePosition = SHADE_POSITION_OFF;
    Serial.println("[ACTION] Shade retracted");
  }
}

void pumpOn() {
  if (!pumpRunning) {
    digitalWrite(WATER_PUMP_PIN, HIGH);
    pumpRunning = true;
    pumpStartTime = millis();
    Serial.println("[PUMP] ON");
  }
}

void pumpOff() {
  if (pumpRunning) {
    digitalWrite(WATER_PUMP_PIN, LOW);
    pumpRunning = false;
    Serial.println("[PUMP] OFF");
  }
}

void openValve(int plantNum) {
  if (plantNum == 1 && !valve1Open) {
    digitalWrite(SOLENOID_VALVE_1_PIN, HIGH);
    valve1Open = true;
    Serial.println("[VALVE] Plant 1 valve OPEN");
  } else if (plantNum == 2 && !valve2Open) {
    digitalWrite(SOLENOID_VALVE_2_PIN, HIGH);
    valve2Open = true;
    Serial.println("[VALVE] Plant 2 valve OPEN");
  }
}

void closeValve(int plantNum) {
  if (plantNum == 1 && valve1Open) {
    digitalWrite(SOLENOID_VALVE_1_PIN, LOW);
    valve1Open = false;
    Serial.println("[VALVE] Plant 1 valve CLOSED");
  } else if (plantNum == 2 && valve2Open) {
    digitalWrite(SOLENOID_VALVE_2_PIN, LOW);
    valve2Open = false;
    Serial.println("[VALVE] Plant 2 valve CLOSED");
  }
}

void closeAllValves() {
  closeValve(1);
  closeValve(2);
}

void checkPumpSafety() {
  if (pumpRunning) {
    unsigned long runTime = millis() - pumpStartTime;

    if (runTime >= WATER_PUMP_MAX_DURATION_MS) {
      Serial.println("[PUMP] SAFETY TIMEOUT!");
      pumpOff();
      closeAllValves();
      currentlyWatering1 = false;
      currentlyWatering2 = false;

      if (wifiConnected) {
        sendNotification("ALERT: Pump safety timeout triggered!", 2);  // CRITICAL
      }
    }
  }
}

// =============================================================================
// PLANT CARE LOGIC
// =============================================================================

// Intelligent watering decision with safety thresholds
bool shouldSkipWateringForRain(int plantNum) {
  int moisture = (plantNum == 1) ? soilMoisture1 : soilMoisture2;

  // Safety check: Never skip if critically dry
  if (moisture < CRITICAL_DRY_THRESHOLD) {
    Serial.println("⚠️ CRITICAL DRY - Cannot skip watering regardless of forecast");
    if (wifiConnected) {
      String msg = "🚨 Plant " + String(plantNum) + " critically dry (" + 
                   String(moisture) + "%). Emergency watering initiated.";
      sendNotification(msg, 2);  // CRITICAL
    }
    return false;  // MUST water
  }

  // Check if we're in offline mode
  if (offlineMode) {
    Serial.println("⚠️ Offline mode - cannot check forecast, watering normally");
    return false;  // Water to be safe
  }

  // Check if forecast data is valid and recent (within last 2 hours)
  if (!currentWeather.isValid || 
      (millis() - currentWeather.forecastTime) > 7200000) {
    Serial.println("⚠️ Forecast data unavailable or stale - will water normally");
    return false;  // Water to be safe
  }

  // Decision based on moisture level and rain forecast

  // Case 1: Dry (20-30%) - can only skip if rain very soon and significant
  if (moisture < SAFE_SKIP_THRESHOLD) {
    if (currentWeather.rainAmount3h >= RAIN_THRESHOLD_3H_MM) {
      Serial.print("🌦️ Rain forecast (");
      Serial.print(currentWeather.rainAmount3h, 1);
      Serial.print("mm in 3h) - Safe to skip watering (moisture: ");
      Serial.print(moisture);
      Serial.println("%)");

      if (wifiConnected) {
        String msg = "💧 Plant " + String(plantNum) + ": Skipping watering. " +
                     String(currentWeather.rainAmount3h, 1) + 
                     "mm rain expected in 3 hours. (Moisture: " + String(moisture) + "%)";
        sendNotification(msg, 0);  // INFO
      }
      return true;  // Skip watering
    } else {
      Serial.println("⏰ Moisture low - rain too far or insufficient - watering now");
      return false;  // Water now
    }
  }

  // Case 2: Low-Optimal (30-40%) - can skip if rain within 6 hours
  else if (moisture < PREVENTIVE_SKIP_THRESHOLD) {
    if (currentWeather.rainAmount6h >= RAIN_THRESHOLD_6H_MM) {
      Serial.print("🌦️ Rain forecast (");
      Serial.print(currentWeather.rainAmount6h, 1);
      Serial.print("mm in 6h) - Skipping watering (moisture: ");
      Serial.print(moisture);
      Serial.println("%)");

      if (wifiConnected) {
        String msg = "🌿 Plant " + String(plantNum) + ": Water conservation mode. " +
                     String(currentWeather.rainAmount6h, 1) + 
                     "mm rain expected within 6 hours. (Moisture: " + String(moisture) + "%)";
        sendNotification(msg, 0);  // INFO
      }
      return true;  // Skip watering
    } else {
      Serial.println("📅 Preventive watering - no significant rain forecast");
      return false;  // Water now
    }
  }

  // Case 3: Already optimal or wet - no watering needed anyway
  else {
    Serial.println("💚 Moisture optimal - no watering needed");
    return false;  // Don't skip (but won't water anyway due to moisture level)
  }
}

void processPlant(int plantNum) {
  Serial.println("\n╔════════════════════════════════════════════════════════════╗");
  Serial.print("║                🌱 PLANT ");
  Serial.print(plantNum);
  Serial.println(" DECISION LOGIC                    ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝");

  // Check if currently watering this plant
  bool* currentlyWatering = (plantNum == 1) ? &currentlyWatering1 : &currentlyWatering2;
  int currentMoisture = (plantNum == 1) ? soilMoisture1 : soilMoisture2;

  if (*currentlyWatering) {
    // Check if target moisture reached or timeout
    Serial.print("💧 Currently watering Plant ");
    Serial.print(plantNum);
    Serial.print(" - Current: ");
    Serial.print(currentMoisture);
    Serial.print("%, Target: ");
    Serial.print(SOIL_MOISTURE_THRESHOLD_TARGET);
    Serial.println("%");

    if (currentMoisture >= SOIL_MOISTURE_THRESHOLD_TARGET) {
      Serial.print("✅ Plant ");
      Serial.print(plantNum);
      Serial.println(" reached target moisture!");
      stopWatering(plantNum);

      if (wifiConnected) {
        String msg = "Plant " + String(plantNum) + " watering complete. Moisture: " + String(currentMoisture) + "%";
        sendNotification(msg, 3);  // SUCCESS
      }
    } else {
      Serial.println("⏳ Waiting for target moisture to be reached...");
    }
    Serial.println("════════════════════════════════════════════════════════════\n");
    return;  // Don't make other decisions while watering
  }

  // Display context
  Serial.println("📋 DECISION CONTEXT:");
  Serial.print("   Moisture: ");
  Serial.print(currentMoisture);
  Serial.print("% ");
  if (isDry(plantNum)) Serial.print("[🟤 DRY]");
  else if (isWet(plantNum)) Serial.print("[💙 WET]");
  else Serial.print("[💚 OPTIMAL]");
  Serial.println();

  Serial.print("   Rain (local): ");
  Serial.print(isRaining() ? "YES 🌧️" : "NO ☀️");
  Serial.println();
  
  Serial.print("   Mode: ");
  Serial.println(offlineMode ? "OFFLINE ⚠️" : "ONLINE ✅");

  // Show forecast data if available
  if (currentWeather.isValid && !offlineMode) {
    Serial.print("   Rain Forecast: ");
    Serial.print(currentWeather.rainAmount3h, 1);
    Serial.print("mm (3h) | ");
    Serial.print(currentWeather.rainAmount6h, 1);
    Serial.println("mm (6h)");
    
    Serial.print("   Temp: ");
    Serial.print(currentWeather.temperature, 1);
    Serial.println("°C");
  }

  Serial.print("   Light: ");
  Serial.print(currentLux, 0);
  Serial.print(" lux ");
  Serial.println(isHighLight() ? "[HIGH ☀️]" : "[Normal 🌤️]");

  Serial.println("\n🧠 WATERING DECISION:");

  // WATERING LOGIC ONLY (shade will be decided separately for both plants)
  // Case A: Soil is DRY
  if (isDry(plantNum)) {
    Serial.println("🟤 SOIL IS DRY - Plant needs water!");

    if (isRaining()) {
      Serial.println("   ✓ Rain detected locally → Using natural watering");
    } 
    else if (shouldSkipWateringForRain(plantNum)) {
      Serial.println("   ✓ Rain forecasted → Skipping watering");
    }
    else {
      if (canWaterPlant(plantNum)) {
        Serial.println("   ✓ No rain detected or forecast → Manual watering required");
        Serial.println("   → Action: START WATERING");
        waterPlant(plantNum);
      } else {
        Serial.println("   ⏰ Watering cooldown active");
        unsigned long timeSince = (millis() - ((plantNum == 1) ? lastWateringTime1 : lastWateringTime2)) / 1000;
        Serial.print("   → Time since last watering: ");
        Serial.print(timeSince);
        Serial.println(" seconds");
      }
    }
  }
  else if (isWet(plantNum)) {
    Serial.println("💙 SOIL IS WET - Plant has enough water");
    Serial.println("   → No watering needed");
  }
  else {
    Serial.println("💚 SOIL IS OPTIMAL - Perfect moisture level");
    Serial.println("   → No watering needed");
  }

  Serial.println("════════════════════════════════════════════════════════════\n");
}

// =============================================================================
// SHADE PRIORITY SYSTEM
// =============================================================================

/**
 * Get shade preference for a plant (0 = no shade, 1 = wants shade)
 * Uses priority system to handle conflicts between two plants
 */
int getShadePreference(int plantNum) {
  int moisture = (plantNum == 1) ? soilMoisture1 : soilMoisture2;
  
  // DRY plant priorities
  if (isDry(plantNum)) {
    if (isRaining()) return 0;  // Wants NO shade (collect rain)
    if (currentlyWatering1 || currentlyWatering2) return 1;  // Wants shade (protect during watering)
    return 1;  // Default: shade for protection
  }
  
  // WET plant priorities
  if (isWet(plantNum)) {
    if (isRaining()) return 1;  // Wants shade (prevent overwatering)
    return 0;                   // Wants NO shade (allow drying & photosynthesis)
  }
  
  // OPTIMAL plant priorities
  if (isRaining()) return 1;    // Wants shade (stay optimal)
  if (isHighLight()) return 1;  // Wants shade (sun protection)
  return 0;                     // Wants NO shade (normal conditions)
}

/**
 * Centralized shade control based on both plants' needs
 * CRITICAL RULE: WET plant gets priority (root rot is fatal, missing rain is not)
 */
void updateShadeBasedOnBothPlants() {
  int plant1Pref = getShadePreference(1);
  int plant2Pref = getShadePreference(2);
  
  bool plant1IsWet = isWet(1);
  bool plant2IsWet = isWet(2);
  
  if (DEBUG_MODE) {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                    SHADE DECISION                          ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.print("Plant 1: ");
    Serial.print(plant1IsWet ? "WET 💙" : (isDry(1) ? "DRY 🟤" : "OPTIMAL 💚"));
    Serial.print(" | Wants shade: ");
    Serial.println(plant1Pref == 1 ? "YES" : "NO");
    
    Serial.print("Plant 2: ");
    Serial.print(plant2IsWet ? "WET 💙" : (isDry(2) ? "DRY 🟤" : "OPTIMAL 💚"));
    Serial.print(" | Wants shade: ");
    Serial.println(plant2Pref == 1 ? "YES" : "NO");
    Serial.println();
  }
  
  // CRITICAL PRIORITY: Protect WET plants from root rot (fatal!)
  // If ANY wet plant + raining, MUST deploy shade
  if ((plant1IsWet || plant2IsWet) && isRaining()) {
    if (DEBUG_MODE) {
      Serial.println("🚨 PRIORITY RULE: WET plant + rain detected!");
      Serial.println("→ Decision: DEPLOY SHADE (prevent root rot)");
    }
    deployShade();
    if (DEBUG_MODE) Serial.println("════════════════════════════════════════════════════════════\n");
    return;
  }
  
  // Standard decision: If ANY plant wants shade, deploy it
  if (plant1Pref == 1 || plant2Pref == 1) {
    if (DEBUG_MODE) Serial.println("→ Decision: DEPLOY SHADE");
    deployShade();
  } else {
    if (DEBUG_MODE) Serial.println("→ Decision: RETRACT SHADE");
    retractShade();
  }
  
  if (DEBUG_MODE) Serial.println("════════════════════════════════════════════════════════════\n");
}

void waterPlant(int plantNum) {
  Serial.print("===== WATERING PLANT ");
  Serial.print(plantNum);
  Serial.println(" =====");

  // Update last watering time
  if (plantNum == 1) {
    lastWateringTime1 = millis();
    currentlyWatering1 = true;
    wateringStartTime1 = millis();
  } else {
    lastWateringTime2 = millis();
    currentlyWatering2 = true;
    wateringStartTime2 = millis();
  }

  // Open valve and start pump
  openValve(plantNum);
  pumpOn();

  // Send notification
  if (wifiConnected) {
    int moisture = (plantNum == 1) ? soilMoisture1 : soilMoisture2;
    String msg = "Watering Plant " + String(plantNum) + " started. Current moisture: " + String(moisture) + "%";
    sendNotification(msg, 0);  // INFO
  }
}

void stopWatering(int plantNum) {
  Serial.print("===== STOPPING WATERING PLANT ");
  Serial.print(plantNum);
  Serial.println(" =====");

  closeValve(plantNum);

  // Turn off pump if no valves are open
  if (!valve1Open && !valve2Open) {
    pumpOff();
  }

  if (plantNum == 1) {
    currentlyWatering1 = false;
  } else {
    currentlyWatering2 = false;
  }
}

bool canWaterPlant(int plantNum) {
  unsigned long lastWatering = (plantNum == 1) ? lastWateringTime1 : lastWateringTime2;
  unsigned long timeSinceLastWatering = millis() - lastWatering;

  // Check if minimum time has passed since last watering
  if (lastWatering == 0) {
    return true;  // Never watered before
  }

  return timeSinceLastWatering >= MIN_TIME_BETWEEN_WATERING_MS;
}

void checkWateringCompletion() {
  // Check if watering has been going on too long (safety timeout)
  if (currentlyWatering1) {
    unsigned long wateringDuration = millis() - wateringStartTime1;
    if (wateringDuration >= WATER_PUMP_MAX_DURATION_MS) {
      Serial.println("Plant 1 watering timeout - stopping");
      stopWatering(1);

      if (wifiConnected) {
        sendNotification("Plant 1 watering timeout reached", 1);  // WARNING
      }
    }
  }

  if (currentlyWatering2) {
    unsigned long wateringDuration = millis() - wateringStartTime2;
    if (wateringDuration >= WATER_PUMP_MAX_DURATION_MS) {
      Serial.println("Plant 2 watering timeout - stopping");
      stopWatering(2);

      if (wifiConnected) {
        sendNotification("Plant 2 watering timeout reached", 1);  // WARNING
      }
    }
  }
}

// =============================================================================
// WEATHER API FUNCTIONS
// =============================================================================

bool updateWeather() {
  Serial.println("Fetching weather data...");

  // Simple retry logic - try 3 times
  for (int attempt = 1; attempt <= 3; attempt++) {
    if (attempt > 1) {
      Serial.print("Retry attempt ");
      Serial.print(attempt);
      Serial.println("/3");
      delay(2000);  // Wait 2 seconds between retries
    }

    WiFiSSLClient weatherClient;

    if (!weatherClient.connect(WEATHER_API_HOST, WEATHER_API_PORT)) {
      Serial.println("Weather API connection failed");
      continue;  // Try again
    }

    // Changed from /weather (current) to /forecast (predictive)
    // cnt=2 gets next 6 hours (2 x 3-hour blocks)
    String url = "/data/2.5/forecast?lat=" + String(WEATHER_LATITUDE) + "&lon=" + String(WEATHER_LONGITUDE) + "&appid=" + String(WEATHER_API_KEY) + "&units=metric&cnt=2";

    weatherClient.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + WEATHER_API_HOST + "\r\n" + "Connection: close\r\n\r\n");

    String response = "";
    response.reserve(WEATHER_RESPONSE_BUFFER_SIZE);  // Pre-allocate memory to prevent fragmentation
    unsigned long timeout = millis() + WEATHER_REQUEST_TIMEOUT_MS;
    unsigned long lastDataTime = millis();

    // Wait for response to start
    while (!weatherClient.available() && millis() < timeout) {
      delay(10);
    }

    if (!weatherClient.available()) {
      Serial.println("No response from server");
      weatherClient.stop();
      continue;
    }

    // Read response with better timeout handling
    while (millis() < timeout) {
      if (weatherClient.available()) {
        char c = weatherClient.read();
        response += c;
        lastDataTime = millis();  // Reset timeout on each character
      } else if (millis() - lastDataTime > 3000) {
        // No data for 3 seconds, assume transfer complete
        break;
      }
      delay(1);
    }
    
    weatherClient.stop();
    
    if (DEBUG_MODE) {
      Serial.print("Response length: ");
      Serial.print(response.length());
      Serial.println(" bytes");
    }

    if (response.length() > 0) {
      WeatherData newWeather = parseWeatherResponse(response);

      if (newWeather.isValid) {
        currentWeather = newWeather;  // Update only if valid
        Serial.print("✅ Weather: ");
        Serial.print(currentWeather.temperature);
        Serial.print("°C, ");
        Serial.println(currentWeather.description);
        return true;
      }
    }

    Serial.println("❌ Invalid weather response");
  }

  // All retries failed - but don't crash, just use old data
  if (currentWeather.isValid) {
    Serial.println("⚠️ Using cached weather data from previous update");
  } else {
    Serial.println("⚠️ No weather data available, system will operate on sensors only");
  }

  return false;
}

WeatherData parseWeatherResponse(const String& response) {
  WeatherData weather;
  weather.isValid = false;
  weather.rainAmount3h = 0;
  weather.rainAmount6h = 0;
  weather.forecastTime = millis();

  int jsonIndex = response.indexOf("\r\n\r\n");
  if (jsonIndex == -1) {
    Serial.println("Error: No HTTP headers found in response");
    if (DEBUG_MODE && response.length() > 0) {
      Serial.print("First 200 chars: ");
      Serial.println(response.substring(0, min(200, (int)response.length())));
    }
    return weather;
  }

  String json = response.substring(jsonIndex + 4);
  
  if (DEBUG_MODE) {
    Serial.print("JSON length: ");
    Serial.print(json.length());
    Serial.println(" bytes");
    if (json.length() < 100) {
      Serial.print("JSON content: ");
      Serial.println(json);
    }
  }

  StaticJsonDocument<WEATHER_JSON_BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("❌ JSON parsing failed: ");
    Serial.println(error.c_str());
    if (DEBUG_MODE) {
      Serial.print("JSON preview (first 300 chars): ");
      Serial.println(json.substring(0, min(300, (int)json.length())));
    }
    return weather;
  }

  // Parse forecast list array
  if (doc.containsKey("list")) {
    JsonArray forecastList = doc["list"];
    int numForecasts = min(2, (int)forecastList.size());  // Only need 2 blocks (6 hours)

    Serial.print("📊 Parsing ");
    Serial.print(numForecasts);
    Serial.println(" forecast blocks...");

    // Extract data from each 3-hour forecast block
    for (int i = 0; i < numForecasts; i++) {
      JsonObject forecast = forecastList[i];

      // Get temperature and humidity from first forecast
      if (i == 0) {
        weather.temperature = forecast["main"]["temp"];
        weather.humidity = forecast["main"]["humidity"];
        weather.description = forecast["weather"][0]["description"].as<String>();
      }

      // Check for rain in each time block
      String desc = forecast["weather"][0]["description"].as<String>();
      bool hasRain = (desc.indexOf("rain") >= 0 || 
                      desc.indexOf("drizzle") >= 0 || 
                      desc.indexOf("shower") >= 0);

      // Extract rain amount if present
      float rainAmount = 0;
      if (hasRain && forecast.containsKey("rain")) {
        if (forecast["rain"].containsKey("3h")) {
          rainAmount = forecast["rain"]["3h"];
        }
      }

      // Accumulate rain data by time window
      if (i == 0) {  // 0-3 hours
        weather.rainAmount3h = rainAmount;
        
        if (DEBUG_MODE) {
          Serial.print("   Block 1 (0-3h): ");
          Serial.print(rainAmount, 1);
          Serial.print("mm");
          if (hasRain) Serial.print(" [RAIN]");
          Serial.println();
        }
      }

      if (i == 1) {  // 3-6 hours
        weather.rainAmount6h = weather.rainAmount3h + rainAmount;
        
        if (DEBUG_MODE) {
          Serial.print("   Block 2 (3-6h): ");
          Serial.print(rainAmount, 1);
          Serial.print("mm");
          if (hasRain) Serial.print(" [RAIN]");
          Serial.print(" | Total 0-6h: ");
          Serial.print(weather.rainAmount6h, 1);
          Serial.println("mm");
        }
      }
    }

    weather.isValid = true;
    weather.forecastTime = millis();

    if (DEBUG_MODE) {
      Serial.println("✅ Forecast parsed successfully");
    }
  }

  return weather;
}

// =============================================================================
// NOTIFICATION FUNCTIONS
// =============================================================================

bool sendNotification(const String& message, int type) {
  if (!wifiConnected) return false;

  // Simple retry - 2 attempts
  for (int attempt = 1; attempt <= 2; attempt++) {
    if (attempt > 1) {
      delay(1000);  // Wait 1 second before retry
    }

    HttpClient http(sslClient, DISCORD_HOST, DISCORD_PORT);

    String emoji = "📢";
    if (type == 0) emoji = "ℹ️";       // INFO
    else if (type == 1) emoji = "⚠️";  // WARNING
    else if (type == 2) emoji = "🚨";       // CRITICAL
    else if (type == 3) emoji = "✅";       // SUCCESS

    String formattedMsg = emoji + " **Plant Care** - " + message;
    String content = "{\"content\":\"" + formattedMsg + "\"}";

    http.beginRequest();
    http.post(DISCORD_WEBHOOK_PATH);
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("Content-Length", content.length());
    http.beginBody();
    http.print(content);
    http.endRequest();

    int statusCode = http.responseStatusCode();
    http.stop();

    if (statusCode >= 200 && statusCode < 300) {
      if (DEBUG_MODE && attempt > 1) {
        Serial.print("📱 [NOTIFICATION] Sent on attempt ");
        Serial.println(attempt);
      }
      return true;
    }

    if (DEBUG_MODE) {
      Serial.print("📱 [NOTIFICATION] Failed (Status: ");
      Serial.print(statusCode);
      Serial.print("), attempt ");
      Serial.print(attempt);
      Serial.println("/2");
    }
  }

  return false;
}

void sendStatusUpdate() {
  String status = "Status Update:\\n";
  status.reserve(200);  // Pre-allocate for status message
  
  status += "Plant 1: " + String(soilMoisture1) + "% | ";
  status += "Plant 2: " + String(soilMoisture2) + "% | ";
  status += "Light: " + String((int)currentLux) + " lux | ";
  status += "Rain: " + String(rainDetected ? "Yes" : "No");

  if (currentWeather.isValid) {
    status += " | Temp: " + String(currentWeather.temperature, 1) + "°C";
  }

  sendNotification(status, 0);  // INFO
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void printSystemStatus() {
  Serial.println("=== SYSTEM STATUS ===");
  Serial.print("WiFi: ");
  Serial.println(wifiConnected ? "Connected" : "Disconnected");
  Serial.print("Lux Sensor: ");
  Serial.println(luxSensorAvailable ? "Available" : "Not available");
  Serial.print("Shade Position: ");
  Serial.println(currentShadePosition);
  Serial.println("=====================\n");
}
