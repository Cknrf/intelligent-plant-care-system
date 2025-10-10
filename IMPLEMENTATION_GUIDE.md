# Intelligent Adaptive Plant Care System - Implementation Guide

## 📋 Table of Contents
1. [System Overview](#system-overview)
2. [System Architecture](#system-architecture)
3. [Decision-Making Logic](#decision-making-logic)
4. [Implementation Flow](#implementation-flow)
5. [Module Requirements](#module-requirements)
6. [Integration Guide](#integration-guide)
7. [Testing Strategy](#testing-strategy)

---

## 🎯 System Overview

### Project Objectives
Based on your project proposal, the system must:
1. **Automated Watering**: Water plants based on soil moisture + weather forecast
2. **Automated Shading**: Move shade horizontally based on weather + plant conditions
3. **Sensor Integration**: Soil moisture, rain, lux sensors for intelligent decisions
4. **Weather Prediction**: Use OpenWeatherMap API for predictive adjustments
5. **Alert System**: Send notifications via Discord webhook

### Key Components
- **Microcontroller**: Arduino Uno R4 WiFi
- **Sensors**: Soil Moisture, Rain, Lux/Luminosity
- **Actuators**: Water Pump, Solenoid Valves (per plant), Servo Motor (shading)
- **Network**: WiFi, Weather API, Webhook API

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    MAIN CONTROLLER                          │
│              (PlantCareSystem.ino)                          │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │         DECISION-MAKING ENGINE                      │  │
│  │  • Analyze sensor data                              │  │
│  │  • Check weather forecast                           │  │
│  │  • Make watering decisions                          │  │
│  │  │  • Make shading decisions                          │  │
│  └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
           ↓                    ↓                    ↓
    ┌──────────┐         ┌──────────┐         ┌──────────┐
    │ SENSORS  │         │ ACTUATORS│         │ NETWORK  │
    └──────────┘         └──────────┘         └──────────┘
         ↓                    ↓                    ↓
    ┌─────────┐         ┌─────────┐         ┌─────────┐
    │ Soil    │         │ Water   │         │ Weather │
    │ Moisture│         │ Pump    │         │ API     │
    ├─────────┤         ├─────────┤         ├─────────┤
    │ Rain    │         │ Solenoid│         │ Webhook │
    │ Sensor  │         │ Valve   │         │ Notify  │
    ├─────────┤         ├─────────┤         └─────────┘
    │ Lux     │         │ Servo   │
    │ Sensor  │         │ Motor   │
    └─────────┘         └─────────┘
```

---

## 🧠 Decision-Making Logic

### 1. AUTOMATED WATERING SYSTEM

#### Decision Tree:
```
START
  ↓
Read Soil Moisture Level
  ↓
Is Moisture < CRITICAL_LEVEL (15%)?
  ├─ YES → IMMEDIATE WATERING
  │         ├─ Send CRITICAL alert
  │         ├─ Activate water pump
  │         └─ Water until OPTIMAL_MIN (40%)
  │
  └─ NO → Is Moisture < THRESHOLD (30%)?
      ├─ YES → Check Weather Forecast
      │         ↓
      │       Is Rain Predicted (next 3 hours)?
      │         ├─ YES → SKIP WATERING
      │         │         └─ Send INFO: "Rain expected, watering postponed"
      │         │
      │         └─ NO → Check Rain Sensor
      │             ↓
      │           Is Currently Raining?
      │             ├─ YES → SKIP WATERING
      │             │         └─ Send INFO: "Currently raining"
      │             │
      │             └─ NO → WATER PLANT
      │                 ├─ Send WARNING: "Low moisture, watering..."
      │                 ├─ Activate water pump
      │                 ├─ Water for configured duration
      │                 └─ Send SUCCESS: "Watering complete"
      │
      └─ NO → Is Moisture > OPTIMAL_MAX (70%)?
          ├─ YES → Send WARNING: "Over-watered"
          └─ NO → Send INFO: "Moisture optimal"
```

#### Watering Logic Pseudocode:
```cpp
void checkAndWaterPlants() {
  float moisture = readSoilMoisture();
  
  // CRITICAL: Immediate watering needed
  if (moisture < CRITICAL_MOISTURE_LEVEL) {
    notifyCritical("Critical moisture: " + moisture + "%");
    waterPlant(EMERGENCY_DURATION);
    return;
  }
  
  // LOW: Check weather before watering
  if (moisture < SOIL_MOISTURE_THRESHOLD) {
    WeatherData weather = getCurrentWeather();
    
    // Check if rain is predicted
    if (isRainPredicted(weather)) {
      notifyInfo("Rain expected, postponing watering");
      return;
    }
    
    // Check if currently raining
    if (isRaining()) {
      notifyInfo("Currently raining, skipping watering");
      return;
    }
    
    // Water the plant
    notifyWarning("Low moisture: " + moisture + "%, watering now");
    waterPlant(WATER_PUMP_DURATION_MS);
    notifySuccess("Watering complete");
  }
  
  // OPTIMAL: No action needed
  else if (moisture >= OPTIMAL_MOISTURE_MIN && 
           moisture <= OPTIMAL_MOISTURE_MAX) {
    // All good, do nothing
  }
  
  // OVER-WATERED: Alert only
  else if (moisture > OPTIMAL_MOISTURE_MAX) {
    notifyWarning("Over-watered: " + moisture + "%");
  }
}
```

---

### 2. AUTOMATED SHADING MECHANISM

#### Decision Tree:
```
START
  ↓
Read Lux Level & Rain Sensor
  ↓
Is Currently Raining?
  ├─ YES → DEPLOY SHADE (Protection Mode)
  │         ├─ Move servo to COVERED position
  │         ├─ Send INFO: "Rain detected, shade deployed"
  │         └─ Keep shade until rain stops
  │
  └─ NO → Check Weather Forecast
      ↓
    Is Heavy Rain Predicted (next 1 hour)?
      ├─ YES → DEPLOY SHADE (Preventive Mode)
      │         ├─ Move servo to COVERED position
      │         └─ Send INFO: "Rain expected, shade deployed"
      │
      └─ NO → Check Temperature & Lux
          ↓
        Is Temperature > 35°C AND Lux > 50000?
          ├─ YES → DEPLOY SHADE (Heat Protection)
          │         ├─ Move servo to PARTIAL_SHADE position
          │         └─ Send WARNING: "Excessive heat, partial shade"
          │
          └─ NO → Check Lux Level
              ↓
            Is Lux < LUX_THRESHOLD_LOW (200)?
              ├─ YES → RETRACT SHADE (Max Light)
              │         ├─ Move servo to OPEN position
              │         └─ Send INFO: "Low light, shade retracted"
              │
              └─ NO → OPTIMAL CONDITIONS
                  └─ Move servo to OPEN position
```

#### Shading Logic Pseudocode:
```cpp
void manageShading() {
  int luxLevel = readLuxSensor();
  bool raining = isRaining();
  WeatherData weather = getCurrentWeather();
  
  // PRIORITY 1: Active rain protection
  if (raining) {
    deployShade(SHADE_FULL_COVER);
    notifyInfo("Rain detected, deploying shade");
    return;
  }
  
  // PRIORITY 2: Predicted rain protection
  if (isRainPredicted(weather, 1)) { // 1 hour window
    deployShade(SHADE_FULL_COVER);
    notifyInfo("Rain predicted, deploying shade");
    return;
  }
  
  // PRIORITY 3: Excessive heat protection
  if (weather.temperature > 35 && luxLevel > 50000) {
    deployShade(SHADE_PARTIAL);
    notifyWarning("Excessive heat, partial shade deployed");
    return;
  }
  
  // PRIORITY 4: Low light - maximize exposure
  if (luxLevel < LUX_THRESHOLD_LOW) {
    retractShade(SHADE_OPEN);
    notifyInfo("Low light, shade retracted");
    return;
  }
  
  // DEFAULT: Optimal conditions - keep open
  retractShade(SHADE_OPEN);
}
```

---

## 📊 Implementation Flow

### System Startup Sequence

```
1. INITIALIZATION PHASE (0-30 seconds)
   ├─ Initialize Serial Communication (9600 baud)
   ├─ Initialize WiFi Connection
   │  └─ Retry up to 3 times if failed
   ├─ Initialize Weather API
   │  └─ Fetch initial weather data
   ├─ Initialize Webhook Notification
   │  └─ Send "System Started" notification
   ├─ Initialize All Sensors
   │  ├─ Soil Moisture Sensor
   │  ├─ Rain Sensor (with interrupt)
   │  └─ Lux Sensor
   ├─ Initialize All Actuators
   │  ├─ Water Pump (OFF state)
   │  ├─ Solenoid Valves (CLOSED state)
   │  └─ Servo Motor (OPEN position)
   └─ Send "System Ready" notification

2. MAIN LOOP (Continuous)
   ├─ Every 10 seconds: System Health Check
   │  ├─ Check WiFi connection
   │  └─ Check sensor status
   │
   ├─ Every 30 seconds: Read Soil Moisture
   │  ├─ Read analog value
   │  ├─ Convert to percentage
   │  └─ Store in variable
   │
   ├─ Every 60 seconds: Read Lux Level
   │  ├─ Read analog value
   │  ├─ Convert to lux
   │  └─ Store in variable
   │
   ├─ Every 5 minutes: Update Weather Data
   │  ├─ Fetch from OpenWeatherMap API
   │  ├─ Parse temperature, humidity, description
   │  └─ Store for decision-making
   │
   ├─ Every 60 seconds: Make Watering Decision
   │  ├─ Analyze soil moisture
   │  ├─ Check weather forecast
   │  ├─ Check rain sensor
   │  └─ Execute watering if needed
   │
   ├─ Every 30 seconds: Make Shading Decision
   │  ├─ Analyze lux level
   │  ├─ Check rain sensor
   │  ├─ Check weather forecast
   │  ├─ Check temperature
   │  └─ Adjust shade position
   │
   └─ Continuous: Monitor Rain Sensor (Interrupt-based)
      └─ Immediate shade deployment on rain detection
```

---

## 🔧 Module Requirements

### 1. Sensor Modules

#### A. Soil Moisture Sensor (`sensors/soil_sensor/soil_sensor.ino`)
**Purpose**: Measure soil moisture level to determine watering needs

**Functions to Implement**:
```cpp
// Initialize soil moisture sensor
void initSoilSensor();

// Read raw analog value (0-1023)
int readSoilMoistureRaw();

// Convert raw value to percentage (0-100%)
float readSoilMoisturePercent();

// Check if soil is dry (below threshold)
bool isSoilDry();

// Check if soil is critically dry
bool isSoilCriticallyDry();

// Calibrate sensor (optional)
void calibrateSoilSensor(int dryValue, int wetValue);
```

**Key Logic**:
- Read analog pin (A0)
- Convert 0-1023 to 0-100% (inverted: high value = dry, low value = wet)
- Apply calibration if available
- Return percentage value

**Calibration**:
- Dry air: ~1023 (0% moisture)
- Fully submerged in water: ~300-400 (100% moisture)

---

#### B. Rain Sensor (`sensors/rain_sensor/rain_sensor.ino`)
**Purpose**: Detect rainfall to prevent unnecessary watering and trigger shade deployment

**Functions to Implement**:
```cpp
// Initialize rain sensor with interrupt
void initRainSensor();

// Read rain sensor state (digital)
bool isRaining();

// Read rain intensity (analog)
int getRainIntensity();

// Interrupt handler for rain detection
void rainDetectedISR();

// Check if it has been raining for X seconds
bool isRainingForDuration(unsigned long durationMs);
```

**Key Logic**:
- Use digital pin (D2) with interrupt
- LOW = rain detected, HIGH = no rain
- Debounce signal to avoid false positives
- Trigger immediate action on rain detection

---

#### C. Lux Sensor (`sensors/lux_sensor/lux_sensor.ino`)
**Purpose**: Measure light intensity to manage shading mechanism

**Functions to Implement**:
```cpp
// Initialize lux sensor
void initLuxSensor();

// Read raw analog value
int readLuxRaw();

// Convert to lux units
int readLuxLevel();

// Check if light is too low
bool isLightLow();

// Check if light is too high (excessive)
bool isLightExcessive();

// Get light condition as string
String getLightCondition();
```

**Key Logic**:
- Read analog pin (A1)
- Convert to lux (depends on sensor type)
- Typical ranges:
  - 0-50 lux: Very dark
  - 50-200 lux: Low light
  - 200-10000 lux: Normal daylight
  - 10000-50000 lux: Bright sun
  - 50000+ lux: Excessive sun

---

### 2. Actuator Modules

#### A. Water Pump (`actuators/water_pump/water_pump.ino`)
**Purpose**: Control water pump for plant watering

**Functions to Implement**:
```cpp
// Initialize water pump
void initWaterPump();

// Turn pump ON
void waterPumpOn();

// Turn pump OFF
void waterPumpOff();

// Water for specific duration
void waterForDuration(unsigned long durationMs);

// Check if pump is currently running
bool isPumpRunning();

// Emergency stop
void emergencyStopPump();
```

**Key Logic**:
- Use digital pin (D3)
- HIGH = pump ON, LOW = pump OFF
- Always turn OFF after duration
- Safety timeout to prevent overflow

---

#### B. Solenoid Valve (`actuators/solenoid_valve/solenoid_valve.ino`)
**Purpose**: Control individual plant watering (one valve per plant)

**Functions to Implement**:
```cpp
// Initialize solenoid valve
void initSolenoidValve(int pin);

// Open valve
void openValve(int valveNumber);

// Close valve
void closeValve(int valveNumber);

// Open valve for duration
void openValveForDuration(int valveNumber, unsigned long durationMs);

// Close all valves (safety)
void closeAllValves();

// Check valve state
bool isValveOpen(int valveNumber);
```

**Key Logic**:
- Multiple valves for multiple plants
- HIGH = valve OPEN, LOW = valve CLOSED
- Coordinate with water pump
- Always close after watering

---

#### C. Servo Motor (`actuators/servo_motor/servo_motor.ino`)
**Purpose**: Control horizontal shading mechanism

**Functions to Implement**:
```cpp
// Initialize servo motor
void initServoMotor();

// Move to specific angle (0-180)
void moveServoToAngle(int angle);

// Deploy shade (cover plants)
void deployShade();

// Retract shade (expose plants)
void retractShade();

// Partial shade (50% coverage)
void partialShade();

// Get current shade position
int getCurrentShadePosition();

// Smooth movement (gradual)
void moveServoSmooth(int targetAngle, int delayMs);
```

**Key Logic**:
- 0° = Fully retracted (open, max sun)
- 90° = Partial shade (50% coverage)
- 180° = Fully deployed (covered, rain protection)
- Use smooth movements to avoid mechanical stress

**Shade Positions**:
```cpp
#define SHADE_OPEN 0        // Full sun exposure
#define SHADE_PARTIAL 90    // Partial coverage
#define SHADE_FULL_COVER 180 // Full protection
```

---

### 3. Main Controller (`PlantCareSystem/PlantCareSystem.ino`)

**Purpose**: Orchestrate all modules and implement decision-making logic

**Main Structure**:
```cpp
// Include all modules
#include "../config.h"
// Include sensor headers
// Include actuator headers
// Include network headers

// Global state variables
struct SystemState {
  float currentMoisture;
  int currentLux;
  bool isRaining;
  WeatherData currentWeather;
  unsigned long lastWatering;
  unsigned long lastShadeAdjustment;
  int currentShadePosition;
};

void setup() {
  // 1. Initialize serial
  // 2. Initialize WiFi
  // 3. Initialize weather API
  // 4. Initialize webhook
  // 5. Initialize all sensors
  // 6. Initialize all actuators
  // 7. Send startup notification
  // 8. Fetch initial weather data
}

void loop() {
  // 1. Update sensor readings (with intervals)
  // 2. Update weather data (every 5 min)
  // 3. Make watering decision (every 60 sec)
  // 4. Make shading decision (every 30 sec)
  // 5. Send status updates (every 30 min)
  // 6. Handle emergency conditions (immediate)
}

// Decision-making functions
void makeWateringDecision();
void makeShadingDecision();
void handleEmergencyConditions();
void updateSensorReadings();
void sendStatusUpdate();
```

---

## 🔗 Integration Guide

### Step-by-Step Integration

#### Phase 1: Individual Module Testing
1. Test each sensor module independently
2. Test each actuator module independently
3. Test weather API module
4. Test webhook notification module
5. Verify all readings and actions

#### Phase 2: Sensor-Actuator Integration
1. Connect soil sensor → water pump
2. Connect rain sensor → servo motor
3. Connect lux sensor → servo motor
4. Test sensor-actuator pairs

#### Phase 3: Weather API Integration
1. Integrate weather data into watering decision
2. Integrate weather data into shading decision
3. Test with different weather conditions

#### Phase 4: Notification Integration
1. Add notifications to all major events
2. Add notifications to error conditions
3. Test notification delivery

#### Phase 5: Full System Integration
1. Combine all modules in main controller
2. Implement complete decision-making logic
3. Test full system operation
4. Fine-tune thresholds and timings

---

## 🧪 Testing Strategy

### Unit Tests (Individual Modules)

**Soil Moisture Sensor**:
- [ ] Test in dry soil → Should read < 30%
- [ ] Test in wet soil → Should read > 70%
- [ ] Test in air → Should read ~0%
- [ ] Test in water → Should read ~100%

**Rain Sensor**:
- [ ] Test in dry conditions → Should return false
- [ ] Test with water drops → Should return true
- [ ] Test debouncing → Should ignore brief signals

**Lux Sensor**:
- [ ] Test in darkness → Should read < 50 lux
- [ ] Test in room light → Should read 200-1000 lux
- [ ] Test in sunlight → Should read > 10000 lux

**Water Pump**:
- [ ] Test ON command → Pump should run
- [ ] Test OFF command → Pump should stop
- [ ] Test duration → Should stop after time
- [ ] Test safety timeout → Should auto-stop

**Servo Motor**:
- [ ] Test 0° position → Should be fully retracted
- [ ] Test 90° position → Should be at middle
- [ ] Test 180° position → Should be fully deployed
- [ ] Test smooth movement → Should move gradually

### Integration Tests

**Watering System**:
- [ ] Dry soil + no rain → Should water
- [ ] Dry soil + raining → Should NOT water
- [ ] Dry soil + rain predicted → Should NOT water
- [ ] Critical moisture → Should water immediately
- [ ] Optimal moisture → Should NOT water

**Shading System**:
- [ ] Rain detected → Should deploy shade
- [ ] Rain predicted → Should deploy shade
- [ ] High temp + high lux → Should deploy partial shade
- [ ] Low lux → Should retract shade
- [ ] Optimal conditions → Should keep open

**Notification System**:
- [ ] System startup → Should send notification
- [ ] Low moisture → Should send warning
- [ ] Watering complete → Should send success
- [ ] Rain detected → Should send info
- [ ] System error → Should send critical alert

### System Tests

**24-Hour Operation**:
- [ ] System runs continuously without crashes
- [ ] All sensors update at correct intervals
- [ ] Weather data refreshes every 5 minutes
- [ ] Decisions are made correctly
- [ ] Notifications are sent appropriately

**Edge Cases**:
- [ ] WiFi disconnection → Should reconnect
- [ ] Weather API failure → Should use last known data
- [ ] Sensor failure → Should send error notification
- [ ] Multiple rain events → Should handle correctly
- [ ] Rapid weather changes → Should adapt quickly

---

## 📝 Configuration Checklist

### Before Deployment

- [ ] Update WiFi credentials in `config.h`
- [ ] Update Weather API key in `config.h`
- [ ] Update Discord webhook URL in `config.h`
- [ ] Update location coordinates in `config.h`
- [ ] Calibrate soil moisture sensor
- [ ] Test rain sensor sensitivity
- [ ] Calibrate lux sensor (if needed)
- [ ] Test water pump flow rate
- [ ] Adjust watering duration
- [ ] Test servo motor range
- [ ] Set appropriate thresholds
- [ ] Verify all pin assignments

### Threshold Tuning

Adjust these values in `config.h` based on your specific plants:

```cpp
// Soil Moisture Thresholds
#define CRITICAL_MOISTURE_LEVEL 15  // Emergency watering
#define SOIL_MOISTURE_THRESHOLD 30  // Normal watering trigger
#define OPTIMAL_MOISTURE_MIN 40     // Optimal range start
#define OPTIMAL_MOISTURE_MAX 70     // Optimal range end

// Lux Thresholds
#define LUX_THRESHOLD_LOW 200       // Deploy shade if below
#define LUX_THRESHOLD_HIGH 50000    // Partial shade if above

// Temperature Thresholds
#define TEMP_THRESHOLD_HIGH 35      // Deploy shade if above

// Timing Intervals
#define WATER_PUMP_DURATION_MS 5000         // 5 seconds
#define WEATHER_UPDATE_INTERVAL_MS 300000   // 5 minutes
#define SOIL_READ_INTERVAL_MS 30000         // 30 seconds
#define LUX_READ_INTERVAL_MS 60000          // 1 minute
```

---

## 🎓 Key Takeaways

### Critical Success Factors

1. **Sensor Accuracy**: Calibrate all sensors properly
2. **Weather Integration**: Always check forecast before watering
3. **Safety First**: Implement timeouts and emergency stops
4. **Robust Error Handling**: Handle WiFi drops, API failures
5. **Proper Timing**: Don't check sensors too frequently
6. **Notification Strategy**: Alert on important events only
7. **Power Management**: Ensure adequate power for pump + servo
8. **Water Management**: Prevent over-watering and flooding

### Common Pitfalls to Avoid

❌ **Don't**: Water while it's raining
❌ **Don't**: Run water pump without timeout
❌ **Don't**: Move servo too fast (mechanical stress)
❌ **Don't**: Check weather API too frequently (rate limits)
❌ **Don't**: Ignore sensor calibration
❌ **Don't**: Use blocking delays in main loop
❌ **Don't**: Water at night (fungal growth risk)
❌ **Don't**: Deploy shade in low light conditions

✅ **Do**: Check weather before watering
✅ **Do**: Implement safety timeouts
✅ **Do**: Use smooth servo movements
✅ **Do**: Cache weather data
✅ **Do**: Calibrate sensors regularly
✅ **Do**: Use non-blocking timing
✅ **Do**: Water in morning/evening
✅ **Do**: Maximize light exposure when possible

---

## 📚 Next Steps

1. **Read this guide thoroughly**
2. **Review the refactored code** (weather_api.ino, webhook_notification.ino)
3. **Implement sensor modules** (following the templates provided)
4. **Implement actuator modules** (following the templates provided)
5. **Implement main controller** (with decision-making logic)
6. **Test each module individually**
7. **Integrate modules step by step**
8. **Calibrate and tune thresholds**
9. **Run 24-hour test**
10. **Deploy and monitor**

---

**Good luck with your implementation! 🌱**

*This guide is based on your project proposal and research objectives. Adjust thresholds and logic based on your specific plant species and environmental conditions.*

