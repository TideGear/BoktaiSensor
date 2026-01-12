#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_LTR390.h"
#include "config.h"
#include "esp_sleep.h"

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensor settings
Adafruit_LTR390 ltr = Adafruit_LTR390();

// UV Index calculation per LTR-390UV datasheet (DS86-2015-0004)
// Reference: 2300 counts/UVI at 18x gain, 400ms (20-bit) integration
// Formula: UVI = raw × (18/gain) × (400/int_time) / 2300
//
// For 1x gain, 18-bit (100ms):
//   UVI = raw × (18/1) × (400/100) / 2300 = raw × 72 / 2300 ≈ raw / 32
const float UV_DIVISOR = 32.0;  // For 1x gain, 18-bit (100ms) resolution

// Power button state tracking
unsigned long buttonPressStart = 0;
bool buttonWasPressed = false;

// Game selection (0 = Boktai 1, 1 = Boktai 2, 2 = Boktai 3)
int currentGame = 0;

void setup() {
  Serial.begin(115200);

  // Configure power button with internal pull-up (active LOW)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize I2C for the ESP32-S3 Mini pins (GP8 = SDA, GP9 = SCL)
  Wire.begin(8, 9); 

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for (;;);
  }

  if (!ltr.begin()) {
    Serial.println(F("LTR390 failed"));
    for (;;);
  }

  // Configure LTR390 for UV sensing mode
  // Gain 1x: Maximum headroom for direct sunlight (won't saturate)
  // Resolution 18-bit: Good balance of precision and speed (100ms per reading)
  // Note: 13-bit was too fast - only ~4 counts/UVI, causing 0 readings
  ltr.setMode(LTR390_MODE_UVS);
  ltr.setGain(LTR390_GAIN_1);
  ltr.setResolution(LTR390_RESOLUTION_18BIT);
  
  // Debug: Print sensor configuration
  Serial.println(F("LTR390 configured: UV mode, 1x gain, 18-bit (100ms)"));
  Serial.println(F("Expected: ~32 counts per UVI"));

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Check if we woke from deep sleep or cold boot
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  bool coldBoot = (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0);
  
  if (!coldBoot) {
    // Woke from deep sleep via button - start with display OFF
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    
    // Wait for button release (from wake trigger)
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }
  }
  
  // Wait for power-on: show prompt for 10s, reset on button activity
  // Cold boot: show message immediately
  // Wake from sleep: wait for button press to show message
  if (!waitForPowerOn(coldBoot)) {
    // Timed out - go back to sleep
    enterDeepSleep();
  }
  
  // Show wake-up confirmation
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(16, 28);
  display.print("Ojo del Sol ON");
  display.display();
  delay(800);
}

void loop() {
  // Check power button for tap (change game) or long-press (sleep)
  handlePowerButton();

  // Wait for new sensor data
  if (ltr.newDataAvailable()) {
    float uvi = calculateUVI();
    int numBars = GAME_BARS[currentGame];
    int filledBars = getBoktaiBars(uvi, currentGame);

    display.clearDisplay();

    // 1. Game Name (Top Left)
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(GAME_NAMES[currentGame]);

    // 2. Bar Count (Large, Prominent)
    display.setTextSize(3);
    display.setCursor(0, 10);
    display.print(filledBars);
    
    // 3. UV Index (Small, Secondary)
    display.setTextSize(1);
    display.setCursor(30, 18);
    display.print("UV:");
    display.print(uvi, 1);

    // 4. Draw Sun Gauge (8 or 10 segments depending on game)
    drawBoktaiGauge(38, 20, filledBars, numBars);

    display.display();
  }
  delay(100); // Faster loop for responsive button handling
}

// Convert UV Index to Boktai bar count based on selected game
int getBoktaiBars(float uvi, int game) {
  int numBars = GAME_BARS[game];
  
  if (AUTO_MODE) {
    // Auto mode: linearly interpolate between UV_MIN and UV_MAX
    // UV < MIN = 0 bars, UV >= MAX = full bars
    if (uvi < AUTO_UV_MIN) return 0;
    if (uvi >= AUTO_UV_MAX) return numBars;
    
    // Calculate which bar this UV level falls into
    // Bar 1 starts at UV_MIN, Bar N (full) is at UV_MAX
    float range = AUTO_UV_MAX - AUTO_UV_MIN;
    float normalized = (uvi - AUTO_UV_MIN) / range;  // 0.0 to 1.0
    int bars = (int)(normalized * numBars) + 1;      // 1 to numBars
    return constrain(bars, 1, numBars);
  }
  
  // Manual mode: use per-game threshold arrays
  const float* thresholds;
  switch (game) {
    case 0: thresholds = BOKTAI_1_UV; break;
    case 1: thresholds = BOKTAI_2_UV; break;
    case 2: thresholds = BOKTAI_3_UV; break;
    default: thresholds = BOKTAI_1_UV; break;
  }
  
  // Find highest threshold that UV exceeds
  for (int i = numBars - 1; i >= 0; i--) {
    if (uvi >= thresholds[i]) {
      return i + 1;  // Bars are 1-indexed in display
    }
  }
  return 0;  // Below minimum threshold
}

// Handle power button: tap to change game, long-press (3s) to sleep
void handlePowerButton() {
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  
  if (buttonPressed && !buttonWasPressed) {
    // Button just pressed - start timing
    buttonPressStart = millis();
    buttonWasPressed = true;
  } 
  else if (buttonPressed && buttonWasPressed) {
    // Button held - check for long press
    if ((millis() - buttonPressStart) >= LONG_PRESS_MS) {
      enterDeepSleep();
    }
  }
  else if (!buttonPressed && buttonWasPressed) {
    // Button released - check if it was a short press (tap)
    unsigned long pressDuration = millis() - buttonPressStart;
    if (pressDuration >= DEBOUNCE_MS && pressDuration < LONG_PRESS_MS) {
      // Short press: cycle to next game
      currentGame = (currentGame + 1) % NUM_GAMES;
    }
    buttonWasPressed = false;
  }
}

// Wait for power-on with 10-second timeout
// Shows "Hold 3s to power on" message
// Button activity resets the 10-second timer
// coldBoot: if true, show message immediately; if false, wait for button press
// Returns true if 3-second hold completed, false if timed out
bool waitForPowerOn(bool coldBoot) {
  const unsigned long DISPLAY_TIMEOUT_MS = 10000;  // 10 seconds
  unsigned long lastActivity = millis();
  unsigned long pressStart = 0;
  bool wasPressed = false;
  bool displayOn = false;
  
  // On cold boot, show message immediately
  if (coldBoot) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(4, 28);
    display.print("Hold 3s to power on");
    display.display();
    displayOn = true;
  }
  
  while (true) {
    bool pressed = (digitalRead(BUTTON_PIN) == LOW);
    unsigned long now = millis();
    
    // Check for timeout (10 seconds of no button activity)
    if ((now - lastActivity) >= DISPLAY_TIMEOUT_MS) {
      if (displayOn) {
        display.ssd1306_command(SSD1306_DISPLAYOFF);
      }
      return false;  // Timed out - caller should go to sleep
    }
    
    // Button just pressed
    if (pressed && !wasPressed) {
      lastActivity = now;  // Reset timeout
      pressStart = now;
      wasPressed = true;
      
      // Turn on display and show message if not already showing
      if (!displayOn) {
        display.ssd1306_command(SSD1306_DISPLAYON);
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(4, 28);
        display.print("Hold 3s to power on");
        display.display();
        displayOn = true;
      }
    }
    // Button held
    else if (pressed && wasPressed) {
      lastActivity = now;  // Reset timeout while holding
      
      // Check for 3-second hold
      if ((now - pressStart) >= LONG_PRESS_MS) {
        return true;  // Successfully powered on
      }
    }
    // Button released
    else if (!pressed && wasPressed) {
      wasPressed = false;
    }
    
    delay(10);
  }
}

// Enter deep sleep mode with button wake-up
void enterDeepSleep() {
  // Turn off display immediately - no message
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  
  // Wait for button release before sleeping
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  delay(50);  // Debounce
  
  // Configure wake-up source: BUTTON_PIN going LOW (pressed)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  
  Serial.println("Entering deep sleep...");
  Serial.flush();
  
  // Enter deep sleep - device resets on wake
  esp_deep_sleep_start();
}

// Draw Boktai-style segmented sun gauge (8 or 10 segments)
void drawBoktaiGauge(int y, int h, int filledBars, int totalBars) {
  int barWidth = 124;
  int gap = 2;
  int segW = (barWidth - (gap * (totalBars - 1))) / totalBars;

  for (int i = 0; i < totalBars; i++) {
    int x = i * (segW + gap);
    if (i < filledBars) {
      display.fillRect(x, y, segW, h, SSD1306_WHITE); // Filled segment
    } else {
      display.drawRect(x, y, segW, h, SSD1306_WHITE); // Outline segment
    }
  }
}

// Calculate UV Index from raw sensor data
float calculateUVI() {
  uint32_t rawUVS = ltr.readUVS();
  float uvi = (float)rawUVS / UV_DIVISOR;
  
  // Debug output
  Serial.print("UV raw: "); Serial.print(rawUVS);
  Serial.print(" UVI: "); Serial.println(uvi);
  
  return uvi;
}
