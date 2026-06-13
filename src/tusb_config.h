#pragma once

// Device runs on native USB port (USB-C to PC)
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE

// Host runs on PIO USB port (USB-A for keyboard)
#define CFG_TUSB_RHPORT1_MODE   OPT_MODE_HOST
#define CFG_TUH_RPI_PIO_USB     1

// Adafruit Feather RP2040 USB Host pinout
// D+ = GPIO16, D- = GPIO17 (must be consecutive)
// 5V power enable = GPIO18 (driven high to power connected device)
#define PIO_USB_DP_PIN          16
#define USB_HOST_5V_POWER_PIN   18

#define CFG_TUD_ENDPOINT0_SIZE  64

// HID device for gamepad output
#define CFG_TUD_HID             1
#define CFG_TUD_HID_EP_BUFSIZE  64

// CDC for config tool
#define CFG_TUD_CDC             1
#define CFG_TUD_CDC_RX_BUFSIZE  256
#define CFG_TUD_CDC_TX_BUFSIZE  256

// Disable unused classes
#define CFG_TUD_VENDOR          0
#define CFG_TUD_MIDI            0
#define CFG_TUD_MSC             0

// HID host for reading keyboard
#define CFG_TUH_HID             4
#define CFG_TUH_DEVICE_MAX      1
#define CFG_TUH_HID_EPIN_BUFSIZE  64
#define CFG_TUH_HID_EPOUT_BUFSIZE 64

#define CFG_TUSB_OS             OPT_OS_PICO
#define CFG_TUSB_MEM_ALIGN      __attribute__((aligned(4)))
#define CFG_TUSB_DEBUG          0
