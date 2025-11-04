#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// PLANT CARE SYSTEM CONFIGURATION
// =============================================================================

// -----------------------------------------------------------------------------
// WiFi Configuration
// -----------------------------------------------------------------------------
#define WIFI_SSID "mesh3"
#define WIFI_PASSWORD "PLDTWIFI@dal7599"
#define WIFI_TIMEOUT_MS 10000
#define WIFI_RETRY_DELAY_MS 1000

// -----------------------------------------------------------------------------
// Weather API Configuration (OpenWeatherMap)
// -----------------------------------------------------------------------------
#define WEATHER_API_HOST "api.openweathermap.org"
#define WEATHER_API_PORT 443
#define WEATHER_API_KEY "0ff8dbec1bf9250f81ce6f628b01a44b"
#define WEATHER_LATITUDE "14.5995"
#define WEATHER_LONGITUDE "120.9842"
#define WEATHER_UPDATE_INTERVAL_MS 300000  // 5 minutes
#define WEATHER_REQUEST_TIMEOUT_MS 10000

// -----------------------------------------------------------------------------
// Notification Configuration (Discord Webhook)
// -----------------------------------------------------------------------------
#define DISCORD_HOST "discord.com"
#define DISCORD_PORT 443
#define DISCORD_WEBHOOK_PATH "/api/webhooks/1419091958651162756/g9U4UFa-8GMlwP2vQdii57b3zd75kdFFurXvV8hgqRrvTRGcuZJJ3jtCN5lWMXxyN0lB"
#define NOTIFICATION_RETRY_COUNT 3
#define NOTIFICATION_RETRY_DELAY_MS 2000

// -----------------------------------------------------------------------------
// Sensor Configuration
// -----------------------------------------------------------------------------
// Soil Moisture Sensors (one per plant)
#define SOIL_MOISTURE_PIN_1 A0
#define SOIL_MOISTURE_PIN_2 A1
#define SOIL_MOISTURE_THRESHOLD_DRY 30     // Below this = dry, needs watering
#define SOIL_MOISTURE_THRESHOLD_WET 70     // Above this = wet
#define SOIL_MOISTURE_THRESHOLD_TARGET 40  // Target level after watering
#define SOIL_MOISTURE_READ_INTERVAL_MS 10000  // Read every 10 seconds

// Lux Sensor (TSL2561 via I2C - shared for both plants)
#define LUX_THRESHOLD_HIGH 15000   // Above this = intense light, shade needed
                                    // Empirically determined from field calibration:
                                    // - Max observed: 20,848 lux (partly cloudy)
                                    // - Threshold set conservatively at 15,000 lux
#define LUX_READ_INTERVAL_MS 10000  // Read every 10 seconds

// Rain Sensor
#define RAIN_SENSOR_PIN A2
#define RAIN_THRESHOLD 500  // Below this value = rain detected
#define RAIN_DEBOUNCE_MS 50

// -----------------------------------------------------------------------------
// Actuator Configuration
// -----------------------------------------------------------------------------
// Water Pump Configuration (shared by both plants)
#define WATER_PUMP_PIN 3
#define WATER_PUMP_MAX_DURATION_MS 30000 // 30 seconds (safety timeout per cycle)

// Solenoid Valve Configuration (one per plant)
#define SOLENOID_VALVE_1_PIN 5  // Plant 1
#define SOLENOID_VALVE_2_PIN 6  // Plant 2

// Servo Motor Configuration (Shading Mechanism - covers both plants)
#define SERVO_PIN 9
#define SERVO_SPEED_DELAY_MS 15          // Delay between steps for smooth movement
#define SHADE_POSITION_OFF 0             // 0° = Shade away (plants exposed)
#define SHADE_POSITION_ON 90             // 90° = Shade covering both plants

// -----------------------------------------------------------------------------
// System Configuration
// -----------------------------------------------------------------------------
#define SERIAL_BAUD_RATE 9600
#define SYSTEM_CHECK_INTERVAL_MS 10000  // 10 seconds
#define DEBUG_MODE true

// -----------------------------------------------------------------------------
// JSON Buffer Sizes
// -----------------------------------------------------------------------------
#define WEATHER_JSON_BUFFER_SIZE 1024
#define NOTIFICATION_JSON_BUFFER_SIZE 256

// -----------------------------------------------------------------------------
// Plant Care Thresholds
// -----------------------------------------------------------------------------
#define CRITICAL_MOISTURE_LEVEL 15  // Critical dry threshold - send alerts
#define OPTIMAL_MOISTURE_MIN 30     // Optimal moisture range minimum
#define OPTIMAL_MOISTURE_MAX 70     // Optimal moisture range maximum

// Temperature Thresholds (from Weather API)
#define TEMP_THRESHOLD_HIGH 35      // Very hot temperature (°C)

// Timing & Intervals
#define MAIN_LOOP_INTERVAL_MS 10000           // Main decision loop every 10 seconds
#define STATUS_UPDATE_INTERVAL_MS 1800000     // Send status update every 30 minutes
#define MIN_TIME_BETWEEN_WATERING_MS 1800000  // Minimum 30 minutes between watering cycles per plant

// Sensor Calibration (adjust based on your sensors)
#define SOIL_SENSOR_DRY_VALUE 1023   // Analog reading in dry air
#define SOIL_SENSOR_WET_VALUE 300    // Analog reading in water

#endif // CONFIG_H


