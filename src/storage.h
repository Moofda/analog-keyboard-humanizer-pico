#pragma once
#include "humanizer.h"
#include <stdbool.h>

// Flash storage for humanizer settings
// Uses the last sector of flash to avoid conflicts with firmware

// Load settings from flash — returns true if valid settings found
// Falls back to defaults if flash is empty or corrupt
bool storage_load(HumanizerSettings *settings);

// Save settings to flash
// Returns true on success
bool storage_save(const HumanizerSettings *settings);

// Erase settings and reset to defaults
void storage_reset(HumanizerSettings *settings);

// Compute checksum for settings validation
uint32_t storage_checksum(const HumanizerSettings *settings);
