#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

// =============================================================================
// LUX SENSOR CALIBRATION TEST
// =============================================================================
// Use this to find the right light thresholds for your system
// Connect TSL2561 to I2C: SDA=A4, SCL=A5

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// Current threshold (adjust based on your readings)
float highLightThreshold = 50000;  // Above this = deploy shade

bool sensorAvailable = false;

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("===============================================");
  Serial.println("        LUX SENSOR CALIBRATION TEST");
  Serial.println("===============================================");
  Serial.println();
  
  // Initialize sensor
  Serial.print("Initializing TSL2561 sensor...");
  int attempts = 0;
  while (!tsl.begin() && attempts < 5) {
    Serial.print(".");
    attempts++;
    delay(200);
  }
  
  if (attempts < 5) {
    Serial.println(" SUCCESS!");
    tsl.enableAutoRange(true);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
    sensorAvailable = true;
    
    // Display sensor details
    sensor_t sensor;
    tsl.getSensor(&sensor);
    Serial.println();
    Serial.println("Sensor Details:");
    Serial.print("  Name: ");
    Serial.println(sensor.name);
    Serial.print("  Max Value: ");
    Serial.print(sensor.max_value);
    Serial.println(" lux");
    Serial.print("  Min Value: ");
    Serial.print(sensor.min_value);
    Serial.println(" lux");
    Serial.print("  Resolution: ");
    Serial.print(sensor.resolution);
    Serial.println(" lux");
  } else {
    Serial.println(" FAILED!");
    Serial.println("Check wiring: SDA=A4, SCL=A5, VCC=3.3V, GND=GND");
    sensorAvailable = false;
  }
  
  Serial.println();
  Serial.println("Instructions:");
  Serial.println("1. Test in different lighting conditions");
  Serial.println("2. Note lux values for each condition");
  Serial.println("3. Determine when shade should be deployed");
  Serial.println("4. Update LUX_THRESHOLD_HIGH in config.h");
  Serial.println();
  Serial.print("Current High Light Threshold: ");
  Serial.print(highLightThreshold);
  Serial.println(" lux");
  Serial.println();
  Serial.println("Readings (update every 2 seconds):");
  Serial.println("LUX VALUE | STATUS | CONDITION");
  Serial.println("----------+--------+-----------");
}

void loop() {
  if (!sensorAvailable) {
    Serial.println("Sensor not available - check wiring!");
    delay(5000);
    return;
  }
  
  // Get sensor reading
  sensors_event_t event;
  tsl.getEvent(&event);
  
  if (event.light > 0) {
    float lux = event.light;
    
    // Determine status
    String status;
    String condition;
    
    if (lux > highLightThreshold) {
      status = "HIGH ☀️";
      condition = "Deploy shade";
    } else if (lux > 10000) {
      status = "MEDIUM 🌤️";
      condition = "Monitor";
    } else if (lux > 200) {
      status = "LOW 🌥️";
      condition = "Normal";
    } else {
      status = "DARK 🌙";
      condition = "Night/Indoor";
    }
    
    // Print reading
    Serial.print(lux, 0);
    Serial.print(" lux");
    
    // Pad for alignment
    String luxStr = String(lux, 0) + " lux";
    for (int i = luxStr.length(); i < 9; i++) {
      Serial.print(" ");
    }
    
    Serial.print(" | ");
    Serial.print(status);
    
    // Pad status
    for (int i = status.length(); i < 10; i++) {
      Serial.print(" ");
    }
    
    Serial.print(" | ");
    Serial.println(condition);
    
  } else {
    Serial.println("Sensor overload or error - try covering sensor");
  }
  
  delay(2000);
}

/*
CALIBRATION NOTES:
==================

TYPICAL LUX VALUES:
- Direct sunlight: 50,000-100,000 lux
- Bright office: 500-1,000 lux
- Living room: 50-200 lux
- Moonlight: 0.1-1 lux
- Overcast day: 1,000-10,000 lux
- Shade outdoors: 10,000-20,000 lux

PLANT CARE CONSIDERATIONS:
- Most plants need 200-500 lux minimum
- Direct sun can be too intense (>50,000 lux)
- Shade deployment protects from excessive light
- Also consider heat from intense sunlight

RECOMMENDED THRESHOLDS:
- DARK: < 200 lux (night/indoor)
- LOW: 200-10,000 lux (normal indoor/cloudy)
- MEDIUM: 10,000-50,000 lux (bright outdoor)
- HIGH: > 50,000 lux (direct sun - deploy shade)

TEST CONDITIONS TO TRY:
1. Indoor room lighting
2. Near a window (indirect sunlight)
3. Direct sunlight (outdoor or through window)
4. Overcast day outside
5. Bright LED flashlight
6. Complete darkness

UPDATE CONFIG.H:
Set LUX_THRESHOLD_HIGH based on when you want shade deployed.
Typical value: 50,000 lux (direct sunlight)
*/

