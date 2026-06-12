#pragma once
#include "humanizer.h"
#include <stdbool.h>
#include <stdint.h>

// XInput USB device — presents Pico as Xbox 360 controller to PC
// Uses custom HID descriptors matching Xbox 360 controller format

// Initialize XInput device
void xinput_device_init(void);

// Send XInput report to PC
// Called after humanization processing
void xinput_device_send_report(const XInputReport *report);

// Returns true if PC has enumerated the device
bool xinput_device_ready(void);

// TinyUSB device callbacks
void tud_mount_cb(void);
void tud_umount_cb(void);
void tud_suspend_cb(bool remote_wakeup_en);
void tud_resume_cb(void);
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                  tusb_control_request_t const *request);
