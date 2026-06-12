#pragma once

// ── TinyUSB configuration for RP2350-USB-A ────────────────────────────────────

// Run device on native USB (USB-C port to PC)
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE

// Run host on PIO USB (USB-A port for keyboard)
#define CFG_TUSB_RHPORT1_MODE   OPT_MODE_HOST

// Use PIO USB for host
#define CFG_TUH_RPI_PIO_USB     1

// Device configuration
#define CFG_TUD_ENDPOINT0_SIZE  64

// Enable vendor class for XInput
#define CFG_TUD_VENDOR          1
#define CFG_TUD_VENDOR_RX_BUFSIZE  64
#define CFG_TUD_VENDOR_TX_BUFSIZE  64

// Enable CDC for config tool communication
#define CFG_TUD_CDC             1
#define CFG_TUD_CDC_RX_BUFSIZE  256
#define CFG_TUD_CDC_TX_BUFSIZE  256

// Host configuration
// Support one HID device (the keyboard)
#define CFG_TUH_HID             4
#define CFG_TUH_DEVICE_MAX      1

// Enable HID host
#define CFG_TUH_HID_EPIN_BUFSIZE  64
#define CFG_TUH_HID_EPOUT_BUFSIZE 64

// OS abstraction
#define CFG_TUSB_OS             OPT_OS_PICO
#define CFG_TUSB_MEM_ALIGN      __attribute__((aligned(4)))

// Debug level (0=off, 1=error, 2=warning, 3=info)
#define CFG_TUSB_DEBUG          0
