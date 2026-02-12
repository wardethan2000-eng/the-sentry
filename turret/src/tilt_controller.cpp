/**
 * @file tilt_controller.cpp
 * @brief Standard servo tilt control with range clamping and rate limiting.
 */

#include "tilt_controller.h"
#include "config.h"
#include <Arduino.h>

// ===================================================================
// Public API
// ===================================================================

void TiltController::init() {
    servo_.attach(PIN_TILT_SERVO);
    currentAngle_ = TILT_HOME_DEG;
    servo_.write(currentAngle_);
    lastStepMs_ = millis();
}

void TiltController::setAngle(uint8_t degrees) {
    // Clamp to allowed range.
    if (degrees < TILT_MIN_DEG) degrees = TILT_MIN_DEG;
    if (degrees > TILT_MAX_DEG) degrees = TILT_MAX_DEG;

    currentAngle_ = degrees;
    servo_.write(currentAngle_);
}

bool TiltController::nudge(int8_t delta) {
    unsigned long now = millis();

    // Rate limiting (Issue #8): reject if too soon since last step.
    if ((now - lastStepMs_) < TILT_HOLDOFF_MS) {
        return false;
    }

    // Clamp step magnitude.
    if (delta > static_cast<int8_t>(TILT_STEP_DEG))  delta =  static_cast<int8_t>(TILT_STEP_DEG);
    if (delta < -static_cast<int8_t>(TILT_STEP_DEG)) delta = -static_cast<int8_t>(TILT_STEP_DEG);

    // Compute new angle with bounds check.
    int16_t newAngle = static_cast<int16_t>(currentAngle_) + delta;
    if (newAngle < TILT_MIN_DEG) newAngle = TILT_MIN_DEG;
    if (newAngle > TILT_MAX_DEG) newAngle = TILT_MAX_DEG;

    currentAngle_ = static_cast<uint8_t>(newAngle);
    servo_.write(currentAngle_);
    lastStepMs_ = now;
    return true;
}

uint8_t TiltController::getAngle() const {
    return currentAngle_;
}

void TiltController::parkHome() {
    setAngle(TILT_HOME_DEG);
}

void TiltController::goScanPosition() {
    setAngle(TILT_SCAN_DEG);
}
