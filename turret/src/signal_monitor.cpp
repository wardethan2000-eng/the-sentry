/**
 * @file signal_monitor.cpp
 * @brief Signal-loss state machine implementation.
 */

#include "signal_monitor.h"
#include "config.h"
#include <Arduino.h>

// ===================================================================
// Public API
// ===================================================================

void SignalMonitor::init() {
    state_        = MonitorState::TRACKING;
    lastSignalMs_ = millis();
    lastBlinkMs_  = millis();
    ledState_     = false;

    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH);  // Solid ON = TRACKING
}

void SignalMonitor::update(bool anySignalDetected) {
    unsigned long now = millis();

    if (anySignalDetected) {
        lastSignalMs_ = now;

        // Any detection immediately returns us to TRACKING.
        state_ = MonitorState::TRACKING;
        return;
    }

    // No signal — compute time since last detection.
    unsigned long elapsed = now - lastSignalMs_;

    if (elapsed >= SIGNAL_LOSS_PARK_MS) {
        state_ = MonitorState::PARKED;
    } else if (elapsed >= SIGNAL_LOSS_SEARCH_MS) {
        state_ = MonitorState::SEARCHING;
    }
    // else: still within holdoff window, stay in current state (TRACKING).
}

MonitorState SignalMonitor::getState() const {
    return state_;
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
