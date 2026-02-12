/**
 * @file tracking_engine.cpp
 * @brief Proportional tracking algorithm with dead band and speed ramping.
 *
 * This is the core of The Sentry's tracking behaviour (Issue #8 resolution).
 */

#include "tracking_engine.h"
#include "config.h"

// ===================================================================
// Public API
// ===================================================================

void TrackingEngine::init(PanController *pan, TiltController *tilt) {
    pan_  = pan;
    tilt_ = tilt;
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

float TrackingEngine::computePanSpeed(const SensorReading &reading) const {
    bool left  = (reading.left  == SensorState::ACTIVE);
    bool right = (reading.right == SensorState::ACTIVE);

    // Dead band: both active (centered) or neither active (no info) → hold.
    if (left == right) {
        return 0.0f;
    }

    // Determine direction and speed.
    // "Far off-center" = only one side active.  We don't have a partial-
    // detection metric from the majority-vote filter directly, so we use
    // a simple heuristic: check if the vertical sensors are also active.
    // If the beacon is close to center, at least one vertical sensor is
    // likely active alongside the horizontal one, suggesting we're nearly
    // aligned.  This is a rough proxy for "almost centered → slow down".

    bool anyVertical = (reading.top  == SensorState::ACTIVE) ||
                       (reading.bottom == SensorState::ACTIVE);

    float speed = anyVertical ? TRACK_PAN_SPEED_SLOW : TRACK_PAN_SPEED_FAST;

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
