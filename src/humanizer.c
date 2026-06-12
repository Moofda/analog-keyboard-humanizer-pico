#include "humanizer.h"
#include <math.h>
#include <string.h>

// ── Default settings ──────────────────────────────────────────────────────────

const HumanizerSettings HUMANIZER_DEFAULTS = {
    .magic   = HUMANIZER_SETTINGS_MAGIC,
    .version = HUMANIZER_SETTINGS_VERSION,

    .magnitude_enabled   = true,
    .magnitude_cap       = 0.95f,
    .magnitude_variation = 0.02f,

    .humanization_enabled = true,
    .global_baseline      = 0.04f,
    .drift_speed          = 0.04f,
    .hold_min_ms          = 120,
    .hold_max_ms          = 360,
    .deadzone_threshold   = 0.03f,
    .release_fade         = 0.88f,

    .curve_point_count = 4,
    .curve_points = {
        { 0.0f, 0.0f },
        { 0.3f, 1.0f },
        { 0.7f, 0.8f },
        { 1.0f, 0.0f },
    },

    .source_controller_index = 0,
    .apply_to_right_stick    = false,

    .checksum = 0  // Computed on save
};

// ── Simple LCG random number generator ───────────────────────────────────────
// Not cryptographic but fine for organic humanization on embedded hardware

static uint32_t rng_state = 0x12345678;

static void rng_seed(uint32_t seed) {
    rng_state = seed;
}

static uint32_t rng_next(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

// Returns float in range [-1.0, 1.0]
static float rng_signed(void) {
    return ((float)(int32_t)rng_next()) / (float)0x80000000;
}

// Returns float in range [0.0, 1.0]
static float rng_positive(void) {
    return (float)(rng_next() >> 1) / (float)0x7FFFFFFF;
}

// ── Init / Reset ──────────────────────────────────────────────────────────────

void humanizer_init(HumanizerState *state) {
    memset(state, 0, sizeof(HumanizerState));
    // Seed RNG differently for left and right sticks
    rng_seed(0xDEADBEEF ^ (uint32_t)(uintptr_t)state);
}

void humanizer_reset(HumanizerState *state) {
    memset(state, 0, sizeof(HumanizerState));
}

// ── Curve sampling ────────────────────────────────────────────────────────────

float humanizer_sample_curve(
    const HumanizerSettings *settings,
    float magnitude)
{
    if (settings->curve_point_count == 0) return 1.0f;
    if (settings->curve_point_count == 1) return settings->curve_points[0].value;

    // Sort is assumed — points should be in order of deflection
    const CurvePoint *pts = settings->curve_points;
    int n = settings->curve_point_count;

    if (magnitude <= pts[0].deflection)   return pts[0].value;
    if (magnitude >= pts[n-1].deflection) return pts[n-1].value;

    for (int i = 0; i < n - 1; i++) {
        if (magnitude >= pts[i].deflection && magnitude <= pts[i+1].deflection) {
            float t = (magnitude - pts[i].deflection) /
                      (pts[i+1].deflection - pts[i].deflection);
            return pts[i].value + (pts[i+1].value - pts[i].value) * t;
        }
    }
    return 1.0f;
}

// ── Magnitude layer ───────────────────────────────────────────────────────────

static StickState magnitude_process(
    HumanizerState *state,
    const HumanizerSettings *settings,
    StickState input,
    uint32_t now_ms)
{
    if (!settings->magnitude_enabled) return input;

    float magnitude = sqrtf(input.x * input.x + input.y * input.y);
    if (magnitude < 0.01f) return input;

    // Cap magnitude
    float capped = fminf(magnitude, settings->magnitude_cap);

    // Subtle variation
    if (now_ms >= state->mag_next_variation_ms) {
        state->mag_target_variation = rng_signed() * settings->magnitude_variation;
        state->mag_next_variation_ms = now_ms + 80 + (uint32_t)(rng_positive() * 120.0f);
    }
    state->mag_variation_offset +=
        (state->mag_target_variation - state->mag_variation_offset) * 0.05f;

    float final_mag = capped + state->mag_variation_offset;
    if (final_mag < 0.0f) final_mag = 0.0f;
    if (final_mag > 1.0f) final_mag = 1.0f;

    float scale = final_mag / magnitude;

    StickState out = {
        .x = fmaxf(-1.0f, fminf(1.0f, input.x * scale)),
        .y = fmaxf(-1.0f, fminf(1.0f, input.y * scale))
    };
    return out;
}

// ── Humanization layer ────────────────────────────────────────────────────────

static StickState humanization_process(
    HumanizerState *state,
    const HumanizerSettings *settings,
    StickState input,
    uint32_t now_ms)
{
    if (!settings->humanization_enabled) return input;

    float magnitude = sqrtf(input.x * input.x + input.y * input.y);

    // Below deadzone — fade offset back to zero
    if (magnitude < settings->deadzone_threshold) {
        state->offset_x *= settings->release_fade;
        state->offset_y *= settings->release_fade;
        state->prev_magnitude = 0.0f;
        StickState out = {
            .x = fmaxf(-1.0f, fminf(1.0f, input.x + state->offset_x)),
            .y = fmaxf(-1.0f, fminf(1.0f, input.y + state->offset_y))
        };
        return out;
    }

    // Sample curve for allowed offset scale at this deflection
    float curve_scale = humanizer_sample_curve(settings, magnitude);
    float max_offset  = settings->global_baseline * curve_scale;

    // Pick new target when timer expires
    if (now_ms >= state->next_target_ms) {
        float angle  = rng_positive() * 6.28318f;  // 2*PI
        float radius = rng_positive() * max_offset;
        state->target_offset_x = cosf(angle) * radius;
        state->target_offset_y = sinf(angle) * radius;

        float hold_range = (float)(settings->hold_max_ms - settings->hold_min_ms);
        state->next_target_ms = now_ms + settings->hold_min_ms +
                                (uint32_t)(rng_positive() * hold_range);
    }

    // Drift toward target — scaled by poll interval for rate consistency
    float drift = settings->drift_speed;
    state->offset_x += (state->target_offset_x - state->offset_x) * drift;
    state->offset_y += (state->target_offset_y - state->offset_y) * drift;

    // Scale offset by magnitude for natural release fade
    float faded_x = state->offset_x * magnitude;
    float faded_y = state->offset_y * magnitude;

    state->prev_magnitude = magnitude;

    StickState out = {
        .x = fmaxf(-1.0f, fminf(1.0f, input.x + faded_x)),
        .y = fmaxf(-1.0f, fminf(1.0f, input.y + faded_y))
    };
    return out;
}

// ── Full pipeline ─────────────────────────────────────────────────────────────

StickState humanizer_process(
    HumanizerState *state,
    const HumanizerSettings *settings,
    int16_t raw_x,
    int16_t raw_y,
    uint32_t now_ms)
{
    StickState s = {
        .x = (float)raw_x / 32767.0f,
        .y = (float)raw_y / 32767.0f
    };

    s = magnitude_process(state, settings, s, now_ms);
    s = humanization_process(state, settings, s, now_ms);

    return s;
}

void humanizer_process_report(
    HumanizerState *left_state,
    HumanizerState *right_state,
    const HumanizerSettings *settings,
    XInputReport *report,
    uint32_t now_ms)
{
    // Process left stick
    StickState left = humanizer_process(
        left_state, settings,
        report->left_x, report->left_y,
        now_ms);

    report->left_x = (int16_t)(left.x * 32767.0f);
    report->left_y = (int16_t)(left.y * 32767.0f);

    // Process right stick if enabled
    if (settings->apply_to_right_stick) {
        StickState right = humanizer_process(
            right_state, settings,
            report->right_x, report->right_y,
            now_ms);
        report->right_x = (int16_t)(right.x * 32767.0f);
        report->right_y = (int16_t)(right.y * 32767.0f);
    }
}
