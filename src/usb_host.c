#include "usb_host.h"
#include "tusb.h"
#include "xinput_host.h"
#include "pico/stdlib.h"
#include "pico/mutex.h"
#include <string.h>
#include <stdio.h>

static volatile bool  _device_connected = false;
static volatile bool  _new_report        = false;
static XInputReport   _latest_report     = {0};
static mutex_t        _report_mutex;

void usb_host_init(void) {
    mutex_init(&_report_mutex);
    memset(&_latest_report, 0, sizeof(XInputReport));
    _device_connected = false;
    _new_report = false;
}

void usb_host_task(void) {
    tuh_task();
}

bool usb_host_device_connected(void) {
    return _device_connected;
}

bool usb_host_get_report(XInputReport *report) {
    if (!_new_report) return false;
    mutex_enter_blocking(&_report_mutex);
    memcpy(report, (const void*)&_latest_report, sizeof(XInputReport));
    _new_report = false;
    mutex_exit(&_report_mutex);
    return true;
}

// ── tusb_xinput callbacks ─────────────────────────────────────────────────────
// These replace the standard HID callbacks and handle XInput devices natively

// Required by tusb_xinput — registers the XInput host driver with TinyUSB
usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return &usbh_xinput_driver;
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance,
                          const xinputh_interface_t *xid_itf)
{
    printf("[HOST] XInput device mounted: dev=%d instance=%d type=%d\n",
           dev_addr, instance, xid_itf->type);
    _device_connected = true;

    // Send LED command and start receiving reports
    tuh_xinput_set_led(dev_addr, instance, 0, true);
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance) {
    printf("[HOST] XInput device unmounted: dev=%d instance=%d\n",
           dev_addr, instance);
    _device_connected = false;
    _new_report = false;
    memset((void*)&_latest_report, 0, sizeof(XInputReport));
}

void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                    const xinputh_interface_t *xid_itf,
                                    uint16_t len)
{
    (void)len;

    if (xid_itf->last_xfer_result == XFER_RESULT_SUCCESS &&
        xid_itf->connected && xid_itf->new_pad_data)
    {
        const xinput_gamepad_t *pad = &xid_itf->pad;

        mutex_enter_blocking(&_report_mutex);

        _latest_report.report_id     = 0x00;
        _latest_report.report_size   = 0x14;
        _latest_report.buttons       = pad->wButtons;
        _latest_report.left_trigger  = pad->bLeftTrigger;
        _latest_report.right_trigger = pad->bRightTrigger;
        _latest_report.left_x        = pad->sThumbLX;
        _latest_report.left_y        = pad->sThumbLY;
        _latest_report.right_x       = pad->sThumbRX;
        _latest_report.right_y       = pad->sThumbRY;
        _new_report = true;

        mutex_exit(&_report_mutex);
    }

    // Request next report immediately
    tuh_xinput_receive_report(dev_addr, instance);
}
