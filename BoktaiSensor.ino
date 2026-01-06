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

void setup() {
  Serial.begin(115200);

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
}

void loop() {
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
  delay(500);
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