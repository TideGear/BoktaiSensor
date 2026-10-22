// Ojo del Sol Calibrator
//
// Reads a real Boktai cartridge's solar sensor (through the GBA cartridge bus)
// and an LTR390 UV sensor at the same time, and logs both to flash so the Ojo
// del Sol's UV thresholds can be tuned against the original hardware. It is
// meant to sit in the sun for hours. It joins your Wi-Fi (or runs its own hotspot) with a web app
// (see WebApp.ino) to watch live readings, browse past sessions and manage and
// back up the stored logs; a serial console offers the same commands.
//
// STATUS: partly tested on hardware (Wi-Fi, web app, logging, LTR390); the cartridge
// reading has not been tried yet. See the README.
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
// Lines starting with # are comments. "# time_sync,<uptime_ms>,<epoch_ms>" lines
// are written when a phone connects to the web app and tell it the real time.
//
// To compare against the game: scaled = calibration - cart_raw (clamped 0..140),
// where the game's calibration is the raw value measured at its sensor setup minus 2.

#include <Wire.h>
#include <LittleFS.h>
#include <Preferences.h>
#include "Adafruit_LTR390.h"
#include "config.h"
#include "GbaCartSensor.h"
#include "Shared.h"

Adafruit_LTR390 ltr = Adafruit_LTR390();
Preferences prefs;

const uint8_t MEAS_RATE_500MS = 0x04;
const uint32_t LTR_WAIT_MS = 700;           // longest wait for a fresh LTR390 reading
const uint8_t LTR_REINIT_AFTER_FAILURES = 5;
const uint32_t LTR_RETRY_INTERVAL_MS = 30000;
const uint32_t STORAGE_RETRY_INTERVAL_MS = 10000;
const uint32_t MIN_INTERVAL_MS = 500;
const uint32_t MAX_INTERVAL_MS = 3600000;

// --- State -------------------------------------------------------------------
SemaphoreHandle_t fsMutex = nullptr;   // see Shared.h

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

// Wall-clock time (there is no RTC): from internet time servers when on your Wi-Fi,
// otherwise from the phone that opens the web app.
bool timeSynced = false;
const char *timeSource = "none";  // "ntp", "browser" or "none"
uint64_t epochOffsetMs = 0;     // epoch_ms = epochOffsetMs + millis()
uint32_t timeSyncedAtMs = 0;

// Last sample, for "status"
uint32_t lastUptimeMs = 0;
bool lastCartOk = false;
uint16_t lastCartRaw = 0;
bool lastLtrFresh = false;
uint32_t lastLtrRaw = 0;

// Recent samples for the web app's live view
LiveSample liveBuf[LIVE_SAMPLES];
volatile uint32_t liveSeq = 0;
portMUX_TYPE liveMux = portMUX_INITIALIZER_UNLOCKED;

char cmdBuf[48];
uint8_t cmdLen = 0;

// --- Status LED ----------------------------------------------------------------
void setLed(uint8_t r, uint8_t g, uint8_t b) {
  if (STATUS_LED_PIN < 0) return;
  uint8_t rs = (uint16_t)r * STATUS_LED_BRIGHTNESS / 255;
  uint8_t gs = (uint16_t)g * STATUS_LED_BRIGHTNESS / 255;
  uint8_t bs = (uint16_t)b * STATUS_LED_BRIGHTNESS / 255;
  // neopixelWrite() is deprecated from esp32 core 3.3.0 in favour of rgbLedWrite()
#if defined(ESP_ARDUINO_VERSION_VAL) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 3, 0)
  rgbLedWrite(STATUS_LED_PIN, rs, gs, bs);
#else
  neopixelWrite(STATUS_LED_PIN, rs, gs, bs);
#endif
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

// --- Settings --------------------------------------------------------------------
// The interval can be changed from the web app or serial and is remembered in NVS.
// 0 restores the SAMPLE_INTERVAL_MS default from config.h.
void loadIntervalSetting() {
  prefs.begin("calib", true);
  uint32_t ms = prefs.getUInt("interval", 0);
  prefs.end();
  sampleIntervalMs = (ms >= MIN_INTERVAL_MS && ms <= MAX_INTERVAL_MS) ? ms : SAMPLE_INTERVAL_MS;
}

bool setSampleInterval(uint32_t ms) {
  if (ms != 0 && (ms < MIN_INTERVAL_MS || ms > MAX_INTERVAL_MS)) return false;
  FsLock lock;
  prefs.begin("calib", false);
  if (ms == 0) prefs.remove("interval"); else prefs.putUInt("interval", ms);
  prefs.end();
  sampleIntervalMs = (ms == 0) ? SAMPLE_INTERVAL_MS : ms;
  return true;
}

// --- Time ------------------------------------------------------------------------
uint64_t nowEpochMs() {
  return timeSynced ? epochOffsetMs + millis() : 0;
}

// Caller holds fsMutex. Writes a comment line pairing uptime with wall-clock time.
bool writeTimeSyncLine() {
  if (!timeSynced || !storageOk || !logFile) return false;
  char line[64];
  char epoch[24];
  fmtU64(epoch, nowEpochMs());
  int n = snprintf(line, sizeof(line), "# time_sync,%lu,%s\n", (unsigned long)millis(), epoch);
  if (n <= 0 || n >= (int)sizeof(line)) return false;
  return logFile.write((const uint8_t *)line, n) == (size_t)n;
}

// source is "ntp" or "browser"; the latest sync wins.
void setTimeSync(uint64_t epochMs, const char *source) {
  FsLock lock;
  epochOffsetMs = epochMs - (uint64_t)millis();
  timeSynced = true;
  timeSource = source;
  timeSyncedAtMs = millis();
  if (writeTimeSyncLine()) logFile.flush();
}

// --- Storage -------------------------------------------------------------------
uint32_t freeBytes() {
  return (uint32_t)(LittleFS.totalBytes() - LittleFS.usedBytes());
}

void makeLogPath(char *out, size_t outLen, uint32_t session) {
  snprintf(out, outLen, "/cal_%04lu.csv", (unsigned long)session);
}

// Highest session number among the cal_NNNN.csv files on flash (0 if none).
uint32_t maxSessionOnDisk() {
  uint32_t best = 0;
  File root = LittleFS.open("/");
  File f = root.openNextFile();
  while (f) {
    uint32_t n;
    if (parseSessionName(f.name(), &n) && n > best) best = n;
    f = root.openNextFile();
  }
  root.close();
  return best;
}

// Reserve the next session number. The counter survives power cycles (NVS), and
// is never lower than a session restored from a backup.
uint32_t allocateSessionNumber() {
  FsLock lock;
  prefs.begin("calib", false);
  uint32_t n = prefs.getUInt("session", 0);
  uint32_t onDisk = maxSessionOnDisk();
  if (onDisk > n) n = onDisk;
  n++;
  prefs.putUInt("session", n);
  prefs.end();
  return n;
}

// Caller holds fsMutex.
bool writeSessionHeader() {
  char header[256];
  int n = snprintf(header, sizeof(header),
                   "# Ojo del Sol Calibrator session %lu (cartridge reading untested)\n"
                   "# tick_us=%lu ltr=UVS,18x,20bit,500ms counts_per_uvi=%.0f interval_ms=%lu\n"
                   "uptime_ms,cart_status,cart_raw,cart_bus,cart_ms,ltr_status,ltr_raw,uvi\n",
                   (unsigned long)sessionNo, (unsigned long)CART_TICK_US,
                   UV_SENSITIVITY_COUNTS_PER_UVI, (unsigned long)sampleIntervalMs);
  if (n <= 0 || n >= (int)sizeof(header)) return false;
  if (logFile.write((const uint8_t *)header, n) != (size_t)n) return false;
  logFile.flush();
  return true;
}

// Begin a new session file.
bool startSession() {
  if (!fsMounted) return false;
  FsLock lock;
  if (logFile) logFile.close();

  sessionNo = allocateSessionNumber();
  makeLogPath(logPath, sizeof(logPath), sessionNo);
  logFile = LittleFS.open(logPath, FILE_WRITE);
  rowsLogged = 0;
  rowsSinceFlush = 0;
  storageFull = false;
  storageOk = logFile && writeSessionHeader();
  if (storageOk && writeTimeSyncLine()) logFile.flush();
  Serial.printf("Session %lu -> %s (%s)\n", (unsigned long)sessionNo, logPath,
                storageOk ? "logging" : "FAILED to open");
  return storageOk;
}

// Try to reopen the current file after a write error.
void retryStorage() {
  if (!fsMounted || storageFull || storageOk) return;
  if (millis() - lastStorageRetryMs < STORAGE_RETRY_INTERVAL_MS) return;
  FsLock lock;
  lastStorageRetryMs = millis();
  if (logFile) logFile.close();
  logFile = LittleFS.open(logPath, FILE_APPEND);
  storageOk = (bool)logFile;
  Serial.println(storageOk ? "Storage recovered" : "Storage still failing");
}

// After space has been freed, start a new session if logging had stopped for lack of room.
void resumeLoggingIfRoom() {
  if (!fsMounted || !storageFull) return;
  if (freeBytes() < MIN_FREE_BYTES * 2) {
    Serial.printf("Still low on space (%lu bytes free) - delete more to resume logging\n",
                  (unsigned long)freeBytes());
    return;
  }
  Serial.println("Space freed - starting a new session");
  startSession();
}

// Caller holds fsMutex.
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

// True for "cal_NNNN.csv" (with or without a leading slash); stores NNNN in *number.
bool parseSessionName(const char *name, uint32_t *number) {
  if (name[0] == '/') name++;
  if (strncmp(name, "cal_", 4) != 0) return false;
  char *end = nullptr;
  uint32_t n = (uint32_t)strtoul(name + 4, &end, 10);
  if (end == name + 4 || strcmp(end, ".csv") != 0) return false;
  if (number) *number = n;
  return true;
}

// Delete one session. The session being written is protected; once logging has
// stopped because flash is full its file is closed, so it can be deleted too.
DeleteResult deleteSession(uint32_t n) {
  FsLock lock;
  if (n == sessionNo && !storageFull) return DEL_CURRENT;
  char path[24];
  makeLogPath(path, sizeof(path), n);
  if (!LittleFS.remove(path)) return DEL_NOT_FOUND;
  Serial.printf("Deleted %s\n", path);
  resumeLoggingIfRoom();
  return DEL_OK;
}

// Delete every session except the one being written. Returns how many were removed.
int deleteAllSessions() {
  FsLock lock;
  int removed = 0;
  for (int pass = 0; pass < 256; pass++) {
    char victim[32] = "";
    File root = LittleFS.open("/");
    File f = root.openNextFile();
    while (f) {
      const char *name = f.name();
      if (name[0] == '/') name++;
      uint32_t n;
      if (parseSessionName(name, &n) && (storageFull || n != sessionNo)) {
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
    removed++;
  }
  resumeLoggingIfRoom();
  return removed;
}

// Erase the whole filesystem and start a fresh session.
bool formatStorage() {
  FsLock lock;
  if (logFile) logFile.close();
  storageOk = false;
  Serial.println("Formatting...");
  LittleFS.end();
  bool ok = LittleFS.format();
  fsMounted = ok && LittleFS.begin(false);
  Serial.println(fsMounted ? "Format done" : "Format/mount FAILED");
  if (fsMounted) startSession();
  return fsMounted;
}

void pushLiveSample(uint32_t uptimeMs, const CartReading &cart, bool ltrFresh, uint32_t ltrRaw) {
  portENTER_CRITICAL(&liveMux);
  uint32_t s = liveSeq + 1;
  LiveSample &e = liveBuf[s % LIVE_SAMPLES];
  e.seq = s;
  e.uptimeMs = uptimeMs;
  e.cartRaw = cart.ok ? (int16_t)cart.raw : (int16_t)-1;
  e.ltrRaw = ltrFresh ? (int32_t)ltrRaw : (int32_t)-1;
  e.bus = cart.busOk ? 1 : 0;
  liveSeq = s;
  portEXIT_CRITICAL(&liveMux);
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

  FsLock lock;
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

  pushLiveSample(uptimeMs, cart, ltrFresh, ltrRaw);
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
  Serial.println("  delete <n|all>     delete a session (not the one being written)");
  Serial.println("  format             erase all logs and start a new session");
  Serial.println("  interval <ms|0>    change the sample interval (saved; 0 = config.h default)");
  Serial.println("  live <on|off>      echo each row to serial");
  Serial.println("The Wi-Fi web app offers all of this too - see the README.");
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
  Serial.println(webDescribe());
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
  {
    FsLock lock;
    if (n == sessionNo && logFile && storageOk) logFile.flush();
  }
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
    Serial.printf("%d file(s) deleted\n", deleteAllSessions());
    return;
  }
  switch (deleteSession((uint32_t)strtoul(arg, nullptr, 10))) {
    case DEL_OK: break;  // deleteSession already printed the name
    case DEL_CURRENT: Serial.println("That is the session being written"); break;
    case DEL_NOT_FOUND: Serial.println("No such file"); break;
  }
}

void cmdFormat() {
  formatStorage();
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
    if (!arg) {
      Serial.printf("Interval %lu ms\n", (unsigned long)sampleIntervalMs);
    } else if (setSampleInterval((uint32_t)strtoul(arg, nullptr, 10))) {
      Serial.printf("Interval %lu ms\n", (unsigned long)sampleIntervalMs);
    } else {
      Serial.printf("interval <ms>: %lu-%lu, or 0 for the default\n", (unsigned long)MIN_INTERVAL_MS,
                    (unsigned long)MAX_INTERVAL_MS);
    }
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
  fsMutex = xSemaphoreCreateRecursiveMutex();
  Serial.begin(SERIAL_BAUD);
  delay(300);
  Serial.println();
  Serial.println("Ojo del Sol Calibrator (cartridge reading untested) - type 'help' for commands");
  setLed(0, 0, 255);

  cartBusBegin();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ltrReady = initLtr();
  Serial.println(ltrReady ? "LTR390 ready" : "LTR390 not found - will keep retrying; cartridge only for now");

  Serial.printf("Flash chip: %lu MB\n", (unsigned long)(ESP.getFlashChipSize() / 1048576));
  if (ESP.getFlashChipSize() < 16UL * 1048576UL) {
    Serial.println("WARNING: partitions.csv assumes a 16 MB flash chip; this chip is smaller, so the log "
                   "partition will not mount. Edit or delete partitions.csv (see the README).");
  }

  loadIntervalSetting();
  fsMounted = LittleFS.begin(false);
  if (fsMounted) {
    startSession();
  } else {
    // Normal on the very first run: the log partition has never been formatted.
    Serial.println("Flash not mounted. On a first run this is expected: press 'Format storage' in the "
                   "web app's Storage tab, or send 'format' here (this erases any logs).");
  }

  webBegin();   // Wi-Fi + web app (WebApp.ino)

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
