#include "../../config.h"

// =============================================================================
// SOLENOID VALVE MODULE
// =============================================================================
// Controls 2 solenoid valves (one per plant)

bool valve1Open = false;
bool valve2Open = false;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
void initSolenoidValves();
void openValve(int plantNum);
void closeValve(int plantNum);
void closeAllValves();
bool isValveOpen(int plantNum);

// =============================================================================
// SETUP FUNCTION (for standalone testing)
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 5000);
  
  Serial.println("=== Solenoid Valve Module ===");
  initSolenoidValves();
  Serial.println("Solenoid valves initialized!");
  
  // Test valves
  Serial.println("Testing Valve 1...");
  openValve(1);
  delay(2000);
  closeValve(1);
  
  Serial.println("Testing Valve 2...");
  openValve(2);
  delay(2000);
  closeValve(2);
  
  Serial.println("Test complete!");
}

// =============================================================================
// MAIN LOOP (for standalone testing)
// =============================================================================
void loop() {
  // Toggle valves every 5 seconds
  static unsigned long lastToggle = 0;
  static int currentValve = 1;
  
  if (millis() - lastToggle >= 5000) {
    if (isValveOpen(currentValve)) {
      Serial.print("Closing valve ");
      Serial.println(currentValve);
      closeValve(currentValve);
      
      // Switch to other valve
      currentValve = (currentValve == 1) ? 2 : 1;
    } else {
      Serial.print("Opening valve ");
      Serial.println(currentValve);
      openValve(currentValve);
    }
    lastToggle = millis();
  }
}

// =============================================================================
// SOLENOID VALVE FUNCTIONS
// =============================================================================

/**
 * Initialize both solenoid valves
 */
void initSolenoidValves() {
  pinMode(SOLENOID_VALVE_1_PIN, OUTPUT);
  pinMode(SOLENOID_VALVE_2_PIN, OUTPUT);
  
  // Start with all valves closed
  digitalWrite(SOLENOID_VALVE_1_PIN, LOW);
  digitalWrite(SOLENOID_VALVE_2_PIN, LOW);
  
  valve1Open = false;
  valve2Open = false;
  
  if (DEBUG_MODE) {
    Serial.println("[VALVE] Initialized (all closed)");
  }
}

/**
 * Open solenoid valve for specific plant
 * @param plantNum: 1 or 2
 */
void openValve(int plantNum) {
  if (plantNum == 1) {
    if (!valve1Open) {
      digitalWrite(SOLENOID_VALVE_1_PIN, HIGH);
      valve1Open = true;
      
      if (DEBUG_MODE) {
        Serial.println("[VALVE] Valve 1 OPENED (Plant 1)");
      }
    }
  } else if (plantNum == 2) {
    if (!valve2Open) {
      digitalWrite(SOLENOID_VALVE_2_PIN, HIGH);
      valve2Open = true;
      
      if (DEBUG_MODE) {
        Serial.println("[VALVE] Valve 2 OPENED (Plant 2)");
      }
    }
  }
}

/**
 * Close solenoid valve for specific plant
 * @param plantNum: 1 or 2
 */
void closeValve(int plantNum) {
  if (plantNum == 1) {
    if (valve1Open) {
      digitalWrite(SOLENOID_VALVE_1_PIN, LOW);
      valve1Open = false;
      
      if (DEBUG_MODE) {
        Serial.println("[VALVE] Valve 1 CLOSED (Plant 1)");
      }
    }
  } else if (plantNum == 2) {
    if (valve2Open) {
      digitalWrite(SOLENOID_VALVE_2_PIN, LOW);
      valve2Open = false;
      
      if (DEBUG_MODE) {
        Serial.println("[VALVE] Valve 2 CLOSED (Plant 2)");
      }
    }
  }
}

/**
 * Close all valves (emergency stop)
 */
void closeAllValves() {
  closeValve(1);
  closeValve(2);
  
  if (DEBUG_MODE) {
    Serial.println("[VALVE] All valves CLOSED");
  }
}

/**
 * Check if a valve is currently open
 * @param plantNum: 1 or 2
 * @return: true if valve is open
 */
bool isValveOpen(int plantNum) {
  if (plantNum == 1) {
    return valve1Open;
  } else if (plantNum == 2) {
    return valve2Open;
  }
  return false;
}

/**
 * Check if any valve is open
 * @return: true if any valve is open
 */
bool isAnyValveOpen() {
  return valve1Open || valve2Open;
}
