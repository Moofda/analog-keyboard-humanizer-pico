#include "xinput_device.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

static volatile bool _device_ready = false;

// XInput uses vendor-specific USB class with specific endpoint configuration
// Endpoint 1 IN: gamepad reports to PC (interrupt, 20 bytes, 1ms interval = 1000hz)
// Endpoint 1 OUT: rumble commands from PC (we ignore these)
#define XINPUT_EP_IN  0x81
#define XINPUT_EP_OUT 0x01

void xinput_device_init(void) {
    _device_ready = false;
}

bool xinput_device_ready(void) {
    return _device_ready && tud_ready();
}

void xinput_device_send_report(const XInputReport *report) {
    if (!xinput_device_ready()) return;

    // Send via vendor endpoint (endpoint 1 IN for XInput)
    // TinyUSB vendor interface maps to our XInput endpoint
    if (tud_vendor_write_available() >= sizeof(XInputReport)) {
        tud_vendor_write((const uint8_t*)report, sizeof(XInputReport));
        tud_vendor_flush();
    }
}

// ── TinyUSB device callbacks ──────────────────────────────────────────────────

void tud_mount_cb(void) {
    printf("[DEVICE] Mounted to PC\n");
    _device_ready = true;
}

void tud_umount_cb(void) {
    printf("[DEVICE] Unmounted from PC\n");
    _device_ready = false;
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    _device_ready = false;
}

void tud_resume_cb(void) {
    _device_ready = true;
}

// Handle XInput-specific control requests from PC
// Windows sends capability queries to identify Xbox 360 controllers
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                  tusb_control_request_t const *request)
{
    if (stage != CONTROL_STAGE_SETUP) return true;

    // XInput capability request — respond with Xbox 360 gamepad capabilities
    if (request->bmRequestType == 0xC1 &&
        request->bRequest == 0x01 &&
        request->wValue == 0x0100)
    {
        // Xbox 360 gamepad capabilities response
        static const uint8_t capabilities[] = {
            0x00,        // Report ID
            0x14,        // Size = 20
            0xFF, 0xFF,  // All buttons supported
            0xFF,        // Left trigger
            0xFF,        // Right trigger
            0xFF, 0x7F,  // Left X
            0xFF, 0x7F,  // Left Y
            0xFF, 0x7F,  // Right X
            0xFF, 0x7F,  // Right Y
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Reserved
        };
        return tud_control_xfer(rhport, request, (void*)capabilities,
                                sizeof(capabilities));
    }

    // LED assignment request — acknowledge but ignore
    if (request->bmRequestType == 0x41 && request->bRequest == 0x01) {
        return tud_control_status(rhport, request);
    }

    return false;
}
