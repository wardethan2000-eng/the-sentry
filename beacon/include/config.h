/**
 * @file config.h
 * @brief Beacon configuration — pins, timing, and electrical constants.
 *
 * All tunable parameters for the IR beacon clip live here so they can be
 * adjusted without touching the implementation files.
 *
 * === Issue #1 Resolution ===
 * Using TSAL6200 high-power IR LEDs driven through a 2N2222 NPN transistor.
 * Resistor: 22 Ω per LED → I = (3.6V − 1.35V) / 22Ω ≈ 100 mA peak (pulsed).
 * The ATtiny pin drives the transistor base via a 1 kΩ resistor (~2 mA).
 *
 * === Issue #2 Resolution ===
 * Burst pattern: 5 × 600 µs pulses every 100 ms (watchdog wake).
 * Active time per cycle ≈ 6 ms → duty cycle ≈ 6%.
 * Average current ≈ 6 mA → LIR2032 (40 mAh) runtime ≈ 6.5 hours.
 */

#ifndef BEACON_CONFIG_H
#define BEACON_CONFIG_H

#include <stdint.h>

// ---------------------------------------------------------------------------
// Pin Assignments (ATtiny85 physical pin → Arduino pin mapping)
// ---------------------------------------------------------------------------
//   Physical Pin 5 = PB0 = Arduino 0 → OC0A (Timer0 PWM output)
//   Physical Pin 6 = PB1 = Arduino 1 → Transistor base (second LED bank)
//
// Both pins are driven in unison by enabling Timer0 Compare Match on OC0A
// and toggling PB1 manually in the ISR for the second LED.

/** @brief Primary IR LED driver pin (OC0A — hardware PWM). */
constexpr uint8_t PIN_IR_LED_A = 0;   // PB0

/** @brief Secondary IR LED driver pin (software-toggled in sync). */
constexpr uint8_t PIN_IR_LED_B = 1;   // PB1

// ---------------------------------------------------------------------------
// Carrier / Modulation Timing
// ---------------------------------------------------------------------------

/** @brief Carrier frequency in Hz (must match TSOP38238 on turret). */
constexpr uint32_t CARRIER_FREQ_HZ = 38000UL;

/** @brief Duration of a single IR burst (carrier ON), in microseconds. */
constexpr uint16_t BURST_ON_US = 600;

/** @brief Silent gap between bursts, in microseconds. */
constexpr uint16_t BURST_OFF_US = 600;

/** @brief Number of ON/OFF burst pairs per wake cycle. */
constexpr uint8_t BURSTS_PER_CYCLE = 5;

// ---------------------------------------------------------------------------
// Power / Sleep
// ---------------------------------------------------------------------------

/**
 * @brief Watchdog prescaler setting for sleep interval.
 *
 * WDTO_120MS ≈ 120 ms between wake-ups (closest standard interval to 100 ms).
 * Defined in <avr/wdt.h>:
 *   WDTO_60MS  = 0x03
 *   WDTO_120MS = 0x04
 *   WDTO_250MS = 0x05
 */
constexpr uint8_t SLEEP_WDT_PRESCALER = 0x04;  // WDTO_120MS

// ---------------------------------------------------------------------------
// Electrical Constants (for reference / documentation)
// ---------------------------------------------------------------------------

/** @brief LED forward voltage (TSAL6200 typical), in volts. */
constexpr float LED_VF = 1.35f;

/** @brief Battery nominal voltage (LIR2032), in volts. */
constexpr float BATTERY_V = 3.6f;

/** @brief Current-limiting resistor per LED, in ohms. */
constexpr float LED_RESISTOR_OHM = 22.0f;

/** @brief Calculated peak LED current (pulsed), in milliamps. */
constexpr float LED_PEAK_MA = (BATTERY_V - LED_VF) / LED_RESISTOR_OHM * 1000.0f;

#endif // BEACON_CONFIG_H
