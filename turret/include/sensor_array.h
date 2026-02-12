/**
 * @file sensor_array.h
 * @brief 4-channel TSOP38238 IR sensor reader with majority-vote filtering.
 *
 * The sensor cross ("Blinder") has four directional IR receivers behind
 * opaque divider walls.  Each sensor outputs LOW when a 38 kHz modulated
 * IR signal is detected, and HIGH when idle.
 *
 * This module provides:
 *   - Raw per-sensor reads
 *   - A rolling majority-vote filter to reject brief reflections (Issue #6)
 *   - Saturation detection (stuck-LOW guard)
 *   - A combined Direction enum for the tracking engine
 */

#ifndef SENSOR_ARRAY_H
#define SENSOR_ARRAY_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

/** @brief Raw state of one sensor (active-low on the wire). */
enum class SensorState : uint8_t {
    INACTIVE = 0,   ///< No signal (pin HIGH)
    ACTIVE   = 1,   ///< Signal detected (pin LOW)
    SATURATED = 2   ///< Stuck LOW for too long — ignore
};

/** @brief Filtered reading for all four sensors. */
struct SensorReading {
    SensorState top;
    SensorState bottom;
    SensorState left;
    SensorState right;

    /** @brief True if *any* sensor is ACTIVE (not saturated). */
    bool anyActive() const;

    /** @brief True if no sensor is ACTIVE. */
    bool noneActive() const;
};

/**
 * @brief Coarse direction derived from opposing sensor pairs.
 *
 * Used by the tracking engine to decide servo commands.
 */
enum class Direction : uint8_t {
    CENTER,     ///< Both pairs balanced or no clear bias
    LEFT,
    RIGHT,
    UP,
    DOWN,
    UP_LEFT,
    UP_RIGHT,
    DOWN_LEFT,
    DOWN_RIGHT,
    NONE        ///< No signal at all
};

// ---------------------------------------------------------------------------
// Class
// ---------------------------------------------------------------------------

class SensorArray {
public:
    /** @brief Configure sensor pins as INPUT_PULLUP. */
    void init();

    /**
     * @brief Sample all four sensors and push into filter buffers.
     *
     * Call this once per main-loop iteration (50 Hz).
     * After calling, use getFiltered() to read the debounced result.
     */
    void update();

    /**
     * @brief Return the majority-vote-filtered reading.
     *
     * A sensor is ACTIVE only if ≥ SENSOR_FILTER_THRESHOLD of the last
     * SENSOR_FILTER_WINDOW samples were LOW.
     *
     * A sensor is SATURATED if it has been continuously LOW for
     * SENSOR_SATURATED_MS — it is then reported as INACTIVE to prevent
     * the tracker from locking onto ambient IR.
     */
    SensorReading getFiltered() const;

    /**
     * @brief Derive a coarse Direction from the filtered reading.
     *
     * Horizontal axis:  LEFT if left active & right inactive, etc.
     * Vertical axis:    UP if top active & bottom inactive, etc.
     * Diagonal combos are also returned.
     */
    Direction getDirection() const;

private:
    // Per-sensor circular buffer for majority-vote filter.
    struct FilterState {
        uint8_t buffer     = 0;       // Bit-packed ring buffer (LSB = newest)
        uint8_t index      = 0;       // Next write position (0..WINDOW-1)
        uint16_t lowRunMs  = 0;       // Consecutive LOW duration for saturation
        bool saturated     = false;
    };

    FilterState filters_[4];          // [0]=top, [1]=bottom, [2]=left, [3]=right

    /** @brief Count set bits in the lower SENSOR_FILTER_WINDOW bits. */
    static uint8_t popcount(uint8_t bits);

    /** @brief Evaluate one sensor's filtered state. */
    SensorState evaluateSensor(uint8_t idx) const;

    /** @brief Push a raw sample (0 or 1) into one sensor's filter. */
    void pushSample(uint8_t idx, bool active);
};

#endif // SENSOR_ARRAY_H
