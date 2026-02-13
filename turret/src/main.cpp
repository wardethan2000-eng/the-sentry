/**
 * @file main.cpp
 * @brief Turret (Fan Base) entry point — sensor → track → actuate loop.
 *
 * Main loop runs at ~50 Hz (LOOP_PERIOD_MS = 20 ms):
 *   1. Feed the ESP32 watchdog timer.
 *   2. Sample sensors and push into majority-vote filter.
 *   3. Evaluate signal-loss state machine.
 *   4. Handle one-time state-entry actions on transitions.
 *   5. Depending on state:
 *        TRACKING  — run proportional tracking engine.
 *        SEARCHING — slow sweep ± SEARCH_SWEEP_DEG.
 *        PARKED    — park servos at home, idle.
 *   6. Update dead-reckoning pan position.
 *   7. Update status LED.
 *   8. Yield remaining time until next loop tick.
 *
 * Fixes applied:
 *   - ESP32 hardware watchdog resets the MCU if the loop stalls for > 4 s.
 *   - State transitions trigger one-time entry actions (sweep reset,
 *     tracker halt, position re-zero on recovery from PARKED).
 *   - Search sweep direction is reset based on current pan position
 *     when entering SEARCHING, preventing asymmetric sweeps.
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "sensor_array.h"
#include "pan_controller.h"
#include "tilt_controller.h"
#include "tracking_engine.h"
#include "signal_monitor.h"

// ===================================================================
// Watchdog configuration
// ===================================================================

/** @brief Watchdog timeout in seconds.  If the main loop doesn't feed
 *         the WDT within this time, the ESP32 resets. */
static constexpr uint32_t WDT_TIMEOUT_S = 4;

// ===================================================================
// Module instances
// ===================================================================

static SensorArray    sensors;
static PanController  pan;
static TiltController tilt;
static TrackingEngine tracker;
static SignalMonitor  monitor;

// ===================================================================
// Search sweep state
// ===================================================================

static bool  sweepDirectionCW = true;   ///< Current sweep direction

// ===================================================================
// State-transition entry actions
// ===================================================================

/**
 * @brief Called once when transitioning INTO the TRACKING state.
 */
static void onEnterTracking(MonitorState fromState) {
    // Coming from PARKED: the dead-reckoning position may have drifted
    // while the fan was stationary, so re-zero it.  The fan should be
    // at (or very near) home after parking, making 0° a good estimate.
    if (fromState == MonitorState::PARKED) {
        pan.resetPosition();
    }

    Serial.println(F("[Transition] → TRACKING"));
}

/**
 * @brief Called once when transitioning INTO the SEARCHING state.
 */
static void onEnterSearching() {
    // Stop the tracker cleanly before the sweep takes over.
    tracker.halt();

    // Reset sweep direction based on current pan position so the sweep
    // is roughly centered.  If we're left of center, sweep CW first
    // (toward center); if right, sweep CCW first.
    sweepDirectionCW = (pan.getPositionDeg() <= 0.0f);

    Serial.println(F("[Transition] → SEARCHING"));
}

/**
 * @brief Called once when transitioning INTO the PARKED state.
 */
static void onEnterParked() {
    // Stop everything.  parkHome() will be called each loop iteration,
    // but we also halt the tracker to ensure no stale commands linger.
    tracker.halt();

    Serial.println(F("[Transition] → PARKED"));
}

// ===================================================================
// Setup
// ===================================================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.println(F("The Sentry — Turret v1.1"));
    Serial.println(F("Initialising..."));

    sensors.init();
    pan.init();
    tilt.init();
    tracker.init(&pan, &tilt);
    monitor.init();

    // Configure the ESP32 Task Watchdog Timer.
    // If the main loop stalls (e.g., I²C hang, library deadlock), the
    // WDT resets the MCU rather than leaving the fan running uncontrolled.
    esp_task_wdt_init(WDT_TIMEOUT_S, true);   // true = trigger reset on timeout
    esp_task_wdt_add(NULL);                    // Subscribe the current task (loopTask)

    Serial.println(F("Ready. Waiting for beacon signal."));
}

// ===================================================================
// Main loop
// ===================================================================

void loop() {
    unsigned long loopStart = millis();

    // --- 1. Feed the watchdog ---
    esp_task_wdt_reset();

    // --- 2. Sample sensors ---
    sensors.update();
    SensorReading reading = sensors.getFiltered();

    // --- 3. Signal monitor ---
    monitor.update(reading.anyActive());
    MonitorState state = monitor.getState();

    // --- 4. Handle state transitions (one-time entry actions) ---
    if (monitor.stateChanged()) {
        MonitorState prev = monitor.getPreviousState();
        switch (state) {
            case MonitorState::TRACKING:  onEnterTracking(prev); break;
            case MonitorState::SEARCHING: onEnterSearching();    break;
            case MonitorState::PARKED:    onEnterParked();       break;
        }
    }

    // --- 5. Act based on current state ---
    switch (state) {

        case MonitorState::TRACKING:
            tracker.update(reading);
            break;

        case MonitorState::SEARCHING:
            // Slow sweep: alternate CW and CCW.
            tilt.goScanPosition();

            if (sweepDirectionCW) {
                pan.setSpeed(SEARCH_SWEEP_SPEED);
                if (pan.getPositionDeg() >= SEARCH_SWEEP_DEG) {
                    sweepDirectionCW = false;
                }
            } else {
                pan.setSpeed(-SEARCH_SWEEP_SPEED);
                if (pan.getPositionDeg() <= -SEARCH_SWEEP_DEG) {
                    sweepDirectionCW = true;
                }
            }
            break;

        case MonitorState::PARKED:
            pan.parkHome();
            tilt.parkHome();
            break;
    }

    // --- 6. Update pan position estimate ---
    pan.updatePosition(LOOP_PERIOD_MS);

    // --- 7. Status LED ---
    monitor.updateStatusLED();

    // --- 8. Debug output (throttled to ~2 Hz to avoid flooding) ---
    static unsigned long lastDebugMs = 0;
    if (millis() - lastDebugMs >= 500) {
        lastDebugMs = millis();
        Serial.print(F("State="));
        switch (state) {
            case MonitorState::TRACKING:  Serial.print(F("TRACK")); break;
            case MonitorState::SEARCHING: Serial.print(F("SEARCH")); break;
            case MonitorState::PARKED:    Serial.print(F("PARK")); break;
        }
        Serial.print(F("  Pan="));
        Serial.print(pan.getPositionDeg(), 1);
        Serial.print(F("°  Tilt="));
        Serial.print(tilt.getAngle());
        Serial.print(F("°  Sensors: T="));
        Serial.print(reading.top    == SensorState::ACTIVE ? '1' : '0');
        Serial.print(F(" B="));
        Serial.print(reading.bottom == SensorState::ACTIVE ? '1' : '0');
        Serial.print(F(" L="));
        Serial.print(reading.left   == SensorState::ACTIVE ? '1' : '0');
        Serial.print(F(" R="));
        Serial.println(reading.right  == SensorState::ACTIVE ? '1' : '0');
    }

    // --- Yield: wait for remainder of the loop period ---
    unsigned long elapsed = millis() - loopStart;
    if (elapsed < LOOP_PERIOD_MS) {
        delay(LOOP_PERIOD_MS - elapsed);
    }
}
