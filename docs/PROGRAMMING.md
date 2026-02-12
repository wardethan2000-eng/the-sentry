# Programming Guide — The Sentry

How to set up PlatformIO and flash both subsystems.

---

## Prerequisites

1. **PlatformIO** — Install as a [VS Code extension](https://platformio.org/install/ide?install=vscode) or as the [CLI](https://docs.platformio.org/en/latest/core/installation/index.html).
2. **ISP Programmer** for the ATtiny85 beacon (one of):
   - **USBasp** — a cheap dedicated AVR programmer (~$3). Recommended.
   - **Arduino-as-ISP** — use a spare Arduino Uno/Nano running the ArduinoISP sketch.
3. **USB cable** for the ESP32 DevKit turret (Micro-USB or USB-C depending on board revision).

---

## 1. Flashing the Beacon (ATtiny85)

### Option A: USBasp Programmer

**Wiring (USBasp → ATtiny85 DIP-8):**

| USBasp Pin | ATtiny85 Pin | Function |
|------------|-------------|----------|
| MOSI       | Pin 5 (PB0) | Data In  |
| MISO       | Pin 6 (PB1) | Data Out |
| SCK        | Pin 7 (PB2) | Clock    |
| RESET      | Pin 1       | Reset    |
| VCC        | Pin 8       | +5 V     |
| GND        | Pin 4       | Ground   |

**Flash:**
```bash
cd beacon
pio run --target upload
```

PlatformIO will auto-detect the USBasp. If you get a permissions error on Windows, install the [Zadig](https://zadig.akeo.ie/) WinUSB driver for the USBasp device.

### Option B: Arduino-as-ISP

1. Open Arduino IDE → File → Examples → 11.ArduinoISP → ArduinoISP.
2. Upload the ArduinoISP sketch to your Uno/Nano.
3. Wire the Uno to the ATtiny85:

| Uno Pin | ATtiny85 Pin | Function |
|---------|-------------|----------|
| D11     | Pin 5 (PB0) | MOSI     |
| D12     | Pin 6 (PB1) | MISO     |
| D13     | Pin 7 (PB2) | SCK      |
| D10     | Pin 1       | RESET    |
| 5V      | Pin 8       | VCC      |
| GND     | Pin 4       | GND      |

4. Add a 10 µF capacitor between the Uno's RESET and GND (prevents auto-reset).

5. Edit `beacon/platformio.ini` — uncomment the Arduino-as-ISP section:
```ini
upload_protocol = stk500v1
upload_flags    = -b19200
upload_port     = COM3          ; ← set your port
```

6. Flash:
```bash
cd beacon
pio run --target upload
```

### Setting Fuses (first time only)

The ATtiny85 ships with a 1 MHz clock divider enabled. You need to burn fuses once to switch to 8 MHz internal:

```bash
cd beacon
pio run --target fuses
```

This writes LFUSE=0xE2, HFUSE=0xDF, EFUSE=0xFF (8 MHz internal, BOD disabled).

---

## 2. Flashing the Turret (ESP32)

Connect the ESP32 via USB and run:

```bash
cd turret
pio run --target upload
```

PlatformIO auto-detects the serial port. If multiple devices are connected, specify the port:

```bash
pio run --target upload --upload-port COM5
```

### Serial Monitor

Open the serial monitor to see debug output (sensor readings, state, servo positions):

```bash
pio device monitor --baud 115200
```

---

## 3. Verifying the Beacon Output

Before assembling the full system, verify the beacon works in isolation:

### Method 1: Oscilloscope
- Probe PB0 (pin 5). You should see a 38 kHz square wave during burst periods.
- Burst duration ≈ 600 µs, gap ≈ 600 µs, 5 bursts per cycle, ~120 ms between cycles.

### Method 2: TSOP38238 + Arduino
1. Wire a TSOP38238: VCC → 5 V, GND → GND, OUT → Arduino pin 2.
2. Upload a simple sketch that reads pin 2 and prints to Serial when LOW is detected.
3. Point the beacon at the sensor from various distances.
4. Expected range with TSAL6200 LEDs: 5–8 m line-of-sight.

### Method 3: Phone Camera
- Most phone cameras can "see" 940 nm IR as a faint purple glow.
- Aim the beacon at your phone's front camera (less filtered than rear cameras).
- You should see the LEDs flashing. This confirms output but not frequency.

---

## 4. Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| `avrdude: error: could not find USBasp` | Driver issue on Windows | Install WinUSB via Zadig |
| `avrdude: Device signature = 0x000000` | Wiring error or no power | Check all ISP connections, ensure VCC is present |
| Beacon LEDs don't light | Wrong pin, dead LED, or transistor wired backwards | Test LED with a coin cell + resistor directly; verify 2N2222 pinout (EBC) |
| No serial output from turret | Wrong baud rate or COM port | Verify `monitor_speed = 115200` in platformio.ini; check Device Manager for port |
| ESP32 won't flash | Boot mode not entered | Hold BOOT button while clicking EN/RST, then release; some DevKit boards need this |
| Servo jitters at startup | Power supply inadequate | Use the 5 V 4 A adapter, not USB power |
