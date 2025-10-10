#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include "../../config.h"

// =============================================================================
// LUX SENSOR MODULE
// =============================================================================
// Handles reading from TSL2561 light sensor via I2C

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// Global variables
float currentLux = 0;
bool luxSensorAvailable = false;
unsigned long lastLuxReadTime = 0;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
bool initLuxSensor();
void updateLuxReading();
float getLux();
bool isHighLight();

// =============================================================================
// SETUP FUNCTION (for standalone testing)
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 5000);
  
  Serial.println("=== Lux Sensor Module ===");
  
  if (!initLuxSensor()) {
    Serial.println("ERROR: Lux sensor not available!");
  } else {
    Serial.println("Lux sensor initialized successfully!");
  }
}

// =============================================================================
// MAIN LOOP (for standalone testing)
// =============================================================================
void loop() {
  updateLuxReading();
  
  Serial.print("Light Level: ");
  Serial.print(currentLux);
  Serial.print(" lux ");
  
  if (isHighLight()) {
    Serial.println("[HIGH LIGHT - Shade recommended]");
  } else {
    Serial.println("[Normal light]");
  }
  
  delay(2000);
}

// =============================================================================
// LUX SENSOR FUNCTIONS
// =============================================================================

/**
 * Initialize the TSL2561 lux sensor
 * Returns: true if successful, false if sensor not found
 */
bool initLuxSensor() {
  Serial.println("Initializing TSL2561 sensor...");
  
  // Retry initialization up to 5 times
  int attempts = 0;
  while (!tsl.begin() && attempts < 5) {
    Serial.print("TSL2561 not detected, retrying... (");
    Serial.print(attempts + 1);
    Serial.println("/5)");
    attempts++;
    delay(200);
  }

  if (attempts == 5) {
    Serial.println("TSL2561 failed to initialize. Check wiring!");
    luxSensorAvailable = false;
    return false;
  }

  // Configure sensor for auto-gain and integration time
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
  
  luxSensorAvailable = true;
  
  // Take initial reading
  updateLuxReading();
  
  return true;
}

/**
 * Update lux reading from sensor
 * Call this periodically from main loop
 */
void updateLuxReading() {
  unsigned long currentTime = millis();
  
  // Check if sensor is available
  if (!luxSensorAvailable) {
    currentLux = 0;
    return;
  }
  
  // Check if enough time has passed since last reading
  if (currentTime - lastLuxReadTime >= LUX_READ_INTERVAL_MS || lastLuxReadTime == 0) {
    sensors_event_t event;
    tsl.getEvent(&event);
    
    if (event.light > 0) {
      currentLux = event.light;
      lastLuxReadTime = currentTime;
      
      if (DEBUG_MODE) {
        Serial.print("[LUX] ");
        Serial.print(currentLux);
        Serial.println(" lux");
      }
    } else {
      // Sensor overload or too dark
      if (DEBUG_MODE) {
        Serial.println("[LUX] Sensor overload or too dark");
      }
      // Keep previous reading if sensor is overloaded
    }
  }
}

/**
 * Get current lux reading
 * Returns: Current light level in lux
 */
float getLux() {
  return currentLux;
}

/**
 * Check if light level is high (shade recommended)
 * Returns: true if lux exceeds threshold
 */
bool isHighLight() {
  return currentLux > LUX_THRESHOLD_HIGH;
}

/**
 * Check if lux sensor is available and working
 * Returns: true if sensor initialized successfully
 */
bool isLuxSensorAvailable() {
  return luxSensorAvailable;
}
