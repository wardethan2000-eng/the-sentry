/**
 * @file tracking_engine.h
 * @brief Proportional pan/tilt tracking with dead band (Issue #8).
 *
 * The tracking engine takes filtered sensor readings and produces servo
 * commands for both axes:
 *
 * Pan (horizontal):
 *   - Only LEFT active  → pan left at TRACK_PAN_SPEED_FAST
 *   - Only RIGHT active → pan right at TRACK_PAN_SPEED_FAST
 *   - Both LEFT & RIGHT → centered, hold (dead band)
 *   - Neither           → hold (no information)
 *   - If the "off" sensor is intermittently hitting (partial detection),
 *     reduce speed to TRACK_PAN_SPEED_SLOW for smooth approach.
 *
 * Tilt (vertical):
 *   - Only TOP active    → nudge up (+TILT_STEP_DEG)
 *   - Only BOTTOM active → nudge down (−TILT_STEP_DEG)
 *   - Both / neither     → hold
 *   - Rate limited by TiltController::nudge() internally.
 */

#ifndef TRACKING_ENGINE_H
#define TRACKING_ENGINE_H

#include "sensor_array.h"
#include "pan_controller.h"
#include "tilt_controller.h"

class TrackingEngine {
public:
    /**
     * @brief Store references to the controller objects.
     *
     * Call once after PanController::init() and TiltController::init().
     */
    void init(PanController *pan, TiltController *tilt);

    /**
     * @brief Run one tracking iteration.
     *
     * Reads the filtered sensor state, computes proportional commands,
     * and writes to the pan/tilt controllers.  Call once per main-loop
     * iteration while in TRACKING state.
     *
     * @param reading  Filtered sensor reading from SensorArray::getFiltered().
     */
    void update(const SensorReading &reading);

    /** @brief Stop both axes (servos hold / stop). */
    void halt();

private:
    PanController  *pan_  = nullptr;
    TiltController *tilt_ = nullptr;

    /**
     * @brief Determine proportional pan speed from horizontal sensor pair.
     *
     * @return Signed normalised speed (−1.0 … +1.0), 0 = hold.
     */
    float computePanSpeed(const SensorReading &reading) const;

    /**
     * @brief Determine tilt nudge direction from vertical sensor pair.
     *
     * @return +1 = up, −1 = down, 0 = hold.
     */
    int8_t computeTiltDelta(const SensorReading &reading) const;
};

#endif // TRACKING_ENGINE_H
