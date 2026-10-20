// Ojo del Sol Calibrator
//
// Reads a real Boktai cartridge's solar sensor (through the GBA cartridge bus)
// and an LTR390 UV sensor at the same time, and logs both to flash so the Ojo
// del Sol's UV thresholds can be tuned against the original hardware. It is
// meant to sit in the sun for hours, then have its log dumped over serial.
//
// STATUS: UNTESTED on hardware. See the README.
//
// One CSV row per sample:
//   uptime_ms   - milliseconds since power-up, at the start of the cartridge read
//   cart_status - ok | timeout (flag never rose, e.g. no cartridge)
//   cart_raw    - the game's raw sensor value (blank on timeout); dark ~230, bright -> 0
//   cart_bus    - 1 if the GPIO register read-back matched, else 0 (wiring check)
//   cart_ms     - how long the cartridge read took
//   ltr_status  - ok | stale (no fresh LTR390 reading) | absent
//   ltr_raw     - LTR390 UVS counts (blank unless ok)
//   uvi         - ltr_raw / 2300 (blank unless ok)
//
// To compare against the game: scaled = calibration - cart_raw (clamped 0..140),
// where the game's calibration is the dark cart_raw minus 2.

#include <Wire.h>
#include <LittleFS.h>
#include <Preferences.h>
#include "Adafruit_LTR390.h"
#include "config.h"
#include "GbaCartSensor.h"

Adafruit_LTR390 ltr = Adafruit_LTR390();
Preferences prefs;

const uint8_t MEAS_RATE_500MS = 0x04;
const uint32_t LTR_WAIT_MS = 700;           // longest wait for a fresh LTR390 reading
const uint8_t LTR_REINIT_AFTER_FAILURES = 5;
const uint32_t LTR_RETRY_INTERVAL_MS = 30000;
const uint32_t STORAGE_RETRY_INTERVAL_MS = 10000;

// --- State -------------------------------------------------------------------
bool ltrReady = false;
uint8_t ltrFailures = 0;
uint32_t lastLtrRetryMs = 0;

bool fsMounted = false;
bool storageOk = false;     // log file open and writable
bool storageFull = false;   // stopped on purpose: out of space
uint32_t lastStorageRetryMs = 0;
File logFile;
char logPath[24];
uint32_t sessionNo = 0;
uint32_t rowsLogged = 0;
uint32_t rowsSinceFlush = 0;

bool liveEcho = LIVE_PRINT_DEFAULT;
uint32_t sampleIntervalMs = SAMPLE_INTERVAL_MS;
uint32_t nextSampleMs = 0;

// Last sample, for "status"
uint32_t lastUptimeMs = 0;
bool lastCartOk = false;
uint16_t lastCartRaw = 0;
bool lastLtrFresh = false;
uint32_t lastLtrRaw = 0;

char cmdBuf[48];
uint8_t cmdLen = 0;

// --- Status LED ----------------------------------------------------------------
void setLed(uint8_t r, uint8_t g, uint8_t b) {
  if (STATUS_LED_PIN < 0) return;
  neopixelWrite(STATUS_LED_PIN,
                (uint16_t)r * STATUS_LED_BRIGHTNESS / 255,
                (uint16_t)g * STATUS_LED_BRIGHTNESS / 255,
                (uint16_t)b * STATUS_LED_BRIGHTNESS / 255);
}

// --- LTR390 --------------------------------------------------------------------
void setLtrMeasurementRate(uint8_t rate) {
  uint8_t resBits = (uint8_t)ltr.getResolution();
  uint8_t regValue = (uint8_t)((resBits << 4) | (rate & 0x07));
  Wire.beginTransmission(LTR390_I2CADDR_DEFAULT);
  Wire.write(LTR390_MEAS_RATE);
  Wire.write(regValue);
  Wire.endTransmission();
}

bool initLtr() {
  for (int attempt = 0; attempt < 3; attempt++) {
    if (ltr.begin(&Wire)) {
      ltr.setMode(LTR390_MODE_UVS);
      ltr.setGain(LTR390_GAIN_18);
      ltr.setResolution(LTR390_RESOLUTION_20BIT);
      setLtrMeasurementRate(MEAS_RATE_500MS);
      return true;
    }
    delay(50);
  }
  return false;
}

// Wait for a fresh LTR390 reading; returns false on timeout.
bool waitLtrNewData(uint32_t timeoutMs) {
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    if (ltr.newDataAvailable()) return true;
    delay(2);
  }
  return false;
}

// --- Storage -------------------------------------------------------------------
uint32_t freeBytes() {
  return (uint32_t)(LittleFS.totalBytes() - LittleFS.usedBytes());
}

void makeLogPath(char *out, size_t outLen, uint32_t session) {
  snprintf(out, outLen, "/cal_%04lu.csv", (unsigned long)session);
}

bool writeSessionHeader() {
  char header[256];
  int n = snprintf(header, sizeof(header),
                   "# Ojo del Sol Calibrator session %lu (UNTESTED firmware)\n"
                   "# tick_us=%lu ltr=UVS,18x,20bit,500ms counts_per_uvi=%.0f interval_ms=%lu\n"
                   "uptime_ms,cart_status,cart_raw,cart_bus,cart_ms,ltr_status,ltr_raw,uvi\n",
                   (unsigned long)sessionNo, (unsigned long)CART_TICK_US,
                   UV_SENSITIVITY_COUNTS_PER_UVI, (unsigned long)sampleIntervalMs);
  if (n <= 0 || n >= (int)sizeof(header)) return false;
  if (logFile.write((const uint8_t *)header, n) != (size_t)n) return false;
  logFile.flush();
  return true;
}

// Begin a new session file. The session number survives power cycles (NVS).
bool startSession() {
  if (!fsMounted) return false;
  if (logFile) logFile.close();

  prefs.begin("calib", false);
  sessionNo = prefs.getUInt("session", 0) + 1;
  prefs.putUInt("session", sessionNo);
  prefs.end();

  makeLogPath(logPath, sizeof(logPath), sessionNo);
  logFile = LittleFS.open(logPath, FILE_WRITE);
  rowsLogged = 0;
  rowsSinceFlush = 0;
  storageFull = false;
  storageOk = logFile && writeSessionHeader();
  Serial.printf("Session %lu -> %s (%s)\n", (unsigned long)sessionNo, logPath,
                storageOk ? "logging" : "FAILED to open");
  return storageOk;
}

// Try to reopen the current file after a write error.
void retryStorage() {
  if (!fsMounted || storageFull || storageOk) return;
  if (millis() - lastStorageRetryMs < STORAGE_RETRY_INTERVAL_MS) return;
  lastStorageRetryMs = millis();
  if (logFile) logFile.close();
  logFile = LittleFS.open(logPath, FILE_APPEND);
  storageOk = (bool)logFile;
  Serial.println(storageOk ? "Storage recovered" : "Storage still failing");
}

void flushLog() {
  logFile.flush();
  rowsSinceFlush = 0;
  if (freeBytes() < MIN_FREE_BYTES) {
    storageOk = false;
    storageFull = true;
    logFile.close();
    Serial.println("Out of flash space - logging stopped. Dump and delete old sessions.");
  }
}

void logSample(uint32_t uptimeMs, const CartReading &cart, bool ltrFresh, uint32_t ltrRaw) {
  char cartRawStr[8] = "";
  char ltrRawStr[12] = "";
  char uviStr[14] = "";
  if (cart.ok) snprintf(cartRawStr, sizeof(cartRawStr), "%u", (unsigned)cart.raw);
  if (ltrFresh) {
    snprintf(ltrRawStr, sizeof(ltrRawStr), "%lu", (unsigned long)ltrRaw);
    snprintf(uviStr, sizeof(uviStr), "%.4f", (float)ltrRaw / UV_SENSITIVITY_COUNTS_PER_UVI);
  }
  const char *ltrStatus = ltrFresh ? "ok" : (ltrReady ? "stale" : "absent");

  char line[112];
  int n = snprintf(line, sizeof(line), "%lu,%s,%s,%d,%lu,%s,%s,%s\n",
                   (unsigned long)uptimeMs, cart.ok ? "ok" : "timeout", cartRawStr,
                   cart.busOk ? 1 : 0, (unsigned long)cart.durationMs,
                   ltrStatus, ltrRawStr, uviStr);
  if (n <= 0 || n >= (int)sizeof(line)) return;

  if (liveEcho) Serial.print(line);

  if (!storageOk) return;
  if (logFile.write((const uint8_t *)line, n) != (size_t)n) {
    storageOk = false;
    Serial.println("Write failed");
    return;
  }
  rowsLogged++;
  if (++rowsSinceFlush >= FLUSH_EVERY_N_SAMPLES) flushLog();
}

// --- Sampling ------------------------------------------------------------------
void takeSample() {
  // Line the cartridge read up with an LTR390 integration window: consume the
  // current reading, read the cartridge while the next one integrates, then take it.
  if (ltrReady && waitLtrNewData(LTR_WAIT_MS)) ltr.readUVS();

  uint32_t uptimeMs = millis();
  CartReading cart = cartReadSolarSensor();

  bool ltrFresh = false;
  uint32_t ltrRaw = 0;
  if (ltrReady) {
    ltrFresh = waitLtrNewData(LTR_WAIT_MS);
    if (ltrFresh) {
      ltrRaw = ltr.readUVS();
      ltrFailures = 0;
    } else if (++ltrFailures >= LTR_REINIT_AFTER_FAILURES) {
      ltrFailures = 0;
      ltrReady = initLtr();
    }
  } else if (millis() - lastLtrRetryMs >= LTR_RETRY_INTERVAL_MS) {
    lastLtrRetryMs = millis();
    ltrReady = initLtr();
  }

  logSample(uptimeMs, cart, ltrFresh, ltrRaw);

  lastUptimeMs = uptimeMs;
  lastCartOk = cart.ok;
  lastCartRaw = cart.raw;
  lastLtrFresh = ltrFresh;
  lastLtrRaw = ltrRaw;

  if (!storageOk) setLed(255, 0, 0);
  else if (!cart.ok || !cart.busOk || !ltrFresh) setLed(255, 160, 0);
  else setLed(0, 255, 0);
}

// --- Serial commands -----------------------------------------------------------
void printHelp() {
  Serial.println("Commands:");
  Serial.println("  status             show state and last sample");
  Serial.println("  list               list session files");
  Serial.println("  dump [n]           print session n (default: current) - sampling pauses meanwhile");
  Serial.println("  delete <n|all>     delete a session (never the current one)");
  Serial.println("  format             erase all logs and start a new session");
  Serial.println("  interval <ms>      change the sample interval (not saved)");
  Serial.println("  live <on|off>      echo each row to serial");
}

void cmdStatus() {
  Serial.printf("Session %lu, file %s, %lu rows, storage %s\n", (unsigned long)sessionNo, logPath,
                (unsigned long)rowsLogged,
                storageFull ? "FULL" : (storageOk ? "ok" : "ERROR"));
  if (fsMounted) {
    Serial.printf("Flash: %lu of %lu bytes free\n", (unsigned long)freeBytes(),
                  (unsigned long)LittleFS.totalBytes());
  } else {
    Serial.println("Flash: not mounted - send 'format' to initialise it");
  }
  Serial.printf("Uptime %lu s, interval %lu ms, live %s\n", (unsigned long)(millis() / 1000),
                (unsigned long)sampleIntervalMs, liveEcho ? "on" : "off");
  Serial.printf("LTR390: %s\n", ltrReady ? "found" : "NOT found");
  Serial.printf("Last sample @%lu ms: cart %s raw=%u, ltr %s raw=%lu\n", (unsigned long)lastUptimeMs,
                lastCartOk ? "ok" : "timeout", (unsigned)lastCartRaw,
                lastLtrFresh ? "ok" : "stale", (unsigned long)lastLtrRaw);
}

void cmdList() {
  if (!fsMounted) { Serial.println("Flash not mounted"); return; }
  File root = LittleFS.open("/");
  File f = root.openNextFile();
  while (f) {
    Serial.printf("%s\t%lu bytes\n", f.name(), (unsigned long)f.size());
    f = root.openNextFile();
  }
}

void cmdDump(const char *arg) {
  if (!fsMounted) { Serial.println("Flash not mounted"); return; }
  uint32_t n = arg ? (uint32_t)strtoul(arg, nullptr, 10) : sessionNo;
  if (n == 0) n = sessionNo;
  char path[24];
  makeLogPath(path, sizeof(path), n);
  if (n == sessionNo && logFile) logFile.flush();
  File f = LittleFS.open(path, FILE_READ);
  if (!f) { Serial.printf("No such file: %s\n", path); return; }
  Serial.printf("----- begin %s -----\n", path);
  uint8_t buf[256];
  int got;
  while ((got = f.read(buf, sizeof(buf))) > 0) Serial.write(buf, got);
  f.close();
  Serial.printf("----- end %s -----\n", path);
  nextSampleMs = millis() + sampleIntervalMs;  // don't try to catch up
}

void cmdDelete(const char *arg) {
  if (!fsMounted) { Serial.println("Flash not mounted"); return; }
  if (!arg) { Serial.println("delete <n|all>"); return; }
  if (strcmp(arg, "all") == 0) {
    for (int pass = 0; pass < 256; pass++) {
      char victim[32] = "";
      File root = LittleFS.open("/");
      File f = root.openNextFile();
      while (f) {
        const char *name = f.name();
        char current[24];
        makeLogPath(current, sizeof(current), sessionNo);
        if (strncmp(name, "cal_", 4) == 0 && strcmp(current + 1, name) != 0) {
          snprintf(victim, sizeof(victim), "/%s", name);
          break;
        }
        f = root.openNextFile();
      }
      f.close();
      root.close();
      if (victim[0] == '\0') break;
      LittleFS.remove(victim);
      Serial.printf("Deleted %s\n", victim);
    }
    return;
  }
  uint32_t n = (uint32_t)strtoul(arg, nullptr, 10);
  if (n == sessionNo) { Serial.println("That is the current session"); return; }
  char path[24];
  makeLogPath(path, sizeof(path), n);
  Serial.println(LittleFS.remove(path) ? "Deleted" : "No such file");
}

void cmdFormat() {
  if (logFile) logFile.close();
  storageOk = false;
  Serial.println("Formatting...");
  LittleFS.end();
  bool ok = LittleFS.format();
  fsMounted = ok && LittleFS.begin(false);
  Serial.println(fsMounted ? "Done" : "Format/mount FAILED");
  if (fsMounted) startSession();
  nextSampleMs = millis() + sampleIntervalMs;
}

void handleCommand(char *line) {
  char *cmd = strtok(line, " \t");
  char *arg = strtok(nullptr, " \t");
  if (!cmd) return;
  if (strcmp(cmd, "help") == 0) printHelp();
  else if (strcmp(cmd, "status") == 0) cmdStatus();
  else if (strcmp(cmd, "list") == 0) cmdList();
  else if (strcmp(cmd, "dump") == 0) cmdDump(arg);
  else if (strcmp(cmd, "delete") == 0) cmdDelete(arg);
  else if (strcmp(cmd, "format") == 0) cmdFormat();
  else if (strcmp(cmd, "interval") == 0) {
    uint32_t ms = arg ? (uint32_t)strtoul(arg, nullptr, 10) : 0;
    if (ms < 500) Serial.println("interval <ms>, minimum 500");
    else { sampleIntervalMs = ms; Serial.printf("Interval %lu ms\n", (unsigned long)ms); }
  } else if (strcmp(cmd, "live") == 0) {
    if (arg && strcmp(arg, "on") == 0) liveEcho = true;
    else if (arg && strcmp(arg, "off") == 0) liveEcho = false;
    Serial.printf("live %s\n", liveEcho ? "on" : "off");
  } else {
    Serial.println("Unknown command - try 'help'");
  }
}

void pollSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (cmdLen > 0) {
        cmdBuf[cmdLen] = '\0';
        handleCommand(cmdBuf);
        cmdLen = 0;
      }
    } else if (cmdLen < sizeof(cmdBuf) - 1) {
      cmdBuf[cmdLen++] = c;
    }
  }
}

// --- Arduino entry points ----------------------------------------------------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);
  Serial.println();
  Serial.println("Ojo del Sol Calibrator (UNTESTED) - type 'help' for commands");
  setLed(0, 0, 255);

  cartBusBegin();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ltrReady = initLtr();
  Serial.println(ltrReady ? "LTR390 ready" : "LTR390 not found - will keep retrying; cartridge only for now");

  fsMounted = LittleFS.begin(false);
  if (fsMounted) {
    startSession();
  } else {
    Serial.println("Flash mount failed. Send 'format' to initialise it (this erases any logs).");
  }

  nextSampleMs = millis() + 1000;
}

void loop() {
  pollSerial();
  retryStorage();

  if ((int32_t)(millis() - nextSampleMs) < 0) {
    delay(1);
    return;
  }
  takeSample();
  nextSampleMs += sampleIntervalMs;
  if ((int32_t)(millis() - nextSampleMs) > 0) nextSampleMs = millis();  // fell behind
}
