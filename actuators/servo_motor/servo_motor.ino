#include <Servo.h>
#include "../../config.h"

// =============================================================================
// SERVO MOTOR MODULE (SHADING MECHANISM)
// =============================================================================
// Controls servo motor for shading both plants
// 0° = Shade away (plants exposed to sun/rain)
// 90° = Shade covering both plants

Servo shadeServo;
int currentPosition = SHADE_POSITION_OFF;  // Track current position
bool servoInitialized = false;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================
void initServo();
void moveShade(int targetPosition);
void deployShade();
void retractShade();
int getShadePosition();
bool isShadeDeployed();

// =============================================================================
// SETUP FUNCTION (for standalone testing)
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 5000);
  
  Serial.println("=== Servo Motor Module ===");
  initServo();
  Serial.println("Servo initialized!");
  
  // Test movement
  Serial.println("Testing shade deployment...");
  deployShade();
  delay(2000);
  
  Serial.println("Testing shade retraction...");
  retractShade();
  delay(2000);
}

// =============================================================================
// MAIN LOOP (for standalone testing)
// =============================================================================
void loop() {
  // Toggle shade every 5 seconds
  static unsigned long lastToggle = 0;
  
  if (millis() - lastToggle >= 5000) {
    if (isShadeDeployed()) {
      Serial.println("Retracting shade...");
      retractShade();
    } else {
      Serial.println("Deploying shade...");
      deployShade();
    }
    lastToggle = millis();
  }
}

// =============================================================================
// SERVO MOTOR FUNCTIONS
// =============================================================================

/**
 * Initialize servo motor
 */
void initServo() {
  shadeServo.attach(SERVO_PIN);
  
  // Start with shade retracted
  shadeServo.write(SHADE_POSITION_OFF);
  currentPosition = SHADE_POSITION_OFF;
  servoInitialized = true;
  
  delay(500);  // Allow servo to reach position
  
  if (DEBUG_MODE) {
    Serial.println("[SERVO] Initialized at position 0° (shade off)");
  }
}

/**
 * Move shade to specific position smoothly
 * @param targetPosition: Desired angle (0-90)
 */
void moveShade(int targetPosition) {
  if (!servoInitialized) {
    Serial.println("[SERVO] ERROR: Servo not initialized!");
    return;
  }
  
  // Constrain to valid range
  targetPosition = constrain(targetPosition, SHADE_POSITION_OFF, SHADE_POSITION_ON);
  
  // Already at target position
  if (currentPosition == targetPosition) {
    return;
  }
  
  if (DEBUG_MODE) {
    Serial.print("[SERVO] Moving from ");
    Serial.print(currentPosition);
    Serial.print("° to ");
    Serial.print(targetPosition);
    Serial.println("°");
  }
  
  // Move smoothly to target position
  if (currentPosition < targetPosition) {
    // Moving forward (deploying)
    for (int pos = currentPosition; pos <= targetPosition; pos++) {
      shadeServo.write(pos);
      delay(SERVO_SPEED_DELAY_MS);
    }
  } else {
    // Moving backward (retracting)
    for (int pos = currentPosition; pos >= targetPosition; pos--) {
      shadeServo.write(pos);
      delay(SERVO_SPEED_DELAY_MS);
    }
  }
  
  currentPosition = targetPosition;
  
  if (DEBUG_MODE) {
    Serial.print("[SERVO] Moved to ");
    Serial.print(currentPosition);
    Serial.println("°");
  }
}

/**
 * Deploy shade to cover both plants
 */
void deployShade() {
  moveShade(SHADE_POSITION_ON);
}

/**
 * Retract shade away from plants
 */
void retractShade() {
  moveShade(SHADE_POSITION_OFF);
}

/**
 * Get current shade position
 * Returns: Current servo angle (0-90)
 */
int getShadePosition() {
  return currentPosition;
}

/**
 * Check if shade is currently deployed
 * Returns: true if shade is covering plants
 */
bool isShadeDeployed() {
  return currentPosition == SHADE_POSITION_ON;
}
