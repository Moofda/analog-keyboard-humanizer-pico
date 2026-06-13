#pragma once
#include "humanizer.h"
#include <stdbool.h>
#include <stdint.h>

void usb_host_init(void);
void usb_host_task(void);
bool usb_host_device_connected(void);
bool usb_host_get_report(XInputReport *report);
