// =============================================================================
// WATER PUMP TEST WITH RELAY MODULE
// =============================================================================
// Test water pump control via 1-channel relay module
// Connect: Arduino Pin 3 → Relay IN

#define WATER_PUMP_RELAY_PIN 3  // Pin that controls the relay
#define PUMP_TEST_DURATION 3000 // 3 seconds for testing

bool pumpRunning = false;
unsigned long pumpStartTime = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("===============================================");
  Serial.println("        WATER PUMP + RELAY TEST");
  Serial.println("===============================================");
  Serial.println();
  
  // Initialize relay control pin
  pinMode(WATER_PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(WATER_PUMP_RELAY_PIN, LOW);  // Start with pump OFF
  pumpRunning = false;
  
  Serial.println("🔧 Relay initialized (Pump OFF)");
  Serial.println();
  Serial.println("WIRING CHECK:");
  Serial.println("  Arduino Pin 3 → Relay IN");
  Serial.println("  Arduino 5V → Relay VCC");
  Serial.println("  Arduino GND → Relay GND");
  Serial.println("  External 5V+ → Relay COM");
  Serial.println("  Water Pump + → Relay NO");
  Serial.println("  Water Pump - → External GND");
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  'on' or '1'    - Turn pump ON");
  Serial.println("  'off' or '0'   - Turn pump OFF");
  Serial.println("  'test'         - Run 3-second test");
  Serial.println("  'status'       - Show current status");
  Serial.println();
  Serial.println("⚠️  SAFETY: Make sure pump is in water container!");
  Serial.println("Enter command:");
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "on" || command == "1") {
      turnPumpOn();
    }
    else if (command == "off" || command == "0") {
      turnPumpOff();
    }
    else if (command == "test") {
      runPumpTest();
    }
    else if (command == "status") {
      showStatus();
    }
    else {
      Serial.println("❌ Unknown command. Try: on, off, test, status");
    }
    
    Serial.println("Enter next command:");
  }
  
  // Safety check - auto turn off after test duration
  if (pumpRunning) {
    unsigned long runTime = millis() - pumpStartTime;
    if (runTime >= 30000) {  // 30 second safety timeout
      Serial.println("⚠️ SAFETY TIMEOUT - Turning pump OFF!");
      turnPumpOff();
    }
  }
  
  delay(100);
}

void turnPumpOn() {
  if (!pumpRunning) {
    Serial.println("💧 Turning water pump ON...");
    digitalWrite(WATER_PUMP_RELAY_PIN, HIGH);  // Activate relay
    pumpRunning = true;
    pumpStartTime = millis();
    Serial.println("✅ Water pump is ON");
    Serial.println("   (Relay activated - should hear click)");
  } else {
    Serial.println("ℹ️ Water pump is already ON");
  }
}

void turnPumpOff() {
  if (pumpRunning) {
    Serial.println("🛑 Turning water pump OFF...");
    digitalWrite(WATER_PUMP_RELAY_PIN, LOW);   // Deactivate relay
    pumpRunning = false;
    
    unsigned long runDuration = millis() - pumpStartTime;
    Serial.println("✅ Water pump is OFF");
    Serial.print("   (Ran for ");
    Serial.print(runDuration / 1000.0);
    Serial.println(" seconds)");
  } else {
    Serial.println("ℹ️ Water pump is already OFF");
  }
}

void runPumpTest() {
  Serial.println("🧪 Running 3-second pump test...");
  Serial.println("   Make sure pump is submerged in water!");
  
  delay(1000);  // Give time to prepare
  
  turnPumpOn();
  delay(PUMP_TEST_DURATION);
  turnPumpOff();
  
  Serial.println("✅ Pump test complete!");
}

void showStatus() {
  Serial.println("📊 === PUMP STATUS ===");
  Serial.print("Relay Pin: ");
  Serial.println(WATER_PUMP_RELAY_PIN);
  Serial.print("Pump State: ");
  Serial.println(pumpRunning ? "ON 💧" : "OFF 🛑");
  
  if (pumpRunning) {
    unsigned long runTime = millis() - pumpStartTime;
    Serial.print("Running for: ");
    Serial.print(runTime / 1000.0);
    Serial.println(" seconds");
  }
  
  Serial.print("Relay Output: ");
  Serial.println(digitalRead(WATER_PUMP_RELAY_PIN) ? "HIGH (Active)" : "LOW (Inactive)");
  Serial.println("=====================");
}

/*
TROUBLESHOOTING GUIDE:
=====================

PUMP DOESN'T RUN:
1. Check relay wiring (IN pin to Arduino Pin 3)
2. Verify external power supply (5V for pump)
3. Ensure common ground between Arduino and external power
4. Listen for relay "click" sound when turning on
5. Check pump connections (+ to NO, - to external GND)

RELAY DOESN'T CLICK:
1. Check VCC and GND connections to relay
2. Verify IN pin connection to Arduino Pin 3
3. Try different Arduino pin if needed

PUMP RUNS BUT NO WATER:
1. Ensure pump is fully submerged
2. Check for air bubbles in pump
3. Verify pump direction (some pumps have flow direction)
4. Check if pump intake is blocked

SAFETY NOTES:
- Always test with pump in water container
- Never run pump dry (can damage pump)
- 30-second safety timeout prevents overheating
- Use external power supply for pump (not Arduino power)

RELAY MODULE TYPES:
- Active HIGH: digitalWrite(pin, HIGH) turns relay ON
- Active LOW: digitalWrite(pin, LOW) turns relay ON
- Most common modules are Active HIGH (this code assumes that)
- If relay works backwards, swap HIGH/LOW in the code
*/
