#include "usb_host.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

// XInput device identification
// Your keyboard presents as Xbox 360 controller
#define XINPUT_VID_MICROSOFT  0x045E
#define XINPUT_PID_XBOX360    0x028E  // Xbox 360 controller
#define XINPUT_INTERFACE_SUBCLASS 0x5D
#define XINPUT_INTERFACE_PROTOCOL 0x01

// XInput report is always 20 bytes
#define XINPUT_REPORT_SIZE 20

static volatile bool     _device_connected = false;
static volatile bool     _new_report        = false;
static XInputReport      _latest_report     = {0};
static XInputReport      _pending_report    = {0};

// Mutex for report access between cores
static volatile uint32_t _report_lock = 0;

static inline void lock_acquire(void) {
    while (__sync_lock_test_and_set(&_report_lock, 1)) { tight_loop_contents(); }
}

static inline void lock_release(void) {
    __sync_lock_release(&_report_lock);
}

void usb_host_init(void) {
    memset(&_latest_report, 0, sizeof(XInputReport));
    memset(&_pending_report, 0, sizeof(XInputReport));
    _device_connected = false;
    _new_report = false;
}

void usb_host_task(void) {
    tuh_task();
}

bool usb_host_device_connected(void) {
    return _device_connected;
}

bool usb_host_get_report(XInputReport *report) {
    if (!_new_report) return false;

    lock_acquire();
    memcpy(report, (const void*)&_latest_report, sizeof(XInputReport));
    _new_report = false;
    lock_release();

    return true;
}

// ── TinyUSB host callbacks ────────────────────────────────────────────────────

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *desc_report, uint16_t desc_len)
{
    (void)desc_report;
    (void)desc_len;

    // Check if this is an XInput device
    tuh_vid_pid_t vid_pid;
    tuh_vid_pid_get(dev_addr, &vid_pid);

    printf("[HOST] Device mounted: VID=0x%04X PID=0x%04X\n",
           vid_pid.vid, vid_pid.pid);

    // Accept any HID device as potential XInput controller
    // XInput devices may not enumerate as standard HID but we try anyway
    _device_connected = true;

    // Start receiving reports
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        printf("[HOST] Failed to request report\n");
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
    printf("[HOST] Device unmounted\n");
    _device_connected = false;
    _new_report = false;
    memset((void*)&_latest_report, 0, sizeof(XInputReport));
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                  uint8_t const *report, uint16_t len)
{
    // XInput reports are always 20 bytes
    if (len >= XINPUT_REPORT_SIZE) {
        lock_acquire();

        // Copy raw bytes into our report structure
        memcpy((void*)&_latest_report, report, sizeof(XInputReport));
        _new_report = true;

        lock_release();
    }

    // Request next report immediately — captures at full device rate
    tuh_hid_receive_report(dev_addr, instance);
}
