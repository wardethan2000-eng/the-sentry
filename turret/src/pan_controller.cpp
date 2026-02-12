/**
 * @file pan_controller.cpp
 * @brief Continuous-rotation servo control with position tracking and limits.
 */

#include "pan_controller.h"
#include "config.h"
#include <Arduino.h>

// ===================================================================
// Public API
// ===================================================================

void PanController::init() {
    servo_.attach(PIN_PAN_SERVO);
    servo_.writeMicroseconds(PAN_STOP_US);
    currentSpeed_ = 0.0f;
    positionDeg_  = 0.0f;
}

void PanController::setSpeed(float speed) {
    // Clamp input to ±1.0.
    if (speed >  1.0f) speed =  1.0f;
    if (speed < -1.0f) speed = -1.0f;

    // Dead zone: ignore commands below the minimum threshold (Issue #5).
    if (fabsf(speed) < PAN_MIN_SPEED) {
        speed = 0.0f;
    }

    // Software limit enforcement (Issue #4).
    // Prevent movement that would push past the limit.
    if (positionDeg_ >=  PAN_LIMIT_DEG && speed > 0.0f) speed = 0.0f;
    if (positionDeg_ <= -PAN_LIMIT_DEG && speed < 0.0f) speed = 0.0f;

    currentSpeed_ = speed;
    servo_.writeMicroseconds(speedToMicroseconds(speed));
}

void PanController::stop() {
    currentSpeed_ = 0.0f;
    servo_.writeMicroseconds(PAN_STOP_US);
}

void PanController::updatePosition(uint16_t dt_ms) {
    // Integrate: Δθ = speed × degPerSec × Δt
    float dt_sec = static_cast<float>(dt_ms) / 1000.0f;
    positionDeg_ += currentSpeed_ * PAN_DEG_PER_SEC * dt_sec;

    // Hard-clamp to limits (safety net for accumulation drift).
    if (positionDeg_ >  PAN_LIMIT_DEG) positionDeg_ =  PAN_LIMIT_DEG;
    if (positionDeg_ < -PAN_LIMIT_DEG) positionDeg_ = -PAN_LIMIT_DEG;
}

float PanController::getPositionDeg() const {
    return positionDeg_;
}

bool PanController::isWithinLimits() const {
    return (positionDeg_ > -PAN_LIMIT_DEG) && (positionDeg_ < PAN_LIMIT_DEG);
}

bool PanController::parkHome() {
    constexpr float HOME_TOLERANCE = 5.0f;  // degrees

    if (fabsf(positionDeg_) < HOME_TOLERANCE) {
        stop();
        return true;
    }

    // Drive toward home: if position is positive, go negative (CCW) and vice versa.
    float homeSpeed = (positionDeg_ > 0.0f) ? -TRACK_PAN_SPEED_SLOW
                                              :  TRACK_PAN_SPEED_SLOW;
    setSpeed(homeSpeed);
    return false;
}

void PanController::resetPosition() {
    positionDeg_ = 0.0f;
}

// ===================================================================
// Private helpers
// ===================================================================

uint16_t PanController::speedToMicroseconds(float speed) const {
    // speed: -1.0 → PAN_CCW_FULL_US,  0.0 → PAN_STOP_US,  +1.0 → PAN_CW_FULL_US
    if (speed >= 0.0f) {
        // CW: interpolate from STOP down to CW_FULL.
        return static_cast<uint16_t>(
            PAN_STOP_US - speed * (PAN_STOP_US - PAN_CW_FULL_US));
    } else {
        // CCW: interpolate from STOP up to CCW_FULL.
        return static_cast<uint16_t>(
            PAN_STOP_US + (-speed) * (PAN_CCW_FULL_US - PAN_STOP_US));
    }
}
