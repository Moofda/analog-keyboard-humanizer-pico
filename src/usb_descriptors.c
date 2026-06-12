#include "tusb.h"
#include <string.h>

// ── Generic HID Gamepad Descriptor ───────────────────────────────────────────
// Uses standard HID gamepad class — Windows picks this up via XInput
// compatibility layer automatically. Same approach as most third party
// controllers, fight sticks, and adapters.

// HID Report Descriptor — standard gamepad layout matching XInput axes
static uint8_t const desc_hid_report[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Gamepad)
    0xA1, 0x01,        // Collection (Application)

    // Buttons (16 buttons)
    0x05, 0x09,        // Usage Page (Button)
    0x19, 0x01,        // Usage Minimum (1)
    0x29, 0x10,        // Usage Maximum (16)
    0x15, 0x00,        // Logical Minimum (0)
    0x25, 0x01,        // Logical Maximum (1)
    0x75, 0x01,        // Report Size (1)
    0x95, 0x10,        // Report Count (16)
    0x81, 0x02,        // Input (Data, Variable, Absolute)

    // Left Trigger (8 bit)
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x32,        // Usage (Z)
    0x15, 0x00,        // Logical Minimum (0)
    0x26, 0xFF, 0x00,  // Logical Maximum (255)
    0x75, 0x08,        // Report Size (8)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x02,        // Input (Data, Variable, Absolute)

    // Right Trigger (8 bit)
    0x09, 0x35,        // Usage (Rz)
    0x15, 0x00,        // Logical Minimum (0)
    0x26, 0xFF, 0x00,  // Logical Maximum (255)
    0x75, 0x08,        // Report Size (8)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x02,        // Input (Data, Variable, Absolute)

    // Left Stick X/Y (16 bit signed)
    0x09, 0x30,        // Usage (X)
    0x09, 0x31,        // Usage (Y)
    0x17, 0x00, 0x80, 0xFF, 0xFF,  // Logical Minimum (-32768)
    0x27, 0xFF, 0x7F, 0x00, 0x00,  // Logical Maximum (32767)
    0x75, 0x10,        // Report Size (16)
    0x95, 0x02,        // Report Count (2)
    0x81, 0x02,        // Input (Data, Variable, Absolute)

    // Right Stick X/Y (16 bit signed)
    0x09, 0x33,        // Usage (Rx)
    0x09, 0x34,        // Usage (Ry)
    0x17, 0x00, 0x80, 0xFF, 0xFF,  // Logical Minimum (-32768)
    0x27, 0xFF, 0x7F, 0x00, 0x00,  // Logical Maximum (32767)
    0x75, 0x10,        // Report Size (16)
    0x95, 0x02,        // Report Count (2)
    0x81, 0x02,        // Input (Data, Variable, Absolute)

    0xC0               // End Collection
};

// Device descriptor — generic gamepad, no Microsoft VID/PID needed
tusb_desc_device_t const desc_device = {
    .bLength            = 18,
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,  // Defined by interface
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0x0F0D,  // Hori (common third party controller vendor)
    .idProduct          = 0x0092,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// Configuration descriptor
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID 0x81

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 1)
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// HID callbacks
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)bufsize;
}

// String descriptors
static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    uint8_t chr_count;

    if (index == 0) {
        _desc_str[1] = 0x0409;
        chr_count = 1;
    } else {
        const char *str;
        switch (index) {
            case 1: str = "Generic"; break;
            case 2: str = "Gamepad"; break;
            default: return NULL;
        }
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++)
            _desc_str[1 + i] = str[i];
    }

    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
