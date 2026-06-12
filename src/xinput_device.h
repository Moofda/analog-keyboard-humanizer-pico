#pragma once
#include "humanizer.h"
#include "tusb.h"
#include <stdbool.h>
#include <stdint.h>

void xinput_device_init(void);
void xinput_device_send_report(const XInputReport *report);
bool xinput_device_ready(void);

void tud_mount_cb(void);
void tud_umount_cb(void);
void tud_suspend_cb(bool remote_wakeup_en);
void tud_resume_cb(void);
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                  tusb_control_request_t const *request);
