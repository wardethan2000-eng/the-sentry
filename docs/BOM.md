# Bill of Materials — The Sentry

All values reflect resolved issues (corrected resistors, upgraded LEDs, etc.).

---

## Beacon (Wearable Clip)

| # | Component | Qty | Specification | Est. Cost | Notes |
|---|-----------|-----|---------------|-----------|-------|
| 1 | Microcontroller | 1 | ATtiny85-20PU (DIP-8) | ~$1.50 | Pulses LEDs at 38 kHz via Timer0 CTC |
| 2 | Battery Kit | 1 | LIR2032 + USB charger | ~$10.00 | 3.6 V rechargeable Li-Ion coin cell, 40 mAh |
| 3 | IR LEDs | 2 | **TSAL6200** (940 nm, 5 mm) | ~$1.00 | 100 mA continuous, 1.5 A pulsed; replaces original 3 mm LEDs (Issue #1) |
| 4 | NPN Transistor | 1 | 2N2222A (TO-92) | ~$0.20 | Drives LEDs so ATtiny pin only sources ~2 mA |
| 5 | LED Resistors | 2 | **22 Ω** ¼W | <$0.10 | I = (3.6 V − 1.35 V) / 22 Ω ≈ 100 mA per LED (pulsed) (Issue #1) |
| 6 | Base Resistor | 1 | 1 kΩ ¼W | <$0.10 | Limits base current to transistor |
| 7 | IC Socket | 1 | 8-pin DIP socket | ~$0.50 | Protects chip during soldering |
| 8 | Battery Holder | 1 | CR2032 PCB mount | ~$1.00 | Structural backbone of the clip |
| 9 | Bypass Capacitor | 1 | 100 nF ceramic (0805 or through-hole) | <$0.10 | **Required.** Place between VCC and GND, close to ATtiny85 pin 4/8. Prevents brownout during 200 mA pulsed loads. |

**Beacon subtotal: ~$14.50**

### Fallback (if TSAL6200 unavailable)
Use standard 3 mm 940 nm IR LEDs with **120 Ω** resistors (20 mA continuous).
Range will be reduced to ~3 m.  Consider adding a third LED to compensate.

### Current Capability Note
The LIR2032 has limited pulse-current capability (typically 50–100 mA depending
on manufacturer).  Two LEDs at 100 mA each (200 mA total) will cause significant
voltage sag, reducing actual LED current below the calculated value.  This is
acceptable — the TSAL6200's high radiant intensity still provides adequate range.
If you need maximum range, consider a CR123A or AAA battery holder instead
(higher capacity and lower internal resistance).

---

## Turret (Fan Base)

| # | Component | Qty | Specification | Est. Cost | Notes |
|---|-----------|-----|---------------|-----------|-------|
| 1 | Fan Unit | 1 | Honeywell HT-900 | ~$15.00 | Donor fan; AC powered (120 V), manual speed control retained |
| 2 | Controller | 1 | ESP32 DevKit v1 | ~$6.00 | Processes sensors, drives servos, serial debug; WiFi/BLE for future expansion |
| 3 | IR Sensors | 4 | TSOP38238 (38 kHz) | ~$2.00 | Must match beacon carrier frequency |
| 4 | Sensor Bypass Caps | 4 | 100 nF ceramic | <$0.20 | **Required.** One per TSOP38238, across VCC–GND close to each sensor. Per datasheet recommendation. |
| 5 | Pan Servo | 1 | MG996R (continuous 360°) | ~$9.00 | Continuous rotation; drives ring gear via pinion |
| 6 | Tilt Servo | 1 | MG996R (standard 180°) | ~$9.00 | Standard positional; 0–45° range |
| 7 | Bearing | 1 | 4-inch lazy susan (metal) | ~$5.00 | Ball-bearing turntable; isolates fan weight from servo |
| 8 | Power Supply | 1 | 5 V 4 A DC adapter | ~$12.00 | Barrel jack; **do NOT use phone chargers** |
| 9 | DC Connector | 1 | Female DC barrel pigtail | ~$1.00 | Barrel jack to bare-wire for breadboard |
| 10 | Level Shifter (recommended) | 1 | 3.3 V ↔ 5 V bi-directional (2-ch) | ~$1.00 | For pan + tilt servo signal lines. ESP32 GPIO is 3.3 V; MG996R expects 5 V signal. Most servos tolerate 3.3 V, but a level shifter eliminates jitter risk. |

**Turret subtotal: ~$60.20**

---

## Consumables & Hardware (not itemised above)

| Item | Notes |
|------|-------|
| Breadboard + jumper wires | For prototyping; solder to perfboard for permanent build |
| 3D printing filament (PLA/PETG) | Ring gear, pinion, tilt bracket, servo coupler, sensor cross |
| M3 bolts, nuts, standoffs | Mounting lazy susan, servo brackets |
| Zip ties / wire sleeve | Cable management through rotating platform |
| Solder + flux | Assembly |
| Heat-shrink tubing | Insulate joints, especially near AC wiring |
| ISP programmer (USBasp or Arduino-as-ISP) | Required for ATtiny85 — see PROGRAMMING.md |

---

## Total Estimated Cost

| Subsystem | Cost |
|-----------|------|
| Beacon | ~$14.50 |
| Turret | ~$60.20 |
| **Total** | **~$75–$85** |

*Excludes filament, fasteners, wire, solder, and ISP programmer.*
