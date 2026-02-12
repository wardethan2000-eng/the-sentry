/**
 * @file pan_controller.h
 * @brief Continuous-rotation servo wrapper for the pan (horizontal) axis.
 *
 * Wraps an MG996R continuous-rotation servo with:
 *   - Normalised speed input (−1.0 … +1.0)
 *   - Dead-reckoning position tracking (integrates speed × time)
 *   - Software rotation limits (±PAN_LIMIT_DEG) to protect cables (Issue #4)
 *   - Minimum speed threshold to overcome gear backlash (Issue #5)
 */

#ifndef PAN_CONTROLLER_H
#define PAN_CONTROLLER_H

#include <ESP32Servo.h>
#include <stdint.h>

class PanController {
public:
    /** @brief Attach servo and initialise to stopped. */
    void init();

    /**
     * @brief Command pan speed.
     *
     * @param speed  Normalised speed: −1.0 = full CCW, 0.0 = stop, +1.0 = full CW.
     *               Values with |speed| < PAN_MIN_SPEED are treated as 0.
     *
     * The command is silently clamped if the estimated position has reached
     * a software limit and the command would push further.
     */
    void setSpeed(float speed);

    /** @brief Stop the pan servo immediately. */
    void stop();

    /**
     * @brief Update the estimated angular position.
     *
     * Must be called once per loop iteration so the dead-reckoning integrator
     * stays current.  Pass the loop period (ms) or let it auto-measure.
     *
     * @param dt_ms  Elapsed time since last call, in milliseconds.
     */
    void updatePosition(uint16_t dt_ms);

    /** @brief Return estimated angular position (degrees, 0 = home). */
    float getPositionDeg() const;

    /** @brief True if within software limits. */
    bool isWithinLimits() const;

    /**
     * @brief Drive to estimated home (0°) by reversing until position ≈ 0.
     *
     * Non-blocking: call repeatedly from the main loop.
     * @return true when position is within 5° of home.
     */
    bool parkHome();

    /** @brief Reset the estimated position to 0 (re-zero). */
    void resetPosition();

private:
    Servo servo_;
    float currentSpeed_  = 0.0f;   ///< Last commanded normalised speed
    float positionDeg_   = 0.0f;   ///< Estimated absolute angle from home

    /** @brief Convert normalised speed to servo microseconds. */
    uint16_t speedToMicroseconds(float speed) const;
};

#endif // PAN_CONTROLLER_H
