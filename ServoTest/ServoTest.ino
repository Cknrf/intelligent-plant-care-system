#include <Servo.h>

// =============================================================================
// SERVO MOTOR CALIBRATION TEST
// =============================================================================
// Use this to test servo movement and find optimal positions
// Connect servo signal wire to Arduino pin 9

// Pin configuration
#define SERVO_PIN 9

// Position settings
#define SHADE_OFF 0      // Shade retracted (plants exposed)
#define SHADE_ON 90      // Shade deployed (plants covered)

Servo shadeServo;
int currentPosition = 0;

void setup() {
  Serial.begin(9600);
  
  Serial.println("===============================================");
  Serial.println("         SERVO MOTOR TEST & CALIBRATION");
  Serial.println("===============================================");
  Serial.println();
  
  // Initialize servo
  shadeServo.attach(SERVO_PIN);
  shadeServo.write(SHADE_OFF);
  currentPosition = SHADE_OFF;
  delay(1000);
  
  Serial.println("Servo initialized at 0° (shade OFF)");
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  'on' or '1'  - Deploy shade (90°)");
  Serial.println("  'off' or '0' - Retract shade (0°)");
  Serial.println("  'test'       - Run automatic test sequence");
  Serial.println("  'smooth'     - Test smooth movement");
  Serial.println("  '45'         - Move to specific angle (0-180)");
  Serial.println();
  Serial.println("Current position: 0° (shade OFF)");
  Serial.println("Enter command:");
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "on" || command == "1") {
      deployShade();
    }
    else if (command == "off" || command == "0") {
      retractShade();
    }
    else if (command == "test") {
      runTestSequence();
    }
    else if (command == "smooth") {
      testSmoothMovement();
    }
    else if (command.toInt() >= 0 && command.toInt() <= 180) {
      moveToPosition(command.toInt());
    }
    else {
      Serial.println("Unknown command. Try: on, off, test, smooth, or angle (0-180)");
    }
    
    Serial.println("Enter next command:");
  }
}

void deployShade() {
  Serial.println("🏠 Deploying shade...");
  moveToPosition(SHADE_ON);
  Serial.println("✅ Shade deployed (90°) - Plants covered");
}

void retractShade() {
  Serial.println("☀️ Retracting shade...");
  moveToPosition(SHADE_OFF);
  Serial.println("✅ Shade retracted (0°) - Plants exposed");
}

void moveToPosition(int targetPosition) {
  targetPosition = constrain(targetPosition, 0, 180);
  
  Serial.print("Moving from ");
  Serial.print(currentPosition);
  Serial.print("° to ");
  Serial.print(targetPosition);
  Serial.println("°");
  
  shadeServo.write(targetPosition);
  currentPosition = targetPosition;
  
  delay(1000);  // Wait for servo to reach position
  
  Serial.print("✅ Moved to ");
  Serial.print(currentPosition);
  Serial.println("°");
}

void runTestSequence() {
  Serial.println("\n🔄 Running test sequence...");
  
  Serial.println("1. Starting position (0°)");
  moveToPosition(0);
  delay(1000);
  
  Serial.println("2. Quarter turn (45°)");
  moveToPosition(45);
  delay(1000);
  
  Serial.println("3. Half turn (90°) - Shade ON");
  moveToPosition(90);
  delay(1000);
  
  Serial.println("4. Three quarters (135°)");
  moveToPosition(135);
  delay(1000);
  
  Serial.println("5. Full range (180°)");
  moveToPosition(180);
  delay(1000);
  
  Serial.println("6. Back to shade ON (90°)");
  moveToPosition(90);
  delay(1000);
  
  Serial.println("7. Back to shade OFF (0°)");
  moveToPosition(0);
  
  Serial.println("✅ Test sequence complete!\n");
}

void testSmoothMovement() {
  Serial.println("\n🌊 Testing smooth movement...");
  
  Serial.println("Smooth deployment (0° → 90°):");
  for (int pos = 0; pos <= 90; pos++) {
    shadeServo.write(pos);
    Serial.print("Position: ");
    Serial.print(pos);
    Serial.println("°");
    delay(50);  // Adjust for speed
  }
  currentPosition = 90;
  
  delay(1000);
  
  Serial.println("Smooth retraction (90° → 0°):");
  for (int pos = 90; pos >= 0; pos--) {
    shadeServo.write(pos);
    Serial.print("Position: ");
    Serial.print(pos);
    Serial.println("°");
    delay(50);  // Adjust for speed
  }
  currentPosition = 0;
  
  Serial.println("✅ Smooth movement test complete!\n");
}

/*
CALIBRATION NOTES:
==================

SERVO POSITIONS:
- 0° = Shade fully retracted (plants get full sun/rain)
- 90° = Shade covering plants (protection mode)
- You can test other angles if needed

MECHANICAL CONSIDERATIONS:
1. Check that servo can move freely
2. Ensure shade mechanism doesn't bind at any position
3. Verify 0° and 90° positions work with your setup
4. Test under load (with actual shade attached)

POWER REQUIREMENTS:
- Servo needs 5V power (red wire)
- Connect to Arduino 5V pin or external 5V supply
- Brown/black wire to GND
- Orange/yellow wire to pin 9

SPEED ADJUSTMENT:
- Change delay in smooth movement for different speeds
- Faster = lower delay (more jerky)
- Slower = higher delay (smoother)

POSITION VERIFICATION:
1. Test 0° position - shade should be away from plants
2. Test 90° position - shade should cover both plants
3. Verify no mechanical interference
4. Check that plants get proper coverage at 90°

TROUBLESHOOTING:
- Servo jitters: Check power supply, connections
- Doesn't move: Verify wiring, power
- Wrong direction: Swap 0° and 90° values in code
- Not enough range: Adjust angles as needed

UPDATE CONFIG.H:
If you need different positions, update:
- SHADE_POSITION_OFF (currently 0°)
- SHADE_POSITION_ON (currently 90°)
- SERVO_SPEED_DELAY_MS (for smooth movement speed)

WIRING CHECK:
- Signal wire (orange/yellow) → Arduino pin 9
- Power wire (red) → Arduino 5V
- Ground wire (brown/black) → Arduino GND
*/

