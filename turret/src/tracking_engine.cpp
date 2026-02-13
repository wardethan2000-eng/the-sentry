/**
 * @file tracking_engine.cpp
 * @brief Proportional tracking algorithm with dead band and speed ramping.
 *
 * This is the core of The Sentry's tracking behaviour (Issue #8 resolution).
 *
 * Fix: The pan speed heuristic now uses a time-based memory of when the
 * opposing horizontal sensor was last active.  If the other side fired
 * recently (within APPROACH_MEMORY_MS), the beacon must be near center,
 * so we slow down for smooth convergence.  This replaces the previous
 * approach of checking vertical sensors, which was unreliable when the
 * beacon was at a different height than the sensor cross.
 */

#include "tracking_engine.h"
#include "config.h"
#include <Arduino.h>

// ===================================================================
// Public API
// ===================================================================

void TrackingEngine::init(PanController *pan, TiltController *tilt) {
    pan_  = pan;
    tilt_ = tilt;
    lastLeftActiveMs_  = 0;
    lastRightActiveMs_ = 0;
}

void TrackingEngine::update(const SensorReading &reading) {
    if (!pan_ || !tilt_) return;

    // --- Horizontal axis (pan) ---
    float panSpeed = computePanSpeed(reading);
    pan_->setSpeed(panSpeed);

    // --- Vertical axis (tilt) ---
    int8_t tiltDelta = computeTiltDelta(reading);
    if (tiltDelta != 0) {
        tilt_->nudge(tiltDelta);
    }
}

void TrackingEngine::halt() {
    if (pan_)  pan_->stop();
    // Tilt holds its last angle automatically (standard servo).
}

// ===================================================================
// Private helpers
// ===================================================================

float TrackingEngine::computePanSpeed(const SensorReading &reading) {
    unsigned long now = millis();

    bool left  = (reading.left  == SensorState::ACTIVE);
    bool right = (reading.right == SensorState::ACTIVE);

    // Track when each horizontal sensor was last active.
    if (left)  lastLeftActiveMs_  = now;
    if (right) lastRightActiveMs_ = now;

    // Dead band: both active (centered) or neither active (no info) → hold.
    if (left == right) {
        return 0.0f;
    }

    // Determine if the opposing sensor was recently active.
    // If so, the beacon is near center → use slow speed for smooth approach.
    bool nearCenter = false;
    if (left) {
        // Beacon is to the left; was the RIGHT sensor active recently?
        nearCenter = (now - lastRightActiveMs_) < APPROACH_MEMORY_MS;
    } else {
        // Beacon is to the right; was the LEFT sensor active recently?
        nearCenter = (now - lastLeftActiveMs_) < APPROACH_MEMORY_MS;
    }

    float speed = nearCenter ? TRACK_PAN_SPEED_SLOW : TRACK_PAN_SPEED_FAST;

    // Convention: negative = left (CCW), positive = right (CW).
    if (left) {
        return -speed;   // Beacon is to the left → pan CCW
    } else {
        return  speed;   // Beacon is to the right → pan CW
    }
}

int8_t TrackingEngine::computeTiltDelta(const SensorReading &reading) const {
    bool top    = (reading.top    == SensorState::ACTIVE);
    bool bottom = (reading.bottom == SensorState::ACTIVE);

    // Dead band: both active or neither → hold.
    if (top == bottom) {
        return 0;
    }

    if (top) {
        return +static_cast<int8_t>(TILT_STEP_DEG);   // Beacon is above → tilt up
    } else {
        return -static_cast<int8_t>(TILT_STEP_DEG);   // Beacon is below → tilt down
    }
}
