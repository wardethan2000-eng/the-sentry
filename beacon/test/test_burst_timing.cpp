/**
 * @file test_burst_timing.cpp
 * @brief Beacon test — verify 38 kHz burst timing.
 *
 * This is a PlatformIO unit-test stub.  On real hardware, connect an
 * oscilloscope probe to PB0 and verify:
 *   - Carrier frequency ≈ 38 kHz (period ≈ 26.3 µs)
 *   - Burst ON duration ≈ 600 µs
 *   - Gap between bursts ≈ 600 µs
 *   - 5 bursts per wake cycle
 *   - Sleep interval ≈ 120 ms between cycles
 *
 * Alternatively, aim the beacon at a TSOP38238 wired to an Arduino and
 * monitor the demodulated output on a serial plotter to confirm detection.
 *
 * TODO: Implement automated test using a second ATtiny85 with a TSOP38238
 *       that measures timing and reports pass/fail over serial.
 */

#include <Arduino.h>
// #include <unity.h>   // Uncomment when PlatformIO test framework is set up

void setup() {
    // Placeholder — run beacon firmware and observe on oscilloscope.
}

void loop() {
    // Intentionally empty.
}
