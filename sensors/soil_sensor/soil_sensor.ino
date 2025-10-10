#include "../../config.h"

// =============================================================================
// SOIL MOISTURE SENSOR MODULE
// =============================================================================
// Handles reading from 2 soil moisture sensors (one per plant)

// Global variables to store latest readings
int soilMoisture1 = 0;  // Plant 1 moisture percentage
int soilMoisture2 = 0;  // Plant 2 moisture percentage
unsigned long lastSoilReadTime = 0;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
void initSoilSensors();
void updateSoilReadings();
int readSoilMoisture(int pin);
int getSoilMoisture(int plantNum);
bool isDry(int plantNum);
bool isWet(int plantNum);
bool isOptimal(int plantNum);

// =============================================================================
// SETUP FUNCTION (for standalone testing)
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 5000);
  
  Serial.println("=== Soil Moisture Sensor Module ===");
  initSoilSensors();
  Serial.println("Soil sensors initialized!");
}

// =============================================================================
// MAIN LOOP (for standalone testing)
// =============================================================================
void loop() {
  updateSoilReadings();
  
  Serial.println("--- Soil Moisture Readings ---");
  Serial.print("Plant 1: ");
  Serial.print(soilMoisture1);
  Serial.print("% ");
  if (isDry(1)) Serial.print("[DRY]");
  else if (isWet(1)) Serial.print("[WET]");
  else Serial.print("[OPTIMAL]");
  Serial.println();
  
  Serial.print("Plant 2: ");
  Serial.print(soilMoisture2);
  Serial.print("% ");
  if (isDry(2)) Serial.print("[DRY]");
  else if (isWet(2)) Serial.print("[WET]");
  else Serial.print("[OPTIMAL]");
  Serial.println();
  Serial.println();
  
  delay(2000);
}

// =============================================================================
// SOIL SENSOR FUNCTIONS
// =============================================================================

/**
 * Initialize soil moisture sensors
 */
void initSoilSensors() {
  pinMode(SOIL_MOISTURE_PIN_1, INPUT);
  pinMode(SOIL_MOISTURE_PIN_2, INPUT);
  
  // Take initial readings
  updateSoilReadings();
}

/**
 * Update soil moisture readings for both plants
 * Call this periodically from main loop
 */
void updateSoilReadings() {
  unsigned long currentTime = millis();
  
  // Check if enough time has passed since last reading
  if (currentTime - lastSoilReadTime >= SOIL_MOISTURE_READ_INTERVAL_MS || lastSoilReadTime == 0) {
    soilMoisture1 = readSoilMoisture(SOIL_MOISTURE_PIN_1);
    soilMoisture2 = readSoilMoisture(SOIL_MOISTURE_PIN_2);
    lastSoilReadTime = currentTime;
    
    if (DEBUG_MODE) {
      Serial.print("[SOIL] Plant 1: ");
      Serial.print(soilMoisture1);
      Serial.print("%, Plant 2: ");
      Serial.print(soilMoisture2);
      Serial.println("%");
    }
  }
}

/**
 * Read soil moisture from a specific pin and convert to percentage
 * Returns: 0-100% (0 = completely dry, 100 = saturated)
 */
int readSoilMoisture(int pin) {
  int rawValue = analogRead(pin);
  
  // Convert to percentage (higher raw value = drier soil for most sensors)
  // Adjust SOIL_SENSOR_DRY_VALUE and SOIL_SENSOR_WET_VALUE in config.h
  int percentage = map(rawValue, SOIL_SENSOR_DRY_VALUE, SOIL_SENSOR_WET_VALUE, 0, 100);
  
  // Constrain to valid range
  percentage = constrain(percentage, 0, 100);
  
  return percentage;
}

/**
 * Get current soil moisture reading for a specific plant
 * @param plantNum: 1 or 2
 * @return: Moisture percentage (0-100)
 */
int getSoilMoisture(int plantNum) {
  if (plantNum == 1) {
    return soilMoisture1;
  } else if (plantNum == 2) {
    return soilMoisture2;
  }
  return 0;
}

/**
 * Check if a plant's soil is dry (needs watering)
 * @param plantNum: 1 or 2
 */
bool isDry(int plantNum) {
  int moisture = getSoilMoisture(plantNum);
  return moisture < SOIL_MOISTURE_THRESHOLD_DRY;
}

/**
 * Check if a plant's soil is wet (too much water)
 * @param plantNum: 1 or 2
 */
bool isWet(int plantNum) {
  int moisture = getSoilMoisture(plantNum);
  return moisture > SOIL_MOISTURE_THRESHOLD_WET;
}

/**
 * Check if a plant's soil moisture is in optimal range
 * @param plantNum: 1 or 2
 */
bool isOptimal(int plantNum) {
  int moisture = getSoilMoisture(plantNum);
  return moisture >= SOIL_MOISTURE_THRESHOLD_DRY && moisture <= SOIL_MOISTURE_THRESHOLD_WET;
}

/**
 * Check if soil moisture is critically low (emergency)
 * @param plantNum: 1 or 2
 */
bool isCriticallyDry(int plantNum) {
  int moisture = getSoilMoisture(plantNum);
  return moisture < CRITICAL_MOISTURE_LEVEL;
}
