#include "../../config.h"

// =============================================================================
// WATER PUMP MODULE
// =============================================================================
// Controls water pump (shared by both plants via solenoid valves)

bool pumpRunning = false;
unsigned long pumpStartTime = 0;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
void initWaterPump();
void pumpOn();
void pumpOff();
bool isPumpRunning();
void checkPumpSafety();

// =============================================================================
// SETUP FUNCTION (for standalone testing)
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 5000);
  
  Serial.println("=== Water Pump Module ===");
  initWaterPump();
  Serial.println("Water pump initialized!");
  
  // Test pump
  Serial.println("Testing pump for 3 seconds...");
  pumpOn();
  delay(3000);
  pumpOff();
  Serial.println("Test complete!");
}

// =============================================================================
// MAIN LOOP (for standalone testing)
// =============================================================================
void loop() {
  checkPumpSafety();  // Always check for safety timeout
  delay(100);
}

// =============================================================================
// WATER PUMP FUNCTIONS
// =============================================================================

/**
 * Initialize water pump
 */
void initWaterPump() {
  pinMode(WATER_PUMP_PIN, OUTPUT);
  digitalWrite(WATER_PUMP_PIN, LOW);  // Start with pump off
  pumpRunning = false;
  
  if (DEBUG_MODE) {
    Serial.println("[PUMP] Initialized (OFF)");
  }
}

/**
 * Turn water pump on
 */
void pumpOn() {
  if (!pumpRunning) {
    digitalWrite(WATER_PUMP_PIN, HIGH);
    pumpRunning = true;
    pumpStartTime = millis();
    
    if (DEBUG_MODE) {
      Serial.println("[PUMP] Turned ON");
    }
  }
}

/**
 * Turn water pump off
 */
void pumpOff() {
  if (pumpRunning) {
    digitalWrite(WATER_PUMP_PIN, LOW);
    pumpRunning = false;
    
    unsigned long runDuration = millis() - pumpStartTime;
    
    if (DEBUG_MODE) {
      Serial.print("[PUMP] Turned OFF (ran for ");
      Serial.print(runDuration / 1000.0);
      Serial.println(" seconds)");
    }
  }
}

/**
 * Check if pump is currently running
 * Returns: true if pump is on
 */
bool isPumpRunning() {
  return pumpRunning;
}

/**
 * Safety check - automatically stop pump if running too long
 * IMPORTANT: Call this in main loop!
 */
void checkPumpSafety() {
  if (pumpRunning) {
    unsigned long runTime = millis() - pumpStartTime;
    
    if (runTime >= WATER_PUMP_MAX_DURATION_MS) {
      Serial.println("[PUMP] SAFETY TIMEOUT - Stopping pump!");
      pumpOff();
    }
  }
}

/**
 * Get how long pump has been running
 * Returns: Duration in milliseconds (0 if not running)
 */
unsigned long getPumpRunTime() {
  if (pumpRunning) {
    return millis() - pumpStartTime;
  }
  return 0;
}
