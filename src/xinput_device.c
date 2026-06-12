#include "xinput_device.h"
#include "tusb.h"
#include <string.h>
#include <stdio.h>

// HID gamepad report matching our descriptor layout
typedef struct __attribute__((packed)) {
    uint16_t buttons;
    uint8_t  left_trigger;
    uint8_t  right_trigger;
    int16_t  left_x;
    int16_t  left_y;
    int16_t  right_x;
    int16_t  right_y;
} HIDGamepadReport;

static volatile bool _device_ready = false;

void xinput_device_init(void) {
    _device_ready = false;
}

bool xinput_device_ready(void) {
    return _device_ready && tud_ready();
}

void xinput_device_send_report(const XInputReport *report) {
    if (!xinput_device_ready()) return;
    if (!tud_hid_ready()) return;

    // Convert XInput report to HID gamepad report
    HIDGamepadReport hid = {
        .buttons       = report->buttons,
        .left_trigger  = report->left_trigger,
        .right_trigger = report->right_trigger,
        .left_x        = report->left_x,
        .left_y        = report->left_y,
        .right_x       = report->right_x,
        .right_y       = report->right_y,
    };

    tud_hid_report(0, &hid, sizeof(hid));
}

void tud_mount_cb(void) {
    printf("[DEVICE] Mounted\n");
    _device_ready = true;
}

void tud_umount_cb(void) {
    printf("[DEVICE] Unmounted\n");
    _device_ready = false;
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    _device_ready = false;
}

void tud_resume_cb(void) {
    _device_ready = true;
}
