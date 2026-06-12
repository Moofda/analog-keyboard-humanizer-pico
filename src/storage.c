#include "storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <string.h>

// Store settings in the last 4KB sector of flash
// RP2350-USB-A has 4MB flash
#define FLASH_SIZE_BYTES     (4 * 1024 * 1024)
#define SETTINGS_FLASH_OFFSET (FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_FLASH_ADDR  (XIP_BASE + SETTINGS_FLASH_OFFSET)

uint32_t storage_checksum(const HumanizerSettings *settings) {
    const uint8_t *data = (const uint8_t *)settings;
    uint32_t checksum = 0;
    // Simple additive checksum over everything except the checksum field itself
    size_t len = sizeof(HumanizerSettings) - sizeof(uint32_t);
    for (size_t i = 0; i < len; i++) {
        checksum += data[i];
        checksum ^= (checksum << 5) | (checksum >> 27);
    }
    return checksum;
}

bool storage_load(HumanizerSettings *settings) {
    const HumanizerSettings *flash_settings =
        (const HumanizerSettings *)SETTINGS_FLASH_ADDR;

    // Validate magic number and version
    if (flash_settings->magic != HUMANIZER_SETTINGS_MAGIC) {
        *settings = HUMANIZER_DEFAULTS;
        return false;
    }

    if (flash_settings->version != HUMANIZER_SETTINGS_VERSION) {
        *settings = HUMANIZER_DEFAULTS;
        return false;
    }

    // Validate checksum
    HumanizerSettings temp;
    memcpy(&temp, flash_settings, sizeof(HumanizerSettings));
    uint32_t expected = storage_checksum(&temp);
    if (temp.checksum != expected) {
        *settings = HUMANIZER_DEFAULTS;
        return false;
    }

    memcpy(settings, flash_settings, sizeof(HumanizerSettings));
    return true;
}

bool storage_save(const HumanizerSettings *settings) {
    HumanizerSettings to_save;
    memcpy(&to_save, settings, sizeof(HumanizerSettings));

    // Set magic and compute checksum
    to_save.magic    = HUMANIZER_SETTINGS_MAGIC;
    to_save.version  = HUMANIZER_SETTINGS_VERSION;
    to_save.checksum = storage_checksum(&to_save);

    // Pad to flash page size (256 bytes)
    uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof(page));
    memcpy(page, &to_save, sizeof(HumanizerSettings));

    // Flash operations must be done with interrupts disabled
    uint32_t ints = save_and_disable_interrupts();

    flash_range_erase(SETTINGS_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_FLASH_OFFSET, page, FLASH_PAGE_SIZE);

    restore_interrupts(ints);

    // Verify write
    const HumanizerSettings *written =
        (const HumanizerSettings *)SETTINGS_FLASH_ADDR;
    return (written->checksum == to_save.checksum);
}

void storage_reset(HumanizerSettings *settings) {
    *settings = HUMANIZER_DEFAULTS;
    storage_save(settings);
}
