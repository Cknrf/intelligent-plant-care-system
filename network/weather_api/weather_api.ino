#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <ArduinoJson.h>
#include "../../config.h"

// =============================================================================
// WEATHER API MODULE
// =============================================================================

WiFiSSLClient client;
unsigned long lastWeatherUpdate = 0;

// Weather data structure
struct WeatherData {
  float temperature;
  int humidity;
  String description;
  String city;
  bool isValid;
};

WeatherData currentWeather;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
bool initializeWeatherAPI();
bool fetchWeatherData();
WeatherData parseWeatherResponse(const String& jsonResponse);
String buildWeatherAPIUrl();
bool connectToWeatherAPI();
void printWeatherData(const WeatherData& weather);

// =============================================================================
// SETUP FUNCTION
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 5000); // Wait max 5 seconds for Serial
  
  Serial.println("=== Weather API Module ===");
  
  if (!initializeWeatherAPI()) {
    Serial.println("Failed to initialize Weather API!");
    while (true) {
      delay(1000);
    }
  }
  
  Serial.println("Weather API initialized successfully!");
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void loop() {
  unsigned long currentTime = millis();
  
  // Check if it's time to update weather data
  if (currentTime - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL_MS) {
    Serial.println("Fetching weather data...");
    
    if (fetchWeatherData()) {
      printWeatherData(currentWeather);
      lastWeatherUpdate = currentTime;
    } else {
      Serial.println("Failed to fetch weather data. Retrying in 30 seconds...");
      delay(30000); // Wait 30 seconds before retry
    }
  }
  
  delay(1000); // Small delay to prevent excessive CPU usage
}

// =============================================================================
// WEATHER API FUNCTIONS
// =============================================================================

/**
 * Initialize WiFi connection for Weather API
 */
bool initializeWeatherAPI() {
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
  
  Serial.println("\nWiFi connected successfully!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
}

/**
 * Fetch weather data from OpenWeatherMap API
 */
bool fetchWeatherData() {
  if (!connectToWeatherAPI()) {
    return false;
  }
  
  // Send HTTP request
  String url = buildWeatherAPIUrl();
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + WEATHER_API_HOST + "\r\n" +
               "Connection: close\r\n\r\n");
  
  // Read response with timeout
  String response = "";
  unsigned long timeout = millis() + WEATHER_REQUEST_TIMEOUT_MS;
  
  while (client.connected() && millis() < timeout) {
    if (client.available()) {
      char c = client.read();
      response += c;
    }
  }
  client.stop();
  
  if (response.length() == 0) {
    Serial.println("Empty response from weather API");
    return false;
  }
  
  // Parse JSON response
  currentWeather = parseWeatherResponse(response);
  return currentWeather.isValid;
}

/**
 * Connect to Weather API server
 */
bool connectToWeatherAPI() {
  Serial.print("Connecting to ");
  Serial.println(WEATHER_API_HOST);
  
  if (!client.connect(WEATHER_API_HOST, WEATHER_API_PORT)) {
    Serial.println("Connection to weather API failed!");
    return false;
  }
  
  Serial.println("Connected to weather API server!");
  return true;
}

/**
 * Build the Weather API URL with parameters
 */
String buildWeatherAPIUrl() {
  return "/data/2.5/weather?lat=" + String(WEATHER_LATITUDE) + 
         "&lon=" + String(WEATHER_LONGITUDE) + 
         "&appid=" + String(WEATHER_API_KEY) + 
         "&units=metric";
}

/**
 * Parse JSON response from Weather API
 */
WeatherData parseWeatherResponse(const String& response) {
  WeatherData weather;
  weather.isValid = false;
  
  // Find JSON start
  int jsonIndex = response.indexOf("\r\n\r\n");
  if (jsonIndex == -1) {
    Serial.println("Invalid HTTP response format");
    return weather;
  }
  
  String json = response.substring(jsonIndex + 4);
  
  if (DEBUG_MODE) {
    Serial.println("Raw JSON:");
    Serial.println(json);
  }
  
  // Parse JSON
  StaticJsonDocument<WEATHER_JSON_BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.f_str());
    return weather;
  }
  
  // Extract weather data
  if (doc.containsKey("main") && doc.containsKey("weather") && doc.containsKey("name")) {
    weather.temperature = doc["main"]["temp"];
    weather.humidity = doc["main"]["humidity"];
    weather.description = doc["weather"][0]["description"].as<String>();
    weather.city = doc["name"].as<String>();
    weather.isValid = true;
  } else {
    Serial.println("Missing required fields in weather response");
  }
  
  return weather;
}

/**
 * Print weather data to Serial
 */
void printWeatherData(const WeatherData& weather) {
  if (!weather.isValid) {
    Serial.println("No valid weather data to display");
    return;
  }
  
  Serial.println("====== WEATHER INFO ======");
  Serial.print("City: ");
  Serial.println(weather.city);
  Serial.print("Temperature: ");
  Serial.print(weather.temperature);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(weather.humidity);
  Serial.println(" %");
  Serial.print("Description: ");
  Serial.println(weather.description);
  Serial.println("===========================");
}

// =============================================================================
// PUBLIC API FUNCTIONS (for use by main system)
// =============================================================================

/**
 * Get current weather data
 */
WeatherData getCurrentWeather() {
  return currentWeather;
}

/**
 * Check if weather data is fresh (updated recently)
 */
bool isWeatherDataFresh() {
  return (millis() - lastWeatherUpdate) < WEATHER_UPDATE_INTERVAL_MS;
}

/**
 * Force weather data update
 */
bool updateWeatherNow() {
  return fetchWeatherData();
}