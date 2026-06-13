#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "tusb.h"
#include "humanizer.h"
#include "storage.h"
#include "usb_host.h"
#include "xinput_device.h"
#include <stdio.h>
#include <string.h>

// Port assignments for RP2350-USB-A
// Native USB (USB-C) = port 0 = device
// PIO USB (USB-A)    = port 1 = host
#define TUD_RHPORT  0
#define TUH_RHPORT  1

#define CMD_GET_SETTINGS  'G'
#define CMD_SET_SETTINGS  'S'
#define CMD_RESET         'R'
#define CMD_VERSION       'V'
#define CMD_PING          'P'
#define REPLY_OK          'K'
#define REPLY_ERROR       'E'

static HumanizerSettings g_settings;
static HumanizerState    g_left_state;
static HumanizerState    g_right_state;
static XInputReport      g_current_report   = {0};
static XInputReport      g_processed_report = {0};

void core1_entry(void) {
    usb_host_init();
    tuh_init(TUH_RHPORT);
    printf("[CORE1] USB host started\n");
    while (true) {
        usb_host_task();
    }
}

static void handle_config_command(void) {
    if (!tud_cdc_connected()) return;

    int c = getchar_timeout_us(0);
    if (c == PICO_ERROR_TIMEOUT) return;

    switch ((char)c) {
        case CMD_PING:
            putchar(REPLY_OK);
            fflush(stdout);
            break;

        case CMD_VERSION: {
            char ver[32];
            snprintf(ver, sizeof(ver), "AKH v%d.%d.%d\n",
                     FIRMWARE_VERSION_MAJOR,
                     FIRMWARE_VERSION_MINOR,
                     FIRMWARE_VERSION_PATCH);
            fputs(ver, stdout);
            fflush(stdout);
            break;
        }

        case CMD_GET_SETTINGS:
            putchar(REPLY_OK);
            fwrite(&g_settings, sizeof(HumanizerSettings), 1, stdout);
            fflush(stdout);
            break;

        case CMD_SET_SETTINGS: {
            HumanizerSettings new_settings;
            size_t received = fread(&new_settings, 1,
                                    sizeof(HumanizerSettings), stdin);
            if (received == sizeof(HumanizerSettings) &&
                new_settings.magic == HUMANIZER_SETTINGS_MAGIC)
            {
                g_settings = new_settings;
                putchar(storage_save(&g_settings) ? REPLY_OK : REPLY_ERROR);
            } else {
                putchar(REPLY_ERROR);
            }
            fflush(stdout);
            break;
        }

        case CMD_RESET:
            storage_reset(&g_settings);
            humanizer_reset(&g_left_state);
            humanizer_reset(&g_right_state);
            putchar(REPLY_OK);
            fflush(stdout);
            break;

        default:
            break;
    }
}

int main(void) {
    stdio_init_all();

    // Enable 5V power to USB-A host port
    // Adafruit Feather RP2040 USB Host requires GPIO18 driven high
    // to provide power to connected USB devices
#ifdef USB_HOST_5V_POWER_PIN
    gpio_init(USB_HOST_5V_POWER_PIN);
    gpio_set_dir(USB_HOST_5V_POWER_PIN, GPIO_OUT);
    gpio_put(USB_HOST_5V_POWER_PIN, 1);
#endif

    printf("\n[MAIN] Analog Keyboard Humanizer v%d.%d.%d\n",
           FIRMWARE_VERSION_MAJOR,
           FIRMWARE_VERSION_MINOR,
           FIRMWARE_VERSION_PATCH);

    if (storage_load(&g_settings)) {
        printf("[MAIN] Settings loaded from flash\n");
    } else {
        printf("[MAIN] Using default settings\n");
    }

    humanizer_init(&g_left_state);
    humanizer_init(&g_right_state);

    xinput_device_init();
    tud_init(TUD_RHPORT);

    printf("[MAIN] USB device started\n");

    multicore_launch_core1(core1_entry);

    printf("[MAIN] Core 1 launched for USB host\n");

    while (true) {
        tud_task();
        handle_config_command();

        if (usb_host_get_report(&g_current_report)) {
            memcpy(&g_processed_report, &g_current_report,
                   sizeof(XInputReport));

            uint32_t now_ms = to_ms_since_boot(get_absolute_time());
            humanizer_process_report(
                &g_left_state, &g_right_state,
                &g_settings, &g_processed_report, now_ms);

            xinput_device_send_report(&g_processed_report);
        }
    }

    return 0;
}
