#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Servo.h>
#include "config.h"

// =============================================================================
// FULL SYSTEM TEST - WITH WEATHER API & WEBHOOK INTEGRATION
// =============================================================================
// Test complete system intelligence including weather and notifications
// Hardware: Soil sensor, Lux sensor, Rain sensor, Servo motor

// =============================================================================
// SENSOR OBJECTS
// =============================================================================
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// Sensor readings
int soilMoisture1 = 0;
float currentLux = 0;
int rainValue = 1023;
bool rainDetected = false;
bool luxSensorAvailable = false;

// =============================================================================
// ACTUATOR OBJECTS
// =============================================================================
Servo shadeServo;
int currentShadePosition = SHADE_POSITION_OFF;

// =============================================================================
// NETWORK OBJECTS
// =============================================================================
WiFiSSLClient sslClient;

// Weather data structure
struct WeatherData {
  float temperature;
  int humidity;
  String description;
  bool isValid;
};

WeatherData currentWeather;

// =============================================================================
// TIMING VARIABLES
// =============================================================================
unsigned long lastSensorUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastStatusReport = 0;
unsigned long lastWateringTime1 = 0;

// =============================================================================
// SYSTEM STATE
// =============================================================================
bool systemInitialized = false;
bool wifiConnected = false;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
// Initialization
void initSystem();
bool initWiFi();
bool initSensors();
void initActuators();

// Sensor functions
void updateAllSensors();
int readSoilMoisture(int pin);
void updateLuxReading();
void updateRainReading();
bool isDry();
bool isWet();
bool isOptimal();
bool isHighLight();
bool isRaining();

// Actuator functions
void deployShade();
void retractShade();

// Plant care logic
void processPlant();
bool canWaterPlant();

// Weather API
bool updateWeather();
WeatherData parseWeatherResponse(const String& response);

// Notifications
bool sendNotification(const String& message, int type);
void sendStatusUpdate();

// Utilities
void printSystemStatus();
void printSensorReadings();

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000);
  
  Serial.println("\n\n===============================================");
  Serial.println("  FULL SYSTEM TEST - WEATHER API & WEBHOOKS");
  Serial.println("===============================================\n");
  
  initSystem();
  
  Serial.println("\n=== System Ready - Full Integration Test ===\n");
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
    printSensorReadings();
    lastSensorUpdate = currentTime;
    
    // Process plant care logic with weather integration
    processPlant();
  }
  
  // Update weather data periodically (every 5 minutes)
  if (currentTime - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL_MS) {
    if (wifiConnected) {
      Serial.println("\n🌍 === WEATHER UPDATE ===");
      updateWeather();
    }
    lastWeatherUpdate = currentTime;
  }
  
  // Send status update periodically (every 30 minutes)
  if (currentTime - lastStatusReport >= STATUS_UPDATE_INTERVAL_MS) {
    if (wifiConnected) {
      Serial.println("\n📱 === STATUS REPORT ===");
      sendStatusUpdate();
    }
    lastStatusReport = currentTime;
  }
  
  delay(100);
}

// =============================================================================
// INITIALIZATION FUNCTIONS
// =============================================================================

void initSystem() {
  Serial.println("Initializing complete system...");
  
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
    Serial.println("\n🌤️ Getting initial weather data...");
    updateWeather();
    
    // Send startup notification
    sendNotification("🚀 Plant Care System (Full Test) started successfully! Weather API and notifications active.", 3);
  } else {
    Serial.println("⚠️ WiFi not connected - Weather API and notifications disabled");
  }
  
  systemInitialized = true;
  Serial.println("✅ Complete system initialization finished!");
}

bool initWiFi() {
  Serial.print("🌐 Connecting to WiFi");
  
  unsigned long startTime = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED && 
         (millis() - startTime) < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" ✅ Connected!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println(" ❌ Failed!");
    Serial.println("System will operate without network features.");
    return false;
  }
}

bool initSensors() {
  Serial.println("\n🔧 Initializing sensors...");
  bool allSuccess = true;
  
  // Soil moisture sensor
  pinMode(SOIL_MOISTURE_PIN_1, INPUT);
  Serial.println("  ✅ Soil moisture sensor (Pin A0)");
  
  // Lux sensor (I2C)
  Serial.print("  🔍 Initializing lux sensor (I2C)...");
  int attempts = 0;
  while (!tsl.begin() && attempts < 5) {
    attempts++;
    delay(200);
  }
  
  if (attempts < 5) {
    tsl.enableAutoRange(true);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
    luxSensorAvailable = true;
    Serial.println(" ✅");
  } else {
    Serial.println(" ❌ Check I2C wiring (SDA=A4, SCL=A5)");
    allSuccess = false;
  }
  
  // Rain sensor
  pinMode(RAIN_SENSOR_PIN, INPUT);
  Serial.println("  ✅ Rain sensor (Pin A2)");
  
  // Take initial readings
  updateAllSensors();
  
  return allSuccess;
}

void initActuators() {
  Serial.println("\n⚙️ Initializing actuators...");
  
  // Servo motor
  shadeServo.attach(SERVO_PIN);
  shadeServo.write(SHADE_POSITION_OFF);
  currentShadePosition = SHADE_POSITION_OFF;
  delay(500);
  Serial.println("  ✅ Servo motor (Pin 9) - Shade mechanism");
  
  Serial.println("  ⏸️ Water pump (TEST MODE - Simulated)");
  Serial.println("  ⏸️ Solenoid valves (TEST MODE - Simulated)");
}

// =============================================================================
// SENSOR FUNCTIONS
// =============================================================================

void updateAllSensors() {
  soilMoisture1 = readSoilMoisture(SOIL_MOISTURE_PIN_1);
  updateLuxReading();
  updateRainReading();
}

int readSoilMoisture(int pin) {
  int rawValue = analogRead(pin);
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

bool isDry() {
  return soilMoisture1 < SOIL_MOISTURE_THRESHOLD_DRY;
}

bool isWet() {
  return soilMoisture1 > SOIL_MOISTURE_THRESHOLD_WET;
}

bool isOptimal() {
  return soilMoisture1 >= SOIL_MOISTURE_THRESHOLD_DRY && soilMoisture1 <= SOIL_MOISTURE_THRESHOLD_WET;
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
    Serial.println("🏠 [ACTION] Deploying shade...");
    
    for (int pos = currentShadePosition; pos <= SHADE_POSITION_ON; pos++) {
      shadeServo.write(pos);
      delay(SERVO_SPEED_DELAY_MS);
    }
    
    currentShadePosition = SHADE_POSITION_ON;
    Serial.println("🏠 [ACTION] Shade deployed (90°)");
  }
}

void retractShade() {
  if (currentShadePosition != SHADE_POSITION_OFF) {
    Serial.println("☀️ [ACTION] Retracting shade...");
    
    for (int pos = currentShadePosition; pos >= SHADE_POSITION_OFF; pos--) {
      shadeServo.write(pos);
      delay(SERVO_SPEED_DELAY_MS);
    }
    
    currentShadePosition = SHADE_POSITION_OFF;
    Serial.println("☀️ [ACTION] Shade retracted (0°)");
  }
}

// =============================================================================
// PLANT CARE LOGIC WITH WEATHER INTEGRATION
// =============================================================================

void processPlant() {
  Serial.println("\n🌱 === INTELLIGENT PLANT CARE DECISION ===");
  
  // Weather-enhanced decision variables
  bool isVeryHot = (currentWeather.isValid && currentWeather.temperature > TEMP_THRESHOLD_HIGH);
  bool rainForecast = (currentWeather.isValid && 
                       (currentWeather.description.indexOf("rain") >= 0 || 
                        currentWeather.description.indexOf("drizzle") >= 0 ||
                        currentWeather.description.indexOf("shower") >= 0));
  
  // Display weather context
  if (currentWeather.isValid) {
    Serial.print("🌡️ Weather Context: ");
    Serial.print(currentWeather.temperature, 1);
    Serial.print("°C, ");
    Serial.print(currentWeather.description);
    if (isVeryHot) Serial.print(" [VERY HOT!]");
    if (rainForecast) Serial.print(" [RAIN FORECAST]");
    Serial.println();
  }
  
  // Case A: Soil is DRY
  if (isDry()) {
    Serial.println("🟤 Plant soil is DRY");
    
    if (isRaining()) {
      Serial.println("🌧️  -> Rain detected, retracting shade to allow natural watering");
      retractShade();
      Serial.println("💧 [WATER DECISION] Skipping artificial watering (rain available)");
    } else if (rainForecast && canWaterPlant()) {
      Serial.println("🌦️  -> Rain forecast detected, skipping watering (rain expected)");
      retractShade();
      Serial.println("💧 [WATER DECISION] Waiting for natural rain");
      
      // Send smart notification
      if (wifiConnected) {
        sendNotification("🌦️ Smart watering: Rain forecast detected, skipping irrigation to save water!", 0);
      }
    } else {
      if (canWaterPlant()) {
        Serial.println("☀️  -> No rain/forecast, would initiate watering");
        deployShade();
        Serial.println("💧 [WATER DECISION] Opening valve + starting pump (TEST MODE: SIMULATED)");
        
        // Simulate watering notification
        if (wifiConnected) {
          String msg = "💧 Watering Plant 1 started. Current moisture: " + String(soilMoisture1) + "%";
          sendNotification(msg, 0);
        }
        
        // Update last watering time (simulate)
        lastWateringTime1 = millis();
      } else {
        Serial.println("⏰ -> Watering cooldown period active");
      }
    }
  }
  
  // Case B: Soil is WET
  else if (isWet()) {
    Serial.println("💙 Plant soil is WET");
    
    if (isRaining()) {
      Serial.println("🌧️  -> Rain detected, deploying shade to prevent overwatering");
      deployShade();
    } else if (isHighLight()) {
      Serial.println("☀️  -> High light detected, retracting shade for photosynthesis");
      retractShade();
    } else {
      Serial.println("🌤️  -> Normal conditions, retracting shade");
      retractShade();
    }
  }
  
  // Case C: Soil is OPTIMAL
  else {
    Serial.println("💚 Plant soil is OPTIMAL");
    
    if (isRaining()) {
      Serial.println("🌧️  -> Rain detected, deploying shade to maintain optimal moisture");
      deployShade();
    } else if (isHighLight() || isVeryHot) {
      if (isVeryHot) {
        Serial.print("🔥 -> Very hot weather (");
        Serial.print(currentWeather.temperature, 1);
        Serial.println("°C), deploying shade for protection");
        
        // Send hot weather alert
        if (wifiConnected) {
          String msg = "🔥 Very hot weather detected: " + String(currentWeather.temperature, 1) + 
                       "°C. Shade protection activated for plant safety.";
          sendNotification(msg, 1);
        }
      } else {
        Serial.println("☀️  -> High light detected, deploying shade for protection");
      }
      deployShade();
    } else {
      Serial.println("🌤️  -> Normal conditions, retracting shade");
      retractShade();
    }
  }
  
  Serial.println("=== END DECISION ===\n");
}

bool canWaterPlant() {
  if (lastWateringTime1 == 0) return true;
  return (millis() - lastWateringTime1) >= MIN_TIME_BETWEEN_WATERING_MS;
}

// =============================================================================
// WEATHER API FUNCTIONS
// =============================================================================

bool updateWeather() {
  Serial.println("🌍 Fetching weather data from OpenWeatherMap...");
  
  WiFiSSLClient weatherClient;
  
  if (!weatherClient.connect(WEATHER_API_HOST, WEATHER_API_PORT)) {
    Serial.println("❌ Weather API connection failed");
    return false;
  }
  
  String url = "/data/2.5/weather?lat=" + String(WEATHER_LATITUDE) + 
               "&lon=" + String(WEATHER_LONGITUDE) + 
               "&appid=" + String(WEATHER_API_KEY) + 
               "&units=metric";
  
  weatherClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                      "Host: " + WEATHER_API_HOST + "\r\n" +
                      "Connection: close\r\n\r\n");
  
  String response = "";
  unsigned long timeout = millis() + WEATHER_REQUEST_TIMEOUT_MS;
  
  while (weatherClient.connected() && millis() < timeout) {
    if (weatherClient.available()) {
      response += (char)weatherClient.read();
    }
  }
  weatherClient.stop();
  
  if (response.length() > 0) {
    currentWeather = parseWeatherResponse(response);
    
    if (currentWeather.isValid) {
      Serial.print("🌡️ Weather Update: ");
      Serial.print(currentWeather.temperature, 1);
      Serial.print("°C, ");
      Serial.print(currentWeather.humidity);
      Serial.print("% humidity, ");
      Serial.println(currentWeather.description);
      
      // Send weather-based notifications
      if (wifiConnected) {
        // Very hot weather alert
        if (currentWeather.temperature > TEMP_THRESHOLD_HIGH) {
          String msg = "🔥 Very hot weather alert: " + String(currentWeather.temperature, 1) + 
                       "°C. Enhanced plant protection activated.";
          sendNotification(msg, 1);  // WARNING
        }
        
        // Rain forecast notification
        if (currentWeather.description.indexOf("rain") >= 0 || 
            currentWeather.description.indexOf("drizzle") >= 0 ||
            currentWeather.description.indexOf("shower") >= 0) {
          String msg = "🌦️ Rain forecast: " + currentWeather.description + 
                       ". System will utilize natural watering.";
          sendNotification(msg, 0);  // INFO
        }
      }
      
      return true;
    }
  }
  
  Serial.println("❌ Failed to get valid weather data");
  return false;
}

WeatherData parseWeatherResponse(const String& response) {
  WeatherData weather;
  weather.isValid = false;
  
  int jsonIndex = response.indexOf("\r\n\r\n");
  if (jsonIndex == -1) return weather;
  
  String json = response.substring(jsonIndex + 4);
  
  StaticJsonDocument<WEATHER_JSON_BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return weather;
  }
  
  if (doc.containsKey("main") && doc.containsKey("weather")) {
    weather.temperature = doc["main"]["temp"];
    weather.humidity = doc["main"]["humidity"];
    weather.description = doc["weather"][0]["description"].as<String>();
    weather.isValid = true;
  }
  
  return weather;
}

// =============================================================================
// WEBHOOK NOTIFICATION FUNCTIONS
// =============================================================================

bool sendNotification(const String& message, int type) {
  if (!wifiConnected) {
    Serial.println("📱 [NOTIFICATION] WiFi not connected, skipping notification");
    return false;
  }
  
  Serial.print("📱 [NOTIFICATION] Sending: ");
  Serial.println(message);
  
  HttpClient http(sslClient, DISCORD_HOST, DISCORD_PORT);
  
  String emoji = "📢";
  if (type == 0) emoji = "ℹ️";        // INFO
  else if (type == 1) emoji = "⚠️";  // WARNING
  else if (type == 2) emoji = "🚨";  // CRITICAL
  else if (type == 3) emoji = "✅";  // SUCCESS
  
  String formattedMsg = emoji + " **Plant Care System** - " + message;
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
  
  bool success = (statusCode >= 200 && statusCode < 300);
  Serial.print("📱 [NOTIFICATION] ");
  Serial.println(success ? "✅ Sent successfully!" : "❌ Failed to send");
  
  return success;
}

void sendStatusUpdate() {
  String status = "📊 System Status Report:\\n";
  status += "🌱 Soil Moisture: " + String(soilMoisture1) + "% | ";
  status += "💡 Light: " + String((int)currentLux) + " lux | ";
  status += "🌧️ Rain: " + String(rainDetected ? "Yes" : "No") + " | ";
  status += "🏠 Shade: " + String(currentShadePosition) + "°";
  
  if (currentWeather.isValid) {
    status += "\\n🌡️ Weather: " + String(currentWeather.temperature, 1) + "°C, " + currentWeather.description;
  }
  
  sendNotification(status, 0);  // INFO
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void printSystemStatus() {
  Serial.println("=== FULL SYSTEM STATUS ===");
  Serial.print("🌐 WiFi: ");
  Serial.println(wifiConnected ? "✅ Connected" : "❌ Disconnected");
  Serial.print("💡 Lux Sensor: ");
  Serial.println(luxSensorAvailable ? "✅ Available" : "❌ Not available");
  Serial.print("🏠 Shade Position: ");
  Serial.print(currentShadePosition);
  Serial.println("°");
  Serial.print("🌤️ Weather API: ");
  Serial.println(wifiConnected ? "✅ Active" : "❌ Disabled");
  Serial.print("📱 Notifications: ");
  Serial.println(wifiConnected ? "✅ Active" : "❌ Disabled");
  Serial.println("💧 Water System: 🧪 TEST MODE (Simulated)");
  Serial.println("===============================\n");
}

void printSensorReadings() {
  Serial.println("📊 --- SENSOR READINGS ---");
  Serial.print("🌱 Soil Moisture: ");
  Serial.print(soilMoisture1);
  Serial.print("% ");
  if (isDry()) Serial.print("[DRY 🟤]");
  else if (isWet()) Serial.print("[WET 💙]");
  else Serial.print("[OPTIMAL 💚]");
  Serial.println();
  
  Serial.print("💡 Light Level: ");
  Serial.print(currentLux, 0);
  Serial.print(" lux ");
  if (isHighLight()) Serial.print("[HIGH ☀️]");
  else Serial.print("[Normal 🌤️]");
  Serial.println();
  
  Serial.print("🌧️ Rain Sensor: ");
  Serial.print(rainValue);
  Serial.print(" ");
  if (isRaining()) Serial.print("[RAIN DETECTED 🌧️]");
  else Serial.print("[No rain ☀️]");
  Serial.println();
  
  Serial.print("🏠 Shade Position: ");
  Serial.print(currentShadePosition);
  Serial.println("°");
  
  if (currentWeather.isValid) {
    Serial.print("🌡️ Weather: ");
    Serial.print(currentWeather.temperature, 1);
    Serial.print("°C, ");
    Serial.print(currentWeather.humidity);
    Serial.print("%, ");
    Serial.println(currentWeather.description);
  }
  
  Serial.println("---------------------------");
}
