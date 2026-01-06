#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_LTR390.h"
#include "config.h"

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensor settings
Adafruit_LTR390 ltr = Adafruit_LTR390();

// Power button state tracking
unsigned long buttonPressStart = 0;
bool buttonWasPressed = false;

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
  ltr.setMode(LTR390_MODE_UVS);
  ltr.setGain(LTR390_GAIN_3);
  ltr.setResolution(LTR390_RESOLUTION_18BIT);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Wait for 3-second button hold to fully power on
  display.setTextSize(1);
  display.setCursor(10, 28);
  display.print("Hold btn 3s to start");
  display.display();
  
  waitForLongPress();
  
  // Show wake-up confirmation
  display.clearDisplay();
  display.setCursor(20, 28);
  display.print("Boktai Sensor ON");
  display.display();
  delay(800);
}

void loop() {
  // Check power button for long-press to sleep
  handlePowerButton();

  // Wait for new sensor data
  if (ltr.newDataAvailable()) {
    float uvi = ltr.readUVI();
    int batPct = getBatteryPercentage();

    display.clearDisplay();
    
    // 1. Draw Battery Indicator (Top Right)
    drawBatteryIcon(105, 2, batPct);

    // 2. Numerical UV Readout
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("UV LEVEL");
    display.setTextSize(2);
    display.setCursor(0, 12);
    display.print(uvi, 1);

    // Calculate Meter Fill Percentage
    float ratio = (uvi - UV_MIN) / (UV_MAX - UV_MIN);
    ratio = constrain(ratio, 0.0, 1.0); // Keep within 0.0 to 1.0

    // 3. Draw 8-Segment Meter
    drawSegmentedMeter(32, 12, ratio, 8);
    
    // 4. Draw 10-Segment Meter
    drawSegmentedMeter(50, 12, ratio, 10);

    display.display();
  }
  delay(100); // Faster loop for responsive button handling
}

// Handle power button: long-press (3s) enters deep sleep
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
  else if (!buttonPressed) {
    buttonWasPressed = false;
  }
}

// Wait for 3-second button press on startup
void waitForLongPress() {
  unsigned long pressStart = 0;
  bool wasPressed = false;
  
  while (true) {
    bool pressed = (digitalRead(BUTTON_PIN) == LOW);
    
    if (pressed && !wasPressed) {
      pressStart = millis();
      wasPressed = true;
    }
    else if (pressed && wasPressed) {
      if ((millis() - pressStart) >= LONG_PRESS_MS) {
        // Wait for button release before continuing
        while (digitalRead(BUTTON_PIN) == LOW) {
          delay(10);
        }
        return;
      }
    }
    else if (!pressed) {
      wasPressed = false;
    }
    delay(10);
  }
}

// Enter deep sleep mode with button wake-up
void enterDeepSleep() {
  // Show sleep message
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(15, 24);
  display.print("Sleeping...");
  display.setCursor(15, 36);
  display.print("Press btn to wake");
  display.display();
  delay(1000);
  
  // Turn off display
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  
  // Configure wake-up source: BUTTON_PIN going LOW (pressed)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  
  Serial.println("Entering deep sleep...");
  Serial.flush();
  
  // Enter deep sleep - device resets on wake
  esp_deep_sleep_start();
}

// Function to draw segmented horizontal bars
void drawSegmentedMeter(int y, int h, float ratio, int totalSegments) {
  int barWidth = 124;
  int gap = 2;
  int segW = (barWidth - (gap * (totalSegments - 1))) / totalSegments;
  int filledCount = ratio * totalSegments;

  for (int i = 0; i < totalSegments; i++) {
    int x = i * (segW + gap);
    if (i < filledCount) {
      display.fillRect(x, y, segW, h, SSD1306_WHITE); // Filled segment
    } else {
      display.drawRect(x, y, segW, h, SSD1306_WHITE); // Outline segment
    }
  }
}

// Function to draw battery percentage and icon
void drawBatteryIcon(int x, int y, int pct) {
  display.setTextSize(1);
  display.setCursor(x - 28, y + 1);
  display.print(pct); display.print("%");
  
  display.drawRect(x, y, 18, 9, SSD1306_WHITE); // Main body
  display.fillRect(x + 18, y + 2, 2, 5, SSD1306_WHITE); // Tip
  
  int fillW = (pct * 14) / 100;
  display.fillRect(x + 2, y + 2, fillW, 5, SSD1306_WHITE);
}

// Calculate battery % based on analog reading
int getBatteryPercentage() {
  float raw = analogRead(BAT_PIN);
  // ADC logic: (Raw / Resolution) * Divider_Correction * Reference_Voltage
  float voltage = (raw / 4095.0) * 2.0 * 3.3 * 1.1; 
  int pct = (voltage - VOLT_MIN) / (VOLT_MAX - VOLT_MIN) * 100;
  return constrain(pct, 0, 100);
}