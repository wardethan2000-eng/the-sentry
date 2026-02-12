# Build Guide — The Sentry

Step-by-step assembly, wiring, and tuning instructions.

---

## Build Order

Complete each phase and verify before moving to the next.

1. **Beacon clip** — build, program, and test in isolation.
2. **Turret mechanical platform** — base plate, lazy susan, gear train.
3. **Tilt mechanism** — servo bracket, fan head coupler.
4. **Sensor cross (Blinder)** — mount TSOP38238 receivers.
5. **Electrical wiring** — breadboard power bus, servo connections, sensor wiring.
6. **Firmware flash & tuning** — upload code, calibrate constants.
7. **Integration test** — all systems together in the target room.

---

## Phase 1: Beacon Clip

### Assembly
1. Solder the 8-pin DIP socket onto a small piece of perfboard or directly into the CR2032 battery holder PCB.
2. Solder the 2N2222A transistor:
   - **Emitter** → GND rail
   - **Collector** → anode of both TSAL6200 LEDs (through individual 22 Ω resistors)
   - **Base** → 1 kΩ resistor → socket pin 5 (PB0)
3. Wire both LED cathodes to Collector-side of the resistors. Each LED has its own 22 Ω resistor in series, then they share the collector.
4. Connect battery holder: **+** → socket pin 8 (VCC), **−** → socket pin 4 (GND).
5. Insert the programmed ATtiny85 into the socket (notch/dot toward pin 1).

### Schematic (text)
```
LIR2032 (+) ─── VCC (pin 8)
LIR2032 (−) ─── GND (pin 4) ─── 2N2222 Emitter

PB0 (pin 5) ─── 1kΩ ─── 2N2222 Base

2N2222 Collector ─┬── 22Ω ── TSAL6200 (A) anode → cathode → VCC
                  └── 22Ω ── TSAL6200 (B) anode → cathode → VCC
```
*Note: LEDs are wired "high-side" with anodes toward VCC and cathodes pulled low through the transistor. Alternatively, wire low-side (cathodes to collector, anodes through resistors to VCC) — either works.*

### Test
See [PROGRAMMING.md](PROGRAMMING.md) § "Verifying the Beacon Output".

---

## Phase 2: Pan Axis Mechanical Platform

### Parts needed
- Wood or 3D-printed base plate (~200 mm square, 6 mm thick)
- 4-inch lazy susan bearing
- 3D-printed ring gear (100–120 mm diameter, module 1.0–1.5)
- 3D-printed pinion gear (to fit MG996R spline, ~15 teeth)
- MG996R continuous-rotation servo + mounting screws

### Assembly
1. **Base plate**: Drill 4 mounting holes matching the lazy susan's bolt pattern. Bolt the **bottom ring** of the lazy susan to the base plate.
2. **Ring gear**: Bolt to the **top ring** of the lazy susan. The fan's U-stand will later attach to this gear.
3. **Servo mount**: Attach the MG996R to the base plate using a 3D-printed bracket, offset so the pinion gear meshes with the ring gear.
4. **Pinion gear**: Press-fit or screw the pinion onto the servo spline.
5. **Test mesh**: Manually rotate the ring gear and confirm smooth engagement. Adjust spacing until there is minimal backlash but no binding.

### 3D Printing Tips (Issue #5)
- **Layer height**: 0.12 mm for gear teeth.
- **Infill**: 80%+ for the pinion (high stress), 40% for the ring gear.
- **Material**: PETG preferred (less brittle than PLA under continuous stress). PLA is fine for prototyping.
- **Tooth profile**: Herringbone ("double helical") reduces backlash and axial play. Standard involute spur gears also work but with more play.
- **Test fit**: Print a small test section of 3–4 teeth first to verify mesh before printing the full ring gear.

---

## Phase 3: Tilt Mechanism

### Parts needed
- 3D-printed servo bracket (secures to the fan's U-stand vertical leg)
- 3D-printed coupler (connects servo spline to fan head pivot shaft)
- MG996R standard 180° servo

### Assembly
1. Remove one of the HT-900's plastic friction-pivot knobs.
2. Attach the servo bracket to the vertical leg of the U-stand (screws or zip ties).
3. Mount the MG996R into the bracket.
4. Press the coupler onto the servo spline, then insert the coupler into the fan head's pivot hole.
5. Verify the servo can sweep 0–45° without binding and that the fan head tilts smoothly.

---

## Phase 4: Sensor Cross (Blinder)

### Parts needed
- 3D-printed cross-shaped housing (4 quadrants with divider walls)
- 4× TSOP38238 IR receivers

### Design requirements (Issue #6)
- **Wall height**: ≥ 25 mm from sensor face to wall top.
- **Wall material**: Matte black (paint or use black filament). Avoid glossy surfaces that reflect IR.
- **Mounting**: Center of the fan grille, or offset slightly forward. Zip-tie or clip to the grille.

### Assembly
1. Insert each TSOP38238 into its quadrant (Top, Bottom, Left, Right).
2. Route the 3-pin wires (VCC, GND, OUT) back through the housing.
3. Add a 100 nF ceramic capacitor across VCC–GND of each sensor (close to the sensor) for power decoupling. The TSOP38238 datasheet recommends this.
4. Mount the cross on the fan grille using zip ties, glue, or a clip bracket.

---

## Phase 5: Electrical Wiring

### Power Bus
```
5V 4A Adapter ──→ DC barrel pigtail ──→ Breadboard (+) and (−) rails

(+) rail ──→ Arduino Nano 5V pin
(+) rail ──→ Pan servo VCC (red)
(+) rail ──→ Tilt servo VCC (red)
(+) rail ──→ All 4× TSOP38238 VCC

(−) rail ──→ Arduino Nano GND
(−) rail ──→ Pan servo GND (brown/black)
(−) rail ──→ Tilt servo GND (brown/black)
(−) rail ──→ All 4× TSOP38238 GND
```

> **WARNING**: Do NOT power the servos from the Arduino's 5V pin or USB port.
> The MG996R can draw up to 2 A under stall — this will destroy the Nano's
> voltage regulator. Always use the 5 V 4 A adapter on a shared bus.

### Signal Wiring
| Wire | From | To |
|------|------|----|
| Pan servo signal (orange/white) | Servo | Arduino D9 |
| Tilt servo signal (orange/white) | Servo | Arduino D10 |
| Sensor Top OUT | TSOP38238 | Arduino D2 |
| Sensor Bottom OUT | TSOP38238 | Arduino D3 |
| Sensor Left OUT | TSOP38238 | Arduino D4 |
| Sensor Right OUT | TSOP38238 | Arduino D5 |

### Cable Management (Issue #4 + Issue #7)
- Route all cables (sensor wires, servo wires, fan AC cord) through the **center hole of the lazy susan**.
- Leave a **30 cm slack loop** coiled below the top plate to accommodate ±135° rotation.
- Bundle DC and AC wiring in **separate sleeves** (split loom or spiral wrap).
- Secure cables to the rotating platform with zip ties so they don't catch on the gear train.

> The fan's AC power cord remains intact. It runs alongside the 5 V DC wiring
> through the lazy susan center opening. The fan's manual speed knob is retained
> as-is — no relay or MOSFET control in this version.

---

## Phase 6: Firmware Flash & Tuning

1. Flash the beacon — see [PROGRAMMING.md](PROGRAMMING.md).
2. Flash the turret — see [PROGRAMMING.md](PROGRAMMING.md).
3. Open the serial monitor (`pio device monitor --baud 115200`).
4. Point the beacon at the sensor cross from ~2 m distance.
5. Verify serial output shows individual sensors toggling as you move the beacon around.

### Tuning Constants (in `turret/include/config.h`)

| Constant | Default | What to adjust |
|----------|---------|----------------|
| `PAN_STOP_US` | 1500 | If the pan servo creeps when it should be stopped, adjust ±10 µs. |
| `PAN_DEG_PER_SEC` | 60.0 | Measure empirically: command full speed, time a known rotation. |
| `SENSOR_FILTER_THRESHOLD` | 6 | Lower (e.g., 4) = more responsive but more false triggers. Higher = more latency. |
| `TRACK_PAN_SPEED_FAST` | 0.80 | Reduce if the fan overshoots. Increase if it's sluggish. |
| `TRACK_PAN_SPEED_SLOW` | 0.30 | Fine approach speed. Lower = smoother but slower convergence. |
| `TILT_HOLDOFF_MS` | 100 | Increase if tilt oscillates; decrease for faster vertical response. |
| `SIGNAL_LOSS_SEARCH_MS` | 3000 | Time before sweep starts. Shorter = more aggressive search. |

---

## Phase 7: Integration Test

1. Power everything up (5 V adapter for turret, LIR2032 for beacon).
2. Stand 3–5 m from the fan. The serial monitor should show `State=TRACK`.
3. Walk slowly left/right — the fan should pan to follow.
4. Raise/lower the beacon — the fan should tilt to follow.
5. Walk behind a wall or turn off the beacon. After 3 s the fan should enter `SEARCH` (slow sweep). After 15 s it should `PARK` at home.
6. Return to line-of-sight — the fan should immediately resume tracking.

### Common Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Fan oscillates rapidly left-right | Backlash + no dead band | Increase `PAN_MIN_SPEED`; verify gear mesh |
| Fan points wrong direction | LEFT/RIGHT sensor wires swapped | Swap D4 ↔ D5 (or swap in `config.h`) |
| Tilt jitters | Rate limit too fast | Increase `TILT_HOLDOFF_MS` to 200 |
| Works close but not across room | Beacon range too short | Verify TSAL6200 LEDs, check resistor values, extend blinder walls |
| False triggers in sunlit room | Ambient IR overwhelming AGC | Close blinds, extend blinder walls to 35 mm, reduce `SENSOR_FILTER_THRESHOLD` |
| Sweeps forever (never locks on) | Beacon dead or out of range | Check beacon battery voltage (should be >3.2 V); test with phone camera |
