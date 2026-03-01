// XboxHIDGamepad.h - Xbox 360 XInput USB gamepad emulation for ESP32-S3
//
// Presents the device as a genuine Xbox 360 wired controller via TinyUSB.
// Uses vendor-specific USB descriptors (NOT standard HID) so that Windows
// loads the XInput driver automatically.
//
// Based on:
//   - https://github.com/Kalasoiro/esp32s3-tinyusb-xinput
//   - https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/
//
// NOTE: This overrides TinyUSB descriptor callbacks. Normal boot uses XInput.
// A runtime RTC flag can switch descriptors to CDC for firmware upload mode.

#pragma once

#if defined(ARDUINO_USB_MODE) && !ARDUINO_USB_MODE

#include <stdint.h>
#include <string.h>
#include "tusb.h"
#include "device/usbd.h"
#include "device/usbd_pvt.h"
#include "esp_attr.h"

// ============================================================================
// CDC mode flag — persists across software resets and deep sleep.
// On first power-on the magic won't match, defaulting to XInput mode.
// ============================================================================
static const uint32_t CDC_MODE_MAGIC = 0xCDC0BEEF;
RTC_NOINIT_ATTR static uint32_t _cdcModeMagic;
RTC_NOINIT_ATTR static bool _cdcModeFlag;

static inline bool isXInputCdcMode() {
  return (_cdcModeMagic == CDC_MODE_MAGIC) && _cdcModeFlag;
}

static inline void setCdcModeFlag() {
  _cdcModeMagic = CDC_MODE_MAGIC;
  _cdcModeFlag = true;
}

static inline void clearCdcModeFlag() {
  _cdcModeMagic = 0;
  _cdcModeFlag = false;
}

// ============================================================================
// Xbox 360 XInput button constants
// 16-bit word: low byte = report byte 2, high byte = report byte 3
// ============================================================================

// Report byte 2 (digital_buttons_1)
#define XINPUT_GAMEPAD_DPAD_UP    0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN  0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT  0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT 0x0008
#define XINPUT_GAMEPAD_START      0x0010
#define XINPUT_GAMEPAD_BACK       0x0020
#define XINPUT_GAMEPAD_LS         0x0040  // Left Stick click (L3)
#define XINPUT_GAMEPAD_RS         0x0080  // Right Stick click (R3)

// Report byte 3 (digital_buttons_2)
#define XINPUT_GAMEPAD_LB         0x0100  // Left Bumper
#define XINPUT_GAMEPAD_RB         0x0200  // Right Bumper
#define XINPUT_GAMEPAD_GUIDE      0x0400  // Xbox Guide button
// Bit 3 (0x0800) is unused
#define XINPUT_GAMEPAD_A          0x1000
#define XINPUT_GAMEPAD_B          0x2000
#define XINPUT_GAMEPAD_X          0x4000
#define XINPUT_GAMEPAD_Y          0x8000

// ============================================================================
// BLE-to-XInput button mapping
// ============================================================================
// Maps BleCompositeHID XBOX_BUTTON_* constants to Xbox 360 USB report bits.
// Only buttons actually used in the USB path need mapping; extend as needed.
//
// Known BLE library values (from XboxGamepadDevice / BleCompositeHID):
//   XBOX_BUTTON_LS = 0x2000,  XBOX_BUTTON_RS = 0x4000
static inline uint16_t bleButtonToXInput(uint16_t ble) {
  uint16_t x = 0;
  if (ble & 0x2000) x |= XINPUT_GAMEPAD_LS;
  if (ble & 0x4000) x |= XINPUT_GAMEPAD_RS;
  return x;
}

// ============================================================================
// Xbox 360 report structure (20 bytes, sent on EP1 IN)
// ============================================================================
typedef struct __attribute__((packed)) {
  uint8_t  report_id;        // Always 0x00
  uint8_t  report_size;      // Always 0x14 (20)
  uint8_t  buttons_lo;       // D-pad, Start, Back, L3, R3
  uint8_t  buttons_hi;       // LB, RB, Guide, (unused), A, B, X, Y
  uint8_t  trigger_left;     // 0-255
  uint8_t  trigger_right;    // 0-255
  int16_t  stick_left_x;     // -32768 to 32767 (LE)
  int16_t  stick_left_y;
  int16_t  stick_right_x;
  int16_t  stick_right_y;
  uint8_t  reserved[6];      // Always 0x00
} XInputReport_t;

// ============================================================================
// USB descriptors - exact Xbox 360 wired controller values
// ============================================================================

static const tusb_desc_device_t xinput_device_desc = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0xFF,
  .bDeviceSubClass    = 0xFF,
  .bDeviceProtocol    = 0xFF,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = 0x045E,   // Microsoft Corp.
  .idProduct          = 0x028E,   // Xbox360 Controller
  .bcdDevice          = 0x0114,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 0x01
};

// Interface 0 only (control data). Total = 9 + 9 + 17 + 7 + 7 = 49 bytes.
static const uint8_t xinput_config_desc[] = {
  // Configuration Descriptor
  0x09, 0x02,
  0x31, 0x00,     // wTotalLength (49)
  0x01,           // bNumInterfaces
  0x01,           // bConfigurationValue
  0x00,           // iConfiguration
  0xA0,           // bmAttributes (bus-powered, remote wakeup)
  0xFA,           // bMaxPower (500 mA)

  // Interface 0: Control Data
  0x09, 0x04,
  0x00,           // bInterfaceNumber
  0x00,           // bAlternateSetting
  0x02,           // bNumEndpoints
  0xFF,           // bInterfaceClass (vendor specific)
  0x5D,           // bInterfaceSubClass
  0x01,           // bInterfaceProtocol
  0x00,           // iInterface

  // Vendor-specific descriptor (real Xbox 360 values, 17 bytes)
  0x11, 0x21,
  0x00, 0x01, 0x01, 0x25,
  0x81,           // bEndpointAddress (IN, 1)
  0x14,           // bMaxDataSize (20)
  0x00, 0x00, 0x00, 0x00, 0x13,
  0x01,           // bEndpointAddress (OUT, 1)
  0x08,           // bMaxDataSize (8)
  0x00, 0x00,

  // Endpoint 1 IN: Control Data Send
  0x07, 0x05,
  0x81,           // bEndpointAddress (IN, 1)
  0x03,           // bmAttributes (Interrupt)
  0x20, 0x00,     // wMaxPacketSize (32)
  0x04,           // bInterval (4 ms → 250 Hz)

  // Endpoint 1 OUT: Control Data Receive (rumble/LED)
  0x07, 0x05,
  0x01,           // bEndpointAddress (OUT, 1)
  0x03,           // bmAttributes (Interrupt)
  0x20, 0x00,     // wMaxPacketSize (32)
  0x08,           // bInterval (8 ms → 125 Hz)
};

static const char* xinput_string_desc[] = {
  (const char[]){0x09, 0x04},   // 0: Language ID (English US)
  "Ojo del Sol",                // 1: Manufacturer
  "Ojo del Sol",                // 2: Product
  "0000001"                     // 3: Serial
};

// ============================================================================
// CDC mode USB descriptors (used when CDC flag is set for firmware upload)
// ============================================================================

static const tusb_desc_device_t cdc_device_desc = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0xEF,    // Misc (IAD)
  .bDeviceSubClass    = 0x02,    // Common Class
  .bDeviceProtocol    = 0x01,    // IAD
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = 0x303A,  // Espressif
  .idProduct          = 0x1001,  // ESP32-S3 generic
  .bcdDevice          = 0x0100,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 0x01
};

#define CDC_EPNUM_NOTIF  0x81
#define CDC_EPNUM_OUT    0x02
#define CDC_EPNUM_IN     0x82

static const uint8_t cdc_config_desc[] = {
  TUD_CONFIG_DESCRIPTOR(1, 2, 0, TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN, 0x00, 500),
  TUD_CDC_DESCRIPTOR(0, 4, CDC_EPNUM_NOTIF, 8, CDC_EPNUM_OUT, CDC_EPNUM_IN, 64),
};

static const char* cdc_string_desc[] = {
  (const char[]){0x09, 0x04},   // 0: Language ID (English US)
  "Ojo del Sol",                // 1: Manufacturer
  "Ojo del Sol (CDC)",          // 2: Product
  "0000001",                    // 3: Serial
  "CDC Serial"                  // 4: CDC Interface
};

// ============================================================================
// TinyUSB descriptor callbacks (override weak definitions in Arduino core)
// ============================================================================

extern "C" {

const uint8_t* tud_descriptor_device_cb(void) {
  if (isXInputCdcMode()) return (const uint8_t*)&cdc_device_desc;
  return (const uint8_t*)&xinput_device_desc;
}

const uint8_t* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  if (isXInputCdcMode()) return cdc_config_desc;
  return xinput_config_desc;
}

static uint16_t _xinput_desc_str_buf[33];

const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  bool cdc = isXInputCdcMode();
  const char* const* desc = cdc ? cdc_string_desc : xinput_string_desc;
  uint8_t desc_count = cdc ? 5 : 4;
  uint8_t chr_count;

  if (index == 0) {
    memcpy(&_xinput_desc_str_buf[1], desc[0], 2);
    chr_count = 1;
  } else {
    if (index >= desc_count)
      return NULL;

    const char* str = desc[index];
    chr_count = strlen(str);
    if (chr_count > 31) chr_count = 31;

    for (uint8_t i = 0; i < chr_count; i++) {
      _xinput_desc_str_buf[1 + i] = str[i];
    }
  }

  _xinput_desc_str_buf[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
  return _xinput_desc_str_buf;
}

} // extern "C"

// ============================================================================
// XInput USB class driver
// ============================================================================

static const uint8_t XINPUT_EP_IN = 0x81;

static void xinput_driver_init(void) {}

static void xinput_driver_reset(uint8_t rhport) {
  (void)rhport;
}

static uint16_t xinput_driver_open(uint8_t rhport,
                                   tusb_desc_interface_t const* itf_desc,
                                   uint16_t max_len) {
  // 17 bytes for the vendor-specific descriptor between interface and endpoints
  const uint16_t drv_len = (uint16_t)(
    sizeof(tusb_desc_interface_t) +
    itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t) +
    17
  );
  TU_VERIFY(max_len >= drv_len, 0);

  const uint8_t* p_desc = tu_desc_next(itf_desc);
  uint8_t found_endpoints = 0;
  while (found_endpoints < itf_desc->bNumEndpoints) {
    if (TUSB_DESC_ENDPOINT == tu_desc_type((const tusb_desc_endpoint_t*)p_desc)) {
      TU_ASSERT(usbd_edpt_open(rhport, (const tusb_desc_endpoint_t*)p_desc));
      found_endpoints++;
    }
    p_desc = tu_desc_next(p_desc);
  }

  return drv_len;
}

static bool xinput_driver_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                          tusb_control_request_t const* request) {
  (void)rhport; (void)stage; (void)request;
  return true;
}

static bool xinput_driver_xfer_cb(uint8_t rhport, uint8_t ep_addr,
                                  xfer_result_t result, uint32_t xferred_bytes) {
  (void)rhport; (void)ep_addr; (void)result; (void)xferred_bytes;
  return true;
}

static const usbd_class_driver_t xinput_class_driver = {
#if CFG_TUSB_DEBUG >= 2
  .name             = "XINPUT",
#endif
  .init             = xinput_driver_init,
  .reset            = xinput_driver_reset,
  .open             = xinput_driver_open,
  .control_xfer_cb  = xinput_driver_control_xfer_cb,
  .xfer_cb          = xinput_driver_xfer_cb,
  .sof              = NULL
};

extern "C" const usbd_class_driver_t* usbd_app_driver_get_cb(uint8_t* driver_count) {
  if (isXInputCdcMode()) {
    *driver_count = 0;
    return NULL;
  }
  *driver_count = 1;
  return &xinput_class_driver;
}

// ============================================================================
// XboxHIDGamepad - high-level API
// ============================================================================

class XboxHIDGamepad {
public:
  XboxHIDGamepad() {
    memset(&_report, 0, sizeof(_report));
    _report.report_id   = 0x00;
    _report.report_size = 0x14;
  }

  void begin() {
    // Descriptors and driver are registered via TinyUSB callback overrides
    // at link time. No runtime registration needed.
  }

  /// Send a full Xbox 360 controller state report.
  /// @param buttons  XInput button bitmask (XINPUT_GAMEPAD_* ORed together)
  /// @param triggerL Left trigger 0-255
  /// @param triggerR Right trigger 0-255
  /// @param lx,ly    Left stick (-32768 to 32767)
  /// @param rx,ry    Right stick (-32768 to 32767)
  /// @return true if the report was queued for transmission
  bool sendReport(uint16_t buttons, uint8_t triggerL, uint8_t triggerR,
                  int16_t lx, int16_t ly, int16_t rx, int16_t ry) {
    if (isXInputCdcMode()) return false;
    _report.report_id    = 0x00;
    _report.report_size  = 0x14;
    _report.buttons_lo   = (uint8_t)(buttons & 0xFF);
    _report.buttons_hi   = (uint8_t)((buttons >> 8) & 0xFF);
    _report.trigger_left  = triggerL;
    _report.trigger_right = triggerR;
    _report.stick_left_x  = lx;
    _report.stick_left_y  = ly;
    _report.stick_right_x = rx;
    _report.stick_right_y = ry;
    memset(_report.reserved, 0, sizeof(_report.reserved));

    if (tud_suspended()) {
      tud_remote_wakeup();
    }

    if (!tud_ready() || usbd_edpt_busy(0, XINPUT_EP_IN))
      return false;

    if (!usbd_edpt_claim(0, XINPUT_EP_IN))
      return false;

    usbd_edpt_xfer(0, XINPUT_EP_IN, (uint8_t*)&_report, sizeof(XInputReport_t), false);
    usbd_edpt_release(0, XINPUT_EP_IN);
    return true;
  }

private:
  XInputReport_t _report;
};

#endif // ARDUINO_USB_MODE check
