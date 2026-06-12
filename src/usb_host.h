#pragma once
#include "humanizer.h"
#include <stdbool.h>
#include <stdint.h>

// USB host — reads XInput HID reports from keyboard on USB-A port
// Runs on core 1 via TinyUSB PIO host

// Initialize USB host
void usb_host_init(void);

// Called from core 1 task loop
void usb_host_task(void);

// Returns true if a keyboard/gamepad is connected
bool usb_host_device_connected(void);

// Get the latest XInput report from the keyboard
// Returns true if a new report is available since last call
bool usb_host_get_report(XInputReport *report);

// TinyUSB host callbacks — called internally by TinyUSB
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *desc_report, uint16_t desc_len);
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance);
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                 uint8_t const *report, uint16_t len);
