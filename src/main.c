#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "tusb.h"
#include "humanizer.h"
#include "storage.h"
#include "usb_host.h"
#include "xinput_device.h"
#include <stdio.h>
#include <string.h>

// ── Config protocol ───────────────────────────────────────────────────────────
// Simple serial protocol for Windows config tool communication
// Commands sent over USB CDC serial
//
// PC → Pico:
//   'G'        — Get settings (Pico replies with binary settings struct)
//   'S' + data — Set settings (20 byte header + settings struct)
//   'R'        — Reset to defaults
//   'V'        — Get version string
//   'P'        — Ping (Pico replies 'K')
//
// Pico → PC:
//   'K'        — OK acknowledgement
//   'E'        — Error
//   Binary settings struct (on 'G' command)

#define CMD_GET_SETTINGS  'G'
#define CMD_SET_SETTINGS  'S'
#define CMD_RESET         'R'
#define CMD_VERSION       'V'
#define CMD_PING          'P'
#define REPLY_OK          'K'
#define REPLY_ERROR       'E'

// ── Global state ──────────────────────────────────────────────────────────────

static HumanizerSettings g_settings;
static HumanizerState    g_left_state;
static HumanizerState    g_right_state;
static XInputReport      g_current_report  = {0};
static XInputReport      g_processed_report = {0};

// ── Core 1: USB host task ─────────────────────────────────────────────────────
// Runs TinyUSB host on core 1 to avoid interfering with device on core 0

void core1_entry(void) {
    usb_host_init();

    // Initialize TinyUSB host
    tuh_init(BOARD_TUH_RHPORT);

    printf("[CORE1] USB host started\n");

    while (true) {
        usb_host_task();
    }
}

// ── Config protocol handler ───────────────────────────────────────────────────

static void handle_config_command(void) {
    if (!stdio_usb_connected()) return;

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
            // Send raw settings struct
            putchar(REPLY_OK);
            fwrite(&g_settings, sizeof(HumanizerSettings), 1, stdout);
            fflush(stdout);
            break;

        case CMD_SET_SETTINGS: {
            // Read settings struct from PC
            HumanizerSettings new_settings;
            size_t received = fread(&new_settings, 1, sizeof(HumanizerSettings), stdin);

            if (received == sizeof(HumanizerSettings) &&
                new_settings.magic == HUMANIZER_SETTINGS_MAGIC)
            {
                g_settings = new_settings;
                if (storage_save(&g_settings)) {
                    putchar(REPLY_OK);
                } else {
                    putchar(REPLY_ERROR);
                }
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

// ── Main — core 0 ─────────────────────────────────────────────────────────────

int main(void) {
    // Init stdio over UART for debug
    stdio_init_all();

    printf("\n[MAIN] Analog Keyboard Humanizer v%d.%d.%d\n",
           FIRMWARE_VERSION_MAJOR,
           FIRMWARE_VERSION_MINOR,
           FIRMWARE_VERSION_PATCH);

    // Load settings from flash
    if (storage_load(&g_settings)) {
        printf("[MAIN] Settings loaded from flash\n");
    } else {
        printf("[MAIN] Using default settings\n");
    }

    // Initialize humanizer state
    humanizer_init(&g_left_state);
    humanizer_init(&g_right_state);

    // Initialize XInput device output
    xinput_device_init();

    // Initialize TinyUSB device on core 0
    tud_init(BOARD_TUD_RHPORT);

    printf("[MAIN] USB device started\n");

    // Launch USB host on core 1
    multicore_launch_core1(core1_entry);

    printf("[MAIN] Core 1 launched for USB host\n");

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (true) {
        // Handle USB device tasks
        tud_task();

        // Handle config commands from PC
        handle_config_command();

        // Process input if a new report is available from keyboard
        if (usb_host_get_report(&g_current_report)) {

            // Copy report for processing
            memcpy(&g_processed_report, &g_current_report, sizeof(XInputReport));

            // Apply humanization
            uint32_t now_ms = to_ms_since_boot(get_absolute_time());
            humanizer_process_report(
                &g_left_state,
                &g_right_state,
                &g_settings,
                &g_processed_report,
                now_ms
            );

            // Send processed report to PC
            xinput_device_send_report(&g_processed_report);
        }
    }

    return 0;
}
