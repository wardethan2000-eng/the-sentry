/**
 * @file main.cpp
 * @brief Beacon (Clip) entry point — burst / sleep loop.
 *
 * Operational sequence (repeats indefinitely):
 *   1. Wake from deep sleep (watchdog timer, ~120 ms interval).
 *   2. Transmit BURSTS_PER_CYCLE × (600 µs ON + 600 µs OFF) IR bursts.
 *   3. Return to SLEEP_MODE_PWR_DOWN.
 *
 * Total active time per cycle ≈ 6 ms → average current ≈ 6 mA.
 * Estimated runtime on LIR2032 (40 mAh) ≈ 6.5 hours.
 */

#include <Arduino.h>
#include "config.h"
#include "ir_emitter.h"
#include "power_mgmt.h"

void setup() {
    // Disable unused peripherals first to minimise current draw.
    powerDisableUnusedPeripherals();

    // Set up Timer0 for 38 kHz carrier (output OFF until first burst).
    irEmitterInit();
}

void loop() {
    // --- Transmit burst train ---
    for (uint8_t i = 0; i < BURSTS_PER_CYCLE; i++) {
        irEmitterSendBurst(BURST_ON_US);

        // Silent gap between bursts (carrier OFF — already handled by
        // irEmitterSendBurst returning with LEDs off).
        // Delay for the gap duration using a simple busy-wait.
        delayMicroseconds(BURST_OFF_US);
    }

    // --- Enter deep sleep until next watchdog wake-up ---
    irEmitterOff();      // Ensure LEDs are off before sleeping
    powerEnterSleep();   // ~120 ms sleep (SLEEP_WDT_PRESCALER)
}
