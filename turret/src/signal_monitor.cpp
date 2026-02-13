/**
 * @file signal_monitor.cpp
 * @brief Signal-loss state machine implementation.
 *
 * Fixes applied:
 *   - SIGNAL_PRESENT_HOLDOFF_MS is now enforced: the state machine stays
 *     in TRACKING for at least this long after the last detection before
 *     beginning the transition toward SEARCHING.
 *   - Previous-state tracking enables the main loop to detect transitions
 *     and run one-time entry actions (sweep reset, position re-zero, etc.).
 */

#include "signal_monitor.h"
#include "config.h"
#include <Arduino.h>

// ===================================================================
// Public API
// ===================================================================

void SignalMonitor::init() {
    state_        = MonitorState::TRACKING;
    prevState_    = MonitorState::TRACKING;
    lastSignalMs_ = millis();
    lastBlinkMs_  = millis();
    ledState_     = false;

    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH);  // Solid ON = TRACKING
}

void SignalMonitor::update(bool anySignalDetected) {
    unsigned long now = millis();

    // Snapshot current state so the main loop can detect transitions.
    prevState_ = state_;

    if (anySignalDetected) {
        lastSignalMs_ = now;

        // Any detection immediately returns us to TRACKING.
        state_ = MonitorState::TRACKING;
        return;
    }

    // No signal — compute time since last detection.
    unsigned long elapsed = now - lastSignalMs_;

    // Holdoff: stay in TRACKING if within the hysteresis window.
    // This prevents brief dropouts from starting the loss timer.
    if (elapsed < SIGNAL_PRESENT_HOLDOFF_MS) {
        // Remain in current state (TRACKING).
        return;
    }

    // Beyond holdoff — evaluate loss thresholds.
    if (elapsed >= SIGNAL_LOSS_PARK_MS) {
        state_ = MonitorState::PARKED;
    } else if (elapsed >= SIGNAL_LOSS_SEARCH_MS) {
        state_ = MonitorState::SEARCHING;
    }
    // else: past holdoff but before search threshold — stay in TRACKING
    // (signal just recently lost, not long enough to start searching).
}

MonitorState SignalMonitor::getState() const {
    return state_;
}

MonitorState SignalMonitor::getPreviousState() const {
    return prevState_;
}

bool SignalMonitor::stateChanged() const {
    return state_ != prevState_;
}

void SignalMonitor::updateStatusLED() {
    switch (state_) {
        case MonitorState::TRACKING:
            // Solid ON.
            digitalWrite(PIN_STATUS_LED, HIGH);
            break;

        case MonitorState::SEARCHING: {
            // Slow blink (500 ms on, 500 ms off).
            unsigned long now = millis();
            if ((now - lastBlinkMs_) >= 500) {
                ledState_ = !ledState_;
                lastBlinkMs_ = now;
            }
            digitalWrite(PIN_STATUS_LED, ledState_ ? HIGH : LOW);
            break;
        }

        case MonitorState::PARKED:
            // OFF.
            digitalWrite(PIN_STATUS_LED, LOW);
            break;
    }
}
