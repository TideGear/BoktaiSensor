// Shared.h - types and locking shared by the sampling code (OjoDelSolCalibrator.ino)
// and the Wi-Fi web app (WebApp.ino).
#ifndef CALIBRATOR_SHARED_H
#define CALIBRATOR_SHARED_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Decimal text for a 64-bit value (avoids relying on %llu support in printf).
static inline void fmtU64(char *out, uint64_t v) {
  char tmp[21];
  int i = 0;
  do { tmp[i++] = (char)('0' + (v % 10)); v /= 10; } while (v);
  int j = 0;
  while (i) out[j++] = tmp[--i];
  out[j] = '\0';
}

enum DeleteResult { DEL_OK, DEL_NOT_FOUND, DEL_CURRENT };

// One entry of the in-RAM ring buffer behind the web app's live view.
// cartRaw / ltrRaw are -1 when that sensor gave no reading.
struct LiveSample {
  uint32_t seq;
  uint32_t uptimeMs;
  int16_t cartRaw;
  int32_t ltrRaw;
  uint8_t bus;      // 1 if the cartridge's GPIO registers read back what was written
};
const int LIVE_SAMPLES = 600;

// The sampling loop (core 1) and the web server task (core 0) both touch the log
// file, the session state and the file operations. Take this around anything that does.
extern SemaphoreHandle_t fsMutex;
class FsLock {
 public:
  FsLock() { xSemaphoreTakeRecursive(fsMutex, portMAX_DELAY); }
  ~FsLock() { xSemaphoreGiveRecursive(fsMutex); }
};

#endif
