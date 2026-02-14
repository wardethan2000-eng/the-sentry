/**
 * @file tilt_controller.h
 * @brief Standard 180° servo wrapper for the tilt (vertical) axis.
 *
 * Wraps an MG996R standard servo with:
 *   - Clamped angle range (TILT_MIN_DEG … TILT_MAX_DEG)
 *   - Incremental nudge with rate limiting (Issue #8)
 *   - Park-to-home convenience method
 *
 * Fix: currentAngle_ is now int16_t to avoid subtle signed/unsigned
 *      issues when nudging near the lower bound.
 */

#ifndef TILT_CONTROLLER_H
#define TILT_CONTROLLER_H

#include <ESP32Servo.h>
#include <stdint.h>

class TiltController {
public:
    /** @brief Attach servo and move to TILT_HOME_DEG. */
    void init();

    /**
     * @brief Set absolute tilt angle.
     * @param degrees  Target angle, clamped to [TILT_MIN_DEG, TILT_MAX_DEG].
     */
    void setAngle(int16_t degrees);

    /**
     * @brief Incremental adjustment, respecting rate limit.
     *
     * Moves the tilt by @p delta degrees (positive = up, negative = down),
     * but only if at least TILT_HOLDOFF_MS have elapsed since the last step.
     *
     * @param delta  Signed step in degrees.  Magnitude is clamped to TILT_STEP_DEG.
     * @return true if the step was applied, false if rate-limited (too soon).
     */
    bool nudge(int8_t delta);

    /** @brief Return current tilt angle (degrees). */
    int16_t getAngle() const;

    /** @brief Move to TILT_HOME_DEG. */
    void parkHome();

    /** @brief Move to TILT_SCAN_DEG (used during SEARCHING state). */
    void goScanPosition();

private:
    Servo servo_;
    int16_t currentAngle_ = 0;
    unsigned long lastStepMs_ = 0;   ///< millis() of last nudge application
};

#endif // TILT_CONTROLLER_H
