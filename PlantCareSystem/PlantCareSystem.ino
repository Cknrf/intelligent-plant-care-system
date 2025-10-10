#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Servo.h>
#include "../config.h"

// =============================================================================
// INTELLIGENT ADAPTIVE PLANT CARE SYSTEM
// =============================================================================
// Main system integrating all sensors, actuators, weather API, and notifications
// for automated care of 2 plants

// =============================================================================
// SENSOR OBJECTS
// =============================================================================
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

// =============================================================================
// SYSTEM STATE
// =============================================================================
bool systemInitialized = false;
bool wifiConnected = false;
bool currentlyWatering1 = false;
bool currentlyWatering2 = false;

// Weather data structure
struct WeatherData {
  float temperature;
  int humidity;
  String description;
  bool isValid;
};

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
void waterPlant(int plantNum);
void stopWatering(int plantNum);
bool canWaterPlant(int plantNum);

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
  }
  
  // Update weather data periodically (every 5 minutes)
  if (currentTime - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL_MS) {
    if (wifiConnected) {
      updateWeather();
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
  
  // Small delay to prevent excessive CPU usage
  delay(100);
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
    updateWeather();
    sendNotification("Plant Care System started successfully!", 3);  // SUCCESS
  }
  
  systemInitialized = true;
  Serial.println("System initialization complete!");
}

bool initWiFi() {
  Serial.print("Connecting to WiFi");
  
  unsigned long startTime = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED && 
         (millis() - startTime) < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println(" Failed!");
    Serial.println("System will operate without WiFi features.");
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
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
    luxSensorAvailable = true;
    Serial.println(" [OK]");
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
    Serial.println("\n--- Sensor Readings ---");
    Serial.print("Plant 1 Moisture: ");
    Serial.print(soilMoisture1);
    Serial.println("%");
    Serial.print("Plant 2 Moisture: ");
    Serial.print(soilMoisture2);
    Serial.println("%");
    Serial.print("Light Level: ");
    Serial.print(currentLux);
    Serial.println(" lux");
    Serial.print("Rain: ");
    Serial.println(rainDetected ? "YES" : "NO");
    Serial.println("-----------------------\n");
  }
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
    
    // Smooth movement
    for (int pos = currentShadePosition; pos <= SHADE_POSITION_ON; pos++) {
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
    
    // Smooth movement
    for (int pos = currentShadePosition; pos >= SHADE_POSITION_OFF; pos--) {
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

void processPlant(int plantNum) {
  // Check if currently watering this plant
  bool* currentlyWatering = (plantNum == 1) ? &currentlyWatering1 : &currentlyWatering2;
  
  if (*currentlyWatering) {
    // Check if target moisture reached or timeout
    int currentMoisture = (plantNum == 1) ? soilMoisture1 : soilMoisture2;
    
    if (currentMoisture >= SOIL_MOISTURE_THRESHOLD_TARGET) {
      Serial.print("Plant ");
      Serial.print(plantNum);
      Serial.println(" reached target moisture!");
      stopWatering(plantNum);
      
      if (wifiConnected) {
        String msg = "Plant " + String(plantNum) + " watering complete. Moisture: " + 
                     String(currentMoisture) + "%";
        sendNotification(msg, 3);  // SUCCESS
      }
    }
    return;  // Don't make other decisions while watering
  }
  
  // DECISION TREE
  
  // Case A: Soil is DRY
  if (isDry(plantNum)) {
    Serial.print("Plant ");
    Serial.print(plantNum);
    Serial.println(" is DRY");
    
    if (isRaining()) {
      // Let rain water the plant naturally
      Serial.println("  -> Rain detected, retracting shade to allow natural watering");
      retractShade();
    } else {
      // No rain, need to water manually
      if (canWaterPlant(plantNum)) {
        Serial.println("  -> No rain, initiating watering");
        deployShade();  // Shade protects during watering
        waterPlant(plantNum);
      } else {
        Serial.println("  -> Watering cooldown period active");
      }
    }
  }
  
  // Case B: Soil is WET
  else if (isWet(plantNum)) {
    if (isRaining()) {
      // Protect from more rain to prevent overwatering
      Serial.print("Plant ");
      Serial.print(plantNum);
      Serial.println(" is WET and raining - deploying shade");
      deployShade();
    } else if (isHighLight()) {
      // Allow photosynthesis, no shade needed
      Serial.print("Plant ");
      Serial.print(plantNum);
      Serial.println(" is WET with high light - retracting shade for photosynthesis");
      retractShade();
    } else {
      // Normal conditions, no shade needed
      retractShade();
    }
  }
  
  // Case C: Soil is OPTIMAL
  else {
    if (isRaining()) {
      // Prevent going from optimal to wet
      deployShade();
    } else if (isHighLight()) {
      // Protect from intense sun
      deployShade();
    } else {
      // Normal conditions
      retractShade();
    }
  }
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
    String msg = "Watering Plant " + String(plantNum) + " started. Current moisture: " + 
                 String(moisture) + "%";
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

// =============================================================================
// WEATHER API FUNCTIONS
// =============================================================================

bool updateWeather() {
  Serial.println("Fetching weather data...");
  
  WiFiSSLClient weatherClient;
  
  if (!weatherClient.connect(WEATHER_API_HOST, WEATHER_API_PORT)) {
    Serial.println("Weather API connection failed");
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
      Serial.print("Weather: ");
      Serial.print(currentWeather.temperature);
      Serial.print("°C, ");
      Serial.println(currentWeather.description);
      return true;
    }
  }
  
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
// NOTIFICATION FUNCTIONS
// =============================================================================

bool sendNotification(const String& message, int type) {
  if (!wifiConnected) return false;
  
  HttpClient http(sslClient, DISCORD_HOST, DISCORD_PORT);
  
  String emoji = "📢";
  if (type == 0) emoji = "ℹ️";        // INFO
  else if (type == 1) emoji = "⚠️";  // WARNING
  else if (type == 2) emoji = "🚨";  // CRITICAL
  else if (type == 3) emoji = "✅";  // SUCCESS
  
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
  
  return (statusCode >= 200 && statusCode < 300);
}

void sendStatusUpdate() {
  String status = "Status Update:\\n";
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
