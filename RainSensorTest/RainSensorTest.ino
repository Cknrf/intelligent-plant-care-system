// =============================================================================
// RAIN SENSOR CALIBRATION TEST
// =============================================================================
// Use this to find the right threshold for your rain sensor
// Connect sensor A0 pin to Arduino A2

// Pin configuration
#define RAIN_SENSOR_PIN A2

// Current threshold (calibrated based on your readings)
int rainThreshold = 800;  // Below this = rain detected

void setup() {
  Serial.begin(9600);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  
  Serial.println("===============================================");
  Serial.println("       RAIN SENSOR CALIBRATION TEST");
  Serial.println("===============================================");
  Serial.println();
  Serial.println("Instructions:");
  Serial.println("1. Test sensor in dry conditions - note RAW value");
  Serial.println("2. Drop water on sensor - note RAW value");
  Serial.println("3. Test with different amounts of water");
  Serial.println("4. Adjust RAIN_THRESHOLD in config.h");
  Serial.println();
  Serial.print("Current Rain Threshold: < ");
  Serial.print(rainThreshold);
  Serial.println(" = Rain detected");
  Serial.println();
  Serial.println("How rain sensors work:");
  Serial.println("- Higher values = DRY (no water on sensor)");
  Serial.println("- Lower values = WET (water detected)");
  Serial.println("- Typical range: 0-1023");
  Serial.println();
  Serial.println("Readings (update every 500ms):");
  Serial.println("RAW VALUE | STATUS | INTENSITY");
  Serial.println("----------+--------+-----------");
}

void loop() {
  // Read raw analog value
  int rawValue = analogRead(RAIN_SENSOR_PIN);
  
  // Determine rain status
  String status;
  String intensity;
  
  if (rawValue < 200) {
    status = "HEAVY RAIN 🌧️";
    intensity = "Very wet";
  } else if (rawValue < rainThreshold) {
    status = "LIGHT RAIN 🌦️";
    intensity = "Wet";
  } else if (rawValue < 800) {
    status = "HUMID 🌫️";
    intensity = "Slightly moist";
  } else {
    status = "DRY ☀️";
    intensity = "No water";
  }
  
  // Print reading with formatting
  Serial.print(rawValue);
  
  // Pad for alignment
  String rawStr = String(rawValue);
  for (int i = rawStr.length(); i < 9; i++) {
    Serial.print(" ");
  }
  
  Serial.print(" | ");
  Serial.print(status);
  
  // Pad status
  for (int i = status.length(); i < 15; i++) {
    Serial.print(" ");
  }
  
  Serial.print(" | ");
  Serial.println(intensity);
  
  delay(500);
}

/*
CALIBRATION NOTES:
==================

TYPICAL RAIN SENSOR VALUES:
- Completely dry: 1000-1023
- Slightly humid: 800-1000
- Light moisture: 500-800
- Water drops: 200-500
- Heavy water: 0-200

RAIN DETECTION LOGIC:
The sensor works opposite to soil moisture:
- HIGH values (1000+) = DRY conditions
- LOW values (0-500) = WET conditions

TESTING PROCEDURE:
1. DRY TEST:
   - Clean and dry the sensor completely
   - Note the RAW value (should be 1000+)

2. MOISTURE TESTS:
   - Breathe on sensor (humidity)
   - Touch with slightly damp finger
   - Drop small amount of water
   - Pour more water on sensor

3. THRESHOLD SETTING:
   Choose a value where you want "rain" to be detected
   Recommended: 500 (good balance)

PLANT CARE CONSIDERATIONS:
- Too sensitive = false rain detection from humidity
- Not sensitive enough = miss light rain
- Consider your local climate conditions

RECOMMENDED THRESHOLDS:
- DRY: > 800 (no rain)
- HUMID: 500-800 (high humidity, no action needed)
- LIGHT RAIN: 200-500 (rain detected)
- HEAVY RAIN: < 200 (lots of water)

UPDATE CONFIG.H:
Set RAIN_THRESHOLD to the value where you want rain detection.
Typical value: 500 (detects light rain and above)

WIRING CHECK:
- VCC → 5V or 3.3V
- GND → GND
- A0 (sensor output) → Arduino A2
- Make sure connections are secure
*/
