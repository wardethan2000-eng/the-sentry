/**
 * @file ir_emitter.h
 * @brief 38 kHz IR carrier generation and burst control for the beacon.
 *
 * Uses Timer0 in CTC mode to generate a hardware-accurate 38 kHz square wave
 * on OC0A (PB0).  The second LED on PB1 is toggled in the Timer0 Compare
 * Match ISR so both LEDs pulse in lockstep.
 *
 * Public API:
 *   irEmitterInit()      — configure Timer0 for 38 kHz, output OFF.
 *   irEmitterSendBurst() — enable carrier for `duration_us`, then silence.
 *   irEmitterOff()       — force carrier OFF immediately.
 */

#ifndef IR_EMITTER_H
#define IR_EMITTER_H

#include <stdint.h>

/**
 * @brief Initialise Timer0 for 38 kHz CTC output on OC0A.
 *
 * After this call the carrier is OFF (OC0A disconnected).
 * Call irEmitterSendBurst() to transmit.
 */
void irEmitterInit();

/**
 * @brief Transmit a single carrier burst of the given duration.
 *
 * Enables the 38 kHz carrier on both LED pins, blocks for @p duration_us
 * microseconds, then disables the carrier.
 *
 * @param duration_us  Burst length in microseconds (typ. 600).
 */
void irEmitterSendBurst(uint16_t duration_us);

/**
 * @brief Immediately disable the carrier (both LED pins LOW).
 */
void irEmitterOff();

#endif // IR_EMITTER_H
