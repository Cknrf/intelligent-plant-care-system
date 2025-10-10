#include "../../config.h"

// =============================================================================
// RAIN SENSOR MODULE
// =============================================================================
// Handles reading from analog rain sensor

// Global variables
int rainValue = 1023;  // Raw analog value (lower = more rain)
bool rainDetected = false;
unsigned long lastRainReadTime = 0;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
void initRainSensor();
void updateRainReading();
bool isRaining();
int getRainValue();

// =============================================================================
// SETUP FUNCTION (for standalone testing)
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 5000);
  
  Serial.println("=== Rain Sensor Module ===");
  initRainSensor();
  Serial.println("Rain sensor initialized!");
}

// =============================================================================
// MAIN LOOP (for standalone testing)
// =============================================================================
void loop() {
  updateRainReading();
  
  Serial.print("Rain Sensor Value: ");
  Serial.print(rainValue);
  Serial.print(" - ");
  
  if (isRaining()) {
    Serial.println("[RAIN DETECTED]");
  } else {
    Serial.println("[No rain]");
  }
  
  delay(1000);
}

// =============================================================================
// RAIN SENSOR FUNCTIONS
// =============================================================================

/**
 * Initialize rain sensor
 */
void initRainSensor() {
  pinMode(RAIN_SENSOR_PIN, INPUT);
  
  // Take initial reading
  updateRainReading();
}

/**
 * Update rain sensor reading
 * Call this periodically from main loop
 */
void updateRainReading() {
  unsigned long currentTime = millis();
  
  // Apply debouncing
  if (currentTime - lastRainReadTime >= RAIN_DEBOUNCE_MS || lastRainReadTime == 0) {
    rainValue = analogRead(RAIN_SENSOR_PIN);
    
    // Check if rain is detected (lower value = more water)
    rainDetected = (rainValue < RAIN_THRESHOLD);
    
    lastRainReadTime = currentTime;
    
    if (DEBUG_MODE) {
      Serial.print("[RAIN] Value: ");
      Serial.print(rainValue);
      Serial.print(" - ");
      Serial.println(rainDetected ? "RAIN" : "No rain");
    }
  }
}

/**
 * Check if rain is currently detected
 * Returns: true if raining, false otherwise
 */
bool isRaining() {
  return rainDetected;
}

/**
 * Get raw rain sensor value
 * Returns: Analog value (0-1023, lower = more rain)
 */
int getRainValue() {
  return rainValue;
}

/**
 * Get rain intensity level
 * Returns: 0 = no rain, 1 = light rain, 2 = heavy rain
 */
int getRainIntensity() {
  if (rainValue > RAIN_THRESHOLD) {
    return 0;  // No rain
  } else if (rainValue > 200) {
    return 1;  // Light rain
  } else {
    return 2;  // Heavy rain
  }
}
