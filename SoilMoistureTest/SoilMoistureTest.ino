// =============================================================================
// SOIL MOISTURE SENSOR CALIBRATION TEST
// =============================================================================
// Use this to find the right thresholds for your soil moisture sensor
// Connect sensor A0 pin to Arduino A0

// Pin configuration
#define SOIL_SENSOR_PIN A0

// Current threshold values (adjust these based on your readings)
int dryThreshold = 30;    // Below this = DRY
int wetThreshold = 70;    // Above this = WET

// Calibration values (adjust these based on your sensor)
int sensorDryValue = 490;  // Reading when sensor is in dry air
int sensorWetValue = 212;   // Reading when sensor is in water

void setup() {
  Serial.begin(9600);
  pinMode(SOIL_SENSOR_PIN, INPUT);
  
  Serial.println("===============================================");
  Serial.println("     SOIL MOISTURE SENSOR CALIBRATION");
  Serial.println("===============================================");
  Serial.println();
  Serial.println("Instructions:");
  Serial.println("1. Leave sensor in air (dry) - note the RAW value");
  Serial.println("2. Dip sensor in water - note the RAW value");
  Serial.println("3. Test with actual soil at different moisture levels");
  Serial.println("4. Adjust thresholds in config.h based on your readings");
  Serial.println();
  Serial.println("Current Thresholds:");
  Serial.print("  DRY: < ");
  Serial.print(dryThreshold);
  Serial.println("%");
  Serial.print("  OPTIMAL: ");
  Serial.print(dryThreshold);
  Serial.print("% - ");
  Serial.print(wetThreshold);
  Serial.println("%");
  Serial.print("  WET: > ");
  Serial.print(wetThreshold);
  Serial.println("%");
  Serial.println();
  Serial.println("Readings (update every 1 second):");
  Serial.println("RAW | PERCENT | STATUS");
  Serial.println("----+--------+--------");
}

void loop() {
  // Read raw analog value
  int rawValue = analogRead(SOIL_SENSOR_PIN);
  
  // Convert to percentage
  int percentage = map(rawValue, sensorDryValue, sensorWetValue, 0, 100);
  percentage = constrain(percentage, 0, 100);
  
  // Determine status
  String status;
  if (percentage < dryThreshold) {
    status = "DRY 🟤";
  } else if (percentage > wetThreshold) {
    status = "WET 💙";
  } else {
    status = "OPTIMAL 💚";
  }
  
  // Print readings
  Serial.print(rawValue);
  Serial.print(" | ");
  Serial.print(percentage);
  Serial.print("%     | ");
  Serial.println(status);
  
  delay(1000);
}

/*
CALIBRATION NOTES:
==================

1. DRY AIR TEST:
   - Leave sensor in open air
   - Note the RAW value (usually 1000-1023)
   - Update sensorDryValue above

2. WATER TEST:
   - Dip sensor probes in water (don't submerge electronics!)
   - Note the RAW value (usually 200-400)
   - Update sensorWetValue above

3. SOIL TESTS:
   - Test with completely dry soil
   - Test with slightly moist soil
   - Test with very wet soil
   - Note the PERCENT values for each condition

4. SET THRESHOLDS:
   Based on your soil tests, update these in config.h:
   - SOIL_MOISTURE_THRESHOLD_DRY (when plant needs water)
   - SOIL_MOISTURE_THRESHOLD_WET (when soil is too wet)

EXAMPLE READINGS:
- Dry air: RAW=1020, PERCENT=0%
- Dry soil: RAW=800, PERCENT=25% 
- Moist soil: RAW=600, PERCENT=50%
- Wet soil: RAW=400, PERCENT=75%
- In water: RAW=300, PERCENT=100%

RECOMMENDED THRESHOLDS:
- DRY: < 30% (needs watering)
- OPTIMAL: 30-70% (good range)
- WET: > 70% (too much water)
*/

