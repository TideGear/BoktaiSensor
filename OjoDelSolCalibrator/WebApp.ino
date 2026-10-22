// WebApp.ino - Wi-Fi (your network, or a hotspot with captive portal as the fallback),
// the mDNS name and the web app's HTTP API.
//
// Runs in its own task on core 0 (the sampling loop stays on core 1), which also does
// the Wi-Fi connecting, so waiting for the network never delays sampling. The page
// itself is in WebPage.h and is entirely self-contained, so nothing needs internet access.
//
//   GET  /                      the web app
//   GET  /api/status            device state as JSON
//   GET  /api/live?since=N      samples newer than sequence number N (RAM ring buffer);
//                               each is [seq, uptime_ms, cart_raw|-1, ltr_raw|-1, bus_ok]
//   GET  /api/files             session files on flash
//   GET  /api/file?n=N          download session N as CSV
//   POST /api/delete?n=N | ?all=1
//   POST /api/newsession
//   POST /api/format?confirm=ERASE
//   POST /api/interval?ms=N     (0 = default from config.h)
//   POST /api/time?ms=EPOCH_MS  set the wall clock (the page sends the phone's time
//                               unless internet time is already in use)
//   POST /api/upload[?overwrite=1]   multipart upload of a cal_NNNN.csv (restore)
//
// Joining your Wi-Fi, the .local name, internet time and the page/status/live APIs work
// on the board; the hotspot fallback, downloads and uploads are untested. See the README.

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "WebPage.h"

static WebServer webServer(80);
static DNSServer dnsServer;
static volatile bool webUp = false;
static bool apMode = false;   // true = running the hotspot, false = joined WIFI_STA_SSID
static IPAddress apIP;
static volatile bool ntpSyncPending = false;  // set by the SNTP callback, handled in webTask

// --- Upload (restore) state -------------------------------------------------------
static const char UPLOAD_TMP_PATH[] = "/upload.tmp";
static File uploadFile;
static char uploadFinalPath[32];
static String uploadError;
static uint32_t uploadBytes = 0;

static void sendJson(int code, const String &body) {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(code, "application/json", body);
}

static void sendJsonError(int code, const char *message) {
  String body = "{\"ok\":false,\"error\":\"";
  body += message;
  body += "\"}";
  sendJson(code, body);
}

static void sendOk() { sendJson(200, "{\"ok\":true}"); }

static const char *storageState() {
  if (!fsMounted) return "unmounted";
  if (storageFull) return "full";
  return storageOk ? "ok" : "error";
}

// --- Handlers ---------------------------------------------------------------------
static void handleRoot() {
  webServer.sendHeader("Cache-Control", "no-cache");
  webServer.send_P(200, "text/html", INDEX_HTML);
}

static void handleStatus() {
  FsLock lock;
  char epoch[24];
  fmtU64(epoch, nowEpochMs());
  char buf[640];
  snprintf(buf, sizeof(buf),
           "{\"session\":%lu,\"file\":\"%s\",\"rows\":%lu,\"storage\":\"%s\",\"flashBytes\":%lu,"
           "\"free\":%lu,\"total\":%lu,\"minFree\":%lu,"
           "\"uptimeMs\":%lu,\"intervalMs\":%lu,\"defaultIntervalMs\":%lu,"
           "\"minIntervalMs\":%lu,\"maxIntervalMs\":%lu,"
           "\"timeSynced\":%s,\"timeSource\":\"%s\",\"epochMs\":%s,\"ltrReady\":%s,\"seq\":%lu,\"clients\":%d}",
           (unsigned long)sessionNo, logPath, (unsigned long)rowsLogged, storageState(),
           (unsigned long)ESP.getFlashChipSize(),
           (unsigned long)(fsMounted ? freeBytes() : 0),
           (unsigned long)(fsMounted ? LittleFS.totalBytes() : 0),
           (unsigned long)MIN_FREE_BYTES, (unsigned long)millis(), (unsigned long)sampleIntervalMs,
           (unsigned long)SAMPLE_INTERVAL_MS, (unsigned long)MIN_INTERVAL_MS,
           (unsigned long)MAX_INTERVAL_MS, timeSynced ? "true" : "false", timeSource, epoch,
           ltrReady ? "true" : "false", (unsigned long)liveSeq,
           apMode ? (int)WiFi.softAPgetStationNum() : 0);
  sendJson(200, buf);
}

static void handleLive() {
  uint32_t since = (uint32_t)strtoul(webServer.arg("since").c_str(), nullptr, 10);
  uint32_t newest = liveSeq;
  if (since > newest) since = 0;                               // device restarted
  uint32_t first = since + 1;
  if (newest > LIVE_SAMPLES && first < newest - LIVE_SAMPLES + 1) first = newest - LIVE_SAMPLES + 1;

  String body;
  body.reserve(64 + (newest >= first ? (newest - first + 1) : 0) * 28);
  body = "{\"seq\":";
  body += newest;
  body += ",\"up\":";
  body += (unsigned long)millis();
  body += ",\"s\":[";
  bool firstEntry = true;
  for (uint32_t s = first; s <= newest && s != 0; s++) {
    LiveSample e;
    portENTER_CRITICAL(&liveMux);
    e = liveBuf[s % LIVE_SAMPLES];
    portEXIT_CRITICAL(&liveMux);
    if (e.seq != s) continue;                                  // overwritten meanwhile
    if (!firstEntry) body += ',';
    firstEntry = false;
    body += '[';
    body += e.seq;
    body += ',';
    body += e.uptimeMs;
    body += ',';
    body += e.cartRaw;
    body += ',';
    body += e.ltrRaw;
    body += ',';
    body += (int)e.bus;
    body += ']';
  }
  body += "]}";
  sendJson(200, body);
}

static void handleFiles() {
  FsLock lock;
  String body = "{\"files\":[";
  bool firstEntry = true;
  if (fsMounted) {
    File root = LittleFS.open("/");
    File f = root.openNextFile();
    while (f) {
      uint32_t n;
      if (parseSessionName(f.name(), &n)) {
        if (!firstEntry) body += ',';
        firstEntry = false;
        body += "{\"n\":";
        body += n;
        body += ",\"size\":";
        body += (unsigned long)f.size();
        body += ",\"current\":";
        body += (n == sessionNo && !storageFull) ? "true" : "false";
        body += '}';
      }
      f = root.openNextFile();
    }
    root.close();
  }
  body += "],\"free\":";
  body += (unsigned long)(fsMounted ? freeBytes() : 0);
  body += ",\"total\":";
  body += (unsigned long)(fsMounted ? LittleFS.totalBytes() : 0);
  body += '}';
  sendJson(200, body);
}

static void handleFileDownload() {
  uint32_t n = (uint32_t)strtoul(webServer.arg("n").c_str(), nullptr, 10);
  if (n == 0) { sendJsonError(400, "Missing session number"); return; }
  char path[24];
  makeLogPath(path, sizeof(path), n);

  File f;
  size_t size = 0;
  {
    FsLock lock;
    if (!fsMounted) { sendJsonError(503, "Flash not mounted"); return; }
    if (n == sessionNo && logFile && storageOk) logFile.flush();   // make buffered rows visible
    f = LittleFS.open(path, FILE_READ);
    if (f) size = f.size();                                        // exact length promised below
  }
  if (!f) { sendJsonError(404, "No such file"); return; }

  webServer.setContentLength(size);
  webServer.sendHeader("Content-Disposition", String("attachment; filename=\"") + (path + 1) + "\"");
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "text/csv", "");

  static uint8_t buf[1024];
  size_t remaining = size;
  while (remaining > 0) {
    size_t want = remaining < sizeof(buf) ? remaining : sizeof(buf);
    int got = f.read(buf, want);
    if (got <= 0) break;
    webServer.sendContent((const char *)buf, (size_t)got);
    remaining -= (size_t)got;
  }
  f.close();
}

static void handleDelete() {
  if (!fsMounted) { sendJsonError(503, "Flash not mounted"); return; }
  if (webServer.hasArg("all")) {
    int removed = deleteAllSessions();
    sendJson(200, String("{\"ok\":true,\"deleted\":") + removed + "}");
    return;
  }
  uint32_t n = (uint32_t)strtoul(webServer.arg("n").c_str(), nullptr, 10);
  if (n == 0) { sendJsonError(400, "Missing session number"); return; }
  switch (deleteSession(n)) {
    case DEL_OK: sendOk(); break;
    case DEL_CURRENT: sendJsonError(409, "That session is still being written"); break;
    case DEL_NOT_FOUND: sendJsonError(404, "No such file"); break;
  }
}

static void handleNewSession() {
  if (!fsMounted) { sendJsonError(503, "Flash not mounted"); return; }
  FsLock lock;
  if (freeBytes() < MIN_FREE_BYTES * 2) { sendJsonError(507, "Not enough free space"); return; }
  startSession();
  sendJson(200, String("{\"ok\":true,\"session\":") + sessionNo + "}");
}

static void handleFormat() {
  if (webServer.arg("confirm") != "ERASE") { sendJsonError(400, "Confirmation missing"); return; }
  if (formatStorage()) sendOk(); else sendJsonError(500, "Format failed");
}

static void handleInterval() {
  uint32_t ms = (uint32_t)strtoul(webServer.arg("ms").c_str(), nullptr, 10);
  if (!setSampleInterval(ms)) { sendJsonError(400, "Interval out of range"); return; }
  sendJson(200, String("{\"ok\":true,\"intervalMs\":") + sampleIntervalMs + "}");
}

static void handleTime() {
  unsigned long long ms = strtoull(webServer.arg("ms").c_str(), nullptr, 10);
  if (ms < 1600000000000ULL) { sendJsonError(400, "Implausible time"); return; }   // before 2020
  setTimeSync((uint64_t)ms, "browser");
  sendOk();
}

// Multipart upload callback: called repeatedly with chunks of the file.
static void handleUploadData() {
  HTTPUpload &up = webServer.upload();
  if (up.status == UPLOAD_FILE_START) {
    uploadError = "";
    uploadBytes = 0;
    uploadFinalPath[0] = '\0';
    uploadFile = File();
    uint32_t n = 0;
    if (!fsMounted) { uploadError = "Flash not mounted"; return; }
    if (!parseSessionName(up.filename.c_str(), &n) || n == 0) {
      uploadError = "File name must look like cal_0001.csv";
      return;
    }
    makeLogPath(uploadFinalPath, sizeof(uploadFinalPath), n);
    FsLock lock;
    if (n == sessionNo && !storageFull) { uploadError = "That session is being written"; return; }
    if (LittleFS.exists(uploadFinalPath) && webServer.arg("overwrite") != "1") {
      uploadError = "Already exists";
      return;
    }
    uploadFile = LittleFS.open(UPLOAD_TMP_PATH, FILE_WRITE);
    if (!uploadFile) uploadError = "Could not create file";
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (uploadError.length() || !uploadFile) return;
    FsLock lock;
    if (freeBytes() < MIN_FREE_BYTES * 2 + up.currentSize) {
      uploadError = "Not enough free space";
      uploadFile.close();
      LittleFS.remove(UPLOAD_TMP_PATH);
      return;
    }
    if (uploadFile.write(up.buf, up.currentSize) != up.currentSize) {
      uploadError = "Write failed";
      uploadFile.close();
      LittleFS.remove(UPLOAD_TMP_PATH);
      return;
    }
    uploadBytes += up.currentSize;
  } else if (up.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      FsLock lock;
      if (uploadError.length() == 0) {
        LittleFS.remove(uploadFinalPath);
        if (!LittleFS.rename(UPLOAD_TMP_PATH, uploadFinalPath)) uploadError = "Could not save file";
      }
      if (uploadError.length()) LittleFS.remove(UPLOAD_TMP_PATH);
    }
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) uploadFile.close();
    FsLock lock;
    LittleFS.remove(UPLOAD_TMP_PATH);
    uploadError = "Upload aborted";
  }
}

static void handleUploadDone() {
  if (uploadError.length() == 0 && uploadFinalPath[0] == '\0') uploadError = "No file received";
  if (uploadError.length()) {
    String message = uploadError;
    message.replace("\"", "'");
    sendJsonError(uploadError == "Already exists" ? 409 : 400, message.c_str());
    return;
  }
  sendJson(200, String("{\"ok\":true,\"name\":\"") + (uploadFinalPath + 1) + "\",\"bytes\":" + uploadBytes + "}");
}

// On the hotspot, anything else (including phones' connectivity checks) goes to the
// app: captive portal. On your own network there is no portal, so it is a plain 404.
static void handleNotFound() {
  if (!apMode) {
    webServer.send(404, "text/plain", "Not found");
    return;
  }
  webServer.sendHeader("Location", String("http://") + apIP.toString() + "/", true);
  webServer.send(302, "text/plain", "");
}

static void handleFavicon() { webServer.send(204, "text/plain", ""); }

// Join WIFI_STA_SSID (from logins.h). Returns false if there is none or it fails.
static bool joinWifi() {
  if (WIFI_STA_SSID[0] == '\0') {
    Serial.println("No logins.h - using the hotspot");
    return false;
  }
  WiFi.setHostname(MDNS_HOSTNAME);   // Must come before WiFi.mode()
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
  WiFi.setSleep(true);               // Modem sleep: the radio naps between router beacons
  Serial.printf("Joining Wi-Fi \"%s\"...\n", WIFI_STA_SSID);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_STA_CONNECT_TIMEOUT_MS) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  if (WiFi.status() == WL_CONNECTED) return true;

  Serial.printf("Could not join \"%s\" within %lu ms - starting the hotspot instead\n",
                WIFI_STA_SSID, (unsigned long)WIFI_STA_CONNECT_TIMEOUT_MS);
  WiFi.disconnect(true);
  return false;
}

static bool startHotspot() {
  size_t passLen = strlen(WIFI_AP_PASSWORD);
  const char *pass = WIFI_AP_PASSWORD;
  if (passLen > 0 && passLen < 8) {
    Serial.println("WIFI_AP_PASSWORD must be 8-63 characters - starting an OPEN network instead");
    pass = nullptr;
  } else if (passLen == 0) {
    pass = nullptr;
  }

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(WIFI_AP_SSID, pass, WIFI_AP_CHANNEL, WIFI_AP_HIDDEN, WIFI_AP_MAX_CLIENTS)) {
    Serial.println("Wi-Fi hotspot failed to start");
    return false;
  }
  apIP = WiFi.softAPIP();
  dnsServer.start(53, "*", apIP);
  apMode = true;
  Serial.printf("Wi-Fi hotspot \"%s\" (%s) - open http://%s or http://%s.local\n", WIFI_AP_SSID,
                pass ? "password protected" : "OPEN", apIP.toString().c_str(), MDNS_HOSTNAME);
  return true;
}

// Runs in the network stack's task whenever a time server has set the system clock.
// Only flags it: setTimeSync() takes the file lock, which must not block networking.
static void onNtpSync(struct timeval *tv) {
  (void)tv;
  ntpSyncPending = true;
}

// Ask the NTP servers for UTC. lwIP moves on to the next server when one doesn't
// answer, and re-syncs every 3 hours (CONFIG_LWIP_SNTP_UPDATE_DELAY).
static void startNtp() {
  sntp_set_time_sync_notification_cb(onNtpSync);
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
  Serial.printf("Getting the time from %s (fallbacks %s, %s)\n", NTP_SERVER_1, NTP_SERVER_2,
                NTP_SERVER_3);
}

// Copy the freshly set system clock into the log's wall-clock time
static void applyNtpSync() {
  ntpSyncPending = false;
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  uint64_t epochMs = (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)(tv.tv_usec / 1000);
  if (epochMs < 1600000000000ULL) return;   // Implausible (before 2020)
  setTimeSync(epochMs, "ntp");
  Serial.println("Clock set from internet time");
}

static void webTask(void *) {
  if (joinWifi()) {
    Serial.printf("Wi-Fi \"%s\" joined - open http://%s.local or http://%s\n", WIFI_STA_SSID,
                  MDNS_HOSTNAME, WiFi.localIP().toString().c_str());
    startNtp();
  } else if (!startHotspot()) {
    vTaskDelete(nullptr);
    return;
  }

  // The name answers on whichever network is up
  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("mDNS failed to start - use the IP address instead");
  }

  webServer.begin();
  webUp = true;
  for (;;) {
    if (apMode) dnsServer.processNextRequest();
    if (ntpSyncPending) applyNtpSync();
    webServer.handleClient();
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

// --- Entry points (called from OjoDelSolCalibrator.ino) ----------------------------------
void webBegin() {
  if (!WIFI_ENABLED) {
    Serial.println("Wi-Fi disabled in config.h");
    return;
  }

  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/api/status", HTTP_GET, handleStatus);
  webServer.on("/api/live", HTTP_GET, handleLive);
  webServer.on("/api/files", HTTP_GET, handleFiles);
  webServer.on("/api/file", HTTP_GET, handleFileDownload);
  webServer.on("/api/delete", HTTP_POST, handleDelete);
  webServer.on("/api/newsession", HTTP_POST, handleNewSession);
  webServer.on("/api/format", HTTP_POST, handleFormat);
  webServer.on("/api/interval", HTTP_POST, handleInterval);
  webServer.on("/api/time", HTTP_POST, handleTime);
  webServer.on("/api/upload", HTTP_POST, handleUploadDone, handleUploadData);
  webServer.on("/favicon.ico", HTTP_GET, handleFavicon);
  webServer.onNotFound(handleNotFound);

  // Connecting (up to WIFI_STA_CONNECT_TIMEOUT_MS) happens in the task, off the sampling core
  xTaskCreatePinnedToCore(webTask, "web", 10240, nullptr, 1, nullptr, 0);
}

String webDescribe() {
  if (!WIFI_ENABLED) return String("Wi-Fi: off");
  if (!webUp) return String("Wi-Fi: starting");
  if (apMode) {
    return String("Wi-Fi hotspot \"") + WIFI_AP_SSID + "\" at http://" + apIP.toString() + " or http://" +
           MDNS_HOSTNAME + ".local, " + WiFi.softAPgetStationNum() + " client(s)";
  }
  return String("Wi-Fi \"") + WIFI_STA_SSID + "\" at http://" + MDNS_HOSTNAME + ".local or http://" +
         WiFi.localIP().toString() + (WiFi.status() == WL_CONNECTED ? "" : " (reconnecting)");
}
