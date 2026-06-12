#include "tusb.h"

// ── XInput USB Descriptors ────────────────────────────────────────────────────
// These make Windows identify the Pico as an Xbox 360 controller
// Matches exactly what a real Xbox 360 controller presents

// Device descriptor
// VID/PID matches Xbox 360 controller so Windows loads xusb22.sys automatically
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0xFF,  // Vendor specific
    .bDeviceSubClass    = 0xFF,
    .bDeviceProtocol    = 0xFF,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x045E,  // Microsoft
    .idProduct          = 0x028E,  // Xbox 360 Controller
    .bcdDevice          = 0x0114,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// Configuration descriptor
// XInput uses vendor class with specific interface subclass/protocol
// Interface has 2 endpoints: IN (reports to PC) and OUT (rumble from PC)
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + 32 + 2*7)
#define XINPUT_DESC_TYPE_SECURITY  0x21

uint8_t const desc_configuration[] = {
    // Config descriptor
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface 0: XInput gamepad
    9, TUSB_DESC_INTERFACE, 0x00, 0x00, 0x02,
    0xFF,   // Class: vendor specific
    0x5D,   // Subclass: XInput
    0x01,   // Protocol: gamepad
    0x00,

    // XInput security descriptor (required by Windows XInput driver)
    0x11, 0x21, 0x00, 0x01, 0x01, 0x25,
    0x81, 0x14, 0x03, 0x03, 0x03, 0x04,
    0x13, 0x02, 0x08, 0x03, 0x03,

    // Endpoint 1 IN: reports to PC at 1000hz (1ms interval)
    7, TUSB_DESC_ENDPOINT,
    0x81,           // EP1 IN
    TUSB_XFER_INTERRUPT,
    U16_TO_U8S_LE(0x0020),  // 32 bytes max packet
    1,              // 1ms polling interval = 1000hz

    // Endpoint 1 OUT: rumble from PC (we receive but ignore)
    7, TUSB_DESC_ENDPOINT,
    0x01,           // EP1 OUT
    TUSB_XFER_INTERRUPT,
    U16_TO_U8S_LE(0x0020),
    8,              // 8ms interval

    // Interface 1: XInput headset (required for full Xbox 360 compatibility)
    9, TUSB_DESC_INTERFACE, 0x01, 0x00, 0x04,
    0xFF, 0x5D, 0x03, 0x00,

    // Security descriptor for interface 1
    0x1B, 0x21, 0x00, 0x01, 0x01, 0x01, 0x82,
    0x40, 0x01, 0x02, 0x20, 0x16, 0x83, 0x40,
    0x01, 0x04, 0x20, 0x16, 0x85, 0x40, 0x01,
    0x05, 0x20, 0x16, 0x8B, 0x46, 0x0B,

    // 4 endpoints for headset interface (required by driver)
    7, TUSB_DESC_ENDPOINT, 0x82, TUSB_XFER_INTERRUPT,
       U16_TO_U8S_LE(0x0020), 2,
    7, TUSB_DESC_ENDPOINT, 0x02, TUSB_XFER_INTERRUPT,
       U16_TO_U8S_LE(0x0020), 4,
    7, TUSB_DESC_ENDPOINT, 0x83, TUSB_XFER_ISOCHRONOUS,
       U16_TO_U8S_LE(0x0020), 1,
    7, TUSB_DESC_ENDPOINT, 0x03, TUSB_XFER_ISOCHRONOUS,
       U16_TO_U8S_LE(0x0020), 1,
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// String descriptors
char const *string_desc_arr[] = {
    (const char[]){ 0x09, 0x04 },  // 0: Language = English
    "Microsoft",                    // 1: Manufacturer
    "Controller",                   // 2: Product
    "000000000001",                 // 3: Serial
};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
            return NULL;
        const char *str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++)
            _desc_str[1 + i] = str[i];
    }

    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
