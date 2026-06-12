#pragma once

#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE
#define CFG_TUSB_RHPORT1_MODE   OPT_MODE_HOST
#define CFG_TUH_RPI_PIO_USB     1

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

// HID host
#define CFG_TUH_HID             4
#define CFG_TUH_DEVICE_MAX      1
#define CFG_TUH_HID_EPIN_BUFSIZE  64
#define CFG_TUH_HID_EPOUT_BUFSIZE 64

#define CFG_TUSB_OS             OPT_OS_PICO
#define CFG_TUSB_MEM_ALIGN      __attribute__((aligned(4)))
#define CFG_TUSB_DEBUG          0
