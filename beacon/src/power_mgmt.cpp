/**
 * @file power_mgmt.cpp
 * @brief ATtiny85 deep-sleep implementation with watchdog wake-up.
 *
 * Peripheral shutdown saves ~3 mA of idle current.  In SLEEP_MODE_PWR_DOWN
 * with BOD disabled the MCU draws ~0.5 µA; the watchdog adds ~6 µA for a
 * total sleep current of ≈ 6.5 µA.
 */

#include "power_mgmt.h"
#include "config.h"

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>

// ---------------------------------------------------------------------------
// Watchdog ISR — empty; the sole purpose is to wake the CPU.
// ---------------------------------------------------------------------------

ISR(WDT_vect) {
    // Nothing to do — execution resumes after sleep_cpu().
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void powerDisableUnusedPeripherals() {
    // Disable ADC (saves ~260 µA).
    ADCSRA &= ~_BV(ADEN);
    power_adc_disable();

    // Disable analog comparator (saves ~70 µA).
    ACSR |= _BV(ACD);

    // Disable Timer1 and USI (not used).
    power_timer1_disable();
    power_usi_disable();
}

void powerEnterSleep() {
    cli();

    // --- Configure watchdog for interrupt mode (no reset) ---
    // Timed sequence: set WDCE and WDE, then write new config within 4 cycles.
    MCUSR &= ~_BV(WDRF);                       // Clear reset flag first
    WDTCR |= _BV(WDCE) | _BV(WDE);            // Begin timed sequence
    WDTCR  = _BV(WDIE) | SLEEP_WDT_PRESCALER;  // Interrupt mode, ~120 ms

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    // Disable BOD during sleep for minimum current (ATtiny85 rev C+).
    // This is a timed sequence: must enter sleep within 3 clock cycles.
#if defined(BODS) && defined(BODSE)
    uint8_t mcucr_bak = MCUCR;
    MCUCR = mcucr_bak | _BV(BODS) | _BV(BODSE);
    MCUCR = (mcucr_bak | _BV(BODS)) & ~_BV(BODSE);
#endif

    sei();          // Interrupts must be enabled for wake-up
    sleep_cpu();    // Zzz… (wakes on WDT interrupt)

    sleep_disable();
}
