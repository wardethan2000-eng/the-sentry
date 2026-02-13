/**
 * @file signal_monitor.h
 * @brief Signal-loss state machine (Issue #9).
 *
 * Three states:
 *
 *   TRACKING  — At least one sensor active within the last
 *               SIGNAL_PRESENT_HOLDOFF_MS.  Normal tracking.
 *
 *   SEARCHING — No signal for SIGNAL_LOSS_SEARCH_MS.  Pan servo
 *               executes a slow ±SEARCH_SWEEP_DEG sweep; tilt moves
 *               to TILT_SCAN_DEG.  If signal returns → TRACKING.
 *
 *   PARKED    — No signal for SIGNAL_LOSS_PARK_MS.  All servos stopped,
 *               fan parked at home position.  Resume on any detection.
 *
 * The built-in LED indicates state:
 *   solid ON   = TRACKING
 *   slow blink = SEARCHING
 *   OFF        = PARKED
 */

#ifndef SIGNAL_MONITOR_H
#define SIGNAL_MONITOR_H

#include <stdint.h>

/** @brief Signal-loss state machine states. */
enum class MonitorState : uint8_t {
    TRACKING,
    SEARCHING,
    PARKED
};

class SignalMonitor {
public:
    /** @brief Initialise timers; starts in TRACKING state. */
    void init();

    /**
     * @brief Feed the monitor with the current signal status.
     *
     * Call once per main-loop iteration.
     *
     * @param anySignalDetected  true if SensorReading::anyActive().
     */
    void update(bool anySignalDetected);

    /** @brief Return the current state. */
    MonitorState getState() const;

    /**
     * @brief Return the state that was active *before* the most recent
     *        call to update().
     *
     * Compare getState() != getPreviousState() to detect transitions.
     */
    MonitorState getPreviousState() const;

    /**
     * @brief True if the most recent update() caused a state change.
     *
     * Convenience wrapper: getState() != getPreviousState().
     */
    bool stateChanged() const;

    /**
     * @brief Drive the status LED according to the current state.
     *
     * Call once per main-loop iteration (handles blink timing internally).
     */
    void updateStatusLED();

private:
    MonitorState state_     = MonitorState::TRACKING;
    MonitorState prevState_ = MonitorState::TRACKING;
    unsigned long lastSignalMs_  = 0;   ///< millis() of last detection
    unsigned long lastBlinkMs_   = 0;   ///< LED blink timer
    bool ledState_ = false;
};

#endif // SIGNAL_MONITOR_H
