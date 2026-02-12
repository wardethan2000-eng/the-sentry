# The Sentry — Auto-Tracking Fan System

A 2-axis (pan/tilt) tracking fan that follows a person around a room using infrared light.

## How It Works

The user wears a small rechargeable **IR beacon clip** that continuously broadcasts
a modulated 940nm infrared signal. A stationary **fan base (turret)** equipped with
four directional IR sensors detects the signal and drives pan/tilt servos to keep
airflow aimed at the user.

## Repository Structure

```
the-sentry/
├── beacon/          ATtiny85 wearable IR beacon (the Clip)
├── turret/          ESP32 motorized fan base (the Turret)
└── docs/            Build guide, BOM, programming instructions
```

## Subsystems

### Beacon (Clip)
- **MCU:** ATtiny85 @ 8 MHz internal oscillator
- **Output:** 2× TSAL6200 940nm IR LEDs pulsed at 38 kHz
- **Power:** LIR2032 rechargeable coin cell (3.6 V)
- **Runtime:** ~6.5 hours with aggressive duty cycling

### Turret (Fan Base)
- **MCU:** ESP32 DevKit v1 (development) / Arduino Nano (production)
- **Sensors:** 4× TSOP38238 (38 kHz demodulating IR receivers)
- **Pan:** MG996R continuous-rotation servo with ring-gear / lazy-susan drivetrain
- **Tilt:** MG996R standard 180° servo coupled to the fan head pivot
- **Power:** 5 V 4 A DC wall adapter (barrel jack)
- **Fan:** Honeywell HT-900 (mains AC, manual speed control retained)

## Quick Start

### Prerequisites
- [PlatformIO CLI or VS Code extension](https://platformio.org/install)
- ISP programmer for ATtiny85 (USBasp **or** Arduino-as-ISP)

### Build & Flash — Beacon
```bash
cd beacon
pio run                   # compile
pio run --target upload   # flash via ISP programmer
```

### Build & Flash — Turret
```bash
cd turret
pio run                   # compile
pio run --target upload   # flash via USB
```

## Estimated Cost
$73–$85 for all electronics (excluding filament, fasteners, wire, solder).

## Documentation
- [Build Guide](docs/BUILD_GUIDE.md) — step-by-step assembly & wiring
- [Bill of Materials](docs/BOM.md) — corrected component list with issue fixes
- [Programming Guide](docs/PROGRAMMING.md) — ATtiny85 ISP setup & PlatformIO usage

## License
This project is open-source hardware and software. See individual files for details.
