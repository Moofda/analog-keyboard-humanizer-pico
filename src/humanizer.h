#pragma once
#include <stdint.h>
#include <stdbool.h>

// ── Humanization settings stored in flash ────────────────────────────────────

#define HUMANIZER_CURVE_MAX_POINTS 8
#define HUMANIZER_SETTINGS_MAGIC   0x484B4D52  // "HKMR"
#define HUMANIZER_SETTINGS_VERSION 1

typedef struct {
    float deflection;  // 0.0 to 1.0
    float value;       // 0.0 to 1.0 (scale factor at this deflection)
} CurvePoint;

typedef struct {
    uint32_t magic;
    uint32_t version;

    // Layer 1: Magnitude shaping
    bool     magnitude_enabled;
    float    magnitude_cap;        // 0.80 to 1.0
    float    magnitude_variation;  // 0.0 to 0.08

    // Layer 2: Humanization
    bool     humanization_enabled;
    float    global_baseline;      // 0.0 to 0.15
    float    drift_speed;          // 0.01 to 0.15
    uint32_t hold_min_ms;          // 20 to 500
    uint32_t hold_max_ms;          // 50 to 1000
    float    deadzone_threshold;   // 0.0 to 0.20
    float    release_fade;         // 0.80 to 0.99

    // Humanization curve points
    uint8_t  curve_point_count;
    CurvePoint curve_points[HUMANIZER_CURVE_MAX_POINTS];

    // Global
    uint8_t  source_controller_index;
    bool     apply_to_right_stick;

    uint32_t checksum;
} HumanizerSettings;

// Default settings
extern const HumanizerSettings HUMANIZER_DEFAULTS;

// ── XInput report structure ───────────────────────────────────────────────────

typedef struct __attribute__((packed)) {
    uint8_t  report_id;      // Always 0x00
    uint8_t  report_size;    // Always 0x14
    uint16_t buttons;
    uint8_t  left_trigger;
    uint8_t  right_trigger;
    int16_t  left_x;
    int16_t  left_y;
    int16_t  right_x;
    int16_t  right_y;
    uint8_t  reserved[6];
} XInputReport;

// Button bitmasks
#define XINPUT_BTN_DPAD_UP      0x0001
#define XINPUT_BTN_DPAD_DOWN    0x0002
#define XINPUT_BTN_DPAD_LEFT    0x0004
#define XINPUT_BTN_DPAD_RIGHT   0x0008
#define XINPUT_BTN_START        0x0010
#define XINPUT_BTN_BACK         0x0020
#define XINPUT_BTN_LEFT_THUMB   0x0040
#define XINPUT_BTN_RIGHT_THUMB  0x0080
#define XINPUT_BTN_LEFT_BUMPER  0x0100
#define XINPUT_BTN_RIGHT_BUMPER 0x0200
#define XINPUT_BTN_A            0x1000
#define XINPUT_BTN_B            0x2000
#define XINPUT_BTN_X            0x4000
#define XINPUT_BTN_Y            0x8000

// ── Stick state ───────────────────────────────────────────────────────────────

typedef struct {
    float x;  // -1.0 to 1.0
    float y;
} StickState;

// ── Humanizer state (one per stick) ──────────────────────────────────────────

typedef struct {
    // Magnitude layer state
    float mag_variation_offset;
    float mag_target_variation;
    uint32_t mag_next_variation_ms;

    // Humanization layer state
    float offset_x;
    float offset_y;
    float target_offset_x;
    float target_offset_y;
    uint32_t next_target_ms;
    float prev_magnitude;
} HumanizerState;

// ── Functions ─────────────────────────────────────────────────────────────────

void humanizer_init(HumanizerState *state);
void humanizer_reset(HumanizerState *state);

// Process a raw stick reading through magnitude + humanization layers
// Returns processed stick state
StickState humanizer_process(
    HumanizerState *state,
    const HumanizerSettings *settings,
    int16_t raw_x,
    int16_t raw_y,
    uint32_t now_ms
);

// Sample the humanization curve at a given deflection level
float humanizer_sample_curve(
    const HumanizerSettings *settings,
    float magnitude
);

// Apply full pipeline to an XInput report
void humanizer_process_report(
    HumanizerState *left_state,
    HumanizerState *right_state,
    const HumanizerSettings *settings,
    XInputReport *report,
    uint32_t now_ms
);
