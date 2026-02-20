#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_LTR390.h"
#include "config.h"
#include "esp_sleep.h"
#include <BleCompositeHID.h>
#include <XboxGamepadDevice.h>
#include <XboxGamepadConfiguration.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool displayInitialized = false;
// Icon dimensions (shared by status bar and screensaver)
const int16_t STATUS_BATT_ICON_W = 20;
const int16_t STATUS_BATT_ICON_H = 9;
const int16_t STATUS_BT_ICON_W = 8;
const int16_t STATUS_BT_ICON_H = 10;
const int16_t STATUS_BT_GAP = 2;
const int16_t STATUS_TEXT_GAP = 2;
const int16_t STATUS_RIGHT_MARGIN = 3;
const uint8_t SSD1306_CHARGEPUMP_DISABLE = 0x10;
const uint8_t SSD1306_CHARGEPUMP_ENABLE = 0x14;

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
float uvDivisor = UV_SENSITIVITY_COUNTS_PER_UVI;
const uint8_t MEAS_RATE_500MS = 0x04;

// Power button state tracking
unsigned long buttonPressStart = 0;
bool buttonWasPressed = false;

// Game selection (0 = Boktai 1, 1 = Boktai 2, 2 = Boktai 3)
int currentGame = 0;

// UI screen selection:
// 0..NUM_GAMES-1 = game screens, NUM_GAMES = DEBUG screen (if enabled)
int currentScreen = 0;
bool uiScreenChanged = false;

// Battery state
unsigned long lastBatterySampleTime = 0;
const int BATTERY_PCT_UNKNOWN = -1;
int cachedBatteryPct = BATTERY_PCT_UNKNOWN;
float cachedBatteryVoltage = -1.0f;
float cachedBatteryAdcAvg = -1.0f;
bool hasBatteryReading = false;
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
uint32_t cachedRawUVS = 0;
float cachedUviRaw = 0.0f;
float cachedUviCorrected = 0.0f;
float cachedUvi = 0.0f;
int cachedFilledBars = 0;
int cachedNumBars = 0;
float smoothedUvi = 0.0f;
bool hasSmoothedUvi = false;
bool gameChanged = false;
bool bleGameChanged = false;
bool gbaFramePhaseHigh = false;
unsigned long gbaFrameLastToggleMs = 0;

// BLE state
XboxGamepadDevice* xboxGamepad = nullptr;
XboxSeriesXControllerDeviceConfiguration* xboxConfig = nullptr;
BleCompositeHID compositeHID(BLE_DEVICE_NAME, BLE_MANUFACTURER, 100);  // Initial battery level; updated once battery is read
unsigned long blePressIntervalMs = 0;  // Set by initBluetooth()
unsigned long blePressHoldMs = 0;      // Set by initBluetooth()
bool bleConnected = false;
bool blePairingActive = false;
unsigned long blePairingStartMs = 0;
bool bleIconFlashOn = true;
unsigned long bleIconLastToggleMs = 0;
bool bleSyncPending = false;
unsigned long bleLastResyncMs = 0;
int bleDeviceBars = 0;
int bleDeviceNumBars = 0;
bool bleEstimateValid = false;
int bleEstimatedSteps = 0;
bool blePressHolding = false;
unsigned long blePressStartMs = 0;
unsigned long bleLastPressMs = 0;
int blePressDirection = 0;
uint16_t bleActiveButton = 0;
int bleSyncTargetSteps = 0;
int bleSyncStepsMax = 0;
int bleSyncRemaining = 0;
int bleSyncDirection = 0;
int bleRefillRemaining = 0;
int bleRefillDirection = 0;
const unsigned long BLE_ICON_FLASH_MS = 500;
const int BLE_DIR_DEC = -1;
const int BLE_DIR_INC = 1;
const int BOKTAI1_STEP_COUNT = 10;
const int BOKTAI1_STEP_TO_BAR[BOKTAI1_STEP_COUNT + 1] = { 0, 1, 2, 3, 3, 4, 5, 6, 7, 7, 8 };
const int BOKTAI1_BAR_TO_STEP_FROM_EMPTY[9] = { 0, 1, 2, 3, 5, 6, 7, 8, 10 };
const int BOKTAI1_BAR_TO_STEP_FROM_FULL[9] = { 0, 1, 2, 4, 5, 6, 7, 9, 10 };
enum BleSyncPhase {
  BLE_SYNC_NONE = 0,
  BLE_SYNC_CLAMP,
  BLE_SYNC_REFILL
};
BleSyncPhase bleSyncPhase = BLE_SYNC_NONE;
// Single Analog mode state
int bleSingleAnalogBars = -1;
int16_t bleSingleAnalogValue = 0;
bool bleSingleAnalogActive = false;
bool bleSingleAnalogButtonHeld = false;
unsigned long bleSingleAnalogLastRefreshMs = 0;

char screensaverBatteryText[6] = "";
int16_t screensaverBatteryTextW = 0;
int16_t screensaverBatteryTextH = 0;
int16_t screensaverBatteryW = 0;
int16_t screensaverBatteryH = 0;
int16_t screensaverBlockW = 0;
int16_t screensaverBlockH = 0;
bool screensaverBatteryVisible = false;
int screensaverLastLayoutPct = -2;  // Cache key for layout recalculation (-2 = never calculated)
bool screensaverLastLayoutBleStatus = false;

void wakeDisplayHardware() {
  // Sleep path disables the charge pump; explicitly restore it before drawing.
  display.ssd1306_command(SSD1306_CHARGEPUMP);
  display.ssd1306_command(SSD1306_CHARGEPUMP_ENABLE);
  display.ssd1306_command(SSD1306_DISPLAYON);
  delay(2);  // SSD1306 charge pump stabilization
}

void showHoldPowerOnPrompt() {
  wakeDisplayHardware();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(4, 28);
  display.print("Hold 3s to power on");
  display.display();
}

void setup() {
  Serial.begin(115200);

  // Configure power button with internal pull-up (active LOW)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (GBA_LINK_ENABLED) {
    pinMode(GBA_PIN_SC, OUTPUT);
    pinMode(GBA_PIN_SD, OUTPUT);
    pinMode(GBA_PIN_SO, OUTPUT);
    digitalWrite(GBA_PIN_SC, LOW);
    digitalWrite(GBA_PIN_SD, LOW);
    digitalWrite(GBA_PIN_SO, LOW);
  }

  if (SENSOR_POWER_ENABLED) {
    pinMode(SENSOR_POWER_PIN, OUTPUT);
    digitalWrite(SENSOR_POWER_PIN, LOW); // keep sensor off until power-on
  }

  if (BATTERY_SENSE_ENABLED) {
    analogSetPinAttenuation(BAT_PIN, ADC_11db);
  }
  // Initialize I2C for XIAO ESP32S3 pins (D4/GPIO5 = SDA, D5/GPIO6 = SCL)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_I2C_ADDR)) {
    Serial.println("SSD1306 failed");
    delay(2000);
    enterDeepSleep();
  }
  displayInitialized = true;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  int16_t ssX1, ssY1;
  uint16_t ssW, ssH;
  display.getTextBounds(SCREENSAVER_TEXT, 0, 0, &ssX1, &ssY1, &ssW, &ssH);
  screensaverTextW = (int16_t)ssW;
  screensaverTextH = (int16_t)ssH;
  calculateScreensaverLayout();
  screensaverLastLayoutPct = cachedBatteryPct;
  screensaverLastLayoutBleStatus = BLUETOOTH_ENABLED && (bleConnected || blePairingActive);
  screensaverX = (SCREEN_WIDTH - screensaverBlockW) / 2;
  screensaverY = (SCREEN_HEIGHT - screensaverBlockH) / 2;

  // Wait for power-on: show prompt for 10s, reset on button activity.
  // This always shows immediately so a short wake tap reliably shows the prompt.
  if (!waitForPowerOn()) {
    // Timed out - go back to sleep
    enterDeepSleep();
  }
  wakeDisplayHardware();

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
  setMeasurementRate(MEAS_RATE_500MS);
  updateUvDivisorFromSensor();
  
  if (DEBUG_SERIAL) {
    Serial.println("LTR390 configured: UV mode, 18x gain, 20-bit (~400ms)");
    Serial.println("Expected: ~2300 counts per UVI");
    if (UV_ENCLOSURE_COMP_ENABLED) {
      Serial.print("UV enclosure compensation: ON (T=");
      Serial.print(UV_ENCLOSURE_TRANSMITTANCE, 4);
      Serial.print(", offset=");
      Serial.print(UV_ENCLOSURE_UVI_OFFSET, 4);
      Serial.println(")");
    } else {
      Serial.println("UV enclosure compensation: OFF");
    }
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
  initBluetooth();
}

void loop() {
  // Check power button for tap (change game) or long-press (sleep)
  handlePowerButton();
  updateScreensaverState();
  updateBluetoothState();
  updateBatteryStatus();
  handleLowBatteryCutoff();

  // Wait for new sensor data
  bool newData = false;
  if (ltr.newDataAvailable()) {
    float uvi = calculateUVI();
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
    updateBluetoothMeter(cachedFilledBars, cachedNumBars);
    newData = true;
  }

  updateGbaLinkOutput(cachedFilledBars);

  if (screensaverActive) {
    drawScreensaver();
  } else if (newData || screensaverJustExited || uiScreenChanged) {
    drawMainDisplay();
    uiScreenChanged = false;
  }

  screensaverJustExited = false;
  handleBlePresses();
  refreshSingleAnalogButton();
  delay(10); // Short delay for responsive button handling
}

void noteScreenActivity() {
  lastScreenActivityMs = millis();
  if (screensaverActive) {
    screensaverActive = false;
    screensaverJustExited = true;
  }
}

bool shouldShowScreensaverBluetoothStatus() {
  return BLUETOOTH_ENABLED && (bleConnected || blePairingActive);
}

void calculateScreensaverLayout() {
  bool bluetoothStatusVisible = shouldShowScreensaverBluetoothStatus();
  screensaverBatteryVisible = (cachedBatteryPct >= 0) || bluetoothStatusVisible;
  screensaverBatteryText[0] = '\0';
  screensaverBatteryTextW = 0;
  screensaverBatteryTextH = 0;
  screensaverBatteryW = 0;
  screensaverBatteryH = 0;

  display.setTextSize(1);
  if (screensaverBatteryVisible) {
    bool hasBattery = (cachedBatteryPct >= 0);
    if (hasBattery) {
      snprintf(screensaverBatteryText, sizeof(screensaverBatteryText), "%d%%", cachedBatteryPct);
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds(screensaverBatteryText, 0, 0, &x1, &y1, &w, &h);
      screensaverBatteryTextW = (int16_t)w;
      screensaverBatteryTextH = (int16_t)h;
    }

    int16_t bluetoothW = 0;
    if (bluetoothStatusVisible) {
      bluetoothW = STATUS_BT_ICON_W;
    }

    if (hasBattery) {
      screensaverBatteryW = screensaverBatteryTextW + SCREENSAVER_BATT_GAP + STATUS_BATT_ICON_W;
      if (bluetoothStatusVisible) {
        screensaverBatteryW += bluetoothW + STATUS_BT_GAP;
      }
      screensaverBatteryH = max(screensaverBatteryTextH, STATUS_BATT_ICON_H);
    } else {
      screensaverBatteryW = bluetoothW;
      screensaverBatteryH = bluetoothW > 0 ? STATUS_BT_ICON_H : 0;
    }

    if (bluetoothStatusVisible) {
      screensaverBatteryH = max(screensaverBatteryH, STATUS_BT_ICON_H);
    }
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

  bool hasBattery = (cachedBatteryPct >= 0);
  bool bluetoothStatusVisible = shouldShowScreensaverBluetoothStatus();
  int16_t cursorX = x;

  display.setTextSize(1);
  if (bluetoothStatusVisible) {
    int16_t btY = y + ((screensaverBatteryH - STATUS_BT_ICON_H) / 2);
    drawBluetoothIcon(cursorX, btY, isBluetoothIconOn());
    cursorX += STATUS_BT_ICON_W;
    if (hasBattery) {
      cursorX += STATUS_BT_GAP;
    }
  }
  if (hasBattery) {
    int16_t textY = y + ((screensaverBatteryH - screensaverBatteryTextH) / 2);
    display.setCursor(cursorX, textY);
    display.print(screensaverBatteryText);
    cursorX += screensaverBatteryTextW + SCREENSAVER_BATT_GAP;

    int16_t iconY = y + ((screensaverBatteryH - STATUS_BATT_ICON_H) / 2);
    drawBatteryGauge(cursorX, iconY, cachedBatteryPct);
  }
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
    screensaverLastLayoutPct = cachedBatteryPct;
    screensaverLastLayoutBleStatus = shouldShowScreensaverBluetoothStatus();
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

  bool bluetoothStatusVisible = shouldShowScreensaverBluetoothStatus();
  if (screensaverLastLayoutPct != cachedBatteryPct ||
      screensaverLastLayoutBleStatus != bluetoothStatusVisible) {
    calculateScreensaverLayout();
    screensaverLastLayoutPct = cachedBatteryPct;
    screensaverLastLayoutBleStatus = bluetoothStatusVisible;
  }

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

void drawDebugDisplay() {
  display.clearDisplay();

  // Draw Status Icons (Top Right)
  drawStatusIcons();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("DEBUG");

  // UV readings
  display.setCursor(0, 10);
  display.print("UV raw:");
  display.print(cachedRawUVS);

  display.setCursor(0, 20);
  display.print("UVI:");
  display.print(cachedUviRaw, 3);

  display.setCursor(0, 30);
  display.print("UVI comp'ed:");
  display.print(cachedUviCorrected, 3);

  // Battery readings
  bool haveBatteryReading = BATTERY_SENSE_ENABLED && hasBatteryReading;

  display.setCursor(0, 42);
  display.print("ADC avg:");
  if (haveBatteryReading) {
    display.print(cachedBatteryAdcAvg, 0);
  } else {
    display.print("N/A");
  }

  display.setCursor(0, 52);
  display.print("Batt V:");
  if (haveBatteryReading) {
    display.print(cachedBatteryVoltage, 2);
  } else {
    display.print("N/A");
  }

  display.display();
}

void drawMainDisplay() {
  if (DEBUG_SCREEN_ENABLED && currentScreen == NUM_GAMES) {
    drawDebugDisplay();
    return;
  }

  display.clearDisplay();

  // 1. Draw Status Icons (Top Right)
  drawStatusIcons();

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
  display.print(cachedUvi, 3);

  // 5. Draw Sun Gauge (8 or 10 segments depending on game)
  drawBoktaiGauge(38, 20, cachedFilledBars, cachedNumBars);

  display.display();
}

void updateGbaLinkOutput(int bars) {
  if (!GBA_LINK_ENABLED) {
    return;
  }

  uint8_t value = (uint8_t)constrain(bars, 0, 15);  // 4-bit bar value encoded over two phases

  // Framed 3-wire mode:
  // - SC carries frame phase (0 = low pair, 1 = high pair)
  // - SD/SO carry two data bits for that phase
  // - SI is unused (wired to ground externally)
  unsigned long now = millis();
  if ((gbaFrameLastToggleMs == 0) || ((now - gbaFrameLastToggleMs) >= GBA_LINK_FRAME_TOGGLE_MS)) {
    gbaFramePhaseHigh = !gbaFramePhaseHigh;
    gbaFrameLastToggleMs = now;
  }

  uint8_t pair = gbaFramePhaseHigh ? ((value >> 2) & 0x03) : (value & 0x03);
  bool soBit = (pair & 0x01) != 0;
  bool sdBit = (pair & 0x02) != 0;

  digitalWrite(GBA_PIN_SC, gbaFramePhaseHigh ? HIGH : LOW);
  digitalWrite(GBA_PIN_SD, sdBit ? HIGH : LOW);
  digitalWrite(GBA_PIN_SO, soBit ? HIGH : LOW);
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
    uvDivisor = UV_SENSITIVITY_COUNTS_PER_UVI;
  } else {
    uvDivisor = (UV_SENSITIVITY_COUNTS_PER_UVI * gainFactor * intMs) /
                (UV_REFERENCE_GAIN * UV_REFERENCE_INT_MS);
  }

  if (DEBUG_SERIAL) {
    Serial.print("UV divisor: ");
    Serial.println(uvDivisor, 4);
  }
}

float applyEnclosureCompensation(float measuredUvi) {
  if (!UV_ENCLOSURE_COMP_ENABLED) {
    return measuredUvi;
  }
  if (UV_ENCLOSURE_TRANSMITTANCE <= 0.0f) {
    return measuredUvi;
  }

  float corrected = (measuredUvi - UV_ENCLOSURE_UVI_OFFSET) / UV_ENCLOSURE_TRANSMITTANCE;
  if (corrected < 0.0f) {
    corrected = 0.0f;
  }
  return corrected;
}

float getAutoThreshold(int numBars, int barIndex) {
  if (AUTO_UV_SATURATION <= AUTO_UV_MIN) {
    return AUTO_UV_MIN;
  }
  if (barIndex < 1) {
    barIndex = 1;
  }
  if (barIndex > numBars) {
    barIndex = numBars;
  }
  float range = AUTO_UV_SATURATION - AUTO_UV_MIN;
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

  if (!BAR_HYSTERESIS_ENABLED || BAR_HYSTERESIS <= 0.0f || (AUTO_MODE && AUTO_UV_SATURATION <= AUTO_UV_MIN)) {
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
    // lastBars > 0 guaranteed: clamped to [0, numBars] above and target < lastBars
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
    // Auto mode: linearly interpolate between UV_MIN and UV_SATURATION
    // UV < MIN = 0 bars, UV >= SATURATION = full bars
    if (AUTO_UV_SATURATION <= AUTO_UV_MIN) {
      return (uvi >= AUTO_UV_MIN) ? numBars : 0;
    }
    if (uvi < AUTO_UV_MIN) return 0;
    if (uvi >= AUTO_UV_SATURATION) return numBars;
    
    // Calculate which bar this UV level falls into
    // Bar 1 starts at UV_MIN; highest bar can begin before UV_SATURATION.
    float range = AUTO_UV_SATURATION - AUTO_UV_MIN;
    float normalized = (uvi - AUTO_UV_MIN) / range;  // 0.0 to 1.0
    int bars = (int)(normalized * numBars) + 1;      // 1 to numBars
    return constrain(bars, 1, numBars);
  }
  
  // Manual mode: use per-game threshold arrays
  const float* thresholds = getManualThresholds(game);
  
  // Find highest threshold that UV exceeds
  for (int i = numBars - 1; i >= 0; i--) {
    if (uvi >= thresholds[i]) {
      return i + 1;  // Bars are 1-indexed in display
    }
  }
  return 0;  // Below minimum threshold
}

// Handle power button: tap to cycle screens, long-press (3s) to sleep
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
      // Short press: cycle to next screen (games + optional DEBUG)
      if (!suppressShortPress) {
        int numScreens = NUM_GAMES + (DEBUG_SCREEN_ENABLED ? 1 : 0);
        currentScreen = (currentScreen + 1) % numScreens;
        uiScreenChanged = true;

        if (currentScreen < NUM_GAMES) {
          currentGame = currentScreen;
          gameChanged = true;
          if (BLUETOOTH_ENABLED) {
            bleGameChanged = true;
          }
        }
      }
    }
    suppressShortPress = false;
    buttonWasPressed = false;
  }
}

// Wait for power-on with 10-second timeout.
// Always shows "Hold 3s to power on" immediately so wake taps are visible.
// Button activity resets the 10-second timer.
// Returns true if 3-second hold completed, false if timed out.
bool waitForPowerOn() {
  const unsigned long DISPLAY_TIMEOUT_MS = 10000;  // 10 seconds
  unsigned long lastActivity = millis();
  unsigned long lastPromptRefresh = lastActivity;
  unsigned long pressStart = 0;
  bool wasPressed = false;
  bool pressedAtStart = (digitalRead(BUTTON_PIN) == LOW);
  
  showHoldPowerOnPrompt();
  if (pressedAtStart) {
    pressStart = lastActivity;
    wasPressed = true;
  }
  
  while (true) {
    bool pressed = (digitalRead(BUTTON_PIN) == LOW);
    unsigned long now = millis();
    
    // Check for timeout (10 seconds of no button activity)
    if ((now - lastActivity) >= DISPLAY_TIMEOUT_MS) {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      return false;  // Timed out - caller should go to sleep
    }
    
    // Button just pressed
    if (pressed && !wasPressed) {
      lastActivity = now;  // Reset timeout
      pressStart = now;
      wasPressed = true;
      
      // Refresh prompt on each press.
      showHoldPowerOnPrompt();
      lastPromptRefresh = now;
    }
    // Button held
    else if (pressed && wasPressed) {
      lastActivity = now;  // Reset timeout while holding
      // Retry prompt draw periodically while held in case the first draw was missed.
      if ((now - lastPromptRefresh) >= 750UL) {
        showHoldPowerOnPrompt();
        lastPromptRefresh = now;
      }
      
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
  // Shut down Bluetooth first - this is critical for low-power sleep
  if (BLUETOOTH_ENABLED) {
    // Release any held buttons/sticks
    if (xboxGamepad != nullptr) {
      if (bleActiveButton != 0) {
        xboxGamepad->release(bleActiveButton);
      }
      if (BLE_METER_UNLOCK_BUTTON_ENABLED && bleSingleAnalogButtonHeld) {
        xboxGamepad->release(BLE_METER_UNLOCK_BUTTON);
      }
      xboxGamepad->release(XBOX_BUTTON_LS);
      xboxGamepad->release(XBOX_BUTTON_RS);
      xboxGamepad->setLeftThumb(0, 0);
      xboxGamepad->setRightThumb(0, 0);
      xboxGamepad->sendGamepadReport();
    }
    // Only touch NimBLE if BLE stack was initialized.
    if (xboxGamepad != nullptr || blePairingActive || bleConnected) {
      // Stop advertising if active
      stopBleAdvertising();
      // Give BLE stack time to clean up
      delay(100);
      // Fully deinitialize NimBLE (true = release memory)
      NimBLEDevice::deinit(true);
    }
    xboxGamepad = nullptr;
  }

  // Turn off display
  if (displayInitialized) {
    display.clearDisplay();
    display.display();
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    display.ssd1306_command(SSD1306_CHARGEPUMP); // Charge pump
    display.ssd1306_command(SSD1306_CHARGEPUMP_DISABLE); // Disable charge pump
  }

  // Release I2C lines to reduce back-powering during sleep
  Wire.end();
  pinMode(I2C_SDA_PIN, INPUT);
  pinMode(I2C_SCL_PIN, INPUT);

  // Set GBA link pins to high-impedance
  if (GBA_LINK_ENABLED) {
    pinMode(GBA_PIN_SC, INPUT);
    pinMode(GBA_PIN_SD, INPUT);
    pinMode(GBA_PIN_SO, INPUT);
  }

  // Turn off sensor power and set pin to INPUT for minimum leakage
  if (SENSOR_POWER_ENABLED) {
    digitalWrite(SENSOR_POWER_PIN, LOW);
    pinMode(SENSOR_POWER_PIN, INPUT);
  }

  // Set battery sense pin to INPUT (disable ADC pull)
  if (BATTERY_SENSE_ENABLED) {
    pinMode(BAT_PIN, INPUT);
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
  if (totalBars <= 0) {
    return;
  }

  int barWidth = SCREEN_WIDTH - 4;  // Target gauge width (keeps side margins)
  int gap = 2;
  int segW = (barWidth - (gap * (totalBars - 1))) / totalBars;
  if (segW < 1) {
    segW = 1;
  }
  int usedWidth = (segW * totalBars) + (gap * (totalBars - 1));
  int xStart = (SCREEN_WIDTH - usedWidth) / 2;

  for (int i = 0; i < totalBars; i++) {
    int x = xStart + (i * (segW + gap));
    if (i < filledBars) {
      display.fillRect(x, y, segW, h, SSD1306_WHITE); // Filled segment
    } else {
      display.drawRect(x, y, segW, h, SSD1306_WHITE); // Outline segment
    }
  }
}

bool isBluetoothIconOn() {
  if (!BLUETOOTH_ENABLED) {
    return false;
  }
  if (bleConnected) {
    return true;
  }
  if (blePairingActive) {
    return bleIconFlashOn;
  }
  return false;
}

void drawBluetoothIcon(int x, int y, bool on) {
  if (!on) {
    return;
  }

  display.drawLine(x + 3, y, x + 3, y + (STATUS_BT_ICON_H - 1), SSD1306_WHITE);
  display.drawLine(x + 3, y, x + 7, y + 2, SSD1306_WHITE);
  display.drawLine(x + 3, y + 4, x + 7, y + 2, SSD1306_WHITE);
  display.drawLine(x + 3, y + 4, x + 7, y + 6, SSD1306_WHITE);
  display.drawLine(x + 3, y + (STATUS_BT_ICON_H - 1), x + 7, y + 6, SSD1306_WHITE);
  display.drawLine(x + 3, y + 4, x, y + 2, SSD1306_WHITE);
  display.drawLine(x + 3, y + 4, x, y + 6, SSD1306_WHITE);
}

void drawBatteryGauge(int x, int y, int pct) {
  int16_t bodyW = STATUS_BATT_ICON_W - 2;
  display.drawRect(x, y, bodyW, STATUS_BATT_ICON_H, SSD1306_WHITE); // Main body
  display.fillRect(x + bodyW, y + 2, 2, STATUS_BATT_ICON_H - 4, SSD1306_WHITE); // Tip

  int fillW = (pct * (bodyW - 4)) / 100;
  display.fillRect(x + 2, y + 2, fillW, STATUS_BATT_ICON_H - 4, SSD1306_WHITE);
}

void drawStatusIcons() {
  bool batteryVisible = (cachedBatteryPct >= 0);
  bool btReserved = BLUETOOTH_ENABLED;
  bool btOn = isBluetoothIconOn();

  int16_t batteryX = SCREEN_WIDTH - STATUS_BATT_ICON_W - STATUS_RIGHT_MARGIN;
  int16_t batteryY = 2;
  int16_t btY = batteryY - 1;
  int16_t textX = 0;

  if (batteryVisible) {
    char pctText[6];
    snprintf(pctText, sizeof(pctText), "%d%%", cachedBatteryPct);
    int16_t x1, y1;
    uint16_t w, h;
    display.setTextSize(1);
    display.getTextBounds(pctText, 0, 0, &x1, &y1, &w, &h);
    textX = batteryX - STATUS_TEXT_GAP - (int16_t)w;
    int16_t textY = batteryY + ((STATUS_BATT_ICON_H - (int16_t)h) / 2);
    display.setCursor(textX, textY);
    display.print(pctText);
    drawBatteryGauge(batteryX, batteryY, cachedBatteryPct);
  }

  if (btReserved) {
    int16_t drawX = 0;
    if (batteryVisible) {
      drawX = textX - STATUS_BT_GAP - STATUS_BT_ICON_W;
    } else {
      drawX = SCREEN_WIDTH - STATUS_BT_ICON_W - STATUS_RIGHT_MARGIN;
    }
    drawBluetoothIcon(drawX, btY, btOn);
  }
}

void initBluetooth() {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  
  // Create Xbox gamepad device with XInput support for proper L3/R3 buttons
  // Use Xbox Series X controller configuration for proper VID/PID recognition
  xboxConfig = new XboxSeriesXControllerDeviceConfiguration();
  BLEHostConfiguration hostConfig = xboxConfig->getIdealHostConfiguration();

  xboxGamepad = new XboxGamepadDevice(xboxConfig);
  compositeHID.addDevice(xboxGamepad);
  compositeHID.begin(hostConfig);

  unsigned long startMs = millis();
  while (NimBLEDevice::getServer() == nullptr && (millis() - startMs) < 2000) {
    delay(10);
  }
  NimBLEServer* server = NimBLEDevice::getServer();
  if (server) {
    server->advertiseOnDisconnect(false);
  }

  if (BLE_BUTTONS_PER_SECOND == 0) {
    blePressIntervalMs = 0;
    blePressHoldMs = 0;
  } else {
    blePressIntervalMs = 1000UL / BLE_BUTTONS_PER_SECOND;
    if (blePressIntervalMs == 0) {
      blePressIntervalMs = 1;
    }
    blePressHoldMs = blePressIntervalMs / 2;
    if (blePressHoldMs == 0) {
      blePressHoldMs = 1;
    }
  }

  startBlePairing();
}

void startBleAdvertising() {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  if (advertising) {
    advertising->start();
  }
}

void stopBleAdvertising() {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  if (advertising) {
    advertising->stop();
  }
}

void startBlePairing() {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  blePairingActive = true;
  blePairingStartMs = millis();
  bleIconFlashOn = true;
  bleIconLastToggleMs = blePairingStartMs;
  startBleAdvertising();
}

void resetBlePressState() {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  if (xboxGamepad != nullptr && blePressHolding && bleActiveButton != 0) {
    xboxGamepad->release(bleActiveButton);
    xboxGamepad->sendGamepadReport();
  }
  blePressHolding = false;
  bleActiveButton = 0;
  blePressDirection = 0;
  blePressStartMs = 0;
  bleLastPressMs = 0;
}

void clearBleSyncPhase() {
  bleSyncPhase = BLE_SYNC_NONE;
  bleSyncRemaining = 0;
  bleRefillRemaining = 0;
  bleSyncDirection = 0;
  bleRefillDirection = 0;
}

void resetBleSyncState() {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  clearBleSyncPhase();
  bleSyncTargetSteps = 0;
  bleSyncStepsMax = 0;
  bleEstimateValid = false;
}

int getBleMeterStepsForGame(int game) {
  if (!BLUETOOTH_ENABLED) {
    return GAME_BARS[game];
  }
  if (game == 0) {
    if (!BLE_BOKTAI1_MGBA_10_STEP_WORKAROUND) {
      return GAME_BARS[game];
    }
    return BOKTAI1_STEP_COUNT;
  }
  return GAME_BARS[game];
}

int getBleBarFromStep(int game, int step) {
  if (!BLUETOOTH_ENABLED) {
    return step;
  }
  int stepsMax = getBleMeterStepsForGame(game);
  if (step < 0) {
    step = 0;
  }
  if (step > stepsMax) {
    step = stepsMax;
  }
  if (game == 0 && BLE_BOKTAI1_MGBA_10_STEP_WORKAROUND) {
    return BOKTAI1_STEP_TO_BAR[step];
  }
  return step;
}

int getBleStepFromBar(int game, int bar, bool fromEmpty) {
  if (!BLUETOOTH_ENABLED) {
    return bar;
  }
  int barsMax = GAME_BARS[game];
  if (bar < 0) {
    bar = 0;
  }
  if (bar > barsMax) {
    bar = barsMax;
  }
  if (game == 0 && BLE_BOKTAI1_MGBA_10_STEP_WORKAROUND) {
    return fromEmpty ? BOKTAI1_BAR_TO_STEP_FROM_EMPTY[bar] : BOKTAI1_BAR_TO_STEP_FROM_FULL[bar];
  }
  return bar;
}

int getBleActiveNumSteps() {
  if (!BLUETOOTH_ENABLED) {
    return getBleMeterStepsForGame(currentGame);
  }
  if (bleSyncPhase != BLE_SYNC_NONE && bleSyncStepsMax > 0) {
    return bleSyncStepsMax;
  }
  return getBleMeterStepsForGame(currentGame);
}

// ---------------------------------------------------------------------------
// Single Analog mode (BLE_CONTROL_MODE == 1)
// Maps the current bar count to a proportional deflection on one analog axis.
// The 0.0-1.0 range is divided into (numBars + 1) equal bands; we send the
// midpoint of the band for the current bar count.
// ---------------------------------------------------------------------------

void resetSingleAnalogState() {
  bleSingleAnalogBars = -1;
  bleSingleAnalogValue = 0;
  bleSingleAnalogActive = false;
  bleSingleAnalogButtonHeld = false;
  bleSingleAnalogLastRefreshMs = 0;
}

// Return the midpoint fraction for a given bar count.
// Boktai 1 (8 bars) → 9 bands, Boktai 2 & 3 (10 bars) → 11 bands.
float getSingleAnalogFraction(int bars, int numBars) {
  if (numBars <= 0) {
    return 0.0f;
  }
  int levels = numBars + 1;            // 0-bar band counts as a level
  float bandWidth = 1.0f / levels;
  float midpoint = (bars * bandWidth) + (bandWidth * 0.5f);
  if (midpoint > 1.0f) midpoint = 1.0f;
  return midpoint;
}

// Convert a 0.0–1.0 fraction to a signed stick deflection for the chosen axis.
// Positive axes map fraction to 0…XBOX_STICK_MAX,
// Negative axes map fraction to 0…XBOX_STICK_MIN (toward negative).
int16_t fractionToStickValue(float fraction, bool negative) {
  if (fraction <= 0.0f) return 0;
  if (fraction > 1.0f) fraction = 1.0f;
  if (negative) {
    return (int16_t)(fraction * XBOX_STICK_MIN);  // XBOX_STICK_MIN is negative
  }
  return (int16_t)(fraction * XBOX_STICK_MAX);
}

void applySingleAnalog(int bars, int numBars) {
  if (!BLUETOOTH_ENABLED || xboxGamepad == nullptr) {
    return;
  }

  if (numBars < 0) numBars = 0;
  bars = constrain(bars, 0, numBars);

  bool barsChanged = (!bleSingleAnalogActive || bars != bleSingleAnalogBars);
  if (!barsChanged) {
    return;
  }

  float frac = getSingleAnalogFraction(bars, numBars);
  uint8_t axis = BLE_SINGLE_ANALOG_AXIS;

  // Determine which stick and sign from the axis selector
  bool isLeft  = (axis < 4);
  bool isX     = (axis % 4) < 2;
  bool isNeg   = (axis % 2) == 0;

  int16_t value = fractionToStickValue(frac, isNeg);

  // Build the stick values; leave the other stick at its current position (0).
  int16_t leftX = 0, leftY = 0, rightX = 0, rightY = 0;
  if (isLeft) {
    if (isX) leftX = value; else leftY = value;
  } else {
    if (isX) rightX = value; else rightY = value;
  }

  // Release-and-repress the unlock button with every stick change so that
  // apps/emulators always register it, even if they started listening late.
  if (BLE_METER_UNLOCK_BUTTON_ENABLED) {
    bool needButton = (value != 0);
    if (needButton) {
      if (bleSingleAnalogButtonHeld) {
        // Release first, send report, then re-press with the new stick value
        xboxGamepad->release(BLE_METER_UNLOCK_BUTTON);
        xboxGamepad->sendGamepadReport();
      }
      xboxGamepad->press(BLE_METER_UNLOCK_BUTTON);
      bleSingleAnalogButtonHeld = true;
      bleSingleAnalogLastRefreshMs = millis();
    } else if (bleSingleAnalogButtonHeld) {
      xboxGamepad->release(BLE_METER_UNLOCK_BUTTON);
      bleSingleAnalogButtonHeld = false;
    }
  }

  // Only update the stick that this mode controls
  if (isLeft) {
    xboxGamepad->setLeftThumb(leftX, leftY);
  } else {
    xboxGamepad->setRightThumb(rightX, rightY);
  }
  xboxGamepad->sendGamepadReport();

  bleSingleAnalogBars = bars;
  bleSingleAnalogValue = value;
  bleSingleAnalogActive = true;
}

void releaseSingleAnalog() {
  if (!BLUETOOTH_ENABLED || xboxGamepad == nullptr) {
    resetSingleAnalogState();
    return;
  }

  bool changed = false;
  uint8_t axis = BLE_SINGLE_ANALOG_AXIS;
  bool isLeft = (axis < 4);

  if (BLE_METER_UNLOCK_BUTTON_ENABLED && bleSingleAnalogButtonHeld) {
    xboxGamepad->release(BLE_METER_UNLOCK_BUTTON);
    changed = true;
  }

  if (bleSingleAnalogValue != 0) {
    if (isLeft) {
      xboxGamepad->setLeftThumb(0, 0);
    } else {
      xboxGamepad->setRightThumb(0, 0);
    }
    changed = true;
  }

  if (changed) {
    xboxGamepad->sendGamepadReport();
  }

  resetSingleAnalogState();
}

// Periodically release and re-press the unlock button so apps that start
// listening mid-session still register it as held.
void refreshSingleAnalogButton() {
  if (!BLUETOOTH_ENABLED || BLE_CONTROL_MODE != 1) {
    return;
  }
  if (!BLE_METER_UNLOCK_BUTTON_ENABLED || BLE_METER_UNLOCK_REFRESH_MS == 0) {
    return;
  }
  if (!bleSingleAnalogButtonHeld || xboxGamepad == nullptr) {
    return;
  }

  unsigned long now = millis();
  if ((now - bleSingleAnalogLastRefreshMs) < BLE_METER_UNLOCK_REFRESH_MS) {
    return;
  }

  xboxGamepad->release(BLE_METER_UNLOCK_BUTTON);
  xboxGamepad->sendGamepadReport();
  xboxGamepad->press(BLE_METER_UNLOCK_BUTTON);
  xboxGamepad->sendGamepadReport();
  bleSingleAnalogLastRefreshMs = now;
}

void applyBlePressEffect(int direction) {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  if (!bleEstimateValid) {
    return;
  }
  int stepsMax = getBleActiveNumSteps();
  if (stepsMax <= 0) {
    return;
  }
  if (direction > 0) {
    bleEstimatedSteps = min(bleEstimatedSteps + 1, stepsMax);
  } else if (direction < 0) {
    bleEstimatedSteps = max(bleEstimatedSteps - 1, 0);
  }
}

void finishBleSync() {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  clearBleSyncPhase();
  bleEstimatedSteps = bleSyncTargetSteps;
  bleEstimateValid = true;
}

void startBleResync(int deviceBars, int numBars) {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  if (!bleConnected || numBars <= 0) {
    return;
  }
  resetBlePressState();
  bleSyncPhase = BLE_SYNC_CLAMP;
  bleSyncStepsMax = getBleMeterStepsForGame(currentGame);
  int targetBars = constrain(deviceBars, 0, numBars);
  int middle = numBars / 2;
  int direction = (targetBars <= middle) ? BLE_DIR_DEC : BLE_DIR_INC;
  bleSyncTargetSteps = getBleStepFromBar(currentGame, targetBars, direction == BLE_DIR_DEC);
  bleSyncDirection = direction;
  bleSyncRemaining = bleSyncStepsMax;
  if (direction == BLE_DIR_DEC) {
    bleRefillDirection = BLE_DIR_INC;
    bleRefillRemaining = bleSyncTargetSteps;
  } else {
    bleRefillDirection = BLE_DIR_DEC;
    bleRefillRemaining = bleSyncStepsMax - bleSyncTargetSteps;
  }
  bleEstimateValid = false;
  bleLastResyncMs = millis();
}

void updateBluetoothState() {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  bool connectedNow = compositeHID.isConnected();
  if (connectedNow != bleConnected) {
    bleConnected = connectedNow;
    if (bleConnected) {
      blePairingActive = false;
      stopBleAdvertising();
      if (BLE_CONTROL_MODE == 1) {
        bleSyncPending = false;
        resetBlePressState();
        resetBleSyncState();
        resetSingleAnalogState();
      } else {
        bleSyncPending = true;
        bleEstimateValid = false;
      }
    } else {
      resetBlePressState();
      resetBleSyncState();
      if (BLE_CONTROL_MODE == 1) {
        releaseSingleAnalog();
      }
      // If a connection drops, re-enter pairing/advertising for the usual timeout.
      startBlePairing();
    }
  }

  if (blePairingActive && !bleConnected) {
    unsigned long now = millis();
    if (BLE_PAIRING_TIMEOUT_MS > 0 && (now - blePairingStartMs) >= BLE_PAIRING_TIMEOUT_MS) {
      blePairingActive = false;
      stopBleAdvertising();
    } else if ((now - bleIconLastToggleMs) >= BLE_ICON_FLASH_MS) {
      bleIconFlashOn = !bleIconFlashOn;
      bleIconLastToggleMs = now;
    }
  }
}

void updateBluetoothMeter(int deviceBars, int numBars) {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  if (!bleConnected) {
    return;
  }

  bleDeviceBars = constrain(deviceBars, 0, numBars);
  bleDeviceNumBars = numBars;

  if (BLE_CONTROL_MODE == 1) {
    if (bleGameChanged) {
      resetSingleAnalogState();
      bleGameChanged = false;
    }
    applySingleAnalog(bleDeviceBars, bleDeviceNumBars);
    return;
  }

  if (bleSyncPending || bleGameChanged) {
    startBleResync(bleDeviceBars, bleDeviceNumBars);
    bleSyncPending = false;
    bleGameChanged = false;
    return;
  }

  if (BLE_RESYNC_ENABLED && BLE_RESYNC_INTERVAL_MS > 0 && bleSyncPhase == BLE_SYNC_NONE) {
    unsigned long now = millis();
    if ((now - bleLastResyncMs) >= BLE_RESYNC_INTERVAL_MS) {
      startBleResync(bleDeviceBars, bleDeviceNumBars);
    }
  }
}

void handleBlePresses() {
  if (!BLUETOOTH_ENABLED) {
    return;
  }
  if (BLE_CONTROL_MODE == 1) {
    return;
  }
  if (!bleConnected) {
    return;
  }

  unsigned long now = millis();

  if (blePressHolding) {
    if ((now - blePressStartMs) >= blePressHoldMs) {
      if (xboxGamepad != nullptr) {
        xboxGamepad->release(bleActiveButton);
        xboxGamepad->sendGamepadReport();
      }
      blePressHolding = false;
      applyBlePressEffect(blePressDirection);
      if (bleSyncPhase != BLE_SYNC_NONE) {
        bleSyncRemaining--;
        if (bleSyncRemaining <= 0) {
          if (bleSyncPhase == BLE_SYNC_CLAMP) {
            if (bleSyncDirection == BLE_DIR_DEC) {
              bleEstimatedSteps = 0;
            } else {
              bleEstimatedSteps = bleSyncStepsMax;
            }
            bleEstimateValid = true;
            bleSyncPhase = BLE_SYNC_REFILL;
            bleSyncRemaining = bleRefillRemaining;
            bleSyncDirection = bleRefillDirection;
            if (bleSyncRemaining <= 0) {
              finishBleSync();
            }
          } else {
            finishBleSync();
          }
        }
      }
    }
    return;
  }

  if (blePressIntervalMs == 0) {
    return;
  }
  if ((now - bleLastPressMs) < blePressIntervalMs) {
    return;
  }

  int direction = 0;
  if (bleSyncPhase != BLE_SYNC_NONE) {
    if (bleSyncRemaining > 0) {
      direction = bleSyncDirection;
    }
  } else if (bleEstimateValid) {
    int estimatedBars = getBleBarFromStep(currentGame, bleEstimatedSteps);
    if (bleDeviceBars > estimatedBars) {
      direction = BLE_DIR_INC;
    } else if (bleDeviceBars < estimatedBars) {
      direction = BLE_DIR_DEC;
    }
  }

  if (direction == 0) {
    return;
  }

  blePressDirection = direction;
  bleActiveButton = (direction > 0) ? BLE_BUTTON_INC : BLE_BUTTON_DEC;
  blePressHolding = true;
  blePressStartMs = now;
  bleLastPressMs = now;
  if (xboxGamepad != nullptr) {
    xboxGamepad->press(bleActiveButton);
    xboxGamepad->sendGamepadReport();
  }
}

// Calculate battery % based on analog reading
void updateBatteryStatus() {
  if (!BATTERY_SENSE_ENABLED) {
    return;
  }

  unsigned long now = millis();
  if ((now - lastBatterySampleTime) < BATTERY_SAMPLE_MS) {
    return;
  }
  lastBatterySampleTime = now;

  cachedBatteryPct = readBatteryPercentage();

  if (BLUETOOTH_ENABLED && cachedBatteryPct >= 0) {
    compositeHID.setBatteryLevel((uint8_t)cachedBatteryPct);
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
  }
  float raw = sum / 10.0f;
  cachedBatteryAdcAvg = raw;
  hasBatteryReading = true;
  
  // ESP32-S3 ADC: 12-bit (0-4095), default ~0-3.3V range
  // VOLT_DIVIDER_MULT compensates for voltage divider ratio + ADC variance
  // Calibrate in config.h if battery % is wrong at full charge
  float voltage = (raw / 4095.0f) * 3.3f * VOLT_DIVIDER_MULT;
  cachedBatteryVoltage = voltage;
  
  if (DEBUG_SERIAL) {
    Serial.print("Battery ADC raw: "); Serial.print(raw);
    Serial.print(" Voltage: "); Serial.println(voltage);
  }

  float range = VOLT_MAX - VOLT_MIN;
  if (range <= 0.0f) {
    return 0;  // Guard: VOLT_MAX must be greater than VOLT_MIN
  }
  int pct = ((voltage - VOLT_MIN) / range) * 100;
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
  float divisor = (uvDivisor > 0.0f) ? uvDivisor : UV_SENSITIVITY_COUNTS_PER_UVI;
  float measuredUvi = (float)rawUVS / divisor;
  float correctedUvi = applyEnclosureCompensation(measuredUvi);

  cachedRawUVS = rawUVS;
  cachedUviRaw = measuredUvi;
  cachedUviCorrected = correctedUvi;
  
  // Debug output
  if (DEBUG_SERIAL) {
    Serial.print("UV raw: "); Serial.print(rawUVS);
    Serial.print(" UVI measured: "); Serial.print(measuredUvi, 3);
    Serial.print(" UVI corrected: "); Serial.println(correctedUvi, 3);
  }
  
  return correctedUvi;
}
