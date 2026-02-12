/**
 * @file power_mgmt.h
 * @brief Deep-sleep and peripheral shutdown helpers for the ATtiny85 beacon.
 *
 * The beacon spends most of its time in SLEEP_MODE_PWR_DOWN (~0.5 µA).
 * A watchdog timer interrupt wakes the MCU for each burst cycle.
 *
 * Public API:
 *   powerDisableUnusedPeripherals() — one-time call to shut down ADC, etc.
 *   powerEnterSleep()               — sleep until watchdog fires.
 */

#ifndef POWER_MGMT_H
#define POWER_MGMT_H

/**
 * @brief Disable unused peripherals to minimise idle/sleep current draw.
 *
 * Shuts down the ADC, analog comparator, Timer1, and USI.
 * Call once during setup().
 */
void powerDisableUnusedPeripherals();

/**
 * @brief Configure the watchdog timer and enter power-down sleep.
 *
 * The MCU will wake when the watchdog interrupt fires (interval set by
 * SLEEP_WDT_PRESCALER in config.h — default ≈ 120 ms).
 *
 * On wake-up this function returns normally.
 */
void powerEnterSleep();

#endif // POWER_MGMT_H
