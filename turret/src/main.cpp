/**
 * @file main.cpp
 * @brief Turret (Fan Base) entry point — sensor → track → actuate loop.
 *
 * Main loop runs at ~50 Hz (LOOP_PERIOD_MS = 20 ms):
 *   1. Sample sensors and push into majority-vote filter.
 *   2. Evaluate signal-loss state machine.
 *   3. Depending on state:
 *        TRACKING  — run proportional tracking engine.
 *        SEARCHING — slow sweep ± SEARCH_SWEEP_DEG.
 *        PARKED    — park servos at home, idle.
 *   4. Update dead-reckoning pan position.
 *   5. Update status LED.
 *   6. Yield remaining time until next loop tick.
 */

#include <Arduino.h>
#include "config.h"
#include "sensor_array.h"
#include "pan_controller.h"
#include "tilt_controller.h"
#include "tracking_engine.h"
#include "signal_monitor.h"

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
// Setup
// ===================================================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.println(F("The Sentry — Turret v1.0"));
    Serial.println(F("Initialising..."));

    sensors.init();
    pan.init();
    tilt.init();
    tracker.init(&pan, &tilt);
    monitor.init();

    Serial.println(F("Ready. Waiting for beacon signal."));
}

// ===================================================================
// Main loop
// ===================================================================

void loop() {
    unsigned long loopStart = millis();

    // --- 1. Sample sensors ---
    sensors.update();
    SensorReading reading = sensors.getFiltered();

    // --- 2. Signal monitor ---
    monitor.update(reading.anyActive());
    MonitorState state = monitor.getState();

    // --- 3. Act based on state ---
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

    // --- 4. Update pan position estimate ---
    pan.updatePosition(LOOP_PERIOD_MS);

    // --- 5. Status LED ---
    monitor.updateStatusLED();

    // --- 6. Debug output (throttled to ~2 Hz to avoid flooding) ---
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
