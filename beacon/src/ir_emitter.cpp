/**
 * @file ir_emitter.cpp
 * @brief 38 kHz IR carrier generation using Timer0 CTC on ATtiny85.
 *
 * Timer0 is configured in CTC mode with a prescaler of 1.  At F_CPU = 8 MHz:
 *     OCR0A = (F_CPU / (2 × CARRIER_FREQ)) − 1
 *           = (8 000 000 / (2 × 38 000)) − 1
 *           = 104.26…  → use 104
 *     Actual freq = 8 000 000 / (2 × (104 + 1)) = 38 095 Hz  (≈ 38 kHz ✓)
 *
 * When the carrier is active, OC0A (PB0) toggles on compare match, and
 * the TIMER0_COMPA ISR toggles PB1 in software so both LEDs pulse together.
 *
 * When the carrier is muted, OC0A is disconnected (normal port operation)
 * and both pins are driven LOW.
 */

#include "ir_emitter.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Pre-computed compare-match value for 38 kHz @ 8 MHz, prescaler = 1.
static constexpr uint8_t TIMER0_TOP =
    static_cast<uint8_t>((F_CPU / (2UL * CARRIER_FREQ_HZ)) - 1);

// ---------------------------------------------------------------------------
// ISR — toggle PB1 in sync with hardware-toggled PB0
// ---------------------------------------------------------------------------

ISR(TIMER0_COMPA_vect) {
    PINB = _BV(PIN_IR_LED_B);  // Writing to PINx toggles the port bit
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void irEmitterInit() {
    // Set LED pins as outputs, initially LOW.
    DDRB  |= _BV(PIN_IR_LED_A) | _BV(PIN_IR_LED_B);
    PORTB &= ~(_BV(PIN_IR_LED_A) | _BV(PIN_IR_LED_B));

    // Configure Timer0: CTC mode, prescaler = 1, OC0A disconnected for now.
    TCCR0A = _BV(WGM01);                  // CTC mode (TOP = OCR0A)
    TCCR0B = _BV(CS00);                   // clk/1 (no prescaling)
    OCR0A  = TIMER0_TOP;                   // Set compare-match value
    TIMSK  &= ~_BV(OCIE0A);               // ISR disabled until burst
}

void irEmitterSendBurst(uint16_t duration_us) {
    // Connect OC0A to toggle on compare match → PB0 outputs 38 kHz.
    TCCR0A |= _BV(COM0A0);
    // Enable compare-match interrupt for PB1 software toggle.
    TIMSK  |= _BV(OCIE0A);
    sei();

    // Block for the burst duration.
    // _delay_us() requires a compile-time constant, so we loop in 10 µs steps.
    uint16_t steps = duration_us / 10;
    while (steps--) {
        _delay_us(10);
    }

    // Mute: disconnect OC0A, disable ISR, pull both pins LOW.
    irEmitterOff();
}

void irEmitterOff() {
    TCCR0A &= ~_BV(COM0A0);               // Disconnect OC0A (normal port op)
    TIMSK  &= ~_BV(OCIE0A);               // Disable compare-match ISR
    PORTB  &= ~(_BV(PIN_IR_LED_A) | _BV(PIN_IR_LED_B));  // LEDs OFF
}
