/**
 * @file config.h
 * @brief Turret configuration — every tunable parameter in one place.
 *
 * Adjust these constants to tune tracking behaviour, servo response,
 * sensor filtering, and signal-loss recovery without editing any .cpp file.
 *
 * Issue resolutions baked into these defaults:
 *   #4  — Pan limited to ±135° (270° total) via PAN_LIMIT_DEG.
 *   #5  — Minimum pan speed threshold (PAN_MIN_SPEED) to overcome gear backlash.
 *   #6  — Majority-vote filter window (SENSOR_FILTER_WINDOW / _THRESHOLD).
 *   #8  — Dead band, proportional speed, tilt rate limiting.
 *   #9  — Signal-loss timeouts (SIGNAL_LOSS_*_MS).
 */

#ifndef TURRET_CONFIG_H
#define TURRET_CONFIG_H

#include <stdint.h>

// ===================================================================
// Pin Assignments  (ESP32 DevKit v1)
//
// Avoid: GPIO 0, 2, 5, 12, 15 (boot strapping)
//        GPIO 6–11  (internal flash SPI)
//        GPIO 34–39 (input-only, no pull-up)
// ===================================================================

/** @brief Pan servo signal (continuous rotation MG996R). PWM capable. */
constexpr uint8_t PIN_PAN_SERVO  = 18;

/** @brief Tilt servo signal (standard 180° MG996R). PWM capable. */
constexpr uint8_t PIN_TILT_SERVO = 19;

/** @brief TSOP38238 sensor — Top (upper quadrant). LOW = signal detected. */
constexpr uint8_t PIN_SENSOR_TOP    = 16;

/** @brief TSOP38238 sensor — Bottom (lower quadrant). */
constexpr uint8_t PIN_SENSOR_BOTTOM = 17;

/** @brief TSOP38238 sensor — Left. */
constexpr uint8_t PIN_SENSOR_LEFT   = 25;

/** @brief TSOP38238 sensor — Right. */
constexpr uint8_t PIN_SENSOR_RIGHT  = 26;

/** @brief Built-in LED for status indication (ESP32 DevKit onboard LED). */
constexpr uint8_t PIN_STATUS_LED    = 2;

// ===================================================================
// Sensor Filtering  (Issue #6)
// ===================================================================

/**
 * @brief Rolling window size for majority-vote filter.
 *
 * Each sensor maintains a circular buffer of this many recent readings.
 * A sensor counts as "active" only if at least SENSOR_FILTER_THRESHOLD
 * of the last SENSOR_FILTER_WINDOW samples were LOW (signal detected).
 */
constexpr uint8_t SENSOR_FILTER_WINDOW    = 8;

/**
 * @brief Minimum detections within the window to count as "active".
 * 6 out of 8 → rejects brief reflections while staying responsive.
 */
constexpr uint8_t SENSOR_FILTER_THRESHOLD = 6;

/**
 * @brief Saturation guard — if a sensor reports LOW for this many
 *        consecutive milliseconds, flag it as saturated and ignore.
 */
constexpr uint16_t SENSOR_SATURATED_MS = 2000;

// ===================================================================
// Pan Axis  (Issues #4, #5)
// ===================================================================

/**
 * @brief Continuous-rotation servo: microsecond value for "stopped".
 * Calibrate per-servo; 1500 µs is the typical center.
 */
constexpr uint16_t PAN_STOP_US = 1500;

/**
 * @brief Full-speed clockwise microsecond value. */
constexpr uint16_t PAN_CW_FULL_US = 1300;

/**
 * @brief Full-speed counter-clockwise microsecond value. */
constexpr uint16_t PAN_CCW_FULL_US = 1700;

/**
 * @brief Software pan limit in degrees from center (±).
 * 135° each side = 270° total travel.  (Issue #4 — cable protection.)
 */
constexpr float PAN_LIMIT_DEG = 135.0f;

/**
 * @brief Minimum normalised speed command (0.0–1.0).
 * Commands below this are ignored to overcome gear backlash.  (Issue #5.)
 */
constexpr float PAN_MIN_SPEED = 0.15f;

/**
 * @brief Estimated full-speed angular rate, degrees per second.
 * Used for dead-reckoning position integration.  Calibrate empirically.
 */
constexpr float PAN_DEG_PER_SEC = 60.0f;

// ===================================================================
// Tilt Axis
// ===================================================================

/** @brief Minimum tilt angle (degrees). Fan pointing level. */
constexpr uint8_t TILT_MIN_DEG = 0;

/** @brief Maximum tilt angle (degrees). Fan pointing upward. */
constexpr uint8_t TILT_MAX_DEG = 45;

/** @brief Default / home tilt angle. */
constexpr uint8_t TILT_HOME_DEG = 0;

/** @brief Tilt scan-mode angle (mid-range, used during SEARCHING). */
constexpr uint8_t TILT_SCAN_DEG = 20;

/**
 * @brief Maximum tilt adjustment per loop iteration (degrees).
 * At 50 Hz loop rate → max 50°/s.  (Issue #8 — rate limiting.)
 */
constexpr uint8_t TILT_STEP_DEG = 1;

/**
 * @brief Minimum time between tilt steps, in milliseconds.
 * Lets the mechanical system settle.  (Issue #8.)
 */
constexpr uint16_t TILT_HOLDOFF_MS = 100;

// ===================================================================
// Tracking Engine  (Issue #8)
// ===================================================================

/**
 * @brief Pan speed when beacon is far off-center (only one sensor active).
 * Normalised 0.0–1.0.
 */
constexpr float TRACK_PAN_SPEED_FAST = 0.80f;

/**
 * @brief Pan speed when beacon is nearly centered (intermittent off-side hits).
 * Normalised 0.0–1.0.
 */
constexpr float TRACK_PAN_SPEED_SLOW = 0.30f;

// ===================================================================
// Signal Monitor  (Issue #9)
// ===================================================================

/**
 * @brief Time with at least one sensor active to remain in TRACKING state.
 * Provides hysteresis against momentary dropouts.
 */
constexpr uint16_t SIGNAL_PRESENT_HOLDOFF_MS = 500;

/**
 * @brief Time without any signal before transitioning to SEARCHING.
 */
constexpr uint16_t SIGNAL_LOSS_SEARCH_MS = 3000;

/**
 * @brief Time without any signal before transitioning to PARKED.
 */
constexpr uint16_t SIGNAL_LOSS_PARK_MS = 15000;

/**
 * @brief Sweep half-angle during SEARCHING state (degrees from center).
 */
constexpr float SEARCH_SWEEP_DEG = 90.0f;

/**
 * @brief Sweep speed during SEARCHING state (normalised, slow).
 */
constexpr float SEARCH_SWEEP_SPEED = 0.25f;

// ===================================================================
// Main Loop
// ===================================================================

/** @brief Target loop period in milliseconds (50 Hz). */
constexpr uint16_t LOOP_PERIOD_MS = 20;

/** @brief Serial debug output baud rate. */
constexpr uint32_t SERIAL_BAUD = 115200;

#endif // TURRET_CONFIG_H
