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
// Formula: UVI = raw * (18/gain) * (400/int_time) / 2300
//
// For 18x gain, 20-bit (400ms):
//   UVI = raw / 2300
const float UV_SENSITIVITY_COUNTS_PER_UVI = 2300.0f;
const float UV_REFERENCE_GAIN = 18.0f;
const float UV_REFERENCE_INT_MS = 400.0f;
const float UV_DIVISOR_FALLBACK = 2300.0f;  // Gain 18, 20-bit (400ms)
float uvDivisor = UV_DIVISOR_FALLBACK;
const uint8_t LTR390_MEAS_RATE_500MS = 0x04;

// Power button state tracking
unsigned long buttonPressStart = 0;
bool buttonWasPressed = false;

// Game selection (0 = Boktai 1, 1 = Boktai 2, 2 = Boktai 3)
int currentGame = 0;

// Battery state
unsigned long lastBatterySampleTime = 0;
int cachedBatteryPct = 0;
float cachedBatteryVoltage = -1.0f;
const int BATTERY_PCT_UNKNOWN = -1;
unsigned long lowBatteryStart = 0;

// Screensaver state
const char SCREENSAVER_TEXT[] = "Ojo del Sol";
const bool SCREENSAVER_ENABLED = SCREENSAVER_ACTIVE && (SCREENSAVER_TIME > 0);
const unsigned long SCREENSAVER_TIMEOUT_MS = SCREENSAVER_TIME * 60UL * 1000UL;
const unsigned long SCREENSAVER_MOVE_MS = 60;
const int16_t SCREENSAVER_IMAGE_W = 24;
const int16_t SCREENSAVER_IMAGE_H = 24;
const int16_t SCREENSAVER_IMAGE_TEXT_GAP = 8;
const int16_t SCREENSAVER_TEXT_BATT_GAP = 8;
const int16_t SCREENSAVER_BATT_GAP = 2;
const int16_t SCREENSAVER_BATT_ICON_W = 20;
const int16_t SCREENSAVER_BATT_ICON_H = 9;

const uint8_t OJO_DEL_SOL_BITMAP[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x0C, 0x00, 0x08, 0x18, 0x00,
  0x0C, 0x18, 0x3C, 0x0E, 0x00, 0x70, 0x04, 0x00, 0x20, 0x00, 0x3C, 0x00,
  0x00, 0xDB, 0x00, 0x81, 0x42, 0x80, 0x42, 0x00, 0x40, 0x71, 0x85, 0x8C,
  0x33, 0x81, 0xCE, 0x03, 0x81, 0xC2, 0x01, 0xC3, 0x81, 0x00, 0xFF, 0x00,
  0x00, 0x7E, 0x00, 0x04, 0x00, 0x20, 0x0E, 0x00, 0x70, 0x3C, 0x18, 0x30,
  0x00, 0x18, 0x10, 0x00, 0x30, 0x10, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00
};
unsigned long lastScreenActivityMs = 0;
unsigned long lastScreensaverMoveMs = 0;
bool screensaverActive = false;
bool screensaverJustExited = false;
bool suppressShortPress = false;
int16_t screensaverX = 0;
int16_t screensaverY = 0;
int8_t screensaverDx = 1;
int8_t screensaverDy = 1;
int16_t screensaverTextW = 0;
int16_t screensaverTextH = 0;

// Cached display values
float cachedUvi = 0.0f;
int cachedFilledBars = 0;
int cachedNumBars = 0;
float smoothedUvi = 0.0f;
bool hasSmoothedUvi = false;
bool gameChanged = false;

char screensaverBatteryText[6] = "";
int16_t screensaverBatteryTextW = 0;
int16_t screensaverBatteryTextH = 0;
int16_t screensaverBatteryW = 0;
int16_t screensaverBatteryH = 0;
int16_t screensaverBlockW = 0;
int16_t screensaverBlockH = 0;
bool screensaverBatteryVisible = false;

void setup() {
  Serial.begin(115200);

  // Configure power button with internal pull-up (active LOW)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (GBA_LINK_ENABLED) {
    pinMode(GBA_PIN_SC, OUTPUT);
    pinMode(GBA_PIN_SD, OUTPUT);
    pinMode(GBA_PIN_SI, OUTPUT);
    pinMode(GBA_PIN_SO, OUTPUT);
    digitalWrite(GBA_PIN_SC, LOW);
    digitalWrite(GBA_PIN_SD, LOW);
    digitalWrite(GBA_PIN_SI, LOW);
    digitalWrite(GBA_PIN_SO, LOW);
  }

  if (SENSOR_POWER_ENABLED) {
    pinMode(SENSOR_POWER_PIN, OUTPUT);
    digitalWrite(SENSOR_POWER_PIN, LOW); // keep sensor off until power-on
  }

  if (BATTERY_SENSE_ENABLED) {
    analogSetPinAttenuation(BAT_PIN, ADC_11db);
  } else {
    cachedBatteryPct = BATTERY_PCT_UNKNOWN;
    cachedBatteryVoltage = -1.0f;
  }
  // Initialize I2C for XIAO ESP32S3 pins (D4/GPIO5 = SDA, D5/GPIO6 = SCL)
  Wire.begin(5, 6); 

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  int16_t ssX1, ssY1;
  uint16_t ssW, ssH;
  display.getTextBounds(SCREENSAVER_TEXT, 0, 0, &ssX1, &ssY1, &ssW, &ssH);
  screensaverTextW = (int16_t)ssW;
  screensaverTextH = (int16_t)ssH;
  calculateScreensaverLayout();
  screensaverX = (SCREEN_WIDTH - screensaverBlockW) / 2;
  screensaverY = (SCREEN_HEIGHT - screensaverBlockH) / 2;

  // Check if we woke from deep sleep or cold boot
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  bool coldBoot = (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0);
  
  if (!coldBoot) {
    // Woke from deep sleep via button - start with display OFF
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
  
  // Wait for power-on: show prompt for 10s, reset on button activity
  // Cold boot: show message immediately
  // Wake from sleep: show message on button press/hold
  if (!waitForPowerOn(coldBoot)) {
    // Timed out - go back to sleep
    enterDeepSleep();
  }

  if (SENSOR_POWER_ENABLED) {
    digitalWrite(SENSOR_POWER_PIN, HIGH);
    delay(SENSOR_POWER_STABLE_MS);
  }

  if (!initLTR390()) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(10, 28);
    display.print("LTR390 failed");
    display.display();
    delay(2000);
    enterDeepSleep();
  }

  // Configure LTR390 for UV sensing mode
  // Gain 18x: Datasheet reference setting for UVI sensitivity
  // Resolution 20-bit: Datasheet accuracy setting (~400ms integration)
  // Measurement rate 500ms: Matches datasheet example timing
  ltr.setMode(LTR390_MODE_UVS);
  ltr.setGain(LTR390_GAIN_18);
  ltr.setResolution(LTR390_RESOLUTION_20BIT);
  setMeasurementRate(LTR390_MEAS_RATE_500MS);
  updateUvDivisorFromSensor();
  
  if (DEBUG_SERIAL) {
    Serial.println(F("LTR390 configured: UV mode, 18x gain, 20-bit (~400ms)"));
    Serial.println(F("Expected: ~2300 counts per UVI"));
  }
  
  // Show wake-up confirmation
  display.clearDisplay();
  display.setTextSize(1);
  const char wakeText[] = "Ojo del Sol ON";
  int16_t wakeX1, wakeY1;
  uint16_t wakeW, wakeH;
  display.getTextBounds(wakeText, 0, 0, &wakeX1, &wakeY1, &wakeW, &wakeH);
  const int16_t wakeGap = 8;
  int16_t blockW = SCREENSAVER_IMAGE_W + wakeGap + (int16_t)wakeW;
  int16_t blockH = (SCREENSAVER_IMAGE_H > (int16_t)wakeH) ? SCREENSAVER_IMAGE_H : (int16_t)wakeH;
  int16_t blockX = (SCREEN_WIDTH - blockW) / 2;
  int16_t blockY = (SCREEN_HEIGHT - blockH) / 2;
  int16_t iconX = blockX;
  int16_t iconY = blockY + ((blockH - SCREENSAVER_IMAGE_H) / 2);
  int16_t textX = blockX + SCREENSAVER_IMAGE_W + wakeGap;
  int16_t textY = blockY + ((blockH - (int16_t)wakeH) / 2);
  display.drawBitmap(iconX, iconY, OJO_DEL_SOL_BITMAP, SCREENSAVER_IMAGE_W, SCREENSAVER_IMAGE_H, SSD1306_WHITE);
  display.setCursor(textX - wakeX1, textY - wakeY1);
  display.print(wakeText);
  display.display();
  delay(2000);

  cachedNumBars = GAME_BARS[currentGame];
  lastScreenActivityMs = millis();
}

void loop() {
  // Check power button for tap (change game) or long-press (sleep)
  handlePowerButton();
  updateScreensaverState();

  // Wait for new sensor data
  bool newData = false;
  if (ltr.newDataAvailable()) {
    float uvi = calculateUVI();
    updateBatteryStatus();
    handleLowBatteryCutoff();
    float uviForBars = uvi;
    if (UVI_SMOOTHING_ENABLED) {
      if (!hasSmoothedUvi) {
        smoothedUvi = uvi;
        hasSmoothedUvi = true;
      } else {
        smoothedUvi = (UVI_SMOOTHING_ALPHA * uvi) + ((1.0f - UVI_SMOOTHING_ALPHA) * smoothedUvi);
      }
      uviForBars = smoothedUvi;
    }
    cachedNumBars = GAME_BARS[currentGame];
    if (gameChanged) {
      cachedFilledBars = getBoktaiBars(uviForBars, currentGame);
      gameChanged = false;
    } else {
      cachedFilledBars = getBoktaiBarsWithHysteresis(uviForBars, currentGame, cachedFilledBars);
    }
    cachedUvi = uviForBars;
    updateGbaLinkOutput(cachedFilledBars);
    newData = true;
  }

  if (screensaverActive) {
    drawScreensaver();
  } else if (newData || screensaverJustExited) {
    drawMainDisplay();
  }

  screensaverJustExited = false;
  delay(10); // Short delay for responsive button handling
}

void noteScreenActivity() {
  lastScreenActivityMs = millis();
  if (screensaverActive) {
    screensaverActive = false;
    screensaverJustExited = true;
  }
}

void calculateScreensaverLayout() {
  screensaverBatteryVisible = (cachedBatteryPct >= 0);
  screensaverBatteryText[0] = '\0';
  screensaverBatteryTextW = 0;
  screensaverBatteryTextH = 0;
  screensaverBatteryW = 0;
  screensaverBatteryH = 0;

  display.setTextSize(1);
  if (screensaverBatteryVisible) {
    snprintf(screensaverBatteryText, sizeof(screensaverBatteryText), "%d%%", cachedBatteryPct);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(screensaverBatteryText, 0, 0, &x1, &y1, &w, &h);
    screensaverBatteryTextW = (int16_t)w;
    screensaverBatteryTextH = (int16_t)h;
    screensaverBatteryW = screensaverBatteryTextW + SCREENSAVER_BATT_GAP + SCREENSAVER_BATT_ICON_W;
    screensaverBatteryH = max(screensaverBatteryTextH, SCREENSAVER_BATT_ICON_H);
  }

  screensaverBlockW = screensaverTextW;
  if (SCREENSAVER_IMAGE_W > screensaverBlockW) {
    screensaverBlockW = SCREENSAVER_IMAGE_W;
  }
  if (screensaverBatteryW > screensaverBlockW) {
    screensaverBlockW = screensaverBatteryW;
  }

  screensaverBlockH = SCREENSAVER_IMAGE_H + SCREENSAVER_IMAGE_TEXT_GAP + screensaverTextH;
  if (screensaverBatteryVisible) {
    screensaverBlockH += SCREENSAVER_TEXT_BATT_GAP + screensaverBatteryH;
  }
}

void drawScreensaverBattery(int16_t x, int16_t y) {
  if (!screensaverBatteryVisible) {
    return;
  }

  display.setTextSize(1);
  int16_t textY = y + ((screensaverBatteryH - screensaverBatteryTextH) / 2);
  display.setCursor(x, textY);
  display.print(screensaverBatteryText);

  int16_t iconX = x + screensaverBatteryTextW + SCREENSAVER_BATT_GAP;
  int16_t iconY = y + ((screensaverBatteryH - SCREENSAVER_BATT_ICON_H) / 2);
  display.drawRect(iconX, iconY, 18, 9, SSD1306_WHITE);
  display.fillRect(iconX + 18, iconY + 2, 2, 5, SSD1306_WHITE);

  int fillW = (cachedBatteryPct * 14) / 100;
  display.fillRect(iconX + 2, iconY + 2, fillW, 5, SSD1306_WHITE);
}

void updateScreensaverState() {
  if (!SCREENSAVER_ENABLED) {
    screensaverActive = false;
    return;
  }
  if (screensaverActive) {
    return;
  }

  unsigned long now = millis();
  if ((now - lastScreenActivityMs) >= SCREENSAVER_TIMEOUT_MS) {
    screensaverActive = true;
    screensaverDx = 1;
    screensaverDy = 1;
    calculateScreensaverLayout();
    screensaverX = (SCREEN_WIDTH - screensaverBlockW) / 2;
    screensaverY = (SCREEN_HEIGHT - screensaverBlockH) / 2;
    lastScreensaverMoveMs = 0;
  }
}

void drawScreensaver() {
  if (!SCREENSAVER_ENABLED) {
    return;
  }

  unsigned long now = millis();
  if ((now - lastScreensaverMoveMs) < SCREENSAVER_MOVE_MS) {
    return;
  }
  lastScreensaverMoveMs = now;

  calculateScreensaverLayout();

  int16_t maxX = SCREEN_WIDTH - screensaverBlockW;
  int16_t maxY = SCREEN_HEIGHT - screensaverBlockH;
  if (maxX < 0) {
    maxX = 0;
  }
  if (maxY < 0) {
    maxY = 0;
  }

  screensaverX += screensaverDx;
  screensaverY += screensaverDy;

  if (screensaverX <= 0) {
    screensaverX = 0;
    screensaverDx = 1;
  } else if (screensaverX >= maxX) {
    screensaverX = maxX;
    screensaverDx = -1;
  }

  if (screensaverY <= 0) {
    screensaverY = 0;
    screensaverDy = 1;
  } else if (screensaverY >= maxY) {
    screensaverY = maxY;
    screensaverDy = -1;
  }

  display.clearDisplay();

  int16_t imageX = screensaverX + ((screensaverBlockW - SCREENSAVER_IMAGE_W) / 2);
  int16_t imageY = screensaverY;
  display.drawBitmap(imageX, imageY, OJO_DEL_SOL_BITMAP, SCREENSAVER_IMAGE_W, SCREENSAVER_IMAGE_H, SSD1306_WHITE);

  int16_t textX = screensaverX + ((screensaverBlockW - screensaverTextW) / 2);
  int16_t textY = imageY + SCREENSAVER_IMAGE_H + SCREENSAVER_IMAGE_TEXT_GAP;
  display.setTextSize(1);
  display.setCursor(textX, textY);
  display.print(SCREENSAVER_TEXT);

  if (screensaverBatteryVisible) {
    int16_t batteryX = screensaverX + ((screensaverBlockW - screensaverBatteryW) / 2);
    int16_t batteryY = textY + screensaverTextH + SCREENSAVER_TEXT_BATT_GAP;
    drawScreensaverBattery(batteryX, batteryY);
  }

  display.display();
}

void drawMainDisplay() {
  display.clearDisplay();

  // 1. Draw Battery Indicator (Top Right)
  drawBatteryIcon(105, 2, cachedBatteryPct);

  // 2. Game Name
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(GAME_NAMES[currentGame]);

  // 3. Bar Count (Large, Prominent)
  display.setTextSize(3);
  display.setCursor(0, 10);
  display.print(cachedFilledBars);

  // 4. UV Index (Small, Secondary)
  display.setTextSize(1);
  display.setCursor(64, 18);
  display.print("UV:");
  display.print(cachedUvi, 1);

  // 5. Draw Sun Gauge (8 or 10 segments depending on game)
  drawBoktaiGauge(38, 20, cachedFilledBars, cachedNumBars);

  display.display();
}

void updateGbaLinkOutput(int bars) {
  if (!GBA_LINK_ENABLED) {
    return;
  }

  uint8_t value = (uint8_t)constrain(bars, 0, 15);
  digitalWrite(GBA_PIN_SC, (value & 0x01) ? HIGH : LOW);
  digitalWrite(GBA_PIN_SD, (value & 0x02) ? HIGH : LOW);
  digitalWrite(GBA_PIN_SI, (value & 0x04) ? HIGH : LOW);
  digitalWrite(GBA_PIN_SO, (value & 0x08) ? HIGH : LOW);
}

void writeLtr390Register(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LTR390_I2CADDR_DEFAULT);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void setMeasurementRate(uint8_t rate) {
  uint8_t resBits = (uint8_t)ltr.getResolution();
  uint8_t regValue = (uint8_t)((resBits << 4) | (rate & 0x07));
  writeLtr390Register(LTR390_MEAS_RATE, regValue);
}

float gainToFactor(ltr390_gain_t gain) {
  switch (gain) {
    case LTR390_GAIN_1: return 1.0f;
    case LTR390_GAIN_3: return 3.0f;
    case LTR390_GAIN_6: return 6.0f;
    case LTR390_GAIN_9: return 9.0f;
    case LTR390_GAIN_18: return 18.0f;
    default: return 1.0f;
  }
}

float resolutionToIntegrationMs(ltr390_resolution_t res) {
  switch (res) {
    case LTR390_RESOLUTION_20BIT: return 400.0f;
    case LTR390_RESOLUTION_19BIT: return 200.0f;
    case LTR390_RESOLUTION_18BIT: return 100.0f;
    case LTR390_RESOLUTION_17BIT: return 50.0f;
    case LTR390_RESOLUTION_16BIT: return 25.0f;
    case LTR390_RESOLUTION_13BIT: return 12.5f;
    default: return 100.0f;
  }
}

void updateUvDivisorFromSensor() {
  float gainFactor = gainToFactor(ltr.getGain());
  float intMs = resolutionToIntegrationMs(ltr.getResolution());

  if (gainFactor <= 0.0f || intMs <= 0.0f) {
    uvDivisor = UV_DIVISOR_FALLBACK;
  } else {
    uvDivisor = (UV_SENSITIVITY_COUNTS_PER_UVI * gainFactor * intMs) /
                (UV_REFERENCE_GAIN * UV_REFERENCE_INT_MS);
  }

  if (DEBUG_SERIAL) {
    Serial.print(F("UV divisor: "));
    Serial.println(uvDivisor, 4);
  }
}

float getAutoThreshold(int numBars, int barIndex) {
  if (AUTO_UV_MAX <= AUTO_UV_MIN) {
    return AUTO_UV_MIN;
  }
  if (barIndex < 1) {
    barIndex = 1;
  }
  if (barIndex > numBars) {
    barIndex = numBars;
  }
  float range = AUTO_UV_MAX - AUTO_UV_MIN;
  return AUTO_UV_MIN + (range * (barIndex - 1)) / numBars;
}

const float* getManualThresholds(int game) {
  switch (game) {
    case 0: return BOKTAI_1_UV;
    case 1: return BOKTAI_2_UV;
    case 2: return BOKTAI_3_UV;
    default: return BOKTAI_1_UV;
  }
}

float getBarThreshold(int game, int barIndex) {
  int numBars = GAME_BARS[game];
  if (AUTO_MODE) {
    return getAutoThreshold(numBars, barIndex);
  }
  const float* thresholds = getManualThresholds(game);
  if (barIndex < 1) {
    barIndex = 1;
  }
  if (barIndex > numBars) {
    barIndex = numBars;
  }
  return thresholds[barIndex - 1];
}

int getBoktaiBarsWithHysteresis(float uvi, int game, int lastBars) {
  int numBars = GAME_BARS[game];
  int target = getBoktaiBars(uvi, game);

  if (BAR_HYSTERESIS <= 0.0f || (AUTO_MODE && AUTO_UV_MAX <= AUTO_UV_MIN)) {
    return target;
  }

  if (lastBars < 0) {
    lastBars = 0;
  }
  if (lastBars > numBars) {
    lastBars = numBars;
  }

  if (target > lastBars) {
    float upThreshold = getBarThreshold(game, lastBars + 1);
    if (uvi >= (upThreshold + BAR_HYSTERESIS)) {
      return target;
    }
    return lastBars;
  }

  if (target < lastBars) {
    if (lastBars <= 0) {
      return lastBars;
    }
    float downThreshold = getBarThreshold(game, lastBars);
    if (uvi < (downThreshold - BAR_HYSTERESIS)) {
      return target;
    }
    return lastBars;
  }

  return lastBars;
}

// Convert UV Index to Boktai bar count based on selected game
int getBoktaiBars(float uvi, int game) {
  int numBars = GAME_BARS[game];
  
  if (AUTO_MODE) {
    // Auto mode: linearly interpolate between UV_MIN and UV_MAX
    // UV < MIN = 0 bars, UV >= MAX = full bars
    if (AUTO_UV_MAX <= AUTO_UV_MIN) {
      return (uvi >= AUTO_UV_MIN) ? numBars : 0;
    }
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
    if (screensaverActive) {
      suppressShortPress = true;
    }
    noteScreenActivity();
  } 
  else if (buttonPressed && buttonWasPressed) {
    // Button held - check for long press
    noteScreenActivity();
    if ((millis() - buttonPressStart) >= LONG_PRESS_MS) {
      enterDeepSleep();
    }
  }
  else if (!buttonPressed && buttonWasPressed) {
    // Button released - check if it was a short press (tap)
    unsigned long pressDuration = millis() - buttonPressStart;
    if (pressDuration >= DEBOUNCE_MS && pressDuration < LONG_PRESS_MS) {
      // Short press: cycle to next game
      if (!suppressShortPress) {
        currentGame = (currentGame + 1) % NUM_GAMES;
        gameChanged = true;
      }
    }
    suppressShortPress = false;
    buttonWasPressed = false;
  }
}

// Wait for power-on with 10-second timeout
// Shows "Hold 3s to power on" message
// Button activity resets the 10-second timer
// coldBoot: if true, show message immediately; if false, show on button press/hold
// Returns true if 3-second hold completed, false if timed out
bool waitForPowerOn(bool coldBoot) {
  const unsigned long DISPLAY_TIMEOUT_MS = 10000;  // 10 seconds
  unsigned long lastActivity = millis();
  unsigned long pressStart = 0;
  bool wasPressed = false;
  bool displayOn = false;
  bool pressedAtStart = (!coldBoot && (digitalRead(BUTTON_PIN) == LOW));
  
  // On cold boot, show message immediately
  if (coldBoot || pressedAtStart) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(4, 28);
    display.print("Hold 3s to power on");
    display.display();
    displayOn = true;
  }
  if (pressedAtStart) {
    pressStart = lastActivity;
    wasPressed = true;
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
  display.ssd1306_command(0x8D); // Charge pump
  display.ssd1306_command(0x10); // Disable charge pump
  // Release I2C lines to reduce back-powering during sleep.
  Wire.end();
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  if (GBA_LINK_ENABLED) {
    pinMode(GBA_PIN_SC, INPUT);
    pinMode(GBA_PIN_SD, INPUT);
    pinMode(GBA_PIN_SI, INPUT);
    pinMode(GBA_PIN_SO, INPUT);
  }

  if (SENSOR_POWER_ENABLED) {
    digitalWrite(SENSOR_POWER_PIN, LOW);
  }
  
  // Wait for button release before sleeping
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  delay(50);  // Debounce
  
  // Configure wake-up source: BUTTON_PIN going LOW (pressed)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  
  if (DEBUG_SERIAL) {
    Serial.println("Entering deep sleep...");
    Serial.flush();
  }
  
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

// Function to draw battery percentage and icon
void drawBatteryIcon(int x, int y, int pct) {
  if (pct < 0) {
    return;
  }

  display.setTextSize(1);
  display.setCursor(x - 28, y + 1);
  
  // Normal battery display
  display.print(pct); display.print("%");
  
  display.drawRect(x, y, 18, 9, SSD1306_WHITE); // Main body
  display.fillRect(x + 18, y + 2, 2, 5, SSD1306_WHITE); // Tip
  
  int fillW = (pct * 14) / 100;
  display.fillRect(x + 2, y + 2, fillW, 5, SSD1306_WHITE);
}

// Calculate battery % based on analog reading
void updateBatteryStatus() {
  unsigned long now = millis();
  if ((now - lastBatterySampleTime) < BATTERY_SAMPLE_MS) {
    return;
  }
  lastBatterySampleTime = now;

  if (BATTERY_SENSE_ENABLED) {
    cachedBatteryPct = readBatteryPercentage();
  } else {
    cachedBatteryPct = BATTERY_PCT_UNKNOWN;
    cachedBatteryVoltage = -1.0f;
    lowBatteryStart = 0;
  }
}

void handleLowBatteryCutoff() {
  if (!BATTERY_SENSE_ENABLED || !BATTERY_CUTOFF_ENABLED) {
    lowBatteryStart = 0;
    return;
  }
  if (cachedBatteryVoltage <= 0.0f) {
    return;
  }

  unsigned long now = millis();
  if (cachedBatteryVoltage <= VOLT_CUTOFF) {
    if (lowBatteryStart == 0) {
      lowBatteryStart = now;
    }
    if ((now - lowBatteryStart) >= BATTERY_CUTOFF_HOLD_MS) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(31, 28);
      display.print("LOW BATTERY");
      display.display();
      delay(1500);
      enterDeepSleep();
    }
  } else {
    lowBatteryStart = 0;
  }
}

int readBatteryPercentage() {
  // Read multiple samples and average for stability
  uint32_t sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(BAT_PIN);
    delay(1);
  }
  float raw = sum / 10.0;
  
  // ESP32-S3 ADC: 12-bit (0-4095), default ~0-3.3V range
  // VOLT_DIVIDER_MULT compensates for voltage divider ratio + ADC variance
  // Calibrate in config.h if battery % is wrong at full charge
  float voltage = (raw / 4095.0) * 3.3 * VOLT_DIVIDER_MULT;
  cachedBatteryVoltage = voltage;
  
  // Debug output
  if (DEBUG_SERIAL) {
    Serial.print("Battery ADC raw: "); Serial.print(raw);
    Serial.print(" Voltage: "); Serial.println(voltage);
  }

  int pct = ((voltage - VOLT_MIN) / (VOLT_MAX - VOLT_MIN)) * 100;
  return constrain(pct, 0, 100);
}

bool initLTR390() {
  const int maxAttempts = 3;

  for (int attempt = 0; attempt < maxAttempts; attempt++) {
    if (ltr.begin()) {
      return true;
    }

    if (DEBUG_SERIAL) {
      Serial.print("LTR390 init failed (");
      Serial.print(attempt + 1);
      Serial.println(")");
    }

    if (SENSOR_POWER_ENABLED) {
      digitalWrite(SENSOR_POWER_PIN, LOW);
      delay(50);
      digitalWrite(SENSOR_POWER_PIN, HIGH);
      delay(SENSOR_POWER_STABLE_MS);
    } else {
      delay(50);
    }
  }

  return false;
}

// Calculate UV Index from raw sensor data
float calculateUVI() {
  uint32_t rawUVS = ltr.readUVS();
  float divisor = (uvDivisor > 0.0f) ? uvDivisor : UV_DIVISOR_FALLBACK;
  float uvi = (float)rawUVS / divisor;
  
  // Debug output
  if (DEBUG_SERIAL) {
    Serial.print("UV raw: "); Serial.print(rawUVS);
    Serial.print(" UVI: "); Serial.println(uvi);
  }
  
  return uvi;
}
